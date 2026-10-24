#include "BSIIPatchFields.h"
#include "BSIIProjectRecall.h"
#include <iostream>
int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "Usage: BSIIPatchInspect single-patch.syx [...]\n"; return 2; }
    bool allValid = true;
    for (int i = 1; i < argc; ++i)
    {
        juce::MemoryBlock bytes;
        const juce::File file(juce::String::fromUTF8(argv[i]));
        if (!file.loadFileAsData(bytes)) { std::cerr << "Cannot read patch\n"; return 2; }
        const auto decoded = bsii::decodePatchFields(bytes);
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty("file", file.getFullPathName());
        object->setProperty("valid", decoded.valid);
        object->setProperty("editBufferEligible", bsii::isRecallPatch(bytes));
        object->setProperty("error", decoded.error);
        auto fields = std::make_unique<juce::DynamicObject>();
        for (const auto& field : bsii::patchFields)
            fields->setProperty(field.id, decoded.*(field.member));
        object->setProperty("fields", juce::var(fields.release()));
        std::cout << juce::JSON::toString(juce::var(object.release()), true).toStdString() << '\n';
        allValid &= decoded.valid;
    }
    return allValid ? 0 : 1;
}
