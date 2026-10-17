# Bass Station II Arp / Seq protocol research

Researched 2026-10-18 without sending MIDI or touching hardware. Read `docs/local-reference.md` and the cached official 4.14 manual first. Documentation establishes routes and musical meanings, but does **not** establish exact wire value encodings for the six new controls.

## Documented routes versus unknown encodings

| Control | Documented message | Musical values | Exact raw values |
|---|---|---|---|
| Arp On | CC 108 | Off / On | Unspecified; do not assume 0/127, 0/1, or a 64 threshold |
| Arp Latch | CC 109 | Off / On | Unspecified |
| Arp Octaves | CC 111 | 1–4 octaves; same physical selector selects SEQ slots in Play/Record | Unspecified; mode-dependent receive/storage behavior unverified |
| Arp Note Mode | CC 118 | Up, Down, UpDn, UpDn2, Played, Random, Record, Play | Unspecified; panel order does not prove 0–7 encoding |
| Arp Rhythm | CC 119 | Patterns 1–32 | Unspecified; do not assume wire equals display or display minus one |
| Seq Retrig | NRPN 106, represented in this project as 0:106 | Off / On | Unspecified; numeric NRPN 106 implies MSB 0, LSB 106, but data-entry encoding remains unverified |

The MIDI table prints `106` for Seq Retrig rather than `0:106`. These addresses, and the empty range cells, appear in the [official 4.14 PDF, page 24](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf#page=24). The [current official online appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix) retains the same six routes and blank ranges. No exact encodings were found in these primary sources.

## Tempo and clock

The documented internal tempo range is 40–240 BPM. Incoming external MIDI clock takes precedence automatically; moving the Tempo control then displays external BPM. After clock disappears, playback continues at the last clock rate until moving the knob returns control to the internal clock. These are documented musical behaviors, not newly observed firmware results. No Tempo CC or NRPN appears in the MIDI parameter list. This is absence of a documented route, not proof that an undocumented route cannot exist. See [official 4.14 PDF, page 18](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf#page=18).

Host clock output would be a separate transport feature requiring timing/start/stop behavior verification. It must not be presented as a confirmed direct parameter mapping.

## Sequence slots and steps

The instrument documents four persistent sequence memories, each holding up to 32 notes/rests; the selected sequence is also saved with a patch. The physical SEQ selector chooses memory 1–4 in Record/Play. Recording advances step position as notes/rests are entered; durations are supplied by Rhythm during playback. Legato and ties are panel recording operations. Seq Retrig On restarts every two bars; Off allows the sequence to finish across rhythm boundaries. See [official 4.14 PDF, page 19](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf#page=19).

No dedicated sequence-slot CC/NRPN, random-access step read/write command, step position message, rest/tie MIDI encoding, or sequence SysEx layout is specified in the reviewed table/manual. The shared physical Octaves/SEQ selector suggests CC 111 as a discovery candidate, but **does not prove** it selects sequence slots remotely. Patch inclusion does not establish sequence byte offsets. Do not implement guessed bit positions.

## External note routing caveat

Novation documents Function + Latch as the toggle between MIDI-to-Arp input and Arp-to-MIDI output. Input mode arpeggiates incoming notes; output mode sends generated notes. No remote address for this routing toggle is given in that article. See [Novation support: external arp control](https://support.novationmusic.com/hc/en-gb/articles/206861739-Control-Bass-Station-II-s-Arp-from-External-Controller).

## Evidence boundaries and next verification

No hardware observations were made during this research. Existing project notes explicitly classify these new addresses as discovery routes, not validated application controls (`docs/midi-lab.md`). Preserve captured raw messages separately from human labels.

The next useful evidence is an incoming-only capture of every physical state: On/Latch, all eight Mode positions, Octaves 1–4, all 32 Rhythms, both Seq Retrig states, and SEQ slots in Play versus Record. Establish emitted encodings first, then separately validate receive behavior and patch dump changes. Recording mode/slot changes may alter sequence state, so discovery should preserve the existing sound/sequence data. Tempo and sequence-step editing should remain explicitly unsupported until their routes are demonstrated.

## Hardware capture evidence — 2026-10-18

Incoming-only MIDI Lab captures on the user's firmware 4.14 unit, channel 1:

- `midi-lab-results/20261018-000928-record-9603f5/events.jsonl`: following the requested Direction sequence Up → Down → UpDn → UpDn2 → Played → Random, CC118 emitted 0, 1, 2, 3, 4, 5 respectively. An initial transient 3 preceded Up. Record and Play were deliberately not tested.
- `midi-lab-results/20261018-001103-record-ecf7a3/events.jsonl`: two Arp On presses emitted CC108 values 1 then 0; two Latch presses emitted CC109 values 1 then 0. This supports the emitted On=1 / Off=0 encoding; no separate LED-state confirmation was collected.

These captures establish emitted messages only. Remote receive behavior and SysEx storage remain unverified. Raw captures are local ignored artifacts; this note preserves the observed values.

### Arp octave capture

With Direction on Random, the requested octave sequence 1 → 2 → 3 → 4 emitted CC111 values 2, 3, 4 (`midi-lab-results/20261018-001301-record-984d0b/events.jsonl`). The initial 1 produced no message. A separate move from 4 back to 1 emitted 3, 2, 1 (`midi-lab-results/20261018-001441-record-97c767/events.jsonl`), confirming octave labels 1–4 match emitted raw values 1–4. This does not establish receive behavior or sequence-slot selection in Play/Record mode.

### Arp rhythm capture

Incoming-only capture `midi-lab-results/20261018-002823-record-c20f2f/events.jsonl`: hardware Rhythm moved fully anticlockwise to labelled pattern 1, then swept fully clockwise to labelled pattern 32. All CC119 values 0–31 were observed, with no gaps. This establishes emitted encoding raw = displayed pattern minus one. Remote receive and SysEx storage remain unverified.

### Seq Retrig capture

Initial capture `midi-lab-results/20261018-003026-record-99c112/events.jsonl` recorded NRPN 0:106 with Data Entry 0 only, so could not establish both states. Repeat capture `midi-lab-results/20261018-003218-record-d88fa8/events.jsonl` recorded Data Entry 1 then 0 for NRPN 0:106; the user explicitly confirmed seeing both On and Off while performing Off → On → Off. Emitted encoding is therefore Off=0 / On=1. Incidental CC1 messages were unrelated. Remote receive and SysEx storage remain unverified.

### Record / Play capture

Capture `midi-lab-results/20261018-003407-record-e5f199/events.jsonl` recorded CC118 values 6, 7, 6, 5 during Random → Record → Play → Random. Intermediate Record was crossed while returning. The raw values are retained, but these initial instructions used the wrong physical Play/Record order; this capture alone did not establish their labels. See corrected capture below. No notes or Rest/Legato actions were requested.

## Implementation status

Six live-only choices appended after the existing 76 host parameters, preserving their indices, IDs and ranges. New Arp / Seq page includes existing Swing (unchanged ID/range), On, Latch, Octaves, Direction / Seq Mode, Rhythm and Seq Retrig. All six new controls support the existing automation, MIDI reception and saved-session parameter paths. Hardware receive checks are recorded below; these do not establish SysEx storage. Get Patch does not populate these six controls: no SysEx offsets have been guessed. Send All deliberately skips them to avoid replaying stale mode/latch values or arming Record during bulk patch recall. Their values remain in DAW state and parameter snapshots; captured raw sound recall remains lossless, subject to the instrument's actual patch contents. Tempo, step editing and remote sequence-slot selection remain unimplemented/unverified.

### Corrected Direction labels

The user corrected the physical clockwise order to Up, Dn, UpDn1, UpDn2, Played, Random, Play, Record. Fresh capture `midi-lab-results/20261018-003727-record-59276e/events.jsonl` requested Random → Play → Record and recorded CC118 6 then 7, with explicit user confirmation of the printed labels. Thus Play=6 and Record=7; the provisional reversed mapping was corrected before building/installing. The JSONL choice strings in this capture were generated from that provisional registry and are wrong; the raw bytes and user-labelled action sequence are the evidence. Earlier UpDn was relabelled UpDn1 to match the hardware.

## Local validation and installed build

2026-10-18: standalone/AU/VST3 Release builds PASS. Seven registered offline CTest groups PASS, including all 13 MIDI Lab checks. Native processor integration PASS with added Arp/Seq CC/NRPN output, incoming editor/APVTS updates without MIDI echo, saved values and stable pre-existing host indices. Native SysEx input regression PASS. Installed AU/VST3 signatures verified and AU validator PASS; previous bundles retained locally in `validation-results/20261018-003852-4230/before/`. No hardware values were sent by these virtual-port checks. User closed Ableton before installation. Subsequent manual Ableton/hardware checks are recorded below.

## Manual Ableton / hardware checks — 2026-10-18

User-reported checks on the installed build from commit 680c7f9:

| Control | Hardware → app | App → hardware |
|---|---|---|
| Rhythm | Full 1–32 sweep followed correctly after startup hiccup below | Changing 1 to 32 audibly changed the running arp |
| Octaves | Hardware 1 reflected as app 1 | App 1 to 4 audibly expanded the arpeggio span |
| Direction | Hardware Random reflected as app Random | Up to Down audibly changed ascending to descending |
| Arp On | Hardware On reflected as app On | App Off stopped the repeating arp |
| Latch | Hardware Off reflected as app Off, arp stopped | App On sustained the arp after releasing keys |
| Seq Retrig | Hardware Off and On both reflected correctly | App Off and On both confirmed by reopening the hardware Function setting without changing its value |

Do not describe the untested reverse transitions or all eight Direction modes as manually verified. Play/Record emitted encodings are captured, but their receive/sequence behavior is still untested.

Initial Rhythm check failed: hardware at 32, app at 1, CC RX reportedly waiting. Hardware Cutoff then worked; user reported Rhythm began following after moving Cutoff. No root cause established. Cold-start retest pending; this must not be classified as fixed.

Feedback correction: incoming MIDI does not automatically make the hardware LCD display the edited parameter. Rhythm, octave span and direction sends were checked audibly. Seq Retrig was checked by explicitly reopening its on-key settings page after each app edit.
