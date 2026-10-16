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
