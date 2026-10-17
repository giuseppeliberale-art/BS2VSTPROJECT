# Early macOS plugin smoke test

Started 2026-10-17 at source commit ac55c82. Initial test exposed a SysEx receive
failure; the direct-input fix and retest are documented below.

## Completed

- Built BSIIControl_VST3 and BSIIControl_AU, Release, native Apple Silicon arm64.
- Installed local ad-hoc signed builds in the user's Library/Audio/Plug-Ins/VST3
  and Components folders. No previous BSII Control bundles existed at these paths.
- Strict/deep signature verification passed for both bundles.
- Apple Audio Unit validator: `auval -v aumf Bs2C Gutz` exited 0 and reported
  AU VALIDATION SUCCEEDED, including parameter scheduling, rendering and MIDI.
- Installed host version: Ableton Live 12 Suite 12.4.6
  (2026-09-10_0de5c8fa9a).

Full build and AU validator logs stay local in ignored
`validation-results/20261017-ableton-smoke/`.

## Live discovery/load check

- Enabled Use VST3 Plug-In System Folders (previously Off); scan completed.
- Browser displayed GutzDev > BSII Control. Loaded VST3 on track1 of the user's
  new empty Untitled Set. Device activator is on and Live remains responsive.
- Live's Show/Hide Plug-In Window reports1 after opening the editor. UI automation
  currently reads the main Set window; user asked to bring the actual plugin
  window forward before verifying its controls.
- Standalone was closed before the plugin test.
- Current hardware sound backed up without editing in
  `midi-lab-results/20261017-175625-backup-3acea8/baseline.syx`.
- AU/AUv3 sources remain Off in Live. Only VST3 has been loaded so far.

## Remaining host checks

- Broader editor interaction, audio/note routing and multiple-instance behavior.
- UI-to-hardware edits and incoming hardware MIDI/patch replies.
- Host parameter control with editor open and closed.
- Save/reopen Set, editor state recall and explicit hardware-send behavior.
- AU loading in Live separately from VST3; do not mix formats in the same test Set.

User was asked to open Live Settings > Plug-Ins and leave it open. Live initially
had the demo arrangement open; user saved it as stock project, then opened an
empty Untitled Set. Preserve that saved project and use the empty Set for testing.
Both plugin formats are installed; no hardware MIDI edits were made in this phase.
Standalone label-fix app was still running at the start; close it before hardware
routing tests to avoid confusing its receive/UI behavior with the plugin instance.

Ableton documents VST3 direct MIDI output and no AU direct MIDI output in Live:
https://help.ableton.com/hc/en-us/articles/209068929-Using-AU-and-VST-plug-ins-on-macOS
This concerns host MIDI routing. Our current editor also opens a direct hardware
MIDI output; whether that path behaves correctly in either wrapper needs testing.

Passing auval is not DAW compatibility, hardware integration or automation approval.
Known source concern: direct MIDI output and user-change callbacks live in the
editor; headless automation and editor destruction/recreation must be exercised.

## Get Patch failure, fix and hardware retest

- User confirmed live hardware cutoff movement updated the plugin, while Get Patch
  timed out. Input Track was enabled. Enabled Output Track as well, left Sync,
  Remote and Control Surface unchanged; Get Patch still timed out.
- Source review: requests already went directly to the selected hardware output,
  but replies depended on the host-provided MIDI buffer. Ableton documents SysEx
  support through Max for Live only:
  https://help.ableton.com/hc/en-us/articles/360003148640-SysEx-support
- Added processor-owned HardwareSysExInput for plugin wrappers, matching exactly
  one input by the selected output's name. It receives only SysEx; CC/NRPN,
  notes and Program Change keep their existing host route. Host SysEx parsing is
  suppressed while the direct input is active to avoid processing duplicate dumps.
  Standalone continues using its Active MIDI inputs. Connection/error text now
  distinguishes the wrapper and reports missing/ambiguous input ports.
- Isolated native integration test uses a separate process's virtual MIDI source
  and a real saved patch. PASS: byte-exact single SysEx callback, non-SysEx messages
  excluded, disconnect, reconnect and missing port. JUCE excludes a process's own
  virtual ports, so a same-process sender could not exercise this path.
- Standalone/VST3/AU built successfully. Existing seven CTest groups passed.
- Saved empty test Set in ignored validation-results/20261017-ableton-smoke/
  BSII VST3 Smoke Test Project/BSII VST3 Smoke Test.als before restart. Recovery
  copy of Untitled was saved as Recovered Untitled.als in that project folder.
- Reinstalled ad-hoc signed VST3/AU after closing Live; prior bundles retained in
  the ignored before-direct-input folder. Saved Set reopened with updated VST3.
- User confirmed Get Patch success. Agent then inspected the floating plugin:
  SysEx RX154 bytes; Live patch received and decoded from Bass Station II.
- Agent paused Live RX and clicked Get Patch again: fresh decoded154-byte reply,
  no editor fields changed. Restored Live RX running. No Send All/preset-save was
  used. User's intervening hardware changes were not overwritten by an old backup.

Run the explicit native-port check on macOS (creates only temporary virtual ports):

```sh
cmake --build build --config Release --target BSIIDirectSysExTest -- CODE_SIGNING_ALLOWED=NO
build/Release/BSIIDirectSysExTest tests/state-fixtures/baseline.syx
```

This test is intentionally not registered in the offline CTest suite because it
requires access to native MIDI services. It sends no messages to the physical synth.
The direct input is independent of the editor lifetime, but headless parameter
sending/automation and full session/hardware recall remain unverified. Exact-name
matching deliberately refuses ambiguous ports. Windows port-sharing, differently
named input/output endpoints and multi-device use remain untested.

Updated AU also passed auval after installation of the direct-input fix. VST3
hardware receipt was rechecked in the floating editor and Live RX left running.
No result yet for host automation with the editor closed or complete hardware
state restoration on session reload; those remain the next tests.
