#include "PluginEditor.h"
#include "BSIIMidi.h"
#include "BSIIPatchDecoder.h"
#include "BSIITuningDisplay.h"

ParameterControl::ParameterControl (juce::AudioProcessorValueTreeState& state,
                                    const bsii::ParameterSpec& parameterSpec,
                                    int specIndexIn,
                                    std::function<void(int, int)> valueChanged)
    : index (specIndexIn), spec (parameterSpec), callback (std::move (valueChanged))
{
    label.setText (spec.name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.86f));
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 20);
    slider.setRange (spec.minimum, spec.maximum, 1.0);
    slider.setDoubleClickReturnValue (true, spec.defaultValue);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, spec.id, slider);
    const juce::String parameterId (spec.id);
    const bool fine = parameterId == "osc1Fine" || parameterId == "osc2Fine";
    const bool coarse = parameterId == "osc1Coarse" || parameterId == "osc2Coarse";
    if (fine || coarse)
    {
        // Set after attachment construction, which installs its own raw formatter.
        slider.textFromValueFunction = [fine] (double raw)
        {
            const auto value = fine ? bsii::fineDisplay (juce::roundToInt (raw))
                                    : bsii::coarseDisplay (juce::roundToInt (raw));
            return (value > 0 ? "+" : "") + juce::String (value, fine ? 0 : 1);
        };
        slider.valueFromTextFunction = [fine] (const juce::String& text)
        {
            return static_cast<double> (fine ? bsii::fineRaw (text.getDoubleValue())
                                            : bsii::coarseRaw (text.getDoubleValue()));
        };
        slider.setTooltip (fine ? "Fine tuning in cents (-100 to +100)."
                               : "Approximate coarse tuning in semitones (-12.0 to +12.0). Hardware display rounding may differ.");
        slider.updateText();
    }
    slider.onValueChange = [this]
    {
        if (! applyingHardware && callback)
            callback (index, juce::roundToInt (slider.getValue()));
    };
}

void ParameterControl::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromTop (22));
    slider.setBounds (area);
}

void ParameterControl::setFromHardware (int value)
{
    juce::ScopedValueSetter<bool> guard (applyingHardware, true);
    slider.setValue (value, juce::sendNotificationSync);
}

ChoiceControl::ChoiceControl (juce::AudioProcessorValueTreeState& state,
                              const bsii::ChoiceSpec& choiceSpec,
                              int choiceIndexIn,
                              std::function<void(int, int)> choiceChanged)
    : index (choiceIndexIn), spec (choiceSpec), callback (std::move (choiceChanged))
{
    label.setText (spec.name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    for (int i = 0; i < static_cast<int> (spec.choices.size()); ++i)
        combo.addItem (spec.choices[static_cast<size_t> (i)].label, i + 1);
    addAndMakeVisible (combo);

    note.setText (spec.verifiedWireValues ? "MIDI verified" : "MIDI test", juce::dontSendNotification);
    note.setJustificationType (juce::Justification::centred);
    note.setColour (juce::Label::textColourId,
                    spec.verifiedWireValues ? juce::Colours::lightgreen.withAlpha (0.75f)
                                            : juce::Colours::orange.withAlpha (0.85f));
    addAndMakeVisible (note);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, spec.id, combo);
    combo.onChange = [this]
    {
        if (! applyingHardware && callback && combo.getSelectedId() > 0)
            callback (index, combo.getSelectedId() - 1);
    };
}

void ChoiceControl::resized()
{
    auto area = getLocalBounds().reduced (6);
    label.setBounds (area.removeFromTop (24));
    area.removeFromTop (18);
    combo.setBounds (area.removeFromTop (28));
    note.setBounds (area.removeFromTop (22));
}

bool ChoiceControl::setFromWireValue (int wireValue)
{
    for (int i = 0; i < static_cast<int> (spec.choices.size()); ++i)
    {
        if (spec.choices[static_cast<size_t> (i)].wireValue == wireValue)
        {
            juce::ScopedValueSetter<bool> guard (applyingHardware, true);
            combo.setSelectedId (i + 1, juce::sendNotificationSync);
            return true;
        }
    }
    note.setText ("RX raw " + juce::String (wireValue), juce::dontSendNotification);
    note.setColour (juce::Label::textColourId, juce::Colours::orange.withAlpha (0.9f));
    return false;
}

void ChoiceControl::setInteractionEnabled (bool enabled)
{
    combo.setEnabled (enabled);
    note.setAlpha (enabled ? 1.0f : 0.55f);
}

ParameterSection::ParameterSection (juce::String titleText) : title (std::move (titleText)) {}
void ParameterSection::addControl (juce::Component* control) { controls.push_back (control); addAndMakeVisible (control); }

int ParameterSection::preferredHeight() const
{
    constexpr int controlsPerRow = 6;
    const auto rows = juce::jmax (1, static_cast<int> ((controls.size() + controlsPerRow - 1) / controlsPerRow));
    return 34 + rows * 130 + 12;
}

