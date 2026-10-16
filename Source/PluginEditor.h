#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "BSIIPresetSync.h"

class ParameterControl : public juce::Component
{
public:
    ParameterControl (juce::AudioProcessorValueTreeState& state,
                      const bsii::ParameterSpec& parameterSpec,
                      int index,
                      std::function<void(int, int)> valueChanged);
    void resized() override;
    void setFromHardware (int value);
    int specIndex() const { return index; }
private:
    void updateDiscreteSwitch();
    int index;
    const bsii::ParameterSpec& spec;
    juce::Label label;
    juce::Slider slider;
    juce::TextButton firstChoice, secondChoice;
    bool isOctaveSwitch = false;
    bool isSyncSwitch = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::function<void(int, int)> callback;
    bool applyingHardware = false;
};

class ChoiceControl : public juce::Component
{
public:
    ChoiceControl (juce::AudioProcessorValueTreeState& state,
                   const bsii::ChoiceSpec& choiceSpec,
                   int index,
                   std::function<void(int, int)> choiceChanged);
    void resized() override;
    bool setFromWireValue (int wireValue);
    int choiceIndex() const { return index; }
    void setInteractionEnabled (bool enabled);
private:
    int index;
    const bsii::ChoiceSpec& spec;
    juce::Label label;
    juce::ComboBox combo;
    juce::Label note;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    std::function<void(int, int)> callback;
    bool applyingHardware = false;
};

class ParameterSection : public juce::Component
{
public:
    explicit ParameterSection (juce::String titleText);
    void addControl (juce::Component* control);
    void resized() override;
    void paint (juce::Graphics&) override;
    int preferredHeight (int width) const;
private:
    juce::String title;
    std::vector<juce::Component*> controls;
};

class EditorPage : public juce::Component
{
public:
    EditorPage();
    void addSection (ParameterSection* section);
    void resized() override;
private:
    juce::Component content;
    juce::Viewport viewport;
    std::vector<ParameterSection*> sections;
};

class BSIIControlAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit BSIIControlAudioProcessorEditor (BSIIControlAudioProcessor&);
    ~BSIIControlAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshMidiOutputs();
    void autoSelectBassStation();
    void selectMidiOutput (int comboBoxId);
    bool reopenSelectedMidiOutput (bool forceReopen = false);
    void requestPatchFromHardware();
    void selectPreset (int program, bool sendToHardware);
    void forgetPreset();
    void sendParameterValue (int specIndex, int value);
    void sendChoiceValue (int choiceIndex, int selectedChoiceIndex);
    void sendAllStateToHardware();
    void saveSnapshot();
    void loadSnapshot();
    void armSysExCapture();
    void saveLastSysEx();
    void sendSysExFileToHardware();
    void importSysExToEditor();
    bool decodeSysExIntoEditor (const juce::MemoryBlock& data, const juce::String& sourceName);
    bool setEditorParameterValue (const char* parameterId, int value);
    void updateSysExDisplay();
    static juce::String hexPreview (const juce::MemoryBlock&, int maxBytes = 28);
    bool saveStateToFile (const juce::File& file);
    bool loadStateFromFile (const juce::File& file);
    int currentParameterValue (const char* parameterId) const;
    void applyHardwareUpdates();
    void updateRxDisplay();
    void updateFilterModeUi();
    void setLiveRxPaused (bool paused);
    void buildPages();

    BSIIControlAudioProcessor& processor;

    juce::Label titleLabel, subtitleLabel, statusLabel, rxLabel, nrpnLabel, sysexLabel, midiLabel, channelLabel, stateHintLabel, diagnosticLabel;
    juce::ComboBox midiOutputBox, channelBox, presetBox;
    juce::Label presetLabel, presetHint;
    juce::TextButton previousPresetButton { "Previous" }, nextPresetButton { "Next" }, loadPresetButton { "Load Preset" };
    bsii::PresetSync presetSync;
    juce::TextButton refreshButton { "Refresh MIDI" };
    juce::ToggleButton pauseLiveRxButton { "Live RX is running" };
    juce::TextButton saveButton { "Save Snapshot…" };
    juce::TextButton loadButton { "Load Snapshot…" };
    juce::TextButton sendAllButton { "Send All → BSII" };
    juce::TextButton getPatchButton { "Get Patch from BSII" };
    juce::TextButton armSysExButton { "Arm Patch Capture" };
    juce::TextButton saveSysExButton { "Save Last .syx…" };
    juce::TextButton sendSysExButton { "Send .syx → BSII" };
    juce::TextButton importSysExButton { "Import .syx → Editor" };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    std::vector<std::unique_ptr<ParameterControl>> parameterControls;
    std::vector<std::unique_ptr<ChoiceControl>> choiceControls;
    std::vector<std::unique_ptr<ParameterSection>> sections;
    std::vector<std::unique_ptr<EditorPage>> pages;

    juce::Array<juce::MidiDeviceInfo> midiOutputs;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::String selectedDeviceIdentifier;
    std::unique_ptr<juce::FileChooser> fileChooser;

    int midiChannel = 1;
    uint32_t lastDisplayedRxSerial = 0;
    uint32_t lastDisplayedNrpnSerial = 0;
    uint32_t lastDisplayedSysExSerial = 0;
    uint32_t captureArmSerial = 0;
    bool sysExCaptureArmed = false;
    bool patchRequestPending = false;
    uint32_t patchRequestStartSerial = 0;
    uint32_t patchRequestStartedMs = 0;
    uint32_t lastReconnectAttemptMs = 0;
    std::vector<int> lastSentValues;
    std::vector<int> lastSentChoiceIndices;
    bool suppressMidiSends = false;
    bool liveRxPaused = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BSIIControlAudioProcessorEditor)
};
