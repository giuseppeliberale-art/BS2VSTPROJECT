#include "BSIITuningDocument.h"
#include "BSIIScalaImport.h"
#include "BSIITunImport.h"
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
    {
        namespace sc = mt::scala;
        const std::string equal = "! own fixture\nTwelve equal steps\n12\n100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n2/1\n";
        const auto identity = sc::parse (equal);
        check (identity.valid() && identity.available == 128, "Scala identity all notes available");
        bool equalOK = true;
        for (int key = 0; key < 128; ++key) equalOK &= std::abs (identity.notes[key].pitch - key) < 1.0e-9;
        check (equalOK, "Scala identity frequencies independently match MIDI pitches");
        const auto low = sc::parse (equal,std::nullopt,{60,69,432});
        check (low.available == 127 && low.notes[0].status == sc::NoteStatus::outsidePitchRange
               && std::abs (low.notes[69].hz - 432) < 1.0e-8, "A432 preserved and lower endpoint flagged");
        mt::Document before; before.table.setCents (60,35); before.targetTable = 7;
        check (! sc::apply (low,before,false), "incomplete import requires explicit preservation");
        const auto imported = sc::apply (low,before,true);
        check (imported && imported->targetTable == 7 && imported->table.pitch (0) == 0
               && std::abs (*imported->table.pitch (69) - (69+12*std::log2 (432.0/440))) < 1.0e-9,
               "explicit keep preserves unavailable notes and destination metadata");
        const auto high = sc::parse (equal,std::nullopt,{60,69,442});
        check (high.notes[127].status == sc::NoteStatus::outsidePitchRange, "upper endpoint flagged");
        const auto tritave = sc::parse ("Tritave\n1\n3/1\n",std::nullopt,{60,60,440});
        check (tritave.valid() && std::abs (tritave.notes[61].hz - 1320) < 1.0e-8, "non-octave ratio scale");
        std::string nineteen = "19 equal steps\n19\n";
        for (int i = 1; i <= 19; ++i) nineteen += std::to_string (1200.0*i/19) + "\n";
        const auto n19 = sc::parse (nineteen,std::nullopt,{60,60,440});
        check (n19.valid() && std::abs (n19.notes[79].hz - 880) < 1.0e-5, "non12 scale maps consecutive keys");
        const std::string map = "2\n60\n72\n60\n60\n432.123456789\n2\n0\nx\n";
        const auto mapped = sc::parse (equal,map);
        check (mapped.available == 7 && mapped.notes[59].status == sc::NoteStatus::outsideKeyboard
               && mapped.notes[61].status == sc::NoteStatus::unmapped
               && std::abs (mapped.notes[60].hz - 432.123456789) < 1.0e-8, "KBM range skips and exact reference applied");
        check (! sc::parse (equal,"2\n0\n127\n60\n61\n440\n2\n0\nx\n").valid(), "unmapped reference rejected");
        std::string crlf; for (char c : equal) crlf += c == '\n' ? "\r\n" : std::string (1,c);
        check (sc::parse (crlf).available == 128, "CRLF comments parse correctly");
        for (const auto& bad : { "Scale\n2junk\n100.0\n2/1\n", "Scale\n1\n-2/-1\n", "Scale\n1\nnan.0\n", "Scale\n1025\n", "Scale\n1\n0.0\n", "Scale\n1\n2/1/1\n" })
            check (! sc::parse (bad).valid(), "malformed or unsupported scale rejected");
        check (sc::parse ("\n1\n2/1\n").valid(), "empty scale description accepted");
        check (! sc::parse (std::string ("Bad\0scale",9)).valid(), "embedded NUL rejected");
        const auto latin = sc::parse (std::string ("Caf") + char (0xe9) + "\n1\n2/1\n");
        const auto latinDoc = sc::apply (latin,before,true);
        check (latinDoc && latinDoc->name == juce::String::fromUTF8 ("Café"), "Latin1 scale description preserved");
        check (! sc::parse (std::string (65537,'a')).valid(), "oversized Scala rejected");
        check (! sc::parse (equal,"0\n0\n127\n60\n69\n440junk\n0\n").valid(), "malformed KBM numeric rejected");
    }

    // Independent expected bytes copied from Novation's published examples.
    {
        namespace ti = mt::tun;
        namespace sc = mt::scala;
        // Author-generated fixtures: the current u-he exporter writes this numeric layout.
        std::string exported = "; AnaMark section\n[Exact Tuning]\n";
        for (int key = 0; key < 128; ++key) exported += "note " + std::to_string (key) + "=" + std::to_string (key*100.0) + "\n";
        const auto equal = ti::parse (exported,"File label");
        check (equal.valid() && equal.available == 128 && equal.name == "File label", "u-he numeric layout imports all128 notes");
        bool identity = true;
        for (int key = 0; key < 128; ++key) identity &= std::abs (equal.notes[key].pitch - key) < 1.0e-9;
        check (identity && std::abs (equal.notes[69].hz - 440) < 1.0e-9, "standard TUN reference has no octave offset");
        const auto changed = ti::parse (exported + "note 69=6950.0000\n");
        check (std::abs (changed.notes[69].pitch - 69.5) < 1.0e-9 && changed.notes[70].pitch == 70,
               "last numeric assignment wins without affecting neighbours");
        const auto period = ti::parse ("[Exact Tuning]\nnote 4=412.5\nnote 12=1200\n");
        check (period.available == 128 && std::abs (period.notes[4].pitch-4.125)<1.0e-9
               && std::abs (period.notes[16].pitch-16.125)<1.0e-9 && period.notes[127].pitch==127,
               "documented periodic Exact Tuning completion and defaults");
        const auto priority = ti::parse ("[Tuning]\nnote 69=7100\n" + exported);
        const auto reversed = ti::parse (exported + "[Tuning]\nnote 69=7100\n");
        check (priority.notes[69].pitch==69 && reversed.notes[69].pitch==69, "Exact Tuning priority independent of section order");
        check (ti::parse ("[Tuning]\nnote 69 = ignored compatibility text\n"+exported).available==128,
               "ignored lower-priority compatibility section is not interpreted");
        check (! ti::parse ("[Exact Tuning]\nnote 69 : 7000\n").valid(), "malformed known assignment rejected");
        const auto legacy = ti::parse ("[Tuning]\nNote 69 = +6950\n");
        check (legacy.available==128 && legacy.notes[69].pitch==69.5 && legacy.notes[60].pitch==60,
               "legacy integer cents and missing-note defaults");
        const auto base = ti::parse ("[EXACT TUNING]\nBaseFreq=16.3515978312874146672\n");
        check (base.available==116 && std::abs (base.notes[69].hz-880)<1.0e-8,
               "file BaseFreq changes actual pitches and flags out-of-range notes");
        check (ti::parse ("[Info]\nName=\"Named; table\"\n"+exported).name=="Named; table", "quoted semicolon kept in TUN name");
        std::string crlf; for (char c : exported) crlf += c=='\n' ? "\r\n" : std::string(1,c);
        check (ti::parse (crlf).available==128, "TUN CRLF input");
        auto nulLines=exported; for (auto& c : nulLines) if (c=='\n') c='\0';
        check (ti::parse (nulLines).available==128, "TUN NUL-delimited input");
        for (const auto& bad : { "[Functional Tuning]\nnote 0=\"formula\"\n", "[Mapping]\n", "[Scale Begin]\nFormatVersion=200\n",
                               "[Exact Tuning]\nnote 69=nan\n", "[Exact Tuning]\nBaseFreq=0\n", "[Exact Tuning]\nnote 0=50\n",
                               "[Tuning]\nnote 69=6900.5\n", "[Exact Tuning]\nnote 128=0\n", "[Exact Tuning]\nnote 1 extra=0\n",
                               "[Exact Tuning]\n[Exact Tuning]\n" })
            check (! ti::parse (bad).valid(), "unsupported or malformed TUN cannot silently fall back");
        check (! ti::parse (exported+"[Functional Tuning]\nnote 0=\"formula\"\n").valid()
               && ! ti::parse (exported+"[Mapping]\n").valid(), "higher-priority TUN data blocks numeric fallback");
        check (! ti::parse ("garbage").valid() && ! ti::parse (std::string(65537,'a')).valid(), "missing or oversized TUN rejected");
        const auto unknown = ti::parse ("[Unknown]\nnot = parsed ?!\n" + exported);
        check (unknown.available==128, "unknown metadata section ignored");
        mt::Document before; before.targetTable=6;
        check (! sc::apply(base,before,false), "TUN outside pitches require explicit keep policy");
        const auto applied=sc::apply(base,before,true);
        check (applied && applied->targetTable==6 && applied->table.pitch(127)==127
               && std::abs(*applied->table.pitch(69)-81)<1.0e-9, "TUN reuses atomic preview application and retains destination");
    }

    {
        namespace ti = mt::tun;
        namespace sc = mt::scala;
        const auto v2 = [] (const std::string& functions, const std::string& mapping = "", const std::string& compatibility = "")
        {
            return "[Scale Begin]\nFormat=\"AnaMark-TUN\"\nFormatVersion=200\n[Info]\nName=\"V2 verification\"\nID=\"V2_test\"\n"
                + compatibility + "[Functional Tuning]\n" + functions
                + (mapping.empty()?"":"[Mapping]\n"+mapping) + "[Scale End]\n";
        };
        const auto empty = ti::parse (v2(""));
        check (empty.available==128 && empty.name=="V2 verification", "TUN v2 implicit equal temperament");
        const auto example = ti::parse (v2("InitEqual=(0,8)\nnote 0=\"\"\nnote 1=\"*2 /3\"\nnote 2=\"#>-1 %1200 +-3\"\nnote 3=\"#=1\"\nnote 1=\"*3 /2\"\n"));
        const auto f1 = 8*std::exp2(1.0/12);
        check (example.valid() && std::abs(example.notes[1].hz-f1)<1e-10
               && std::abs(example.notes[2].hz-(f1*4/3-3))<1e-10 && std::abs(example.notes[3].hz-f1*2/3)<1e-10,
               "published functional example preserves sequential references");
        const auto arithmetic=ti::parse(v2("note 69=\"#220 *3 /2 %1200 +10\"\nnote 70=\"#=69\"\nnote 71=\"#>-1 +>-2\"\n"));
        check (arithmetic.valid() && arithmetic.notes[69].hz==670 && arithmetic.notes[70].hz==670
               && arithmetic.notes[71].hz==1340, "all equation tokens and reference operands");
        const auto duplicate=ti::parse(v2("note 69=\"#220 *2 *3 /2 /3 %1200 %0 +10 +20\"\n"));
        check (duplicate.valid() && duplicate.notes[69].hz==240, "duplicate formula tokens use last value");
        const auto loops=ti::parse(v2("note 3=\"*2 ~5\"\nnote 100=\"*3 ~-3\"\n"));
        check (loops.valid() && std::abs(loops.notes[7].hz/(440*std::exp2((7-69)/12.0))-2)<1e-12
               && std::abs(loops.notes[8].hz/(440*std::exp2((8-69)/12.0))-1)<1e-12
               && std::abs(loops.notes[98].hz/(440*std::exp2((98-69)/12.0))-3)<1e-12,
               "positive and negative formula loops have correct endpoints");
        const auto periodic=ti::parse(v2("note 9=\"%22\"\nnote 12=\"#>-12 *2 ~999\"\n"));
        check (periodic.available==128 && std::abs(periodic.notes[69].hz-440*std::exp2(22.0/1200))<1e-9
               && std::abs(periodic.notes[68].pitch-68)<1e-9, "published periodic formula expands to last key");
        const auto reverse=ti::parse(v2("note 127=\"#440 ~-999\"\n"));
        check (reverse.available==128 && reverse.notes[0].hz==440 && reverse.notes[127].hz==440, "open backward loop is bounded and inclusive");
        const auto ensured=ti::parse(v2("InitEqual=(69,432)\nnote 69=\"!440\"\n"));
        check (ensured.available==128 && std::abs(ensured.notes[69].hz-440)<1e-9 && std::abs(ensured.notes[60].pitch-60)<1e-9,
               "ensure-frequency shifts whole scale");
        const auto reset=ti::parse(v2("note 69=\"*3\"\nInitEqual=(69,432)\n"));
        check (reset.valid() && reset.notes[69].hz==432, "InitEqual resets earlier functional edits");
        const auto priority=ti::parse(v2("", "", "[Exact Tuning]\nnote 69=not numeric compatibility data\n"));
        check (priority.available==128 && std::abs(priority.notes[69].hz-440)<1e-9, "authoritative v2 ignores numeric compatibility data");
        const auto mapped=ti::parse(v2("", "Keyboard 60=69\nKeyboard 127=200\nKeyboard 0=-1\n"));
        check (mapped.valid() && mapped.notes[60].hz==440 && mapped.notes[127].status==sc::NoteStatus::unmapped
               && mapped.notes[0].status==sc::NoteStatus::unmapped, "mapping is applied without boundary clamping");
        const auto mapLoop=ti::parse(v2("", "LoopSize=12\nKeyboard 0=7\n"));
        check (mapLoop.valid() && std::abs(mapLoop.notes[60].pitch-67)<1e-9, "keyboard mapping loop uses scale degree not input frequency");
        const auto muted=ti::parse(v2("note 69=\"#0\"\nnote 70=\"#-1\"\n"));
        check (muted.available==126 && muted.notes[69].status==sc::NoteStatus::muted && muted.notes[70].status==sc::NoteStatus::muted,
               "nonpositive frequencies remain explicitly muted");
        mt::Document before; before.table.setCents(69,20);
        check (!sc::apply(muted,before,false), "muted v2 keys require explicit keep policy");
        const auto kept=sc::apply(muted,before,true);
        check (kept && kept->table.pitch(69)==before.table.pitch(69), "keep muted key retains audible current pitch");
        for(const auto& bad : { "note 69=\"/0\"\n", "note 69=\"!0\"\n", "note 69=\"*2 !440\"\n", "note 0=\"#>-1\"\n",
                               "note 127=\"#>1\"\n", "note 69=\"#=128\"\n", "note 69=\"#=-1\"\n", "note 128=\"#440\"\n",
                               "note 69=\"#nan\"\n", "note 69=\"#inf\"\n", "note 69=\"/1e-300 *1e9\"\n", "note 69=\"*\"\n",
                               "note 69=\"~99999999999999999999\"\n", "note 69=\"#1.0oops\"\n", "InitEqual=(69,0)\n", "InitEqual=(69junk,440)\n" })
            check (!ti::parse(v2(bad)).valid(), "invalid formula/reference/math rejects entire v2 import");
        check (!ti::parse("[Functional Tuning]\nnote 69=\"#440\"\n").valid(), "v2 needs its framing and declared version");
        auto unclosed=v2("");unclosed.erase(unclosed.find("[Scale End]"));
        check (!ti::parse(unclosed).valid(), "missing Scale End rejected");
        check (!ti::parse(v2("")+v2("")).valid(), "multiple scale datasets rejected");
        const auto escaped=ti::parse(v2("note 69=\"\\0x23440\"\n"));
        check (escaped.valid() && escaped.notes[69].hz==440, "TUN hexadecimal escape decoded before formula evaluation");
        const auto engineEscaped=ti::parse(v2("note 69=\"\\x023440\"\n"));
        check (engineEscaped.valid() && engineEscaped.notes[69].hz==440, "AnaMark writer hexadecimal escape spelling supported");
    }

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

    {
        mt::Table bulk;
        const std::array<int, 2> keys { 60, 61 };
        check (mt::editNotes (bulk, keys, mt::BulkEdit::setOffset, 25)
               && bulk.pitch (60) == 60.25 && bulk.pitch (61) == 61.25 && bulk.pitch (69) == 69,
               "bulk absolute offsets affect selected notes only");
        check (mt::editNotes (bulk, keys, mt::BulkEdit::addCents, -50)
               && bulk.pitch (60) == 59.75 && bulk.pitch (61) == 60.75, "bulk additive cents retain existing offsets");
        const std::array<int, 3> boundary { 60, 0, 61 };
        check (! mt::editNotes (bulk, boundary, mt::BulkEdit::setOffset, -50)
               && bulk.pitch (60) == 59.75 && bulk.pitch (61) == 60.75, "boundary failure rolls back entire operation");
        check (! mt::editNotes (bulk, {}, mt::BulkEdit::reset)
               && ! mt::editNotes (bulk, keys, mt::BulkEdit::addCents, std::numeric_limits<double>::quiet_NaN()),
               "empty selection and nonfinite bulk value rejected");
        const std::array<int, 2> invalid { 60, 128 };
        check (! mt::editNotes (bulk, invalid, mt::BulkEdit::reset) && bulk.pitch (60) == 59.75,
               "invalid key rolls back earlier valid edit");
        check (mt::editNotes (bulk, keys, mt::BulkEdit::reset) && bulk.pitch (60) == 60 && bulk.pitch (61) == 61,
               "reset selected notes to equal temperament");
    }

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
