#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
namespace bsii
{
    inline void finishIncomingMidi (juce::MidiBuffer& messages, bool standalone)
    {
        // The standalone is a hardware editor, not a MIDI-through router.
        // Consume only after parsing; explicit editor sends use their own output.
        if (standalone)
            messages.clear();
    }
}
