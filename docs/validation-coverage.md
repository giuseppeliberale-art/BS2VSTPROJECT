# Validation coverage audit — 17 October 2026

Audited application source and committed captures at `d436925`; updated after the eight main oscillator waveform captures and mapping fix following `0d408be`, and the Coarse regression sweeps following `06d8d5c`, full Fine validation following `7240353`, and snapshot/Send All integration following `72e3fed`. This is a coverage inventory, not a new hardware test or a declaration that every UI path works. No synth settings were changed during this audit. The `v0.8.2` baseline tag remains on `91d1ffc`.

## Current result

- **76 exposed controls:** 58 numeric parameters and 18 choices (`Source/BSIIMidi.h`).
- **All 76 controls have patch fields applied to the editor.** The two main oscillator waveform omissions identified by the audit have now been filled using hardware captures.
- **All 76 controls have setting/value captures replayed through their decoder fields.** Coverage includes expected hardware clamping and normalization, not necessarily a unique result for every MIDI input.
- **Both Fine controls now have full 256-input captured sweeps**, including measured clamping to 27–228 and both center codes, plus five earlier manual positions each.
- **Both Coarse controls now have full 256-value captured decoder sweeps and five earlier manual observations each.** The previous Coarse regression gap is closed; exact LCD rounding remains uncalibrated.
- **Both main oscillator waveforms now have four-setting SysEx captures and decoder replay**, in addition to their earlier manually confirmed bidirectional live MIDI. Sub Wave also has captured SysEx coverage.

The fixture directory contains **95 files / 9,123 patch messages**, all traversed by the captured-patch test: 63 sweep files (9,003 messages), four option-combination files (92 messages), eight range messages, ten Fine messages, and ten Coarse messages. These are fixture-message counts, not independent acoustic tests or assertions of end-to-end UI coverage.

`ctest --test-dir build -C Release --output-on-failure` now passes six registered tests: StandaloneMidiRouting, two snapshot round-trip comparisons, two complete Send All recall comparisons, and Osc1RangeCapturedPatches. The latter's historical name now covers all 76 exposed controls; the name does not imply that only Oscillator 1 Range is tested. Two additional integration SysEx files and four snapshots live separately under `tests/state-fixtures/`; they are not included in the 95-file sweep/observation count above.

## Next work, in order

1. **Completed: main oscillator waveform SysEx.** Eight captured settings now drive both decoder fields and editor application. Both missing-update regressions were reproduced before the fix.
2. **Completed: Coarse regression.** Both new 256-value sweeps and all ten existing observations now replay through the actual decoder. Full-patch restoration was verified after each sweep.
3. **Completed: Fine transport coverage.** Both full 256-input sweeps now replay the measured 27–228 clamp and both center codes. Slider limits match the hardware; acoustic and exact LCD calibration remain outside these tests.
4. **Partly completed: application integration.** Snapshot save/load preserves all 82 persisted entries; actual app Send All reproduces the baseline patch exactly and an alternate snapshot matches all 76 decoded controls. See [state-integration-validation.md](state-integration-validation.md). Remaining work includes hardware-origin live reception, Acid/other combined recalls, and plugin-host smoke tests. Most full value sweeps still use the CoreMIDI sampler rather than hardware knobs or the app sending function.

## Evidence limits

- Firmware version was not recorded. Results describe this connected synth and the saved captures.
- The tests establish MIDI transport, returned patch storage, and decoding. They do not measure acoustic modulation, frequency, envelope timing, output level, waveform shape, or physical-pot resolution.
- Coarse's decimal display remains approximate; the LCD does not update from received MIDI, so automated sweeps do not calibrate its exact rounding.
- Envelope options were tested as Trigger × Retrigger and Fixed Sustain × Count pairs. Filter switches cover all 12 combinations. There is no exhaustive interaction test of every control together.
- “MIDI verified” UI badges describe the historical live MIDI checks. Decoder “verified” versus “candidate” counters are historical classification, not a current coverage score; many candidate fields now have exhaustive captures.
- Selected fixes received visual import/fetch checks, and one combined alternate Send All recall now matches every exposed control. These do not prove every intermediate UI setting, parameter interaction, or live receive route.
- Current changes were built for Standalone and the regression targets. A fresh AU/VST3 build and host/session recall test of this head are not recorded.
- Malformed/truncated SysEx, device disconnect/reconnect, wrong-channel behavior, and simultaneous UI/host/hardware edits do not have dedicated captured-path regression coverage.
- Bend's editor supports only +1 to +12 despite the captured wider signed hardware wire range. Live bend conversion was changed, but physical hardware-origin live CC reception was not manually checked afterward.
- Raw Sync and Sub Octave controls retain numeric UI values; patch decoding uses canonical 0/127. Acoustic octave direction was not established by the sub-octave transport sweep.
- Hardware functions not exposed in the app (for example additional arp or LFO sync modes) are outside this 76-control inventory.

