# Preset selection validation

2026-10-17, Bass Station II firmware4.14 reported by user, MIDI channel1.

- User selected hardware preset1. Passive capture recorded final Program Change1.
  Evidence: ignored `midi-lab-results/20261017-064829-record-97d68e/summary.json`.
- Read-only backup of current preset1:
  `midi-lab-results/20261017-065533-backup-4c8a19/baseline.syx`.
- New app launched with unknown preset and no patch request/Program Change on
  startup. Preset0 selected via app, then Next selected1. Each received154-byte
  edit-buffer reply and automatic decode. Previous disabled at0, enabled at1.
- Saved editor state `~/Documents/preset-one.bsiistate` compared through
  BSIISnapshotRecallTest against hardware backup: PASS all76 controls.
- Offline7 CTest groups passed; preset state tests cover unknown startup, bounds,
  settling, rapid changes, stale reply rejection and counter wrap.
- User selected preset2 on the hardware. App selector followed to2 automatically
  and received/decoded a154-byte patch without Get Patch. This patch decoded to
  the same exposed field values as preset1 (no editor fields changed).
- Paused RX and channel mismatch/recovery were exercised live as detailed below.
  Superseded replies tested offline;
  no induced delayed hardware replies. Firmware replies lack request IDs, so
  arbitrarily late replies after request timeout cannot be reliably correlated.

Preset selection sends Program Change and current edit-buffer requests only.
It does not save to a stored preset slot. Temporary edits are replaced by a load.

## Paused RX and channel filtering follow-up

2026-10-17, existing signed label-fix standalone; no source changes.

| Setup | Hardware action | Observed result |
| --- | --- | --- |
| App channel 1, Live RX paused, displayed preset 20 | Select preset 21 | Independent capture recorded Program Change 21 on channel 1. App remained on 20, with unchanged patch display. |
| Same paused state | Agent clicked Get Patch | App received/decoded a fresh 154-byte patch and updated sound controls while remaining paused. The preset number stayed 20 because a dump does not establish a preset number. |
| App channel 2, Live RX running; hardware channel unchanged at 1 | Select preset 22 | Independent capture recorded Program Change 22 on channel 1. App retained unknown preset identity and its previous patch display; no automatic patch refresh appeared. |
| Restore app channel 1, Live RX running | Select preset 23 | Capture recorded Program Change 23 on channel 1. App followed to 23 and automatically received/decoded a 154-byte patch, updating sound controls. |

Ignored capture reports, in order:

- `midi-lab-results/20261017-172528-record-416d48/summary.json`
- `midi-lab-results/20261017-172632-record-d87885/summary.json`
- `midi-lab-results/20261017-172744-record-ce0622/summary.json`

These passive recordings contain the Program Change events. Patch receipt/decode
was verified separately in the app's UI; the recorder did not retain those replies.
This checks rejecting channel 1 when channel 2 is selected and recovering when
channel 1 is restored. Matching reception/transmission on channels 2–16 was not
exercised. Final state: hardware preset 23, app channel 1, Live RX running,
recorder stopped. No preset memory save/clear operation was performed.
