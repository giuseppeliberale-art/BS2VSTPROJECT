# BSII Control

A MIDI editor for the **Novation Bass Station II**, built with JUCE and C++20.
Edit the connected synth from your Mac, follow hardware knob movements in the app,
fetch its current patch, and save editor snapshots or raw SysEx files.
The sound comes from the Bass Station II; this project does not emulate its audio engine.

**Current status:** working standalone editor under active development. The project
also defines AU and VST3 builds, but the latest changes have been validated in the
standalone app; DAW compatibility and session recall still need testing.
Hardware checks have used a Bass Station II reported as running firmware **4.14**.

The `v0.8.2` tag preserves the original project baseline. `main` includes the later
fixes and features described below; it is not a new tagged release. Some app/build
version labels still show earlier numbers.

## Available features

- **76 sound controls** across Osc 1, Osc 2, Mixer, Filter, LFO/Glide,
  Amp Env, Mod Env and FX/Performance pages.
- **Live MIDI in both directions:** app edits send CC/NRPN messages, and supported
  hardware changes update the editor while Live RX is running.
- **Preset selection, 0–127:** select a number, step with Previous/Next, or reload
  with Load Preset. Hardware preset changes are followed automatically, with a
  short settling delay before the app fetches the current patch.
- **Get Patch from BSII:** request the current temporary patch and apply its
  supported fields to the editor, without manually performing a hardware dump.
- **Snapshots and SysEx:** save/load editor state, capture and save a raw patch,
  import a patch into the editor, or send a patch to the synth.
- **Clearer controls:** Sub Octave −1/−2 and Osc Sync Off/On switches, signed bend
  and modulation values, resizable pages and scrolling for smaller windows.

This is a partial instrument editor. Arp Swing is available, but the remaining
Arp/Seq controls, LFO sync divisions, several modulation routes and newer firmware
functions are still on the roadmap. **AFX mode is planned last.**

## Build on macOS

Requirements:

- Xcode with command-line tools.
- CMake **3.22 or newer**.
- Internet access for the first configure, which downloads **JUCE 9.0.4**.
- Python **3.10 or newer** for the regression suite and MIDI Lab only.

