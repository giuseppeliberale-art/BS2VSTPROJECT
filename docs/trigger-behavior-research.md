# Envelope triggering: documentation and hardware evidence

Research date: 2026-10-17. User reports firmware 4.14. No code or synth changes.

## Conclusion

Keep the independent Amp and Mod Trigger controls. Official Novation documentation
lists separate MIDI parameters. Our patch readbacks confirm that this unit can
retain different settings. Independent audible triggering still needs a listening
test; separate stored values alone do not prove every musical interaction.

The stronger statement that the hardware switch always changes both envelopes,
regardless of Env Select, is **not established by the official documentation**.
It is an observation from the user's three guided tests on this unit. Avoid
presenting it as a universal manufacturer specification.

## Official sources

- [Novation: Bass Station II in detail](https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail),
  “The envelopes section”: Env Select assigns ADSR sliders to Amp, Mod or both.
  The Single/Multi descriptions refer to the “selected envelope(s)”. This wording
  suggests selection matters, but does not explicitly describe the MIDI messages
  transmitted when the Triggering switch moves. It does not resolve our observed
  discrepancy.
- [Novation: Bass Station II appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix),
  “MIDI parameters list”: Amp Env Triggering is NRPN 0:73; Mod Env Triggering is
  NRPN 0:105. Separate addresses support individual MIDI control. The table labels
  the range 1,2,3, whereas our measured wire values are 0,1,2; retain the tested
  encoding rather than silently treating the published range as byte-accurate.
- [Official User Guide 4.14 PDF](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf):
  downloaded locally after browser extraction exceeded the tool's size limit.
  Its envelope section also uses the same selected-envelope wording, and its MIDI
  table lists the same separate addresses and 1,2,3 range. Therefore the ambiguity
  exists in the 4.14 guide too. Online pages were accessed 2026-10-17; the detail
  page displays an update date of 2025-04-29.

## Local evidence

See [manual-test-review.md](manual-test-review.md) and the capture paths recorded
there. All three user-confirmed Env Select positions produced both trigger NRPNs
when the physical Triggering switch moved:

| Env Select | Switch movement | Captured wire values |
|---|---|---|
| Amp | Multi → Single | Amp 1, Mod 1 |
| Mod | Single → Multi | Amp 0, Mod 0 |
| Amp+Mod | Multi → Single | Amp 1, Mod 1 |

With no hardware switch movement, an app edit followed by patch readback gave
Amp=Multi / Mod=Single. A subsequent Mod-only edit gave Amp=Multi /
Mod=Autoglide. These establish independent storage for the two tested combinations.

The captures establish outgoing MIDI behavior on this tested unit and setup;
they do not establish a firmware-wide guarantee, the physical LED's semantics,
or audible independence. No routing fix is justified by the current evidence.

## Follow-up search: intended behavior versus possible fault

Further targeted searches for switch-both behavior, independent triggering and
triggering bugs did not locate an explicit Novation statement establishing the
paired physical-switch transmission as intended. Absence of a found statement
is not proof of a unit defect. The official overview assigns Env Select specifically
to the ADSR faders; detailed Single/Multi text still says selected envelope(s).
Next diagnostic control: repeat passive capture with BSII Control and other MIDI
routing apps closed, before attributing paired output solely to synth firmware.
Then compare another unit on firmware 4.14 or request Novation clarification.
No support request has been sent and no normal/defective verdict is established.

## Isolated capture follow-up (app closed)

BSII Control PID30270 was terminated and exit verified. Passive recorder has no
MIDI output port. User reports Amp selected; photo appears consistent with top
Amp position. First physical move Single→Multi sent ONLY NRPN0:105 value0.
Prior readback state was Amp Multi / Mod Autoglide, so only Mod needed to change.
A later physical move Multi→Single sent BOTH NRPN0:73 value1 and0:105 value1.
Capture: chat workspace `work/midi-debug/trigger-app-closed.txt` (still collecting).
This disproves the universal wording 'always sends both on every switch move'.
Hypothesis: physical switch applies a common mode, transmitting only parameters
whose stored values change. This explains both events but requires controlled
repetition; it is not established manufacturer intent or a hardware-fault verdict.

## Isolated Mod-selected switch and readback

Keeping the app closed, user selected Mod only and moved Single→Multi. Both
NRPN0:73=0 and0:105=0 were captured. A fresh read-only SysEx request confirmed
both stored fields Multi (Amp wire0, Mod wire0). Passive recorder then stopped.
Evidence: trigger-app-closed.txt and trigger-app-closed-mod-multi/baseline.syx
under the chat workspace work/midi-debug directory. This excludes BSII Control
feedback for this result and shows actual stored shared updating, not just LEDs.
Still no explicit manufacturer statement found establishing intended switch behavior.

## Audible tests refine the conclusion

Controlled simple patch on user's4.14 unit: Amp Single / Mod Multi did NOT restart
Mod pitch sweep on overlapping higher note; Amp Multi / Mod Single DID restart it.
Second combination verified by fresh readback. Both observations are user listening
reports, not an instrumented audio measurement. Amp Trigger appears to govern the
observed ModEnv legato restart here. Independent stored NRPN/SysEx settings should
not be described as verified independent sound-engine behavior. Manufacturer intent
and wider mode/firmware behavior remain unresolved.
