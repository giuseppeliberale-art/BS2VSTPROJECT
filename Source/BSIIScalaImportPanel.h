#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "BSIIScalaImport.h"

// Temporary import session. Preview is never part of the processor or MIDI state.
class ScalaImportPanel : public juce::Component, private juce::TableListBoxModel
{
public:
    explicit ScalaImportPanel (BSIIControlAudioProcessor&);
    ~ScalaImportPanel() override;
    void resized() override;
    bool loadScaleFile (const juce::File&);
    bool loadMappingFile (const juce::File&);
    void rebuildPreview();
    bool applyPreview();

private:
    int getNumRows() override { return preview && preview->error.empty() ? 128 : 0; }
    void paintRowBackground (juce::Graphics&, int, int, int, bool) override;
    void paintCell (juce::Graphics&, int, int, int, int, bool) override;
    void chooseFile (bool mapping);
    void invalidate();
    juce::String referenceSettings() const;
    void updateApply();
    void message (const juce::String&);
    BSIIControlAudioProcessor& processor;
    BSIIControlAudioProcessor::LocalTuningInfo baseline;
    std::optional<bsii::microtuning::scala::Preview> preview;
    std::string scaleText;
    std::optional<std::string> mappingText;
    juce::Label files, referenceHint, summary, policyHint;
    juce::TextButton scaleButton { "Choose .scl..." }, mappingButton { "Choose .kbm..." },
        clearMapping { "No KBM" }, previewButton { "Preview" }, applyButton { "Apply to Local Table" };
    juce::TextEditor rootNote, referenceNote, referenceHz;
    juce::ToggleButton keepUnavailable { "Keep existing pitches for unavailable notes" };
    juce::TableListBox table { "Scala preview", this };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String scaleName, mappingName, previewSettings;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScalaImportPanel)
};
