# BSII Control v0.8.2

Bass Station II hardware editor / VST3 / AU / Standalone built with JUCE.

## What changed in v0.8.2

- Added **Osc 1 Waveform** selector: Sine / Triangle / Saw / Pulse.
- Added **Osc 2 Waveform** selector: Sine / Triangle / Saw / Pulse.
- Added named **Sub Wave** selector: Sine / Pulse / Square.
- Oscillator waveform changes use the Bass Station II live MIDI implementation:
  - Osc 1 waveform: NRPN `0:72`
  - Osc 2 waveform: NRPN `0:82`
  - Sub waveform: CC `80`
- Removed the old raw Sub Wave CC control to avoid duplicate ownership of CC80.
- Choice badges now say **MIDI verified** / **MIDI test** so they are not confused with SysEx-patch verification.
- Existing v0.8.1 **Pause Live RX**, **Get Patch from BSII**, snapshots, SysEx capture/import, and decoder diagnostics remain intact.

The three waveform selectors are now marked **MIDI verified** following physical-unit testing. All three waveform selectors now also have captured SysEx decoding coverage. See [hardware validation](docs/hardware-validation.md) for the results and the [current coverage audit](docs/validation-coverage.md) for remaining gaps.

## Build on macOS

```bash
cd /path/to/BS2VSTPROJECT
cmake -B build -G Xcode
cmake --build build --config Release
```

Launch Standalone:

```bash
open "build/BSIIControl_artefacts/Release/Standalone/BSII Control.app"
```

## First waveform test

With Live RX enabled and Bass Station II selected as MIDI output:

1. On the hardware, move **Osc 1 Waveform** through Sine / Triangle / Saw / Pulse. The app selector should follow.
2. Change **Osc 1 Waveform** from the app. The hardware sound should follow.
3. Repeat for **Osc 2 Waveform**.
4. Repeat for **Sub Wave**.

Live MIDI and SysEx decoding are separate validation paths. `Get Patch from BSII` now applies Osc 1 Waveform, Osc 2 Waveform, and Sub Wave; see the coverage audit.

## Repository baseline

The initial commit and `v0.8.2` tag preserve the source from the v0.8.2 working project. Build outputs and downloaded JUCE sources are excluded. CMake currently declares version `0.8.0`; it is intentionally preserved in this baseline.

Requirements: macOS, Xcode with command-line tools, CMake 3.22 or newer, and an internet connection for the initial JUCE 9.0.4 download. The project uses C++20. Build outputs appear under `build/BSIIControl_artefacts/Release/`.

Open this folder in GitHub Desktop with **File → Add Local Repository**. Review changes, commit, then push to keep GitHub up to date.

Snapshot save/load and a combined Send All hardware recall are covered separately in [state integration validation](docs/state-integration-validation.md).

The current standalone UI shows Sub Octave as a highlighted **−1 / −2 switch**. The window is resizable, control rows adapt to width, and taller pages scroll so lower values remain reachable.
