AnaMark Tuning Library formula-engine subset.
Upstream: https://github.com/zardini123/AnaMark-Tuning-Library
Revision: 9d6160141f53235807ac65381ab3839a35ab4856
Licence: MIT; see LICENSE.md.

Only CFormula and its string helpers are included. The upstream file reader,
multiple-scale reader and file writer are not used. BSII supplies MaxNumOfNotes=128
and validates file structure, numbers, references and results before application.

Local changes: TUN_StringTools.cpp includes charconv/cstdio explicitly and replaces
the two strtod/strtol Eval implementations with locale-independent from_chars.
The formula algorithm is unmodified. Keep these changes when updating upstream.
