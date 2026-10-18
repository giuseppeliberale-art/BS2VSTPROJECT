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
