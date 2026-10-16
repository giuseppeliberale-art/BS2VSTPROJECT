#include "BSIIPatchDecoder.h"
#include "BSIITuningDisplay.h"
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
    return passed ? 0 : 1;
}
