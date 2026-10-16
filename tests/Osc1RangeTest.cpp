#include "BSIIPatchDecoder.h"
#include "BSIITuningDisplay.h"
#include "BSIIPairedCC.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const char* names[] = {"16", "8", "4", "2"};
    bool passed = true;
    for (int i = 0; i < 4; ++i) {
        const auto path = std::string(argv[1]) + "/osc1-" + names[i] + ".syx";
        std::ifstream input(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data(), bytes.size()));
        const bool ok = decoded.valid && decoded.osc1Range == i;
        std::cout << (ok ? "PASS " : "FAIL ") << names[i] << " feet: choice " << decoded.osc1Range << " expected " << i << "\n";
        passed &= ok;
    }
    for (int i = 0; i < 4; ++i) {
        const auto path = std::string(argv[1]) + "/osc2-" + names[i] + ".syx";
        std::ifstream input(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data(), bytes.size()));
        const bool ok = decoded.valid && decoded.osc2Range == i;
        std::cout << (ok ? "PASS Osc 2 " : "FAIL Osc 2 ") << names[i] << " feet: choice " << decoded.osc2Range << " expected " << i << "\n";
        passed &= ok;
    }
    for (int osc = 1; osc <= 2; ++osc) {
        const char* positions[] = {"min", "minus50", "centre", "plus50", "max"};
        const int expected[] = {-100, -50, 0, 50, 100};
        for (int i = 0; i < 5; ++i) {
            std::ifstream input(std::string(argv[1]) + "/osc" + std::to_string(osc) + "-fine-" + positions[i] + ".syx", std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(input)), {});
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data(), bytes.size()));
            passed &= decoded.valid && bsii::fineDisplay(osc == 1 ? decoded.osc1Fine : decoded.osc2Fine) == expected[i];
        }
    }
    passed &= bsii::fineDisplay(127) == 0 && bsii::fineDisplay(128) == 0;
    passed &= bsii::coarseDisplay(0) == -12 && bsii::coarseDisplay(255) == 12;
    passed &= bsii::coarseDisplay(127) == 0 && bsii::coarseDisplay(128) == 0;
    for (int raw = 27; raw <= 228; ++raw)
        passed &= std::abs(bsii::fineRaw(bsii::fineDisplay(raw)) - raw) <= 1;
    for (int raw = 0; raw <= 255; ++raw)
        passed &= std::abs(bsii::coarseRaw(bsii::coarseDisplay(raw)) - raw) <= 1;
    std::cout << (passed ? "PASS" : "FAIL") << " tuning displays, hardware Fine captures, and input round trips\n";
    passed &= bsii::pairedCCToRaw(0, 0) == 0;
    passed &= bsii::pairedCCToRaw(127, 64) == 255;
    for (int raw = 0; raw < 256; ++raw) {
        const int packed = bsii::rawToPairedCC(raw);
        passed &= bsii::pairedCCToRaw(packed >> 7, packed & 127) == raw;
    }
    std::cout << (passed ? "PASS" : "FAIL") << " captured paired-CC packing and all 256 round trips\n";
    const char* mixerNames[] = {"osc1", "osc2", "sub", "noise", "ring", "external"};
    for (int mixer = 0; mixer < 6; ++mixer) {
        std::ifstream mixerInput(std::string(argv[1]) + "/mix-" + mixerNames[mixer] + "-sweep.syx", std::ios::binary);
        const std::string mixerBytes((std::istreambuf_iterator<char>(mixerInput)), {});
        bool mixerOK = mixerBytes.size() == 256 * 154;
        if (mixerOK) {
            for (int raw = 0; raw < 256; ++raw) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(mixerBytes.data() + raw * 154, 154));
                mixerOK &= decoded.valid && (mixer == 0 ? decoded.mixOsc1 : mixer == 1 ? decoded.mixOsc2 : mixer == 2 ? decoded.mixSub : mixer == 3 ? decoded.mixNoise : mixer == 4 ? decoded.mixRing : decoded.mixExternal) == raw;
            }
        }
        std::cout << (mixerOK ? "PASS" : "FAIL") << " all 256 captured " << mixerNames[mixer] << " Mixer levels\n";
        passed &= mixerOK;
    }
    std::ifstream cutoffInput(std::string(argv[1]) + "/filter-cutoff-sweep.syx", std::ios::binary);
    const std::string cutoffBytes((std::istreambuf_iterator<char>(cutoffInput)), {});
    bool cutoffOK = cutoffBytes.size() == 256 * 154;
    if (cutoffOK) {
        for (int raw = 0; raw < 256; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(cutoffBytes.data() + raw * 154, 154));
            cutoffOK &= decoded.valid && decoded.filterCutoff == raw;
        }
    }
    std::cout << (cutoffOK ? "PASS" : "FAIL") << " all 256 captured Filter Cutoff values\n";
    passed &= cutoffOK;
    std::ifstream resonanceInput(std::string(argv[1]) + "/filter-resonance-sweep.syx", std::ios::binary);
    const std::string resonanceBytes((std::istreambuf_iterator<char>(resonanceInput)), {});
    bool resonanceOK = resonanceBytes.size() == 128 * 154;
    if (resonanceOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(resonanceBytes.data() + raw * 154, 154));
            resonanceOK &= decoded.valid && decoded.filterResonance == raw;
        }
    }
    std::cout << (resonanceOK ? "PASS" : "FAIL") << " all 128 captured Filter Resonance values\n";
    passed &= resonanceOK;
    std::ifstream overdriveInput(std::string(argv[1]) + "/filter-overdrive-sweep.syx", std::ios::binary);
    const std::string overdriveBytes((std::istreambuf_iterator<char>(overdriveInput)), {});
    bool overdriveOK = overdriveBytes.size() == 128 * 154;
    if (overdriveOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(overdriveBytes.data() + raw * 154, 154));
            overdriveOK &= decoded.valid && decoded.filterOverdrive == raw;
        }
    }
    std::cout << (overdriveOK ? "PASS" : "FAIL") << " all 128 captured Filter Overdrive values\n";
    passed &= overdriveOK;
    std::ifstream modenvInput(std::string(argv[1]) + "/filter-modenv-sweep.syx", std::ios::binary);
    const std::string modenvBytes((std::istreambuf_iterator<char>(modenvInput)), {});
    bool modenvOK = modenvBytes.size() == 128 * 154;
    if (modenvOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(modenvBytes.data() + raw * 154, 154));
            modenvOK &= decoded.valid && decoded.filterModEnv == raw;
        }
    }
    std::cout << (modenvOK ? "PASS" : "FAIL") << " all 128 captured Filter Mod Env values\n";
    passed &= modenvOK;
    std::ifstream lfo2Input(std::string(argv[1]) + "/filter-lfo2-sweep.syx", std::ios::binary);
    const std::string lfo2Bytes((std::istreambuf_iterator<char>(lfo2Input)), {});
    bool lfo2OK = lfo2Bytes.size() == 256 * 154;
    if (lfo2OK) {
        for (int raw = 0; raw < 256; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(lfo2Bytes.data() + raw * 154, 154));
            lfo2OK &= decoded.valid && decoded.filterLfo2 == raw;
        }
    }
    std::cout << (lfo2OK ? "PASS" : "FAIL") << " all 256 captured Filter LFO 2 values\n";
    passed &= lfo2OK;
    std::ifstream switchInput(std::string(argv[1]) + "/filter-switches.syx", std::ios::binary);
    const std::string switchBytes((std::istreambuf_iterator<char>(switchInput)), {});
    bool switchesOK = switchBytes.size() == 12 * 154;
    if (switchesOK) {
        int index = 0;
        for (int type = 0; type < 2; ++type)
            for (int shape = 0; shape < 3; ++shape)
                for (int slope = 0; slope < 2; ++slope) {
                    const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(switchBytes.data() + index++ * 154, 154));
                    switchesOK &= decoded.valid && decoded.filterType == type
                        && decoded.filterShape == (type == 1 ? 0 : shape)
                        && decoded.filterSlope == (type == 1 ? 1 : slope);
                }
    }
    std::cout << (switchesOK ? "PASS" : "FAIL") << " all 12 captured Filter switch cases\n";
    passed &= switchesOK;
    std::ifstream trackingInput(std::string(argv[1]) + "/filter-tracking-sweep.syx", std::ios::binary);
    const std::string trackingBytes((std::istreambuf_iterator<char>(trackingInput)), {});
    bool trackingOK = trackingBytes.size() == 8 * 154;
    if (trackingOK) {
        for (int raw = 0; raw < 8; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(trackingBytes.data() + raw * 154, 154));
            trackingOK &= decoded.valid && decoded.filterTracking == raw;
        }
    }
    std::cout << (trackingOK ? "PASS" : "FAIL") << " all eight captured Filter Tracking values\n";
    passed &= trackingOK;
    std::ifstream attackInput(std::string(argv[1]) + "/amp-attack-sweep.syx", std::ios::binary);
    const std::string attackBytes((std::istreambuf_iterator<char>(attackInput)), {});
    bool attackOK = attackBytes.size() == 128 * 154;
    if (attackOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(attackBytes.data() + raw * 154, 154));
            attackOK &= decoded.valid && decoded.ampAttack == raw;
        }
    }
    std::cout << (attackOK ? "PASS" : "FAIL") << " all 128 captured Amp Attack values\n";
    passed &= attackOK;
    std::ifstream decayInput(std::string(argv[1]) + "/amp-decay-sweep.syx", std::ios::binary);
    const std::string decayBytes((std::istreambuf_iterator<char>(decayInput)), {});
    bool decayOK = decayBytes.size() == 128 * 154;
    if (decayOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(decayBytes.data() + raw * 154, 154));
            decayOK &= decoded.valid && decoded.ampDecay == raw;
        }
    }
    std::cout << (decayOK ? "PASS" : "FAIL") << " all 128 captured Amp Decay values\n";
    passed &= decayOK;
    std::ifstream sustainInput(std::string(argv[1]) + "/amp-sustain-sweep.syx", std::ios::binary);
    const std::string sustainBytes((std::istreambuf_iterator<char>(sustainInput)), {});
    bool sustainOK = sustainBytes.size() == 128 * 154;
    if (sustainOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(sustainBytes.data() + raw * 154, 154));
            sustainOK &= decoded.valid && decoded.ampSustain == raw;
        }
    }
    std::cout << (sustainOK ? "PASS" : "FAIL") << " all 128 captured Amp Sustain values\n";
    passed &= sustainOK;
    std::ifstream releaseInput(std::string(argv[1]) + "/amp-release-sweep.syx", std::ios::binary);
    const std::string releaseBytes((std::istreambuf_iterator<char>(releaseInput)), {});
    bool releaseOK = releaseBytes.size() == 128 * 154;
    if (releaseOK) {
        for (int raw = 0; raw < 128; ++raw) {
            const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(releaseBytes.data() + raw * 154, 154));
            releaseOK &= decoded.valid && decoded.ampRelease == raw;
        }
    }
    std::cout << (releaseOK ? "PASS" : "FAIL") << " all 128 captured Amp Release values\n";
    passed &= releaseOK;
    std::ifstream ampSwitchInput(std::string(argv[1]) + "/amp-switches.syx", std::ios::binary);
    const std::string ampSwitchBytes((std::istreambuf_iterator<char>(ampSwitchInput)), {});
    bool ampSwitchOK = ampSwitchBytes.size() == 6 * 154;
    if (ampSwitchOK) {
        int index = 0;
        for (int trigger = 0; trigger < 3; ++trigger)
            for (int retrigger = 0; retrigger < 2; ++retrigger) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(ampSwitchBytes.data() + index++ * 154, 154));
                ampSwitchOK &= decoded.valid && decoded.ampTrigger == (trigger == 0 ? 1 : trigger == 1 ? 0 : 2)
                    && decoded.ampRetrigger == retrigger;
            }
    }
    std::cout << (ampSwitchOK ? "PASS" : "FAIL") << " six captured Amp trigger/retrigger combinations\n";
    passed &= ampSwitchOK;
    std::ifstream optionsInput(std::string(argv[1]) + "/amp-options.syx", std::ios::binary);
    const std::string optionsBytes((std::istreambuf_iterator<char>(optionsInput)), {});
    bool optionsOK = optionsBytes.size() == 34 * 154;
    if (optionsOK) {
        int index = 0;
        for (int fixed = 0; fixed < 2; ++fixed)
            for (int count = 0; count <= 16; ++count) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(optionsBytes.data() + index++ * 154, 154));
                optionsOK &= decoded.valid && decoded.ampFixed == fixed && decoded.ampRetriggerCount == count;
            }
    }
    std::cout << (optionsOK ? "PASS" : "FAIL") << " 34 captured Amp Fixed Sustain/Retrigger Count combinations\n";
    passed &= optionsOK;
    {
        std::ifstream input(std::string(argv[1]) + "/mod-attack-sweep.syx", std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        bool ok = bytes.size() == 128 * 154;
        if (ok)
            for (int raw = 0; raw < 128; ++raw) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data() + raw * 154, 154));
                ok &= decoded.valid && decoded.modAttack == raw;
            }
        std::cout << (ok ? "PASS" : "FAIL") << " all 128 captured Mod attack values\n";
        passed &= ok;
    }
    {
        std::ifstream input(std::string(argv[1]) + "/mod-decay-sweep.syx", std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        bool ok = bytes.size() == 128 * 154;
        if (ok)
            for (int raw = 0; raw < 128; ++raw) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data() + raw * 154, 154));
                ok &= decoded.valid && decoded.modDecay == raw;
            }
        std::cout << (ok ? "PASS" : "FAIL") << " all 128 captured Mod decay values\n";
        passed &= ok;
    }
    {
        std::ifstream input(std::string(argv[1]) + "/mod-sustain-sweep.syx", std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        bool ok = bytes.size() == 128 * 154;
        if (ok)
            for (int raw = 0; raw < 128; ++raw) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data() + raw * 154, 154));
                ok &= decoded.valid && decoded.modSustain == raw;
            }
        std::cout << (ok ? "PASS" : "FAIL") << " all 128 captured Mod sustain values\n";
        passed &= ok;
    }
    {
        std::ifstream input(std::string(argv[1]) + "/mod-release-sweep.syx", std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        bool ok = bytes.size() == 128 * 154;
        if (ok)
            for (int raw = 0; raw < 128; ++raw) {
                const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data() + raw * 154, 154));
                ok &= decoded.valid && decoded.modRelease == raw;
            }
        std::cout << (ok ? "PASS" : "FAIL") << " all 128 captured Mod release values\n";
        passed &= ok;
    }
    return passed ? 0 : 1;
}
