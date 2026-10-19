#include "BSIITuningDocument.h"
#include <iostream>
#include <limits>
#include <set>

namespace mt = bsii::microtuning;
namespace
{
int failures = 0;
void check (bool result, const char* label)
{
    if (! result) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

void golden (int table, std::initializer_list<mt::NoteChange> notes,
             const mt::Message& expected, const char* label)
{
    const auto messages = mt::encodeChanges (table, { notes.begin(), notes.size() });
    check (messages && messages->size() == 1 && messages->front() == expected, label);
}
}

int main()
{
    // Independent expected bytes copied from Novation's published examples.
    golden (0, { {69, 71.0} },
            {0xf0,0x7f,0,8,2,0,1,0x45,0x47,0,0,0xf7}, "documented A4 to B4");
    golden (1, { {69, 69.5} },
            {0xf0,0x7f,0,8,2,1,1,0x45,0x45,0x40,0,0xf7}, "documented A4 +50 cents");
    golden (0, { {69, 71.0}, {71, 72.0} },
            {0xf0,0x7f,0,8,2,0,2,0x45,0x47,0,0,0x47,0x48,0,0,0xf7}, "documented two-note edit");
    golden (1, { {69, 68.5} },
            {0xf0,0x7f,0,8,2,1,1,0x45,0x44,0x40,0,0xf7}, "negative cents uses lower semitone");
    check (mt::encodePitch (69.99999) == mt::PitchBytes {70,0,0}, "fraction carry into next semitone");
    check (mt::encodePitch (0) == mt::PitchBytes {0,0,0}
           && mt::encodePitch (127) == mt::PitchBytes {127,0,0}, "application endpoints");
    bool precisionOK = true;
    for (int units = 0; units <= 127 * mt::unitsPerSemitone; units += 31)
    {
        const auto pitch = static_cast<double> (units) / mt::unitsPerSemitone;
        const auto encoded = mt::encodePitch (pitch);
        const auto expected = mt::PitchBytes { static_cast<std::uint8_t> (units >> 14),
            static_cast<std::uint8_t> ((units >> 7) & 127), static_cast<std::uint8_t> (units & 127) };
        precisionOK &= encoded && *encoded == expected && mt::decodePitch (*encoded) == pitch;
        const auto between = pitch + 0.37 / mt::unitsPerSemitone;
        const auto rounded = mt::encodePitch (between);
        precisionOK &= rounded && std::abs (*mt::decodePitch (*rounded) - between)
                                     <= 0.5 / mt::unitsPerSemitone;
    }
    check (precisionOK, "packing and half-step error bound across pitch range");
    for (double invalid : { -0.001, 127.001, std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN() })
        check (! mt::encodePitch (invalid), "invalid pitch rejected rather than clipped");
    check (! mt::decodePitch ({69,128,0}) && ! mt::decodePitch ({127,127,127}), "invalid/reserved bytes rejected");

    mt::Table table;
    bool identity = true;
    for (int key = 0; key < 128; ++key) identity &= table.pitch (key) == static_cast<double> (key);
    check (identity && ! table.pitch (-1) && ! table.pitch (128), "128-note identity and key bounds");
    check (table.setCents (69, -50) && table.pitch (69) == 68.5, "negative edit");
    check (table.setCents (69, 50) && table.pitch (69) == 69.5, "edits are absolute offsets, not cumulative");
    check (! table.setCents (0,-1) && table.pitch (0) == 0
           && ! table.setCents (127,1) && table.pitch (127) == 127
           && ! table.setPitch (69,999) && table.pitch (69) == 69.5, "failed edits preserve previous values");

    const auto notes = table.notes();
    bool chunksOK = true;
    for (std::size_t batch : {std::size_t {1}, std::size_t {16}, std::size_t {127}})
    {
        const auto messages = mt::encodeChanges (8, notes, batch);
        chunksOK &= messages && messages->size() == (128 + batch - 1) / batch;
        std::set<int> seen;
        if (! messages) continue;
        for (const auto& message : *messages)
        {
            chunksOK &= message.size() == 8 + 4 * static_cast<std::size_t> (message[6]);
            chunksOK &= message[0] == 0xf0 && message[1] == 0x7f && message[2] == 0
                     && message[3] == 8 && message[4] == 2 && message[5] == 8
                     && message[6] > 0 && message[6] <= batch && message.back() == 0xf7;
            for (std::size_t i = 1; i + 1 < message.size(); ++i) chunksOK &= message[i] < 128;
            for (std::size_t i = 7; i + 3 < message.size() - 1; i += 4)
            {
                seen.insert (message[i]);
                const auto pitch = mt::decodePitch ({message[i+1],message[i+2],message[i+3]});
                chunksOK &= pitch && std::abs (*pitch - *table.pitch (message[i])) <= 0.5 / mt::unitsPerSemitone;
            }
        }
        chunksOK &= seen.size() == 128;
    }
    check (chunksOK, "all keys encoded exactly once across chunk boundaries");
    check (! mt::encodeChanges (-1,notes) && ! mt::encodeChanges (9,notes)
           && ! mt::encodeChanges (1,notes,0) && ! mt::encodeChanges (1,notes,128)
           && ! mt::encodeChanges (1,{}), "invalid table/count and empty operation rejected");
    auto invalidNotes = notes;
    invalidNotes.back().pitch = 128;
    check (! mt::encodeChanges (1,invalidNotes), "late invalid entry rejects complete operation");
    invalidNotes = notes;
    invalidNotes.back().key = 0;
    check (! mt::encodeChanges (1,invalidNotes), "duplicate key across chunks rejected");

    mt::Document document;
    document.table = table;
    document.targetTable = 8;
    document.name = "Local tuning \"test\"\n";
    const auto text = mt::serialise (document);
    const auto restored = text ? mt::parseDocument (*text) : std::nullopt;
    check (restored && restored->name == document.name && restored->targetTable == 8
           && restored->table.pitch (69) == 69.5 && restored->table.pitch (127) == 127,
           "versioned local document round trip");
    check (! mt::parseDocument ("not json") && ! mt::parseDocument ("[]")
           && ! mt::parseDocument ("{}"), "malformed and incomplete documents rejected");
    if (text)
    {
        const auto reject = [&text] (const char* property, const juce::var& value)
        {
            auto root = juce::JSON::parse (*text);
            // JUCE array equality can coerce element types; replace unconditionally
            // so malformed fixtures really contain the requested strings/bools.
            root.getDynamicObject()->removeProperty (property);
            root.getDynamicObject()->setProperty (property,value);
            return ! mt::parseDocument (juce::JSON::toString (root));
        };
        check (reject ("version",2) && reject ("version","1")
               && reject ("format","patch") && reject ("name","")
               && reject ("targetTable",9) && reject ("targetTable",true), "document schema/type guards");
        auto root = juce::JSON::parse (*text);
        auto pitches = *root["pitches"].getArray();
        pitches.set (69, "69.5");
        check (reject ("pitches",pitches), "string pitch rejected");
        pitches.set (69, true);
        check (reject ("pitches",pitches), "boolean pitch rejected");
        pitches.set (69, 128.0);
        check (reject ("pitches",pitches), "out-of-range imported pitch rejected");
        pitches.removeLast();
        check (reject ("pitches",pitches), "partial table rejected");
    }
    std::cout << (failures == 0 ? "PASS" : "FAIL") << " offline MicroTune codec/model/document checks ("
              << failures << " failures). No hardware I/O.\n";
    return failures == 0 ? 0 : 1;
}
