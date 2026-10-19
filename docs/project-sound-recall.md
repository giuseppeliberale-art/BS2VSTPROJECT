# Project sound recall

Implemented 17 October 2026. The user confirmed stored-preset recall and captured-sound recall in Ableton; additional reconnect cases remain pending.

Local validation: all seven offline CTest groups PASS; native processor integration
PASS (fresh capture, stale-receipt exclusion, saved raw-patch roundtrip, one-shot
preset/captured recall before pending automation, invalid/truncated/other-command
refusal, missing capture, and timeout preserving the previous capture). Existing
processor automation/connection/host-notification checks remain passing. Release
Standalone/VST3/AU builds, installed signature verification and AU validation PASS.
Updated VST3/AU installed after the user saved/closed Live; prior bundles retained
in ignored `validation-results/20261017-project-recall/before/`. Stored-preset and captured-sound recall were subsequently confirmed by the user in Ableton. Cutoff automation was temporarily disabled to make the captured-sound check clear. Stored-preset USB power/reconnect was subsequently confirmed as described below; captured-sound USB reconnect remains unverified.

## Behavior

The editor's **On reconnect** menu is an explicit, non-automatable session setting:

- **Off** (default, including old Sets): reconnect the port without sending a sound.
- **Stored preset**: remember the selected known preset number and send one Program
  Change on the saved channel when loading the Set or reconnecting its saved USB
  endpoint. Selecting another preset in the plugin updates this recall target.
- **Captured sound**: send the project's captured 154-byte current-edit-buffer patch.
  This recalls unsaved edits and survives later changes to stored hardware presets.
  It does not write a hardware preset slot.

**Capture Project Sound** requests a fresh current patch from the connected BSII.
It does not send the editor's parameter values to hardware. Wait for the capture
confirmation, choose Captured sound, and save the DAW Set. Further DAW saves retain
that same capture; neither automation nor Get Patch silently replaces it. Capture
again deliberately to change it. A failed/invalid capture preserves the previous
one. Capture is unavailable during the editor's preset refresh/Get Patch request;
preset selection/Get Patch is blocked while a project capture is pending.

Choosing a recall mode does not immediately replace the current sound. It arms
future session loading/reconnection. Stop/Play and opening/closing the editor do
not trigger recall. There is no new per-play preset-master behavior.

## Timing and connection

Recall belongs to the processor and runs without an editor. It precedes pending
parameter sends; those sends wait 300 ms for the hardware to settle. Queued host
parameter values are retained and then dispatched. This delay is not sample-accurate
scheduling and does not delay notes sent by Ableton External Instrument. Wait for
recall to finish before starting playback. Actual hardware timing still needs testing.

Device disappearance is checked once per second. Reappearance of the saved MIDI
endpoint triggers one new recall. The saved identifier is preferred; fallback uses
only one exact name match. If a power cycle does not remove/recreate a MIDI endpoint,
it cannot be detected this way. A missing device is retried. Two plugin instances
addressing one synth can compete; this feature does not arbitrate ownership.

## Stored data and validation

Host XML state includes recall mode, target program, and a base64 encoded raw
capture. Portable `.bsiistate` parameter snapshots do not contain these session
settings. Existing parameter identifiers and normalized ranges are unchanged.

Only the existing tested edit-buffer transfer format is accepted: 154 bytes,
Novation/BSII header, command/address bytes 6–12 all zero, a terminating F7 and
7-bit interior bytes. Other command types, truncated messages, or corrupt saved
captures cannot be replayed automatically. Capture has a 3-second timeout. The
request/reply protocol has no correlation ID, so unsolicited simultaneous dumps
cannot be distinguished from a reply; do not manually dump while capturing.

## Manual acceptance checklist

Use a disposable test Set and preserve any unsaved hardware sound first. No
hardware preset overwrite is needed for these tests.

- Stored preset: choose a preset in the plugin, choose Stored preset, save and
  quit Live. Select a different hardware preset, reopen the Set without opening
  the plugin editor, and confirm it loads the saved preset and automation works.
- Captured sound: stop playback, make one recognizable hardware edit, capture it,
  wait for success, choose Captured sound and save/quit. Select another hardware
  preset, reopen the Set, and verify the captured sound returns without opening
  the editor. Check the same controls with Get Patch and listen.
- Stop/Play several times after making a hardware tweak: verify it is not reset
  by playback restart. Automated controls will still follow their automation.
- Reopen an old Set with no recall metadata: verify no unsolicited sound recall.
- USB removal/reappearance with the editor closed: verify one recall on the same
  selected device, followed by normal automation; check the saved channel.

Exact hardware readback, audio and reconnect cases must be recorded separately
from local virtual-port test results. Do not call a stored preset overwritten or
saved until the synth's explicit hardware save action has actually been used.

## Stored-preset USB reconnect acceptance — recorded 19 October 2026

User-reported test completed in the preceding session, with Ableton playback
stopped and the plugin editor closed. USB removal powered the synth off.

- Initial configuration: Stored preset, target preset 1, plugin channel 1.
  After USB reconnect the hardware remained on preset 0, including after an
  additional ten-second wait. Opening the editor showed preset 1 and the status
  “Restored stored preset 1.” Load Preset, including after Refresh MIDI, did not
  change the hardware. Get Patch still received and decoded a patch.
- The user then opened the hardware MIDI Channel setting and observed channel 2.
  Changing the plugin communication channel to 2 allowed Load Preset to select
  hardware preset 1. This establishes a channel mismatch in the failed run;
  SysEx reception alone did not establish channel matching.
- With both synth and plugin on channel 2, the user saved the Set, closed only
  the editor, disconnected USB for at least five seconds, then reconnected it.
  After ten seconds the hardware displayed preset 1. Stored-preset recall on
  USB power/reconnection therefore passed this matched-channel, editor-closed
  scenario. This observation does not count transmitted recall messages or
  verify post-reconnect automation/audio, exact startup timing, or every channel.
- The user subsequently set hardware channel 1 manually. With Live RX running,
  both the plugin's top Channel selector and Global channel selector followed.
  Ableton External Instrument was confirmed on channel 1. A final Set save was
  requested but not separately confirmed after this last channel change.

Why the earlier remote channel-1 restoration did not survive power-off is
unresolved. Do not claim that Set Synth Channel persists across reboot, or that
recall automatically discovers a mismatched startup channel. The “Restored”
status was not an acknowledgement from the instrument in the failed run.
Captured-sound USB reconnect, Off-mode reconnect and post-reconnect automation
remain separate acceptance checks. No code change was made for these results.

## Local MicroTune document scope — 19 October 2026

The MicroTune page's local working document is now embedded separately in DAW
state, but is never transmitted by stored-preset or captured-sound recall. A patch
can restore a selected table ID without restoring that table's 128 pitch values.
The user must not treat a local document in the Set as automatic hardware tuning
backup/recall. See [MicroTune editor](microtune-editor.md).

## Dispatch status wording — 20 October 2026

Recall status now says “Sent recall for stored preset N. Hardware not confirmed.”
or “Sent captured sound to edit buffer. Hardware not confirmed.” These messages
report dispatch only. MIDI routing, recall timing, stored-preset numbering and
automation behavior are unchanged. This does not add acknowledgement/readback.
The change awaits the next installed plugin build.

Verification: rebuilt `BSIIProcessorAutomationTest` and its existing isolated
virtual-MIDI recall/automation regression passed. Local log:
`validation-results/recall-status-wording/processor-automation.log`. No new tests
were added for these label-only changes; broader tests and plugin rebuild/install
were not repeated.
