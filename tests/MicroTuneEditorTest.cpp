#include <CoreFoundation/CoreFoundation.h>
#include "PluginEditor.h"
#include "BSIIMicroTunePage.h"
#include "BSIIScalaImportPanel.h"
#include "BSIIPatchState.h"
#include <iostream>

namespace
{
void pump (int ms = 150) { CFRunLoopRunInMode (kCFRunLoopDefaultMode, ms / 1000.0, false); }
int failures = 0;
void check (bool ok, const char* label)
{
    if (! ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
struct Capture : juce::MidiInputCallback
{
    std::atomic<int> messages { 0 };
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override { ++messages; }
};
struct Listener : juce::AudioProcessorListener
{
    int parameters = 0, localChanges = 0;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override { ++parameters; }
    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails& change) override
    { if (change.nonParameterStateChanged) ++localChanges; }
};
juce::TableListBox* findTable (juce::Component& component)
{
    if (auto* table = dynamic_cast<juce::TableListBox*> (&component)) return table;
    for (auto* child : component.getChildren()) if (auto* table = findTable (*child)) return table;
    return nullptr;
}
MicroTunePage* showPage (juce::AudioProcessorEditor& editor)
{
    for (auto* child : editor.getChildren())
        if (auto* tabs = dynamic_cast<juce::TabbedComponent*> (child))
        {
            const auto index = tabs->getTabNames().indexOf ("MicroTune");
            if (index < 0) return nullptr;
            tabs->setCurrentTabIndex (index);
            pump();
            return dynamic_cast<MicroTunePage*> (tabs->getTabContentComponent (index));
        }
    return nullptr;
}
bool edit (juce::TableListBox& table, int key, const juce::String& text)
{
    table.selectRow (key);
    table.scrollToEnsureRowIsOnscreen (key);
    table.updateContent();
    pump();
    auto* cell = dynamic_cast<juce::Label*> (table.getCellComponent (3, key));
    if (! cell) return false;
    cell->setText (text, juce::sendNotificationSync);
    pump();
    return true;
}
template <typename T, typename Predicate>
T* findComponent (juce::Component& parent, Predicate predicate)
{
    if (auto* match = dynamic_cast<T*> (&parent); match && predicate (*match)) return match;
    for (auto* child : parent.getChildren())
        if (auto* match = findComponent<T> (*child, predicate)) return match;
    return nullptr;
}
bool click (juce::Component& page, const juce::String& text)
{
    auto* button = findComponent<juce::TextButton> (page, [&] (auto& b) { return b.getButtonText() == text; });
    if (! button || ! button->isEnabled()) return false;
    button->triggerClick(); pump();
    return true;
}

bool picture (juce::Component& component, const juce::File& path)
{
    const auto image = component.createComponentSnapshot (component.getLocalBounds());
    juce::FileOutputStream output (path);
    return output.openedOk() && juce::PNGImageFormat().writeImageToStream (image, output);
}
}

int main (int argc, char** argv)
{
    if (argc != 2) return 2;
    juce::ScopedJuceInitialiser_GUI initialise;
    const juce::File report (argv[1]);
    if (! report.createDirectory()) return 2;
    Capture capture;
    const auto name = "BSII Local Tuning Test " + juce::Uuid().toString();
    auto sink = juce::MidiInput::createNewDevice (name, &capture);
    if (! sink) return 2;
    sink->start();
    juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
    BSIIControlAudioProcessor processor;
    juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Undefined);
    for (int n = 0; n < 20 && ! processor.hardwareMidiOutput(); ++n)
    {
        for (const auto& port : juce::MidiOutput::getAvailableDevices())
            if (port.name == name) processor.hardwareMidiOutput() = juce::MidiOutput::openDevice (port.identifier);
        if (! processor.hardwareMidiOutput()) pump (50);
    }
    if (! processor.hardwareMidiOutput()) return 2;
    Listener listener;
    processor.addListener (&listener);
    const auto parametersBefore = processor.getParameters().copyState();
    juce::MemoryBlock originalState;
    processor.getStateInformation (originalState);
    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
    editor->setVisible (true);
    auto* page = showPage (*editor);
    auto* table = page ? findTable (*page) : nullptr;
    check (page && table && table->getNumRows() == 128, "MicroTune tab and all 128 rows");
    if (! page || ! table) return 1;
    check (edit (*table,69,"+50"), "editable cents cell");
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69.5, "UI edits actual working table");
    check (click (*page,"Undo") && processor.getLocalTuningInfo().document.table.pitch (69) == 69,
           "UI undo restores single note");
    check (click (*page,"Redo") && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "UI redo reapplies note");
    auto* bulk = findComponent<juce::TextEditor> (*page, [] (auto& e) { return e.getComponentID() == "bulkCents"; });
    check (bulk != nullptr, "bulk cents input available");
    if (bulk)
    {
        table->deselectAllRows(); table->selectRangeOfRows (60,61); bulk->setText ("25",false);
        check (click (*page,"Set Offset") && processor.getLocalTuningInfo().document.table.pitch (60) == 60.25
               && processor.getLocalTuningInfo().document.table.pitch (61) == 61.25, "bulk set from actual selection");
        check (click (*page,"Add Cents") && processor.getLocalTuningInfo().document.table.pitch (60) == 60.5,
               "bulk add uses current pitch");
        check (click (*page,"Undo") && processor.getLocalTuningInfo().document.table.pitch (60) == 60.25
               && processor.getLocalTuningInfo().document.table.pitch (61) == 61.25, "whole bulk operation is one undo");
        check (click (*page,"Redo") && processor.getLocalTuningInfo().document.table.pitch (61) == 61.5,
               "whole bulk operation redoes");
        check (click (*page,"Reset Notes") && processor.getLocalTuningInfo().document.table.pitch (60) == 60,
               "bulk reset from UI");
        table->deselectAllRows(); table->selectRow (0); table->selectRow (60,false,false); bulk->setText ("-50",false);
        const auto before = processor.getLocalTuningInfo().revision;
        check (click (*page,"Set Offset") && processor.getLocalTuningInfo().revision == before
               && processor.getLocalTuningInfo().document.table.pitch (60) == 60, "invalid bulk edit is atomic and adds no history");
        bulk->setText ("not a number",false);
        check (click (*page,"Add Cents") && processor.getLocalTuningInfo().revision == before, "invalid bulk text rejected");
        check (click (*page,"Select All") && table->getNumSelectedRows() == 128, "all notes selectable");
        table->deselectAllRows();
        bulk->setText ("0",false);
    }
    check (edit (*table,69,"not a number") && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "invalid text keeps table");
    check (edit (*table,127,"+50") && processor.getLocalTuningInfo().document.table.pitch (127) == 127,
           "boundary edit rejected");
    check (edit (*table,60,"-50") && processor.getLocalTuningInfo().document.table.pitch (60) == 59.5,
           "negative offset from UI");
    const auto savedFile = report.getChildFile ("working.json");
    check (page->saveDocumentFile (savedFile) && ! processor.getLocalTuningInfo().unsavedFile, "save local document");
    check (processor.undoLocalTuning() && processor.getLocalTuningInfo().unsavedFile, "undo after save marks changed content unsaved");
    check (processor.redoLocalTuning() && ! processor.getLocalTuningInfo().unsavedFile, "redo to saved document restores clean file status");
    check (edit (*table,69,"0") && processor.getLocalTuningInfo().document.table.pitch (69) == 69, "independent second edit");
    check (page->loadDocumentFile (savedFile) && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "load local document");
    const auto badFile = report.getChildFile ("invalid.json");
    badFile.replaceWithText ("{}");
    check (! page->loadDocumentFile (badFile) && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "failed load preserves current document");
    check (! page->saveDocumentFile (report.getChildFile ("missing-directory/output.json")), "failed save reported");
    check (parametersBefore.isEquivalentTo (processor.getParameters().copyState()), "no APVTS parameter changes");
    check (listener.parameters == 0 && listener.localChanges > 0, "host sees document changes, not automation gestures");
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    check (xml && xml->hasAttribute ("bsiiLocalTuningDocument"), "host state includes separate local document");
    check (! bsii::patchOnlyState (processor.getParameters().copyState()).hasProperty ("bsiiLocalTuningDocument"),
           "patch snapshot state has no local table");
    table->scrollToEnsureRowIsOnscreen (60); pump();
    check (picture (*editor,report.getChildFile ("microtune-default.png")), "default-size page render");
    editor->setSize (760,600); pump();
    check (picture (*editor,report.getChildFile ("microtune-small.png")), "minimum-size page render");
    for (auto* child : page->getChildren())
        if (auto* viewport = dynamic_cast<juce::Viewport*> (child)) viewport->setViewPosition (0,10000);
    pump();
    check (picture (*editor,report.getChildFile ("microtune-small-notes.png")), "minimum-size notes reachable by scrolling");
    {
        BSIIControlAudioProcessor fresh;
        fresh.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        check (fresh.getLocalTuningInfo().document.table.pitch (69) == 69.5
               && fresh.getLocalTuningInfo().document.table.pitch (60) == 59.5,
               "document restored in a new processor instance");
    }
    editor.reset();
    editor.reset (processor.createEditor());
    editor->setVisible (true);
    page = showPage (*editor);
    table = page ? findTable (*page) : nullptr;
    check (table && processor.getLocalTuningInfo().document.table.pitch (60) == 59.5,
           "working table survives editor recreation");
    check (processor.getLocalTuningInfo().canUndo, "history survives editor recreation");
    processor.setStateInformation (originalState.getData(), static_cast<int> (originalState.getSize())); pump();
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "restore identity document");
    check (! processor.getLocalTuningInfo().canUndo && ! processor.getLocalTuningInfo().canRedo, "host restore starts fresh history");
    processor.setStateInformation (state.getData(), static_cast<int> (state.getSize())); pump();
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69.5
           && ! processor.getParameters().state.hasProperty ("bsiiLocalTuningDocument"), "restore strips metadata from APVTS");
    if (xml)
    {
        xml->setAttribute ("bsiiLocalTuningDocument", "{}");
        juce::MemoryBlock malformed;
        juce::AudioProcessor::copyXmlToBinary (*xml, malformed);
        processor.setStateInformation (malformed.getData(), static_cast<int> (malformed.getSize())); pump();
        check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "malformed session document safely resets");
        xml->removeAttribute ("bsiiLocalTuningDocument");
        juce::AudioProcessor::copyXmlToBinary (*xml, malformed);
        processor.setStateInformation (malformed.getData(), static_cast<int> (malformed.getSize())); pump();
        check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "old session without metadata loads");
    }
    {
        BSIIControlAudioProcessor history;
        auto document = history.getLocalTuningInfo().document;
        for (int i = 1; i <= 70; ++i)
        {
            document.table.setCents (60, i);
            history.replaceLocalTuningDocument (document);
        }
        int count = 0;
        while (history.undoLocalTuning()) ++count;
        check (count == 64, "history bounded to latest 64 changes");
        check (history.redoLocalTuning(), "redo available after undo");
        document = history.getLocalTuningInfo().document;
        history.replaceLocalTuningDocument (document);
        check (history.getLocalTuningInfo().canRedo, "no-op does not discard redo");
        document.name = "New branch";
        history.replaceLocalTuningDocument (document);
        check (! history.getLocalTuningInfo().canRedo, "new document edit discards redo branch");
        check (history.undoLocalTuning() && history.getLocalTuningInfo().document.name != "New branch",
               "metadata changes included in history");
    }
    {
        auto before = processor.getLocalTuningInfo().document;
        before.table.setCents (60,35); processor.replaceLocalTuningDocument (before);
        const auto scaleFile = report.getChildFile ("own-scale.scl");
        scaleFile.replaceWithText ("! test fixture\nTwelve equal steps\n12\n100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n2/1\n");
        ScalaImportPanel panel (processor);
        panel.setVisible (true);
        check (panel.loadScaleFile (scaleFile), "Scala file selection reads without applying");
        auto* hz = findComponent<juce::TextEditor> (panel, [] (auto& e) { return e.getComponentID() == "scalaHz"; });
        auto* keep = findComponent<juce::ToggleButton> (panel, [] (auto&) { return true; });
        check (hz && keep, "Scala reference and preservation controls");
        if (hz && keep)
        {
            hz->setText ("432",true); pump(); panel.rebuildPreview();
            const auto revision = processor.getLocalTuningInfo().revision;
            check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == revision, "preview does not mutate and incomplete import blocks by default");
            check (picture (panel,report.getChildFile ("scala-preview.png")), "Scala preview render");
            keep->setToggleState (true,juce::dontSendNotification); panel.rebuildPreview();
            check (click (panel,"Apply to Local Table") && processor.getLocalTuningInfo().document.table.pitch (0) == before.table.pitch (0)
                   && std::abs (*processor.getLocalTuningInfo().document.table.pitch (69) - (69+12*std::log2 (432.0/440))) < 1.0e-9,
                   "explicit keep applies valid notes atomically from preview");
            check (processor.undoLocalTuning() && processor.getLocalTuningInfo().document.table.pitch (60) == before.table.pitch (60), "Scala import is one undo step");
            check (processor.redoLocalTuning(), "Scala import can redo");
            const auto encoded = bsii::microtuning::serialise (processor.getLocalTuningInfo().document);
            const auto roundTrip = encoded ? bsii::microtuning::parseDocument (*encoded) : std::nullopt;
            check (roundTrip && roundTrip->table.pitch (69) == processor.getLocalTuningInfo().document.table.pitch (69), "Scala result round trips local JSON");
            panel.rebuildPreview();
            hz->setText ("433",true);
            const auto beforePendingText = processor.getLocalTuningInfo().revision;
            check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == beforePendingText,
                   "pending text notification cannot apply an older reference preview");
            pump(); panel.rebuildPreview();
            auto changed = processor.getLocalTuningInfo().document; changed.name = "Changed since preview";
            processor.replaceLocalTuningDocument (changed);
            check (! panel.applyPreview() && processor.getLocalTuningInfo().document.name == changed.name, "stale preview cannot replace newer document");
        }
        const auto mapping = report.getChildFile ("own-mapping.kbm");
        mapping.replaceWithText ("0\n0\n127\n60\n69\n440\n0\n");
        check (panel.loadMappingFile (mapping) && hz && ! hz->isEnabled(), "KBM reference replaces manual reference controls");
        panel.rebuildPreview();
        check (panel.applyPreview() && std::abs (*processor.getLocalTuningInfo().document.table.pitch (69) - 69) < 1.0e-9,
               "KBM import uses file reference frequency");
        const auto bad = report.getChildFile ("bad-scale.scl"); bad.replaceWithText ("bad");
        check (panel.loadScaleFile (bad), "malformed selection still requires parse");
        const auto revision = processor.getLocalTuningInfo().revision;
        panel.rebuildPreview();
        check (! panel.applyPreview() && processor.getLocalTuningInfo().revision == revision, "bad scale keeps current table");
    }
    check (processor.getParameters().getParameter ("paraphonic")->getParameterIndex() == 94
           && static_cast<juce::AudioProcessor&> (processor).getParameters().size() == 95, "existing automation layout retained");
    check (listener.parameters == 0, "Scala imports do not change automation parameters");
    check (click (*page,"Import Scala..."), "MicroTune opens actual import dialog");
    juce::Component::SafePointer<juce::DialogWindow> importWindow;
    for (int i = 0; i < juce::Desktop::getInstance().getNumComponents(); ++i)
        if (auto* dialog = dynamic_cast<juce::DialogWindow*> (juce::Desktop::getInstance().getComponent (i));
            dialog && dialog->getName() == "Import Scala - local preview") importWindow = dialog;
    check (importWindow != nullptr, "import dialog present");
    editor.reset(); pump();
    check (importWindow == nullptr, "closing editor destroys import session safely");
    pump (300);
    check (capture.messages.load() == 0, "edit/load/save/restore emits no MIDI");
    processor.removeListener (&listener);
    editor.reset(); sink->stop();
    std::cout << (failures == 0 ? "PASS" : "FAIL") << " MicroTune editor, local files, session restore and MIDI isolation ("
              << failures << " failures)\n";
    return failures == 0 ? 0 : 1;
}
