#pragma once

#include <JuceHeader.h>
#include "BSIIPairedCC.h"
#include "BSIIPerformanceEncoding.h"
#include <cmath>
#include <vector>

namespace bsii
{
    enum class Protocol
    {
        cc7,
        pairedCC8,
        nrpn7
    };

    struct ParameterSpec
    {
        const char* id;
        const char* name;
        const char* section;
        Protocol protocol;
        int numberMSB;    // CC number for cc7/pairedCC8, NRPN MSB for nrpn7
        int numberLSB;    // -1 for cc7, paired CC LSB, NRPN LSB for nrpn7
        int minimum;
        int maximum;
        int defaultValue;
    };

    struct ChoiceValue
    {
        int wireValue;
        const char* label;
    };

    struct ChoiceSpec
    {
        const char* id;
        const char* name;
        const char* section;
        Protocol protocol;
        int numberMSB;
        int numberLSB;
        std::vector<ChoiceValue> choices;
        int defaultChoiceIndex;
        bool verifiedWireValues;
    };

    inline const std::vector<ParameterSpec>& parameterSpecs()
    {
        static const std::vector<ParameterSpec> specs
        {
            { "patchVolume",       "Patch Volume",          "Master",      Protocol::cc7,        7,  -1, 0, 127, 100 },

            { "osc1Fine",          "Fine",                  "Osc 1",       Protocol::pairedCC8, 26, 58, 0, 255, 128 },
            { "osc1Coarse",        "Coarse",                "Osc 1",       Protocol::pairedCC8, 27, 59, 0, 255, 128 },
            { "osc1ModEnvPitch",   "Mod Env -> Pitch",      "Osc 1",       Protocol::cc7,       71, -1, 0, 127,  64 },
            { "osc1Lfo1Pitch",     "LFO1 -> Pitch",         "Osc 1",       Protocol::pairedCC8, 28, 60, 0, 255, 128 },
            { "osc1ModEnvPW",      "Mod Env -> PW",         "Osc 1",       Protocol::cc7,       72, -1, 0, 127,  64 },
            { "osc1Lfo2PW",        "LFO2 -> PW",            "Osc 1",       Protocol::cc7,       73, -1, 0, 127,  64 },
            { "osc1ManualPW",      "Manual PW",             "Osc 1",       Protocol::cc7,       74, -1, 0, 127,  64 },

            { "osc2Fine",          "Fine",                  "Osc 2",       Protocol::pairedCC8, 29, 61, 0, 255, 128 },
            { "osc2Coarse",        "Coarse",                "Osc 2",       Protocol::pairedCC8, 30, 62, 0, 255, 128 },
            { "osc2ModEnvPitch",   "Mod Env -> Pitch",      "Osc 2",       Protocol::cc7,       76, -1, 0, 127,  64 },
            { "osc2Lfo1Pitch",     "LFO1 -> Pitch",         "Osc 2",       Protocol::pairedCC8, 31, 63, 0, 255, 128 },
            { "osc2ModEnvPW",      "Mod Env -> PW",         "Osc 2",       Protocol::cc7,       77, -1, 0, 127,  64 },
            { "osc2Lfo2PW",        "LFO2 -> PW",            "Osc 2",       Protocol::cc7,       78, -1, 0, 127,  64 },
            { "osc2ManualPW",      "Manual PW",             "Osc 2",       Protocol::cc7,       79, -1, 0, 127,  64 },

            { "mixOsc1",           "Osc 1",                 "Mixer",       Protocol::pairedCC8, 20, 52, 0, 255, 255 },
            { "mixOsc2",           "Osc 2",                 "Mixer",       Protocol::pairedCC8, 21, 53, 0, 255,   0 },
            { "mixSub",            "Sub",                   "Mixer",       Protocol::pairedCC8, 22, 54, 0, 255,   0 },
            { "mixNoise",          "Noise",                 "Mixer",       Protocol::pairedCC8, 23, 55, 0, 255,   0 },
            { "mixRing",           "Ring Mod",              "Mixer",       Protocol::pairedCC8, 24, 56, 0, 255,   0 },
            { "mixExternal",       "External",              "Mixer",       Protocol::pairedCC8, 25, 57, 0, 255,   0 },

            { "subOctaveRaw",      "Sub Octave (raw CC)",   "Osc 1",       Protocol::cc7,       81, -1, 0, 127,   0 },

            { "filterCutoff",      "Cutoff",                "Filter",      Protocol::pairedCC8, 16, 48, 0, 255, 255 },
            { "filterResonance",   "Resonance",             "Filter",      Protocol::cc7,       82, -1, 0, 127,   0 },
            { "filterModEnv",      "Mod Env",               "Filter",      Protocol::cc7,       85, -1, 0, 127,  64 },
            { "filterLfo2",        "LFO2",                  "Filter",      Protocol::pairedCC8, 17, 49, 0, 255, 128 },
            { "filterOverdrive",   "Overdrive",             "Filter",      Protocol::cc7,      114, -1, 0, 127,   0 },

            { "portamento",        "Portamento",            "LFO / Glide", Protocol::cc7,        5, -1, 0, 127,   0 },
            { "lfo1Speed",         "LFO1 Speed",            "LFO / Glide", Protocol::pairedCC8, 18, 50, 0, 255,  75 },
            { "lfo1Delay",         "LFO1 Delay",            "LFO / Glide", Protocol::cc7,       86, -1, 0, 127,   0 },
            { "lfo2Speed",         "LFO2 Speed",            "LFO / Glide", Protocol::pairedCC8, 19, 51, 0, 255,  52 },
            { "lfo2Delay",         "LFO2 Delay",            "LFO / Glide", Protocol::cc7,       87, -1, 0, 127,   0 },
            { "lfo1Slew",          "LFO1 Slew",             "LFO / Glide", Protocol::nrpn7,      0, 86, 0, 127,   0 },
            { "lfo2Slew",          "LFO2 Slew",             "LFO / Glide", Protocol::nrpn7,      0, 90, 0, 127,   0 },
            { "filterTracking",    "Filter Tracking",       "Filter",      Protocol::nrpn7,      0,108, 0,   7,   0 },

            { "ampAttack",         "Attack",                "Amp Env",     Protocol::cc7,       90, -1, 0, 127,   0 },
            { "ampDecay",          "Decay",                 "Amp Env",     Protocol::cc7,       91, -1, 0, 127,   0 },
            { "ampSustain",        "Sustain",               "Amp Env",     Protocol::cc7,       92, -1, 0, 127, 127 },
            { "ampRelease",        "Release",               "Amp Env",     Protocol::cc7,       93, -1, 0, 127,   0 },
            { "ampRetriggerCount", "Retrigger Count",       "Amp Env",     Protocol::nrpn7,      0,117, 0,  16,   0 },
            { "modAttack",         "Attack",                "Mod Env",     Protocol::cc7,      102, -1, 0, 127,   0 },
            { "modDecay",          "Decay",                 "Mod Env",     Protocol::cc7,      103, -1, 0, 127,   0 },
            { "modSustain",        "Sustain",               "Mod Env",     Protocol::cc7,      104, -1, 0, 127, 127 },
            { "modRelease",        "Release",               "Mod Env",     Protocol::cc7,      105, -1, 0, 127,   0 },
            { "modRetriggerCount", "Retrigger Count",       "Mod Env",     Protocol::nrpn7,      0,118, 0,  16,   0 },

            { "distortion",        "Distortion",            "FX / Perf",   Protocol::cc7,       94, -1, 0, 127,   0 },
            { "oscSyncRaw",         "Osc Sync (raw CC)",     "FX / Perf",   Protocol::cc7,      110, -1, 0, 127,   0 },
            { "oscFilterMod",      "Osc Filter Mod",        "FX / Perf",   Protocol::cc7,      115, -1, 0, 127,   0 },
            { "arpSwing",          "Arp Swing",             "FX / Perf",   Protocol::cc7,      116, -1, 3,  97,  64 },
            { "bendAmount",        "Bend Amount",           "FX / Perf",   Protocol::cc7,      107, -1, 1,  12,  12 },
            { "velocityAmp",       "Velocity -> Amp",       "FX / Perf",   Protocol::cc7,      112, -1, 1, 127,  64 },
            { "velocityMod",       "Velocity -> Mod",       "FX / Perf",   Protocol::cc7,      113, -1, 1, 127,  64 },
            { "vcaLimit",          "VCA Limit",             "FX / Perf",   Protocol::cc7,       95, -1, 0, 127,   0 },
            { "modWhLfo1Pitch",    "ModWh -> LFO1 Pitch",   "FX / Perf",   Protocol::nrpn7,      0, 70, 0, 127,  64 },
            { "modWhOsc2Pitch",    "ModWh -> Osc2 Pitch",   "FX / Perf",   Protocol::nrpn7,      0, 78, 0, 127,  64 },
            { "aftertouchFilter",  "Aftertouch -> Filter",  "FX / Perf",   Protocol::nrpn7,      0, 74, 0, 127,  64 },
            { "aftertouchLfo1",    "Aftertouch -> LFO1",    "FX / Perf",   Protocol::nrpn7,      0, 75, 0, 127,  64 },
            { "aftertouchLfo2Spd", "Aftertouch -> LFO2 Spd", "FX / Perf",  Protocol::nrpn7,      0, 76, 0, 127,   0 }
        };
        return specs;
    }

