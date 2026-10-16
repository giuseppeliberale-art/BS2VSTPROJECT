# Hardware validation

## Live MIDI waveforms — 16 October 2026

Tested with the v0.8.2 standalone app built from repository baseline `91d1ffc` on macOS and a connected Novation Bass Station II. Results below were confirmed by the user operating the physical unit. Firmware version was not recorded.

| Control | Live MIDI mapping | Values tested | Hardware → app | App → hardware sound |
| --- | --- | --- | --- | --- |
| Osc 1 Waveform | NRPN 0:72 | 0 Sine, 1 Triangle, 2 Saw, 3 Pulse | Pass | Pass |
| Osc 2 Waveform | NRPN 0:82 | 0 Sine, 1 Triangle, 2 Saw, 3 Pulse | Pass | Pass |
| Sub Wave | CC 80 | 0 Sine, 1 Pulse, 2 Square | Pass | Pass |

Bass Station II MIDI input and output were enabled, channel was set to 1 in the app, and Pause Live RX was off. Hardware selectors were moved through their positions and the app followed; app selectors were then changed and the hardware sound followed.

These results justify the **MIDI verified** badges for these three controls. They do not validate SysEx patch decoding or stored-patch positions. No MIDI mappings, value ranges, or defaults were changed. The `v0.8.2` tag remains on the original baseline.

## Osc 1 Range SysEx — 16 October 2026

With Live RX paused, the user set only Osc 1 Range on the physical BSII to 16′, 8′, 4′, then 2′. Each Get Patch request returned 154 bytes. The extracted field at zero-based byte 20 (mask 0x07) and byte 21 (mask 0x78) was respectively 63, 64, 65, and 66. The editor requires choice indexes 0–3. Previously the decoder rejected every value and left the editor unchanged. The decoder now subtracts 63 and validates the resulting index.

The four raw captures are regression fixtures in `tests/fixtures/`. Osc 2 Range remains separately unverified.

Run the regression check:

```sh
cmake -B build -G Xcode -DBSII_BUILD_TESTS=ON
cmake --build build --config Release --target BSIIOsc1RangeTest
ctest --test-dir build -C Release --output-on-failure
```

After the fix, all four captured-patch regression cases passed. In the rebuilt app, importing the 16′ capture set Range to 16′; fetching the physical BSII still set to 2′ then changed Range from index 0 to 3 (2′), with Live RX paused. This verifies the editor application path as well as the decoder.

## Osc 2 Range SysEx — 16 October 2026

Controlled captures with only physical Osc 2 Range changed and Live RX paused confirmed 16′/8′/4′/2′ = 63/64/65/66. Extract the field from zero-based byte 26 (mask 0x1F) and byte 27 (mask 0x60), then subtract 63 for editor choice indexes 0–3. Previously the decoder rejected these values and the editor deliberately skipped applying Osc 2 Range. Both steps are now enabled using the hardware-verified mapping. The four `osc2-*.syx` captures join the Osc 1 regression fixtures.

All eight captured range cases passed after the Osc 2 fix. A fresh live fetch in the rebuilt app changed Osc 2 Range from 8′ to the hardware’s 2′ (choice index 1→3), with Live RX paused.

## Osc 1 tuning observations — 16 October 2026

With Live RX paused, the user moved physical Osc 1 Fine and Coarse through centre and both end stops. Each capture used Get Patch from BSII. Fine decoded as 128 at centre, 228 fully clockwise, and 27 fully anticlockwise. Coarse decoded as 127 at the user-reported centre, 255 fully clockwise, and 0 fully anticlockwise. The app visibly applied the fetched values. Six raw captures are preserved in `tests/fixtures/osc1-{fine,coarse}-{centre,max,min}.syx`.

These are observed raw editor values, not a claim about musical units. A repeat fetch after the user confirmed physical Coarse at displayed 0 again updated the app to raw 127 (from raw 0), while Fine stayed at 128. Thus hardware-displayed Coarse 0 was observed twice as raw 127; musical-unit display/conversion remains a separate concern. No tuning decoder changes were made.

## Osc 2 Fine observations — 16 October 2026

With Live RX paused, physical Osc 2 Fine was set to centre, fully clockwise, and fully anticlockwise. Get Patch decoded raw values 127, 228, and 27 respectively; the app visibly changed from 127 to 228 and then to 27. Three captures are preserved as `tests/fixtures/osc2-fine-{centre,max,min}.syx`. A repeat fetch with physical Osc 2 Fine returned to centre confirmed raw 127 again (app changed 27→127). The user clarified the hardware Fine display scale: fully anticlockwise −100, centre 0, fully clockwise +100. Accordingly the observed Osc 2 Fine pairs are −100→27, 0→127, +100→228. These observations do not change the decoder or establish musical-unit conversion.

