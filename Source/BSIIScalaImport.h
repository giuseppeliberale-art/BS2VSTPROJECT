#pragma once
#include "BSIITuningDocument.h"
#include "ThirdParty/tuning-library/Tunings.h"
#include <charconv>
#include <sstream>

namespace bsii::microtuning::scala
{
struct Options { int scaleStart = 60, referenceNote = 69; double referenceHz = 440; };
enum class NoteStatus { available, unmapped, outsideKeyboard, outsidePitchRange, muted };
struct Note { NoteStatus status = NoteStatus::unmapped; double pitch = 0, hz = 0; };
struct Preview
{
    std::string name, error;
    int scaleStart = 60, referenceNote = 69, firstKey = 0, lastKey = 127;
    double referenceHz = 440;
    std::array<Note,128> notes {};
    int available = 0;
    std::string sourceInfo;
    bool valid() const { return error.empty() && available > 0; }
};

inline std::string trim (std::string text)
{
    const auto first = text.find_first_not_of (" \t\r");
    return first == std::string::npos ? "" : text.substr (first, text.find_last_not_of (" \t\r") - first + 1);
}
inline double number (std::string text)
{
    if (! text.empty() && text.front() == '+') text.erase (0,1);
    double value = 0;
    const auto result = std::from_chars (text.data(), text.data() + text.size(), value);
    if (text.empty() || result.ec != std::errc {} || result.ptr != text.data() + text.size() || ! std::isfinite (value))
        throw std::runtime_error ("Invalid numeric value.");
    return value;
}
inline int integer (const std::string& text, int low, int high)
{
    int value = 0;
    const auto result = std::from_chars (text.data(), text.data() + text.size(), value);
    if (text.empty() || result.ec != std::errc {} || result.ptr != text.data() + text.size() || value < low || value > high)
        throw std::runtime_error ("Integer outside supported range.");
    return value;
}
inline std::vector<std::string> lines (const std::string& text, bool scale)
{
    if (text.empty() || text.size() > 65536 || text.find('\0') != std::string::npos)
        throw std::runtime_error ("Use a nonempty Scala file up to 64 KiB.");
    std::vector<std::string> result;
    std::string normalized = text;
    if (normalized.starts_with ("\xef\xbb\xbf")) normalized.erase (0,3);
    for (std::size_t i = 0; i < normalized.size(); ++i)
        if (normalized[i] == '\r')
        {
            if (i + 1 < normalized.size() && normalized[i + 1] == '\n') normalized.erase (i,1);
            else normalized[i] = '\n';
        }
    std::istringstream stream (normalized);
    for (std::string row; std::getline (stream,row);)
    {
        row = trim (row);
        if (! row.empty() && row.front() == '!') continue;
        // An empty scale description is meaningful; other blank lines are skipped.
        if (row.empty() && (! scale || ! result.empty())) continue;
        if (result.size() > 1032) throw std::runtime_error ("Too many Scala entries (maximum 1024 notes).");
        result.push_back (row);
    }
    return result;
}
inline Preview parse (const std::string& scl, const std::optional<std::string>& kbm = std::nullopt,
                      Options options = {})
{
    Preview result;
    try
    {
        auto rows = lines (scl,true);
        if (rows.size() < 3) throw std::runtime_error ("Incomplete scale.");
        const auto token = [] (const std::string& row)
        { return row.substr (0, row.find_first_of (" \t!")); };
        const auto count = integer (token (rows[1]),1,1024);
        if (rows.size() != static_cast<std::size_t> (count + 2)) throw std::runtime_error ("Scale note count does not match file.");
        std::ostringstream canonical;
        canonical << rows[0] << '\n' << count << '\n';
        for (int i = 0; i < count; ++i)
        {
            const auto text = token (rows[static_cast<std::size_t> (i + 2)]);
            if (text.find ('.') != std::string::npos)
            {
                const auto cents = number (text);
                if (std::abs (cents) > 120000 || (i == count - 1 && cents <= 0))
                    throw std::runtime_error ("Scale period must be positive; intervals must be within 120000 cents.");
            }
            else
            {
                const auto slash = text.find ('/');
                integer (text.substr (0,slash),1,2147483647);
                if (slash != std::string::npos) integer (text.substr (slash + 1),1,2147483647);
                if (i == count - 1 && Tunings::toneFromString (text).cents <= 0)
                    throw std::runtime_error ("Scale period must be positive.");
            }
            canonical << text << '\n';
        }
        const auto scale = Tunings::parseSCLData (canonical.str());
        Tunings::KeyboardMapping mapping;
        if (kbm)
        {
            auto map = lines (*kbm,false);
            if (map.size() < 7) throw std::runtime_error ("Incomplete keyboard mapping.");
            for (auto& row : map) row = token (row);
            const auto size = integer (map[0],0,1024);
            if (map.size() != static_cast<std::size_t> (size + 7)) throw std::runtime_error ("Keyboard mapping count does not match file.");
            const auto first = integer (map[1],0,127), last = integer (map[2],0,127);
            if (first > last) throw std::runtime_error ("Keyboard range is reversed.");
            integer (map[3],0,127); integer (map[4],0,127);
            const auto hz = number (map[5]);
            if (hz < 0.000001 || hz > 1.0e9) throw std::runtime_error ("Reference frequency outside supported range.");
            integer (map[6],0,count);
            for (std::size_t i = 7; i < map.size(); ++i)
                if (map[i] != "x") integer (map[i],0,count);
            std::ostringstream frequency;
            frequency.imbue (std::locale::classic());
            frequency << std::fixed << std::setprecision (17) << hz;
            map[5] = frequency.str();
            std::string normalized;
            for (const auto& row : map) normalized += row + '\n';
            mapping = Tunings::parseKBMData (normalized);
            mapping.tuningFrequency = hz;
            mapping.tuningPitch = hz / Tunings::MIDI_0_FREQ;
        }
        else
        {
            if (! validKey (options.scaleStart) || ! validKey (options.referenceNote)
                || ! std::isfinite (options.referenceHz) || options.referenceHz < 0.000001 || options.referenceHz > 1.0e9)
                throw std::runtime_error ("Invalid scale start or reference pitch.");
            mapping = Tunings::startScaleOnAndTuneNoteTo (options.scaleStart,options.referenceNote,options.referenceHz);
            // The upstream helper serialises Hz at default stream precision. Keep the user's full value.
            mapping.tuningFrequency = options.referenceHz;
            mapping.tuningPitch = options.referenceHz / Tunings::MIDI_0_FREQ;
        }
        const Tunings::Tuning tuning (scale,mapping);
        result.name = rows[0].empty() ? "Imported Scala scale" : rows[0];
        result.scaleStart = mapping.middleNote; result.referenceNote = mapping.tuningConstantNote;
        result.referenceHz = mapping.tuningFrequency; result.firstKey = mapping.firstMidi; result.lastKey = mapping.lastMidi;
        for (int key = 0; key < 128; ++key)
        {
            auto& note = result.notes[static_cast<std::size_t> (key)];
            if (key < mapping.firstMidi || key > mapping.lastMidi) { note.status = NoteStatus::outsideKeyboard; continue; }
            if (! tuning.isMidiNoteMapped (key)) continue;
            note.hz = tuning.frequencyForMidiNote (key);
            note.pitch = 69.0 + 12.0 * std::log2 (note.hz / 440.0);
            // Snap only floating-point round-off at the endpoints, never a meaningful pitch error.
            if (std::abs (note.pitch) < 1.0e-10) note.pitch = 0;
            if (std::abs (note.pitch - 127) < 1.0e-10) note.pitch = 127;
            note.status = std::isfinite (note.hz) && note.hz > 0 && encodePitch (note.pitch)
                ? NoteStatus::available : NoteStatus::outsidePitchRange;
            if (note.status == NoteStatus::available) ++result.available;
        }
    }
    catch (const std::exception& e) { result.error = e.what(); }
    return result;
}

inline juce::String displayName (const std::string& text)
{
    if (juce::CharPointer_UTF8::isValidString (text.data(),static_cast<int> (text.size())))
        return juce::String::fromUTF8 (text.data(),static_cast<int> (text.size()));
    // Scala descriptions may use Latin-1. Pitch syntax is validated separately.
    juce::String result;
    for (const auto c : text) result += juce::String::charToString (static_cast<juce::juce_wchar> (static_cast<unsigned char> (c)));
    return result;
}

inline std::optional<Document> apply (const Preview& preview, const Document& current, bool keepUnavailable)
{
    if (! preview.valid() || (! keepUnavailable && preview.available != 128)) return std::nullopt;
    auto result = current;
    result.name = displayName (preview.name).substring (0,128);
    for (int key = 0; key < 128; ++key)
        if (preview.notes[static_cast<std::size_t> (key)].status == NoteStatus::available
            && ! result.table.setPitch (key,preview.notes[static_cast<std::size_t> (key)].pitch)) return std::nullopt;
    return result;
}
}
