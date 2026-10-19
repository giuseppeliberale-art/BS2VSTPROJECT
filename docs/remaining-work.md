# Current coverage and remaining work

Updated 19 October 2026. Audited source at d20c653, the cached official firmware
4.14 manual, retained fixtures, and the preceding session's user reports.
This is the current task checklist; older status documents preserve chronology.
No synth was connected or measured for this audit. A mapped field is not proof
of every UI path, acoustic result or parameter interaction.

## Implemented inventory

The source registry has **95 automatable parameters: 63 numeric and 32 choices**.
**94** have entries in the shared patch-decoder registry. **Input Gain** is the
one global parameter intentionally absent from ordinary patch decoding,
patch snapshots and Send All. Hardware MIDI Channel setup is a separate,
non-automatable command, not a 96th automation parameter.

| Registry section | Automatable parameters |
| --- | ---: |
| Master | 1 |
| Osc 1, including independent Sub tuning | 13 |
| Osc 2 | 9 |
| Mixer | 6 |
| Filter | 9 |
| LFO / Glide | 16 |
| Amp Env | 8 |
| Mod Env | 8 |
| FX / Perf | 17 |
| Arp / Seq | 7 |
| Global: Input Gain | 1 |

Source: BSIIMidi.h, BSIIPatchFields.h and PluginProcessor.cpp. MIDI Lab's
registry check compares the implemented IDs with all shared decoder fields.
The layout explicitly retains published host indices; do not append a numeric
parameter in a way that shifts the existing Paraphonic parameter at index 94.

## Completed feature groups and evidence limits

| Feature group | Current evidence | Remaining limit |
| --- | --- | --- |
| Original 76 controls | Retained patch captures, decoder regression, manual MIDI tests and targeted fixes | Exact Coarse display rounding, acoustic calibration and exhaustive combinations are not established |
| Arp/Seq six additions plus existing Swing | Live MIDI, 22 labelled patch fixtures, paused-RX Get Patch; basic Play/Record mode checks | No sequence-step editor or verified sequence-slot contents workflow |
| LFO Speed/Sync and both 35-entry division selectors | Full stored-value sampling; manual bidirectional mode/division and paused-RX checks; Ableton LFO1 1/4-to-1/8 automation reached the synth | Audible clock accuracy and broader automation remain untested |
| Two mod-wheel filter routes | All 256 raw sent-value/readback samples and hardware-emitted sweeps | Explicit installed-plugin UI and audible modulation acceptance still pending |
| MicroTune table selection 0–8 | All nine readbacks; basic hosted send/receive, paused-RX Get Patch | Table contents are not edited or transferred |
| Oscillator Error, Glide Divergence, Paraphonic | Retained captures, output/readback tests and basic hosted acceptance | Audible effects and broader automation/session checks remain |
| Independent Sub Fine/Coarse | All 458 sent-value/readback samples; basic hosted checks | Intermediate display rounding is approximate |
| Input Gain | Full emitted raw range; targeted incoming endpoints; hosted checks and patch isolation | No calibrated dB display, global query or power-cycle persistence test |
| Hardware MIDI Channel | Remote 1-to-2-to-1 and incoming Cutoff on 2; hardware-origin selector following with Live RX on | Other channels and remote-change persistence across power-off not hardware-verified |
| Presets, direct live input, automation and saved connection | Targeted Ableton VST3 and native regression checks, including editor-closed output and Set reopen | Other DAWs/AU host use and multiple devices/instances not broadly tested |
| Project sound recall | Stored preset and captured sound on Set reopen; stored-preset USB reconnect with editor closed and matched channel | Captured-sound/Off USB reconnect and post-reconnect automation still pending |

Evidence: [recall](project-sound-recall.md), [global controls](global-control-research.md),
[Arp/Seq](arp-seq-research.md), [LFO sync](lfo-sync-research.md),
[mod-wheel routes](modwheel-filter-research.md), [oscillator extras](osc-extra-research.md),
[Sub tuning](sub-tuning-research.md), [Input Gain](input-gain-research.md),
[host checks](plugin-smoke-test.md).

## Unfinished features

- [ ] **Global Tune:** establish a negative/zero/positive receive encoding.
  Hardware emits NRPN 64:6 with offset values, but replaying raw 14 produced
  +14 rather than -50. Standard RPN 0:1 is only an unverified candidate, not
  established BSII support. Keep a control unimplemented until receive works.
- [ ] **Local Control:** establish reliable Off and On reception/readback.
  CC122 Off was reported; attempts at On did not confirm success. A manual
  toggle emitted no messages in two captures. No plugin control yet.
- [ ] **Sequence slots and contents:** verify selection in Play/Record,
  backup/restoration, notes/rests/ties and persistence before attempting step
  editing. Random-access sequence read/write support is not established.
- [ ] **Tempo and clock:** decide scope for DAW clock/transport integration
  and verify routing. Internal tempo has no verified plugin control here.
  Existing LFO sync-division controls do not implement a clock generator.
- [ ] **Arp MIDI routing:** investigate the hardware Function + Latch toggle
  between external MIDI-to-Arp and Arp-to-MIDI; no verified remote command.
- [ ] **MicroTune hardware table workflow:** an [offline table editor and SysEx
  exporter](microtune-offline.md), plus a [local editor page](microtune-editor.md),
  are implemented. Actual hardware transfer, readback/restoration, persistent save
  remain pending; table selection is already implemented and hardware-tested.
- [ ] **Tuning imports:** investigate the Surge tuning library for `.scl`/`.kbm`
  and a separate `.tun` importer for u-he compatibility. Define reference pitch,
  unmapped keys and out-of-range notes in a preview before applying one undoable
  local edit. See [import research](tuning-import-research.md). Library browsing,
  scale generators and MTS-file import remain later work; no importer is installed.
