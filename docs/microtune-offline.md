# Offline MicroTune table editor

Implemented 19 October 2026. This is a local file tool and reusable table/codec
module. It does not open MIDI ports, change the connected synth, save hardware
tuning tables, or add controls to the plugin window. Hardware transfer and pitch
verification are pending. Existing 95 host parameters and their indices are
unchanged; the installed AU/VST3 and public preview are unchanged.

## What it can do

- Create a complete 128-note equal-tempered working table.
- Set any key to a signed cents offset from that key's equal-tempered pitch.
- Store the work as a version-1 BSII MicroTune JSON document.
- Inspect the target table and count of changed notes.
- Export real-time note-change SysEx, in configurable batches of 1–127 entries.
  Default: 16 entries/message, eight messages for a complete table.

**A new document starts from equal temperament, not the synth's current table.**
Export contains all 128 assignments, including unchanged notes. If subsequently
transmitted with another MIDI utility, it can replace the target table's complete
working contents. No persistent-save command is included. The codec has no
hardware table-readback, Scala importer, or automatic DAW-session recall feature.

Target IDs are 0–8; the default is 1. Novation documents that table 0 is reset at
startup and only 1–8 can be saved. The number in the document is a destination,
not evidence that those contents exist on the synth. See the
[protocol research and later hardware plan](microtune-transfer-research.md).

## Build and use

Run `./tools/validate.sh` from the repository. This builds the tool and runs the
offline checks. On the existing Xcode/Release build, the executable is
`build/Release/BSIITuningTableTool`. Commands below run from the repo root;
outputs must use new paths in existing directories. Existing files are refused.

```sh
build/Release/BSIITuningTableTool new equal.json 1
build/Release/BSIITuningTableTool set equal.json a4-sharp.json 69 50
build/Release/BSIITuningTableTool set a4-sharp.json edited.json 60 -50
build/Release/BSIITuningTableTool inspect edited.json
build/Release/BSIITuningTableTool export edited.json table-1.syx
```

This example assigns key 69 to A4 +50 cents and key 60 to C4 -50 cents; every
other key remains equal-tempered. `set` assigns an offset from the key itself,
not an additional offset from its previous edit. Running `set` twice with +50
therefore remains +50. An optional final export argument changes the batch size.
Table contents may also be edited in JSON, but the parser requires all 128 notes.

The JSON document contains `format` (BSII MicroTune), `version` (1), `name`,
`targetTable` and `pitches` (128 absolute MIDI-note coordinates in key order).
For example, 69.5 means A4 +50 cents, independently of which key is assigned that
pitch. It is a separate format from `.bsiistate` or 154-byte patch SysEx.
The CLI limits input documents to 64 KiB. The library rejects malformed required
fields, unsupported versions, wrong types, missing notes and non-finite/range errors.

## Precision and bounds

The first implementation deliberately accepts absolute pitch coordinates 0–127
inclusive. That is an application restriction, not a claim about the full MIDI
Tuning Standard or every BSII-supported frequency. Thus key 0 cannot be lowered
and key 127 cannot be raised in this version. Invalid edits are rejected, never
clamped, and leave previous documents unchanged. Reserved top-of-range MTS values
and wider frequency ranges remain deferred until the full specification is checked.

Encoding rounds to the nearest 1/16384 semitone with proper carry to the next
semitone. Negative cents use a lower base semitone and a positive fraction.
This is wire precision; Novation describes a coarser hardware accuracy limit.
The default batching is an application choice, not a measured hardware buffer
limit or pacing policy. Exporting a file is not a hardware send or acknowledgement.

## Validation

`validation-results/20261019-113656-33869/summary.txt`: standalone/tool/test build
and all **eight** CTest groups passed. New MicroTuningOffline checks include
Novation's fixed examples, negative offsets, semitone carry, broad packing/error
bounds, atomic invalid edits, all 128 keys across batch sizes 1/16/127, duplicate
keys and malformed local documents. No MIDI was sent.

The initial run's two malformed-array fixture failures were corrected in the
test setup: JUCE property replacement used array equality that could coerce element
types and skip the replacement. The fixture builder now removes the old property
before inserting malformed data. The parser was independently checked against
actual JSON files and did not require a runtime fix for those two failures.

End-to-end CLI verification is retained in
`validation-results/microtune-cli-3td6ufzy/summary.json`: create, edit, inspect and
export passed; all emitted entries/framing were inspected; existing files were
preserved; invalid edits/batches and boolean note data were rejected. Reports and
sample files are local ignored artifacts. No AU/VST3 install or hardware test was
performed for this offline increment.

## Next integration work

Add an editor surface and explicit transmit workflow after agreeing how to
preserve existing custom tables. Persistent save must remain a separate action.
Keep ordinary patch snapshots/Send All and captured-sound recall unchanged until
a complete tuning-document restore workflow is designed and hardware-tested.
