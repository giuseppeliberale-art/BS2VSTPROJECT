#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "BSIIMidi.h"
#include "BSIIHardwareSysExInput.h"

class BSIIControlAudioProcessor : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::Timer
{
public:
    BSIIControlAudioProcessor();
    ~BSIIControlAudioProcessor() override;

    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getParameters() { return apvts; }

    bool usesDirectHardwareSysEx() const { return wrapperType != wrapperType_Standalone; }
    bool connectHardwareSysExInput (const juce::String& outputName)
    { return ! usesDirectHardwareSysEx() || directSysExInput.connect (outputName); }
    // Message-thread access for explicit patch/preset commands. The processor
    // retains the port when the editor is destroyed.
    std::unique_ptr<juce::MidiOutput>& hardwareMidiOutput() { return midiOutput; }
    int getHardwareMidiChannel() const { return hardwareMidiChannel.load(); }

    // Reception/import/recall updates parameters without transmitting them.
    // Thread-local scopes leave concurrent automation on other threads enabled.
    class ScopedMidiSilence
    {
    public:
        explicit ScopedMidiSilence (BSIIControlAudioProcessor&, bool discardPendingTrigger = true);
        ~ScopedMidiSilence();
    private:
        BSIIControlAudioProcessor& processor;
        ScopedMidiSilence* previous;
        bool discardPendingTrigger;
        friend class BSIIControlAudioProcessor;
        JUCE_DECLARE_NON_COPYABLE (ScopedMidiSilence)
    };
    void clearPendingMidi(); // Message thread, before explicit full-state sends.
    void setHardwareMidiChannel (int channel);
    void setLiveMidiRxEnabled (bool enabled) { liveMidiRxEnabled.store (enabled, std::memory_order_relaxed); }
    bool isLiveMidiRxEnabled() const { return liveMidiRxEnabled.load (std::memory_order_relaxed); }
    // Returns bsii::noHardwareValue when no update is pending (negative bend is valid).
    int consumeHardwareProgram() { return pendingHardwareProgram.exchange (-1, std::memory_order_acq_rel); }
    int consumeHardwareValue (int specIndex);
    int consumeChoiceWireValue (int choiceIndex);

    struct ReceivedMidi
    {
        int channel = 0;
        int controller = 0;
        int value = 0;
        int nrpnMSB = -1;
        int nrpnLSB = -1;
        int nrpnValue = -1;
        uint32_t serial = 0;
        uint32_t nrpnSerial = 0;
    };

    ReceivedMidi getLatestReceivedMidi() const;

    struct ReceivedSysEx
    {
        juce::MemoryBlock data;
        uint32_t serial = 0;
    };

    ReceivedSysEx getLatestSysEx() const;

private:
    void parameterChanged (const juce::String&, float) override;
    void timerCallback() override;
    bool midiIsSilenced() const;
    static thread_local ScopedMidiSilence* midiSilence;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parseIncomingMidi (const juce::MidiBuffer&, bool acceptSysEx = true);
    void receiveSysEx (const juce::MidiMessage&);
    void publishNRPN (int channel, int nrpnMSB, int nrpnLSB, int value);

    juce::AudioProcessorValueTreeState apvts;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    std::atomic<int> hardwareMidiChannel { 1 };
    std::atomic<int> pendingHardwareProgram { -1 };
    std::atomic<bool> liveMidiRxEnabled { true };

    static constexpr size_t maxSpecs = 96;
    static constexpr size_t maxChoices = 48;
    // Callbacks only publish atomics; native port I/O runs on the message thread.
    std::array<std::atomic<int>, maxSpecs + maxChoices> pendingMidi {};
    std::atomic<int> pendingTrigger { -1 };
    std::array<std::atomic<int>, maxSpecs> pendingHardwareValues {};
    std::array<std::atomic<int>, maxChoices> pendingChoiceWireValues {};
    std::array<std::atomic<int>, 128> pairedMSB {};

    std::array<std::atomic<int>, 17> currentNrpnMSB {};
    std::array<std::atomic<int>, 17> currentNrpnLSB {};

    std::atomic<int> lastRxChannel { 0 };
    std::atomic<int> lastRxController { 0 };
    std::atomic<int> lastRxValue { 0 };
    std::atomic<uint32_t> lastRxSerial { 0 };

    std::atomic<int> lastNrpnMSB { -1 };
    std::atomic<int> lastNrpnLSB { -1 };
    std::atomic<int> lastNrpnValue { -1 };
    std::atomic<uint32_t> lastNrpnSerial { 0 };

    mutable juce::CriticalSection sysExLock;
    juce::MemoryBlock lastSysExData;
    std::atomic<uint32_t> lastSysExSerial { 0 };

    // Declared last: stop native callbacks before destroying receive state.
    bsii::HardwareSysExInput directSysExInput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BSIIControlAudioProcessor)
};
