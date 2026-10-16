# Snapshot and Send All validation — 17 October 2026

Tested the standalone app built from `72e3fed`, connected to the Bass Station II on MIDI channel 1, with Live RX paused. Firmware version was not recorded. No application feature changes were needed.

## Snapshot round-trip

1. Requested the current synth patch and saved it separately as a 154-byte SysEx checkpoint.
2. Used the app's Save Snapshot control to save the decoded baseline editor state.
3. Prepared a valid alternate snapshot changing 75 of the 76 exposed controls. Kept Filter Type at Classic to avoid Acid's intentional forced LP/24 dB normalization. Every changed value remained inside the current numeric/choice bounds.
4. Loaded the alternate snapshot through the app's Load Snapshot control, then saved it again through Save Snapshot.
5. Compared all persisted parameter IDs and values: **82/82 entries identical**. This includes the 76 exposed controls plus six legacy tree entries; their presence is retained rather than treated as additional exposed controls.
6. Requested a hardware patch independently: **the complete original patch was unchanged** after loading and saving the alternate snapshot.
7. Loaded the original baseline snapshot, saved it again, and compared: **82/82 entries identical**.

Comparison ignores root metadata (`savedAt`, version marker), child ordering, and numeric text formatting such as `0` versus `0.0`. It preserves the distinction between an entry with no value attribute and a numeric entry. The legacy entries are `filterShapeRaw`, `filterSlopeRaw`, `filterTypeRaw`, `lfo1WaveRaw`, `lfo2WaveRaw`, and `subWaveRaw` (the last has no value attribute). The older snapshot version marker remains unchanged; no migration was introduced.

## Actual app Send All

- With the baseline snapshot restored in the editor, clicked **Send All → BSII**, requested a new hardware patch, and compared all 154 bytes with the checkpoint: **identical**.
- Loaded the alternate snapshot and clicked **Send All → BSII**. Captured the returned patch, then immediately restored the saved original SysEx and verified all 154 bytes: **identical to the original**.
- Replayed the alternate hardware response through the actual C++ decoder and compared all **76 exposed controls** with the alternate snapshot: **76/76 match**. The baseline snapshot/patch pair also matches all 76 controls.
- Requested the restored synth patch through the app's Get Patch control, returning the editor to the original values.

The alternate state changes many controls simultaneously and includes endpoint values, both main waveform choices, envelope options, Fine limits, bend's offset encoding, and raw switch states. The unchanged Filter Type is still compared. This covers one combined Classic-mode recall, not every combination, Acid recall, or every intermediate value.

## Retained evidence and replay

`tests/state-fixtures/` contains the baseline/alternate snapshots, their app-saved round-trip counterparts, the baseline SysEx, and the alternate Send All hardware response. `tests/SnapshotRecallTest.cpp` checks snapshot parameter equality and all 76 decoded recall values. These are offline replays of actual app/hardware results; they do not automate the UI or drive a connected synth during CTest.

`tools/PatchCheckpoint.swift OUTPUT_DIRECTORY capture|compare|restore` can save, compare, or restore a full edit-buffer checkpoint on channel 1. Capture refuses to overwrite an existing `baseline.syx`; restore validates the expected 154-byte BSII single-patch format and verifies the returned patch byte-for-byte. It does not send a patch-save command.

Build and run all retained checks:

```sh
cmake -S . -B build -G Xcode -DBSII_BUILD_TESTS=ON
cmake --build build --config Release --target BSIIMidiRoutingTest BSIIOsc1RangeTest BSIISnapshotRecallTest -- CODE_SIGNING_ALLOWED=NO
ctest --test-dir build -C Release --output-on-failure
```

All six registered checks passed. Standalone UI save/load and actual Send All were exercised; AU/VST3 host recall, physical hardware-origin live CC reception, disconnect/reconnect, and malformed snapshot handling remain separate work.
