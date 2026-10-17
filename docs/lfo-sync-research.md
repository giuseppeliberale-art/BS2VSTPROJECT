# LFO Speed/Sync hardware evidence

Captured on the user's Bass Station II firmware 4.14, MIDI channel 1, 2026-10-18.
Normal unsynchronized LFO speeds were already mapped and tested; these are separate
mode and clock-division settings. This document records discovery, not completed
plugin integration or audible clock-sync validation.

## Incoming live MIDI

| Setting | NRPN | Values |
|---|---|---|
| LFO 1 Speed/Sync | 0:88 | SPd=0, Snc=1 |
| LFO 2 Speed/Sync | 0:92 | SPd=0, Snc=1 |
| LFO 1 sync division | 0:87 | 0–34, all 35 captured |
| LFO 2 sync division | 0:91 | 0–34, all 35 captured |

Local incoming-only capture evidence (ignored output directories):

- LFO 1 mode: `midi-lab-results/20261018-032451-record-b8b2a2/events.jsonl`, values 0 then 1.
- LFO 2 mode: `midi-lab-results/20261018-033816-record-eb153f/events.jsonl`, values 0 then 1.
- LFO 1 division: `midi-lab-results/20261018-033304-record-390a7d/events.jsonl`, uninterrupted 0–34 sweep.
- LFO 2 division: `midi-lab-results/20261018-033425-record-992a5f/events.jsonl`, 1–34 including harmless 30→29→30 reversal; `20261018-033518-record-8e250d/events.jsonl`, reverse sweep to 0.

The cached official manual (`docs/vendor-cache/bsii-4.14.txt`, LFO Speed/Sync
section and Sync Values table) describes 35 divisions and the lower A/A-sharp
on-key selectors. Its MIDI table ambiguously repeats division addresses under
Speed/Sync. The hardware captures establish distinct selector addresses 88/92.
Plugin labels follow the official table's display-meaning column in that order:
quarter-note beats for long periods, then normal/dotted/triplet note divisions.
The table contains inconsistent musical-description/tick entries (for example
42 beats versus 1002 ticks); no numeric timing calculation was inferred from
those entries. Audible hardware timing has not yet been independently measured.

## SysEx discovery

Offsets below are zero-based in the complete 154-byte message.

| Setting | Decoding expression | Verified input values |
|---|---|---|
| LFO 1 mode | `(b[69] & 0x08) >> 3` | 0, 1 |
| LFO 2 mode | `(b[76] & 0x10) >> 4` | 0, 1 |
| LFO 1 division | `((b[67] & 0x1F) << 3) \| ((b[68] & 0x70) >> 4)` | 0–34 |
| LFO 2 division | `((b[74] & 0x3F) << 2) \| ((b[75] & 0x60) >> 5)` | 0–34 |

Hardware-only mode changes were dumped in each direction first. LFO 1 changed
only byte 69 bit 3; LFO 2 changed only byte 76 bit 4. Reverting each restored the
entire original dump byte for byte. Hardware endpoint comparisons located each
division field before automated testing.

`tools/LFOSyncSampler.swift` then sent only those four confirmed NRPN routes,
captured all 74 settings, compared decoded values with sent values, and checked
that no bits outside the selected field changed between samples. All passed,
with no timeouts. The complete starting patch was restored and read back exactly.
This verifies MIDI reception and stored patch representation; it does not verify
audible rates, external clock handling, GUI movement, or plugin automation.

Retained evidence: `tests/lfo-sync-fixtures/`, with `before.syx`, `after.syx`,
74 individual dumps, and `samples.csv`. Original run:
`midi-lab-results/lfo-sync-discovery-20261018-0350/`.

To rerun, stop DAW playback/automation, leave hardware controls untouched, and use
`swift tools/LFOSyncSampler.swift NEW_OUTPUT_DIRECTORY`. The routine requires
exactly one BSII input/output and uses MIDI channel 1. It refuses an existing
output directory and saves a backup before changing parameters. It attempts full
restoration after caught errors; a killed process or disconnected device can
require manual restoration from `before.syx`. It never sends a preset-save command.

## Plugin integration

Appended `lfo1Sync`, `lfo2Sync`, `lfo1Division`, `lfo2Division` after the existing
82 parameters, preserving all old indices, IDs and ranges. The LFO page exposes
all four as choices, with separate normal Speed controls retained. The existing
generic send/receive and session-state paths handle these parameters. Send All
includes them. Get Patch applies their verified decoded values with the same
host-silent behavior as other patch fields.

Offline validation: all seven CTest groups passed, including all 74 captured
patches replayed through the actual app decoder and incoming-route/label checks.
Native integration, plugin build/install and manual acceptance are recorded below
as completed; manual live sync and audible clock behavior remain pending.

### Build and installation — 2026-10-18

Release Standalone/AU/VST3 builds passed. Seven offline CTest groups passed,
including 17 MIDI Lab tests. Native processor integration passed: new NRPN
output, incoming values without echo, session persistence, stable parameter
indices, and host-silent patch refresh. The direct native SysEx regression
passed with an LFO division capture. Swift sampler compile-only check passed.

User saved the Set and quit Ableton before installation. Updated AU/VST3 were
installed in the user Library; strict/deep signatures and Apple AU validation
passed. Prior bundles and validation logs are retained under
`validation-results/20261018-034847-10341/`. An overlapping build initially hit
Xcode's build database lock; the sequential retry passed. No install occurred
before the passing build and integration checks. Manual Ableton acceptance
of the new controls is still pending.
