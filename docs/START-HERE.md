# BSII Control — start here

Updated 20 October 2026. This repository is the durable project record. Read this
file and the linked checklist before continuing; do not reconstruct the project
from the old chat. Verify the current checkout and validation status first.

## Location and current state

- Local checkout: `/Users/giuseppeliberale/Documents/Codex/BS2VSTPROJECT`.
- GitHub: https://github.com/rydeen-hrmntcs/BS2VSTPROJECT
- JUCE 9.0.4: `build/_deps/juce-src` (downloaded by CMake).
- [Remaining work and hardware acceptance](remaining-work.md) is the detailed
  checklist. Its dated validation sections are historical, not current evidence.
- Current source includes local MicroTune editing, JSON persistence, bulk editing,
  undo/redo, Scala SCL/KBM, numeric TUN and TUN v2 import. Tuning-table hardware
  transfer/readback/persistent save are still pending.
- [TUN v2 scope and limits](tun-v2-import.md): formulas and keyboard mapping are
  flattened to 128 pitches. Muted/unmapped keys require explicit preservation;
  the editor does not implement note muting. JSON remains supported.
- Fresh offline validation passed all nine groups on 20 October 2026. Local report:
  `validation-results/20261020-030552-19210/summary.txt`. This covers the standalone
  build and offline regressions, not hardware or hosted acceptance.
- TUN v2 AU/VST3 builds and installation completed on 20 October 2026.
  Both native test programs passed: MicroTune preview/apply/undo/JSON/host state
  and processor automation/routing. Local evidence: `validation-results/tun-v2-final/`.
  The preview screenshot was inspected. Both MIT notices match the vendored
  licenses. Installed bundles passed strict local ad-hoc signature verification;
  Apple AU validation passed. Prior bundles are in that report's `installed-before/`.
  The user subsequently confirmed TUN v2 preview, Apply, Undo and Redo in Ableton
  with the A4-plus-50-cents fixture. DAW reopen persistence and hardware tuning
  remain separate checks. Public downloads were not replaced.
- A later source-only recall wording fix reports commands as sent rather than
  claiming hardware restoration. It awaits the next plugin installation. The
  full nine-group validation above predates this fix; do not treat its fingerprint
  as current. See `docs/project-sound-recall.md` for focused verification.
- Hardware reconnect checks on 20 October: captured sound returned byte-identical
  with the editor closed; Off did not restore that capture; hardware Cutoff changed
  during automation after Off reconnect. Audio was not connected. Recall was last
  saved Off. See `docs/project-sound-recall.md` for evidence and limits.
- Tuning transfer testing was deferred: the user needs tables 1–8 preserved and
  had no audio routing for pitch measurement. Table 0 was authorized for a later
  test and selected after power-cycle, but no tuning data was sent. Resume pitch
  measurements only once audio/tuner is available; hardware backup is unresolved.
- Global Tune isolation (20 October): the paired-data candidate did not establish
  negative receive support. Later manual-adjustment problems disappeared with
  Ableton closed, an empty Set, and the original Set's BS2 track set to MIDI From
  = No Input. This strongly implicates host-track MIDI echo interacting with
  asymmetric Tune send/receive encoding, not a proven persistent firmware fault.
  Keep that host input route disabled; the last requested resting Tune value is 0.
  No new probe commands were sent during isolation; the temporary sender was
  removed. Read `docs/global-control-research.md` before further work. Hardware
  recording needs a separate route that does not return these messages to BS2.
- Public prerelease `v0.8.2-preview.20261018` is an OLDER binary release.
  A source push does not replace its downloads. Publish new binaries only when
  requested and clearly state their compatibility and validation scope.

## Immediate next steps

1. Run `python3 -B tools/midi-lab/lab.py status` for compact, source-fingerprinted
   offline validation status. If stale, inspect the intervening changes and run
   relevant checks; use `./tools/validate.sh` when a full refresh is warranted.
2. The TUN v2 build/install increment and manual Ableton preview/apply/undo/redo
   check are complete. Manual DAW reopen persistence is not newly verified.
   Choose the next unfinished feature from remaining-work; hardware tasks need
   the synth.
3. With the synth available, follow the ordered hardware checks in remaining-work.
   Without it, continue only work that can be meaningfully validated offline.
4. For later installations, confirm Ableton has actually quit; never quit it for
   the user. Ask for save and Cmd+Q if needed, back up the bundles and verify them.

## Remaining feature areas

Global Tune receive encoding; Local Control; sequence slots/steps; tempo/clock
routing; MicroTune hardware transfer, readback and save; patch-bank management;
AFX after the other synth areas; GUI redesign after features settle. Wider host,
multiple-device, Intel/older macOS coverage and Developer ID/notarization remain
separate work. See the detailed checklist for evidence and unresolved questions.

## Decisions and working preferences

- Preserve the 95 published host parameter indices (Paraphonic is index 94).
  Local tuning documents are separate from automation parameters and ordinary
  patch data; local editing/import sends no hardware MIDI.
- Current builds target Apple Silicon/macOS 27+. Do not silently broaden support.
- External Instrument currently provides the Ableton notes/audio workflow.
  Standalone note forwarding/audio integration is optional future scope.
- GUI reference: `/Users/giuseppeliberale/Documents/Bass Station II Synth Editor.png`.
- The synth was unavailable during the current offline work. Do not claim hardware
  acceptance from an offline build, virtual MIDI test or documentation.
- Keep token use low: short updates, compact status output, targeted file reads,
  batch independent checks, and let the user operate Ableton for manual tests.
  Avoid repeated full audits, screenshots and tests without a concrete reason.
- Source changes and routine GitHub updates are authorized in this workstream;
  keep release assets separate. Respect current tool/sandbox approval requirements.
- Synced ChatGPT-project `sources/` files are read-only reference material.

## Suggested first message in a new chat

“Continue BSII Control in /Users/giuseppeliberale/Documents/Codex/BS2VSTPROJECT.
Read docs/START-HERE.md and docs/remaining-work.md, check Git status and compact
validation status, then tell me the next unfinished step. Keep token use low.”
