#include <CoreFoundation/CoreFoundation.h>
#include "PluginProcessor.h"
#include "BSIIPatchDecoder.h"
#include "BSIIProjectRecall.h"
#include <iostream>
#include <thread>

namespace
{
void pump (int ms = 100) { CFRunLoopRunInMode (kCFRunLoopDefaultMode, ms / 1000.0, false); }
struct HostEdits : juce::AudioProcessorListener
{
    int count = 0;
    int gestureStarts = 0, gestureEnds = 0;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override { ++count; }
    void audioProcessorParameterChangeGestureBegin (juce::AudioProcessor*, int) override { ++gestureStarts; }
    void audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int) override { ++gestureEnds; }
    void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override {}
};
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
    if (argc == 2 && juce::String (argv[1]) == "--hardware-gesture-probe")
    {
        BSIIControlAudioProcessor processor;
        HostEdits edits;
        processor.addListener (&edits);
        juce::MidiBuffer incoming;
        incoming.addEvent (juce::MidiMessage::controllerEvent (1, 16, 20), 0);
        incoming.addEvent (juce::MidiMessage::controllerEvent (1, 48, 64), 1);
        juce::AudioBuffer<float> audio (2, 64);
        processor.processBlock (audio, incoming);
        // Queue the hardware-fallback message before an editor can auto-open a
        // physical direct input. The editor then consumes the real update queue.
        auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
        pump (300);
        processor.removeListener (&edits);
        const bool ok = edits.count > 0 && edits.gestureStarts > 0
            && edits.gestureStarts == edits.gestureEnds;
        std::cout << (ok ? "PASS" : "FAIL") << " hardware cutoff host notifications: values="
            << edits.count << " touch-start=" << edits.gestureStarts
            << " touch-end=" << edits.gestureEnds << "\n";
        return ok ? 0 : 1;
    }
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
    if (argc == 4 && juce::String (argv[1]) == "--controls-source")
    {
        auto output = juce::MidiOutput::createNewDevice (argv[2]);
        if (! output) return 2;
        const juce::File command (argv[3]);
        juce::String previous;
        auto cc = [&] (int channel, int controller, int value)
        { output->sendMessageNow (juce::MidiMessage::controllerEvent (channel, controller, value)); };
        for (int n = 0; n < 3000; ++n)
        {
            const auto action = command.loadFileAsString();
            if (action == "stop") return 0;
            if (action.isNotEmpty() && action != previous)
            {
                previous = action;
                if (action == "live")
                {
                    output->sendMessageNow (juce::MidiMessage::programChange (1, 23));
                    cc (1, 82, 55); cc (1, 16, 60); cc (1, 48, 64);
                    cc (1, 99, 0); cc (1, 98, 72); cc (1, 6, 1);
                    output->sendMessageNow (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)));
                    output->sendMessageNow (juce::MidiMessage::pitchWheel (1, 9000));
                    output->sendMessageNow (juce::MidiMessage::midiClock());
                }
                if (action == "paused")
                {
                    cc (1, 82, 66);
                    output->sendMessageNow (juce::MidiMessage::programChange (1, 30));
                    const juce::uint8 bytes[] { 0, 0x20, 0x29, 0, 0x33, 0 };
                    output->sendMessageNow (juce::MidiMessage::createSysExMessage (bytes, sizeof (bytes)));
                }
                if (action == "channel")
                {
                    output->sendMessageNow (juce::MidiMessage::programChange (1, 20)); cc (1, 82, 70);
                    output->sendMessageNow (juce::MidiMessage::programChange (2, 24)); cc (2, 82, 77);
                }
                command.withFileExtension ("done").replaceWithText (action);
            }
            pump (10);
        }
        return 2;
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
        // Existing host indices must survive appending the live-only Arp/Seq choices.
        ok &= processor.getParameters().getParameter ("lfo2KeySync")->getParameterIndex() == 75;
        ok &= processor.getParameters().getParameter ("arpOn")->getParameterIndex() == 76;
        set ("arpOn", 1); pump(); cc (108, 1);
        set ("arpLatch", 1); pump(); cc (109, 1);
        set ("arpOctaves", 3); pump(); cc (111, 4);
        set ("arpMode", 5); pump(); cc (118, 5);
        set ("arpRhythm", 31); pump(); cc (119, 31);
        set ("seqRetrig", 0); pump(); nrpn (106, 0);
        ok &= processor.getParameters().getParameter ("seqRetrig")->getParameterIndex() == 81;
        ok &= processor.getParameters().getParameter ("lfo1Sync")->getParameterIndex() == 82;
        set ("lfo1Sync", 1); pump(); nrpn (88, 1);
        set ("lfo2Sync", 1); pump(); nrpn (92, 1);
        set ("lfo1Division", 25); pump(); nrpn (87, 25);
        set ("lfo2Division", 34); pump(); nrpn (91, 34);
        ok &= processor.getParameters().getParameter ("lfo2Division")->getParameterIndex() == 85;
        ok &= processor.getParameters().getParameter ("modWhFilter")->getParameterIndex() == 86;
        ok &= processor.getParameters().getParameter ("modWhLfo2Filter")->getParameterIndex() == 87;
        set ("modWhFilter", 0); pump(); nrpn (94, 0);
        set ("modWhLfo2Filter", 127); pump(); nrpn (71, 127);
        ok &= processor.getParameters().getParameter ("microTuneTable")->getParameterIndex() == 88;
        set ("microTuneTable", 8); pump();
        cc (100, 3); cc (101, 0); cc (6, 8); cc (100, 127); cc (101, 127);
        ok &= processor.getParameters().getParameter ("oscError")->getParameterIndex() == 89;
        ok &= processor.getParameters().getParameter ("glideDivergence")->getParameterIndex() == 90;
        set ("oscError", 7); pump(); nrpn (111, 7);
        set ("glideDivergence", 15); pump(); nrpn (113, 15);
        ok &= processor.getParameters().getParameter ("subFine")->getParameterIndex() == 91;
        ok &= processor.getParameters().getParameter ("subCoarse")->getParameterIndex() == 92;
        set ("subFine", 27); pump(); nrpn (77, 13); cc (38, 64);
        set ("subCoarse", 255); pump(); nrpn (84, 127); cc (38, 64);
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
        ok &= processor.getParameters().getRawParameterValue ("arpOctaves")->load() == 3;
        ok &= processor.getParameters().getRawParameterValue ("arpRhythm")->load() == 31;
        ok &= processor.getParameters().getRawParameterValue ("lfo1Sync")->load() == 1;
        ok &= processor.getParameters().getRawParameterValue ("lfo2Sync")->load() == 1;
        ok &= processor.getParameters().getRawParameterValue ("lfo1Division")->load() == 25;
        ok &= processor.getParameters().getRawParameterValue ("lfo2Division")->load() == 34;
        ok &= processor.getParameters().getRawParameterValue ("modWhFilter")->load() == 0;
        ok &= processor.getParameters().getRawParameterValue ("modWhLfo2Filter")->load() == 127;
        ok &= processor.getParameters().getRawParameterValue ("microTuneTable")->load() == 8;
        ok &= processor.getParameters().getRawParameterValue ("oscError")->load() == 7;
        ok &= processor.getParameters().getRawParameterValue ("glideDivergence")->load() == 15;
        ok &= processor.getParameters().getRawParameterValue ("subFine")->load() == 27;
        ok &= processor.getParameters().getRawParameterValue ("subCoarse")->load() == 255;
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
        incoming.clear();
        for (const auto [controller, value] : { std::pair {99, 0}, {98, 0}, {6, 3} })
            incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, value), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("microTuneTable")->load() == 3;
        // RPN data entry must not be decoded as a stale MicroTune NRPN.
        incoming.clear();
        for (const auto [controller, value] : { std::pair {100, 3}, {101, 0}, {6, 7} })
            incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, value), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("microTuneTable")->load() == 3;
        incoming.clear();
        for (const auto address : {111, 113})
            for (const auto [controller, value] : { std::pair {99, 0}, {98, address}, {6, 0} })
                incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, value), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("oscError")->load() == 0;
        ok &= processor.getParameters().getRawParameterValue ("glideDivergence")->load() == 0;
        const auto osc1FineBefore = processor.getParameters().getRawParameterValue ("osc1Fine")->load();
        incoming.clear();
        for (const auto [controller, value] : { std::pair {99, 0}, {98, 77}, {38, 64}, {6, 100} })
            incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, value), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        // Orphaned LSB and MSB-only reception must not publish partial tuning.
        ok &= processor.getParameters().getRawParameterValue ("subFine")->load() == 27;
        incoming.clear();
        incoming.addEvent (juce::MidiMessage::controllerEvent (1, 38, 64), 0);
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("subFine")->load() == 201;
        ok &= processor.getParameters().getRawParameterValue ("osc1Fine")->load() == osc1FineBefore;
        incoming.clear();
        for (const auto [controller, value] : { std::pair {99, 0}, {98, 84}, {6, 0}, {38, 64} })
            incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, value), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("subCoarse")->load() == 1;
        ok &= processor.getParameters().getRawParameterValue ("subFine")->load() == 201;
        // Incoming captured encodings update the attached editor/APVTS without MIDI echo.
        incoming.clear();
        for (const auto [controller, value] : { std::pair {108, 0}, {109, 0}, {111, 2}, {118, 4}, {119, 0}, {99, 0}, {98, 106}, {6, 1} })
            incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, value), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        for (const auto [id, value] : { std::pair {"arpOn", 0}, {"arpLatch", 0}, {"arpOctaves", 1}, {"arpMode", 4}, {"arpRhythm", 0}, {"seqRetrig", 1} })
            ok &= processor.getParameters().getRawParameterValue (id)->load() == value;
        incoming.clear();
        for (const auto [address, value] : { std::pair {88, 0}, {92, 0}, {87, 34}, {91, 25} })
            for (const auto [controller, data] : { std::pair {99, 0}, {98, address}, {6, value} })
                incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, data), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("lfo1Sync")->load() == 0;
        ok &= processor.getParameters().getRawParameterValue ("lfo2Sync")->load() == 0;
        ok &= processor.getParameters().getRawParameterValue ("lfo1Division")->load() == 34;
        ok &= processor.getParameters().getRawParameterValue ("lfo2Division")->load() == 25;
        incoming.clear();
        for (const auto [address, value] : { std::pair {94, 127}, {71, 0} })
            for (const auto [controller, data] : { std::pair {99, 0}, {98, address}, {6, value} })
                incoming.addEvent (juce::MidiMessage::controllerEvent (1, controller, data), incoming.getNumEvents());
        processor.processBlock (audio, incoming); pump();
        ok &= processor.getParameters().getRawParameterValue ("modWhFilter")->load() == 127;
        ok &= processor.getParameters().getRawParameterValue ("modWhLfo2Filter")->load() == 0;
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
    {
        juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
        BSIIControlAudioProcessor receiver;
        juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Undefined);
        for (const auto& port : juce::MidiOutput::getAvailableDevices())
            if (port.name == name) receiver.hardwareMidiOutput() = juce::MidiOutput::openDevice (port.identifier);
        auto editor = std::unique_ptr<juce::AudioProcessorEditor> (receiver.createEditor());
        auto* resonance = receiver.getParameters().getParameter ("filterResonance");
        resonance->setValueNotifyingHost (resonance->convertTo0to1 (99));
        pump();
        auto addCC = [&] (int value)
        {
            auto message = juce::MidiMessage::controllerEvent (1, 82, value);
            expected.add (juce::String::toHexString (message.getRawData(), message.getRawDataSize()));
        };
        addCC (99);
        for (auto* child : editor->getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*> (child))
                if (button->getButtonText() == "Get Patch from BSII") button->triggerClick();
        pump();
        expected.add ("f0 00 20 29 00 33 00 40 f7");
        juce::MemoryBlock patch;
        ok &= juce::File::getCurrentWorkingDirectory().getChildFile ("tests/arp-seq-fixtures/seq-retrig-on.syx").loadFileAsData (patch);
        const auto decoded = bsii::decodePatchFields (patch);
        HostEdits edits;
        receiver.addListener (&edits);
        juce::MidiBuffer incoming;
        incoming.addEvent (juce::MidiMessage (patch.getData(), static_cast<int> (patch.getSize()), 0.0), 0);
        juce::AudioBuffer<float> audio (2, 64);
        receiver.processBlock (audio, incoming); pump (200);
        if (edits.count != 0) std::cerr << "FAIL: patch refresh reported " << edits.count << " manual parameter edits to host\n";
        ok &= edits.count == 0 && decoded.valid
            && juce::roundToInt (receiver.getParameters().getRawParameterValue ("filterResonance")->load()) == decoded.filterResonance;
        for (const auto [id, value] : { std::pair {"arpOn", decoded.arpOn}, {"arpLatch", decoded.arpLatch}, {"arpOctaves", decoded.arpOctaves}, {"arpMode", decoded.arpMode}, {"arpRhythm", decoded.arpRhythm}, {"seqRetrig", decoded.seqRetrig} })
            if (value >= 0) ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue (id)->load()) == value;
        for (const auto [id, value] : { std::pair {"lfo1Sync", decoded.lfo1Sync}, {"lfo2Sync", decoded.lfo2Sync}, {"lfo1Division", decoded.lfo1Division}, {"lfo2Division", decoded.lfo2Division} })
            if (value >= 0) ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue (id)->load()) == value;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("modWhFilter")->load()) == decoded.modWhFilter;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("modWhLfo2Filter")->load()) == decoded.modWhLfo2Filter;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("microTuneTable")->load()) == decoded.microTuneTable;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("subFine")->load()) == decoded.subFine;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("subCoarse")->load()) == decoded.subCoarse;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("oscError")->load()) == decoded.oscError;
        ok &= juce::roundToInt (receiver.getParameters().getRawParameterValue ("glideDivergence")->load()) == decoded.glideDivergence;
        incoming.clear();
        incoming.addEvent (juce::MidiMessage::controllerEvent (1, 82, 60), 0);
        incoming.addEvent (juce::MidiMessage::controllerEvent (1, 83, 1), 1);
        receiver.processBlock (audio, incoming); pump();
        ok &= edits.count == 2 // Physical knob edits retain their host notifications.
            && juce::roundToInt (receiver.getParameters().getRawParameterValue ("filterResonance")->load()) == 60
            && juce::roundToInt (receiver.getParameters().getRawParameterValue ("filterType")->load()) == 1;
        resonance->setValueNotifyingHost (resonance->convertTo0to1 (23)); pump(); addCC (23);
        ok &= edits.count == 3; // Ordinary user edits still notify the host.
        receiver.removeListener (&edits);
        editor.reset();
        incoming.clear();
        incoming.addEvent (juce::MidiMessage::programChange (1, 23), 0);
        receiver.processBlock (audio, incoming);
        ok &= receiver.consumeHardwareProgram() == 23;
        editor.reset (receiver.createEditor());
        bool presetShown = false;
        for (auto* child : editor->getChildren())
            if (auto* combo = dynamic_cast<juce::ComboBox*> (child))
                if (combo->getNumItems() == 128) presetShown = combo->getSelectedId() == 24;
        ok &= presetShown && receiver.getKnownHardwareProgram() == 23;
    }
    {
        using Mode = BSIIControlAudioProcessor::RecallMode;
        juce::MemoryBlock patch;
        ok &= juce::File::getCurrentWorkingDirectory().getChildFile ("tests/state-fixtures/baseline.syx").loadFileAsData (patch);
        ok &= bsii::isRecallPatch (patch);
        auto command = patch;
        static_cast<juce::uint8*> (command.getData())[7] = 1;
        ok &= ! bsii::isRecallPatch (command); // Never accept a different write command.
        command = patch;
        static_cast<juce::uint8*> (command.getData())[20] = 0x80;
        ok &= ! bsii::isRecallPatch (command);
        ok &= ! bsii::isRecallPatch (juce::MemoryBlock (patch.getData(), 153));
        juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
        BSIIControlAudioProcessor capture;
        juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Undefined);
        for (const auto& port : juce::MidiOutput::getAvailableDevices())
            if (port.name == name) capture.hardwareMidiOutput() = juce::MidiOutput::openDevice (port.identifier);
        capture.rememberHardwareMidiOutput (capture.hardwareMidiOutput()->getIdentifier(), name);
        capture.setHardwareMidiChannel (3);
        ok &= ! capture.configureProjectRecall (Mode::preset, -1);
        ok &= ! capture.configureProjectRecall (Mode::capturedSound, -1);
        juce::AudioBuffer<float> audio (2, 64);
        juce::MidiBuffer incoming;
        incoming.addEvent (juce::MidiMessage (patch.getData(), static_cast<int> (patch.getSize()), 0.0), 0);
        capture.processBlock (audio, incoming); // Old receipt cannot satisfy fresh capture.
        ok &= capture.captureProjectSound();
        expected.add ("f0 00 20 29 00 33 00 40 f7");
        pump();
        ok &= capture.getProjectRecallInfo().capturing && ! capture.getProjectRecallInfo().hasCapture;
        incoming.addEvent (juce::MidiMessage (patch.getData(), static_cast<int> (patch.getSize()), 0.0), 0);
        capture.processBlock (audio, incoming); pump();
        ok &= capture.getProjectRecallInfo().hasCapture && ! capture.getProjectRecallInfo().capturing;
        ok &= capture.configureProjectRecall (Mode::capturedSound, -1);
        juce::MemoryBlock capturedState;
        capture.getStateInformation (capturedState);
        auto tree = capture.getParameters().copyState();
        ok &= ! tree.hasProperty ("bsiiProjectSound"); // Portable parameter snapshots exclude recall.
        {
            BSIIControlAudioProcessor fresh;
            HostEdits edits;
            fresh.addListener (&edits);
            fresh.setStateInformation (capturedState.getData(), static_cast<int> (capturedState.getSize()));
            const int recallEdits = edits.count;
            auto* resonance = fresh.getParameters().getParameter ("filterResonance");
            resonance->setValueNotifyingHost (resonance->convertTo0to1 (45));
            pump (450);
            expected.add (juce::String::toHexString (patch.getData(), static_cast<int> (patch.getSize())));
            auto message = juce::MidiMessage::controllerEvent (3, 82, 45);
            expected.add (juce::String::toHexString (message.getRawData(), message.getRawDataSize()));
            pump (400); // Exactly one recall, before pending automation; no editor.
            ok &= fresh.getProjectRecallInfo().hasCapture && edits.count == recallEdits + 1;
            fresh.removeListener (&edits);
        }
        ok &= capture.configureProjectRecall (Mode::preset, 23);
        juce::MemoryBlock presetState;
        capture.getStateInformation (presetState);
        {
            BSIIControlAudioProcessor fresh;
            fresh.setStateInformation (presetState.getData(), static_cast<int> (presetState.getSize()));
            pump (450);
            auto message = juce::MidiMessage::programChange (3, 23);
            expected.add (juce::String::toHexString (message.getRawData(), message.getRawDataSize()));
            ok &= fresh.getKnownHardwareProgram() == 23;
            pump (400);
        }
        auto xml = juce::AudioProcessor::getXmlFromBinary (capturedState.getData(), static_cast<int> (capturedState.getSize()));
        xml->setAttribute ("bsiiProjectSound", "invalid patch");
        juce::MemoryBlock corrupt;
        juce::AudioProcessor::copyXmlToBinary (*xml, corrupt);
        {
            BSIIControlAudioProcessor fresh;
            fresh.setStateInformation (corrupt.getData(), static_cast<int> (corrupt.getSize())); pump (400);
            ok &= fresh.getProjectRecallInfo().mode == Mode::off && ! fresh.getProjectRecallInfo().hasCapture;
        }
        ok &= capture.captureProjectSound();
        expected.add ("f0 00 20 29 00 33 00 40 f7");
        pump (3100); // Timeout keeps the previous captured patch.
        ok &= ! capture.getProjectRecallInfo().capturing && capture.getProjectRecallInfo().hasCapture;
    }
    {
        const auto sourceName = "BSII direct controls " + juce::Uuid().toString();
        const auto command = log.withFileExtension ("command");
        const auto done = command.withFileExtension ("done");
        juce::ChildProcess source;
        ok &= source.start (juce::StringArray {
            juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName(),
            "--controls-source", sourceName, command.getFullPathName() });
        BSIIControlAudioProcessor direct;
        bool connected = false;
        for (int n = 0; n < 100 && ! connected; ++n)
        {
            connected = direct.connectHardwareSysExInput (sourceName);
            if (! connected) pump (20);
        }
        ok &= connected;
        auto phase = [&] (const juce::String& action)
        {
            command.replaceWithText (action);
            for (int n = 0; n < 100 && done.loadFileAsString() != action; ++n) pump (20);
            pump (100);
            ok &= done.loadFileAsString() == action;
        };
        auto index = [&] (const char* id)
        {
            const auto& specs = bsii::parameterSpecs();
            for (size_t n = 0; n < specs.size(); ++n)
                if (juce::String (specs[n].id) == id) return static_cast<int> (n);
            return -1;
        };
        phase ("live");
        ok &= direct.consumeHardwareProgram() == 23;
        ok &= direct.consumeHardwareValue (index ("filterResonance")) == 55;
        ok &= direct.consumeHardwareValue (index ("filterCutoff")) == 121;
        ok &= direct.consumeChoiceWireValue (0) == 1;
        const auto serial = direct.getLatestReceivedMidi().serial;
        ok &= serial == 6; // Notes/wheel/clock are not controller input.
        juce::AudioBuffer<float> audio (2, 64);
        juce::MidiBuffer duplicate;
        duplicate.addEvent (juce::MidiMessage::programChange (1, 99), 0);
        duplicate.addEvent (juce::MidiMessage::controllerEvent (1, 82, 99), 1);
        duplicate.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 2);
        direct.processBlock (audio, duplicate);
        ok &= duplicate.getNumEvents() == 3 && direct.getLatestReceivedMidi().serial == serial;
        ok &= direct.consumeHardwareValue (index ("filterResonance")) == bsii::noHardwareValue;
        ok &= direct.consumeHardwareProgram() == -1;
        direct.setLiveMidiRxEnabled (false);
        phase ("paused");
        ok &= direct.getLatestReceivedMidi().serial == serial && direct.getLatestSysEx().serial == 1;
        ok &= direct.consumeHardwareProgram() == -1;
        direct.setHardwareMidiChannel (2);
        direct.setLiveMidiRxEnabled (true);
        phase ("channel");
        ok &= direct.consumeHardwareProgram() == 24;
        ok &= direct.consumeHardwareValue (index ("filterResonance")) == 77;
        direct.connectHardwareSysExInput ({});
        duplicate.clear();
        duplicate.addEvent (juce::MidiMessage::programChange (2, 25), 0);
        duplicate.addEvent (juce::MidiMessage::controllerEvent (2, 82, 88), 1);
        direct.processBlock (audio, duplicate);
        ok &= direct.consumeHardwareProgram() == 25;
        ok &= direct.consumeHardwareValue (index ("filterResonance")) == 88;
        command.replaceWithText ("stop");
        source.waitForProcessToFinish (3000);
        command.deleteFile(); done.deleteFile();
    }
    log.withFileExtension ("stop").replaceWithText ("stop");
    sink.waitForProcessToFinish (5000);
    const auto actual = juce::StringArray::fromLines (log.loadFileAsString());
    ok &= actual == expected;
    if (actual != expected)
        std::cerr << "Expected:\n" << expected.joinIntoString ("\n") << "\nActual:\n" << actual.joinIntoString ("\n") << "\n";
    log.deleteFile(); log.withFileExtension ("stop").deleteFile();
    std::cout << (ok ? "PASS" : "FAIL") << " processor automation before/after editor, native CC/paired CC/NRPN, linked triggers, silent receive/recall, channel persistence, saved connection, patch-refresh notifications, capture/recall plus direct live controls, duplicate filtering, pause/channel and host fallback\n";
    return ok ? 0 : 1;
}