## Complete exposed-control inventory

The table maps every current parameter/choice ID to its available evidence. “Captured sweep” refers to committed files under `tests/fixtures/`, replayed by `tests/Osc1RangeTest.cpp`; CSVs and methodology are described in [hardware-validation.md](hardware-validation.md). Read historical notes chronologically; this audit summarizes the current coverage.

| Section | Control ID | MIDI route | Current patch/decoder coverage |
| --- | --- | --- | --- |
| Master | `patchVolume` | CC 7 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1Fine` | CC 26/58 | 256-input captured clamp/center sweep + five manual observations + display tests |
| Osc 1 | `osc1Coarse` | CC 27/59 | 256-value captured sweep + five manual observations replayed |
| Osc 1 | `osc1ModEnvPitch` | CC 71 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1Lfo1Pitch` | CC 28/60 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1ModEnvPW` | CC 72 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1Lfo2PW` | CC 73 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1ManualPW` | CC 74 | Captured setting/value sweep + decoder replay |
| Osc 2 | `osc2Fine` | CC 29/61 | 256-input captured clamp/center sweep + five manual observations + display tests |
| Osc 2 | `osc2Coarse` | CC 30/62 | 256-value captured sweep + five manual observations replayed; earlier CSV sweeps retained |
| Osc 2 | `osc2ModEnvPitch` | CC 76 | Captured setting/value sweep + decoder replay |
| Osc 2 | `osc2Lfo1Pitch` | CC 31/63 | Captured setting/value sweep + decoder replay |
| Osc 2 | `osc2ModEnvPW` | CC 77 | Captured setting/value sweep + decoder replay |
| Osc 2 | `osc2Lfo2PW` | CC 78 | Captured setting/value sweep + decoder replay |
| Osc 2 | `osc2ManualPW` | CC 79 | Captured setting/value sweep + decoder replay |
| Mixer | `mixOsc1` | CC 20/52 | Captured setting/value sweep + decoder replay |
| Mixer | `mixOsc2` | CC 21/53 | Captured setting/value sweep + decoder replay |
| Mixer | `mixSub` | CC 22/54 | Captured setting/value sweep + decoder replay |
| Mixer | `mixNoise` | CC 23/55 | Captured setting/value sweep + decoder replay |
| Mixer | `mixRing` | CC 24/56 | Captured setting/value sweep + decoder replay |
| Mixer | `mixExternal` | CC 25/57 | Captured setting/value sweep + decoder replay |
| Osc 1 | `subOctaveRaw` | CC 81 | Captured setting/value sweep + decoder replay |
| Filter | `filterCutoff` | CC 16/48 | Captured setting/value sweep + decoder replay |
| Filter | `filterResonance` | CC 82 | Captured setting/value sweep + decoder replay |
| Filter | `filterModEnv` | CC 85 | Captured setting/value sweep + decoder replay |
| Filter | `filterLfo2` | CC 17/49 | Captured setting/value sweep + decoder replay |
| Filter | `filterOverdrive` | CC 114 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `portamento` | CC 5 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo1Speed` | CC 18/50 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo1Delay` | CC 86 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo2Speed` | CC 19/51 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo2Delay` | CC 87 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo1Slew` | NRPN 0:86 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo2Slew` | NRPN 0:90 | Captured setting/value sweep + decoder replay |
| Filter | `filterTracking` | NRPN 0:108 | Captured setting/value sweep + decoder replay |
| Amp Env | `ampAttack` | CC 90 | Captured setting/value sweep + decoder replay |
| Amp Env | `ampDecay` | CC 91 | Captured setting/value sweep + decoder replay |
| Amp Env | `ampSustain` | CC 92 | Captured setting/value sweep + decoder replay |
| Amp Env | `ampRelease` | CC 93 | Captured setting/value sweep + decoder replay |
| Amp Env | `ampRetriggerCount` | NRPN 0:117 | Captured option combinations + decoder replay (see limits below) |
| Mod Env | `modAttack` | CC 102 | Captured setting/value sweep + decoder replay |
| Mod Env | `modDecay` | CC 103 | Captured setting/value sweep + decoder replay |
| Mod Env | `modSustain` | CC 104 | Captured setting/value sweep + decoder replay |
| Mod Env | `modRelease` | CC 105 | Captured setting/value sweep + decoder replay |
| Mod Env | `modRetriggerCount` | NRPN 0:118 | Captured option combinations + decoder replay (see limits below) |
| FX / Perf | `distortion` | CC 94 | Captured setting/value sweep + decoder replay |
| FX / Perf | `oscSyncRaw` | CC 110 | Captured setting/value sweep + decoder replay |
| FX / Perf | `oscFilterMod` | CC 115 | Captured setting/value sweep + decoder replay |
| FX / Perf | `arpSwing` | CC 116 | Captured setting/value sweep + decoder replay |
| FX / Perf | `bendAmount` | CC 107 | Captured setting/value sweep + decoder replay |
| FX / Perf | `velocityAmp` | CC 112 | Captured setting/value sweep + decoder replay |
| FX / Perf | `velocityMod` | CC 113 | Captured setting/value sweep + decoder replay |
| FX / Perf | `vcaLimit` | CC 95 | Captured setting/value sweep + decoder replay |
| FX / Perf | `modWhLfo1Pitch` | NRPN 0:70 | Captured setting/value sweep + decoder replay |
| FX / Perf | `modWhOsc2Pitch` | NRPN 0:78 | Captured setting/value sweep + decoder replay |
| FX / Perf | `aftertouchFilter` | NRPN 0:74 | Captured setting/value sweep + decoder replay |
| FX / Perf | `aftertouchLfo1` | NRPN 0:75 | Captured setting/value sweep + decoder replay |
| FX / Perf | `aftertouchLfo2Spd` | NRPN 0:76 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1Wave` | NRPN 0:72 | Four captured waveform settings + decoder replay; live MIDI manually confirmed |
| Osc 1 | `subWave` | CC 80 | Captured setting/value sweep + decoder replay |
| Osc 1 | `osc1Range` | CC 70 | Captured setting/value sweep + decoder replay |
| Osc 2 | `osc2Wave` | NRPN 0:82 | Four captured waveform settings + decoder replay; live MIDI manually confirmed |
| Osc 2 | `osc2Range` | CC 75 | Captured setting/value sweep + decoder replay |
| Filter | `filterType` | CC 83 | Captured option combinations + decoder replay (see limits below) |
| Filter | `filterShape` | CC 84 | Captured option combinations + decoder replay (see limits below) |
| Filter | `filterSlope` | CC 106 | Captured option combinations + decoder replay (see limits below) |
| LFO / Glide | `lfo1Wave` | CC 88 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo2Wave` | CC 89 | Captured setting/value sweep + decoder replay |
| Amp Env | `ampTrigger` | NRPN 0:73 | Captured option combinations + decoder replay (see limits below) |
| Mod Env | `modTrigger` | NRPN 0:105 | Captured option combinations + decoder replay (see limits below) |
| Amp Env | `ampRetrigger` | NRPN 0:109 | Captured option combinations + decoder replay (see limits below) |
| Mod Env | `modRetrigger` | NRPN 0:110 | Captured option combinations + decoder replay (see limits below) |
| Amp Env | `ampFixed` | NRPN 0:114 | Captured option combinations + decoder replay (see limits below) |
| Mod Env | `modFixed` | NRPN 0:115 | Captured option combinations + decoder replay (see limits below) |
| LFO / Glide | `lfo1KeySync` | NRPN 0:89 | Captured setting/value sweep + decoder replay |
| LFO / Glide | `lfo2KeySync` | NRPN 0:93 | Captured setting/value sweep + decoder replay |
