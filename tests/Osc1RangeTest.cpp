#include "BSIIPatchDecoder.h"
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
    return passed ? 0 : 1;
}