    inline const std::vector<ChoiceSpec>& choiceSpecs()
    {
        static const std::vector<ChoiceSpec> specs
        {
            { "osc1Wave", "Waveform", "Osc 1", Protocol::nrpn7, 0, 72,
              { {0, "Sine"}, {1, "Triangle"}, {2, "Saw"}, {3, "Pulse"} }, 2, true },
            { "subWave", "Sub Wave", "Osc 1", Protocol::cc7, 80, -1,
              { {0, "Sine"}, {1, "Pulse"}, {2, "Square"} }, 0, true },
            { "osc1Range", "Range", "Osc 1", Protocol::cc7, 70, -1,
              { {63, "16'"}, {64, "8'"}, {65, "4'"}, {66, "2'"} }, 1, true },
            { "osc2Wave", "Waveform", "Osc 2", Protocol::nrpn7, 0, 82,
              { {0, "Sine"}, {1, "Triangle"}, {2, "Saw"}, {3, "Pulse"} }, 2, true },
            { "osc2Range", "Range", "Osc 2", Protocol::cc7, 75, -1,
              { {63, "16'"}, {64, "8'"}, {65, "4'"}, {66, "2'"} }, 1, true },
            { "filterType", "Type", "Filter", Protocol::cc7, 83, -1,
              { {0, "Classic"}, {1, "Acid"} }, 0, true },
            { "filterShape", "Shape", "Filter", Protocol::cc7, 84, -1,
              { {0, "LP"}, {1, "BP"}, {2, "HP"} }, 0, true },
            { "filterSlope", "Slope", "Filter", Protocol::cc7, 106, -1,
              { {0, "12 dB"}, {1, "24 dB"} }, 1, true },
            { "lfo1Wave", "LFO1 Wave", "LFO / Glide", Protocol::cc7, 88, -1,
              { {0, "Triangle"}, {1, "Ramp"}, {2, "Square"}, {3, "S&H"} }, 0, true },
            { "lfo2Wave", "LFO2 Wave", "LFO / Glide", Protocol::cc7, 89, -1,
              { {0, "Triangle"}, {1, "Ramp"}, {2, "Square"}, {3, "S&H"} }, 0, true },
            { "ampTrigger", "Trigger", "Amp Env", Protocol::nrpn7, 0, 73,
              { {1, "Single"}, {0, "Multi"}, {2, "Autoglide"} }, 1, true },
            { "modTrigger", "Trigger", "Mod Env", Protocol::nrpn7, 0, 105,
              { {1, "Single"}, {0, "Multi"}, {2, "Autoglide"} }, 1, true },
            { "ampRetrigger", "Retrigger", "Amp Env", Protocol::nrpn7, 0, 109,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "modRetrigger", "Retrigger", "Mod Env", Protocol::nrpn7, 0, 110,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "ampFixed", "Fixed Sustain", "Amp Env", Protocol::nrpn7, 0, 114,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "modFixed", "Fixed Sustain", "Mod Env", Protocol::nrpn7, 0, 115,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "lfo1KeySync", "LFO1 Key Sync", "LFO / Glide", Protocol::nrpn7, 0, 89,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "lfo2KeySync", "LFO2 Key Sync", "LFO / Glide", Protocol::nrpn7, 0, 93,
              { {0, "Off"}, {1, "On"} }, 1, true },
        };
        return specs;
    }