## Osc 2 Coarse observations — 16 October 2026

With Live RX paused and Fine centred, the user confirmed hardware Coarse display 0, +12.0 fully clockwise, and −12.0 fully anticlockwise. Get Patch decoded raw values 128, 255, and 0 respectively. The app visibly updated 128→255→0 through SysEx. Captures are preserved as `tests/fixtures/osc2-coarse-{centre,max,min}.syx`. No tuning decoder changes were made; endpoint observations do not establish conversion at every intermediate value.

### Intermediate tuning check

With both oscillators initially centred on the hardware, the user set only Osc 1 Fine to displayed +50. Get Patch with Live RX paused changed Osc 1 Fine from raw 128 to 178. The returned patch also gave Osc 1 Coarse 128 and Osc 2 Coarse 127 at hardware-reported zero, demonstrating that displayed zero has been observed at adjacent raw midpoint values; exact display rounding/conversion is not yet established. Capture: `osc1-fine-plus50.syx`.

Osc 1 Fine at hardware-displayed −50 fetched as raw 77, changing the app from 178→77 with Live RX paused. Capture: `osc1-fine-minus50.syx`. Together with +50→178, this provides intermediate observations; precise rounding across the centre remains to be established.

Osc 1 Coarse at hardware-displayed +6.0 fetched as raw 193; Osc 1 Fine returned to hardware 0 fetched as raw 127. App changes were Coarse 128→193 and Fine 77→127 with Live RX paused. Capture: `osc1-coarse-plus6.syx`. Coarse display quantization must be checked before using a simple linear display conversion.

Osc 1 Coarse at hardware-displayed −6.0 fetched as raw 63, changing the app 193→63 with Live RX paused. Capture: `osc1-coarse-minus6.syx`. Intermediate Coarse observations are −6.0→63 and +6.0→193; these do not by themselves define the full quantization/rounding rule.

Osc 2 Fine at hardware-displayed +50 fetched as raw 178, with Osc 1 Fine returned to 0/raw 128. App changes were Osc 1 Fine 178→128 and Osc 2 Fine 127→178. Capture: `osc2-fine-plus50.syx`. A preceding attempt changed Osc 1 instead; it was identified and excluded from the Osc 2 fixtures.

Osc 2 Fine at hardware-displayed −50 fetched as raw 77 (app 178→77), matching the Osc 1 −50 observation. Live RX remained paused. Capture: `osc2-fine-minus50.syx`.

Osc 2 Coarse at hardware-displayed +6.0 fetched as raw 192 (app 127→192), with Fine restored to hardware 0/raw 128. Live RX remained paused. Capture: `osc2-coarse-plus6.syx`. Osc 1 +6.0 was observed at raw 193, so adjacent raw values may share an LCD indication; full display quantization remains unverified.

Osc 2 Coarse at hardware-displayed −6.0 fetched as raw 62 (app 192→62), with Live RX paused. Capture: `osc2-coarse-minus6.syx`. This differs by one raw step from the Osc 1 −6.0 capture (63), paralleling the +6.0 observations (192 versus 193). The LCD indication alone does not yet establish a unique raw value or full conversion formula.

## Live paired-CC endpoint diagnosis

The user reported Coarse ±12.0 appearing as ±11.9 through live MIDI, while a fresh SysEx fetch of physical Osc 2 −12.0 decoded raw 0 and displayed −12.0 correctly. A read-only CoreMIDI capture of Osc 2 Coarse contained CC30 MSB with CC62 LSB 0/64 across the sweep, including MSB 127 / LSB 64 at the high end. This is high-eight-bit packing: `raw = (MSB << 1) | (LSB >> 6)`. The previous full-range rescale mapped the packed upper value 16320 to raw 254 instead of 255. TX likewise now packs raw as `raw << 6`, rather than stretching to 16383. Shared pairedCC8 RX/TX paths use this packing.

Tests cover exact low/high packets and all 256 encode/decode round trips. The first captured low-region message was MSB 0 / LSB 64 (raw 1), so the negative-endpoint report still needs a fresh live confirmation after rebuild. No change was made to Coarse’s approximate musical display formula.

## Isolated automated MIDI → SysEx sweep

With BSII Control closed, `tools/MIDISampler.swift` sent all 256 Osc 2 Coarse raw values via CC30/62 and requested a patch after each. All 256 SysEx values matched exactly, with zero timeouts. The starting raw value 255 was restored; full before/after SysEx messages were byte-identical. Results: `osc2-coarse-sweep.csv`.

