# Project handover

Updated 2026-10-17. Read this first; use evidence links below for details.

## Current working state

- Repo: BS2VSTPROJECT, main. Manual-test fixes landed in776d44c; this update
  adds MIDI Lab and preset selection/sync. Original v0.8.2 tag stays immutable.
- Active host: Ableton Live12.4.6 with the installed user-library BSII Control VST3.
  Standalone is closed. The old label-fix temp app predates the direct SysEx fix;
  build output contains the updated standalone. Do not launch old copies for retests.
- Synth firmware: user reports4.14 with AFX overlay. Hardware now on preset23 after preset-sync validation. Read-only backup saved in
  `midi-lab-results/20261017-065533-backup-4c8a19/baseline.syx`.
- App Live RX running. MIDI recorder stopped. User prefers one physical instruction
  at a time, explicit hardware/app location, concise nontechnical communication.

## Completed manual-test fixes

- Single local `tools/validate.sh`: configures tests, builds standalone+tests,
  runs CTest, saves ignored validation-results logs; no MIDI/launch/install/commit.
- Manual checklist and reviewed user results in docs (links below).
- Trigger selectors linked: editing either page sets both app modes and sends both
  NRPNs; badges say Shared: Amp + Mod. Underlying IDs and incoming mixed patch
  values remain separate, preserving old data. Retrigger/ADSR stay independent.
- Bend Amount supports -24…+24; CC107 wire=semitones+64. Negative -1 no longer
  collides with missing-update/send-cache sentinels. Signed numeric UI verified.
- Velocity Amp/Mod display -63…+63; mod-wheel LFO1/Osc2 and aftertouch Filter/LFO1/
  LFO2 Speed display -64…+63. Display=input raw-64; MIDI/snapshot storage unchanged.
- Osc Sync highlighted Off/On switch, raw0/127 canonical; nonzero receive means On.
  Sub Octave still -1/-2. Portamento renamed Portamento / Glide Time.
- Checklist shortcuts corrected to overlay labels; Filter Tracking is Function +
  first C twice (F: FiltTrk), NOT the lower E instruction given earlier.

## Verification and unresolved observations

- All7 registered test groups pass (including12 MIDI Lab tests).96 root fixtures /9,251 captured patch messages cover76
  currently exposed controls. This is NOT complete instrument or acoustic coverage.
- User manually tested most controls both directions. Noise/Sync clarified as pass.
  Osc2 LFO2→PW receive retest passed with183 CC78 messages; earlier failure unreproduced.
- Linked Trigger UI tested from both pages and all3 modes; fresh hardware readbacks
  confirmed paired modes. Physical switch applies both stored modes in tested selector
  positions; it may transmit only changed values, so NOT always two messages.
- Separate trigger fields can store mixed values, but listening on a simple patch:
  Amp Single/Mod Multi did NOT restart Mod on overlapping note; Amp Multi/Mod Single
  DID. Apparent Amp-mode dominance in that context; manufacturer intent unconfirmed.
  Both-Autoglide correctly glides only for overlapping notes. Do not claim independent
  mixed-mode engine behavior or declare this unit faulty/normal on that evidence.
- Bend hardware -24/0/+24 → CC10740/64/88; user confirmed final-build live -1 updates.
  App signed entries and negative Get Patch checked;128-input fixture red before fix.
- All7 modulation signed numeric entries checked against hardware readback; Sync
  On/Off checked. Full current patch byte-identical after returning UI test edits.
- LCD --- occurred twice. Function/Exit did not clear it; selecting another preset
  cleared the latest event without reboot. A subsequent user patch transfer played
  correctly with --- displayed; user corrected their initial report and confirmed
  knobs did update the LCD. No documented meaning found; do not call
  it inactive/dump mode/malformed SysEx/crash. Cause remains unknown.
- Coarse display approximate; exact LCD rounding unverified. No acoustic measurements,
  full AU/VST3 host tests, or normalized automation migration checks (bend range changed).
  Disconnect/reconnect, malformed input and race conditions not comprehensively tested.

## Next scope

All actionable fixes from the completed manual pass are addressed. User may spot-check
new UI. Next: host automation with editor open/closed and session recall checks, then Arp/Seq. Missing also LFO speed/sync divisions,
mod-wheel direct filter and LFO2→filter routes, paraphonic mode, oscillator error,
independent sub tuning, glide divergence, globals/tuning. Verify remote support before
implementation. User explicitly wants AFX LAST. Preset work is now implemented below; remaining features have not started.

## Evidence and recovery

- `docs/manual-test-checklist.md`: user's original ticks/notes plus follow-up annotations.
- `docs/manual-test-review.md`: complete results and guided retests.
- `docs/trigger-behavior-research.md`: official citations, contradictions and observations.
- `docs/hardware-validation.md`, `docs/validation-coverage.md`: capture history/limits.
- `docs/state-integration-validation.md`: snapshot and Send All checks.
- `docs/next-version.md`: release changes; `Source/BSIIMidi.h`: implemented control list.
- Chat workspace `work/midi-debug/simple-envelope-test/original.syx`: original sound
  before envelope listening experiments. Do not restore over user's new preset casually.
- Chat `work/midi-debug/presentation-ui-start/baseline.syx`: latest UI-test starting
  patch, final comparison byte-identical. Earlier captures are dated by directory.
- Hardware tools `MIDISampler.swift` and `PatchCheckpoint.swift` can send/restore
  patches: inspect before use. Validation routine is offline and sends nothing.