    inline int clampMidiChannel (int channel) { return juce::jlimit (1, 16, channel); }
    inline int clamp7 (int value) { return juce::jlimit (0, 127, value); }
    inline int clamp8 (int value) { return juce::jlimit (0, 255, value); }

    inline void sendCC (juce::MidiOutput& out, int channel, int controller, int value)
    {
        out.sendMessageNow (juce::MidiMessage::controllerEvent (
            clampMidiChannel (channel), clamp7 (controller), clamp7 (value)));
    }

    inline void sendPairedCC8 (juce::MidiOutput& out, int channel,
                               int msbController, int lsbController, int value0To255)
    {
        const auto value8 = clamp8 (value0To255);
        const auto value14 = rawToPairedCC (value8);

        sendCC (out, channel, msbController, (value14 >> 7) & 0x7f);
        sendCC (out, channel, lsbController, value14 & 0x7f);
    }

    inline void sendNRPN7 (juce::MidiOutput& out, int channel,
                           int nrpnMSB, int nrpnLSB, int value)
    {
        sendCC (out, channel, 99, nrpnMSB);
        sendCC (out, channel, 98, nrpnLSB);
        sendCC (out, channel, 6, value);
    }

    inline void sendParameter (juce::MidiOutput& out, int channel,
                               const ParameterSpec& spec, int value)
    {
        const auto clamped = juce::jlimit (spec.minimum, spec.maximum, value);
        switch (spec.protocol)
        {
            case Protocol::cc7:       sendCC (out, channel, spec.numberMSB,
                                            juce::String (spec.id) == "bendAmount" ? bendAmountToWire (clamped) : clamped); break;
            case Protocol::pairedCC8: sendPairedCC8 (out, channel, spec.numberMSB, spec.numberLSB, clamped); break;
            case Protocol::nrpn7:     sendNRPN7 (out, channel, spec.numberMSB, spec.numberLSB, clamped); break;
        }
    }

    inline void sendChoice (juce::MidiOutput& out, int channel,
                            const ChoiceSpec& spec, int wireValue)
    {
        if (spec.protocol == Protocol::nrpn7)
            sendNRPN7 (out, channel, spec.numberMSB, spec.numberLSB, wireValue);
        else
            sendCC (out, channel, spec.numberMSB, wireValue);
    }
}