Runs with the standalone app open failed; those samples were rejected. Closing the app removed the interference. Its processBlock currently retains incoming MIDI messages, making MIDI pass-through/echo a candidate cause requiring separate confirmation. Do not run the sampler concurrently with the app. No hardware LCD values are automatically observed, so this sweep confirms transport/patch storage rather than the display-rounding formula.

Run on macOS with the synth connected and the app closed:

```sh
swift tools/MIDISampler.swift /tmp/bsii-sampling-results
```

The sampler changes the current edit buffer, restores its starting Coarse value and verifies it, and never sends a patch-save command. It uses MIDI channel 1 and requires exactly one Bass Station II input and output.

## Standalone MIDI echo fix and concurrent sweep

The processor parsed incoming MIDI but left it in the outgoing buffer. The standalone wrapper could therefore send incoming patch replies back to the synth. It now consumes incoming MIDI after parsing in standalone mode; explicit editor sends remain on their existing output path, and plugin-mode pass-through is preserved. A routing regression test failed before the change and passed afterward.

With the rebuilt standalone app open and Live RX running, the same automated 256-value Osc 2 Coarse sweep returned 256 exact matches and zero timeouts. The starting Coarse value 255 was restored, and the complete before/after patches were byte-identical. This reproduces the previously failing concurrent setup and verifies its correction. Results: `osc2-coarse-app-open-sweep.csv`. Hardware-knob-to-app confirmation remains a separate manual check.

## LCD rounding probe limitation

A software-controlled Osc 2 Coarse probe sent raw 62 and verified raw 62 through SysEx. The user confirmed that the physical LCD did not update in response to MIDI. The probe therefore provides no LCD mapping evidence. The pre-probe patch was subsequently restored; its full SysEx readback was byte-identical (Osc 2 Coarse raw 63).

Conclusion: 256 distinct MIDI/SysEx control values are verified. Exact LCD rounding and actual acoustic pitch spacing remain unverified. Retain the approximate Coarse display and its tooltip. Further manual knob sampling is not required at present.

## Osc 1 Mixer level automated sweep

Sent all 256 raw values on paired CC20/52, fetched a 154-byte patch after each, and decoded the Osc 1 Mixer field at byte 37 mask 0x07 / byte 38 mask 0x7C. All 256 values matched with zero timeouts. The complete starting patch was restored and verified byte-for-byte (original Osc 1 level 255). No synth patch-save command was sent. Results: `mix-osc1-sweep.csv`. The 256 captured messages are concatenated in `tests/fixtures/mix-osc1-sweep.syx` and replayed through the actual C++ decoder regression check. No decoder mapping change was needed.

Sampler invocation:

```sh
swift tools/MIDISampler.swift /tmp/bsii-mix-osc1-results mix-osc1
```

## Osc 2 Mixer level automated sweep

Sent all 256 raw values on paired CC21/53 and fetched a patch after each. The Osc 2 Mixer field (byte 38 mask 0x03 / byte 39 mask 0x7E) matched every sent value. Zero mismatches or timeouts; original level 0 and the entire starting patch were restored byte-for-byte. Results: `mix-osc2-sweep.csv`. All 256 captured messages in `tests/fixtures/mix-osc2-sweep.syx` are replayed through the C++ decoder alongside the Osc 1 sweep. No decoder mapping change was needed.

## Sub oscillator Mixer level automated sweep

Sent all 256 raw values on paired CC22/54 and fetched a patch after each. Sub Mixer decoding (byte 39 mask 0x01 / byte 40 mask 0x7F) matched every value, with zero mismatches/timeouts. The original level 0 and entire starting patch were restored byte-for-byte. Results: `mix-sub-sweep.csv`; captures: `tests/fixtures/mix-sub-sweep.syx`. All 256 messages are replayed through the actual C++ decoder alongside both oscillator Mixer sweeps. No mapping change was needed.

## Noise Mixer level automated sweep

Sent all 256 raw values on paired CC23/55 and fetched a patch after each. Noise Mixer decoding (byte 41 mask 0x7F / byte 42 mask 0x40) matched every value, with zero mismatches/timeouts. Original level 0 and the entire starting patch were restored byte-for-byte. Results: `mix-noise-sweep.csv`; captures: `tests/fixtures/mix-noise-sweep.syx`. All 256 messages are replayed through the actual C++ decoder alongside the other Mixer sweeps. No mapping change was needed.

## Ring Mod Mixer level automated sweep

Sent all 256 raw values on paired CC24/56 and fetched a patch after each. Ring Mod Mixer decoding (byte 42 mask 0x3F / byte 43 mask 0x60) matched every value, with zero mismatches/timeouts. Original level 0 and the entire starting patch were restored byte-for-byte. Results: `mix-ring-sweep.csv`; captures: `tests/fixtures/mix-ring-sweep.syx`. All 256 messages are replayed through the actual C++ decoder alongside the other Mixer sweeps. No mapping change was needed.

