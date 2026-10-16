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
