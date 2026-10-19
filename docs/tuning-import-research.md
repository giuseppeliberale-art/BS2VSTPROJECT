# Tuning imports: library research

19 October 2026. Research only: no importer, dependency, plugin build or hardware
transfer was added. This note replaces assumptions in the pasted external chat
with checked upstream documentation and source. It is not a claim of full parser
conformance or a completed integration test.

## Recommendation

Use Surge's tuning-library as the candidate for Scala `.scl`/`.kbm` conversion,
behind a small local adapter. Evaluate `.tun` separately so u-he compatibility
stays an explicit requirement. Preserve the existing JSON document, editor, undo
history and silent application boundary. Build a preview before replacing notes.

Upstream revision inspected: `193669cdd27c17536a4e1d23dc001ae40194d400`.
Links below pin that revision for reproducible source review.

## What Surge provides

The standalone library is header-only C++20, with a CMake integration target. Its
README distinguishes this MIT-licensed library from the GPL synth projects. Our
project already uses C++20, so the stated language requirement fits; an actual
pinned integration/build test remains necessary. Preserve the upstream copyright
and permission notice when distributing its code.
[README](https://github.com/surge-synthesizer/tuning-library),
[licence](https://github.com/surge-synthesizer/tuning-library/blob/193669cdd27c17536a4e1d23dc001ae40194d400/LICENSE.md).

The public API separates a Scale, KeyboardMapping and calculated Tuning. It has
in-memory Scala parsers (`parseSCLData`, `parseKBMData`), frequency lookup and
TuningError exceptions. This lets our adapter keep file limits and errors outside
the editor's document mutation. Some implementation numeric parsing is permissive,
so successful parsing is not complete application validation: independently bound
file size and counts and validate reference fields and computed results. Source also exposes Ableton ASCL support, but that
is outside the proposed first import scope. No TUN or MTS-file parser appears in
the inspected public API. Adding this library does not add those formats.
[Public API](https://github.com/surge-synthesizer/tuning-library/blob/193669cdd27c17536a4e1d23dc001ae40194d400/include/Tunings.h).

## Mapping details we must handle

Scala's scale file describes intervals, not a complete keyboard/reference-pitch
assignment. The Scala specification points developers to the separate KBM format.
A ratio such as `2/1` and a cents value such as `1200.0` have different text syntax;
importing an SCL is not equivalent to reading 128 offsets.
[Scala file specification](https://www.huygens-fokker.org/scala/scl_format.html).

Surge's mapping contains reference note/frequency, scale start, key range and
skipped keys. Its API explicitly warns that default frequency values for skipped
notes are meaningless; check `isMidiNoteMapped` before accepting a result. Do not
automatically interpolate those notes, since that changes the intended mapping.
The inspected implementation parses firstMidi/lastMidi but does not use them to
restrict Tuning's calculated mapping: our adapter must enforce declared keyboard
bounds independently. For SCL without KBM, show an explicit scale-start and
reference note/frequency instead of silently adopting a library default.
[API](https://github.com/surge-synthesizer/tuning-library/blob/193669cdd27c17536a4e1d23dc001ae40194d400/include/Tunings.h),
[implementation](https://github.com/surge-synthesizer/tuning-library/blob/193669cdd27c17536a4e1d23dc001ae40194d400/include/TuningsImpl.h).

## TUN and u-he

u-he's official generator produces `.TUN` text. It offers repeating twelve-note
offsets, arbitrary interval/ratio sequences and a 128-note absolute-frequency editor.
That makes TUN import directly useful without recreating its generator first.
[Official generator](https://u-he.com/tools/microtuning/).

AnaMark's published version-2 format has scale sections and also defines `.MSF`
for multiple scale datasets. Do not equate all TUN files with a simple list of
128 frequencies, or promise every variant from a narrow parser. The maintained
AnaMark-library repository advertises TUN, MSF and Scala support with an MIT/X11
licence; it is an alternative to assess, not a dependency chosen by this research.
Inspect its actual parser, licence, build portability and fixtures before adoption.
[Official TUN specification](https://www.mark-henning.de/files/am/Tuning_File_V2_Doc.pdf),
[AnaMark library](https://github.com/zardini123/AnaMark-Tuning-Library).

MIDI Tuning Standard messages are a transfer representation. Ordinary `.mid`
note files, MTS SysEx files and TUN/Scala imports require different parsers. Our
existing documented BSII note-change encoder does not establish generic MTS-file
import or hardware table readback.
[Existing transfer research](microtune-transfer-research.md).

## Proposed import boundary (not implemented)

1. Read size-limited files and parse off the audio thread; report errors without
   changing the working table. Pin reviewed upstream sources and retain licences.
2. Preview all 128 keys with reference pitch, scale start, optional KBM mapping,
   target Hz and cents differences. Track unmapped and out-of-range keys separately.
3. Convert valid positive finite frequencies using
   `pitch = 69 + 12 * log2(hz / 440)`. This preserves the imported absolute pitch;
   it does not force the file's reference frequency to 440 Hz.
4. Respect our current absolute-pitch bounds 0–127. For example, 12-TET at A432
   lowers key 0 below the minimum; A442 raises key 127 above the maximum. Do not
   clip, wrap or silently drop those notes. Define an explicit import range and
   preserve-current-notes option, or reject the operation with affected keys shown.
5. Unmapped KBM keys require a visible policy. Our complete table cannot express
   mute/unmapped semantics. Keeping their existing tuning is possible only as an
   explicit choice; never label that as exact keyboard-map reproduction.
6. Apply the approved result through one `replaceLocalTuningDocument` call so
   import is atomic, undoable and silent. Keep hardware destination metadata distinct
   from source reference/mapping. The current version-1 JSON stores flattened
   pitches, not original Scala/TUN text; provenance or editable source definitions
   need a separately designed format extension.

Implementation acceptance should cover standard 12-TET, non-12 and non-octave
scales, ratio/cents syntax, explicit KBM reference and skipped/range-limited keys,
malformed inputs, unsupported boundary pitches, complete rollback, one-step undo,
JSON round trips and no MIDI output. Use author-generated fixtures and checked
reference values. Archive bundling/licensing and a searchable library are separate
work; public availability alone is not a redistribution permission check.

No new physical-synth test is needed to establish local parsing and preview, but
hardware tuning transmission and acoustic pitch accuracy remain unverified.

## Implementation follow-up

The first [local Scala importer](scala-import.md) is now implemented using the
pinned headers above. It includes preview, explicit reference fields, KBM bounds
and skip handling, unavailable-note preservation by explicit choice, and one-step
undo. This does not implement TUN, ASCL, MTS-file import or hardware transfer.

Numeric TUN v0/v1 support subsequently added: [format details](tun-import.md).
No second library was needed for that bounded numeric adapter. A subsequent
[TUN v2 increment](tun-v2-import.md) uses the AnaMark formula-engine subset, with
a documented locale-parsing patch and explicit handling of mapped/muted keys.
