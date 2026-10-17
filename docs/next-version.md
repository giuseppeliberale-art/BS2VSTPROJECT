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

## Linked Trigger selectors

Editing Trigger on either Amp Env or Mod Env now sets both selectors and sends
both trigger NRPNs to the hardware. Both pages label it 'Shared: Amp + Mod'.
This follows the requested front-panel-style workflow and observed switch behavior;
it does not claim every mixed-mode sound-engine interaction is established.
Distinct parameter IDs and incoming patch/snapshot values remain preserved.
Retrigger, Retrigger Count, Fixed Sustain and ADSR remain independently editable.

## Full signed Bend Amount

Bend Amount now supports -24 through +24 semitones, displays a plus sign for
positive values and accepts signed numeric entry. CC107 and stored byte16 use
semitones+64. Negative values no longer collide with missing-update sentinels in
live reception, SysEx application or duplicate-send tracking. Parameter ID remains
bendAmount; snapshot numeric values remain semitones. Expanding the parameter
range changes normalized host automation scaling; AU/VST3 migration is not verified.

## Signed modulation and clearer panel controls

Velocity-to-Amp/Mod displays -63…+63; Modwheel-to-LFO1/Osc2 and Aftertouch-to-
Filter/LFO1/LFO2 Speed display -64…+63. Numeric input converts back to existing
raw0…127 storage (velocity retains raw minimum1). Confirmed hardware Aftertouch
LFO2 Speed endpoints -64/+63 correspond to NRPN0:76 values0/127. No route or
snapshot storage change. Osc Sync now uses highlighted Off/On buttons with
canonical raw0/127; all nonzero received values show On. Sub Octave remains -1/-2.
Portamento label now reads Portamento / Glide Time. Checklist hardware instructions
now use overlay labels and the confirmed first-C Filter Tracking shortcut.

### Preset selection and refresh

- Preset selector 0–127, Previous/Next and reload. Selecting sends Program Change
  on the selected MIDI channel; does not write any stored preset.
- Hardware Program Change follows the selected channel and Live RX setting.
- Waits 200 ms after the latest selection before requesting the edit-buffer patch.
  Requests are serialized; a reply belonging to a superseded selection is discarded.
- Starts with unknown preset identity. Snapshot load, SysEx import/send and MIDI
  output/channel changes clear it. An edit-buffer dump alone cannot identify a
  stored preset slot.
- Hardware display preset 1 was captured sending raw Program Change 1 on channel 1.
  Remaining slot numbering follows the documented 0–127 range.
- Automated checks cover settling, rapid changes, stale response rejection,
  unknown state, bounds and millisecond counter wrap.

## Correct Unicode label encoding

Labels containing arrows, ellipses and the title dash now explicitly use UTF-8
conversion. JUCE's ASCII string constructor previously interpreted individual
UTF-8 bytes as characters, producing garbled symbols. This affects presentation
only; MIDI routes, parameter state and patch data are unchanged.