## External Input Mixer level automated sweep

Sent all 256 raw values on paired CC25/57 and fetched a patch after each. External Input Mixer decoding (byte 43 mask 0x1F / byte 44 mask 0x70) matched every value, with zero mismatches/timeouts. Original level 0 and the entire starting patch were restored byte-for-byte. Results: `mix-external-sweep.csv`; captures: `tests/fixtures/mix-external-sweep.syx`. All 256 messages are replayed through the actual C++ decoder. No mapping change was needed. This checks parameter transport/storage and decoding, not external audio gain or signal quality.

All six Mixer controls now have exhaustive 256-value MIDI-to-SysEx sweeps and C++ decoder replays: 1,536 captured cases in total.

## Filter Cutoff automated sweep

Sent all 256 raw values on paired CC16/48 and fetched a patch after each. The existing verified Cutoff formula (byte 44 low nibble / byte 45 bits 3–6) matched every value, with zero mismatches/timeouts. Original cutoff 38 and the entire starting patch were restored byte-for-byte. Results: `filter-cutoff-sweep.csv`; captures: `tests/fixtures/filter-cutoff-sweep.syx`. All 256 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not a cutoff-frequency calibration in Hz or audio response.

## Filter Resonance automated sweep

Sent all 128 raw values on single CC82 and fetched a patch after each. Existing verified Resonance decoding (byte 45 mask 0x03 / byte 46 bits 2–6) matched every value, with zero mismatches/timeouts. Original Resonance 0 and the entire starting patch were restored byte-for-byte. Results: `filter-resonance-sweep.csv`; captures: `tests/fixtures/filter-resonance-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not resonance’s acoustic response.

## Filter Overdrive automated sweep

Sent all 128 raw values on single CC114 and fetched a patch after each. Existing verified Overdrive decoding (byte 46 mask 0x01 / byte 47 bits 1–6) matched every value, with zero mismatches/timeouts. Original Overdrive 0 and the entire starting patch were restored byte-for-byte. Results: `filter-overdrive-sweep.csv`; captures: `tests/fixtures/filter-overdrive-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not the acoustic distortion response.

## Filter Mod Env amount automated sweep

Sent all 128 raw values on single CC85 and fetched a patch after each. Existing candidate Mod Env decoding (byte 105 mask 0x3F / byte 106 mask 0x40) matched every value, with zero mismatches/timeouts. Original amount 64 and the entire starting patch were restored byte-for-byte. Results: `filter-modenv-sweep.csv`; captures: `tests/fixtures/filter-modenv-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. These are raw control values; bipolar display calibration and acoustic modulation response were not tested.

## Filter LFO 2 amount automated sweep

Sent all 256 raw values on paired CC17/49 and fetched a patch after each. Existing candidate LFO 2 amount decoding (byte 97 mask 0x7F / byte 98 mask 0x40) matched every value, with zero mismatches/timeouts. Original amount 128 and the entire starting patch were restored byte-for-byte. Results: `filter-lfo2-sweep.csv`; captures: `tests/fixtures/filter-lfo2-sweep.syx`. All 256 messages are replayed through the C++ decoder. No mapping change was needed. These are raw control values; bipolar display calibration and acoustic modulation response were not tested.

## Filter Type, Shape, and Slope combinations

Tested all 12 combinations: Classic/Acid × LP/BP/HP × 12/24 dB. Each case sets Classic, sets Shape via CC84 and Slope via CC106, then sets Type via CC83 before requesting a patch. All returned raw fields matched the sent selections. Acid patches retained the Shape/Slope selections in their raw bytes; the existing decoder normalizes their effective display to LP/24 dB. The sweep verifies storage and the existing normalization, not the acoustic Acid response. Entire starting patch restored byte-for-byte.

Results: `filter-switches.csv`; captures: `tests/fixtures/filter-switches.syx`. All 12 captured cases are replayed through the actual C++ decoder. No mapping change was needed.

## Filter Keyboard Tracking automated sweep

Sent all eight raw settings (0–7) via NRPN 0:108 and fetched a patch after each. Existing candidate Tracking decoding (byte 112 mask 0x07) matched every setting, with zero mismatches/timeouts. Original setting 0 and the entire starting patch were restored byte-for-byte. Results: `filter-tracking-sweep.csv`; captures: `tests/fixtures/filter-tracking-sweep.syx`. All eight messages are replayed through the C++ decoder. No mapping change was needed. This validates settings transport/storage and decoding, not measured acoustic tracking ratios.

Filter transport/decoder coverage now includes Cutoff, Resonance, Overdrive, Mod Env amount, LFO 2 amount, Type/Shape/Slope combinations, and Keyboard Tracking: 916 captured cases.

## Amp Envelope Attack automated sweep

Sent all 128 raw values on single CC90 and fetched a patch after each. Existing verified Attack decoding (byte 50 mask 0x1F / byte 51 bits 5–6) matched every value, with zero mismatches/timeouts. Original Attack 0 and the entire starting patch were restored byte-for-byte. Results: `amp-attack-sweep.csv`; captures: `tests/fixtures/amp-attack-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not envelope timing in milliseconds or its acoustic response.

