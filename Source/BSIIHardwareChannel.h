#pragma once
#include <JuceHeader.h>

namespace bsii
{
// Captured BSII global-channel notification, not an ordinary patch dump.
inline int hardwareChannelNotification (const juce::MidiMessage& message)
{
    const auto* b = message.getRawData();
    if (! message.isSysEx() || message.getRawDataSize() != 10) return -1;
    if (b[0] != 0xf0 || b[1] != 0 || b[2] != 0x20 || b[3] != 0x29
        || b[4] != 0 || b[5] != 0x33 || b[6] != 0 || b[7] != 0x58
        || b[9] != 0xf7 || b[8] < 1 || b[8] > 16) return -1;
    return b[8];
}
}
