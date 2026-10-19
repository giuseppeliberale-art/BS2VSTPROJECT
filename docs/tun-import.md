# Numeric TUN import

19 October 2026. **MicroTune → Import Tuning...** accepts `.scl` or numeric `.tun`
files. JSON remains the native Load Table / Save Table format. No hardware tuning
transfer is added by this feature.

## Workflow

Choose a `.tun` file and press Preview. The dialog shows all 128 keys, current and
imported Hz, cents differences and unavailable notes. TUN supplies its own pitch
reference; manual reference controls and KBM selection are disabled. Choosing a
new source file clears the previous keep-unavailable choice and discards an old
KBM when switching to TUN.

Apply changes the local table and its name in one undoable operation, retaining
the chosen file-target slot. If any pitch is outside our current 0–127 application
range, Apply requires **Keep existing pitches for unavailable notes**. The previous
pitches of those keys are retained explicitly; no clipping or automatic octave
shift is performed. At least one available note is required.

You can edit the imported table, undo/redo, save it as BSII MicroTune JSON or retain
it in the DAW Set. The original file is never modified. Preview/cancel/file errors
do not change the document or transmit MIDI. Reference/document staleness checks
and dialog lifetime behave as in [Scala import](scala-import.md).

## Supported subset

This is a numeric TUN v0/v1 reader, not a complete AnaMark v2 implementation.
Supported musical data:

- `[Tuning]`: integer-cent note values using the standard TUN base reference.
- `[Exact Tuning]`: numeric/scientific cent values and optional positive `BaseFreq`.
  This section takes priority over `[Tuning]`, irrespective of their order.
- Default pitches and periodic completion defined for these numeric sections.
  Missing legacy notes use equal-tempered defaults. Exact Tuning fills unspecified
  notes below its highest explicit index with defaults, then repeats the indicated
  period when that index is below 127. A period specified only at note 0 is rejected
  as unsupported; export a complete table instead. An empty numeric section uses
  the defined default table, modified by BaseFreq for Exact Tuning.

Optional common `[Info] Name` strings are used as the table label; otherwise the
filename is used. Basic quoted-name escapes are supported; unsupported name
escapes fall back to the filename rather than changing the musical data. Unknown
metadata/sections are ignored. Known note assignments must be well formed;
duplicate note/base assignments use the last value. Repeated known sections or
multiple-scale datasets are rejected.

`[Functional Tuning]`, `[Mapping]`, a declared version other than 100, and multi-scale
files are rejected rather than importing lower-priority compatibility data. No
formula evaluation is performed. A valid AnaMark v2 file can therefore be rejected;
export numeric TUN v1 for this importer. `.msf`, MIDI files and generic tuning SysEx
import are outside this increment.

Input limits: 64 KiB, note indices 0–127, finite cents within ±120000, BaseFreq
0.000001–1000000000 Hz. Case-insensitive section/key names, comments, CR/LF/NUL
line delimiters, UTF-8 BOM and UTF-8/Latin-1 names are handled. These bounds and
subset restrictions are application choices, not claims of full TUN conformance.
Version-1 JSON stores flattened pitches, not original TUN expressions/metadata.

## u-he compatibility and sources

The current official u-he generator emits a complete numeric `[Exact Tuning]`
section with four-decimal cent values. It does not need a formula interpreter.
Its internal note numbering/base reference are offset together; the exported
standard default table imports with key 69 at 440 Hz, without a special octave
correction. Do not infer a correction from a single internal generator constant.
[Official generator](https://u-he.com/tools/microtuning/),
[export implementation inspected](https://u-he.com/tools/microtuning/assets/tungen.js?v=27523).

The interpretation of numeric sections, defaults, base reference, periodic
completion and precedence comes from the author's specification, sections 3.5–3.7.
[Official AnaMark format specification](https://www.mark-henning.de/files/am/Tuning_File_V2_Doc.pdf).
The parser is independently implemented from that description; no u-he generator
code or tuning preset collection is bundled. Tests use independently generated
numeric fixtures matching the export layout, not copied u-he application code.

## Verification

Offline tests cover full numeric tables, the 440 Hz reference, fractional edits,
last-assignment behavior, documented periodic/default completion, section priority
in both orders, explicit BaseFreq, comments/names/line endings, unsupported primary
sections, malformed numbers/keys and atomic application policies. Native editor
checks cover TUN selection, disabled reference/KBM controls, preservation reset,
preview and apply, one-step undo/redo, JSON round trips, unsupported-file rollback
and zero MIDI output. Physical tuning transfer remains separate work.

Validation report: `validation-results/20261019-215955-11958/summary.txt`.
All nine offline groups passed against the final source. Standalone/AU/VST3 builds
passed. Native editor checks in `validation-results/tun-import-editor/` passed
with zero failures; the TUN preview render was visually inspected. Existing native
processor automation/connection/receive checks also passed. There was no physical
MIDI output from the local import workflow. Host parameters remain at 95 with
unchanged indices. Actual user-file-chooser and DAW acceptance remains manual.

Installation was initially stopped before replacing either plugin because Live
was running. After the user saved and quit, process checks confirmed it was closed;
AU/VST3 builds were installed with prior bundles retained under the final report's
`installed-before/`. Installed signatures, bundled licence notices and Apple AU
validation passed. Public preview release assets were not replaced.