- [ ] **Patch-bank management:** full-bank backup/import and any stored-slot
  writing need explicit design and validation. Current transfers target the
  temporary sound; no new stored-preset overwrite workflow is implemented.
- [ ] **AFX, last synth feature area:** overlay-bank selection, protection,
  per-note data editing, transfer and explicit save/clear operations. The manual
  documents dedicated messages; their presence does not verify our hardware
  implementation or justify treating overlay data as a normal 154-byte patch.
- [ ] **Final GUI:** redesign against the user's reference after features settle.

Performance notes, pitch bend, sustain and aftertouch remain part of DAW routing;
modulation-depth controls are implemented separately. Operating without Ableton
External Instrument via dedicated note forwarding/audio integration is an optional
future workflow, not a completed or agreed prerequisite for the editor.
Extended software modulation is also deferred; see the user's ideas document
before defining that scope.

## Next hardware session — short, ordered checks

Run with playback stopped initially, an expendable test Set, and the hardware,
plugin and External Instrument channels matched. Check the channel again after
power cycling; a successful SysEx reply does not prove channel matching.

1. [ ] Verify the final channel-1 Set save; use a fresh backup for any unsaved sound.
2. [ ] Finish mod-wheel filter UI/send/receive/Get Patch checks, then listen to
   both assignments on a simple patch. These are implemented, not missing routes.
3. [ ] Captured-sound USB reconnect with editor closed; inspect returned patch
   fields and listen. Check that Off does not recall and automation resumes.
4. [ ] Investigate remote MIDI-channel power-cycle persistence separately from
   immediate channel following; use a restorable starting configuration.
5. [ ] Run focused Global Tune and Local Control probes only after reviewing
   the existing evidence and planning manual restoration.
6. [ ] Investigate sequence slots/contents and clock behavior, then remaining
   tuning/bank workflows. Preserve data before any recording or stored write.
7. [ ] Implement and validate AFX after the other synth features.

Recall UI follow-up: “Restored stored preset” currently reports dispatch, not a
hardware acknowledgement. Clarify that wording or design a verification path;
do not imply that matching-channel readiness has been confirmed by that status.

Additional release checks: multiple plugin instances/devices; disconnect during
capture/recall/automation; flat automation segments and hardware recording;
AU in a real host, other DAWs and testers' computers. Existing native malformed
input and reconnect tests cover specific paths, not all real-world failures.

## Work possible without hardware

- [x] Record the latest user-confirmed recall/channel results and audit counts.
- [x] Research sequence access and tuning-table protocols against primary
  sources; see [sequence findings](sequence-access-research.md) and
  [MicroTune transfer plan](microtune-transfer-research.md). No documented sequence
  payload was found; tuning note-change/save commands provide a stronger codec
  candidate. Hardware acceptance remains pending.
- [ ] Research patch-bank transfer protocols separately.
- [ ] Extend offline fixture checks when a new reproducible case warrants it.
- [ ] Prepare release notes and a newer preview when requested. The public
  v0.8.2-preview.20261018 binaries remain the earlier 4993591/86-control build;
  updating source documentation does not update those binaries.

Existing architecture/OS support is unchanged: current public binaries target
Apple Silicon/macOS 27+. Wider support, Developer ID signing and notarization
remain deferred. This checklist makes no development-percentage estimate.

## Offline validation — 19 October 2026

`./tools/validate.sh` passed configure, the standalone/regression build and all
seven CTest groups (5.51 seconds for the test run). Report:
`validation-results/20261019-112719-32467/summary.txt` (local ignored output).
These checks sent no MIDI and required no synth. Registry extraction found 95
unique controls, 63 numeric/32 choices, and 94 shared patch fields; Input Gain
was the sole unmapped control. No runtime source, installed plugin or public
preview binary was changed. Native virtual-port integration, AU validation and
fresh AU/VST3 builds were not repeated for these documentation-only changes.

### Offline MicroTune increment — 19 October 2026

Added reusable table/document/encoding modules and a local create/edit/inspect/
export tool. Host parameters, MIDI device behavior and installed plugins are
unchanged. `./tools/validate.sh` now builds these targets and passes **eight**
CTest groups. The earlier seven-group report above remains historical evidence.
See [tool usage, limits and validation](microtune-offline.md).

### Local MicroTune page — 19 October 2026

Added note editing and local file load/save in a MicroTune tab. A separate working
document survives editor recreation and DAW-state save/restore, silently. The
hardware-selected table, patch/snapshot paths and 95 automation parameters remain
separate. See [page instructions and limits](microtune-editor.md). Hardware table
transfer/save is still pending, and the public preview is not automatically updated.

### Offline research tools — 19 October 2026

Added a compact `status` command with source-fingerprinted validation and an
`analyse-sweep` command for existing edit-buffer captures. The Glide Divergence
fixture provides a reproducible reference; reports retain evidence hashes and
separate inferred candidates from hardware claims. See [research toolkit](research-toolkit.md).
The suite now has nine registered test groups. This increment changes no plugin
runtime code and adds no tuning-editor or hardware-transfer features.

### Local tuning history and bulk editing — 19 October 2026

MicroTune now has Undo/Redo (64 document changes per processor instance) and
selection-based Set Offset, Add Cents and Reset Notes. Invalid operations reject
the entire selection. History survives editor recreation and resets on host state
restore; the current document remains embedded in the Set. Scala import and
hardware table transfer remain pending. See [editor use](microtune-editor.md).
