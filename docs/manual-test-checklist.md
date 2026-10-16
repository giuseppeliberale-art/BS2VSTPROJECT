# Manual live MIDI checklist

Prepared 2026-10-17 against the current 76 implemented controls.
These are proposed checks, not recorded passes. AFX is deferred.

## How to use this list

- Use only the updated app, with Live RX running and the correct MIDI connection.
- Save your working sound before starting. Keep listening volume comfortable.
- For each listed control, first change it on the HARDWARE and watch the APP follow.
  This is a visual receive check (V) and applies to every hardware-accessible entry.
- Then change it in the APP and check the HARDWARE sound (A), or its indication
  where available. A physical knob or switch will not move in response to MIDI.
- The hardware LCD has not reliably refreshed for incoming MIDI during our tests.
  An unchanged LCD alone does not prove a send failure.
- If a listening comparison is inconclusive, mark it inconclusive. Later use Get Patch
  for a separate stored-value check (R). R is not a live-reception pass.
- For continuous controls try low, middle and high; for bipolar controls include both
  sides and neutral; for choices try every option. Exact knob positioning is unnecessary.
- Change one item at a time. Keep separate results for hardware→app and app→hardware.
- Velocity, mod-wheel and aftertouch assignment amounts now show signed values.
  Bend Amount shows signed semitones; Coarse remains approximate. Report any mismatch.

## Ordered tests: normal panel controls (53 parameters)

All rows include V for hardware→app. The final column describes the app→hardware check.
Do each control within a row separately, and each oscillator/envelope separately.

| Order | Controls | Hardware access | Send check / listening setup |
|---|---|---|---|
| 1 | Osc 1 level; Osc 2 level; Sub level; Noise level; Ring Mod level; External level (6) | Mixer knobs; select Noise/Ring/Ext for the shared knob | A: each source level changes. Ring needs both oscillators; External needs an audio input. Mark External untested if unavailable. |
| 2 | Osc 1 Waveform and Range (2) | Select Osc 1; waveform/range buttons | A: timbre and octave change. Hardware indicators are an additional visual check if they follow incoming MIDI. |
| 3 | Osc 1 Coarse and Fine (2) | Select Osc 1; tuning knobs | A: pitch moves each way and returns near neutral. |
| 4 | Osc 2 Waveform, Range, Coarse and Fine (4) | Select Osc 2; same controls | A: isolate Osc 2 in the mixer and repeat. |
| 5 | Sub Wave and Sub Octave (2) | Sub buttons | A: isolate Sub; compare timbre and -1/-2 pitch. V: correct highlighted switch/choice. |
| 6 | Filter Type, Shape and Slope (3) | Filter switches | A: tonal response; check Shape/Slope with Classic selected. Acid fixes these to LP/24 dB, so they are not independent there. |
| 7 | Filter Cutoff, Resonance and Overdrive (3) | Filter knobs | A: use a harmonically rich sound. |
| 8 | Amp Attack, Decay, Sustain and Release (4) | Env Select = Amp; ADSR sliders | A: repeated notes and held notes. Decay needs Sustain below maximum; Release needs audible level at note release. |
| 9 | Mod Attack, Decay, Sustain and Release (4) | Env Select = Mod; same sliders | A: give Mod Env an audible destination, such as filter or pitch. |
| 10 | Shared Trigger, shown on Amp and Mod pages (2 stored fields) | Trigger switch (applies shared mode regardless of Env Select in our tests) | A: compare separate and overlapping notes for Single/Multi; Autoglide needs nonzero glide. |
| 11 | Portamento (1) | Glide Time knob | A: alternate different notes with a trigger mode that permits glide. |
| 12 | LFO1 Wave, Speed and Delay (3) | LFO1 controls; select Speed or Delay | A: add LFO1→Pitch, use free Speed mode and retrigger notes for Delay. |
| 13 | LFO2 Wave, Speed and Delay (3) | LFO2 controls; select Speed or Delay | A: add LFO2→Filter, with cutoff in an audible range. |
| 14 | Osc 1 Mod Env→Pitch and LFO1→Pitch; repeat for Osc 2 (4) | Osc Select plus modulation knobs | A: isolate the oscillator being tested; use an active envelope/LFO. |
| 15 | Osc 1 Manual PW, Mod Env→PW and LFO2→PW; repeat for Osc 2 (6) | Pulse Width source selector plus PW knob | A: Pulse waveform; isolate that oscillator. Set the other PW amounts neutral when comparing. |
| 16 | Filter Mod Env and LFO2 amounts (2) | Filter modulation knobs | A: use an active envelope/LFO and moderate cutoff. |
| 17 | Distortion and Osc Filter Mod (2) | Effects knobs | A: timbre changes; Osc Filter Mod needs a suitable Osc 2 setting. |

