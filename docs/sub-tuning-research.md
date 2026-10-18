# Independent Sub Oscillator tuning — discovery

18 October 2026, user firmware 4.14. Hardware Osc Select set to 1 for consistency;
no selector-independence claim has been tested. Shortcut from cached official
manual: hold Function/Exit while turning oscillator Coarse/Fine knobs.

## Fine — emitted MIDI captured

User held Function while sweeping Fine left→right and confirmed display -100/+100.
Capture: `midi-lab-results/20261018-164252-record-3f6d38/events.jsonl`.

Messages select NRPN 0:77, then send Data Entry MSB CC6 and LSB CC38. This is NOT
an NRPN7-only value: the captured LSB is significant. Reconstructed effective
8-bit raw value is `(CC6 << 1) | (CC38 >> 6)`. 304 complete samples included
201 unique raw values, minimum 27 and maximum 228; the sweep did not capture
every intermediate value. CC6 alone spans 13–114 and is not the tuning value.

The displayed signed conversion, center encoding, remote receive/clamping and
SysEx field still need verification. Do not use the generic NRPN7 sampler for
this parameter. No Sub tuning controls have been implemented in the plugin yet.

### Fine hardware-labelled SysEx endpoints

At displayed +100, dump `midi-lab-results/20261018-164415-backup-58a518/baseline.syx`
has zero-based byte 34=57, byte 35=0. At displayed -100, dump
`20261018-164513-backup-cd6b2f/baseline.syx` has byte 34=6, byte 35=96.
Only those two bytes changed. Candidate effective raw field:
`((b[34] & 0x3F) << 2) | ((b[35] & 0x60) >> 5)`, giving 228 and 27.
User mentioned a brief reverse movement; final patch still matched raw 27 exactly.
Full remote sweep and center/display conversion checks remain pending.

## Coarse — emitted MIDI captured

User held Function while sweeping the oscillator Coarse knob fully left→right,
then explicitly confirmed displayed endpoints -12 and +12 semitones. Capture:
`midi-lab-results/20261018-164604-record-5e5674/events.jsonl`.

Route is NRPN 0:84 with Data Entry MSB CC6 and LSB CC38; effective raw value
`(CC6 << 1) | (CC38 >> 6)`. 316 complete samples covered 250 unique values,
including raw 0 and 255. CC6 alone spans 0–127 and is not the complete value.
Remote reception, SysEx storage, center/display conversion and plugin integration
remain unverified. Do not use the existing NRPN7 path for this control.

### Coarse hardware-labelled SysEx endpoints

At displayed +12, dump `midi-lab-results/20261018-164728-backup-f148fe/baseline.syx`
has zero-based byte 33=127, byte 34=70. At displayed -12, dump
`20261018-164812-backup-6edd5f/baseline.syx` has byte 33=0, byte 34=6.
Only those two bytes changed. Candidate raw field:
`((b[33] & 0x7F) << 1) | ((b[34] & 0x40) >> 6)`, giving 255 and 0.
It shares byte 34 with Fine; preserve the other six bits when testing Coarse.

## Full captured-range remote/SysEx verification

Automatic `sub-tuning` group in `tools/LFOSyncSampler.swift` sent paired NRPN
Data Entry (CC6=raw>>1, CC38=(raw&1)<<6). All 202 Fine settings 27–228 and all
256 Coarse settings 0–255 matched the candidate SysEx fields exactly. Every
sample checked for changed bits outside its field; none changed. The full
starting patch was restored and verified byte for byte. No preset was saved.

Run: `midi-lab-results/sub-tuning-discovery-20261018/`; retained evidence:
`tests/sub-tuning-fixtures/` with 458 setting dumps, before/after and samples.csv.
These tests verify only the captured Fine range: behavior outside raw 27–228
was not tested, so do not claim remote clamping at those boundaries.

The raw MIDI routes and SysEx fields are now verified across those ranges.
Hardware-displayed endpoints are user-confirmed; exact center/display conversion,
Osc Select independence, audible behavior and plugin integration remain pending.
Both Sub parameters remain absent from the installed plugin.

### Hardware zero observation — Fine

User held Function/Exit while setting Fine until the hardware displayed 0.
Dump `midi-lab-results/20261018-165117-backup-33938a/baseline.syx` stores
byte 34=31, byte 35=96: effective Sub Fine raw 127. Compared with the sweep's
restored patch, only byte 34 changed. This confirms displayed zero at raw 127;
it does not independently establish whether raw 128 also displays zero.

### Hardware zero observation — Coarse