## Amp Envelope Decay automated sweep

Sent all 128 raw values on single CC91 and fetched a patch after each. Existing candidate Decay decoding (byte 51 mask 0x0F / byte 52 mask 0x70) matched every value, with zero mismatches/timeouts. Original Decay 0 and the entire starting patch were restored byte-for-byte. Results: `amp-decay-sweep.csv`; captures: `tests/fixtures/amp-decay-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not envelope timing in milliseconds or acoustic response.

## Amp Envelope Sustain automated sweep

Sent all 128 raw values on single CC92 and fetched a patch after each. Existing candidate Sustain decoding (byte 52 mask 0x07 / byte 53 mask 0x78) matched every value, with zero mismatches/timeouts. Original Sustain 127 and the entire starting patch were restored byte-for-byte. Results: `amp-sustain-sweep.csv`; captures: `tests/fixtures/amp-sustain-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not acoustic sustain level calibration.

## Amp Envelope Release automated sweep

Sent all 128 raw values on single CC93 and fetched a patch after each. Existing candidate Release decoding (byte 53 mask 0x03 / byte 54 mask 0x7C) matched every value, with zero mismatches/timeouts. Original Release 0 and the entire starting patch were restored byte-for-byte. Results: `amp-release-sweep.csv`; captures: `tests/fixtures/amp-release-sweep.syx`. All 128 messages are replayed through the C++ decoder. No mapping change was needed. This validates parameter transport/storage and decoding, not envelope timing in milliseconds or acoustic response.

Amp ADSR transport/decoder sweeps now cover 512 cases across Attack, Decay, Sustain, and Release.

## Amp Envelope Trigger and Retrigger combinations

Tested all six combinations of Trigger (Multi, Single, Autoglide; NRPN 0:73) and Retrigger (Off/On; NRPN 0:109). Trigger occupies byte 55 bits 1–2 with wire values 0, 1, 2 respectively. Retrigger occupies byte 114 bit 6; its existing mapping matched all captures. The entire starting patch was restored byte-for-byte.

Trigger was missing from patch decoding and editor application. Added its captured mapping, converting wire order to editor choice order (Single, Multi, Autoglide). The six-capture regression failed before that fix. Results: `amp-switches.csv`; captures: `tests/fixtures/amp-switches.syx`; sampler: `tools/AmpSwitchSampler.swift`. This validates settings transport/storage and decoding, not acoustic envelope behavior. Other envelope options remain separately untested.

## Amp Envelope Fixed Sustain and Retrigger Count

Tested all 34 combinations of Fixed Sustain (Off/On; NRPN 0:114) and Retrigger Count (0–16; NRPN 0:117). Fixed Sustain occupies byte 119 bit 1. Count occupies byte 121 bits 0–3 followed by byte 122 bit 6. Each returned patch matched the sent settings. The entire starting patch was restored byte-for-byte.

Both fields were missing from patch decoding and editor application. Added the captured mappings and a regression replay of all 34 messages; this test failed before the mappings were added. Results: `amp-options.csv`; captures: `tests/fixtures/amp-options.syx`; sampler: `tools/AmpOptionsSampler.swift`. This validates parameter transport/storage and decoding, not acoustic envelope behavior.

## Mod Envelope ADSR automated sweeps

Sent all 128 raw values for each of Attack (CC102), Decay (CC103), Sustain (CC104), and Release (CC105), requesting a patch after every setting: 512 cases with zero mismatches/timeouts. Existing decoder mappings matched every capture: Attack uses byte 57 mask 0x3F / byte 58 mask 0x40; Decay byte 58 mask 0x1F / byte 59 mask 0x60; Sustain byte 59 mask 0x0F / byte 60 mask 0x70; Release byte 60 mask 0x07 / byte 61 mask 0x78. No mapping changes were needed.

The entire starting patch was restored and verified byte-for-byte after each sweep (starting Attack 0, Decay 0, Sustain 127, Release 0). Results are in `mod-{attack,decay,sustain,release}-sweep.csv`; corresponding fixtures in `tests/fixtures/` replay every captured message through the actual C++ decoder. This validates transport/storage and decoding, not envelope timing or acoustic response. Mod Envelope options are covered separately below.

