#include "BSIIPatchFields.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <string>

using Values = std::map<std::string, std::optional<double>>;
static std::optional<Values> readSnapshot(const char* path) {
    const auto xml = juce::parseXML(juce::File(juce::String::fromUTF8(path)));
    if (!xml || !xml->hasTagName("STATE")) return std::nullopt;
    Values values;
    for (const auto* child : xml->getChildIterator()) {
        if (!child->hasTagName("PARAM") || !child->hasAttribute("id")) return std::nullopt;
        const auto id = child->getStringAttribute("id").toStdString();
        if (values.count(id)) return std::nullopt;
        values[id] = child->hasAttribute("value")
            ? std::optional<double>(child->getDoubleAttribute("value")) : std::nullopt;
    }
    return values;
}
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    const auto expected = readSnapshot(argv[2]);
    if (!expected) return 2;
    if (std::string(argv[1]) == "snapshot") {
        const auto actual = readSnapshot(argv[3]);
        const bool ok = actual && *actual == *expected;
        std::cout << (ok ? "PASS" : "FAIL") << " snapshot parameter entries: " << expected->size() << "\n";
        return ok ? 0 : 1;
    }
    if (std::string(argv[1]) != "patch") return 2;
    std::ifstream input(argv[3], std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)), {});
    const auto decoded = bsii::decodePatchFields(juce::MemoryBlock(bytes.data(), bytes.size()));
    if (!decoded.valid) return 2;
    const auto& fields = bsii::patchFields;
    bool ok = true;
    for (const auto& field : fields) {
        const auto found = expected->find(field.id);
        const int actual = decoded.*(field.member);
        if (found == expected->end() || !found->second || *found->second != actual) {
            std::cout << "FAIL " << field.id << " actual " << actual << "\n";
            ok = false;
        }
    }
    std::cout << (ok ? "PASS" : "FAIL") << " all " << std::size(fields) << " recalled editor controls\n";
    return ok ? 0 : 1;
}
