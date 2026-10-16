# Review of user live MIDI checklist — 2026-10-17

Source: `docs/manual-test-checklist.md`, user-completed notes and checkboxes.
Preserve that file as the original report. These are user observations, not independent
reproductions. Accept spaced ticks such as `[ x]` / `[x ]` as intended checkmarks.
Arp Swing send-pass has `[ ]x`; treat as an apparent intended pass, flagged here.

## Priority retests

1. Osc 2 LFO2→PW: explicit hardware→app failure; app→hardware passed.
   Reproduce with Osc 2 selected and PWM target LFO2; capture incoming MIDI.
2. Amp / Mod Trigger: inconclusive, including hardware selector and LED behavior.
   User observed LED Single when switching from Amp to Mod, despite a prior Multi
   setting. Do not infer stored values from the LED alone. Compare Amp, Mod,
   Amp+Mod with MIDI traces and independent patch readbacks, then audible behavior.
3. Bend Amount: both directions marked Inconclusive; note says it works but
   hardware supports -24…+24 while app is restricted. Recorded app source range
   is +1…+12 (user described 0…12). Verify signed full range and display before
   expanding parameter behavior.

## Interface requests from this pass

- Rename Portamento to Portamento / Glide Time.
- Osc Sync: use an Off / On switch instead of a raw numeric knob.
- Bend Amount: clear signed numeric control, with full hardware range after validation.
- Velocity→Amp/Mod, ModWh→LFO1 Pitch/Osc2 Pitch, Aftertouch→Filter/LFO1/LFO2 Speed:
  show meaningful signed values rather than raw MIDI 0…127. User reported -63…63
  for velocity and -64…63 for wheel/aftertouch. Check actual endpoint/neutral
  encodings per parameter; do not apply one generic conversion without evidence.

## Remaining observations

Most entries are marked passed in both directions, including Filter Tracking and
extended envelope retrigger/count/fixed-sustain settings. Patch Volume is marked
passed app→hardware only, as intended by the checklist. Keep exact per-control
results in the original file; do not promote this to exhaustive validation or
assume any of the ambiguous failures above have been resolved.

## Hardware instruction corrections

User has firmware 4.14 and an AFX printed overlay. User identified Filter Tracking
as 'F: FiltTrk' under Modwheel → Filter Freq on the FIRST C key, accessed with
Function + two presses. Earlier checklist instructions specifying lower E were
wrong for this hardware. Filter Tracking subsequently marked passed.
Some other verbal key locations were unreliable. Before further guided tests,
use the actual overlay labels / a verified diagram instead of guessed key positions.

## User clarification

Noise level and Osc Sync work as intended; user confirmed both entries should pass.
Checklist updated to Pass in both directions. Osc Sync binary-switch request remains.

## Osc 2 LFO2→PW guided retest

User selected Osc 2 and PWM source LFO2, then swept the hardware PW knob.
App followed correctly. Read-only capture recorded 183 CC78 messages, values 0–127.
Evidence: chat workspace `work/midi-debug/osc2-lfo2-pw-live.txt`.
Checklist updated to receive Pass with initial failure retained in notes.
No receive defect reproduced and no implementation change made.

## Trigger retest: hardware Env Select = Amp

User confirmed Amp-only selection. Switching hardware Triggering to Multi changed
both app trigger selectors. A subsequent captured switch to Single sent both:
`B0 63 00 B0 62 49 B0 06 01` (NRPN 0:73, Amp) and
`B0 63 00 B0 62 69 B0 06 01` (NRPN 0:105, Mod).
Thus the hardware itself transmitted both trigger updates in this attempt; this
observation does not establish whether independent incoming MIDI settings persist
or how all Env Select positions affect internal storage. No code change.
Capture: chat workspace `work/midi-debug/amp-trigger-live.txt`.

## Trigger retest: hardware Env Select = Mod

User selected Mod-only, then changed Single→Multi. Capture again contains both:
`B0 63 00 B0 62 49 B0 06 00` (Amp, NRPN 0:73) and
`B0 63 00 B0 62 69 B0 06 00` (Mod, NRPN 0:105).
Together with the Amp-selected retest, this shows the physical Triggering switch
transmitted both parameters in both individual selector positions on this unit.
Amp+Mod and independent incoming edits / stored values remain to be checked.
Capture: chat workspace `work/midi-debug/mod-trigger-live.txt`.

## Trigger retest: hardware Env Select = Amp+Mod

