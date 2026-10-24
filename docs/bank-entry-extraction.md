# Extract a bank entry as a temporary patch

25 October 2026. This offline tool creates a new .syx file; it does not open MIDI
ports, send a patch, overwrite a stored slot or save a bank.

```sh
python3 -B tools/midi-lab/lab.py extract-bank-entry bank.syx 11 entry-11.syx
python3 -B tools/midi-lab/lab.py extract-bank-entry bank.syx 11 entry-11.syx --json
```

The source must pass the strict complete observed export profile used by
[bank review](bank-restoration-plan.md). Select export index 0–127. Lookup uses
that index, not physical message order. The seven command/address bytes 6–12 are
set to zero to produce the known temporary edit-buffer header; on this profile
only bytes 7/8 can change. Prefix, terminator and every payload byte are retained.
No unknown parameter bytes are regenerated or dropped.

Output uses exclusive creation: existing files, the source path and existing
symlink aliases are refused. A malformed bank or invalid index produces no output.
Parent folders must already exist. Keep the source bank and conversion report;
the destination is not a complete backup or a stored-slot write command.

JSON reports source/output paths and SHA256 hashes, selected export index, the
154-byte output length, midiSent=false and storedWrite=false. Creating a file does
not establish hardware receipt. Only deliberately sending it through the validated
temporary-patch workflow changes the active sound. Preserve that active sound
first; no automatic Set/transport/reconnect transfer was added.

## Relevant validation

The small BSIIPatchInspect utility now reports editBufferEligible by calling the
same `bsii::isRecallPatch` validator as the installed sender. This distinguishes
field-decodable bank messages from files eligible for temporary-patch sending.
It describes format eligibility, not user authorization or hardware acceptance.

Focused MidiLabTools and ResearchTools CTest groups passed (4.11 s). Tests cover
all 128 indices, reordered messages, byte-exact payload/header conversion,
distinct payloads, invalid index types/ranges, incomplete banks, non-overwrite and
source-alias protection, CLI errors and actual production-gate results. Original
bank-shaped messages fail that gate; converted messages pass it. Plugin runtime
code did not change, so no AU/VST3 rebuild/install was repeated.

Private real-data check: exported indices 0, 1, 11 and 127 were extracted locally;
all four matched the corresponding saved hardware edit-buffer readbacks exactly
and passed the production gate. Source bank hash was unchanged. Evidence is in
`validation-results/bank-extraction-check/summary.json` and the adjacent files,
ignored by Git. No private user patch data was committed.

## Temporary hardware round trip

Following the user's instruction to proceed, the current temporary sound was
freshly backed up and checked as restorable. Extracted entry 11 was sent only to
the edit buffer. The transport verified a complete readback, then a separate
readback file was compared byte-for-byte with the target. The fresh starting
backup was then sent back and independently read again; all bytes matched.

Both targetVerified and restorationVerified are true. Evidence:
`midi-lab-results/20261025-012517-bank-entry-temporary-roundtrip/summary.json`,
with before.syx, target-readback.syx, restored-readback.syx and transport logs.
No stored-slot, bank-save, global Tune or tuning-table data commands were sent.
This establishes one temporary-transfer/restoration scenario without audio; it
is not stored-bank restoration, acoustic verification or exhaustive firmware
coverage. Restoring a temporary sound does not guarantee the displayed hardware
preset number or plugin preset identity is restored.

Stored-memory restoration remains governed by the separate
[restoration plan](bank-restoration-plan.md), including fresh backups and explicit
authorization for a concrete disposable destination before any stored write.
