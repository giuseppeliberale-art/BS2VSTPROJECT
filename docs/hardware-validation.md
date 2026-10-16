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
