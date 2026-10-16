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
    return passed ? 0 : 1;
}