void ParameterSection::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.drawText (title, 14, 6, getWidth() - 28, 24, juce::Justification::centredLeft);
}

void ParameterSection::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (28);
    constexpr int controlsPerRow = 6;
    constexpr int controlHeight = 126;

    for (size_t i = 0; i < controls.size(); ++i)
    {
        const int row = static_cast<int> (i) / controlsPerRow;
        const int column = static_cast<int> (i) % controlsPerRow;
        const int columnsThisRow = juce::jmin (controlsPerRow,
            static_cast<int> (controls.size()) - row * controlsPerRow);
        const int cellWidth = area.getWidth() / juce::jmax (1, columnsThisRow);
        controls[i]->setBounds (area.getX() + column * cellWidth,
                                area.getY() + row * controlHeight,
                                cellWidth, controlHeight);
    }
}

void EditorPage::addSection (ParameterSection* section) { sections.push_back (section); addAndMakeVisible (section); }
void EditorPage::resized()
{
    auto area = getLocalBounds().reduced (8);
    for (auto* section : sections)
    {
        const auto h = section->preferredHeight();
        section->setBounds (area.removeFromTop (h));
        area.removeFromTop (8);
    }
}

BSIIControlAudioProcessorEditor::BSIIControlAudioProcessorEditor (BSIIControlAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (1080, 840);
    titleLabel.setText ("Bass Station II Control — Waveforms + SysEx Validation v0.8.2", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (23.0f).withStyle ("Bold"));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Oscillator waveforms are now exposed via live MIDI. Pause Live RX to validate SysEx separately from CC/NRPN sync.", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.68f));
    addAndMakeVisible (subtitleLabel);

    midiLabel.setText ("MIDI output", juce::dontSendNotification);
    channelLabel.setText ("Channel", juce::dontSendNotification);
    addAndMakeVisible (midiLabel);
    addAndMakeVisible (channelLabel);
    addAndMakeVisible (midiOutputBox);
    addAndMakeVisible (channelBox);
    addAndMakeVisible (refreshButton);
    addAndMakeVisible (pauseLiveRxButton);

    for (int channel = 1; channel <= 16; ++channel)
        channelBox.addItem (juce::String (channel), channel);
    channelBox.setSelectedId (1, juce::dontSendNotification);
    processor.setHardwareMidiChannel (1);
    channelBox.onChange = [this]
    {
        midiChannel = juce::jmax (1, channelBox.getSelectedId());
        processor.setHardwareMidiChannel (midiChannel);
        std::fill (lastSentValues.begin(), lastSentValues.end(), -1);
        std::fill (lastSentChoiceIndices.begin(), lastSentChoiceIndices.end(), -1);
    };

    refreshButton.onClick = [this] { refreshMidiOutputs(); autoSelectBassStation(); };
    pauseLiveRxButton.onClick = [this] { setLiveRxPaused (pauseLiveRxButton.getToggleState()); };
    midiOutputBox.onChange = [this] { selectMidiOutput (midiOutputBox.getSelectedId()); };

    saveButton.onClick = [this] { saveSnapshot(); };
    loadButton.onClick = [this] { loadSnapshot(); };
    sendAllButton.onClick = [this] { sendAllStateToHardware(); };
    getPatchButton.onClick = [this] { requestPatchFromHardware(); };
    armSysExButton.onClick = [this] { armSysExCapture(); };
    saveSysExButton.onClick = [this] { saveLastSysEx(); };
    sendSysExButton.onClick = [this] { sendSysExFileToHardware(); };
    importSysExButton.onClick = [this] { importSysExToEditor(); };
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (sendAllButton);
    addAndMakeVisible (getPatchButton);
    addAndMakeVisible (armSysExButton);
    addAndMakeVisible (saveSysExButton);
    addAndMakeVisible (sendSysExButton);
    addAndMakeVisible (importSysExButton);

    stateHintLabel.setText ("Get Patch requests the current edit-buffer patch automatically. Manual Function/Exit + Dump capture remains available as a fallback.",
                            juce::dontSendNotification);
    stateHintLabel.setColour (juce::Label::textColourId, juce::Colours::orange.withAlpha (0.82f));
    stateHintLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (stateHintLabel);

    addAndMakeVisible (tabs);
    addAndMakeVisible (rxLabel);
    addAndMakeVisible (nrpnLabel);
    addAndMakeVisible (sysexLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (diagnosticLabel);
    rxLabel.setText ("CC RX: waiting...", juce::dontSendNotification);
    nrpnLabel.setText ("NRPN RX: waiting...", juce::dontSendNotification);
    sysexLabel.setText ("SysEx RX: waiting...", juce::dontSendNotification);
    rxLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.78f));
    nrpnLabel.setColour (juce::Label::textColourId, juce::Colours::lightblue.withAlpha (0.88f));
    sysexLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen.withAlpha (0.88f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.75f));
    diagnosticLabel.setColour (juce::Label::textColourId, juce::Colours::yellow.withAlpha (0.82f));
    diagnosticLabel.setJustificationType (juce::Justification::centredLeft);
    diagnosticLabel.setText ("SysEx validation: no patch decoded yet.", juce::dontSendNotification);

    lastSentValues.assign (bsii::parameterSpecs().size(), -1);
    lastSentChoiceIndices.assign (bsii::choiceSpecs().size(), -1);
    buildPages();
    refreshMidiOutputs();
    autoSelectBassStation();
    startTimerHz (30);
}

