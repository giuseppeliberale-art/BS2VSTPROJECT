#pragma once
#include <algorithm>
namespace bsii
{
    // BSII eight-bit values occupy the high eight bits of the paired CC word.
    inline int pairedCCToRaw (int msb, int lsb)
    {
        return ((msb & 127) << 1) | ((lsb & 127) >> 6);
    }
    inline int rawToPairedCC (int raw)
    {
        return std::clamp (raw, 0, 255) << 6;
    }
}
