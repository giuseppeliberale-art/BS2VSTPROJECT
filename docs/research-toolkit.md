# Offline research toolkit

Use these commands from the repository root to reduce repeated investigation and
keep evidence in local files. Neither command opens a MIDI device.

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

## Deferred work

Generic live experiment recipes and transport consolidation can follow after this
increment proves useful. Richer tuning editing (undo, bulk operations and scale-file
import) and hardware table transfer remain separate work; this toolkit adds none of
those features.
