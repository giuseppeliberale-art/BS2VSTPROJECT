# Built-in sequence access research

Researched 2026-10-19 without hardware or MIDI transmission. Read the cached official 4.14 manual, `local-reference.md` and the complete `arp-seq-research.md` first. No runtime changes.

## What the manufacturer documents

The sequencer has four persistent memories, each up to 32 notes/rests. The SEQ selector chooses a memory before Record. Note/rest entry advances the displayed step; Rhythm supplies playback durations. Legato links neighbouring notes; repeating the same pitch with legato creates a tie. The manual says the selected sequence is stored with a patch, but provides no sequence payload layout. This wording alone does not establish whether a patch transfer contains the notes or a reference to a global slot. Incoming MIDI clock overrides internal Tempo; after its loss the rate continues until Tempo is adjusted. These are documented behaviours, not new hardware observations. [Official guide: Sequencer and Arpeggiator](https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail).

Function + Latch selects IN (incoming notes drive the arp) or OUT (generated arp notes are transmitted). No remote command for that routing switch is specified in this article. [Novation external-controller instructions](https://support.novationmusic.com/hc/en-gb/articles/206861739-Control-Bass-Station-II-s-Arp-from-External-Controller).

## Remote access: evidence boundaries

| Operation | Established evidence | Still unknown |
|---|---|---|
| Play/Record mode | Existing project captures and hosted tests: CC118 Play=6, Record=7 | Recording note content through external MIDI |
| Sequence slot | Hardware shares the Octaves/SEQ selector; current registry sends CC111 values 1–4 | Whether incoming CC111 selects the intended sequence memory in Play and Record |
| Note entry | Manual explains physical key entry | Remote note acceptance, velocity handling, length and overwrite rules |
| Rest and legato/tie | Manual explains physical Rest/Legato buttons | Whether CC109/108 perform those secondary actions in Record; any dedicated command |
| Sequence read/write/export | No dedicated route found in reviewed first-party sources | Step address, length, payload, slot requests, save/clear/copy commands |
| Playback phase | Existing Rhythm and Seq Retrig controls | MIDI Start/Continue/Stop response, position reporting and restart timing |

The [official MIDI appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix) lists Arp On CC108, Latch CC109, Rhythm CC119, Note Mode CC118, Octaves CC111 and Seq Retrig NRPN106. Its documented SysEx sections cover patches, AFX overlays and tuning; no sequence-transfer format is provided. Shared panel functions do not prove shared MIDI semantics. No first-party public BSII firmware/protocol implementation was located in targeted searches; third-party reverse engineering was not used as protocol authority. This bounded search cannot establish that undocumented access is impossible.

Existing empirical evidence is detailed in [Arp/Seq research](arp-seq-research.md), including the six decoded patch fields and manual checks. Those fields describe arp settings; they do not establish sequence-note contents. The 154-byte dump must not be advertised as backing up all four sequence memories without a content round-trip test.

## Implementation candidates

1. Keep the existing six controls; remote slot selection is the smallest next investigation. Label it unverified until distinct sequence contents identify each selected slot audibly.
2. If hardware accepts sequential note/rest/tie input, a whole-sequence upload workflow may be possible. That would still not provide random-access editing or readback. Do not implement CC108/109 recording actions from assumption.
3. A plugin-generated MIDI sequencer is a separate possible product feature, not an editor for the four built-in memories. Defer that decision until built-in access is understood.
4. Keep DAW clock routing separate from parameter automation. A direct clock sender would require deliberate scheduling, output ownership and transport tests; do not add a second clock source implicitly.

## Later hardware plan

1. Inventory and preserve existing sequences. If no reliable export exists, obtain approval for a disposable slot and transcribe valuable contents before overwriting anything.
2. In Play, compare physical slot selection with incoming CC111 using recognisably different existing sequences. Capture slot messages and patch dumps; repeat separately in Record only when safe.
3. In a disposable slot, test two MIDI notes, then physical Rest and Legato separately while capturing output. Only then test identified remote candidates, one action at a time. Confirm steps through display and playback.
4. Change one note while keeping slot/settings constant and compare edit-buffer and saved-patch transfers. Independently compare changing the selected slot. No guessed offsets or broad SysEx opcode probing.
5. Test one clock source over USB, then DIN separately: tempo following, held-note/latch playback, Start/Stop/Continue, reconnect and clock loss. Record what happens rather than assuming DAW transport starts the built-in sequencer.

No sequence editing, slot management or sequence backup capability has been newly implemented or verified by this research.
