# TUN v2 formulas and mapping

19 October 2026. TUN v2 imports through **MicroTune → Import Tuning...** alongside
numeric TUN and Scala. Imports remain local, previewed and undoable. JSON save/load
and DAW working-table persistence remain unchanged. No hardware tuning send/save
or MIDI-note muting was added.

## Use and meaning

Choose a single `.tun` dataset and press Preview. The v2 file supplies its own
reference, ordered formulas and keyboard mapping; manual Scala reference fields
and KBM are disabled. The result lists all 128 MIDI keys after mapping, with
calculated Hz, changes in cents and unavailable reasons.

TUN v2 formulas govern the result even when numeric `[Tuning]` or `[Exact Tuning]`
compatibility sections are present. The adapter never imports those as a fallback
when a v2 formula is malformed. Formula evaluation happens in a temporary table;
errors or cancelling the preview leave the working document unchanged.

Keys can be unavailable because their frequency is outside our current application
range, because a mapping refers outside the 128 scale notes, or because the file
mutes them with a nonpositive frequency. The preview distinguishes muted keys.
Mapping indices are not silently clamped to an endpoint. Apply requires an explicit
**Keep existing pitches for unavailable notes** choice for an incomplete result,
and at least one available note. Keeping a muted/unmapped key preserves its current
pitch; it does not implement muting. Our complete table and version-1 JSON have
pitch assignments, not a per-key mute state.

Apply replaces the table and name once, preserving the destination slot. Undo/Redo
handles the complete import. Save Table writes the flattened pitches to native
JSON. Saving the DAW Set retains those pitches. Original formulas, raw metadata,
channel assignments and source mapping definitions are not embedded in JSON;
this is not lossless TUN-file editing or TUN export.

## Supported v2 operations

The adapter evaluates `InitEqual` and the documented formula operations: range Hz,
multiplication, division, cents, Hz shift, absolute/relative note references,
forward/backward loops and ensure-frequency whole-scale retuning. Repeated formula
tokens take their last value; repeated note assignments execute in file order.
Keyboard entries and LoopSize map MIDI keys to the calculated scale notes.
These semantics come from sections 3.7–3.8 of the
[author's specification](https://www.mark-henning.de/files/am/Tuning_File_V2_Doc.pdf).

The import requires one dataset with Scale Begin, declared FormatVersion 200,
Info, Functional Tuning and Scale End. Unknown metadata and numeric compatibility
content are ignored. Multiple datasets and additional sections after Scale End
are rejected; this is not an MSF/channel-routing browser. Ordinary single-scale
Assignment metadata does not change our MIDI routing. Name is read from Info or
falls back to the filename. C-style string escaping, the specification's hex
spelling and AnaMark writer hex spelling are decoded before formula validation.

## Application limits

- 64 KiB per input file and at most 4096 records across the processed sections.
- 128 scale notes and 128 MIDI keys. InitEqual reference note is 0–127; reference
  Hz is 0.000001–1000000000. Ensure-frequency must be positive and the only token
  in its formula.
- Finite scalar formula parameters up to magnitude 1e9, cents up to magnitude
  120000, absolute reference indices 0–127 and relative offsets −127–127.
  Loop counts are bounded to magnitude 1e6; actual execution stops at the scale's
  ends, so each formula visits at most 128 notes. Keyboard mapping values and
  LoopSize are also bounded to magnitude 1e6.
- Out-of-scale formula references, malformed syntax, division by zero and
  non-finite results reject the complete import. Finite zero/negative frequencies
  are instead retained as explicit muted-note preview states.
- The working table still restricts absolute pitches to MIDI coordinates 0–127.
  No clipping or automatic octave shifting is used. Invalid mapped indices are
  unavailable instead of using the upstream map getter's endpoint clamp.

These are deliberate import bounds, not universal TUN-v2 conformance claims.
There is no arbitrary expression language, recursion, host callback or script
execution. Formula parsing/evaluation runs through the local UI workflow, never
through the audio processing callback, and opens no files or MIDI devices.

## Dependency and verification

The formula engine is the MIT-licensed AnaMark CFormula, pinned to
`9d6160141f53235807ac65381ab3839a35ab4856`. Only the formula engine and string helpers
are vendored. The upstream file reader/writer and MSF implementation are not used.
The adapter validates structure, numeric parameters and results itself. A documented
portability patch replaces the string helpers' locale-dependent strtod/strtol
with from_chars; the formula algorithm is unchanged. The MIT notice ships alongside
the existing Surge notice inside every macOS product bundle.
[Upstream source](https://github.com/zardini123/AnaMark-Tuning-Library/tree/9d6160141f53235807ac65381ab3839a35ab4856),
[vendored changes](../Source/ThirdParty/anamark/README.bsii.md).

Offline tests include the published sequential-reference and periodic examples,
all equation tokens, duplicate tokens, loop directions/endpoints, ensure-frequency,
InitEqual resets, mapping loops, muted/unmapped notes, precedence, hexadecimal
escapes and malformed/non-finite/out-of-bounds input. Native checks exercise actual
preview/apply, explicit preservation, one-step undo/redo, JSON and DAW-state
restoration, invalid-formula rollback and zero MIDI output. Hardware tuning-transfer
and acoustic accuracy remain separate acceptance work.

## Handoff verification — 20 October 2026

A fresh `./tools/validate.sh` passed the standalone build and all nine offline
CTest groups (3.39 seconds). Local report:
`validation-results/20261020-030552-19210/summary.txt`. AU/VST3 builds and both native test programs then passed. Native editor tests
covered preview/apply, explicit preservation, undo/redo, JSON and host-state
restoration with zero MIDI output; the generated preview was visually inspected.
The processor automation/routing regression also passed. Logs and screenshot:
`validation-results/tun-v2-final/` (local ignored evidence).

With Ableton confirmed closed, both plugins were installed under the user's
`Library/Audio/Plug-Ins` after backing up the previous bundles to that report's
`installed-before/` directory. Both third-party MIT notices matched their source
licenses, strict local ad-hoc signature verification passed, and
`auval -v aumf Bs2C Gutz` succeeded. This is not Developer ID/notarization or fresh
Ableton/hardware acceptance. See [the handoff](START-HERE.md) for next steps.
Public release assets were not replaced.