From the repository folder:

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Release --target BSIIControl_Standalone -- CODE_SIGNING_ALLOWED=NO
```

The standalone app is created at:

```text
build/BSIIControl_artefacts/Release/Standalone/BSII Control.app
```

Open it in Finder after the build. This is a local development build; the build
command does not provide a signed, notarized installer. A fresh AU/VST3 host test
is still required before treating the plugin formats as ready for use.

Build outputs and downloaded JUCE sources are excluded from Git. To work through
GitHub Desktop, clone this repository, or use **File → Add Local Repository** for
an existing checkout. Building does not automatically replace another app copy
you may already have open.

## Connect and start editing

1. Connect the Bass Station II to the Mac over USB and turn it on. Listen through
   the synth's audio output or your usual audio interface.
2. Open the standalone app. In **Settings… → Active MIDI inputs**, enable the
   Bass Station II input.
3. Select **Bass Station II** as the app's MIDI output. Set **Channel** to match
   the synth's MIDI channel; the app starts on channel 1.
4. Click **Get Patch from BSII** to bring the current sound into the editor before
   editing. At startup, the displayed controls may reflect an earlier app state.
5. Keep **Live RX is running** enabled to follow supported hardware changes.
   App control edits are sent to the selected MIDI output immediately.

The preset selector initially shows **Unknown / edit buffer**. A current-patch
reply does not identify a stored preset number. Selecting a preset in the app or
receiving a hardware preset change establishes the number.

**Selecting a preset replaces the synth's temporary sound.** Save any edits you
want to keep first. The preset selector does not overwrite a stored preset slot;
use the hardware's save procedure when you intend to save into its preset memory.

## Which save, load or transfer control should I use?

| Control | What it does |
| --- | --- |
| **Get Patch from BSII** | Reads the synth's current edit buffer and updates the editor's supported controls. |
| **Save Snapshot…** | Saves the editor's parameter state as a `.bsiistate` file. Fetch the hardware patch first if you want its current values. |
| **Load Snapshot…** | Loads a `.bsiistate` into the editor. It does not send that state to the synth automatically. |
| **Send All → BSII** | Sends all exposed editor controls to the synth through live MIDI. This is not a full raw-patch transfer. |
| **Import .syx → Editor** | Decodes a raw patch file into the editor without sending it to the synth. |
| **Send .syx → BSII** | Sends the chosen SysEx file to the hardware. For the single-patch files used here, this replaces the current edit buffer. |
| **Arm Patch Capture** | Arms receipt of the next manual dump. Perform the dump on the hardware, then save the received message. |
| **Save Last .syx…** | Saves the last received SysEx message unchanged. Use Get Patch or a manual capture first to obtain a fresh one. |

A snapshot represents editor state; a raw `.syx` preserves the received patch
message, including data for fields the editor may not expose. Importing a file,
loading a snapshot or sending SysEx clears the displayed preset identity.

Clicking the Live RX control toggles between **Live RX is running** and
**Live RX is paused**. Pausing stops live CC/NRPN and hardware preset following;
SysEx reception and Get Patch remain available. It does not disable app-to-synth
editing. A tick on the current control means reception is paused; read the status
text to see the state.

## What has been tested?

- All **76 exposed controls** have captured patch/decoder coverage. The retained
  fixture set includes **96 files and 9,251 patch messages**.
- Manual checks have exercised supported controls in both directions, with fixes
  and retests recorded in the [manual test review](docs/manual-test-review.md).
- Snapshot round trips and combined Send All recalls have been checked against
  saved hardware patches: [state validation](docs/state-integration-validation.md).
- Preset changes have been checked from the app and hardware. After app preset
  selection, all 76 editor values matched the saved hardware patch:
  [preset validation](docs/preset-validation.md).
- The current offline suite has **seven registered test groups**, including
  12 Python checks for the MIDI Lab tools.

These checks establish specific MIDI, patch storage and editor behaviors. They do
not measure acoustic accuracy or establish complete instrument/DAW coverage.
See the [coverage audit](docs/validation-coverage.md) for the detailed control
inventory and [current handover](docs/project-status.md) for newer results and limits.

Known limits include approximate Coarse display rounding, remaining live preset
channel/paused-RX spot checks, device reconnect behavior and AU/VST3 host testing.
Trigger edits in either envelope page currently apply to both envelopes, matching
the chosen front-panel workflow; independent mixed-trigger sound-engine behavior
has not been established. See [trigger findings](docs/trigger-behavior-research.md).
The hardware has occasionally shown `---` after SysEx transfer while remaining
playable; its meaning is unresolved.

## Local validation and MIDI tools

Run the build and offline checks with:

```sh
./tools/validate.sh
```

No synth connection is needed and no MIDI is sent. The routine builds the
standalone app and regression tools, prints a short result and saves full logs
under the ignored `validation-results/` folder. App signing and live hardware
checks are separate.

For passive hardware MIDI recording and short reports:

```sh
python3 tools/midi-lab/lab.py
```

The default mode listens for 30 seconds without creating a MIDI output port.
Recordings stay in the ignored `midi-lab-results/` folder. The tool also offers
explicit patch backup/restore, controlled sampling, offline replay and comparison
using the app's actual patch decoder. See [MIDI Lab instructions](docs/midi-lab.md)
before using modes that change the current sound.

## Project reference

- [Current state and next work](docs/project-status.md)
- [Changes since the baseline](docs/next-version.md)
- [Hardware validation evidence](docs/hardware-validation.md)
- [Manual test checklist](docs/manual-test-checklist.md)
- [Official manual reference index](docs/local-reference.md)
- [Implemented MIDI control definitions](Source/BSIIMidi.h)
