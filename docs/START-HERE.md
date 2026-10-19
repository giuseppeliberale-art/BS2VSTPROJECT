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
  Fresh Ableton acceptance of this installed version and hardware tuning remain
  separate checks. Public downloads were not replaced.
- Public prerelease `v0.8.2-preview.20261018` is an OLDER binary release.
  A source push does not replace its downloads. Publish new binaries only when
  requested and clearly state their compatibility and validation scope.

## Immediate next steps

1. Run `python3 -B tools/midi-lab/lab.py status` for compact, source-fingerprinted
   offline validation status. If stale, run `./tools/validate.sh` once.
2. The TUN v2 build/install increment is complete. Optionally reopen Ableton and
   check Import Tuning with a known file; compare preview/apply/undo. This requires
   no synth. No fresh hosted acceptance is claimed by the native tests.
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
