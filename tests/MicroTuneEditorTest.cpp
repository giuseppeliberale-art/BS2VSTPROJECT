#include <CoreFoundation/CoreFoundation.h>
#include "PluginEditor.h"
#include "BSIIMicroTunePage.h"
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
    check (edit (*table,69,"not a number") && processor.getLocalTuningInfo().document.table.pitch (69) == 69.5,
           "invalid text keeps table");
    check (edit (*table,127,"+50") && processor.getLocalTuningInfo().document.table.pitch (127) == 127,
           "boundary edit rejected");
    check (edit (*table,60,"-50") && processor.getLocalTuningInfo().document.table.pitch (60) == 59.5,
           "negative offset from UI");
    const auto savedFile = report.getChildFile ("working.json");
    check (page->saveDocumentFile (savedFile) && ! processor.getLocalTuningInfo().unsavedFile, "save local document");
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
        if (auto* viewport = dynamic_cast<juce::Viewport*> (child)) viewport->setViewPosition (0,120);
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
    processor.setStateInformation (originalState.getData(), static_cast<int> (originalState.getSize())); pump();
    check (processor.getLocalTuningInfo().document.table.pitch (69) == 69, "restore identity document");
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
    check (processor.getParameters().getParameter ("paraphonic")->getParameterIndex() == 94
           && static_cast<juce::AudioProcessor&> (processor).getParameters().size() == 95, "existing automation layout retained");
    pump (300);
    check (capture.messages.load() == 0, "edit/load/save/restore emits no MIDI");
    processor.removeListener (&listener);
    editor.reset(); sink->stop();
    std::cout << (failures == 0 ? "PASS" : "FAIL") << " MicroTune editor, local files, session restore and MIDI isolation ("
              << failures << " failures)\n";
    return failures == 0 ? 0 : 1;
}
