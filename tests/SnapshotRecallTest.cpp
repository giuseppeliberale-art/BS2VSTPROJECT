#include "BSIIPatchDecoder.h"
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
    struct Field { const char* id; int bsii::DecodedPatchFields::* member; };
    const Field fields[] = {
        {"patchVolume", &bsii::DecodedPatchFields::patchVolume},
        {"osc1Fine", &bsii::DecodedPatchFields::osc1Fine},
        {"osc1Coarse", &bsii::DecodedPatchFields::osc1Coarse},
        {"osc1ModEnvPitch", &bsii::DecodedPatchFields::osc1ModEnvPitch},
        {"osc1Lfo1Pitch", &bsii::DecodedPatchFields::osc1Lfo1Pitch},
        {"osc1ModEnvPW", &bsii::DecodedPatchFields::osc1ModEnvPW},
        {"osc1Lfo2PW", &bsii::DecodedPatchFields::osc1Lfo2PW},
        {"osc1ManualPW", &bsii::DecodedPatchFields::osc1ManualPW},
        {"osc2Fine", &bsii::DecodedPatchFields::osc2Fine},
        {"osc2Coarse", &bsii::DecodedPatchFields::osc2Coarse},
        {"osc2ModEnvPitch", &bsii::DecodedPatchFields::osc2ModEnvPitch},
        {"osc2Lfo1Pitch", &bsii::DecodedPatchFields::osc2Lfo1Pitch},
        {"osc2ModEnvPW", &bsii::DecodedPatchFields::osc2ModEnvPW},
        {"osc2Lfo2PW", &bsii::DecodedPatchFields::osc2Lfo2PW},
        {"osc2ManualPW", &bsii::DecodedPatchFields::osc2ManualPW},
        {"mixOsc1", &bsii::DecodedPatchFields::mixOsc1},
        {"mixOsc2", &bsii::DecodedPatchFields::mixOsc2},
        {"mixSub", &bsii::DecodedPatchFields::mixSub},
        {"mixNoise", &bsii::DecodedPatchFields::mixNoise},
        {"mixRing", &bsii::DecodedPatchFields::mixRing},
        {"mixExternal", &bsii::DecodedPatchFields::mixExternal},
        {"subOctaveRaw", &bsii::DecodedPatchFields::subOctaveRaw},
        {"filterCutoff", &bsii::DecodedPatchFields::filterCutoff},
        {"filterResonance", &bsii::DecodedPatchFields::filterResonance},
        {"filterModEnv", &bsii::DecodedPatchFields::filterModEnv},
        {"filterLfo2", &bsii::DecodedPatchFields::filterLfo2},
        {"filterOverdrive", &bsii::DecodedPatchFields::filterOverdrive},
        {"portamento", &bsii::DecodedPatchFields::portamento},
        {"lfo1Speed", &bsii::DecodedPatchFields::lfo1Speed},
        {"lfo1Delay", &bsii::DecodedPatchFields::lfo1Delay},
        {"lfo2Speed", &bsii::DecodedPatchFields::lfo2Speed},
        {"lfo2Delay", &bsii::DecodedPatchFields::lfo2Delay},
        {"lfo1Slew", &bsii::DecodedPatchFields::lfo1Slew},
        {"lfo2Slew", &bsii::DecodedPatchFields::lfo2Slew},
        {"filterTracking", &bsii::DecodedPatchFields::filterTracking},
        {"ampAttack", &bsii::DecodedPatchFields::ampAttack},
        {"ampDecay", &bsii::DecodedPatchFields::ampDecay},
        {"ampSustain", &bsii::DecodedPatchFields::ampSustain},
        {"ampRelease", &bsii::DecodedPatchFields::ampRelease},
        {"ampRetriggerCount", &bsii::DecodedPatchFields::ampRetriggerCount},
        {"modAttack", &bsii::DecodedPatchFields::modAttack},
        {"modDecay", &bsii::DecodedPatchFields::modDecay},
        {"modSustain", &bsii::DecodedPatchFields::modSustain},
        {"modRelease", &bsii::DecodedPatchFields::modRelease},
        {"modRetriggerCount", &bsii::DecodedPatchFields::modRetriggerCount},
        {"distortion", &bsii::DecodedPatchFields::distortion},
        {"oscSyncRaw", &bsii::DecodedPatchFields::oscSyncRaw},
        {"oscFilterMod", &bsii::DecodedPatchFields::oscFilterMod},
        {"arpSwing", &bsii::DecodedPatchFields::arpSwing},
        {"bendAmount", &bsii::DecodedPatchFields::bendAmount},
        {"velocityAmp", &bsii::DecodedPatchFields::velocityAmp},
        {"velocityMod", &bsii::DecodedPatchFields::velocityMod},
        {"vcaLimit", &bsii::DecodedPatchFields::vcaLimit},
        {"modWhLfo1Pitch", &bsii::DecodedPatchFields::modWhLfo1Pitch},
        {"modWhOsc2Pitch", &bsii::DecodedPatchFields::modWhOsc2Pitch},
        {"aftertouchFilter", &bsii::DecodedPatchFields::aftertouchFilter},
        {"aftertouchLfo1", &bsii::DecodedPatchFields::aftertouchLfo1},
        {"aftertouchLfo2Spd", &bsii::DecodedPatchFields::aftertouchLfo2Spd},
        {"osc1Wave", &bsii::DecodedPatchFields::osc1Wave},
        {"subWave", &bsii::DecodedPatchFields::subWave},
        {"osc1Range", &bsii::DecodedPatchFields::osc1Range},
        {"osc2Wave", &bsii::DecodedPatchFields::osc2Wave},
        {"osc2Range", &bsii::DecodedPatchFields::osc2Range},
        {"filterType", &bsii::DecodedPatchFields::filterType},
        {"filterShape", &bsii::DecodedPatchFields::filterShape},
        {"filterSlope", &bsii::DecodedPatchFields::filterSlope},
        {"lfo1Wave", &bsii::DecodedPatchFields::lfo1Wave},
        {"lfo2Wave", &bsii::DecodedPatchFields::lfo2Wave},
        {"ampTrigger", &bsii::DecodedPatchFields::ampTrigger},
        {"modTrigger", &bsii::DecodedPatchFields::modTrigger},
        {"ampRetrigger", &bsii::DecodedPatchFields::ampRetrigger},
        {"modRetrigger", &bsii::DecodedPatchFields::modRetrigger},
        {"ampFixed", &bsii::DecodedPatchFields::ampFixed},
        {"modFixed", &bsii::DecodedPatchFields::modFixed},
        {"lfo1KeySync", &bsii::DecodedPatchFields::lfo1KeySync},
        {"lfo2KeySync", &bsii::DecodedPatchFields::lfo2KeySync}
    };
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
