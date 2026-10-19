#include "BSIIMicroTunePage.h"
#include <cstdlib>

namespace
{
juce::String centsText (double pitch, int key)
{
    const auto cents = (pitch - key) * 100.0;
    return (cents > 0 ? "+" : "") + juce::String (cents, 4);
}
}

class MicroTunePage::CentsCell : public juce::Label
{
public:
    explicit CentsCell (MicroTunePage& p) : page (p)
    {
        setEditable (false, true, false);
        setJustificationType (juce::Justification::centredLeft);
        setTooltip ("Double-click to enter cents above or below this key's normal pitch. Nothing is sent to the synth.");
        onTextChange = [this] { page.editCents (row, getText()); };
    }
    void display (int key, double pitch)
    {
        row = key;
        if (! isBeingEdited()) setText (centsText (pitch, key), juce::dontSendNotification);
    }
private:
    MicroTunePage& page;
    int row = 0;
};

MicroTunePage::MicroTunePage (BSIIControlAudioProcessor& p) : processor (p)
{
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
    for (auto* component : std::initializer_list<juce::Component*> { &heading, &hint, &nameLabel,
            &nameEditor, &targetLabel, &target, &status, &newButton, &loadButton, &saveButton, &table })
        content.addAndMakeVisible (component);
    heading.setText ("MicroTune - local table", juce::dontSendNotification);
    heading.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    hint.setText ("Edit locally. Nothing is sent to the synth. Target Hz assumes A4 = 440 Hz.", juce::dontSendNotification);
    hint.setColour (juce::Label::textColourId, juce::Colours::lightblue);
    nameLabel.setText ("Name", juce::dontSendNotification);
    nameEditor.setInputRestrictions (128);
    nameEditor.onReturnKey = [this] { commitName(); };
    nameEditor.onFocusLost = [this] { commitName(); };
    targetLabel.setText ("File target", juce::dontSendNotification);
    for (int id = 0; id <= 8; ++id) target.addItem (juce::String (id), id + 1);
    target.setTooltip ("Destination stored in this local file. Does not select or change the synth's active table.");
    target.onChange = [this]
    {
        auto document = processor.getLocalTuningInfo().document;
        document.targetTable = target.getSelectedId() - 1;
        processor.replaceLocalTuningDocument (document);
        refresh();
    };
    newButton.onClick = [this] { confirmReplace ([this] { newTable(); }); };
    loadButton.onClick = [this] { confirmReplace ([this] { chooseLoad(); }); };
    saveButton.onClick = [this] { chooseSave(); };
    table.setRowHeight (28);
    table.getHeader().addColumn ("Key", 1, 60, 50, 100);
    table.getHeader().addColumn ("Note", 2, 85, 65, 130);
    table.getHeader().addColumn ("Offset (cents)", 3, 200, 130, 450);
    table.getHeader().addColumn ("Target (Hz)", 4, 180, 110, 400);
    table.getHeader().setStretchToFitActive (true);
    table.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff202326));
    refresh();
    startTimerHz (10);
}

MicroTunePage::~MicroTunePage()
{
    stopTimer();
    chooser.reset();
    table.setModel (nullptr);
    viewport.setViewedComponent (nullptr, false);
}

void MicroTunePage::resized()
{
    viewport.setBounds (getLocalBounds());
    const auto height = juce::jmax (320, getHeight());
    const auto width = juce::jmax (1, getWidth() - (height > getHeight() ? viewport.getScrollBarThickness() : 0));
    content.setSize (width, height);
    auto area = content.getLocalBounds().reduced (12);
    heading.setBounds (area.removeFromTop (26));
    hint.setBounds (area.removeFromTop (28));
    auto actions = area.removeFromTop (30);
    newButton.setBounds (actions.removeFromLeft (100)); actions.removeFromLeft (8);
    loadButton.setBounds (actions.removeFromLeft (118)); actions.removeFromLeft (8);
    saveButton.setBounds (actions.removeFromLeft (118)); actions.removeFromLeft (16);
    targetLabel.setBounds (actions.removeFromLeft (86)); target.setBounds (actions.removeFromLeft (70));
    area.removeFromTop (8);
    auto names = area.removeFromTop (28);
    nameLabel.setBounds (names.removeFromLeft (50)); nameEditor.setBounds (names);
    area.removeFromTop (8);
    status.setBounds (area.removeFromBottom (30));
    table.setBounds (area);
}

void MicroTunePage::timerCallback() { refresh(); }

