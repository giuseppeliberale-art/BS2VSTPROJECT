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
        nrpn7,
        nrpn8
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

            { "osc1Fine",          "Fine",                  "Osc 1",       Protocol::pairedCC8, 26, 58, 27, 228, 128 },
            { "osc1Coarse",        "Coarse",                "Osc 1",       Protocol::pairedCC8, 27, 59, 0, 255, 128 },
            { "osc1ModEnvPitch",   "Mod Env -> Pitch",      "Osc 1",       Protocol::cc7,       71, -1, 0, 127,  64 },
            { "osc1Lfo1Pitch",     "LFO1 -> Pitch",         "Osc 1",       Protocol::pairedCC8, 28, 60, 0, 255, 128 },
            { "osc1ModEnvPW",      "Mod Env -> PW",         "Osc 1",       Protocol::cc7,       72, -1, 0, 127,  64 },
            { "osc1Lfo2PW",        "LFO2 -> PW",            "Osc 1",       Protocol::cc7,       73, -1, 0, 127,  64 },
            { "osc1ManualPW",      "Manual PW",             "Osc 1",       Protocol::cc7,       74, -1, 0, 127,  64 },

            { "osc2Fine",          "Fine",                  "Osc 2",       Protocol::pairedCC8, 29, 61, 27, 228, 128 },
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

            { "subOctaveRaw",      "Sub Octave",            "Osc 1",       Protocol::cc7,       81, -1, 0, 127,   0 },

            { "filterCutoff",      "Cutoff",                "Filter",      Protocol::pairedCC8, 16, 48, 0, 255, 255 },
            { "filterResonance",   "Resonance",             "Filter",      Protocol::cc7,       82, -1, 0, 127,   0 },
            { "filterModEnv",      "Mod Env",               "Filter",      Protocol::cc7,       85, -1, 0, 127,  64 },
            { "filterLfo2",        "LFO2",                  "Filter",      Protocol::pairedCC8, 17, 49, 0, 255, 128 },
            { "filterOverdrive",   "Overdrive",             "Filter",      Protocol::cc7,      114, -1, 0, 127,   0 },

            { "portamento",        "Portamento / Glide Time", "LFO / Glide", Protocol::cc7,        5, -1, 0, 127,   0 },
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
            { "oscSyncRaw",         "Osc Sync",     "FX / Perf",   Protocol::cc7,      110, -1, 0, 127,   0 },
            { "oscFilterMod",      "Osc Filter Mod",        "FX / Perf",   Protocol::cc7,      115, -1, 0, 127,   0 },
            { "arpSwing",          "Arp Swing",             "Arp / Seq",   Protocol::cc7,      116, -1, 3,  97,  64 },
            { "bendAmount",        "Bend Amount",           "FX / Perf",   Protocol::cc7,      107, -1, -24, 24,  12 },
            { "velocityAmp",       "Velocity -> Amp",       "FX / Perf",   Protocol::cc7,      112, -1, 1, 127,  64 },
            { "velocityMod",       "Velocity -> Mod",       "FX / Perf",   Protocol::cc7,      113, -1, 1, 127,  64 },
            { "vcaLimit",          "VCA Limit",             "FX / Perf",   Protocol::cc7,       95, -1, 0, 127,   0 },
            { "modWhLfo1Pitch",    "ModWh -> LFO1 Pitch",   "FX / Perf",   Protocol::nrpn7,      0, 70, 0, 127,  64 },
            { "modWhOsc2Pitch",    "ModWh -> Osc2 Pitch",   "FX / Perf",   Protocol::nrpn7,      0, 78, 0, 127,  64 },
            { "aftertouchFilter",  "Aftertouch -> Filter",  "FX / Perf",   Protocol::nrpn7,      0, 74, 0, 127,  64 },
            { "aftertouchLfo1",    "Aftertouch -> LFO1",    "FX / Perf",   Protocol::nrpn7,      0, 75, 0, 127,  64 },
            { "aftertouchLfo2Spd", "Aftertouch -> LFO2 Spd", "FX / Perf",  Protocol::nrpn7,      0, 76, 0, 127,   0 },
            { "modWhFilter",      "ModWh -> Filter",       "FX / Perf",  Protocol::nrpn7,      0, 94, 0, 127,  64 },
            { "modWhLfo2Filter",  "ModWh -> LFO2 Filter",  "FX / Perf",  Protocol::nrpn7,      0, 71, 0, 127,  64 },
            { "subFine",           "Sub Fine",              "Osc 1",       Protocol::nrpn8,      0, 77, 27, 228, 127 },
            { "subCoarse",         "Sub Coarse",            "Osc 1",       Protocol::nrpn8,      0, 84, 0, 255, 128 },
            { "inputGain",         "Input Gain",            "Global",      Protocol::nrpn7,     64, 28, 0, 31, 0 }
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
            // Append choices to preserve every existing host parameter index and ID.
            { "arpOn", "Arp On", "Arp / Seq", Protocol::cc7, 108, -1,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "arpLatch", "Latch", "Arp / Seq", Protocol::cc7, 109, -1,
              { {0, "Off"}, {1, "On"} }, 0, true },
            { "arpOctaves", "Octaves", "Arp / Seq", Protocol::cc7, 111, -1,
              { {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"} }, 0, true },
            { "arpMode", "Direction / Seq Mode", "Arp / Seq", Protocol::cc7, 118, -1,
              { {0, "Up"}, {1, "Down"}, {2, "UpDn1"}, {3, "UpDn2"},
                {4, "Played"}, {5, "Random"}, {6, "Play"}, {7, "Record"} }, 0, true },
            { "arpRhythm", "Rhythm", "Arp / Seq", Protocol::cc7, 119, -1,
              { {0, "1"}, {1, "2"}, {2, "3"}, {3, "4"}, {4, "5"}, {5, "6"}, {6, "7"}, {7, "8"}, {8, "9"}, {9, "10"}, {10, "11"}, {11, "12"}, {12, "13"}, {13, "14"}, {14, "15"}, {15, "16"}, {16, "17"}, {17, "18"}, {18, "19"}, {19, "20"}, {20, "21"}, {21, "22"}, {22, "23"}, {23, "24"}, {24, "25"}, {25, "26"}, {26, "27"}, {27, "28"}, {28, "29"}, {29, "30"}, {30, "31"}, {31, "32"} }, 0, true },
            { "seqRetrig", "Seq Retrig", "Arp / Seq", Protocol::nrpn7, 0, 106,
              { {0, "Off"}, {1, "On"} }, 1, true },
            { "lfo1Sync", "LFO1 Speed/Sync", "LFO / Glide", Protocol::nrpn7, 0, 88,
              { {0, "Speed"}, {1, "Sync"} }, 0, true },
            { "lfo2Sync", "LFO2 Speed/Sync", "LFO / Glide", Protocol::nrpn7, 0, 92,
              { {0, "Speed"}, {1, "Sync"} }, 0, true },
            { "lfo1Division", "LFO1 Division", "LFO / Glide", Protocol::nrpn7, 0, 87,
              { {0, "64 beats"}, {1, "48 beats"}, {2, "42 beats"}, {3, "36 beats"},
                {4, "32 beats"}, {5, "30 beats"}, {6, "28 beats"}, {7, "24 beats"},
                {8, "21 2/3 beats"}, {9, "20 beats"}, {10, "18 2/3 beats"}, {11, "18 beats"},
                {12, "16 beats"}, {13, "13 1/3 beats"}, {14, "12 beats"}, {15, "10 2/3 beats"},
                {16, "8 beats"}, {17, "6 beats"}, {18, "5 1/3 beats"}, {19, "4 beats"},
                {20, "3 beats"}, {21, "2 2/3 beats"}, {22, "1/2"}, {23, "1/4 dotted"},
                {24, "1 1/3 beats"}, {25, "1/4"}, {26, "1/8 dotted"}, {27, "1/4 triplet"},
                {28, "1/8"}, {29, "1/16 dotted"}, {30, "1/8 triplet"}, {31, "1/16"},
                {32, "1/16 triplet"}, {33, "1/32"}, {34, "1/32 triplet"} }, 0, true },
            { "lfo2Division", "LFO2 Division", "LFO / Glide", Protocol::nrpn7, 0, 91,
              { {0, "64 beats"}, {1, "48 beats"}, {2, "42 beats"}, {3, "36 beats"},
                {4, "32 beats"}, {5, "30 beats"}, {6, "28 beats"}, {7, "24 beats"},
                {8, "21 2/3 beats"}, {9, "20 beats"}, {10, "18 2/3 beats"}, {11, "18 beats"},
                {12, "16 beats"}, {13, "13 1/3 beats"}, {14, "12 beats"}, {15, "10 2/3 beats"},
                {16, "8 beats"}, {17, "6 beats"}, {18, "5 1/3 beats"}, {19, "4 beats"},
                {20, "3 beats"}, {21, "2 2/3 beats"}, {22, "1/2"}, {23, "1/4 dotted"},
                {24, "1 1/3 beats"}, {25, "1/4"}, {26, "1/8 dotted"}, {27, "1/4 triplet"},
                {28, "1/8"}, {29, "1/16 dotted"}, {30, "1/8 triplet"}, {31, "1/16"},
                {32, "1/16 triplet"}, {33, "1/32"}, {34, "1/32 triplet"} }, 0, true },
            // Hardware emits NRPN 0:0; sendChoice uses the documented RPN 0:3.
            { "microTuneTable", "MicroTune Table", "FX / Perf", Protocol::nrpn7, 0, 0,
              { {0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"},
                {5, "5"}, {6, "6"}, {7, "7"}, {8, "8"} }, 0, true },
            { "oscError", "Oscillator Error", "FX / Perf", Protocol::nrpn7, 0, 111,
              { {0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"} }, 0, true },
            { "glideDivergence", "Glide Divergence", "LFO / Glide", Protocol::nrpn7, 0, 113,
              { {0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"}, {8, "8"}, {9, "9"}, {10, "10"}, {11, "11"}, {12, "12"}, {13, "13"}, {14, "14"}, {15, "15"} }, 0, true },
            { "paraphonic", "Paraphonic Mode", "FX / Perf", Protocol::nrpn7, 0, 107,
              { {0, "Off"}, {1, "On"} }, 0, true },
        };
        return specs;
    }

    // Global controls must not be sent or replaced by patch-only operations.
    inline bool isPatchParameter (const juce::String& id) { return id != "inputGain"; }

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
            case Protocol::nrpn8:
                sendNRPN7 (out, channel, spec.numberMSB, spec.numberLSB, clamped >> 1);
                sendCC (out, channel, 38, (clamped & 1) << 6);
                break;
        }
    }

    inline void sendChoice (juce::MidiOutput& out, int channel,
                            const ChoiceSpec& spec, int wireValue)
    {
        // Selection reception is asymmetric: do not echo the hardware's NRPN.
        if (juce::String (spec.id) == "microTuneTable")
        {
            if (wireValue < 0 || wireValue > 8) return;
            sendCC (out, channel, 100, 3);
            sendCC (out, channel, 101, 0);
            sendCC (out, channel, 6, wireValue);
            sendCC (out, channel, 100, 127);
            sendCC (out, channel, 101, 127);
        }
        else if (spec.protocol == Protocol::nrpn7)
            sendNRPN7 (out, channel, spec.numberMSB, spec.numberLSB, wireValue);
        else
            sendCC (out, channel, spec.numberMSB, wireValue);
    }
}
