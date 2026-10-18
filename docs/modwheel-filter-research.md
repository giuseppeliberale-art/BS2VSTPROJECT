# Mod-wheel filter controls — hardware discovery

2026-10-18, user's Bass Station II firmware 4.14, channel 1.

| Hardware on-key setting | Emitted NRPN | Raw range | Display |
|---|---|---|---|
| Mod Wheel / Filter Freq, lowest C (single press) | 0:94 | 0–127 | raw minus 64: -64 to +63 |
| Mod Wheel / LFO2 to Filter Freq, lower D | 0:71 | 0–127 | raw minus 64: -64 to +63 |

Passive captures requested a hardware minimum-to-maximum sweep; both recorded
all 128 unique raw values and the user explicitly confirmed displayed endpoints
-64 and +63. Mod-wheel movement itself was not the action being captured.

- First route: `midi-lab-results/20261018-051157-record-ed8a20/events.jsonl`.
- Second route: `midi-lab-results/20261018-051347-record-201bcb/events.jsonl`.

The cached official manual (`docs/vendor-cache/bsii-4.14.txt`, On-key Functions
and MIDI parameter table) documents these destinations and NRPN 0:71 for LFO2
filter modulation. The captured 0:94 route and -64 endpoint are hardware evidence;
do not infer another address or range from neighboring manual entries.

## SysEx and remote MIDI reception

Hardware-labelled endpoint comparisons first isolated these zero-based byte
offsets in the complete 154-byte message:

| Setting | Decoding expression | Verified raw inputs |
|---|---|---|
| Mod Wheel → Filter Frequency | `((b[82] & 0x1F) << 2) \| ((b[83] & 0x60) >> 5)` | 0–127 |
| Mod Wheel → LFO2 → Filter Frequency | `((b[84] & 0x07) << 4) \| ((b[85] & 0x78) >> 3)` | 0–127 |

Hardware dumps: initial both +63 in
`midi-lab-results/20261018-051448-backup-f4a167/baseline.syx`; first route set
to -64 in `20261018-051606-backup-43865a/baseline.syx`; second route also
set to -64 in `20261018-051705-backup-cdfcbf/baseline.syx`. Only the expected
two bytes changed for each action.

With Ableton playback stopped, the user authorized an automatic sweep. The
reusable sampler now accepts an optional `modwheel-filter` group:

`swift tools/LFOSyncSampler.swift NEW_OUTPUT_DIRECTORY modwheel-filter`

Run `midi-lab-results/modwheel-filter-discovery-20261018/` passed all 256
sent-value / patch-readback comparisons, checking every raw value and checking
for unrelated changed bits at every step. Complete starting-patch restoration
was verified byte for byte. No preset-save command was sent. Both controls
started and finished at raw 0 (display -64).

Retained fixtures: `tests/modwheel-filter-fixtures/`, containing each labelled
setting dump, before/after patches and samples.csv. These establish remote MIDI
reception and patch storage, not audible modulation behavior.

## Implementation batch

Added `modWhFilter` (NRPN 0:94) and `modWhLfo2Filter` (NRPN 0:71) to FX + Perf,
with signed -64…+63 display/input and raw 0…127 storage. Appended host indices
86/87 after the published 86 controls; the layout now explicitly preserves its
original 58 numeric controls followed by 28 choices before the new numeric
controls. Future additions must preserve that published order too. IDs, ranges
and indices of existing parameters are unchanged.

Both controls use the existing MIDI, automation, saved-state and Send All paths.
Verified decoder fields feed Get Patch using its existing host-silent refresh.
Offline fixture tests cover all 256 samples; native integration checks cover
host indices, sends, incoming updates without echo, session values and patch
refresh. Manual hosted UI/audible checks remain pending. Oscillator-extra mapping
is explicitly deferred rather than bundled into this implementation.

### Local validation — 18 October 2026

Standalone, AU and VST3 Release builds passed. All seven CTest groups passed,
including all 256 mod-wheel capture replays. Native processor automation and
direct SysEx checks passed. Logs: `validation-results/20261018-162141-16251/`.
The first installation attempt detected Live still running and stopped before
backing up or replacing plugins. User was asked to save and quit with Cmd+Q.
Installed-plugin validation is recorded below when completed. No new hardware
tests were requested after the user stopped testing for usage conservation.