BSIIControlAudioProcessorEditor::~BSIIControlAudioProcessorEditor()
{
    stopTimer();
    fileChooser.reset();
    midiOutput.reset();
}

void BSIIControlAudioProcessorEditor::buildPages()
{
    const std::vector<std::pair<juce::String, std::vector<juce::String>>> pageDefinitions
    {
        { "Osc 1", { "Master", "Osc 1" } },
        { "Osc 2", { "Osc 2" } },
        { "Mixer", { "Mixer" } },
        { "Filter", { "Filter" } },
        { "LFO", { "LFO / Glide" } },
        { "Amp Env", { "Amp Env" } },
        { "Mod Env", { "Mod Env" } },
        { "FX + Perf", { "FX / Perf" } }
    };

    const auto& params = bsii::parameterSpecs();
    const auto& choices = bsii::choiceSpecs();

    for (const auto& pageDef : pageDefinitions)
    {
        auto page = std::make_unique<EditorPage>();
        for (const auto& sectionName : pageDef.second)
        {
            auto section = std::make_unique<ParameterSection> (sectionName);

            for (size_t i = 0; i < choices.size(); ++i)
            {
                if (sectionName != choices[i].section) continue;
                auto control = std::make_unique<ChoiceControl> (processor.getParameters(), choices[i], static_cast<int> (i),
                    [this] (int choiceIndex, int selectedIndex) { sendChoiceValue (choiceIndex, selectedIndex); });
                section->addControl (control.get());
                choiceControls.push_back (std::move (control));
            }

            for (size_t i = 0; i < params.size(); ++i)
            {
                if (sectionName != params[i].section) continue;
                auto control = std::make_unique<ParameterControl> (processor.getParameters(), params[i], static_cast<int> (i),
                    [this] (int specIndex, int value) { sendParameterValue (specIndex, value); });
                section->addControl (control.get());
                parameterControls.push_back (std::move (control));
            }

            page->addSection (section.get());
            sections.push_back (std::move (section));
        }
        tabs.addTab (pageDef.first, juce::Colours::transparentBlack, page.get(), false);
        pages.push_back (std::move (page));
    }
}

void BSIIControlAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15171a));
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (10.0f), 8.0f, 1.0f);
}

void BSIIControlAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (34));
    subtitleLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (6);

    auto deviceRow = area.removeFromTop (32);
    midiLabel.setBounds (deviceRow.removeFromLeft (82));
    midiOutputBox.setBounds (deviceRow.removeFromLeft (280));
    deviceRow.removeFromLeft (12);
    channelLabel.setBounds (deviceRow.removeFromLeft (58));
    channelBox.setBounds (deviceRow.removeFromLeft (70));
    deviceRow.removeFromLeft (10);
    refreshButton.setBounds (deviceRow.removeFromLeft (120));
    deviceRow.removeFromLeft (10);
    pauseLiveRxButton.setBounds (deviceRow.removeFromLeft (190));

    area.removeFromTop (8);
    auto stateRow = area.removeFromTop (32);
    saveButton.setBounds (stateRow.removeFromLeft (150));
    stateRow.removeFromLeft (8);
    loadButton.setBounds (stateRow.removeFromLeft (150));
    stateRow.removeFromLeft (8);
    sendAllButton.setBounds (stateRow.removeFromLeft (170));
    stateRow.removeFromLeft (8);
    getPatchButton.setBounds (stateRow.removeFromLeft (190));

    area.removeFromTop (6);
    auto sysexRow = area.removeFromTop (32);
    armSysExButton.setBounds (sysexRow.removeFromLeft (160));
    sysexRow.removeFromLeft (8);
    saveSysExButton.setBounds (sysexRow.removeFromLeft (150));
    sysexRow.removeFromLeft (8);
    sendSysExButton.setBounds (sysexRow.removeFromLeft (160));
    sysexRow.removeFromLeft (8);
    importSysExButton.setBounds (sysexRow.removeFromLeft (170));
    sysexRow.removeFromLeft (14);
    stateHintLabel.setBounds (sysexRow);

    area.removeFromTop (10);
    auto bottom = area.removeFromBottom (136);
    rxLabel.setBounds (bottom.removeFromTop (24));
    nrpnLabel.setBounds (bottom.removeFromTop (24));
    sysexLabel.setBounds (bottom.removeFromTop (24));
    statusLabel.setBounds (bottom.removeFromTop (28));
    diagnosticLabel.setBounds (bottom.removeFromTop (28));
    area.removeFromBottom (6);
    tabs.setBounds (area);
}

