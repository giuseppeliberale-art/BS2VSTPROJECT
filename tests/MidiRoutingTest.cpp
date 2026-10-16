#include "BSIIMidiRouting.h"
#include "BSIIPresetSync.h"
#include <iostream>
int main()
{
    juce::MidiBuffer incoming;
    const juce::uint8 reply[] {0x00, 0x20, 0x29, 0x00, 0x33, 0x00};
    incoming.addEvent(juce::MidiMessage::createSysExMessage(reply, sizeof(reply)), 0);
    incoming.addEvent(juce::MidiMessage::controllerEvent(1, 30, 127), 1);
    incoming.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 2);
    auto plugin = incoming;
    bsii::finishIncomingMidi(incoming, true);
    bsii::finishIncomingMidi(plugin, false);
    const bool standaloneOK = incoming.isEmpty();
    const bool pluginOK = plugin.getNumEvents() == 3;
    std::cout << (standaloneOK ? "PASS" : "FAIL") << " standalone consumes incoming replies and controls instead of echoing them\n";
    std::cout << (pluginOK ? "PASS" : "FAIL") << " plugin MIDI pass-through preserved\n";
    bsii::PresetSync sync;
    bool presetOK = sync.selected() == -1 && ! sync.ready (1000);
    presetOK &= ! sync.select (-1, 0) && ! sync.select (128, 0);
    sync.select (1, 100);
    presetOK &= ! sync.ready (299) && sync.ready (300);
    sync.requestStarted();
    presetOK &= sync.responseCurrent() && ! sync.ready (1000);
    sync.select (2, 310);
    presetOK &= ! sync.responseCurrent(); // Old reply must not overwrite preset 2.
    sync.select (3, 400);
    presetOK &= ! sync.ready (599) && sync.ready (600) && sync.selected() == 3;
    sync.requestStarted();
    presetOK &= sync.responseCurrent();
    sync.forget();
    presetOK &= sync.selected() == -1 && ! sync.responseCurrent() && ! sync.ready (1000);
    sync.select (127, 0xffffff80u);
    presetOK &= ! sync.ready (0x47u) && sync.ready (0x48u); // Counter wrap.
    sync.select (0, 1000);
    presetOK &= sync.selected() == 0;
    std::cout << (presetOK ? "PASS" : "FAIL") << " preset settling, rapid changes, stale replies, boundaries and clock wrap\n";
    return standaloneOK && pluginOK && presetOK ? 0 : 1;
}
