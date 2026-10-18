# Oscillator extra controls — discovery batch, 18 October 2026

Firmware 4.14, MIDI channel 1. User corrected overlay label to `E:OscErr`.
Hardware access: hold Function/Exit, press PB Range twice; value display E-0…E-7.

Oscillator Error incoming capture
`midi-lab-results/20261018-052231-record-a3bdb1/events.jsonl` contains NRPN
0:111 values 1–7. Initial 0 produced no event. Subsequent capture
`20261018-052332-record-cdaafa/events.jsonl` contains no complete NRPN events,
so emitted zero is not yet confirmed by that capture.

Follow-up `midi-lab-results/20261018-163206-record-3e9eb6/events.jsonl`
captured NRPN 0:111 values 1 then 0 after explicit E-1→E-0 hardware changes.
Together with the earlier 1–7 sweep, all emitted values 0–7 are now confirmed.
The hardware was left at E-0. Full remote/SysEx sweep remains pending.

### Oscillator Error automatic verification completed

With Ableton closed, the `osc-error` group in `tools/LFOSyncSampler.swift`
tested all eight NRPN 0:111 values. Every requested patch reply matched
`(b[117] & 0x38) >> 3`, and no unrelated patch bits changed. The complete starting
patch was restored and read back byte for byte. No hardware preset was saved.
Evidence: `midi-lab-results/osc-error-discovery-20261018/`, retained as
`tests/osc-error-fixtures/` (eight values, before/after and samples.csv).

Oscillator Error mapping is now verified for emitted MIDI, remote receipt and
SysEx storage across all values 0–7. Plugin implementation and manual hosted/
audible checks are still deferred to the next implementation batch.

Labelled hardware patch dumps at error 7 (`20261018-052328-backup-741f71/baseline.syx`)
and error 0 (`20261018-052506-backup-d7fefb/baseline.syx`) differ only at zero-based
byte 117, decimal 56 → 0. Candidate field `(b[117] & 0x38) >> 3`. Full remote sweep
and decoder integration remain pending. Paths above are under `midi-lab-results/`.

User accidentally entered PB Range with a single press during the repeat, then
corrected access and set E-0. Both before/after dumps store bend wire 76 at byte
16 (+12 semitones); Pitch Bend Range ended identical to the backup. No restoration
was needed or performed. Do not treat incidental CC107 events as Oscillator Error.

No new oscillator-extra controls have been implemented in the plugin yet.

## Paraphonic mode — unexpected emitted route

Access: Function/Exit + Osc Sync keyboard key twice; display P-0/P-1.
Manual documents receive route NRPN 0:107, not yet remotely verified.

Two passive captures, `20261018-052603-record-7d3fd8` and
`20261018-052735-record-f463c3`, each recorded CC110=1 followed by CC110=0,
not NRPN 0:107. User explicitly confirmed P-0 before the second capture. Initial
instructions incorrectly described Off/On labels; later corrected to P-0/P-1.
Do not attribute the second result to user menu selection without evidence.

Controlled patch dumps proved the hardware action changed Paraphonic storage:

- P-0: `midi-lab-results/20261018-052914-backup-4506ca/baseline.syx`.
- P-1: `midi-lab-results/20261018-052949-backup-82bf9e/baseline.syx`.
- P-0 restored: `midi-lab-results/20261018-053027-backup-9a60f9/baseline.syx`.

P-0→P-1 changed only zero-based byte 111 from 0 to 2 (mask 0x02), leaving
the existing Osc Sync field at byte 18 mask 0x40 unchanged. Returning to P-0
restored the complete earlier patch byte for byte. This verifies the labelled
SysEx mode bit both ways, not yet the documented remote NRPN receive route.

The app currently interprets CC110 as Osc Sync. These captures suggest hardware
Paraphonic edits may therefore be misrepresented by live UI sync; do not alias
CC110 to both parameters or guess which mode was edited. Investigate an explicit
patch refresh or another reliable discriminator before implementing live follow.
Firmware 4.14 transmission quirk is a possibility, not an established diagnosis.

### Paraphonic remote reception verified

The automatic `paraphonic` group sent documented NRPN 0:107 values 0 and 1.
Both patch replies matched byte 111 mask 0x02, with no unrelated bits changed.
The complete starting patch was restored and verified exactly. No preset was saved.
Evidence: `midi-lab-results/paraphonic-discovery-20261018/`, retained as
`tests/paraphonic-fixtures/` (both settings, before/after and samples.csv).

This establishes the remote receive route and SysEx field. It does not resolve
the different CC110 messages emitted by hardware edits. Keep the existing
CC110 Osc Sync receive meaning; do not treat incoming CC110 as an unambiguous
Paraphonic change. Plugin implementation and manual audible checks remain pending.

## Glide Divergence — mapping verified

Hardware shortcut: hold Function/Exit and press Input Gain twice; display g-0…g-15.
Forward capture `midi-lab-results/20261018-163652-record-76cb47/events.jsonl`
recorded NRPN 0:113 values 1–15. Reverse capture
`20261018-163754-record-b1b1fd/events.jsonl` recorded 14 down through 0, completing
all emitted values. Hardware endpoint dumps at g-15
(`20261018-163751-backup-84828c/baseline.syx`) and g-0
(`20261018-163832-backup-2322ba/baseline.syx`) differed only at zero-based byte
118, decimal 60→0. All paths above are under `midi-lab-results/`.