## Mod Envelope Trigger, Retrigger, Fixed Sustain, and Retrigger Count

Captured six Trigger × Retrigger cases (NRPN 0:105 and 0:110), then 34 Fixed Sustain × Retrigger Count cases (NRPN 0:115 and 0:118). The other pair was set to Multi/Off or Off/0 respectively. All 40 returned patches matched the selections, and the complete starting patch was restored byte-for-byte. These are pairwise sweeps, not an exhaustive four-way interaction test.

Trigger uses byte 62 bits 2–3 (wire Multi=0, Single=1, Autoglide=2); Retrigger uses byte 115 bit 5; Fixed Sustain uses byte 120 bit 0; Count uses byte 122 bits 0–2 followed by byte 123 bits 5–6. Existing Retrigger decoding matched. Trigger, Fixed Sustain, and Count were missing from decoding and editor application; added these mappings, with Trigger converted to editor choice order. The 40-capture regression failed before the additions.

Results: `mod-options.csv`; captures: `tests/fixtures/mod-options.syx`; sampler: `tools/ModOptionsSampler.swift`. This validates settings transport/storage and decoding, not acoustic envelope behavior.

## LFO waveforms and speeds

Sent all four waveform selections for LFO 1 (CC88) and LFO 2 (CC89), then all 256 speed values for LFO 1 (paired CC18/50) and LFO 2 (paired CC19/51): 520 captures, zero mismatches/timeouts. Existing decoder mappings matched every capture: LFO 1 Wave byte 63 mask 0x06, LFO 2 Wave byte 70 mask 0x0C, LFO 1 Speed byte 66 mask 0x3F / byte 67 mask 0x60, LFO 2 Speed byte 73 mask 0x7F / byte 74 mask 0x40. No app mapping changes were needed.

The complete starting patch was restored and verified byte-for-byte after each sweep (waveforms 0, speeds 75 and 52). Results: `lfo{1,2}-{wave,speed}-sweep.csv`; corresponding captures in `tests/fixtures/` are replayed through the actual C++ decoder. The sampler now restores the complete saved edit-buffer patch, rather than depending on the tested field's original decoded value. This validates MIDI transport/storage and decoding, not measured frequency or waveform shape. Delay, Slew, and sync options remain separately untested.

## LFO Delay, Slew, and Key Sync

Swept all 128 values for both Delays (CC86/87) and Slews (NRPN 0:86 / 0:90), plus both Off/On settings for each Key Sync (NRPN 0:89 / 0:93): 516 captures with zero mismatches/timeouts. Existing decoder mappings matched all captures. Delay mappings: LFO 1 byte 64 mask 0x7F; LFO 2 byte 70 mask 0x01 / byte 71 mask 0x7E. Slew mappings: LFO 1 byte 65 mask 0x3F / byte 66 mask 0x40; LFO 2 byte 72 mask 0x7F. Key Sync mappings: LFO 1 byte 69 mask 0x10; LFO 2 byte 76 mask 0x20. No app mapping changes were needed.

The entire starting patch was restored and verified byte-for-byte after every sweep (Delays/Slews 0, LFO 1 Key Sync Off, LFO 2 Key Sync On). Results: `lfo{1,2}-{delay,slew,keysync}-sweep.csv`; corresponding fixtures replay all 516 messages through the actual C++ decoder. This validates MIDI transport/storage and decoding, not measured delay/slew times or acoustic key synchronization. These tests cover the Key Sync controls exposed in the app; other hardware sync modes were not tested.

## Distortion, Osc Filter Mod, and VCA Limit

Sent all 128 values for Distortion (CC94), Osc Filter Mod (CC115), and VCA Limit (CC95): 384 captures with zero mismatches/timeouts. Existing decoder mappings matched all captures. Osc Filter Mod uses byte 106 mask 0x1F / byte 107 mask 0x60; Distortion uses byte 107 mask 0x0F / byte 108 mask 0x70; VCA Limit uses byte 108 mask 0x07 / byte 109 mask 0x78. No app mapping changes were needed.

The complete starting patch was restored and verified byte-for-byte after each sweep; all three starting values were 0. Results: `{distortion,osc-filter-mod,vca-limit}-sweep.csv`; corresponding fixtures replay all 384 messages through the actual C++ decoder. This validates MIDI transport/storage and decoding, not measured acoustic distortion, modulation response, or limiting behavior. Other performance controls remain separately untested.

## Modulation wheel and aftertouch assignments