void BSIIControlAudioProcessorEditor::timerCallback()
{
    const auto devicesNow = juce::MidiOutput::getAvailableDevices();
    if (devicesNow != midiOutputs)
    {
        const auto previousId = selectedDeviceIdentifier;
        refreshMidiOutputs();
        for (int i = 0; i < midiOutputs.size(); ++i)
            if (midiOutputs[i].identifier == previousId)
            {
                midiOutputBox.setSelectedId (i + 1, juce::sendNotificationSync);
                break;
            }
        if (midiOutputBox.getSelectedId() == 0)
            autoSelectBassStation();
    }

    const auto nowMs = juce::Time::getMillisecondCounter();
    if (midiOutput == nullptr && selectedDeviceIdentifier.isNotEmpty()
        && nowMs - lastReconnectAttemptMs > 2000u)
    {
        lastReconnectAttemptMs = nowMs;
        reopenSelectedMidiOutput (false);
    }

    if (patchRequestPending && nowMs - patchRequestStartedMs > 2500u)
    {
        patchRequestPending = false;
        statusLabel.setText ("No patch reply received. In Standalone Settings, ensure Bass Station II is enabled under Active MIDI inputs, then try Get Patch again.",
                             juce::dontSendNotification);
    }

    applyHardwareUpdates();
    updateRxDisplay();
    updateSysExDisplay();
    updateFilterModeUi();
}

