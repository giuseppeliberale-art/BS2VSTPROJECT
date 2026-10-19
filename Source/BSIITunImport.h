#pragma once
#include "BSIIScalaImport.h"
#include <set>

namespace bsii::microtuning::tun
{
// Numeric TUN v0/v1 adapter. Reuse the existing local preview/apply policy.
// Never fall back to numeric compatibility data when higher-priority formula or
// mapping sections are present: that could silently import a different tuning.
inline std::string lower (std::string text)
{
    for (auto& c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char> (c + ('a' - 'A'));
    return text;
}
inline std::string uncomment (const std::string& text)
{
    bool quoted = false, escaped = false;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const auto c = text[i];
        if (! escaped && c == '"') quoted = ! quoted;
        if (! quoted && c == ';') return text.substr (0,i);
        escaped = ! escaped && c == '\\';
    }
    return text;
}
inline int integer (std::string text, int low, int high)
{
    if (! text.empty() && text.front() == '+') text.erase (0,1);
    return scala::integer (text,low,high);
}
inline std::optional<std::string> label (const std::string& value)
{
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') return std::nullopt;
    std::string result;
    for (std::size_t i = 1; i + 1 < value.size(); ++i)
    {
        auto c = value[i];
        if (c == '"') return std::nullopt;
        if (c == '\\')
        {
            if (++i >= value.size() - 1) return std::nullopt;
            c = value[i];
            if (c == 'n' || c == 'r' || c == 't') c = ' ';
            else if (c != '"' && c != '\\') return std::nullopt;
        }
        result += c;
    }
    return result;
}
inline scala::Preview parseNumeric (const std::string& input, const std::string& fallbackName = "Imported TUN table")
{
    scala::Preview result;
    try
    {
        if (input.empty() || input.size() > 65536) throw std::runtime_error ("Use a nonempty TUN file up to 64 KiB.");
        std::string text = input;
        if (text.starts_with ("\xef\xbb\xbf")) text.erase (0,3);
        // The TUN specification permits NUL, CR and LF as line delimiters.
        for (auto& c : text) if (c == '\r' || c == '\0') c = '\n';
        std::istringstream stream (text);
        std::string section;
        std::set<std::string> seen;
        std::array<double,128> quantized {}, exact {};
        for (int key = 0; key < 128; ++key) quantized[key] = exact[key] = key * 100.0;
        bool hasQuantized = false, hasExact = false;
        // Select the governing section before validating its data. Lower-priority
        // compatibility content must really be ignored, regardless of file order.
        std::istringstream headers (text);
        for (std::string row; std::getline (headers,row);)
        {
            row = scala::trim (uncomment (row));
            if (row.size() < 2 || row.front() != '[' || row.back() != ']') continue;
            row = lower (scala::trim (row.substr (1,row.size() - 2)));
            if (row == "exact tuning") hasExact = true;
            if (row == "functional tuning" || row == "mapping")
                throw std::runtime_error ("Formula-based TUN v2/mapping is not supported. Export a numeric TUN v1 table.");
        }
        int highestExact = -1;
        double baseHz = 8.1757989156437073336;
        result.name = fallbackName.empty() ? "Imported TUN table" : fallbackName;
        for (std::string row; std::getline (stream,row);)
        {
            row = scala::trim (uncomment (row));
            if (row.empty()) continue;
            if (row.front() == '[')
            {
                if (row.back() != ']') throw std::runtime_error ("Malformed TUN section header.");
                section = lower (scala::trim (row.substr (1,row.size() - 2)));
                if (section == "functional tuning" || section == "mapping")
                    throw std::runtime_error ("Formula-based TUN v2/mapping is not supported. Export a numeric TUN v1 table.");
                if (section == "tuning" || section == "exact tuning" || section == "scale begin" || section == "scale end" || section == "info")
                    if (! seen.insert (section).second) throw std::runtime_error ("Repeated section or multiple-scale TUN file is not supported.");
                if (section == "tuning") hasQuantized = true;
                if (section == "exact tuning") hasExact = true;
                continue;
            }
            if (section == "tuning" && hasExact) continue;
            const auto equals = row.find ('=');
            if (equals == std::string::npos)
            {
                const auto textKey = lower (row);
                if ((section == "tuning" || section == "exact tuning")
                    && (textKey == "note" || textKey.starts_with ("note ") || textKey.starts_with ("note\t")
                        || textKey == "basefreq" || textKey.starts_with ("basefreq ")))
                    throw std::runtime_error ("Malformed numeric TUN assignment.");
                continue; // Unrecognised metadata/content is ignored.
            }
            const auto key = lower (scala::trim (row.substr (0,equals)));
            const auto value = scala::trim (row.substr (equals + 1));
            if (section == "scale begin")
            {
                if (key == "formatversion" && integer (value,0,65535) != 100)
                    throw std::runtime_error ("Only numeric TUN v0/v1 tables are supported.");
                if (key == "format" && lower (value) != "\"anamark-tun\"")
                    throw std::runtime_error ("Unknown TUN format identifier.");
            }
            else if (section == "info" && key == "name")
            {
                if (auto name = label (value); name && ! name->empty()) result.name = *name;
            }
            else if (section == "exact tuning" && key == "basefreq")
            {
                baseHz = scala::number (value);
                if (baseHz < 0.000001 || baseHz > 1.0e9) throw std::runtime_error ("TUN BaseFreq outside supported range.");
            }
            else if (section == "tuning" || section == "exact tuning")
            {
                std::istringstream words (key);
                std::string name, index, extra;
                words >> name;
                if (name != "note") continue;
                if (! (words >> index) || words >> extra) throw std::runtime_error ("Malformed TUN note key.");
                const auto note = integer (index,0,127);
                const auto cents = section == "tuning" ? static_cast<double> (integer (value,-120000,120000)) : scala::number (value);
                if (std::abs (cents) > 120000) throw std::runtime_error ("TUN cents outside supported range (+/-120000).");
                if (section == "tuning") quantized[note] = cents;
                else { exact[note] = cents; highestExact = std::max (highestExact,note); }
            }
        }
        if (! hasExact && ! hasQuantized) throw std::runtime_error ("No numeric TUN tuning section found.");
        if (hasExact && highestExact == 0)
            throw std::runtime_error ("Cannot expand an Exact Tuning period defined only at note 0. Export all 128 notes.");
        if (hasExact && highestExact > 0 && highestExact < 127)
        {
            const auto period = exact[highestExact];
            for (int key = highestExact; key < 128; ++key) exact[key] = exact[key - highestExact] + period;
        }
        const auto& cents = hasExact ? exact : quantized;
        if (! hasExact) baseHz = 8.1757989156437073336;
        const auto basePitch = 69.0 + 12.0 * std::log2 (baseHz / 440.0);
        result.referenceHz = baseHz; // TUN base reference, not the frequency assigned to key 0.
        for (int key = 0; key < 128; ++key)
        {
            auto& note = result.notes[key];
            note.pitch = basePitch + cents[key] / 100.0;
            if (std::abs (note.pitch) < 1.0e-10) note.pitch = 0;
            if (std::abs (note.pitch - 127) < 1.0e-10) note.pitch = 127;
            note.hz = baseHz * std::exp2 (cents[key] / 1200.0);
            note.status = std::isfinite (note.hz) && note.hz > 0 && encodePitch (note.pitch)
                ? scala::NoteStatus::available : scala::NoteStatus::outsidePitchRange;
            if (note.status == scala::NoteStatus::available) ++result.available;
        }
    }
    catch (const std::exception& error) { result.error = error.what(); }
    return result;
}
// V2 lives in a compiled adapter so third-party implementation details do not
// spread into editor/processor translation units.
scala::Preview parseV2 (const std::string&, const std::string&);
inline scala::Preview parse (const std::string& input, const std::string& fallbackName = "Imported TUN table")
{
    if (input.empty() || input.size() > 65536) { scala::Preview failed; failed.error = "Use a nonempty TUN file up to 64 KiB."; return failed; }
    std::string text = input;
    for (auto& c : text) if (c == '\r' || c == '\0') c = '\n';
    std::istringstream lines (text);
    std::string section;
    for (std::string row; std::getline (lines,row);)
    {
        row = scala::trim (uncomment (row));
        if (row.size() >= 2 && row.front() == '[' && row.back() == ']')
        {
            section = lower (scala::trim (row.substr (1,row.size()-2)));
            if (section == "functional tuning" || section == "mapping") return parseV2 (input,fallbackName);
        }
        const auto equal = row.find ('=');
        if (section == "scale begin" && equal != std::string::npos
            && lower (scala::trim (row.substr (0,equal))) == "formatversion"
            && scala::trim (row.substr (equal+1)) == "200") return parseV2 (input,fallbackName);
    }
    return parseNumeric (input,fallbackName);
}

}
