#pragma once

namespace bsii
{
    // Captured CC107 and patch byte 16 use a signed offset around 64.
    // The current editor exposes the positive 1–12 semitone subset.
    constexpr int bendAmountToWire (int semitones) { return semitones + 64; }
    constexpr int bendAmountFromWire (int wire) { return wire - 64; }
}