Swept all 128 values for ModWh → LFO1 Pitch (NRPN 0:70), ModWh → Osc2 Pitch (0:78), Aftertouch → Filter (0:74), Aftertouch → LFO1 (0:75), and Aftertouch → LFO2 Speed (0:76): 640 captures, zero mismatches/timeouts. Existing decoder mappings matched every capture. Fields respectively occupy byte 83 mask 0x0F / byte 84 mask 0x70; byte 85 mask 0x03 / byte 86 mask 0x7C; byte 86 mask 0x01 / byte 87 mask 0x7E; byte 88 mask 0x7F; byte 89 mask 0x3F / byte 90 mask 0x40. No app mapping changes were needed.

The entire starting patch was restored and verified byte-for-byte after every sweep. Original values in the above order were 74, 64, 74, 64, 64. Results: `{modwheel-lfo1-pitch,modwheel-osc2-pitch,aftertouch-filter,aftertouch-lfo1,aftertouch-lfo2-speed}-sweep.csv`; corresponding fixtures replay all 640 messages through the actual C++ decoder. This validates assignment transport/storage and decoding, not physical wheel/pressure response or acoustic modulation depth.

## Glide, pitch bend range, and arpeggiator swing

Glide (Portamento, CC5) passed all 128 values with the existing mapping (byte 13 mask 0x03 / byte 14 mask 0x7C). Swing (CC116) stores byte 81 mask 0x3F / byte 82 mask 0x40; a full 128-message probe showed hardware clamping to 3–97. The 33 out-of-range inputs clamp to the endpoints. Limited the app slider to 3–97; regression replay checks every captured value against that clamp.

Bend Amount (CC107) exposed a wire encoding error. Direct values 1–12 all stored byte 16 = 40. A separate 128-message wire probe showed values clamped to 40–88, centered at 64. The current app's positive 1–12 semitone subset therefore needs wire 65–76. Corrected outgoing MIDI, live CC decoding, and patch decoding using a shared conversion. The corrected twelve-setting sweep matched every value; the captured-patch regression failed before the fix. Unsupported bend values outside the editor's existing range are skipped on reception. Wider signed bend support remains a separate UI task.

Novation's [MIDI appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix) lists CC107 and bend range 1–12, but does not describe the offset observed on this hardware. The offset and clamp conclusions above come from these captures, not the documentation. They validate parameter transport/storage, not measured pitch bend intervals or swing timing.

All starting patches were restored and verified byte-for-byte after every sweep/probe (Glide 0, bend wire 76, Swing 50). Committed fixtures cover 128 Glide settings, 12 corrected bend settings, and all 128 Swing inputs: 268 cases. Results: `{portamento,bend-amount,arp-swing}-sweep.csv`; fixtures: corresponding files under `tests/fixtures/`. Swing CSV includes expected clamping and bend CSV records actual wire values.

Rebuilt-app check: setting Bend Amount to 6 sent wire 70 (patch byte 16 = 0x46); fetching the patch retained app value 6. Restored original bend wire 76, verified the complete original patch byte-for-byte, and fetched it back into the app as 12. The shared conversion has a 1–12 round-trip regression; physical hardware-origin live CC reception was not manually exercised during this check.

## Velocity assignments and oscillator sync

Swept all 128 CC inputs for Velocity → Amp (CC112), Velocity → Mod (CC113), and Oscillator Sync (CC110): 384 captures. Both velocity fields clamp input 0 to 1; all 1–127 values match directly. Their existing packed mappings are correct (Amp: byte 49 mask 0x3F / byte 50 mask 0x40; Mod: byte 56 mask 0x7F). Limited both editor controls to the measured 1–127 range.

Sync stores byte 18 bit 6: input 0 is Off; all inputs 1–127 are On. Added missing patch decoding and editor application, using canonical 0/127 for the existing raw control. The 128-capture decoder regression failed before the mapping was added. The raw slider remains a raw MIDI control; a labeled Off/On selector would be a separate UI change.

Every sweep restored the complete starting patch byte-for-byte (both velocity assignments 64, sync Off). Results: `{velocity-amp,velocity-mod,osc-sync}-sweep.csv`; fixtures: matching files in `tests/fixtures/`. CSV expected values account for velocity clamping and sync normalization. This validates MIDI settings transport/storage and patch decoding, not measured acoustic velocity response or oscillator synchronization.

## Oscillator pitch modulation and pulse-width controls

Swept both oscillators' Mod Envelope → Pitch (CC71/76), LFO 1 → Pitch (paired CC28/60 and CC31/63), Mod Envelope → PW (CC72/77), LFO 2 → PW (CC73/78), and Manual PW (CC74/79). Both LFO pitch controls cover 256 values; the other eight controls cover 128 each: 1,536 captured cases. All existing mappings matched; Oscillator 1 Mod Envelope → PW was missing from patch decoding and editor application.

