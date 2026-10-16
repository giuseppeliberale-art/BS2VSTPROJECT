# Next version notes

## Live RX status wording

Requested on 16 October 2026 during hardware validation.

Show the current state using these exact labels:

- **Live RX is paused** when live CC/NRPN reception is paused.
- **Live RX is running** when live CC/NRPN reception is enabled.

The current checked control labelled “Resume Live RX” was confusing because it described an action while its tick represented the paused state. Use the requested status wording in the next version. SysEx reception remains active while Live RX is paused.

Recorded for the next version; no UI behavior or labels changed yet.
