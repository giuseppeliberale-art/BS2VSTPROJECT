#pragma once
#include "BSIIMidi.h"

namespace bsii
{
inline juce::ValueTree patchOnlyState (const juce::ValueTree& source)
{
    auto state = source.createCopy();
    for (int i = state.getNumChildren() - 1; i >= 0; --i)
        if (! isPatchParameter (state.getChild (i).getProperty ("id").toString()))
            state.removeChild (i, nullptr);
    return state;
}

inline juce::ValueTree patchStateKeepingGlobals (const juce::ValueTree& patch,
                                                const juce::ValueTree& current)
{
    auto state = patchOnlyState (patch);
    for (const auto child : current)
        if (! isPatchParameter (child.getProperty ("id").toString()))
            state.addChild (child.createCopy(), -1, nullptr);
    return state;
}
}
