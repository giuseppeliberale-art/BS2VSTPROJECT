#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>
#include <atomic>

namespace bsii
{
// Independent of host track MIDI filtering. Configure on the message thread;
// the receiver runs on the native MIDI thread and must not touch the UI.
class HardwareSysExInput : private juce::MidiInputCallback
{
public:
    explicit HardwareSysExInput (std::function<void(const juce::MidiMessage&)> receive,
                                 bool includeLiveControls = false)
        : receiver (std::move (receive)), liveControls (includeLiveControls) {}
    ~HardwareSysExInput() override { disconnect(); }

    bool connect (const juce::String& outputName)
    {
        juce::MidiDeviceInfo match;
        int matches = 0;
        for (const auto& device : juce::MidiInput::getAvailableDevices())
            if (outputName.isNotEmpty() && device.name == outputName)
            {
                match = device;
                ++matches;
            }
        // Input/output identifiers differ. Require an unambiguous name match.
        if (matches != 1) { disconnect(); return false; }
        if (input && input->getIdentifier() == match.identifier) return true;
        disconnect();
        input = juce::MidiInput::openDevice (match.identifier, this);
        if (! input) return false;
        open.store (true, std::memory_order_release);
        input->start();
        return true;
    }
    bool isOpen() const { return open.load (std::memory_order_acquire); }

private:
    void disconnect()
    {
        if (input) input->stop();
        input.reset();
        open.store (false, std::memory_order_release);
    }
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message) override
    {
        // Never intercept notes, clock, pitch wheel or other performance traffic.
        // Plugin instances may opt into controls independent of the device chain.
        if (message.isSysEx() || (liveControls && (message.isController() || message.isProgramChange())))
            receiver (message);
    }
    std::function<void(const juce::MidiMessage&)> receiver;
    const bool liveControls;
    std::unique_ptr<juce::MidiInput> input;
    std::atomic<bool> open { false };
};
}