## Ordered tests: Function + key settings (22 parameters)

For ordinary on-key settings, hold Function/Exit and press the labelled key, release
both, then adjust with Patch/Value. Function/Exit leaves the setting page.
Use the printed function names to avoid octave-name confusion. Repeated presses below
mean selecting the extended page while holding Function. Firmware support is required.
Hardware access follows [Novation's on-key instructions](https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail).

All rows require V first. A is the proposed listening check after editing from the app.

| Order | App control(s) | Hardware Function access | Send check |
|---|---|---|---|
| 18 | Osc Sync (raw CC) (1) | Osc 1-2 Sync; once | V: Sync LED, if refreshed. A: listen to Osc 2 while changing its tuning. Use the app Off/On switch; raw storage remains 0=Off, nonzero=On. |
| 19 | Bend Amount (1) | Pitch Bend Range; once | A: use pitch wheel after changing amount. App now supports the full -24…+24 range; negative reverses wheel direction, zero disables bend. |
| 20 | Filter Tracking (1) | Modwheel → Filter Freq, first C (overlay: F: FiltTrk); twice | A: compare low/high notes with a revealing filter setting. App 0=full tracking; 7=none. |
| 21 | LFO1 Key Sync; LFO2 Key Sync (2) | Keysync LFO1; Keysync LFO2 | A: repeat notes; compare repeatable LFO starts against free-running behavior. Use slow modulation with an audible destination. |
| 22 | LFO1 Slew; LFO2 Slew (2) | Slew LFO1; Slew LFO2 | A: use Square LFO; compare abrupt versus softened modulation. |
| 23 | Velocity→Amp; Velocity→Mod (2) | Amp Env; Mod Env; once | A: play soft/hard notes. Mod needs an audible Mod Env destination. |
| 24 | Amp Retrigger; Mod Retrigger (2) | Amp Env or Mod Env; twice | A: hold a note; use short envelope times and turn Fixed Sustain off. Mod needs an audible destination. |
| 25 | Amp Retrigger Count; Mod Retrigger Count (2) | Amp Env or Mod Env; three times | A: Retrigger on; try 1, 2, then 0 (continuous). Count envelope cycles. |
| 26 | Amp Fixed Sustain; Mod Fixed Sustain (2) | Amp Env or Mod Env; four times | A: hold a note; compare automatic progression to Release. Decay sets the fixed sustain duration. Test independently of retriggering. |
| 27 | ModWh→LFO1 Pitch (1) | LFO1 to Osc Pitch | A: move mod wheel with LFO1 active. The wheel itself need not appear as an app control. |
| 28 | ModWh→Osc2 Pitch (1) | Osc2 Pitch | A: isolate Osc 2, then move mod wheel. |
| 29 | Aftertouch→Filter (1) | Filter Freq; once | A: hold a note and add key pressure; use moderate cutoff. |
| 30 | Aftertouch→LFO1 (1) | LFO1 to Osc Pitch | A: add key pressure with LFO1 active. |
| 31 | Aftertouch→LFO2 Speed (1) | LFO2 Speed | A: pressure changes modulation speed; LFO2 needs an audible destination. |
| 32 | VCA Limit (1) | Limiter | A: compare level/compression on a suitable sound; an audio level meter helps. |
| 33 | Arp Swing (1) | Swing; once | A: enable arp on hardware; compare rhythmic feel. Other arp controls are not yet in the app. |

Extended envelope pages and tracking are described in the
[official guide, oscillator/filter/envelope sections](https://fael-downloads-prod.focusrite.com/customer/prod/downloads/bass_station_ii_user_guide_v5_en.pdf).

## Last check: app-only access in this checklist (1 parameter)

| Order | Control | Check |
|---|---|---|
| 34 | Patch Volume (1) | Change it in the APP; A: patch level changes. R: separate Get Patch readback. Do not use the physical Master Volume knob as an equivalent incoming control. No hardware→app pass is assumed here. |

Total: 53 panel + 22 Function-accessed + 1 Patch Volume = 76 implemented controls.

## Not in this test pass

Do not expect app updates for missing controls: Arp On/Latch/Mode/Octaves/Rhythm,
sequence selection/recording/retrigger; LFO Speed/Sync selection/divisions; paraphonic
mode, oscillator error, independent sub tuning, glide divergence, globals or AFX.
Mod-wheel direct Filter Freq and Mod-wheel LFO2→Filter are also absent. (The earlier
missing-features summary omitted the direct Filter Freq route.)
Hardware selectors that assign a shared knob are not themselves independent app sound
parameters. Select Osc 1/2, envelope or PWM target on hardware and check the corresponding
separate app control. Moving a wheel or pressing harder tests its modulation action;
it does not imply that the app contains a wheel/pressure indicator.

Instruction update: use the printed function labels on your AFX overlay. Earlier
note-position descriptions were unreliable and have been removed except the first-C
Filter Tracking shortcut explicitly verified on your hardware. Trigger edits from
either app envelope page now set both; Retrigger/Fixed Sustain/ADSR remain separate.

## Fill-in results — return this file when ready

Date: ____________________
App build / location: ____________________
BSII firmware (if known): ____________________
Patch / setup: ____________________

Tick **one result per direction**. Leave all boxes empty for untested items.
For uncertain results, tick Inconclusive and add a note. You can return a partly
completed checklist. Keep the instructions above with your results.

In a Markdown editor, change `[ ]` to `[x]` to tick a box (or click it if supported).
Use Notes for values, intermittent behavior, listening setup, or separate Get Patch
readback results. Do not count a readback result as a live MIDI pass.

### Test 1

**Osc 1 level**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**Osc 2 level**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Sub level**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _______works as intended_____________

**Noise level**

- [x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**Ring Mod level**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**External level**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 2

**Osc 1 Waveform**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Osc 1 Range**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___work as intended_________________

### Test 3

**Osc 1 Coarse**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _______works as intended_____________

**Osc 1 Fine**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

### Test 4

**Osc 2 Waveform**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _work as intended___________________

**Osc 2 Range**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

**Osc 2 Coarse**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

**Osc 2 Fine**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 5

**Sub Wave**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Sub Octave**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 6

**Filter Type**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**Filter Shape**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

**Filter Slope**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

### Test 7

**Filter Cutoff**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**Filter Resonance**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

**Filter Overdrive**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 8

**Amp Attack**

- [x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ________works as intended____________

**Amp Decay**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

**Amp Sustain**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

**Amp Release**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

### Test 9

**Mod Attack**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _______works as intended_____________

**Mod Decay**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Mod Sustain**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

**Mod Release**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 10

**Amp Trigger**

- [ ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ x] Hardware → app: Inconclusive
- [ ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ x] App → hardware: Inconclusive

Notes: __inconclusive, hardware interface unclear__________

**Mod Trigger**

- [ ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ x] App → hardware: Inconclusive

Notes: ___hardware interface unclear_________________

### Test 11

**Portamento**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended rename to Portamento/glide time on the interface_______________

### Test 12

**LFO1 Wave**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**LFO1 Speed**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**LFO1 Delay**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____________________

### Test 13

**LFO2 Wave**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**LFO2 Speed**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**LFO2 Delay**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 14

**Osc 1 Mod Env→Pitch**

- [x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Osc 1 LFO1→Pitch**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

**Osc 2 Mod Env→Pitch**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

**Osc 2 LFO1→Pitch**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

### Test 15

**Osc 1 Manual PW**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**Osc 1 Mod Env→PW**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**Osc 1 LFO2→PW**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Osc 2 Manual PW**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**Osc 2 Mod Env→PW**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

**Osc 2 LFO2→PW**

- [x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: Initial hardware→app failure reported. Guided retest passed: app followed hardware; capture recorded 183 CC78 messages spanning 0–127. Earlier failure not reproduced. App→hardware previously passed.

### Test 16

**Filter Mod Env**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Filter LFO2**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 17

**Distortion**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**Osc Filter Mod**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

### Test 18

**Osc Sync**

- [x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___it works , however the interface should show a binary switch, not a knob_________________

### Test 19

**Bend Amount**

- [x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __it works  however the interface should show a potentiometer like interface or a numeral,also_the pitch bend  on the interface goes from 0 to 12 , while the bend values i can set from the hardware go from -24 to 24_______________

Follow-up: full -24…+24 signed range implemented. App→hardware values verified by readback; user confirmed live hardware→app -1. Original notes above retained.

### Test 20

**Filter Tracking**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ______works as intended______________

### Test 21

**LFO1 Key Sync**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

**LFO2 Key Sync**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 22

**LFO1 Slew**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**LFO2 Slew**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 23

**Velocity→Amp**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _works, however the interface shows values from 0 to 127, the hardware from -63 to 63___________________

**Velocity→Mod**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works, however the interface shows values from 0 to 127, the hardware from -63 to 63______________

### Test 24

**Amp Retrigger**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _____works as intended_______________

**Mod Retrigger**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 25

**Amp Retrigger Count**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

**Mod Retrigger Count**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________

### Test 26

**Amp Fixed Sustain**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: __works as intended__________________

**Mod Fixed Sustain**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 27

**ModWh→LFO1 Pitch**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: _works, but app shows values from 0 to 127 , hardware ___-64 to 63________________

### Test 28

**ModWh→Osc2 Pitch**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: works, but app shows values from 0 to 127 , hardware ___-64 to 63________________
### Test 29

**Aftertouch→Filter**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: works, but app shows values from 0 to 127 , hardware ___-64 to 63________________

### Test 30

**Aftertouch→LFO1**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes:works, but app shows values from 0 to 127 , hardware ___-64 to 63________________

### Test 31

**Aftertouch→LFO2 Speed**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: works, but app shows values from 0 to 127 , hardware ___-64 to 63____________________

### Test 32

**VCA Limit**

- [ x] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ x] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 33

**Arp Swing**

- [x ] Hardware → app: Pass (visual)
- [ ] Hardware → app: Fail
- [ ] Hardware → app: Inconclusive
- [ ]x App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ___works as intended_________________

### Test 34

**Patch Volume**

Hardware → app: N/A in this checklist.

- [x ] App → hardware: Pass (audio / hardware indicator)
- [ ] App → hardware: Fail
- [ ] App → hardware: Inconclusive

Notes: ____works as intended________________


## Follow-up completion: presentation requests

The requested signed displays for velocity/mod-wheel/aftertouch, Osc Sync Off/On
switch and Portamento / Glide Time name are implemented. All seven signed numeric
entries were checked against hardware readback, and the test edits were returned
byte-exact to their starting patch state. Aftertouch LFO2 Speed -64/+63 endpoints
were confirmed directly by the user. Original observations above remain preserved.
