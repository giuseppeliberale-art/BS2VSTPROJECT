#include "BSIIHardwareSysExInput.h"
#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    const bool sourceMode = argc == 4 && juce::String::fromUTF8 (argv[1]) == "--source";
    if (argc != 2 && ! sourceMode) return 2;
    const auto patchPath = juce::String::fromUTF8 (argv[sourceMode ? 3 : 1]);
    juce::MemoryBlock expected;
    if (! juce::File (patchPath).loadFileAsData (expected)) return 2;
    if (sourceMode)
    {
        auto source = juce::MidiOutput::createNewDevice (juce::String::fromUTF8 (argv[2]));
        if (! source) return 2;
        juce::Thread::sleep (1500);
        source->sendMessageNow (juce::MidiMessage::controllerEvent (1, 16, 70));
        source->sendMessageNow (juce::MidiMessage::programChange (1, 23));
        source->sendMessageNow (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)));
        source->sendMessageNow (juce::MidiMessage (expected.getData(), static_cast<int> (expected.getSize()), 0.0));
        juce::Thread::sleep (1500);
        return 0;
    }
    // JUCE excludes its own virtual ports from discovery. Use a separate process
    // to exercise the same OS endpoint discovery as a real external MIDI device.
    const auto name = "BSII isolated receive test " + juce::String (juce::Time::getMillisecondCounter());
    juce::ChildProcess source;
    if (! source.start (juce::StringArray {
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName(),
        "--source", name, juce::File (patchPath).getFullPathName() })) return 2;
    for (int i = 0; i < 50; ++i)
    {
        bool found = false;
        for (const auto& port : juce::MidiInput::getAvailableDevices()) found |= port.name == name;
        if (found) break;
        juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    }
    std::atomic<int> received { 0 };
    juce::MemoryBlock actual;
    bsii::HardwareSysExInput input ([&] (const juce::MidiMessage& message)
    {
        actual = juce::MemoryBlock (message.getRawData(), static_cast<size_t> (message.getRawDataSize()));
        received.fetch_add (1, std::memory_order_release);
    });
    if (! input.connect (name))
    {
        std::cerr << "FAIL: direct patch receive input did not connect; requested " << name << "\n";
        for (const auto& port : juce::MidiInput::getAvailableDevices())
            std::cerr << "Available input: " << port.name << "\n";
        return 1;
    }
    for (int i = 0; i < 150 && received.load (std::memory_order_acquire) == 0; ++i)
        juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    input.connect ({}); // Stop callbacks before comparing payload or testing teardown.
    const bool payloadOK = received.load() == 1 && actual == expected;
    const bool stopped = ! input.isOpen();
    const bool reopen = input.connect (name) && input.isOpen();
    const bool missing = ! input.connect (name + " missing") && ! input.isOpen();
    source.waitForProcessToFinish (5000);
    std::cout << (payloadOK && stopped && reopen && missing ? "PASS" : "FAIL")
              << " direct SysEx receipt without host MIDI; CC/notes/programs excluded; close/reopen/missing port\n";
    return payloadOK && stopped && reopen && missing ? 0 : 1;
}