User selected Amp+Mod, then changed Multi→Single. Capture contains both
NRPN 0:73 = 1 and NRPN 0:105 = 1, as in the Amp-selected Single test.
All three hardware selector positions transmitted both trigger settings during
these checks. The shared app receive update follows hardware output; no receive
bug demonstrated. Independent app→hardware trigger settings still require separate
stored-value and audible verification. Do not merge the app parameters based only
on the physical switch behavior.
Capture: chat workspace `work/midi-debug/amp-mod-trigger-live.txt`.

## Independent trigger app→hardware readback, first direction

After the paired Single hardware test, user set Amp Trigger to Multi in the app,
leaving Mod Single. Read-only patch request returned Amp wire 0 (Multi) and Mod
wire 1 (Single), extracted from byte55 mask06 and byte62 mask0C. This confirms
independent stored values for this combination despite the hardware switch sending
both NRPNs. Audible independence and the opposite combination remain unchecked.
Capture: chat workspace `work/midi-debug/trigger-independent-amp-multi/baseline.syx`.

## Independent Mod edit readback

User then changed only Mod to Autoglide in the app. Fresh patch readback returned
Amp wire 0 (Multi), Mod wire 2 (Autoglide). Both individual app edits now have
hardware storage confirmation. Separate audible triggering behavior is not yet
verified. Conclusion: retain independent app controls; physical switch sends both
settings regardless of Env Select on this tested unit. No trigger routing fix needed.
Capture: chat workspace `work/midi-debug/trigger-independent-mod-autoglide/baseline.syx`.

## Simple listening patch prepared

At user's request, saved current hardware edit buffer to chat workspace
work/midi-debug/simple-envelope-test/original.syx before modifying it. Prepared
single Osc1 saw, other mixer sources muted, no pitch/PW/filter modulation, glide
and effects off; Amp long attack/full sustain, loop/fixed duration disabled.
Amp Single / Mod Multi confirmed in hardware readback simple-test.syx. No hardware
preset save sent. Mod envelope not yet made audible; first isolate Amp legato test.
Editor may require Get Patch to refresh. Restore original.syx when testing finished
if user wants the prior sound; do not confuse it with older checkpoints.

## Audible test: Amp Single / Mod Multi

Simple test patch: user heard Amp volume remain steady on an overlapping higher
note (long attack did not restart). Added Osc1 ModEnv pitch depth CC71=72, with
Mod Attack0/Decay75/Sustain0/Release0. Readback confirmed Amp Single / Mod Multi.
User reports downward pitch sweep on first fresh note, but NO new pitch sweep on
second higher note while first remains held. Thus this combination did not exhibit
independent audible triggering in the test. Do not claim independent engine behavior
from separate MIDI addresses or stored SysEx fields. Opposite combination still
needs testing. Amp Attack was subsequently requested at0 to expose pitch onset;
user confirmed first-note sweep vs absent overlapping-note sweep.

## Audible opposite combination: Amp Multi / Mod Single

Fresh hardware readback confirmed Amp wire0 (Multi), Mod wire1 (Single).
User held a note until pitch settled, then pressed a higher overlapping note and
reported YES: downward ModEnv pitch sweep restarted. Evidence readback:
chat work/midi-debug/trigger-audio-amp-multi-mod-single/baseline.syx.

Combined listening observations on this firmware4.14 unit / simple test patch:
- Amp Single / Mod Multi: Mod pitch envelope did not restart on overlapping note.
- Amp Multi / Mod Single: Mod pitch envelope did restart on overlapping note.
This supports the hypothesis that Amp Trigger governs ModEnv legato restart in
this context. It does not prove behavior of every mode, firmware or modulation
route. Separate stored fields are NOT proof of independent audible modes. Earlier
claim that independent engine triggering was established is withdrawn. Do not
merge controls or patch encodings without further evidence/manufacturer guidance.

## Autoglide listening check

User set both app Trigger modes to Autoglide and hardware Glide Time about halfway.
Initial slide report was confounded by the temporary ModEnv pitch sweep. Removed
that sweep (Osc1 CC71=64) and read back Amp wire2 / Mod wire2. With all keys
released and a one-second gap, user reported no slide on either separate note.
With the first low key held while pressing a higher key, user reported glide.
Both-Autoglide audible behavior passes this comparison on the simple test patch.
Readback: chat work/midi-debug/simple-envelope-test/autoglide-neutral-pitch.syx.
This does not establish independent Autoglide behavior in mixed mode combinations.
Amp+Mod isolated hardware capture/readback remains to complete.

## Final isolated Amp+Mod check

