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
- Channel filtering and paused RX have not yet been exercised live.
  Superseded replies tested offline;
  no induced delayed hardware replies. Firmware replies lack request IDs, so
  arbitrarily late replies after request timeout cannot be reliably correlated.

Preset selection sends Program Change and current edit-buffer requests only.
It does not save to a stored preset slot. Temporary edits are replaced by a load.
