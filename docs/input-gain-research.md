# External Input Gain — discovery, 18 October 2026

Hardware firmware 4.14, MIDI channel 1. Function/Exit + Input Gain key once.
User explicitly observed display minimum 0, maximum 62, increments of 2.

Capture `midi-lab-results/20261018-165356-record-e3c2c3/events.jsonl` selected
NRPN **64:28** (MSB 64, not 0) and sent all raw values 0–31. Capture began
4,3,2,1,0, then ascended through 31. Combined with the user observation, display
is provisionally raw*2; no dB calibration has been established.

Cached official firmware 4.14 manual's Global: Input Gain section instead lists
-10 dB to +60 dB. Do not overwrite captured range or claim the displayed numbers
are dB. This discrepancy remains unresolved.

Remote reception, global setting readback/restoration, whether the value appears
in ordinary patch SysEx, and audible input gain have not been verified. The
existing automatic sampler assumes NRPN MSB 0 and patch-stored settings: do not
use it unchanged for NRPN 64:28. No Input Gain control is implemented yet.

## Patch comparison

User set Input Gain to displayed 62 and then 0. The requested 154-byte edit-buffer
dumps were byte-for-byte identical:

- 62: `midi-lab-results/20261018-165509-backup-89134b/baseline.syx`.
- 0: `midi-lab-results/20261018-165545-backup-4f125d/baseline.syx`.

This comparison found no ordinary single-patch SysEx field for the setting. It
supports treating it as a separate global control, rather than inventing a patch
offset; it does not rule out other global SysEx messages. Hardware ended at
displayed Input Gain 0. Remote reception and global readback remain unverified.

## Remote reception spot-check

With hardware starting at displayed 0 (raw 0), the narrowly scoped
`tools/InputGainSender.swift` sent NRPN 64:28 raw 1 on MIDI channel 1. User
reopened the Input Gain hardware menu without touching Patch/Value and explicitly
confirmed displayed 2. This verifies remote reception for raw 1 and its display
mapping; it is not a remote sweep of all 32 settings.

The sender then transmitted raw 0 to restore the known initial setting. MIDI
transport succeeded; hardware display confirmation of restoration is pending.
No preset-save or patch-replacement messages were sent. Global value-query
support and physical dB calibration remain unresolved.

User subsequently reopened Input Gain without editing and confirmed displayed
0. Restoration of the known initial global setting is therefore hardware-verified.
