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
