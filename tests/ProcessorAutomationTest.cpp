#include <CoreFoundation/CoreFoundation.h>
#include "PluginProcessor.h"
#include <iostream>
#include <thread>

namespace
{
void pump (int ms = 100) { CFRunLoopRunInMode (kCFRunLoopDefaultMode, ms / 1000.0, false); }
struct Capture : juce::MidiInputCallback
{
    juce::CriticalSection lock;
    juce::StringArray lines;
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message) override
    {
        const juce::ScopedLock guard (lock);
        lines.add (juce::String::toHexString (message.getRawData(), message.getRawDataSize()));
    }
};
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    if (argc == 4 && juce::String (argv[1]) == "--sink")
    {
        Capture capture;
        auto input = juce::MidiInput::createNewDevice (argv[2], &capture);
        if (! input) return 2;
        input->start();
        const juce::File log (argv[3]);
        const auto stop = log.withFileExtension ("stop");
        for (int i = 0; i < 1500 && ! stop.existsAsFile(); ++i) pump (20);
        input->stop();
        return log.replaceWithText (capture.lines.joinIntoString ("\n")) ? 0 : 2;
    }
    const auto name = "BSII isolated automation " + juce::Uuid().toString();
    const auto log = juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getNonexistentChildFile ("bsii-automation", ".txt");
    juce::ChildProcess sink;
    if (! sink.start (juce::StringArray {
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName(),
        "--sink", name, log.getFullPathName() })) return 2;

    bool ok = true;
    juce::StringArray expected;
    juce::MemoryBlock savedSession;
    {
        BSIIControlAudioProcessor processor;
        for (int n = 0; n < 100 && ! processor.hardwareMidiOutput(); ++n)
        {
            for (const auto& port : juce::MidiOutput::getAvailableDevices())
                if (port.name == name) processor.hardwareMidiOutput() = juce::MidiOutput::openDevice (port.identifier);
            if (! processor.hardwareMidiOutput()) pump (20);
        }
        if (! processor.hardwareMidiOutput()) { std::cerr << "FAIL virtual output unavailable\n"; return 1; }
        auto set = [&] (const char* id, int value)
        {
            auto* p = processor.getParameters().getParameter (id);
            p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (value)));
        };
        auto cc = [&] (int controller, int value, int channel = 1)
        {
            auto m = juce::MidiMessage::controllerEvent (channel, controller, value);
            expected.add (juce::String::toHexString (m.getRawData(), m.getRawDataSize()));
        };
        auto nrpn = [&] (int lsb, int value) { cc (99, 0); cc (98, lsb); cc (6, value); };

        set ("filterResonance", 10); pump(); cc (82, 10); // No editor ever created.
        auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
        pump();
        editor.reset(); pump();
        std::thread automation ([&] { set ("filterResonance", 20); }); automation.join();
        pump(); cc (82, 20); // Editor destroyed; non-message-thread callback.
        {
            BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);
            set ("filterResonance", 30);
        }
        pump();
        set ("filterResonance", 20); pump(); cc (82, 20); // No stale sent-value cache.
        set ("osc1Wave", 1); pump(); nrpn (72, 1);
        set ("filterCutoff", 128); pump(); cc (16, 64); cc (48, 0);
        set ("ampTrigger", 0); pump(); nrpn (73, 1); nrpn (105, 1);
        ok &= processor.getParameters().getRawParameterValue ("modTrigger")->load() == 0;
        {
            BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);
            set ("filterResonance", 42);
            set ("ampTrigger", 1);
            set ("modTrigger", 2);
        }
        juce::MemoryBlock state;
        processor.getStateInformation (state);
        set ("filterResonance", 43); pump(); cc (82, 43);
        processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        pump(); // Host recall stays silent, and mixed stored triggers survive.
        ok &= processor.getParameters().getRawParameterValue ("filterResonance")->load() == 42;
        ok &= processor.getParameters().getRawParameterValue ("ampTrigger")->load() == 1;
        ok &= processor.getParameters().getRawParameterValue ("modTrigger")->load() == 2;
        set ("filterResonance", 50);
        {
            BSIIControlAudioProcessor::ScopedMidiSilence silence (processor);
            set ("filterResonance", 51);
        }
        pump(); // Reception cancels stale pending sends for the same control.
        editor.reset (processor.createEditor()); pump();
        juce::MidiBuffer incoming;
        incoming.addEvent (juce::MidiMessage::controllerEvent (1, 82, 60), 0);
        juce::AudioBuffer<float> audio (2, 64);
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("filterResonance")->load() == 60;
        processor.setHardwareMidiChannel (2);
        processor.setLiveMidiRxEnabled (false);
        editor.reset();
        editor.reset (processor.createEditor()); pump();
        ok &= processor.getHardwareMidiChannel() == 2 && ! processor.isLiveMidiRxEnabled();
        editor.reset();
        set ("filterResonance", 44); pump(); cc (82, 44, 2);
        processor.rememberHardwareMidiOutput (processor.hardwareMidiOutput()->getIdentifier(), name);
        processor.setHardwareMidiChannel (3);
        processor.getStateInformation (savedSession);
    }
    {
        BSIIControlAudioProcessor fresh;
        juce::String portId;
        for (const auto& port : juce::MidiOutput::getAvailableDevices())
            if (port.name == name) portId = port.identifier;
        fresh.setStateInformation (savedSession.getData(), static_cast<int> (savedSession.getSize()));
        pump (200);
        const bool connected = fresh.hardwareMidiOutput() != nullptr;
        if (! connected) std::cerr << "FAIL: saved output did not reconnect without editor\n";
        ok &= connected && fresh.getHardwareMidiChannel() == 3 && ! fresh.isLiveMidiRxEnabled();
        auto* resonance = fresh.getParameters().getParameter ("filterResonance");
        resonance->setValueNotifyingHost (resonance->convertTo0to1 (45));
        pump();
        auto message = juce::MidiMessage::controllerEvent (3, 82, 45);
        expected.add (juce::String::toHexString (message.getRawData(), message.getRawDataSize()));
        juce::MemoryBlock roundtrip;
        fresh.getStateInformation (roundtrip);
        auto xml = juce::AudioProcessor::getXmlFromBinary (roundtrip.getData(), static_cast<int> (roundtrip.getSize()));
        ok &= xml && xml->getStringAttribute ("bsiiMidiOutputId") == portId;
        if (xml)
        {
            xml->setAttribute ("bsiiMidiOutputId", "obsolete endpoint identifier");
            juce::MemoryBlock renamed;
            juce::AudioProcessor::copyXmlToBinary (*xml, renamed);
            fresh.setStateInformation (renamed.getData(), static_cast<int> (renamed.getSize()));
            pump (200);
            ok &= fresh.hardwareMidiOutput() && fresh.hardwareMidiOutput()->getIdentifier() == portId;
            xml->setAttribute ("bsiiMidiOutputName", "missing saved device");
            juce::AudioProcessor::copyXmlToBinary (*xml, renamed);
            fresh.setStateInformation (renamed.getData(), static_cast<int> (renamed.getSize()));
            pump (200);
            auto editor = std::unique_ptr<juce::AudioProcessorEditor> (fresh.createEditor());
            pump();
            ok &= fresh.hardwareMidiOutput() == nullptr; // Do not select a different synth.
        }
    }
    log.withFileExtension ("stop").replaceWithText ("stop");
    sink.waitForProcessToFinish (5000);
    const auto actual = juce::StringArray::fromLines (log.loadFileAsString());
    ok &= actual == expected;
    if (actual != expected)
        std::cerr << "Expected:\n" << expected.joinIntoString ("\n") << "\nActual:\n" << actual.joinIntoString ("\n") << "\n";
    log.deleteFile(); log.withFileExtension ("stop").deleteFile();
    std::cout << (ok ? "PASS" : "FAIL") << " processor automation before/after editor, native CC/paired CC/NRPN, linked triggers, silent receive/recall, channel persistence, saved connection without editor and exact-name fallback\n";
    return ok ? 0 : 1;
}
