#pragma once
#include "BSIIMicroTuning.h"
#include <juce_core/juce_core.h>

namespace bsii::microtuning
{
// Local working document. This is not a hardware dump or a patch snapshot.
struct Document
{
    juce::String name { "Equal temperament" };
    int targetTable = 1;
    Table table;
};

inline bool isNumber (const juce::var& value)
{
    return value.isInt() || value.isInt64() || value.isDouble();
}

inline std::optional<juce::String> serialise (const Document& document)
{
    if (! validTable (document.targetTable) || document.name.isEmpty()) return std::nullopt;
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("format", "BSII MicroTune");
    object->setProperty ("version", 1);
    object->setProperty ("name", document.name);
    object->setProperty ("targetTable", document.targetTable);
    juce::Array<juce::var> notes;
    for (const auto& note : document.table.notes()) notes.add (note.pitch);
    object->setProperty ("pitches", notes);
    return juce::JSON::toString (juce::var (object.release()));
}

inline std::optional<Document> parseDocument (const juce::String& text)
{
    juce::var root;
    if (juce::JSON::parse (text, root).failed() || ! root.isObject()) return std::nullopt;
    const auto format = root["format"], version = root["version"], name = root["name"],
               target = root["targetTable"], pitches = root["pitches"];
    if (! format.isString() || format.toString() != "BSII MicroTune"
        || ! version.isInt() || static_cast<int> (version) != 1
        || ! name.isString() || name.toString().isEmpty()
        || ! target.isInt() || ! validTable (static_cast<int> (target))
        || ! pitches.isArray() || pitches.size() != 128) return std::nullopt;
    Document document;
    document.name = name.toString();
    document.targetTable = static_cast<int> (target);
    for (int key = 0; key < 128; ++key)
        if (! isNumber (pitches[key]) || ! document.table.setPitch (key, static_cast<double> (pitches[key])))
            return std::nullopt;
    return document;
}
}
