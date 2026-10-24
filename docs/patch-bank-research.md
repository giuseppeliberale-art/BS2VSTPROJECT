# Patch-bank backup and import research

20 October 2026. Official Novation sources and current project source only.
No MIDI was sent, no hardware data was changed, and no runtime code was changed.

## Documented workflows

- Manual Global: Dump offers onE/All; All transmits all 128 patches sequentially
  over USB and MIDI OUT. See the manufacturer's procedure before performing it.
  [Novation: Global Dump](https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail).
- Components offers Get Bank from Bass Station II, then editing/download/save.
  Editor changes do not change stored hardware content until Send to Bass Station
  II is used. [Components guide: Banks](https://support.novationmusic.com/hc/en-gb/articles/4411786558098-Bass-Station-II-Components-guide).
- Components Upload Bank loads bank SysEx for inspection; bank Send sends all
  patches. Replace can load an individual patch into a bank, with a separate
  individual Send action. [Importing SysEx to Components](https://support.novationmusic.com/hc/en-gb/articles/4405239706770-Importing-SysEx-to-Novation-Components).
- The manual distinguishes a single imported patch becoming the active edit
  buffer (normal Save needed for persistence) from a complete library upload,
  which automatically overwrites all stored patches.
  [Novation appendix: Importing patches via SysEx](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix).

## Coverage and unknowns

A patch bank must not be labelled a complete device backup. AFX overlays have
separate banks and dedicated transfer commands. The manual describes four
persistent sequence memories and a selected sequence stored with a patch, but
this does not establish that bank SysEx contains all note/rest/tie contents.
[Novation: Sequencer and AFX mode](https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail).

Tuning selection can be saved with a patch; tuning contents and their save
operation are separate. The appendix publishes AFX-specific indexed messages
and a 106-byte response; those cannot be extrapolated into ordinary-bank slot
addressing. [Novation appendix: Micro-tuning and AFX SysEx](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix).

The bounded official-source review did not establish ordinary bank request
commands, exact packet layout/length, slot-address bytes/numbering, transfer
timing/acknowledgements, cross-tool stream equivalence or sequence-content
coverage. Workflow documentation does not supply those wire details. These
remain unknown, not proof that a protocol is unavailable.

## Current source boundaries

- `Source/BSIIPatchDecoder.h::looksLikeSingleBsiIIPatchDump` checks 154 bytes,
  the Novation/BSII prefix and F7, without command/destination validation.
- `Source/BSIIProjectRecall.h::isRecallPatch` additionally requires 7-bit
  interiors, bytes 6–12 zero and successful decode. This is the previously
  tested current-edit-buffer format, not a stored-slot authorization.
- `Source/PluginEditor.cpp::saveLastSysEx` saves only the latest message;
  it is not a complete multi-message bank recorder.
- `sendSysExFileToHardware` checks minimum size and outer F0/F7 only. It does not
  validate interior message count, command or destination. Its edit-buffer status
  wording does not establish write protection for arbitrary files.

## Staged implementation plan

1. Build an offline stream inspector first: preserve raw bytes and hashes, split
   messages, report offsets/lengths/headers and malformed/truncated data. Retain
   unknown messages. Keep framing, classification, decoding and write eligibility
   distinct. A 154-byte match alone must never authorize transmission.
2. Capture a manual All dump without software output, recording firmware,
   connection and method. Compare offline with a separately exported Components
   bank. Establish completeness, count/order and candidate slot layout before any
   restoration. Preserve valuable unsaved sound before a manual capture session.
3. Establish command/addressing semantics from controlled labelled fixtures.
   Keep sequence contents, AFX and tuning coverage explicitly separate.
4. Harden the existing sender before restoration: restrict edit-buffer send to
   its validated command; give stored-slot/bank writes separate validation,
   destination previews and complete-backup prerequisites.
5. Only with explicit authorization for a specified disposable slot: write one
   slot, read back, compare and confirm neighbouring slots unchanged. A later
   full-bank restore needs its own review and authorization.

This is a research plan, not an implemented bank feature or permission to write
hardware memory. Existing patch and recall paths were left unchanged.

## Offline inspector implemented — 20 October 2026

Stage 1 now has `lab.py inspect-sysex FILE [--json]`, a bounded read-only stream
inspector. It preserves all bytes and hashes, identifies framing issues and
normalized duplicates, and only labels the known edit-buffer shape as a candidate.
Unknown commands remain unknown; all transmit eligibility stays false.
[Usage and focused validation](research-toolkit.md#inspect-a-binary-sysex-stream).
Next: passively capture a complete manual All dump, then compare its contents and
structure with a Components export. No live bank capture or slot write was added.

## First passive hardware All capture — 20 October 2026

The user saved/quit Ableton and manually selected aLL on Global: Dump, then
triggered export while the passive CoreMIDI listener was running. The first
attempt produced only one message (selection not confirmed); it is not a bank.
The second attempt followed explicit user confirmation of aLL and yielded:

- 128 complete SysEx messages, each 154 bytes: 19,712 SysEx bytes total.
- Six leading channel-message bytes (a Note On/Off pair) in the original stream.
  The inspector correctly flagged outside-SysEx data. These bytes were retained
  in the original stream, with a separate clean SysEx-only extraction.
- The 128 messages have the BSII prefix. Zero-based header byte 7 is 1, byte 8
  progresses exactly 0 through 127, and bytes 6 and 9–12 are zero in this capture.
  These are observed export-header values, not verified write commands/addresses.
  All messages correctly remain classified bsii-unknown-command by the inspector.
- Existing patch-field decoding accepted all 128 messages. This does not establish
  that every field/unknown byte has been interpreted correctly or that restoration
  is supported. No message was transmitted back, and no memory was overwritten.
- Bank completeness is supported by the manual All procedure and observed
  sequential 128-message export, but independent Components comparison and any
  restoration/readback test remain pending. The generic inspector still reports
  verifiedBank=false and transmitEligible=false.

Private local evidence (ignored by Git; user patches are not published):
`midi-lab-results/20261020-111111-record-9cb540/` contains packets.jsonl,
received-stream.syx, stream-inspection.json, patch-bank.syx,
patch-bank-inspection.json, bank-summary.json and individually split messages/.
The initial single-message attempt is in `20261020-110928-record-960207/`.

SHA256 of received-stream.syx (19,718 bytes):
`22043a5dc1904f87f7a10b22c1a937e848bfcb50f2b5c55e165dd19d3933fca6`.
SHA256 of patch-bank.syx (19,712 bytes):
`675ee6c6bced2facd0b880a58b1bbe8bdfd2587237d774d78d5c61dc64a8a232`.

Next: independently Get Bank in Components, download without Send, compare the
export offline and document any ordering/header/payload differences. Do not use
the ordinary Send .syx action to test this bank: stored-memory effects remain
unverified. This bank export is not a backup of tuning-table contents or AFX.

## Independent Components comparison — 20 October 2026

The user supplied a Components bank export, `Downloads/components patch.syx`,
following Get Bank/download instructions. The offline inspector reported 128
complete messages, clean framing, no duplicate messages and 19,712 bytes. It was
byte-for-byte identical to the SysEx-only manual All dump, including message
order, headers and payloads. Both SHA256 hashes are:
`675ee6c6bced2facd0b880a58b1bbe8bdfd2587237d774d78d5c61dc64a8a232`.

A private copy (`components-bank.syx`) and `components-comparison.json` are retained
beside the manual capture in the ignored evidence directory above. Neither user's
bank content was published to GitHub. Original files were left unchanged. No MIDI
was sent and no bank restoration/stored-slot writing was attempted. This confirms
export equivalence for these two files on this device, not a universally verified
write protocol or completeness of separate tuning/AFX/sequence stores.

Procedure clarification: the user reported the Dump menu timed out before the
listener was ready. They re-entered the function, selected aLL and triggered Dump
while listening. Future instructions should select aLL after the listener is ready,
or explicitly recheck it immediately before transmission. No recapture was needed.

Next prerequisite before any restoration: harden the generic .syx sender so it
cannot mistake these stored/bank-shaped messages for the validated edit-buffer
command. A separate bank-write workflow still requires destination semantics,
readback/restore design and explicitly authorized disposable-slot testing.

## Edit-buffer sender restricted — 25 October 2026

The former generic file sender now accepts only the strict known edit-buffer
command, with a bounded file read. Banks, stored/bank-shaped single messages,
other commands and malformed/trailing data are rejected without sending or
changing preset identity. Native virtual-MIDI tests, builds, signature checks,
installation and Apple AU validation passed. See
[file-send behavior and evidence](edit-buffer-file-send.md). This supersedes the
historical unrestricted-sender source finding above. It adds no slot restoration
or bank-writing feature and changes no observed bank header interpretation.
