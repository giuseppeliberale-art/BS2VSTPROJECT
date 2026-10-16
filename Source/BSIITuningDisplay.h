#pragma once
#include <algorithm>
#include <cmath>

namespace bsii
{
    // Presentation only: raw parameter storage and MIDI remain unchanged.
    inline double fineDisplay (int raw)
    {
        return std::clamp (raw < 127 ? raw - 127 : raw > 128 ? raw - 128 : 0, -100, 100);
    }

    inline int fineRaw (double cents)
    {
        const int value = static_cast<int> (std::lround (std::clamp (cents, -100.0, 100.0)));
        return value < 0 ? value + 127 : value + 128;
    }

    // Approximate semitone display; exact hardware LCD quantization is unknown.
    inline double coarseDisplay (int raw)
    {
        return raw < 127 ? (raw - 127) * 12.0 / 127.0
             : raw > 128 ? (raw - 128) * 12.0 / 127.0 : 0.0;
    }

    inline int coarseRaw (double semitones)
    {
        const double value = std::clamp (semitones, -12.0, 12.0);
        return static_cast<int> (std::lround (value * 127.0 / 12.0 + (value < 0 ? 127 : 128)));
    }
}
