# Scala import into the local MicroTune table

19 October 2026. Import is local editing only. No synth or network is needed to
use it. It does not send tuning data or save hardware tuning slots.

## Use

1. In MicroTune choose **Import Scala...**, then **Choose .scl...**.
2. Optionally choose a `.kbm` keyboard mapping. It supplies its own scale start,
   reference key/frequency and mapping, so the manual reference fields are disabled.
   **No KBM** restores the manual controls.
3. Without a KBM, set the scale-start MIDI key, reference MIDI key and reference
   frequency. Defaults are scale start 60, reference key 69, reference 440 Hz.
4. Press **Preview**. Inspect all 128 keys: current Hz, imported Hz, change in
   cents and a per-note result. The summary shows the reference and available count.
5. If every note is available, **Apply to Local Table** is enabled. Otherwise
   review the unavailable notes and explicitly enable **Keep existing pitches for
   unavailable notes** if that is the desired result. At least one available note
   is required. This choice retains their current pitches, not necessarily equal
   temperament. They remain playable; it does not reproduce KBM muting.
6. Apply changes available pitches and the table name in one undoable action.
   The existing file-target slot is retained. Close the preview to continue editing
   or use Undo/Redo. Save Table writes the resulting BSII MicroTune JSON file;
   saving the DAW Set retains the working table as before.

Choosing files, previewing, cancelling or closing the dialog does not change the
working document. Changing files or reference fields invalidates the preview.
Changing the document elsewhere or restoring host state also makes an old preview
unusable; press Preview again. Closing the plugin editor discards the temporary
import session. A successful import is local unsaved work until saved.

## Formats, limits and semantics

- `.scl`: Scala interval scales with cents or ratios, including non-12-note and
  non-octave periods. Unison is implicit. A scale alone has no reference frequency
  or complete keyboard mapping; the explicit controls supply those.
- `.kbm`: optional key mapping, reference pitch, keyboard bounds and `x` skipped
  entries. Skipped keys and keys outside the declared range are flagged separately.
- The importer accepts files up to 64 KiB each, with 1–1024 scale steps or 0–1024
  keyboard-map entries. Ratio numerator/denominator must be positive integers up
  to 2147483647. Cents magnitude is limited to 120000 and the final period must
  be positive. KBM start/reference/range keys must be 0–127, formal octave and
  mapped degrees 0–scale-step-count, and reference Hz 0.000001–1000000000.
  Unsupported/malformed inputs show an error and keep the current document.
- Comments, CR/LF/CRLF line endings and UTF-8 BOM are handled. Scale descriptions
  support UTF-8 and Latin-1. Files with embedded NUL bytes are rejected.
- Our current table permits absolute pitch coordinates 0–127. Valid external
  tunings can exceed these application bounds: even 12-TET at A432 makes key 0
  unavailable, while A442 makes key 127 unavailable. Preview flags this and never
  clips meaningful pitch differences or silently transposes the scale. Only
  floating-point endpoint round-off below 1e-10 semitone is snapped.
- Imported absolute frequencies are preserved; converting to a MIDI pitch
  coordinate referenced to 440 Hz does not retune an imported reference to 440 Hz.
- Results are flattened into our existing 128-note document. Original interval
  expressions, source file paths and KBM skip/mute semantics are not embedded in
  version-1 JSON, and source files are never modified. Name comes from the scale
  description (up to 128 characters), with a fallback for empty descriptions.

The editor's normal Load Table still reads BSII MicroTune JSON. The separate
command-line file tool still reads/writes that JSON and exports documented
note-change SysEx. Neither now imports TUN, `.mid`, `.syx` or Ableton `.ascl`.
TUN compatibility with u-he remains explicitly pending.

## Dependency and validation

The importer uses unmodified Surge Synth Team tuning-library headers pinned to
`193669cdd27c17536a4e1d23dc001ae40194d400`. Its MIT notice is retained under
`Source/ThirdParty/tuning-library/` and copied into macOS product bundles.
Our adapter checks numeric syntax/counts before calling its parser, enforces KBM
key bounds separately and never reads skipped-note frequency placeholders.
[Research and upstream sources](tuning-import-research.md).

Offline checks cover identity, A432/A442 endpoints, non-octave ratios, 19 equal
steps, KBM ranges/skips/reference precision, malformed numbers/counts/ratios,
oversized files, encoding/line endings and atomic import policies. Native editor
checks additionally cover preview/apply, explicit preservation, one-step undo/redo,
JSON round trips, stale preview rejection, KBM control state, invalid input and
safe dialog destruction. A private virtual MIDI sink checks no physical output.
Physical tuning transfer and acoustic verification remain pending.

Final validation: `validation-results/20261019-214107-10042/summary.txt` records
all nine offline groups and the standalone build passing against the final source.
AU/VST3 builds passed; native editor tests in
`validation-results/scala-import-editor-final/` passed with zero failures,
including pending reference-notification rejection and dialog lifetime. Existing
native automation/connection/receive tests also passed. The preview render was
visually inspected. The first UI test attempted preview before JUCE's queued
text-change callback had settled; its event timing was corrected, and direct
reference-value comparison now additionally blocks applying stale field settings.
No debug instrumentation remains. Manual file-chooser/DAW acceptance remains a
user check; automated file methods are the same ones used by chooser callbacks.

Ableton was checked closed before AU/VST3 replacement. Prior bundles are retained
under that report's `installed-before/`. Installed signatures, bundled MIT notices
and Apple AU validation passed. Public preview release assets were not replaced.