void MicroTunePage::refresh()
{
    const auto latest = processor.getLocalTuningInfo();
    if (latest.revision == displayedRevision) return;
    shown = latest;
    displayedRevision = shown.revision;
    if (! nameEditor.hasKeyboardFocus (true)) nameEditor.setText (shown.document.name, false);
    target.setSelectedId (shown.document.targetTable + 1, juce::dontSendNotification);
    status.setText (shown.unsavedFile ? "Local edits - save a table file to keep a separate copy."
                                    : "Local working table. Save the DAW Set to keep it in this project.", juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    table.updateContent();
    table.repaint();
}

void MicroTunePage::paintRowBackground (juce::Graphics& g, int row, int width, int height, bool selected)
{
    g.fillAll (selected ? juce::Colour (0xff24556a) : row % 2 == 0 ? juce::Colour (0xff202326) : juce::Colour (0xff272b2f));
    if (std::abs (shown.document.table.pitch (row).value_or (row) - static_cast<double> (row)) > 1.0e-9)
    { g.setColour (juce::Colours::deepskyblue); g.fillRect (0, 0, 3, height); }
    juce::ignoreUnused (width);
}

void MicroTunePage::paintCell (juce::Graphics& g, int row, int column, int width, int height, bool)
{
    if (row < 0 || row >= 128 || column == 3) return;
    const auto pitch = shown.document.table.pitch (row).value_or (row);
    const auto text = column == 1 ? juce::String (row)
        : column == 2 ? juce::MidiMessage::getMidiNoteName (row, true, true, 4)
        : juce::String (440.0 * std::pow (2.0, (pitch - 69.0) / 12.0), 3);
    g.setColour (juce::Colours::white);
    g.drawText (text, 8, 0, width - 12, height, juce::Justification::centredLeft);
}

juce::Component* MicroTunePage::refreshComponentForCell (int row, int column, bool, juce::Component* existing)
{
    if (column != 3 || row < 0 || row >= 128) { delete existing; return nullptr; }
    auto* cell = dynamic_cast<CentsCell*> (existing);
    if (! cell) { delete existing; cell = new CentsCell (*this); }
    cell->display (row, shown.document.table.pitch (row).value_or (row));
    return cell;
}

void MicroTunePage::editCents (int key, const juce::String& input)
{
    const auto text = input.trim();
    char* end = nullptr;
    const auto* start = text.toRawUTF8();
    const auto cents = std::strtod (start, &end);
    auto document = processor.getLocalTuningInfo().document;
    if (text.isEmpty() || end == start || *end != 0 || ! document.table.setCents (key, cents))
    {
        status.setText ("Invalid offset. Keep the resulting pitch within MIDI notes 0-127.", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, juce::Colours::orange);
        const juce::Component::SafePointer<MicroTunePage> safe (this);
        juce::MessageManager::callAsync ([safe, key]
        {
            if (! safe) return;
            if (auto* cell = dynamic_cast<CentsCell*> (safe->table.getCellComponent (3, key)))
                cell->display (key, safe->processor.getLocalTuningInfo().document.table.pitch (key).value_or (key));
        });
        return;
    }
    processor.replaceLocalTuningDocument (document);
    // Refresh on the timer, after the edited label's callback has returned.
}

void MicroTunePage::commitName()
{
    const auto name = nameEditor.getText().trim();
    auto document = processor.getLocalTuningInfo().document;
    if (name.isEmpty()) { nameEditor.setText (document.name, false); return; }
    if (name == document.name) return;
    document.name = name;
    processor.replaceLocalTuningDocument (document);
    refresh();
}

void MicroTunePage::confirmReplace (std::function<void()> action)
{
    if (! processor.getLocalTuningInfo().unsavedFile) { action(); return; }
    const juce::Component::SafePointer<MicroTunePage> safe (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon)
        .withTitle ("Replace local table?")
        .withMessage ("This table has changes that have not been saved to a local table file.")
        .withButton ("Replace").withButton ("Cancel"),
        [safe, pendingAction = std::move (action)] (int result) { if (safe && result == 1) pendingAction(); });
}

void MicroTunePage::newTable()
{
    bsii::microtuning::Document document;
    document.targetTable = processor.getLocalTuningInfo().document.targetTable;
    processor.replaceLocalTuningDocument (document);
    refresh();
}

bool MicroTunePage::loadDocumentFile (const juce::File& file)
{
    const auto document = file.existsAsFile() && file.getSize() <= 65536
        ? bsii::microtuning::parseDocument (file.loadFileAsString()) : std::nullopt;
    if (! document || ! processor.replaceLocalTuningDocument (*document, false))
    {
        status.setText ("Could not load a valid MicroTune table. Current table kept.", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, juce::Colours::orange);
        return false;
    }
    refresh();
    return true;
}

bool MicroTunePage::saveDocumentFile (const juce::File& file)
{
    commitName();
    const auto document = bsii::microtuning::serialise (processor.getLocalTuningInfo().document);
    if (! document || ! file.replaceWithText (*document))
    {
        status.setText ("Could not save table file. Your local edits are still here.", juce::dontSendNotification);
        return false;
    }
    processor.markLocalTuningFileSaved();
    refresh();
    status.setText ("Saved local table: " + file.getFileName(), juce::dontSendNotification);
    return true;
}

void MicroTunePage::chooseLoad()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a local MicroTune table", juce::File {}, "*.json");
    const juce::Component::SafePointer<MicroTunePage> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe] (const juce::FileChooser& selection)
        {
            if (! safe) return;
            const auto file = selection.getResult();
            if (file != juce::File {}) safe->loadDocumentFile (file);
            safe->chooser.reset();
        });
}

void MicroTunePage::chooseSave()
{
    chooser = std::make_unique<juce::FileChooser> ("Save a local MicroTune table",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BSII Tuning.json"), "*.json");
    const juce::Component::SafePointer<MicroTunePage> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe] (const juce::FileChooser& selection)
        {
            if (! safe) return;
            const auto file = selection.getResult();
            if (file != juce::File {}) safe->saveDocumentFile (file);
            safe->chooser.reset();
        });
}
