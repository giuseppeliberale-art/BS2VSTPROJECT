# Tuning display conversion investigation

## Evidence

Twenty saved hardware captures cover five positions for each oscillator’s Fine and Coarse controls. Fetching was performed with Live RX paused. Raw parameter storage and MIDI transmission have not been changed.

| Oscillator | Control | Position | Raw value |
| --- | --- | --- | ---: |
| 1 | fine | min | 27 |
| 1 | fine | minus50 | 77 |
| 1 | fine | centre | 128 |
| 1 | fine | plus50 | 178 |
| 1 | fine | max | 228 |
| 1 | coarse | min | 0 |
| 1 | coarse | minus6 | 63 |
| 1 | coarse | centre | 127 |
| 1 | coarse | plus6 | 193 |
| 1 | coarse | max | 255 |
| 2 | fine | min | 27 |
| 2 | fine | minus50 | 77 |
| 2 | fine | centre | 127 |
| 2 | fine | plus50 | 178 |
| 2 | fine | max | 228 |
| 2 | coarse | min | 0 |
| 2 | coarse | minus6 | 62 |
| 2 | coarse | centre | 128 |
| 2 | coarse | plus6 | 192 |
| 2 | coarse | max | 255 |

## Fine: candidate supported at every captured point

Candidate display in cents: raw < 127 → raw − 127; raw 127 or 128 → 0; raw > 128 → raw − 128. This maps observed 27/77/127/128/178/228 to −100/−50/0/0/+50/+100. Both centre codes must be accepted. A single subtraction of 128 would incorrectly show −101, −51, and −1 for three observations.

This is an inference from captured points, not a firmware-derived formula. Behaviour at uncaptured values and raw codes outside the observed 27–228 interval is not established. Do not silently clamp imported raw values or change parameter/MIDI ranges.

## Coarse: exact LCD conversion remains unresolved

Observed midpoint codes 127 and 128 both corresponded to hardware 0. Hardware +6.0 corresponded to raw 192 and 193; −6.0 corresponded to raw 62 and 63. Endpoints were 0 and 255. A linear map from 0–255 to −12–+12 rounded to one decimal yields +6.1 at 192, +6.2 at 193, −6.2 at 62, and −6.1 at 63, contradicting the reported LCD readings.

The samples establish direction and endpoints but do not uniquely identify display quantization, detents, or rounding. A lookup table interpolated through these points would overstate what has been verified. No exact Coarse display implementation is justified yet.

## Sources and next implementation boundary

Novation’s official MIDI table confirms Fine −100 to +100 and Coarse −12 to +12, with the documented paired CCs. It does not specify the full raw-to-LCD conversion formula.

[Novation MIDI parameter table](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix)

Keep display formatting separate from raw parameter storage, patch decoding, snapshot values, and MIDI output. Fine can be prototyped against the inferred centre-deadband rule, but should not be presented as an exact firmware match without broader evidence. Coarse requires an authoritative conversion table or a controlled raw-value/LCD sweep. No further manual knob sampling is needed to replay or compare the captures already saved.

## Implemented presentation

At the user’s request, both oscillators now format Fine as −100…+100 using the captured-point candidate above. Display values outside that observed musical range are clamped, but stored raw values are preserved. Entered Fine text is constrained to that musical range and converted back to a raw code; zero selects raw 128.

Coarse uses an explicitly approximate centred scale: raw below 127 maps as `(raw − 127) × 12 / 127`; raw 127 and 128 both display zero; raw above 128 maps as `(raw − 128) × 12 / 127`. Text is shown to one decimal and parsed back through the inverse scale, constrained to −12…+12. This guarantees requested endpoints and centre but is not an exact reproduction of the hardware LCD at intermediate positions. The control tooltip describes the approximation.

Parameter ranges, defaults, snapshot storage, SysEx decoding, and MIDI mapping remain raw and unchanged. Tests replay all Fine captures, check endpoints and both zero codes, and check conversion round trips over every raw code (allowing the shared zero).
