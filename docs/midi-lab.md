# Local MIDI Lab

One reusable tool for the next preset and Arp/Seq work:
`python3 tools/midi-lab/lab.py`. Requires macOS, Python3.10+, Xcode Swift and the
project's existing CMake/JUCE setup. No third-party Python packages.

## What it does

Default mode listens for30seconds without creating a MIDI output port. It saves
original packet bytes, decoded events and a short report in ignored
`midi-lab-results/`. It identifies implemented controls from `Source/BSIIMidi.h`,
including paired8-bit CC and NRPN, and records raw Program Change numbers.
Arp CC108/109/111/118/119 and Seq Retrig NRPN0:106 are now recognized live-only controls. Generated references mark patch readback coverage separately: their SysEx offsets are not established. Recognition is not a hardware receive or UI pass.
Unmapped messages remain in the evidence rather than disappearing.

Reports say OBSERVED / NOT OBSERVED, not 'UI passed' or 'sounds correct'. Optional
`--channel 1` prevents another channel from satisfying an expected-control check.
Raw evidence always retains all channels. Per-channel NRPN selection, running
status, fragmented packets, interleaved clock and split SysEx are handled.

## Commands

Run these from the repository:

```bash
# Build the cached native transport and the actual C++ patch inspector once.
python3 tools/midi-lab/lab.py prepare

# Passive capture; no MIDI output. Ctrl-C stops and writes a report.
python3 tools/midi-lab/lab.py record --seconds 30 --channel 1 --expect bendAmount

# Re-analyse a previous JSONL packet recording or legacy hex-text capture offline.
python3 tools/midi-lab/lab.py replay /path/to/packets.jsonl --expect bendAmount

# Generate a current control/route reference directly from app source.
python3 tools/midi-lab/lab.py reference

# Read-only request: save the synth's current edit buffer; never overwrite a backup.
python3 tools/midi-lab/lab.py backup

# Offline: decode using the app's actual BSIIPatchDecoder, not a Python copy.
python3 tools/midi-lab/lab.py inspect /path/to/patch.syx
python3 tools/midi-lab/lab.py compare /path/to/before.syx /path/to/after.syx

# Offline: replay an existing sampler directory through that actual decoder.
python3 tools/midi-lab/lab.py check-sweep bend-amount /path/to/sweep-directory
```

`./tools/validate.sh` also builds the inspector and runs the MIDI Lab tests. Keep
it current after changing decoder headers: the lab refuses to use a stale inspector.

## Explicit hardware-changing commands

These modes are separate from passive recording:

```bash
# Sends sampled settings. Existing sampler backs up and restores the edit buffer.
python3 tools/midi-lab/lab.py sweep bend-amount

# Explicitly replace the current edit buffer with a chosen backup.
python3 tools/midi-lab/lab.py restore /path/to/baseline.syx
```

Sweep delegates measured sampling rules to `MIDISampler.swift`, then compares every
sample with the real C++ decoder. Normal completion requires the entire original
154-byte patch to match after restoration. On failure/interruption, the wrapper
stops the sampler process group and attempts verified recovery from THAT run's
backup. It retains logs/backup and reports an error if recovery fails; it cannot
recover through a disconnected synth, power loss or forced process termination.
No preset-save or bank-write command is used. Full SysEx restoration may show the
unexplained `---` indicator; previous tests still played and knobs updated the LCD.

Use an explicit test mode only when changing the current sound is appropriate.
Do not change presets or manually edit during a sweep. Hearing, LCD behavior and
app widgets still require separate checks. Starting a capture alone never proves
live reception in the app.

## Checked behavior

- Offline tests cover source-registry/decoder-field agreement, fragmented/running
  MIDI, per-channel NRPN and RPN cancellation, paired CC, Program Change, SysEx,
  absent expected messages, wrong channel, and failed recovery reporting.
- Actual decoder output is checked against a saved snapshot; truncated input fails.
- Replayed the old Bend capture:12 mismatches detected; corrected capture:0/12.
- Native transport compiled; interrupted passive recording stopped cleanly and
  saved its report. A brief passive connection succeeded with no incoming
  messages (correctly reported NO MESSAGES); read-only154-byte backup succeeded.
- Hardware-changing sweep recovery is simulated in tests, not newly exercised on
  the user's current patch. Existing sampler restoration has prior hardware evidence.

## Local manual reference

Official4.14 PDF and extracted text are cached under ignored `docs/vendor-cache/`:
`bsii-4.14.pdf`, `bsii-4.14.txt`, `next-phase-excerpts.txt`. PDF pages7 (patches),
18–19 (arp/seq) and24 (MIDI list) are indexed in `docs/local-reference.md`.
Vendor files stay local; only this index and project notes enter Git.
