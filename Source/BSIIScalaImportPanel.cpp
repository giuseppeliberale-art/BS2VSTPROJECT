#include "BSIIScalaImportPanel.h"

namespace mt = bsii::microtuning;
namespace
{
juce::String reason (mt::scala::NoteStatus status)
{
    switch (status)
    {
        case mt::scala::NoteStatus::available: return "Import";
        case mt::scala::NoteStatus::unmapped: return "Unmapped key";
        case mt::scala::NoteStatus::outsideKeyboard: return "Outside KBM range";
        case mt::scala::NoteStatus::muted: return "Muted by tuning";
        case mt::scala::NoteStatus::outsidePitchRange: return "Pitch outside 0-127";
    }
    return {};
}
std::optional<std::string> read (const juce::File& file)
{
    if (! file.existsAsFile() || file.getSize() <= 0 || file.getSize() > 65536) return std::nullopt;
    juce::MemoryBlock bytes;
    if (! file.loadFileAsData (bytes)) return std::nullopt;
    return std::string (static_cast<const char*> (bytes.getData()), bytes.getSize());
}
}
ScalaImportPanel::ScalaImportPanel (BSIIControlAudioProcessor& p) : processor (p)
{
    for (auto* c : std::initializer_list<juce::Component*> { &files, &referenceHint, &summary, &policyHint,
            &scaleButton, &mappingButton, &clearMapping, &previewButton, &applyButton,
            &rootNote, &referenceNote, &referenceHz, &keepUnavailable, &table }) addAndMakeVisible (c);
    rootNote.setText ("60",false); referenceNote.setText ("69",false); referenceHz.setText ("440",false);
    rootNote.setComponentID ("scalaRoot"); referenceNote.setComponentID ("scalaReference"); referenceHz.setComponentID ("scalaHz");
    rootNote.setTooltip ("MIDI key for scale degree zero (0-127).");
    referenceNote.setTooltip ("MIDI reference key (0-127)."); referenceHz.setTooltip ("Absolute frequency of the reference key.");
    rootNote.setTitle ("Scale start key"); referenceNote.setTitle ("Reference key"); referenceHz.setTitle ("Reference Hz");
    for (auto* editor : { &rootNote, &referenceNote, &referenceHz }) editor->onTextChange = [this] { if (! preview || previewSettings != referenceSettings()) invalidate(); };
    referenceHint.setText ("Without KBM: scale start key / reference key / reference Hz",juce::dontSendNotification);
    files.setText ("Choose a Scala scale. Optional KBM supplies mapping and reference pitch.",juce::dontSendNotification);
    policyHint.setText ("Keeping unavailable notes preserves their pitches; it does not mute them. Nothing is sent to the synth.",juce::dontSendNotification);
    scaleButton.onClick = [this] { chooseFile (false); };
    mappingButton.onClick = [this] { chooseFile (true); };
    clearMapping.onClick = [this] { mappingText.reset(); mappingName.clear(); invalidate(); };
    previewButton.onClick = [this] { rebuildPreview(); };
    applyButton.onClick = [this] { applyPreview(); };
    keepUnavailable.onClick = [this] { updateApply(); };
    table.setRowHeight (25);
    table.getHeader().addColumn ("Key",1,45);
    table.getHeader().addColumn ("Current Hz",2,110);
    table.getHeader().addColumn ("Import Hz",3,110);
    table.getHeader().addColumn ("Change (cents)",4,130);
    table.getHeader().addColumn ("Result",5,230);
    table.getHeader().setStretchToFitActive (true);
    table.setColour (juce::ListBox::backgroundColourId,juce::Colour (0xff202326));
    setOpaque (true);
    setSize (740,620);
    invalidate();
}
ScalaImportPanel::~ScalaImportPanel() { chooser.reset(); table.setModel (nullptr); }
void ScalaImportPanel::resized()
{
    auto a = getLocalBounds().reduced (14);
    auto buttons = a.removeFromTop (30);
    scaleButton.setBounds (buttons.removeFromLeft (164)); buttons.removeFromLeft (8);
    mappingButton.setBounds (buttons.removeFromLeft (135)); buttons.removeFromLeft (8);
    clearMapping.setBounds (buttons.removeFromLeft (85)); buttons.removeFromLeft (8);
    previewButton.setBounds (buttons.removeFromLeft (100));
    files.setBounds (a.removeFromTop (42));
    referenceHint.setBounds (a.removeFromTop (24));
    auto fields = a.removeFromTop (28);
    rootNote.setBounds (fields.removeFromLeft (100)); fields.removeFromLeft (10);
    referenceNote.setBounds (fields.removeFromLeft (100)); fields.removeFromLeft (10);
    referenceHz.setBounds (fields.removeFromLeft (140));
    a.removeFromTop (6); summary.setBounds (a.removeFromTop (60));
    auto apply = a.removeFromBottom (30); applyButton.setBounds (apply.removeFromRight (180));
    keepUnavailable.setBounds (apply);
    policyHint.setBounds (a.removeFromBottom (42));
    table.setBounds (a);
}
void ScalaImportPanel::message (const juce::String& text) { summary.setText (text,juce::dontSendNotification); }
juce::String ScalaImportPanel::referenceSettings() const
{
    return rootNote.getText() + "\n" + referenceNote.getText() + "\n" + referenceHz.getText();
}
void ScalaImportPanel::invalidate()
{
    preview.reset(); table.updateContent(); table.repaint(); applyButton.setEnabled (false);
    for (auto* editor : { &rootNote, &referenceNote, &referenceHz }) editor->setEnabled (! mappingText && ! tunInput);
    mappingButton.setEnabled (! tunInput); clearMapping.setEnabled (! tunInput);
    referenceHint.setText (tunInput ? "TUN uses the file's pitch definitions and mapping; manual reference and KBM do not apply."
        : "Without KBM: scale start key / reference key / reference Hz",juce::dontSendNotification);
    files.setText ("Source: " + (scaleName.isEmpty() ? "not chosen" : scaleName)
        + (tunInput ? "   |   TUN" : "   |   Mapping: " + (mappingText ? mappingName : "no KBM")),juce::dontSendNotification);
    message ("Press Preview to review all 128 notes before applying.");
}
bool ScalaImportPanel::loadScaleFile (const juce::File& file)
{
    invalidate(); const auto text = read (file);
    if (! text) { scaleText.clear(); scaleName.clear(); message ("Could not read scale (maximum 64 KiB). Current table kept."); return false; }
    keepUnavailable.setToggleState (false,juce::dontSendNotification);
    tunInput = file.hasFileExtension ("tun");
    if (tunInput) { mappingText.reset(); mappingName.clear(); }
    scaleText = *text; scaleName = file.getFileName(); invalidate(); return true;
}
bool ScalaImportPanel::loadMappingFile (const juce::File& file)
{
    if (tunInput) return false;
    invalidate(); const auto text = read (file);
    if (! text) { message ("Could not read KBM (maximum 64 KiB). Previous file selection kept; preview again."); return false; }
    mappingText = *text; mappingName = file.getFileName(); invalidate(); return true;
}
void ScalaImportPanel::rebuildPreview()
{
    baseline = processor.getLocalTuningInfo();
    previewSettings = referenceSettings();
    mt::scala::Options options;
    try
    {
        if (! mappingText && ! tunInput)
        {
            options.scaleStart = mt::scala::integer (rootNote.getText().trim().toStdString(),0,127);
            options.referenceNote = mt::scala::integer (referenceNote.getText().trim().toStdString(),0,127);
            options.referenceHz = mt::scala::number (referenceHz.getText().trim().toStdString());
        }
        preview = tunInput ? mt::tun::parse (scaleText,scaleName.upToLastOccurrenceOf (".",false,false).toStdString())
                           : mt::scala::parse (scaleText,mappingText,options);
    }
    catch (const std::exception& e) { preview.emplace(); preview->error = e.what(); }
    if (! preview->error.empty()) message ("Import error: " + juce::String (preview->error));
    else message (juce::String (preview->available) + " notes available; " + juce::String (128 - preview->available)
        + (tunInput ? " unavailable. " + (preview->sourceInfo.empty() ? "Numeric TUN; base reference " + juce::String (preview->referenceHz,9) + " Hz." : juce::String (preview->sourceInfo)) + "\n"
            : " unavailable. Scale start " + juce::String (preview->scaleStart) + "; reference key "
                + juce::String (preview->referenceNote) + " = " + juce::String (preview->referenceHz,6) + " Hz.\n")
        + "Applying replaces available pitches and the table name; the file target stays unchanged.");
    table.updateContent(); table.repaint(); updateApply();
}
void ScalaImportPanel::updateApply()
{
    applyButton.setEnabled (preview && preview->valid() && (preview->available == 128 || keepUnavailable.getToggleState()));
}
bool ScalaImportPanel::applyPreview()
{
    if (! preview || processor.getLocalTuningInfo().revision != baseline.revision || previewSettings != referenceSettings())
    { invalidate(); message ("The working table changed. Press Preview again before applying."); return false; }
    const auto document = mt::scala::apply (*preview,baseline.document,keepUnavailable.getToggleState());
    if (! document || ! processor.replaceLocalTuningDocument (*document)) return false;
    invalidate(); message ("Imported into the local table. Close this window to edit or undo. Nothing was sent to the synth.");
    return true;
}
void ScalaImportPanel::paintRowBackground (juce::Graphics& g, int row, int w, int h, bool selected)
{
    g.fillAll (selected ? juce::Colour (0xff24556a) : row % 2 == 0 ? juce::Colour (0xff202326) : juce::Colour (0xff272b2f));
    juce::ignoreUnused (w,h);
}
void ScalaImportPanel::paintCell (juce::Graphics& g, int row, int column, int w, int h, bool)
{
    if (! preview || row < 0 || row >= 128 || ! preview->error.empty()) return;
    const auto& note = preview->notes[static_cast<std::size_t> (row)];
    const auto current = baseline.document.table.pitch (row).value();
    juce::String text;
    if (column == 1) text = juce::String (row);
    else if (column == 2) text = juce::String (440 * std::pow (2.0,(current - 69) / 12),4);
    else if (column == 3) text = note.hz > 0 && std::isfinite (note.hz) ? juce::String (note.hz,4) : "--";
    else if (column == 4) text = note.status == mt::scala::NoteStatus::available ? juce::String ((note.pitch - current) * 100,4) : "--";
    else text = reason (note.status);
    g.setColour (note.status == mt::scala::NoteStatus::available ? juce::Colours::white : juce::Colours::orange);
    g.drawText (text,5,0,w-10,h,juce::Justification::centredLeft);
}
void ScalaImportPanel::chooseFile (bool mapping)
{
    chooser = std::make_unique<juce::FileChooser> (mapping ? "Choose Scala keyboard mapping" : "Choose Scala scale or numeric TUN table",
        juce::File {},mapping ? "*.kbm" : "*.scl;*.tun");
    const juce::Component::SafePointer<ScalaImportPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe,mapping] (const juce::FileChooser& choice)
        {
            if (! safe) return;
            const auto file = choice.getResult();
            if (file != juce::File {}) { if (mapping) safe->loadMappingFile (file); else safe->loadScaleFile (file); }
            safe->chooser.reset();
        });
}
