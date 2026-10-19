#include "BSIITuningDocument.h"
#include <iostream>
#include <string>

namespace mt = bsii::microtuning;
namespace
{
int fail (const char* message) { std::cerr << message << '\n'; return 1; }

std::optional<int> integer (const char* text)
{
    try
    {
        std::size_t used = 0;
        const auto result = std::stoi (text, &used);
        if (used == std::string (text).size()) return result;
    }
    catch (...) {}
    return std::nullopt;
}

std::optional<double> number (const char* text)
{
    try
    {
        std::size_t used = 0;
        const auto result = std::stod (text, &used);
        if (used == std::string (text).size() && std::isfinite (result)) return result;
    }
    catch (...) {}
    return std::nullopt;
}

juce::File file (const char* path) { return juce::File::getCurrentWorkingDirectory().getChildFile (path); }

std::optional<mt::Document> load (const char* path)
{
    const auto input = file (path);
    if (! input.existsAsFile() || input.getSize() > 65536) return std::nullopt;
    return mt::parseDocument (input.loadFileAsString());
}

bool write (const char* path, const void* data, std::size_t bytes)
{
    const auto output = file (path);
    // Keep previous documents and exports intact; choose a fresh output path.
    if (output.exists() || ! output.getParentDirectory().isDirectory()) return false;
    juce::FileOutputStream stream (output);
    if (! stream.openedOk() || ! stream.write (data, bytes)) return false;
    stream.flush();
    return stream.getStatus().wasOk();
}

bool save (const char* path, const mt::Document& document)
{
    const auto text = mt::serialise (document);
    return text && write (path, text->toRawUTF8(), text->getNumBytesAsUTF8());
}
}

int main (int argc, char** argv)
{
    if (argc < 3)
        return fail ("Offline only. Usage: new OUT.json [TABLE] | set IN.json OUT.json KEY CENTS | export IN.json OUT.syx [BATCH] | inspect IN.json");
    const std::string command (argv[1]);
    if (command == "new" && (argc == 3 || argc == 4))
    {
        mt::Document document;
        if (argc == 4)
        {
            const auto target = integer (argv[3]);
            if (! target || ! mt::validTable (*target)) return fail ("Table must be 0..8.");
            document.targetTable = *target;
        }
        return save (argv[2], document) ? 0 : fail ("Cannot create output; existing files are not overwritten.");
    }
    const auto loaded = load (argv[2]);
    if (! loaded) return fail ("Invalid or missing version-1 BSII MicroTune document (maximum 64 KiB).");
    auto document = *loaded;
    if (command == "set" && argc == 6)
    {
        const auto key = integer (argv[4]);
        const auto cents = number (argv[5]);
        if (! key || ! cents || ! document.table.setCents (*key, *cents))
            return fail ("Invalid key/cents: key must be 0..127, resulting absolute pitch must be 0..127.");
        return save (argv[3], document) ? 0 : fail ("Cannot create output; existing files are not overwritten.");
    }
    if (command == "export" && (argc == 4 || argc == 5))
    {
        const auto batch = argc == 5 ? integer (argv[4]) : std::optional<int> {16};
        if (! batch || *batch < 1 || *batch > 127) return fail ("Batch must be 1..127 notes.");
        const auto notes = document.table.notes();
        const auto messages = mt::encodeChanges (document.targetTable, notes, static_cast<std::size_t> (*batch));
        if (! messages) return fail ("Cannot encode table.");
        mt::Message bytes;
        for (const auto& message : *messages) bytes.insert (bytes.end(), message.begin(), message.end());
        if (! write (argv[3], bytes.data(), bytes.size())) return fail ("Cannot create output; existing files are not overwritten.");
        std::cout << "Exported " << messages->size() << " note-change messages for table " << document.targetTable
                  << ". Offline file only; no MIDI sent and no persistent-save command included.\n";
        return 0;
    }
    if (command == "inspect" && argc == 3)
    {
        int changed = 0;
        for (const auto& note : document.table.notes()) if (note.pitch != note.key) ++changed;
        std::cout << "Local table: " << document.name << "; target " << document.targetTable
                  << "; " << changed << " notes differ from equal temperament. Not hardware readback.\n";
        return 0;
    }
    return fail ("Unknown command or incorrect argument count.");
}