The missing field occupies byte 101 mask 0x03 / byte 102 mask 0x7C. Added that verified mapping and editor application; the full captured-patch regression failed only for this field before the fix. The exploratory sampler initially reported unknown values for this field; its committed CSV was decoded from all 128 captures after locating the field. Every value matches. Other sweep CSVs report zero mismatches/timeouts.

The complete starting patch was restored and verified byte-for-byte after every sweep. Initial values: Mod Envelope Pitch 64/49, LFO 1 Pitch 128/128, Mod Envelope PW 64/64, LFO 2 PW 64/64, Manual PW 64/64 (Oscillator 1/2). CSVs and fixtures use `osc{1,2}-{modenv-pitch,lfo1-pitch,modenv-pw,lfo2-pw,manual-pw}-sweep` names under `docs/` and `tests/fixtures/`. All 1,536 messages replay through the actual C++ decoder. This validates control transport/storage and decoding, not acoustic pitch depth, pulse width, or waveform response.

## Patch volume and sub-oscillator settings

Captured all 128 Patch Volume inputs (CC7), all 128 Sub Octave inputs (CC81), and three Sub Wave choices (CC80): 259 cases. Volume occupies byte 109 mask 0x03 / byte 110 mask 0x7C and matches 0–127 directly. Sub Wave occupies byte 36 mask 0x30 and matches choice indexes 0–2. Sub Octave occupies byte 37 bit 3: inputs 0–62 clear the bit; 63–127 set it. The existing raw octave control uses canonical 0/127 on patch decoding. These captures establish the wire threshold, not acoustic octave direction.

All three fields were missing from patch decoding and editor application. Added their captured mappings; regression replay failed for all three before the fix. The entire starting patch was restored and verified byte-for-byte after each test (Volume 100, Sub Wave 2, octave bit set). Results: `{patch-volume,sub-octave,sub-wave}-sweep.csv`; matching fixtures under `tests/fixtures/`.

An initial sampler routing error sent these new control names to the fallback CC30. Those exploratory runs restored the complete starting patch and were discarded. Corrected routes CC7/81/80 were rerun; only the corrected captures are committed. The exploratory decoder initially returned unknown values; committed CSVs were decoded from the returned bytes after mapping discovery. All 259 corrected captures match. This validates MIDI transport/storage and decoding, not measured output level or acoustic sub waveform/octave.

## Main oscillator waveform SysEx

Captured all four selections for Oscillator 1 (NRPN 0:72) and Oscillator 2 (NRPN 0:82). Oscillator 1 occupies byte 19 mask 0x60; Oscillator 2 occupies byte 24 mask 0x03. Both store Sine/Triangle/Saw/Pulse as indexes 0/1/2/3. Added both missing decoder fields and editor application. Both four-message regression replays failed before the mappings and pass afterward.

The entire starting patch was restored and verified byte-for-byte after each four-setting capture (both starting waveforms Sine). Results: `osc{1,2}-wave-sweep.csv`; fixtures: corresponding files under `tests/fixtures/`. Exploratory sampler CSVs initially marked these unmapped fields unknown; the committed CSVs were decoded from all eight captures after locating the fields. All eight values match. Earlier manual tests establish live MIDI behavior separately; these captures validate returned patch storage and decoding.

Rebuilt-app verification with Live RX paused: importing each oscillator's Pulse capture changed its visible selector Sine → Pulse; a fresh Get Patch from the restored hardware changed it Pulse → Sine. Both editor application paths were verified without changing the hardware during import.

## Coarse tuning regression completion — 17 October 2026

Repeated full Coarse sweeps for Oscillator 1 (paired CC27/59) and Oscillator 2 (CC30/62). Each returned all 256 raw values exactly, with zero mismatches/timeouts. Both complete starting patches were restored byte-for-byte (starting raw values 127 and 63). Existing decoder mappings remain correct; no app source/display changes were needed.

Committed both full raw sweeps as `tests/fixtures/osc{1,2}-coarse-sweep.syx` and results as `docs/osc{1,2}-coarse-regression-sweep.csv`, retaining the earlier Oscillator 2 CSVs separately. Added actual decoder replay of all 512 messages and the ten earlier Coarse captures: Oscillator 1 raw 0/63/127/193/255 and Oscillator 2 raw 0/62/128/192/255 for min/−6/center/+6/max. All 522 checks pass. These observations intentionally preserve oscillator-to-oscillator and midpoint differences; they do not establish an exact LCD rounding function. Existing approximate display conversion is unchanged.
