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

## Upper receive endpoint and restoration

Starting hardware display 0 was explicitly confirmed. The scoped sender
transmitted raw 31; user reopened the menu without editing and confirmed 62.
The sender then transmitted raw 0. User may have accidentally changed preset
during navigation, but subsequently reopened Input Gain and confirmed zero.
Together with the earlier raw-1/display-2 check, remote receipt has now been
spot-checked at raw 0, 1 and 31. This is not a full 32-value remote sweep.

## Global control implementation

Input Gain is appended as host parameter index 93, preserving all 93 previous
indices. Raw storage is 0–31; editor and host text show 0–62 in steps of 2.
No dB label is used. It lives on a separate Global page whose note states
that Get Patch does not read it and shown values follow edits/received MIDI.

Direct edits and host automation use NRPN 64:28. Hardware reception follows
the selected MIDI channel and Live RX setting. There is no invented patch
decoder field. Send All excludes global controls. Portable patch snapshots
omit Input Gain; snapshot loading preserves the current global control even
if an older/custom snapshot contains an Input Gain entry. Host session state
remembers the parameter value silently; restoring that state does not itself
send an Input Gain command. A host automation lane can explicitly send it.

No global-value query, persistence across synth power cycles or dB calibration
is established. Plugin hosted/audible acceptance remains pending.

### Local validation and installation completed

Report: `validation-results/20261018-222337-26455/`. All seven offline groups
passed. Native processor integration passed NRPN 64:28 output/reception,
host text scaling, index 93, silent host-state recall, patch snapshot filtering
and merge preservation. A real virtual-port Send All capture excluded Input
Gain. Get Patch refresh preserved its existing value. Direct SysEx regression
passed. Standalone, AU and VST3 builds passed. Live was confirmed closed;
AU/VST3 were backed up, replaced and locally signed. Strict signature
verification and Apple AU validation passed. No live hardware gain changes
were sent during these isolated regression checks. Hosted acceptance remains
pending; public preview release assets have not been replaced.
