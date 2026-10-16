# Local reference for preset / Arp / Seq work

Cached 2026-10-17 from [Novation's official4.14 user guide](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf).
The ignored vendor-cache directory contains the original PDF and extracted text.
Read these locally first; search online when the source is insufficient or newer
firmware facts are needed. If cache is absent, download that URL again and extract
with `pdftotext -layout`; do not assume an online manual's key positions match the
user's physical overlay. Verify printed labels and actual received MIDI.

| Topic | Local PDF page | Text lookup |
|---|---|---|
| Loading/saving patches |7|Loading Patches / Saving Patches|
| Arpeggiator |18–19|The Arpeggiator Section|
| Sequencer |19|The Sequencer|
| MIDI implementation |24|MIDI parameters list|

Transport facts for discovery (implementation and hardware behavior still to test):

| Feature | Documented message |
|---|---|
| Preset selection |Program Change, raw0…127; verify hardware display numbering|
| Arp On |CC108|
| Arp Latch |CC109|
| Arp Octaves |CC111|
| Arp Note Mode |CC118|
| Arp Rhythm |CC119|
| Seq Retrig |NRPN0:106|

Do not infer value encodings or SysEx bit positions for new controls from their
names or knob positions. The official table sometimes differs from observed
firmware4.14 ranges; retain raw observations alongside the documentation.
Tempo routing and preset-change refresh timing are not confirmed here.

Already verified display conventions: Bend=wire−64, full−24…+24. Velocity raw1…127
shows−63…+63. Wheel/aftertouch assignments raw0…127 show−64…+63; Aftertouch LFO2
Speed endpoints were confirmed physically despite the unsigned manual table.
Envelope Trigger edits are linked by user request; separate stored fields do not
prove independent audible behavior. AFX remains last.

Generate the full current implemented list using `lab.py reference` rather than
re-reading the entire source or assuming every BSII function is implemented.
