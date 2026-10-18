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