Automatic group `glide-divergence` tested all 16 NRPN 0:113 values against
`(b[118] & 0x3C) >> 2`. Every value matched, with no unrelated bits changed.
Full starting-patch restoration was verified byte for byte; no preset was saved.
Evidence run: `midi-lab-results/glide-divergence-discovery-20261018/`, retained
under `tests/glide-divergence-fixtures/` with before/after, 16 dumps and samples.csv.

Glide Divergence is fully mapped for emitted MIDI, remote receipt and SysEx
storage. Like Oscillator Error and Paraphonic mode, it is not yet implemented in
the installed plugin. Manual hosted/audible checks remain pending.

## Plugin integration — Oscillator Error and Glide Divergence

The discovery-stage deferrals above are superseded for these two controls:
Oscillator Error 0–7 is implemented in FX + Perf, and Glide Divergence 0–15
in LFO. Both are stepped choices matching hardware ranges, with existing
NRPN send/receive, snapshot/session state and automation paths. New host
indices 89 and 90 follow MicroTune at 88, preserving all published indices.
Get Patch and SysEx imports decode the verified fields. No stored-preset
write command is added. Paraphonic ambiguity remains unresolved and its
implementation is still deferred. Hosted and audible acceptance are pending.

### Integration validation completed

Report: `validation-results/20261018-173004-20712/`. All seven offline test
groups passed, including decoding the eight Oscillator Error and sixteen
Glide Divergence hardware fixtures. Virtual MIDI processor integration passed
NRPN output, hardware input without echo, saved-state recall, patch refresh
and preservation of published host indices. Direct SysEx integration passed.
Standalone, AU and VST3 builds passed. With Live confirmed closed, both plugins
were backed up and replaced; strict signature verification and Apple AU
validation passed. Manual Ableton and audible acceptance remain pending.
The public preview release assets have not been replaced.

### Manual Ableton acceptance passed

User verified Oscillator Error plugin selection 7 as E-7 on the hardware
after reopening the on-key menu without editing Patch/Value. Hardware E-6
updated the plugin to 6. With Live RX paused, hardware E-0 left the plugin
at 6; Get Patch then updated it to 0.

User verified Glide Divergence plugin selection 15 as g-15 on the hardware
after reopening its on-key menu without editing Patch/Value. Hardware g-14
updated the plugin to 14. With Live RX paused, hardware g-0 left the plugin
at 14; Get Patch then updated it to 0. User confirmed resuming Live RX and
exiting the hardware menu. Both controls ended at zero.

These results supersede the basic hosted acceptance deferrals above.
Dedicated audible checks, hosted automation recording/playback and hosted
session-reopen acceptance for these controls remain untested.

## Paraphonic repeat — documented emitted NRPN confirmed

A new labelled test began with user-confirmed P-0 and an immediate baseline
dump. Capture `20261018-223413-record-124954/events.jsonl` recorded exactly
NRPN 0:107 value 1 for the requested P-0 to P-1 edit. The corresponding dump
`20261018-223439-backup-6d06d4/baseline.syx` differed from
`20261018-223412-backup-9f4f5b/baseline.syx` only at byte 111, 0 to 2.
Osc Sync byte 18 mask 0x40 stayed off.

Reverse capture `20261018-223457-record-5d0817/events.jsonl` recorded exactly
NRPN 0:107 value 0. Final dump
`20261018-223521-backup-9c55c8/baseline.syx` matched the original full patch
byte for byte. Paths are under midi-lab-results. Labelled dumps and emitted
event files are retained as hardware-p*-repeat* under tests/paraphonic-fixtures.

This verifies the documented emitted route in the current configuration.
The earlier CC110 observations are retained as unresolved history; their
cause is not established, and they must not be silently reinterpreted.
Implement explicit Paraphonic receive on NRPN 0:107, retain Osc Sync on CC110,
and do not alias one CC to both parameters.

### Paraphonic plugin integration

Paraphonic Mode Off/On is appended to FX + Perf at host index 94. It uses
NRPN 0:107 for output and explicit live reception, plus verified byte 111
mask 0x02 for Get Patch/import decoding. CC110 retains Osc Sync meaning
only. No heuristic alias or automatic ambiguous-CC patch refresh is added.
The current verified route supersedes the implementation deferral; earlier
conflicting capture history remains unexplained. Basic hosted/audible
acceptance is pending.

### Paraphonic local validation and installation completed

Report: `validation-results/20261018-223650-27596/`. All seven offline
groups passed, including replay of both new emitted NRPN captures and
decoding labelled P-0/P-1 fixtures. Native processor integration passed
NRPN output, reception, silent state/patch refresh, host index preservation
and independent CC110 Osc Sync behavior. Direct SysEx regression passed.
Standalone/AU/VST3 builds passed. Installation initially waited because
Live remained running; after the second quit confirmation, the process
guard passed and both plugins were backed up and replaced. Strict installed
signature verification and Apple AU validation passed. Manual hosted/audible
acceptance remains pending. Public preview assets have not been replaced.

### Paraphonic manual Ableton acceptance passed

User set plugin Paraphonic Mode On, reopened the hardware on-key function
without editing Patch/Value, and confirmed P-1. Hardware P-1 to P-0 updated
the plugin to Off. With Live RX paused, hardware P-0 to P-1 left the plugin
Off; Get Patch then updated it to On. User completed plugin Off and resumed
Live RX. A separate hardware readback after the final Off command was not
requested. No stored preset was saved.

This supersedes basic hosted acceptance deferrals for Paraphonic Mode above.
Dedicated audible two-note behavior, hosted automation/session-reopen checks,
and the cause of the earlier conflicting CC110 captures remain unverified.