## Local tool now available

`python3 tools/midi-lab/lab.py`: default passive recording, control recognition,
channel filter, raw preset events and short reports. Replays previous captures;
compares samples using actual C++ decoder, backed by shared BSIIPatchFields table.
Explicit backup/restore/sweep modes with per-run recovery; default sends nothing.
Native passive capture/interruption and read-only patch backup tested. Actual decoder
caught12/12 old Bend errors and accepted12/12 corrected samples. Active recovery
simulated in tests, not run against the user's current patch in this tool session.
Read docs/midi-lab.md for commands/limits. Official4.14 PDF and text cached locally
under ignored docs/vendor-cache; docs/local-reference.md indexes preset/arp/MIDI pages.
Generate current76-control registry via `lab.py reference`.

## Preset selection implemented and checked

- Added Preset0–127, Previous/Next/Load Preset; follows hardware Program Change
  on selected channel while Live RX runs. Reads current patch after200ms settling.
  One request in flight; superseded replies rejected by selection generation.
- Unknown at startup, snapshot/SysEx file load/send or output/channel change.
  No startup Program Change; host program API unchanged. No preset memory writes.
- User hardware preset1 captured as raw Program Change1 (channel1).
- Latest validation report: `validation-results/20261017-065628-45285/summary.txt`;
  all7 groups pass; preset state regressions extend StandaloneMidiRouting.
- New temp app signed/verified and running. App selection0 then Next→1 received
  automatic154-byte patch replies. Saved app snapshot compared against hardware
  preset1 backup: all76 fields match. Snapshot saved by user in Documents as
  `preset-one.bsiistate`.
- User selected HARDWARE preset2. App selector followed to2 and automatically
  received/decoded154-byte patch. Exposed values matched preset1. Both directions
  checked. Follow-up checks passed: paused RX ignored preset21; Get Patch still
  decoded while paused; app channel2 ignored hardware channel1 preset22; restoring
  app channel1 followed preset23 with automatic decode. Matching channels2–16 and
  induced late hardware replies remain untested. See docs/preset-validation.md.

## Label encoding fix

UI title/buttons/status strings with non-ASCII punctuation explicitly decode UTF-8.
Old build reproduced garbled ASCII interpretation; fresh signed standalone verified
all six affected button labels and title through full accessibility text and visual
inspection. Existing7 regression groups passed (20261017-170201-48080).
No MIDI was sent for this check; sound controls restored from app state on restart.
GUI redesign remains deferred until features are complete. Preset checks resumed
at the user's request and are now complete; recorder stopped, Live RX running.

## Early Ableton plugin check and SysEx fix

Ableton Live12.4.6 (Apple Silicon), VST3 installed under user Library/Audio/Plug-Ins.
AU installed too; initial and updated auval both passed.
VST3 loaded on MIDI track1 in the saved BSII VST3 Smoke Test Set under ignored
validation-results/20261017-ableton-smoke/. Standalone app is closed.
Live input/output Track enabled; Sync/Remote off. Hardware live CC worked, but
Get Patch initially timed out because reply reception depended on host MIDI.
Added processor-owned SysEx-only input with unambiguous output-name matching.
User confirmed fixed Get Patch; agent verified154-byte reply and paused-RX Get
Patch. Live RX restored running. Current synth sound was not restored over user
edits. This update contains source/tests/docs; installed VST3/AU contain the fix.
Build all3 formats and existing7 CTest groups pass; isolated virtual MIDI test passes.
Host-parameter/editor-lifetime failure reproduced and fixed: processor now owns
MIDI output and dispatches APVTS changes independently of the editor. Native
processor regression passed, including silent receive/recall and editor recreation.
Ableton hardware retest passed: editor closed, host Resonance30, patch readback30.
Restored0 while still closed; all decoded fields match the pre-test backup.
Updated Standalone/VST3/AU build, seven offline groups and AU validation passed.
Installed VST3/AU updated, backups retained. Test Set saved with Resonance0.
Next: recorded automation playback and full session/connection/hardware recall.
Port selection still initially needs the editor; connection persistence across
reloading a Set is not implemented. Hardware live-state application and automatic
preset refresh still involve the editor.
See docs/plugin-smoke-test.md. Do not claim full AU/VST3 compatibility yet.


## Saved MIDI connection (follow-up)

User reports Cutoff/Resonance automation plus External Instrument notes working,
including after reopening a Set once the editor had been opened once. Automatic
connection was absent before that first editor open. Added host-state persistence
for the selected output, channel and Live RX, restored from the processor timer.
Legacy Sets need one save with the updated build after choosing their MIDI output.
Pending: manual restart test of the updated build with editor kept closed.


## Preset refresh and DAW automation

User confirmed automatic saved-connection restart worked in Ableton. Subsequent
plugin preset selection disabled automation. Native editor-response regression
reproduced 76 manual-edit notifications during patch reception; fixed to zero.
Received patches update APVTS and views without manual DAW edit notifications;
ordinary physical live edits and explicit UI/import edits retain notifications.
Parameter IDs, ordering, ranges and discrete choice labels are preserved. Known
preset identity is now processor-owned across editor recreation, provided the DAW
actually forwards Program Change. Seven offline groups, native integration,
all format builds and installed AU validation pass. User's Ableton preset-change
playback retest is pending; flat automation segments are not yet verified.
