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

## Host parameter sending with the editor closed

The original build reproduced an editor-lifetime failure:

- Physical sound backed up in `midi-lab-results/20261017-181533-backup-435929/`;
  decoded Resonance was 0.
- Plugin UI edit to 10 reached hardware (181710-backup-0be993).
- Ableton's exposed Resonance parameter set to 20 reached hardware while the
  editor existed (181802-backup-713ec2).
- Closed the editor using Live's Show/Hide Plug-In Window checkbox, then set
  Live's parameter to 30. Readback remained 20 (181848-backup-6efffe): FAIL.
  Merely hiding all plugin windows with a keyboard shortcut does not destroy
  the editor and is not equivalent to this test.
- Reopened the editor and restored Resonance to 0. Readback confirmed 0
  (182135-backup-b82dec). No stored preset was overwritten.

Cause: the editor owned the native MIDI output, and only Slider/ComboBox callbacks
transmitted parameter changes. The processor now retains the output and listens
to APVTS changes. Callbacks publish atomics; a message-thread timer sends the latest
pending value for each parameter at up to 100 Hz. This is coalesced hardware
control, not sample-accurate MIDI scheduling. Linked Trigger edits are handled in
that same processor path. Reception, patch imports, snapshot loads and host-state
recall use scoped suppression so parameter notifications do not echo to hardware.
MIDI channel and Live RX selection survive editor destruction/recreation within
an instance. Port discovery still initially happens when opening the editor.
Full session connection persistence and automatic hardware recall remain separate
work; receiving/applying live controls and preset refresh still involve the editor.

A macOS-only native regression target uses the actual processor/APVTS and an
isolated external virtual MIDI sink, with no messages sent to the physical synth:

```sh
cmake --build build --config Release --target BSIIProcessorAutomationTest -- CODE_SIGNING_ALLOWED=NO
build/Release/BSIIProcessorAutomationTest
```

It checks sends before any editor exists and after destruction, a worker-thread
parameter change, CC/paired-CC/NRPN bytes, shared Trigger edits, silent received
values and host-state recall, retained mixed stored Trigger fields, and channel/RX
selection across editor recreation. Like the direct SysEx test, it is not part of
the offline CTest suite because native MIDI services are required.


Retest results (2026-10-17):

- Native processor regression PASS; seven offline CTest groups PASS.
- Standalone/VST3/AU Release build PASS. Reinstalled local VST3/AU with prior
  bundles backed up in ignored before-processor-automation/. Strict/deep signatures
  and updated AU validator PASS.
- Saved Set reopened in Live. Opened the editor once to initialise its selected
  hardware port, then closed it with Show/Hide Plug-In Window=0. Host Resonance30
  reached the synth: decoded patch readback30 in 183028-backup-c8f33e. PASS.
- Reset host Resonance0 while the editor remained closed. Readback0 in
  183045-backup-40dea6. All decoded patch fields matched the pre-test backup.
  Saved the test Set, leaving Resonance0 and editor closed.
- This verifies host parameter control after editor destruction, not recorded
  envelope playback, initial port connection without ever opening an editor,
  full Set recall to hardware, multiple instances, or other DAW/OS combinations.


## Recorded playback and saved connection follow-up

User tested an Ableton External Instrument sending notes to Bass Station II on
channel1, with BSII Control Cutoff and Resonance automation. User reported both
automated controls working during playback. After saving, quitting Live and
reopening the Set, automation required opening the plugin editor once; after
opening and closing it, the user confirmed notes and both automation lanes worked.
This is user-reported hardware playback evidence, not an independently captured
MIDI trace. External Instrument's audio return was not validated in this test.

A fresh-processor native regression reproduced the connection failure without
creating an editor: restored connection metadata left the output closed and the
expected channel3 Resonance45 message was absent. Connection persistence now
stores output identifier/name, channel and Live RX setting in host state. Restore
schedules native MIDI opening on the processor's message-thread timer, with a
one-second retry for missing devices. An obsolete identifier falls back only to
one exact name match. An unavailable saved port does not auto-select another synth.
Patch snapshots remain separate, and loading a Set does not send a patch/Send All.

Older Sets contain no connection metadata. With the updated build, open the editor
once to select the hardware, close it, then save the Set again. Subsequent loads
can reopen that saved connection without the editor. A new manual Ableton restart
check is required before calling this fixed in the host.


Local validation of saved-connection change: fresh-instance roundtrip regression
PASS, including exact-name fallback and missing-device refusal; existing seven
CTest groups PASS; Release Standalone/VST3/AU build PASS. Updated local VST3/AU
installed after user saved/closed Live; prior bundles retained under ignored
before-saved-connection/. Strict/deep code signatures and AU validator PASS.
Manual Ableton restart with editor kept closed remains pending.
