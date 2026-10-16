# Project handover

Updated 2026-10-17. Read this first; use evidence links below for details.

## Current working state

- Repo: BS2VSTPROJECT, main; prior baseline d286567.
  This handover accompanies the completed manual-test fixes. Original v0.8.2 tag stays immutable.
- LATEST RUNNING APP: `/private/tmp/BSII-Control-panel-polish/BSII Control.app`.
  Temporary location may disappear after cleanup/reboot. Build alone does not update it.
  Older Downloads app / earlier temp copies must not be launched alongside it.
- Synth firmware: user reports4.14 with AFX overlay. Current user-selected preset
  retained; temporary envelope test sound no longer loaded. Bend Amount -1;
  Aftertouch LFO2 Speed+63; panel test edits returned to their starting values.
- App Live RX running. MIDI recorder stopped. User prefers one physical instruction
  at a time, explicit hardware/app location, concise nontechnical communication.

## Implemented since the last pushed commit

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

- All6 regressions pass.96 root fixtures /9,251 captured patch messages cover76
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
new UI. Next regular feature area: Arp/Seq. Missing also LFO speed/sync divisions,
mod-wheel direct filter and LFO2→filter routes, paraphonic mode, oscillator error,
independent sub tuning, glide divergence, globals/tuning. Verify remote support before
implementation. User explicitly wants AFX LAST. No feature work on these started.

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
