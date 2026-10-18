# Global-control discovery — 18 October 2026

## Local Control

User opened Function/Exit + Local once and selected Off then On, explicitly
confirming both labels on the hardware display. Passive capture
`midi-lab-results/20261018-170122-record-3819f5/` recorded zero MIDI messages.
Hardware was left On. This is a single no-transmission observation, not proof
that Local Control never emits MIDI in any configuration or that reception is
unsupported. Timing overlap was not independently measured.

The cached official manual's Global: Local section describes the on-key control
and On/Off behavior but provides no receive route in that section. No BSII-specific
CC/NRPN receive route has been verified. Do not claim a mapping or automatically
send a generic channel-mode message based only on its standard MIDI name.
Global readback, ordinary patch inclusion and hosted UI remain unverified.
No Local Control parameter has been added to the plugin.

## Global Tune — emitted MIDI verified

User opened Function/Exit + Tune once. Starting display was 0, endpoints -50/+50,
and user explicitly confirmed restoring display 0 after the sweep.
Capture `midi-lab-results/20261018-170313-record-d5b16b/events.jsonl` contains
NRPN **64:6** raw values 14–114, all 101 unique values. Final raw value was 64.
Display conversion is raw minus 64, matching the manual's cents range -50…+50.
This is NRPN MSB 64, not 0, and only Data Entry MSB was observed.

Remote reception, independent value-query support, ordinary patch inclusion and
plugin integration remain unverified. Do not assume the patch-based sampler can
restore this global setting. Hardware ended at its original Tune 0.

### Tune receive test failed; hardware restored manually

The scoped sender transmitted NRPN 64:6 with Data Entry CC6=65 (channel 1),
expecting +1 from the emitted raw-minus-64 model. User reopened Tune without
touching Patch/Value and observed **50**, not 1. This invalidates the assumed
simple symmetric receive encoding. The correct encoding and mismatch cause
remain unknown. No global-value query is verified yet.

User manually restored Tune to 0 and confirmed completion. No further tuning
test was sent. Keep Tune receive support disabled until independently verified;
do not use the emitted offset as a receive implementation specification. The
temporary Tune option was removed from the sender, retaining verified Input Gain.

## Global MIDI Channel — emitted changes captured

User performed the requested hardware channel 2 then channel 1 selection.
Capture `midi-lab-results/20261018-171002-record-f7e410/events.jsonl`
contains two matching groups:

- Channel 2: SysEx `F0 00 20 29 00 33 00 58 02 F7`, then NRPN 64:4, CC6=2 on channel 2.
- Channel 1: SysEx `F0 00 20 29 00 33 00 58 01 F7`, then NRPN 64:4, CC6=1 on channel 1.

Channel numbering here is human-facing 1–16. The NRPN notification uses the
newly selected channel. Final emitted channel was 1. Initial channel was not
reported separately by the user. Only values 1 and 2 were tested; remote
reception, other channel values, query support and persistence remain unverified.
Neither observed message format is yet a verified command to send to hardware.
No plugin control was added.

## MicroTune table selection — emitted values 0 and 1 verified

User opened the second Tune/MicroTune on-key function and reported starting
display `t 0`. Requested sequence was `t 0` to `t 1` then back to `t 0`,
followed by Function/Exit; user confirmed completion. Capture
`midi-lab-results/20261018-171137-record-600ae3/events.jsonl` contains
NRPN **0:0**, CC6=1 then CC6=0 on channel 1. No other messages were recorded.
Final emitted selection was 0. This verifies outgoing table selection only for
tables 0 and 1. Supported table range, incoming commands, tuning-table contents,
patch inclusion, persistence and query support remain unverified. No tuning
table contents were edited and no plugin control was added.

### MicroTune full hardware selection range

User confirmed maximum `t 8` and return to `t 0`. Capture
`midi-lab-results/20261018-171238-record-51d87d/events.jsonl` contains
NRPN 0:0 on channel 1 with CC6 sequence 1,2,3,4,5,6,7,8,7,6,5,4,3,2,1,0.
All nine selection values 0–8 are therefore observed, with final selection 0.
This resolves the hardware selection range left open above. Incoming command
support, patch inclusion, persistence and table contents remain unverified.

### MicroTune receive spot-check — documented RPN command

Official source: https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix
(Table selection section). The receive route is MIDI tuning program change
**RPN 0:3**, distinct from the emitted NRPN 0:0. Documented channel-1
sequence: `B0 64 03 65 00 06 tt 64 7F 65 7F` (hex; running status).
The guide says tt 0–9, whereas the observed hardware selector offers 0–8;
value 9 is untested and the scoped sender restricts values to 0–8.

`swift tools/MicroTuneSender.swift 1` sent CC100=3, CC101=0, CC6=1,
then CC100=127 and CC101=127 to clear RPN selection. User reopened
MicroTune without changing Patch/Value and explicitly confirmed display t 1.
This is a successful receive spot-check for table 1, not a full-range test.
Then `swift tools/MicroTuneSender.swift 0` completed to restore the starting
selection; user subsequently confirmed display t 0 after reopening MicroTune
without changing Patch/Value. Restoration is therefore confirmed.
No tuning-table contents or saved presets were written. The official guide
also states the active tuning table can be saved with a patch; local patch
encoding has not yet been established. No plugin control has been added.

### MicroTune ordinary patch storage — table 0/1 comparison