void BSIIControlAudioProcessorEditor::refreshMidiOutputs()
{
    midiOutputs = juce::MidiOutput::getAvailableDevices();
    midiOutputBox.clear (juce::dontSendNotification);
    for (int i = 0; i < midiOutputs.size(); ++i)
        midiOutputBox.addItem (midiOutputs[i].name, i + 1);
    statusLabel.setText (midiOutputs.isEmpty() ? "No MIDI outputs detected." : "Select the Bass Station II MIDI output.",
                         juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::autoSelectBassStation()
{
    for (int i = 0; i < midiOutputs.size(); ++i)
    {
        const auto name = midiOutputs[i].name.toLowerCase();
        if (name.contains ("bass station") || name.contains ("bassstation"))
        {
            midiOutputBox.setSelectedId (i + 1, juce::sendNotificationSync);
            return;
        }
    }
}

void BSIIControlAudioProcessorEditor::selectMidiOutput (int comboBoxId)
{
    const int index = comboBoxId - 1;
    if (! juce::isPositiveAndBelow (index, midiOutputs.size()))
    {
        midiOutput.reset();
        selectedDeviceIdentifier.clear();
        statusLabel.setText ("No MIDI output selected.", juce::dontSendNotification);
        return;
    }

    selectedDeviceIdentifier = midiOutputs[index].identifier;
    reopenSelectedMidiOutput (true);
}

bool BSIIControlAudioProcessorEditor::reopenSelectedMidiOutput (bool forceReopen)
{
    if (selectedDeviceIdentifier.isEmpty())
        return false;

    if (midiOutput != nullptr && ! forceReopen)
        return true;

    auto devices = juce::MidiOutput::getAvailableDevices();
    juce::String deviceName;
    bool found = false;
    for (const auto& device : devices)
    {
        if (device.identifier == selectedDeviceIdentifier)
        {
            deviceName = device.name;
            found = true;
            break;
        }
    }

    if (! found)
    {
        midiOutput.reset();
        return false;
    }

    midiOutput.reset();
    midiOutput = juce::MidiOutput::openDevice (selectedDeviceIdentifier);
    if (midiOutput != nullptr)
    {
        std::fill (lastSentValues.begin(), lastSentValues.end(), -1);
        std::fill (lastSentChoiceIndices.begin(), lastSentChoiceIndices.end(), -1);
        statusLabel.setText ("TX connected to " + deviceName + ". RX requires Bass Station II enabled under Settings -> Active MIDI inputs.",
                             juce::dontSendNotification);
        return true;
    }

    statusLabel.setText ("Could not open " + deviceName + ".", juce::dontSendNotification);
    return false;
}

void BSIIControlAudioProcessorEditor::requestPatchFromHardware()
{
    if (! reopenSelectedMidiOutput (true))
    {
        statusLabel.setText ("Cannot request patch: Bass Station II MIDI output is not open.", juce::dontSendNotification);
        return;
    }

    const auto latest = processor.getLatestSysEx();
    patchRequestStartSerial = latest.serial;
    patchRequestPending = true;
    patchRequestStartedMs = juce::Time::getMillisecondCounter();

    static constexpr uint8_t requestBytes[] { 0xF0, 0x00, 0x20, 0x29, 0x00, 0x33, 0x00, 0x40, 0xF7 };
    juce::MidiMessage request (requestBytes, static_cast<int> (sizeof (requestBytes)), 0.0);
    midiOutput->sendMessageNow (request);
    statusLabel.setText ("Requested current patch from Bass Station II…", juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::sendParameterValue (int specIndex, int value)
{
    if (suppressMidiSends) return;
    const auto& specs = bsii::parameterSpecs();
    if (midiOutput == nullptr || ! juce::isPositiveAndBelow (specIndex, static_cast<int> (specs.size()))) return;
    if (lastSentValues[static_cast<size_t> (specIndex)] == value) return;
    lastSentValues[static_cast<size_t> (specIndex)] = value;
    bsii::sendParameter (*midiOutput, midiChannel, specs[static_cast<size_t> (specIndex)], value);
}

void BSIIControlAudioProcessorEditor::sendChoiceValue (int choiceIndex, int selectedChoiceIndex)
{
    if (suppressMidiSends) return;
    const auto& specs = bsii::choiceSpecs();
    if (midiOutput == nullptr || ! juce::isPositiveAndBelow (choiceIndex, static_cast<int> (specs.size()))) return;
    const auto& spec = specs[static_cast<size_t> (choiceIndex)];
    if (! juce::isPositiveAndBelow (selectedChoiceIndex, static_cast<int> (spec.choices.size()))) return;
    if (lastSentChoiceIndices[static_cast<size_t> (choiceIndex)] == selectedChoiceIndex) return;
    lastSentChoiceIndices[static_cast<size_t> (choiceIndex)] = selectedChoiceIndex;
    bsii::sendChoice (*midiOutput, midiChannel, spec, spec.choices[static_cast<size_t> (selectedChoiceIndex)].wireValue);
}

int BSIIControlAudioProcessorEditor::currentParameterValue (const char* parameterId) const
{
    if (auto* value = processor.getParameters().getRawParameterValue (parameterId))
        return juce::roundToInt (value->load());
    return 0;
}

void BSIIControlAudioProcessorEditor::sendAllStateToHardware()
{
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Cannot send state: no MIDI output is open.", juce::dontSendNotification);
        return;
    }

    const auto& params = bsii::parameterSpecs();
    const auto& choices = bsii::choiceSpecs();

    for (size_t i = 0; i < params.size(); ++i)
    {
        const auto value = currentParameterValue (params[i].id);
        bsii::sendParameter (*midiOutput, midiChannel, params[i], value);
        lastSentValues[i] = value;
    }

    for (size_t i = 0; i < choices.size(); ++i)
    {
        const auto choiceIndex = juce::jlimit (0, static_cast<int> (choices[i].choices.size()) - 1,
                                              currentParameterValue (choices[i].id));
        bsii::sendChoice (*midiOutput, midiChannel, choices[i],
                          choices[i].choices[static_cast<size_t> (choiceIndex)].wireValue);
        lastSentChoiceIndices[i] = choiceIndex;
    }

    statusLabel.setText ("Sent complete editor state to Bass Station II on MIDI channel " + juce::String (midiChannel) + ".",
                         juce::dontSendNotification);
}

juce::String BSIIControlAudioProcessorEditor::hexPreview (const juce::MemoryBlock& block, int maxBytes)
{
    const auto* bytes = static_cast<const uint8_t*> (block.getData());
    const auto count = juce::jmin (static_cast<int> (block.getSize()), maxBytes);
    juce::StringArray parts;
    for (int i = 0; i < count; ++i)
        parts.add (juce::String::toHexString (static_cast<int> (bytes[i])).paddedLeft ('0', 2).toUpperCase());
    auto result = parts.joinIntoString (" ");
    if (static_cast<int> (block.getSize()) > count) result += " …";
    return result;
}

void BSIIControlAudioProcessorEditor::armSysExCapture()
{
    const auto latest = processor.getLatestSysEx();
    captureArmSerial = latest.serial;
    sysExCaptureArmed = true;
    statusLabel.setText ("Armed. On BSII: hold Function/Exit + top C (Dump) until onE, then press Dump again while still holding Function.",
                         juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::updateSysExDisplay()
{
    const auto latest = processor.getLatestSysEx();
    if (latest.serial == 0 || latest.serial == lastDisplayedSysExSerial)
        return;

    lastDisplayedSysExSerial = latest.serial;
    const auto size = static_cast<int> (latest.data.getSize());
    const auto preview = hexPreview (latest.data);
    sysexLabel.setText ("SysEx RX: " + juce::String (size) + " bytes | " + preview,
                        juce::dontSendNotification);

    if (patchRequestPending && latest.serial > patchRequestStartSerial)
    {
        patchRequestPending = false;
        if (decodeSysExIntoEditor (latest.data, "live Bass Station II patch"))
            statusLabel.setText ("Live patch received and decoded from Bass Station II (" + juce::String (size) + " bytes).",
                                 juce::dontSendNotification);
        return;
    }

    if (sysExCaptureArmed && latest.serial > captureArmSerial)
    {
        sysExCaptureArmed = false;
        const auto* bytes = static_cast<const uint8_t*> (latest.data.getData());
        const bool looksLikeNovation = latest.data.getSize() >= 5
            && bytes[0] == 0xF0 && bytes[1] == 0x00 && bytes[2] == 0x20 && bytes[3] == 0x29;
        statusLabel.setText (looksLikeNovation
            ? "Captured a Novation SysEx message (" + juce::String (size) + " bytes). Use Save Last .syx… to keep the raw patch dump."
            : "Captured SysEx (" + juce::String (size) + " bytes). Header is not the expected Novation manufacturer ID.",
            juce::dontSendNotification);
    }
}

void BSIIControlAudioProcessorEditor::saveLastSysEx()
{
    const auto latest = processor.getLatestSysEx();
    if (latest.data.isEmpty())
    {
        statusLabel.setText ("No SysEx has been received yet.", juce::dontSendNotification);
        return;
    }

    auto defaultFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile ("Bass Station II Patch.syx");
    fileChooser = std::make_unique<juce::FileChooser> ("Save captured Bass Station II SysEx", defaultFile, "*.syx");
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (flags, [this, data = latest.data] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            if (! file.hasFileExtension ("syx")) file = file.withFileExtension ("syx");
            const bool ok = file.replaceWithData (data.getData(), data.getSize());
            statusLabel.setText (ok ? "Saved raw SysEx: " + file.getFileName() : "Could not save SysEx file.",
                                 juce::dontSendNotification);
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::sendSysExFileToHardware()
{
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Cannot send SysEx: no MIDI output is open.", juce::dontSendNotification);
        return;
    }

    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Send a single Bass Station II .syx patch", start, "*.syx");
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            juce::MemoryBlock data;
            if (! file.loadFileAsData (data) || data.getSize() < 3)
            {
                statusLabel.setText ("Could not read SysEx file.", juce::dontSendNotification);
                fileChooser.reset();
                return;
            }

            const auto* bytes = static_cast<const uint8_t*> (data.getData());
            if (bytes[0] != 0xF0 || bytes[data.getSize() - 1] != 0xF7)
            {
                statusLabel.setText ("This file is not one complete SysEx message (expected F0 ... F7).", juce::dontSendNotification);
                fileChooser.reset();
                return;
            }

            juce::MidiMessage message (data.getData(), static_cast<int> (data.getSize()), 0.0);
            midiOutput->sendMessageNow (message);
            statusLabel.setText ("Sent " + juce::String (static_cast<int> (data.getSize())) + "-byte SysEx message to Bass Station II edit buffer.",
                                 juce::dontSendNotification);
        }
        fileChooser.reset();
    });
}


bool BSIIControlAudioProcessorEditor::setEditorParameterValue (const char* parameterId, int value)
{
    auto* parameter = processor.getParameters().getParameter (parameterId);
    if (parameter == nullptr)
        return false;

    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
    {
        const auto normalised = ranged->convertTo0to1 (static_cast<float> (value));
        ranged->setValueNotifyingHost (normalised);
        return true;
    }

    return false;
}

bool BSIIControlAudioProcessorEditor::decodeSysExIntoEditor (const juce::MemoryBlock& data,
                                                              const juce::String& sourceName)
{
    const auto decoded = bsii::decodePatchFields (data);
    if (! decoded.valid)
    {
        statusLabel.setText ("Could not decode " + sourceName + ": " + decoded.error,
                             juce::dontSendNotification);
        return false;
    }

    juce::ScopedValueSetter<bool> guard (suppressMidiSends, true);

    juce::StringArray changedFields;
    auto set = [this, &changedFields] (const char* id, int value)
    {
        if (value < 0) return;
        const auto before = currentParameterValue (id);
        if (setEditorParameterValue (id, value) && before != value)
            changedFields.add (juce::String (id) + " " + juce::String (before) + "->" + juce::String (value));
    };

    set ("portamento", decoded.portamento);
    set ("bendAmount", decoded.bendAmount);

    set ("osc1Fine", decoded.osc1Fine);
    set ("osc1Coarse", decoded.osc1Coarse);
    set ("osc1ManualPW", decoded.osc1ManualPW);
    set ("osc1ModEnvPW", decoded.osc1ModEnvPW);
    set ("osc1Range", decoded.osc1Range);
    set ("osc1ModEnvPitch", decoded.osc1ModEnvPitch);
    set ("osc1Lfo1Pitch", decoded.osc1Lfo1Pitch);
    set ("osc1Lfo2PW", decoded.osc1Lfo2PW);

    set ("osc2Fine", decoded.osc2Fine);
    set ("osc2Coarse", decoded.osc2Coarse);
    set ("osc2ManualPW", decoded.osc2ManualPW);
    set ("osc2Range", decoded.osc2Range);
    set ("osc2ModEnvPitch", decoded.osc2ModEnvPitch);
    set ("osc2Lfo1Pitch", decoded.osc2Lfo1Pitch);
    set ("osc2ModEnvPW", decoded.osc2ModEnvPW);
    set ("osc2Lfo2PW", decoded.osc2Lfo2PW);

    set ("mixOsc1", decoded.mixOsc1);
    set ("mixOsc2", decoded.mixOsc2);
    set ("mixSub", decoded.mixSub);
    set ("mixNoise", decoded.mixNoise);
    set ("mixRing", decoded.mixRing);
    set ("mixExternal", decoded.mixExternal);

    set ("filterCutoff", decoded.filterCutoff);
    set ("filterResonance", decoded.filterResonance);
    set ("filterOverdrive", decoded.filterOverdrive);
    set ("filterType", decoded.filterType);
    set ("filterShape", decoded.filterShape);
    set ("filterSlope", decoded.filterSlope);
    set ("filterModEnv", decoded.filterModEnv);
    set ("filterLfo2", decoded.filterLfo2);
    set ("filterTracking", decoded.filterTracking);

    set ("velocityAmp", decoded.velocityAmp);
    set ("ampAttack", decoded.ampAttack);
    set ("ampTrigger", decoded.ampTrigger);
    set ("ampDecay", decoded.ampDecay);
    set ("ampSustain", decoded.ampSustain);
    set ("ampRelease", decoded.ampRelease);
    set ("ampRetrigger", decoded.ampRetrigger);
    set ("ampFixed", decoded.ampFixed);
    set ("ampRetriggerCount", decoded.ampRetriggerCount);

    set ("velocityMod", decoded.velocityMod);
    set ("modAttack", decoded.modAttack);
    set ("modDecay", decoded.modDecay);
    set ("modSustain", decoded.modSustain);
    set ("modRelease", decoded.modRelease);
    set ("modRetrigger", decoded.modRetrigger);
    set ("modTrigger", decoded.modTrigger);
    set ("modFixed", decoded.modFixed);
    set ("modRetriggerCount", decoded.modRetriggerCount);

    set ("lfo1Wave", decoded.lfo1Wave);
    set ("lfo1Delay", decoded.lfo1Delay);
    set ("lfo1Slew", decoded.lfo1Slew);
    set ("lfo1Speed", decoded.lfo1Speed);
    set ("lfo1KeySync", decoded.lfo1KeySync);
    set ("lfo2Wave", decoded.lfo2Wave);
    set ("lfo2Delay", decoded.lfo2Delay);
    set ("lfo2Slew", decoded.lfo2Slew);
    set ("lfo2Speed", decoded.lfo2Speed);
    set ("lfo2KeySync", decoded.lfo2KeySync);

    set ("arpSwing", decoded.arpSwing);
    set ("modWhLfo1Pitch", decoded.modWhLfo1Pitch);
    set ("modWhOsc2Pitch", decoded.modWhOsc2Pitch);
    set ("aftertouchFilter", decoded.aftertouchFilter);
    set ("aftertouchLfo1", decoded.aftertouchLfo1);
    set ("aftertouchLfo2Spd", decoded.aftertouchLfo2Spd);
    set ("oscFilterMod", decoded.oscFilterMod);
    set ("oscSyncRaw", decoded.oscSyncRaw);
    set ("distortion", decoded.distortion);
    set ("vcaLimit", decoded.vcaLimit);

    std::fill (lastSentValues.begin(), lastSentValues.end(), -1);
    std::fill (lastSentChoiceIndices.begin(), lastSentChoiceIndices.end(), -1);
    updateFilterModeUi();

    diagnosticLabel.setText (changedFields.isEmpty()
        ? "SysEx decoder changed no editor fields."
        : "SysEx changed: " + changedFields.joinIntoString (", "),
        juce::dontSendNotification);

    statusLabel.setText (
        "Decoded " + juce::String (decoded.decodedCount) + " patch fields ("
        + juce::String (decoded.verifiedCount) + " verified, "
        + juce::String (decoded.candidateCount) + " safe candidates)"
        + (decoded.patchName.isNotEmpty() ? " from \"" + decoded.patchName + "\"" : juce::String())
        + " (" + sourceName + "). Out-of-range/unknown fields were skipped.",
        juce::dontSendNotification);
    return true;
}

void BSIIControlAudioProcessorEditor::importSysExToEditor()
{
    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Import a captured Bass Station II .syx into the editor",
                                                        start, "*.syx");
    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            juce::MemoryBlock data;
            if (! file.loadFileAsData (data))
                statusLabel.setText ("Could not read " + file.getFileName() + ".", juce::dontSendNotification);
            else
                decodeSysExIntoEditor (data, file.getFileName());
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::setLiveRxPaused (bool paused)
{
    liveRxPaused = paused;
    processor.setLiveMidiRxEnabled (! paused);
    pauseLiveRxButton.setButtonText (paused ? "Live RX is paused" : "Live RX is running");
    rxLabel.setText (paused ? "CC RX: paused (SysEx still active)" : "CC RX: waiting...", juce::dontSendNotification);
    nrpnLabel.setText (paused ? "NRPN RX: paused (SysEx still active)" : "NRPN RX: waiting...", juce::dontSendNotification);
    statusLabel.setText (paused
        ? "Live CC/NRPN RX paused. Change the BSII preset/controls, then use Get Patch to validate SysEx only."
        : "Live CC/NRPN RX resumed.",
        juce::dontSendNotification);
}

void BSIIControlAudioProcessorEditor::updateFilterModeUi()
{
    const auto acid = currentParameterValue ("filterType") == 1;

    const auto& choices = bsii::choiceSpecs();
    for (size_t i = 0; i < choices.size() && i < choiceControls.size(); ++i)
    {
        const juce::String id (choices[i].id);
        if (id == "filterShape" || id == "filterSlope")
            choiceControls[i]->setInteractionEnabled (! acid);
    }

    if (acid)
    {
        juce::ScopedValueSetter<bool> guard (suppressMidiSends, true);
        setEditorParameterValue ("filterShape", 0); // Acid is LP only
        setEditorParameterValue ("filterSlope", 1); // Acid is fixed 24 dB/oct
    }
}

bool BSIIControlAudioProcessorEditor::saveStateToFile (const juce::File& requestedFile)
{
    auto file = requestedFile;
    if (! file.hasFileExtension ("bsiistate"))
        file = file.withFileExtension ("bsiistate");

    auto state = processor.getParameters().copyState();
    state.setProperty ("bsiiControlVersion", "0.8.1", nullptr);
    state.setProperty ("savedAt", juce::Time::getCurrentTime().toISO8601 (true), nullptr);

    if (auto xml = state.createXml())
        return xml->writeTo (file);
    return false;
}

bool BSIIControlAudioProcessorEditor::loadStateFromFile (const juce::File& file)
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || ! state.hasType (processor.getParameters().state.getType()))
        return false;

    juce::ScopedValueSetter<bool> guard (suppressMidiSends, true);
    processor.getParameters().replaceState (state);
    std::fill (lastSentValues.begin(), lastSentValues.end(), -1);
    std::fill (lastSentChoiceIndices.begin(), lastSentChoiceIndices.end(), -1);
    return true;
}

void BSIIControlAudioProcessorEditor::saveSnapshot()
{
    auto defaultFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile ("Bass Station II Snapshot.bsiistate");
    fileChooser = std::make_unique<juce::FileChooser> ("Save Bass Station II editor snapshot", defaultFile,
                                                        "*.bsiistate");
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            const auto ok = saveStateToFile (file);
            statusLabel.setText (ok ? "Saved snapshot: " + file.withFileExtension ("bsiistate").getFileName()
                                    : "Could not save snapshot.",
                                 juce::dontSendNotification);
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::loadSnapshot()
{
    auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> ("Load Bass Station II editor snapshot", start,
                                                        "*.bsiistate");
    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file != juce::File{})
        {
            const auto ok = loadStateFromFile (file);
            if (ok)
            {
                statusLabel.setText ("Loaded snapshot: " + file.getFileName() + ". Use Send All → BSII to recall it on hardware.",
                                     juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText ("Could not load snapshot or file is not a BSII Control state.",
                                     juce::dontSendNotification);
            }
        }
        fileChooser.reset();
    });
}

