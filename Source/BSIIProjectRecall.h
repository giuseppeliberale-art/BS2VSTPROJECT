#pragma once
#include <JuceHeader.h>
#include "BSIIPatchDecoder.h"

namespace bsii
{
// Only the previously tested current-edit-buffer command is eligible for recall.
// Reject stored/bank commands even if the ordinary field decoder accepts them.
inline bool isRecallPatch (const juce::MemoryBlock& patch)
{
    if (! looksLikeSingleBsiIIPatchDump (patch)) return false;
    const auto* bytes = static_cast<const uint8_t*> (patch.getData());
    for (size_t i = 1; i + 1 < patch.getSize(); ++i)
        if (bytes[i] > 127) return false;
    for (size_t i = 6; i <= 12; ++i)
        if (bytes[i] != 0) return false;
    return decodePatchFields (patch).valid;
}
}