Read-only edit-buffer requests returned 154 bytes at table 0, after RPN selection
of table 1, and after restoring table 0. Retained under
`tests/microtune-fixtures/` as table-0-before.syx, table-1.syx and
table-0-restored.syx. The only 0-to-1 difference is zero-based byte 116,
0 to 16 (0x00 to 0x10). The restored dump is byte-for-byte identical to
the baseline. Thus table selection affects ordinary patch data; bit 0x10
at offset 116 is implicated by this pair. The complete field encoding for
values 2–8 is not yet established. No patch was sent back or saved to a preset
and no tuning-table contents were edited.

### MicroTune full selection field verified for 0–8

The scoped RPN sender selected tables 2–8; each was followed by an ordinary
154-byte edit-buffer request. Relative to table 0, table 2 changed byte 116
to 32, table 3 to 48, table 4 to 64, table 5 to 80, table 6 to 96, and
table 7 to 112. Table 8 instead changed byte 115 from 0 to 1. No other
bytes differed. Combined with the earlier table 1 fixture (byte 116 = 16),
all nine selections decode as:

`table = ((b115 & 0x01) << 3) | ((b116 & 0x70) >> 4)`

Offsets are zero-based. Fixtures table-2.syx through table-8.syx,
range-before.syx and range-restored.syx are retained in
`tests/microtune-fixtures/`. Final RPN selection of table 0 produced a dump
byte-for-byte identical to the range baseline. This verifies RPN reception
and ordinary patch readback for all nine supported selections. Receiving a
whole patch containing this field, host automation and UI integration remain
untested. No tuning-table contents or stored presets were written.

### Plugin integration and local validation

MicroTune Table 0–8 is now implemented in FX + Perf. New parameter index 88
follows all previously published controls. The receive registry retains NRPN
0:0; the output helper explicitly sends RPN 0:3 and nulls the RPN selection.
Processor input clears stale NRPN state on RPN selection. Patch decoding,
editor refresh, parameter state recall and normal automation paths include
the new field. Earlier statements that no control exists describe the
discovery stage and are superseded by this integration.

Validation report: `validation-results/20261018-171953-19614/`. All seven
offline CTest groups passed, including the nine retained hardware fixtures.
Native processor integration passed output bytes, host index preservation,
state recall, emitted NRPN reception and RPN/NRPN isolation. Direct SysEx
integration passed. AU/VST3 built and installed after checking Live was closed,
with previous bundles backed up. Both installed signatures passed strict
verification; Apple auval reported AU VALIDATION SUCCEEDED. Manual acceptance
in Ableton for this control remains pending. Public prerelease assets were
not replaced by this local installation.

### Manual Ableton acceptance passed

User verified the installed plugin in Ableton: selecting table 1 in the plugin
was confirmed as t 1 on the hardware display after reopening MicroTune without
editing Patch/Value. Changing hardware to t 2 updated the plugin automatically.
With Live RX paused, hardware selection t 8 left the plugin at 2; Get Patch
then updated the plugin to 8. User completed the requested plugin selection
back to 0 and resumed Live RX. No separate hardware-display confirmation was
requested for that final selection. These results complete basic hosted
send, receive, pause and SysEx readback acceptance; MicroTune automation
recording/playback and whole-patch transmission remain separately untested.

## Global Tune repeat receive probes

After user-confirmed hardware Tune 0, the scoped diagnostic sender repeated
NRPN 64:6 with CC6=65. User reopened Tune without touching Patch/Value and
again observed +50. User manually restored 0 and confirmed completion.

The next probe sent CC6=1, producing user-confirmed +1. A subsequent CC6=127
probe produced +50, not -1. User then manually returned Tune to 0, exited
the function, and confirmed completion. User also reconfirmed the hardware
range -50 through +50.

These are individual command/display observations, not a complete receive
mapping. They suggest positive direct values with an upper limit but do not
establish a universal clamp rule, negative encoding, 14-bit behavior, a
different RPN route, or a firmware defect. In particular, do not send raw 64
as a presumed neutral value or add a partially working signed Tune control.
No ordinary patch can verify or restore this global setting in our current
tools; restoration was manual and user-confirmed.

### Negative endpoint and exact-message round trip

At the user’s suggestion, a fresh Tune-zero edit-buffer dump was captured:
`midi-lab-results/20261018-225133-backup-df343a/baseline.syx`.
User set hardware Tune to -50 and exited the function. Passive capture
`20261018-225137-record-bd8895/events.jsonl` contains exactly 50 NRPN
64:6 values, CC6 descending 63 through 14, using only CC99, CC98 and CC6.
No Data Entry LSB or SysEx messages were captured.

The -50 dump `20261018-225214-backup-548844/baseline.syx` is byte-for-byte
identical to the zero dump (both 154 bytes). No ordinary patch field for
Global Tune was found in this comparison; this does not rule out other
global SysEx formats. These dumps and the emitted trace are retained in
`tests/global-tune-fixtures/`.

After user-confirmed manual restoration to zero, the diagnostic sender
replayed NRPN 64:6 CC6=14. User reopened Tune without editing and observed
+14, rather than the -50 represented by the hardware’s outgoing raw 14.
User manually returned Tune to 0 and exited, then confirmed completion.

Thus outgoing and incoming representations demonstrably differ for this
exact message. Negative receive encoding remains unresolved. No Global
Tune plugin control or speculative restore encoding has been added. The
temporary diagnostic sender was removed after the experiments. See
global-tune-protocol-research.md for the checked primary references.
