# Next version notes

## Live RX status wording

Requested on 16 October 2026 during hardware validation.

Show the current state using these exact labels:

- **Live RX is paused** when live CC/NRPN reception is paused.
- **Live RX is running** when live CC/NRPN reception is enabled.

The current checked control labelled “Resume Live RX” was confusing because it described an action while its tick represented the paused state. Use the requested status wording in the next version. SysEx reception remains active while Live RX is paused.

Implemented: the control now shows these status labels. A tick still means reception is paused; clicking toggles the state.

## Sub Octave switch and control visibility — 17 October 2026

Replaced the Sub Octave rotary presentation with connected −1/−2 buttons and an explicit selected-state highlight. The underlying `subOctaveRaw` parameter ID, 0–127 storage, CC81 route, and snapshot layout remain compatible. Raw values below 63 select −2; 63 and above select −1. The user identified hardware −2 while a fresh patch showed byte 37 bit 3 clear, establishing the label association. Clicking −1 sends 127; clicking −2 sends 0. Both selections were verified by fresh hardware patch readback; hardware ended at its starting −2 position. Clicking an already selected button keeps one state selected.

The standalone editor now starts at 900×760 and allows resizing (760×600 through 1600×1200). Control grids adapt to width; pages scroll vertically when their content is taller than the viewport. Patch Volume shares the Oscillator 1 section, placing its twelve controls in two visible rows at the default size. Visual checks confirmed both rows and their values are visible, and scrolling reveals the bottom FX control/value. MIDI, decoding, and snapshot data formats are unchanged.
