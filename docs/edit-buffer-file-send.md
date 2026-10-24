# Validated edit-buffer file sending

25 October 2026. Send .syx → BSII accepts only the previously validated temporary
edit-buffer format. It is not a bank importer, arbitrary SysEx player or stored-slot
writer. A single message exported from a stored bank can therefore be rejected even
when its 154-byte parameter payload resembles a patch.

## Behavior

The actual file chooser delegates to `sendEditBufferFile`. This requires an open
MIDI output, opens the selected file and reads at most 155 bytes. The existing
`bsii::isRecallPatch` gate requires exactly 154 bytes, the Novation/BSII header,
7-bit interior bytes, zero command/address bytes at offsets 6–12, the terminator
and successful patch decoding. A trailing byte, concatenated messages, a bank
command or an unknown command fails the gate. No guessed conversion of bank
headers or slot numbering is performed.

Rejected/unreadable files send no MIDI and retain the known preset identity.
An accepted file sends its original bytes once and clears the known preset,
because the active sound is now a temporary patch. Status reports dispatch, not
hardware acknowledgement. The plugin's 95 parameter indices and patch recall,
local tuning import and normal automation paths are unchanged.

## Validation scope

Native checks exercise this selected-file path through a virtual MIDI output:
stored/bank-shaped messages, a synthetic 128-patch bank, command/address changes,
high-bit and nested status bytes, a wrong header, truncated/trailing/concatenated
input, a tuning-save command, empty/missing files and a missing output. Invalid
files must emit no MIDI and preserve preset identity. A retained valid temporary
patch must emit its exact original bytes once. Private user banks are not added
as test fixtures. Results and installation evidence are recorded below when run.

This restriction does not implement bank restoration. The separate
[bank research](patch-bank-research.md) records the manual/Components exports and
remaining addressing/write validation. It also does not prevent another MIDI
application or host echo route from sending messages directly to the hardware.

## Completed verification and installation — 25 October 2026

AU/VST3 and BSIIMicroTuneEditorTest builds passed. The native test passed with
zero failures, including the real selected-file path through virtual MIDI,
malformed/bank rejection with zero output, preserved preset identity on rejection,
and exactly one byte-identical output for a valid edit-buffer patch. Existing
MicroTune import, undo/redo, JSON/host-state and silent-edit checks also passed.
Local ignored evidence: `validation-results/edit-buffer-gate/build.log` and
`native-test.log`. No physical synth transfer was used for this verification.

Ableton was confirmed closed before installing both plugin bundles. Previous
bundles are in `validation-results/edit-buffer-gate/installed-before/`. Both MIT
licence notices matched, strict local ad-hoc signature checks passed, and Apple
AU validation succeeded (`auval.log`). This also installs the previously pending
recall dispatch wording change. Fresh manual Ableton acceptance and bank
restoration remain separate work; public release downloads were not replaced.
The full offline-suite fingerprint is older; only these relevant checks were run.
