# Offline research toolkit

Use these commands from the repository root to reduce repeated investigation and
keep evidence in local files. These offline commands do not open a MIDI device.

## Compact status

```sh
python3 tools/midi-lab/lab.py status
python3 tools/midi-lab/lab.py status --json
```

Reports the commit, working-tree state, control coverage and latest stamped local
validation. Run `./tools/validate.sh` to produce a stamped report. Its fingerprint
covers CMakeLists.txt and files under Source/, tests/ and tools/ (excluding Python
cache files). Documentation-only edits and commits do not invalidate those inputs.
Changes during or after validation, or a different platform, make the result stale.
Incomplete, malformed and legacy unstamped reports cannot establish a current pass.

A pass covers the standalone build and offline CTest suite only. It does not certify
hardware, sound, signing, native GUI tests or AU/VST3 host acceptance. Dependencies,
compiler versions and external build settings are not fully fingerprinted; this is
a local workflow aid, not a reproducible-build attestation.

## Analyse existing captures

```sh
python3 tools/midi-lab/lab.py analyse-sweep tests/glide-divergence-fixtures
```

Requires a current BSIIPatchInspect build. Writes report.md and report.json into a
new ignored midi-lab-results/ directory. Original captures remain unchanged. The
included Glide Divergence experiment covers values 0–15 on recorded firmware 4.14.

Place experiment.json beside the raw captures. Example:

```json
{
  "schemaVersion": 1,
  "id": "example",
  "stateDomain": "edit-buffer",
  "firmware": "record the actual firmware",
  "baseline": "before.syx",
  "restored": "after.syx",
  "comparisonField": "existingDecoderField",
  "expectedRange": [0, 1],
  "samples": [
    {"file": "value-0.syx", "value": 0},
    {"file": "value-1.syx", "value": 1}
  ]
}
```

Only framed 154-byte edit-buffer patches and unsigned input values are supported.
An optional sample `expected` integer supplies a different expected decoder value.
Paths must stay inside the experiment directory. Globals, tuning tables and sequence
state are excluded because restoring a normal patch cannot recover those domains.

The analyser proposes direct, split or inverted bit placement. With eight or more
samples, every fourth sample is held out from candidate selection. It compares the
result with the actual production decoder, reports other decoded changes, and checks
for unexplained baseline-relative bit changes even when the decoder ignores them.
Ambiguous candidates require review. Offsets, clamping and interactions are not inferred.

CAPTURED_MAPPING_MATCH requires a unique candidate matching all samples, matching
production decoding, complete declared range coverage, no unexplained changes and
byte-identical saved before/after files. This is evidence about those captures only;
it is not proof of causality or compatibility with other firmware. Saved restoration
files do not prove the synth was restored, and the analyser performs no restoration.
No production mapping or source code is generated automatically.

## Inspect a binary SysEx stream

```sh
python3 -B tools/midi-lab/lab.py inspect-sysex path/to/capture.syx
python3 -B tools/midi-lab/lab.py inspect-sysex path/to/capture.syx --json
```

Reads an existing binary file only. No MIDI transport, decoder build, subprocess,
report directory, source-file modification or hardware send is involved. Output is
printed to stdout. Exit 0 means clean framing only; exit 1 reports malformed input
or an inspection error. Unknown but well-framed messages remain accepted as unknown.
Standard MIDI files (MThd) are rejected; this is not a .mid parser or hex-text parser.

JSON retains the source SHA256 and every original byte in ordered segments with
zero-based offsets, lengths, hashes and rawHex. Concatenating all segment rawHex
values reconstructs the original file exactly, including outside data and malformed
fragments. Complete SysEx messages have a separate normalizedHex/hash with interleaved
real-time bytes removed for classification/duplicate comparison. The original wire
bytes are never normalized away. Duplicate references identify earlier segment
indices, not hardware slot numbers. Real-time-only outside segments are allowed;
other outside data, interrupted/nested starts and unterminated messages flag issues.
No channel-message or real-time semantic validation is claimed.

The existing 154-byte Novation/BSII prefix plus zero command/address bytes 6–12
is labelled **bsii-edit-buffer-shape-candidate**. No field decoding or provenance
verification happens. Other BSII commands and other manufacturers are retained
as unknown. Every report has verifiedBank=false and transmitEligible=false,
including a clean 128-message stream. No bank slot addressing is inferred.

Limits: 4 MiB input, bounded during the read, and 4096 total segments. Errors beyond
those bounds abort inspection without a partial JSON report. Keep the original
capture; inspection cannot prove that a live dump finished or contains all stored
patches. This is the first offline stage of the [bank plan](patch-bank-research.md).

Verification (20 October 2026): focused CTest MidiLabTools and ResearchTools both
passed (2.29 s). New cases cover a retained hardware single patch, a synthetic
128-message duplicate stream, unknown commands, real-time interleaving, byte-exact
reconstruction, malformed resynchronization, limits, and CLI file preservation.
A direct CLI check on tests/state-fixtures/baseline.syx reported one clean
154-byte shape candidate. No full bank was captured or restored; no MIDI sent.
No plugin build/install or broader validation was repeated for this tool-only change.

## Review a BSII bank export

`python3 -B tools/midi-lab/lab.py review-bank FILE [--against BACKUP] [--json]`
validates the observed complete export profile and compares entries by export
index. It opens no MIDI ports and creates no output files. Preview does not
establish write addresses or enable transmission. See
[bank review, restoration design and tests](bank-restoration-plan.md).

## Extract one bank entry without MIDI

`python3 -B tools/midi-lab/lab.py extract-bank-entry BANK INDEX NEW_FILE.syx [--json]`
creates one temporary-patch file from the strict export profile. Source bytes are
preserved, existing output files are refused, and no MIDI is sent.
See [extraction limits and hardware round-trip evidence](bank-entry-extraction.md).

## Deferred work

Generic live experiment recipes and transport consolidation can follow after this
increment proves useful. Richer tuning editing (undo, bulk operations and scale-file
import) and hardware table transfer remain separate work; this toolkit adds none of
those features.
