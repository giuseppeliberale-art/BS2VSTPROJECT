#pragma once

namespace bsii
{
    // Captured CC107 and patch byte 16 use a signed offset around 64.
    // Supported hardware range: -24 through +24 semitones.
    // Outside every parameter range; -1 is a valid signed Bend Amount.
    constexpr int noHardwareValue = -1000;
    constexpr int bendAmountToWire (int semitones) { return semitones + 64; }
    constexpr int bendAmountFromWire (int wire) { return wire - 64; }
}
