#pragma once

#include <JuceHeader.h>
#include "BSIIPerformanceEncoding.h"

namespace bsii
{
    struct DecodedPatchFields
    {
        bool valid = false;
        juce::String error;
        int decodedCount = 0;
        int verifiedCount = 0;
        int candidateCount = 0;
        juce::String patchName;

        int portamento=-1, bendAmount=-1;
        int osc1ModEnvPW=-1;
        int osc1Fine=-1, osc1Coarse=-1, osc1ManualPW=-1, osc1Range=-1, osc1ModEnvPitch=-1, osc1Lfo1Pitch=-1, osc1Lfo2PW=-1;
        int osc2Fine=-1, osc2Coarse=-1, osc2ManualPW=-1, osc2Range=-1, osc2ModEnvPitch=-1, osc2Lfo1Pitch=-1, osc2ModEnvPW=-1, osc2Lfo2PW=-1;
        int mixOsc1=-1, mixOsc2=-1, mixSub=-1, mixNoise=-1, mixRing=-1, mixExternal=-1;
        int filterCutoff=-1, filterResonance=-1, filterOverdrive=-1, filterType=-1, filterShape=-1, filterSlope=-1, filterModEnv=-1, filterLfo2=-1, filterTracking=-1;
        int ampFixed=-1, ampRetriggerCount=-1;
        int ampTrigger=-1; // Editor choice index: Single=0, Multi=1, Autoglide=2.
        int velocityAmp=-1, ampAttack=-1, ampDecay=-1, ampSustain=-1, ampRelease=-1, ampRetrigger=-1;
        int modTrigger=-1, modFixed=-1, modRetriggerCount=-1;
        int velocityMod=-1, modAttack=-1, modDecay=-1, modSustain=-1, modRelease=-1, modRetrigger=-1;
        int lfo1Wave=-1, lfo1Delay=-1, lfo1Slew=-1, lfo1Speed=-1, lfo1KeySync=-1;
        int lfo2Wave=-1, lfo2Delay=-1, lfo2Slew=-1, lfo2Speed=-1, lfo2KeySync=-1;
        int arpSwing=-1, modWhLfo1Pitch=-1, modWhOsc2Pitch=-1, aftertouchFilter=-1, aftertouchLfo1=-1, aftertouchLfo2Spd=-1;
        int oscSyncRaw=-1;
        int oscFilterMod=-1, distortion=-1, vcaLimit=-1;
    };

    inline bool looksLikeSingleBsiIIPatchDump (const juce::MemoryBlock& data)
    {
        if (data.getSize() != 154) return false;
        const auto* b = static_cast<const uint8_t*> (data.getData());
        return b[0]==0xF0 && b[1]==0x00 && b[2]==0x20 && b[3]==0x29
            && b[4]==0x00 && b[5]==0x33 && b[153]==0xF7;
    }

    inline int compactMaskedBits (uint8_t value, uint8_t mask)
    {
        int out = 0, outBit = 0;
        for (int bit = 0; bit < 7; ++bit)
        {
            const int bitMask = 1 << bit;
            if ((mask & bitMask) != 0)
            {
                if ((value & bitMask) != 0) out |= (1 << outBit);
                ++outBit;
            }
        }
        return out;
    }

    inline int bitCount7 (uint8_t mask)
    {
        int count = 0;
        for (int bit = 0; bit < 7; ++bit)
            if ((mask & (1 << bit)) != 0) ++count;
        return count;
    }

    inline int extractField (const uint8_t* b, int offset, uint8_t mask0, uint8_t mask1 = 0)
    {
        if (mask1 == 0) return compactMaskedBits (b[offset], mask0);
        const int lowBits = bitCount7 (mask1);
        return (compactMaskedBits (b[offset], mask0) << lowBits)
             | compactMaskedBits (b[offset + 1], mask1);
    }

    inline void acceptCandidate (int raw, int minimum, int maximum, int& destination,
                                 DecodedPatchFields& out)
    {
        if (raw >= minimum && raw <= maximum)
        {
            destination = raw;
            ++out.candidateCount;
            ++out.decodedCount;
        }
    }

    inline void acceptVerified (int raw, int minimum, int maximum, int& destination,
                                DecodedPatchFields& out)
    {
        if (raw >= minimum && raw <= maximum)
        {
            destination = raw;
            ++out.verifiedCount;
            ++out.decodedCount;
        }
    }

