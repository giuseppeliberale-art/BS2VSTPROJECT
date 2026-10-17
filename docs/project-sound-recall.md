# Project sound recall

Implemented 17 October 2026. The user confirmed stored-preset recall and captured-sound recall in Ableton; additional reconnect cases remain pending.

Local validation: all seven offline CTest groups PASS; native processor integration
PASS (fresh capture, stale-receipt exclusion, saved raw-patch roundtrip, one-shot
preset/captured recall before pending automation, invalid/truncated/other-command
refusal, missing capture, and timeout preserving the previous capture). Existing
processor automation/connection/host-notification checks remain passing. Release
Standalone/VST3/AU builds, installed signature verification and AU validation PASS.
Updated VST3/AU installed after the user saved/closed Live; prior bundles retained
in ignored `validation-results/20261017-project-recall/before/`. Stored-preset and captured-sound recall were subsequently confirmed by the user in Ableton. Cutoff automation was temporarily disabled to make the captured-sound check clear. USB power/reconnect remains unverified.

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
