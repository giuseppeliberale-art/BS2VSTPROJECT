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
