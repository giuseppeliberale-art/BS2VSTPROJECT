# Oscillator extra controls — discovery batch, 18 October 2026

Firmware 4.14, MIDI channel 1. User corrected overlay label to `E:OscErr`.
Hardware access: hold Function/Exit, press PB Range twice; value display E-0…E-7.

Oscillator Error incoming capture
`midi-lab-results/20261018-052231-record-a3bdb1/events.jsonl` contains NRPN
0:111 values 1–7. Initial 0 produced no event. Subsequent capture
`20261018-052332-record-cdaafa/events.jsonl` contains no complete NRPN events,
so emitted zero is not yet confirmed by that capture.

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
