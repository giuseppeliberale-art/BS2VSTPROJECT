#pragma once
#include <cstdint>

namespace bsii
{
// One outstanding edit-buffer request: replies contain no request identifier.
class PresetSync
{
public:
    bool select (int program, uint32_t now)
    {
        if (program < 0 || program > 127) return false;
        preset = program;
        ++generation;
        changedAt = now;
        queued = true;
        return true;
    }
    void forget() { preset = -1; ++generation; queued = false; }
    bool ready (uint32_t now) const { return queued && now - changedAt >= 200u; }
    void requestStarted() { requestedGeneration = generation; queued = false; }
    bool responseCurrent() const { return requestedGeneration == generation && ! queued; }
    int selected() const { return preset; }
private:
    int preset = -1;
    uint32_t generation = 0, requestedGeneration = 0, changedAt = 0;
    bool queued = false;
};
}
