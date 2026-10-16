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
