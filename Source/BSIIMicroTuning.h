#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace bsii::microtuning
{
// Offline codec only. No MIDI device access or persistent-save command.
// Conservative application range avoids unverified top-of-range MTS encodings.
inline constexpr double minimumPitch = 0.0, maximumPitch = 127.0;
inline constexpr int unitsPerSemitone = 16384;
using PitchBytes = std::array<std::uint8_t, 3>;
using Message = std::vector<std::uint8_t>;
using Messages = std::vector<Message>;

inline bool validTable (int table) { return table >= 0 && table <= 8; }
inline bool validKey (int key) { return key >= 0 && key < 128; }

// Pitch is an absolute MIDI-note coordinate: 69.5 is A4 +50 cents.
inline std::optional<PitchBytes> encodePitch (double pitch)
{
    if (! std::isfinite (pitch) || pitch < minimumPitch || pitch > maximumPitch)
        return std::nullopt;
    const auto units = static_cast<int> (std::llround (pitch * unitsPerSemitone));
    return PitchBytes { static_cast<std::uint8_t> (units >> 14),
                        static_cast<std::uint8_t> ((units >> 7) & 127),
                        static_cast<std::uint8_t> (units & 127) };
}

inline std::optional<double> decodePitch (const PitchBytes& bytes)
{
    for (auto byte : bytes) if (byte > 127) return std::nullopt;
    const auto pitch = static_cast<double> ((bytes[0] << 14) | (bytes[1] << 7) | bytes[2])
                     / unitsPerSemitone;
    if (pitch > maximumPitch) return std::nullopt;
    return pitch;
}

struct NoteChange { int key; double pitch; };

class Table
{
public:
    Table()
    {
        for (std::size_t key = 0; key < pitches.size(); ++key)
            pitches[key] = static_cast<double> (key);
    }

    std::optional<double> pitch (int key) const
    {
        if (! validKey (key)) return std::nullopt;
        return pitches[static_cast<std::size_t> (key)];
    }

    bool setPitch (int key, double value)
    {
        if (! validKey (key) || ! encodePitch (value)) return false;
        pitches[static_cast<std::size_t> (key)] = value;
        return true;
    }

    // Offset is relative to this key's equal-tempered pitch, never cumulative.
    bool setCents (int key, double cents)
    {
        return validKey (key) && std::isfinite (cents)
            && setPitch (key, static_cast<double> (key) + cents / 100.0);
    }

    std::array<NoteChange, 128> notes() const
    {
        std::array<NoteChange, 128> result {};
        for (std::size_t key = 0; key < pitches.size(); ++key)
            result[key] = { static_cast<int> (key), pitches[key] };
        return result;
    }

private:
    std::array<double, 128> pitches {};
};

// Novation's documented real-time note-change family; framing includes F0/F7.
// Validate the entire operation first: invalid input produces no partial output.
// Batches of 16 are an application choice, not a hardware-tested maximum.
inline std::optional<Messages> encodeChanges (int table, std::span<const NoteChange> changes,
                                             std::size_t entriesPerMessage = 16)
{
    if (! validTable (table) || changes.empty() || entriesPerMessage == 0
        || entriesPerMessage > 127) return std::nullopt;
    std::array<bool, 128> seen {};
    for (const auto& change : changes)
    {
        if (! validKey (change.key) || ! encodePitch (change.pitch)) return std::nullopt;
        const auto key = static_cast<std::size_t> (change.key);
        if (seen[key]) return std::nullopt;
        seen[key] = true;
    }

    Messages result;
    for (std::size_t start = 0; start < changes.size(); start += entriesPerMessage)
    {
        const auto count = std::min (entriesPerMessage, changes.size() - start);
        Message message { 0xf0, 0x7f, 0x00, 0x08, 0x02,
                          static_cast<std::uint8_t> (table), static_cast<std::uint8_t> (count) };
        for (std::size_t offset = 0; offset < count; ++offset)
        {
            const auto& change = changes[start + offset];
            message.push_back (static_cast<std::uint8_t> (change.key));
            const auto bytes = *encodePitch (change.pitch);
            message.insert (message.end(), bytes.begin(), bytes.end());
        }
        message.push_back (0xf7);
        result.push_back (std::move (message));
    }
    return result;
}
}