Editor closed; user selected Amp+Mod and moved Triggering to Single. Passive
capture recorded NRPN0:73=1 and0:105=1. First readback attempt could not find
exactly one BSII endpoint pair; retry succeeded, confirming both stored modes
Single. No persistent disconnect diagnosed. Capture stopped. Evidence in chat
work/midi-debug/trigger-app-closed-both-final.txt and
trigger-app-closed-both-single/baseline.syx.

Both-Autoglide and Amp+Mod checks pass for the tested behaviors. Mixed trigger
mode audible independence remains contradicted by the listening comparisons.
No manufacturer intent or universal firmware conclusion established.

## Requested linked Trigger UI implemented and checked

Changing Trigger from either app envelope page now updates both selectors and sends
both NRPNs. UI badge: Shared: Amp + Mod. Underlying IDs / snapshot and SysEx fields
remain separate; patch reception can preserve differing values until a user edit.
Live UI QA caught an initial positional control lookup error; fixed by matching each
control's explicit choiceIndex. Final build checked Amp→Single reflected on Mod,
Mod→Multi reflected on Amp, Amp→Autoglide reflected on Mod. Fresh synth readbacks
confirmed Multi/Multi and Autoglide/Autoglide respectively (chat work/midi-debug/
linked-ui-multi and linked-ui-autoglide). Existing six offline regressions passed.
Temporary test patch restored byte-exact to trigger-app-closed-both-single baseline,
then editor refreshed: both selectors Single. This is the temporary test sound,
NOT the user's pre-listening original.syx backup, which remains preserved.
Latest app: /private/tmp/BSII-Control-linked-trigger-final/BSII Control.app.

## Bend test blocked by hardware display

Before changing Pitch Bend Range, user reported hardware LCD --- again.
Passive MIDI recorder stopped; it sends no MIDI or SysEx. Cause unestablished;
no malformed-message diagnosis. Pre-listening original.syx backup still present
with154-byte expected header/end framing. Pause transmission and recover front
panel before continuing. No negative hardware-origin bend capture completed yet.

## Display recovery without restart

User changed hardware preset and normal display returned; no restart needed.
User suggests --- may indicate inactive state. No official definition found in
focused search; do not label it inactive, dump mode, malformed SysEx or a crash.
A temporary/edit-buffer indication remains a hypothesis. Changing preset replaced
the test edit buffer; do not restore it over the user's newly selected sound.
Editor remains closed and MIDI capture stopped. Bend testing not resumed yet.

## Bend Amount completed

Implemented full -24…+24 signed range. Verified app→hardware -24,-1,0 by patch
readback, Get Patch applies -1, and user confirmed hardware→app live -1 on the
final signed-bend build. Six regressions pass including128 captured wire inputs.
Signed numeric display and negative update sentinel fixed. Test19 range/display
request addressed; current hardware/app Bend Amount -1 after user live check.

## Remaining presentation requests completed

- Velocity Amp/Mod now display -63…+63; the two mod-wheel and three aftertouch
  assignments display -64…+63. Stored raw parameter values / MIDI routes unchanged.
- User specifically confirmed Aftertouch LFO2 Speed hardware minimum -64 and
  maximum+63; passive capture recorded NRPN0:76 values0 and127 respectively.
  Trace: chat work/midi-debug/aftertouch-lfo2-speed-display-check.txt.
- Osc Sync is now an Off/On switch, retaining oscSyncRaw storage and CC110.
- Portamento label now reads Portamento / Glide Time; visually fits the grid.
- Checklist instructions corrected to printed labels and first-C Filter Tracking;
  original filled notes/ticks preserved.

Final-build UI signed-entry checks and hardware readback verified all seven routes:
VelocityAmp -20→44, VelocityMod -63→1, ModWhLFO1+63→127, ModWhOsc2 -64→0,
AftertouchFilter -20→44, AftertouchLFO1+20→84, AftertouchLFO2Speed -64→0.
Osc Sync On set stored flag; Off cleared it. Restored these controls through the UI
and compared entire154-byte patch: byte-identical to presentation-ui-start baseline.
No full-patch SysEx restore needed. Six regressions pass. New app opened at
/private/tmp/BSII-Control-panel-polish/BSII Control.app.

All actionable interface items from this test pass addressed. Remaining research:
intended mixed trigger semantics and unknown --- LCD indicator; new Arp/Seq scope
is separate. AU/VST3 host checks still not part of this standalone validation.

## Subsequent single-patch transfer

User sent a single patch through the app and confirmed it plays. Hardware showed
---; user initially reported knobs did not update LCD, then corrected that they do.
Do not retain the initial nonresponsive-LCD report as a confirmed symptom.
Official meaning of --- still unknown. No claim of malformed SysEx established.