void BSIIControlAudioProcessorEditor::applyHardwareUpdates()
{
    for (auto& control : parameterControls)
    {
        const auto index = control->specIndex();
        const auto value = processor.consumeHardwareValue (index);
        if (value < 0) continue;
        lastSentValues[static_cast<size_t> (index)] = value;
        control->setFromHardware (value);
    }

    for (auto& control : choiceControls)
    {
        const auto index = control->choiceIndex();
        const auto wireValue = processor.consumeChoiceWireValue (index);
        if (wireValue < 0) continue;
        if (control->setFromWireValue (wireValue))
        {
            const auto& spec = bsii::choiceSpecs()[static_cast<size_t> (index)];
            for (int i = 0; i < static_cast<int> (spec.choices.size()); ++i)
                if (spec.choices[static_cast<size_t> (i)].wireValue == wireValue)
                    lastSentChoiceIndices[static_cast<size_t> (index)] = i;
        }
    }
}

void BSIIControlAudioProcessorEditor::updateRxDisplay()
{
    const auto rx = processor.getLatestReceivedMidi();
    const auto hex = [] (int value)
    {
        return juce::String::toHexString (value & 0xff).paddedLeft ('0', 2).toUpperCase();
    };

    if (rx.serial != 0 && rx.serial != lastDisplayedRxSerial)
    {
        lastDisplayedRxSerial = rx.serial;
        const auto statusByte = 0xB0 + juce::jlimit (0, 15, rx.channel - 1);
        rxLabel.setText ("CC RX: " + hex (statusByte) + " " + hex (rx.controller) + " " + hex (rx.value)
                         + " | Ch " + juce::String (rx.channel) + " CC " + juce::String (rx.controller)
                         + " = " + juce::String (rx.value), juce::dontSendNotification);
    }

    if (rx.nrpnSerial != 0 && rx.nrpnSerial != lastDisplayedNrpnSerial)
    {
        lastDisplayedNrpnSerial = rx.nrpnSerial;
        nrpnLabel.setText ("NRPN RX: " + juce::String (rx.nrpnMSB) + ":" + juce::String (rx.nrpnLSB)
                           + " = " + juce::String (rx.nrpnValue), juce::dontSendNotification);
    }
}
