#include "BSIIMidiRouting.h"
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
    return standaloneOK && pluginOK ? 0 : 1;
}