    // v0.8.0 strategy:
    // - Six fields independently verified against the user's current BSII firmware
    //   are always decoded with the exact formulas established from controlled dumps.
    // - Historical/publicly reverse-engineered fields are decoded individually only
    //   when their resulting value fits the editor's known legal range.
    // - A bad/obsolete field can therefore never reject the whole patch or alter an
    //   unrelated control. Unknown/out-of-range fields simply remain -1.
    inline DecodedPatchFields decodePatchFields (const juce::MemoryBlock& data)
    {
        DecodedPatchFields out;
        if (! looksLikeSingleBsiIIPatchDump (data))
        {
            out.error = "Expected one 154-byte Bass Station II patch SysEx message.";
            return out;
        }

        const auto* b = static_cast<const uint8_t*> (data.getData());

        // ---- Verified on current hardware by controlled one-parameter dumps ----
        acceptVerified (((b[44] & 0x0F) << 4) | ((b[45] >> 3) & 0x0F), 0, 255, out.filterCutoff, out);
        acceptVerified (((b[45] & 0x03) << 5) | ((b[46] >> 2) & 0x1F), 0, 127, out.filterResonance, out);
        acceptVerified (((b[46] & 0x01) << 6) | ((b[47] >> 1) & 0x3F), 0, 127, out.filterOverdrive, out);
        acceptVerified (b[48] & 0x03, 0, 2, out.filterShape, out);
        acceptVerified (((b[50] & 0x1F) << 2) | ((b[51] >> 5) & 0x03), 0, 127, out.ampAttack, out);
        // Hardware wire order is Multi, Single, Autoglide; the editor lists Single first.
        const int ampTriggerWire = extractField (b, 55, 0x06);
        // CC110 accepts any nonzero value as On; the raw editor uses 127 for On.
        acceptVerified (extractField (b, 18, 0x40) == 0 ? 0 : 127, 0, 127, out.oscSyncRaw, out);
        const int modTriggerWire = extractField (b, 62, 0x0C);
        if (modTriggerWire <= 2)
            acceptVerified (modTriggerWire == 0 ? 1 : modTriggerWire == 1 ? 0 : 2,
                            0, 2, out.modTrigger, out);
        acceptVerified (extractField (b, 120, 0x01), 0, 1, out.modFixed, out);
        acceptVerified (extractField (b, 122, 0x07, 0x60), 0, 16, out.modRetriggerCount, out);
        acceptVerified (extractField (b, 119, 0x02), 0, 1, out.ampFixed, out);
        acceptVerified (extractField (b, 121, 0x0F, 0x40), 0, 16, out.ampRetriggerCount, out);
        if (ampTriggerWire <= 2)
            acceptVerified (ampTriggerWire == 0 ? 1 : ampTriggerWire == 1 ? 0 : 2,
                            0, 2, out.ampTrigger, out);
        acceptVerified (((b[98] & 0x1F) << 2) | ((b[99] >> 5) & 0x03), 0, 127, out.osc1ModEnvPitch, out);
        acceptVerified (extractField (b, 101, 0x03, 0x7C), 0, 127, out.osc1ModEnvPW, out);

        // ---- Candidate fields from the historical packed-patch map ----
        // Each field is range-checked independently. Nothing here can invalidate
        // the verified fields above.
        acceptCandidate (extractField (b, 13, 0x03, 0x7C), 0, 127, out.portamento, out);
        acceptVerified (bendAmountFromWire (extractField (b, 16, 0x7F)), 1, 12, out.bendAmount, out);

        acceptCandidate (extractField (b, 19, 0x0F, 0x70), 0, 127, out.osc1ManualPW, out);
        // Controlled hardware dumps: 16'/8'/4'/2' encode as 63/64/65/66.
        // The editor choice parameter uses indexes 0/1/2/3.
        acceptVerified (extractField (b, 20, 0x07, 0x78) - 63, 0, 3, out.osc1Range, out);
        acceptCandidate (extractField (b, 21, 0x07, 0x7C), 0, 255, out.osc1Coarse, out);
        acceptCandidate (extractField (b, 22, 0x03, 0x7E), 0, 255, out.osc1Fine, out);

        acceptCandidate (extractField (b, 25, 0x3F, 0x40), 0, 127, out.osc2ManualPW, out);
        // Controlled Osc 2 dumps use the same 63–66 range encoding as Osc 1.
        acceptVerified (extractField (b, 26, 0x1F, 0x60) - 63, 0, 3, out.osc2Range, out);
        acceptCandidate (extractField (b, 27, 0x1F, 0x70), 0, 255, out.osc2Coarse, out);
        acceptCandidate (extractField (b, 28, 0x0F, 0x78), 0, 255, out.osc2Fine, out);

        acceptCandidate (extractField (b, 37, 0x07, 0x7C), 0, 255, out.mixOsc1, out);
        acceptCandidate (extractField (b, 38, 0x03, 0x7E), 0, 255, out.mixOsc2, out);
        acceptCandidate (extractField (b, 39, 0x01, 0x7F), 0, 255, out.mixSub, out);
        acceptCandidate (extractField (b, 41, 0x7F, 0x40), 0, 255, out.mixNoise, out);
        acceptCandidate (extractField (b, 42, 0x3F, 0x60), 0, 255, out.mixRing, out);
        acceptCandidate (extractField (b, 43, 0x1F, 0x70), 0, 255, out.mixExternal, out);

        acceptCandidate (extractField (b, 48, 0x04), 0, 1, out.filterType, out);
        acceptCandidate (extractField (b, 48, 0x08), 0, 1, out.filterSlope, out);
        acceptCandidate (extractField (b, 49, 0x3F, 0x40), 0, 127, out.velocityAmp, out);

        acceptCandidate (extractField (b, 51, 0x0F, 0x70), 0, 127, out.ampDecay, out);
        acceptCandidate (extractField (b, 52, 0x07, 0x78), 0, 127, out.ampSustain, out);
        acceptCandidate (extractField (b, 53, 0x03, 0x7C), 0, 127, out.ampRelease, out);

        acceptCandidate (extractField (b, 56, 0x7F), 0, 127, out.velocityMod, out);
        acceptCandidate (extractField (b, 57, 0x3F, 0x40), 0, 127, out.modAttack, out);
        acceptCandidate (extractField (b, 58, 0x1F, 0x60), 0, 127, out.modDecay, out);
        acceptCandidate (extractField (b, 59, 0x0F, 0x70), 0, 127, out.modSustain, out);
        acceptCandidate (extractField (b, 60, 0x07, 0x78), 0, 127, out.modRelease, out);

        acceptCandidate (extractField (b, 63, 0x06), 0, 3, out.lfo1Wave, out);
        acceptCandidate (extractField (b, 64, 0x7F), 0, 127, out.lfo1Delay, out);
        acceptCandidate (extractField (b, 65, 0x3F, 0x40), 0, 127, out.lfo1Slew, out);
        acceptCandidate (extractField (b, 66, 0x3F, 0x60), 0, 255, out.lfo1Speed, out);
        acceptCandidate (extractField (b, 69, 0x10), 0, 1, out.lfo1KeySync, out);

        acceptCandidate (extractField (b, 70, 0x01, 0x7E), 0, 127, out.lfo2Delay, out);
        acceptCandidate (extractField (b, 70, 0x0C), 0, 3, out.lfo2Wave, out);
        acceptCandidate (extractField (b, 72, 0x7F), 0, 127, out.lfo2Slew, out);
        acceptCandidate (extractField (b, 73, 0x7F, 0x40), 0, 255, out.lfo2Speed, out);
        acceptCandidate (extractField (b, 76, 0x20), 0, 1, out.lfo2KeySync, out);

        acceptCandidate (extractField (b, 81, 0x3F, 0x40), 0, 127, out.arpSwing, out);
        acceptCandidate (extractField (b, 83, 0x0F, 0x70), 0, 127, out.modWhLfo1Pitch, out);
        acceptCandidate (extractField (b, 85, 0x03, 0x7C), 0, 127, out.modWhOsc2Pitch, out);
        acceptCandidate (extractField (b, 86, 0x01, 0x7E), 0, 127, out.aftertouchFilter, out);
        acceptCandidate (extractField (b, 88, 0x7F), 0, 127, out.aftertouchLfo1, out);
        acceptCandidate (extractField (b, 89, 0x3F, 0x40), 0, 127, out.aftertouchLfo2Spd, out);

        acceptCandidate (extractField (b, 90, 0x3F, 0x60), 0, 255, out.osc1Lfo1Pitch, out);
        acceptCandidate (extractField (b, 91, 0x1F, 0x70), 0, 255, out.osc2Lfo1Pitch, out);
        acceptCandidate (extractField (b, 93, 0x03, 0x7C), 0, 127, out.osc1Lfo2PW, out);
        acceptCandidate (extractField (b, 94, 0x01, 0x7E), 0, 127, out.osc2Lfo2PW, out);
        acceptCandidate (extractField (b, 97, 0x7F, 0x40), 0, 255, out.filterLfo2, out);
        acceptCandidate (extractField (b, 99, 0x0F, 0x70), 0, 127, out.osc2ModEnvPitch, out);
        acceptCandidate (extractField (b, 102, 0x01, 0x7E), 0, 127, out.osc2ModEnvPW, out);
        acceptCandidate (extractField (b, 105, 0x3F, 0x40), 0, 127, out.filterModEnv, out);
        acceptCandidate (extractField (b, 106, 0x1F, 0x60), 0, 127, out.oscFilterMod, out);
        acceptCandidate (extractField (b, 107, 0x0F, 0x70), 0, 127, out.distortion, out);
        acceptCandidate (extractField (b, 108, 0x07, 0x78), 0, 127, out.vcaLimit, out);
        acceptCandidate (extractField (b, 112, 0x07), 0, 7, out.filterTracking, out);
        acceptCandidate (extractField (b, 114, 0x40), 0, 1, out.ampRetrigger, out);
        acceptCandidate (extractField (b, 115, 0x20), 0, 1, out.modRetrigger, out);

        juce::String name;
        for (int i = 137; i < 153; ++i)
        {
            const auto c = static_cast<char> (b[i] & 0x7F);
            if (c >= 32 && c <= 126)
                name += juce::String::charToString (static_cast<juce::juce_wchar> (c));
        }
        out.patchName = name.trimEnd();

        // Hardware rule verified on the actual unit: Acid is fixed LP / 24 dB.
        if (out.filterType == 1)
        {
            out.filterShape = 0;
            out.filterSlope = 1;
        }

        out.valid = true;
        return out;
    }
}