User held Function/Exit while setting Coarse until the hardware displayed 0.
Dump `midi-lab-results/20261018-165204-backup-112c06/baseline.syx` stores
byte 33=64, byte 34=31, byte 35=96: Sub Coarse raw 128, with Sub Fine still 127.
Only byte 33 changed from the Fine-zero capture. Thus one displayed zero is
confirmed for each control; adjacent raw values' display rounding is not yet
established. The synth was left with both Sub tuning settings displaying zero.
Retained labelled copies: `tests/sub-tuning-fixtures/hardware-fine-zero.syx`
and `hardware-coarse-zero.syx`.

## Plugin implementation

Sub Fine and Sub Coarse are added to Osc 1 as independent numeric controls.
Sub Fine uses captured range 27–228, default 127 (hardware-observed zero).
Sub Coarse uses 0–255, default 128 (hardware-observed zero). These follow all
91 published parameters at host indices 91 and 92, preserving existing IDs
and indices. A new paired NRPN protocol sends CC99/98 selection, CC6 high
bits and CC38 low bit. Reception waits for the LSB, tracks data per channel
and discards cached high bits on selection changes, RPN selection and pause.
Get Patch/import decoding uses the verified masks and the existing silent
parameter-refresh path. No preset-save command is added.

The tuning display reuses the oscillator signed presentation model as an
approximation: endpoint and zero observations are confirmed, but intermediate
LCD rounding and Sub Fine raw 128 displaying zero are not independently
verified. The tooltip states this limitation. Typing Fine zero and resetting
the knob select the captured raw 127; no assumption about display precision
is used in raw storage or MIDI transmission. Hosted/audible checks pending.

### Local build and integration verification

Report: `validation-results/20261018-215141-24495/`. All seven offline test
groups passed, including all 458 captured Sub tuning values, unchanged main
oscillator fields, and MIDI channel/selection isolation. Native processor
integration passed paired-NRPN output, deferred publication until CC38,
independent hardware updates without echo, saved state, published indices
and patch refresh. Direct SysEx integration passed. Standalone/AU/VST3
builds passed. Live was confirmed closed; installed AU/VST3 were backed up,
replaced and locally signed. Strict signature verification and Apple AU
validation passed. Manual hosted/audible checks remain pending. Public
preview release assets have not been replaced.

## Manual Ableton acceptance passed

User verified the correctly labelled Sub Fine and Sub Coarse controls:

- Sub Fine plugin +100 produced raw 228 in hardware patch
  `midi-lab-results/20261018-220601-backup-fdfb66/baseline.syx`.
  Compared with immediate baseline
  `20261018-220523-backup-547e16/baseline.syx`, only Sub Fine field bits
  changed (byte 34: 32 to 57). Hardware Function + Fine fully left followed
  in the plugin after resuming Live RX. With RX paused, moving hardware to
  +100 left the plugin at -100; Get Patch updated it to +100.
- Sub Coarse plugin +12.0 produced raw 255; readback
  `20261018-221115-backup-78e386/baseline.syx` also confirmed Sub Fine
  raw 127. Hardware Function + Coarse fully left updated plugin to -12.0.
  With RX paused, moving hardware fully right left plugin at -12.0;
  Get Patch updated it to +12.0.
- User set Sub Coarse back to 0.0 and resumed Live RX. Final readback
  `20261018-221348-backup-8881ca/baseline.syx` confirms Sub Fine raw 127
  and Sub Coarse raw 128, both captured zero settings.

### Resolved test confusion; no runtime fix needed

Initial reports of Sub Fine +100 and mouse-drag +91 not reaching hardware
were traced to the user adjusting the LFO1 -> Pitch knob above Sub Fine.
Readback `20261018-220149-backup-9a7004/baseline.syx` identified Osc 1
LFO1 Pitch raw 100 -> 91 while Sub Fine remained raw 128; user explicitly
confirmed the control mix-up after finding the lower Sub tuning row.

A temporary narrowly scoped sender replayed expected Sub Fine raw 228
with zero inter-message delay; hardware accepted it with only Sub Fine bits
changed. Restoring raw 128 reproduced the starting full patch exactly.
Thus that probe did not support the spacing hypothesis. The temporary
sender was removed; no runtime changes were made during diagnosis.
A later hardware-follow attempt was made with Live RX paused; it followed
after RX resumed. Neither episode is evidence of a Sub tuning software bug.

These results supersede basic hosted acceptance deferrals above.
Intermediate LCD rounding, Osc Select independence, dedicated audible tests,
hosted automation and session-reopen acceptance remain separate untested work.
Accidental LFO1 Pitch and diagnostic Resonance edits are not claimed restored
to their original pre-test values.
