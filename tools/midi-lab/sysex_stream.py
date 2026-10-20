"""Bounded, read-only binary SysEx inspection. No MIDI or process dependencies."""
from collections import Counter
import hashlib
from pathlib import Path

MAX_BYTES = 4 * 1024 * 1024
MAX_SEGMENTS = 4096
BSII_PREFIX = bytes.fromhex('F0 00 20 29 00 33')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def inspect_bytes(data):
    if not data or len(data) > MAX_BYTES:
        raise ValueError('Input must contain 1 byte to 4 MiB of binary SysEx.')
    if data.startswith(b'MThd'):
        raise ValueError('Standard MIDI files are not supported; use binary .syx.')
    segments, issues, seen = [], [], {}

    def append(start, end, kind, complete=False):
        if len(segments) >= MAX_SEGMENTS:
            raise ValueError('Input exceeds the 4096-segment inspection limit.')
        raw = data[start:end]
        entry = dict(offset=start, length=len(raw), kind=kind,
                     rawHex=raw.hex(' '), sha256=digest(raw))
        if kind == 'sysex':
            normalized = bytes(b for b in raw if b < 0xF8)
            entry.update(complete=complete, realtimeBytes=len(raw)-len(normalized),
                         normalizedHex=normalized.hex(' '), normalizedSha256=digest(normalized),
                         classification='unknown', transmitEligible=False)
            if complete:
                if normalized.startswith(BSII_PREFIX):
                    entry['classification'] = 'bsii-unknown-command'
                    # Shape only: no field decode, slot assignment or write permission.
                    if len(normalized) == 154 and normalized[6:13] == bytes(7):
                        entry['classification'] = 'bsii-edit-buffer-shape-candidate'
                entry['normalizedLength'] = len(normalized)
                if normalized in seen:
                    entry['duplicateOfSegment'] = seen[normalized]
                else:
                    seen[normalized] = len(segments)
        else:
            entry['realtimeOnly'] = all(b >= 0xF8 for b in raw)
            if not entry['realtimeOnly']:
                issues.append(dict(offset=start, code='outside-sysex-data', length=len(raw)))
        segments.append(entry)

    start, outside = None, 0
    for i, byte in enumerate(data):
        if start is None:
            if byte == 0xF0:
                if outside < i:
                    append(outside, i, 'outside')
                start = i
            continue
        if byte >= 0xF8:  # Real-time status may be interleaved; raw bytes stay intact.
            continue
        if byte == 0xF7:
            append(start, i+1, 'sysex', True)
            start, outside = None, i+1
        elif byte >= 0x80:
            append(start, i, 'sysex')
            issues.append(dict(offset=i, code='nested-start' if byte == 0xF0 else 'interrupted-sysex'))
            if byte == 0xF0:
                start = i
            else:
                start, outside = None, i
    if start is not None:
        append(start, len(data), 'sysex')
        issues.append(dict(offset=start, code='unterminated-sysex'))
    elif outside < len(data):
        append(outside, len(data), 'outside')
    messages = [s for s in segments if s['kind'] == 'sysex']
    if not messages:
        issues.append(dict(offset=0, code='no-sysex-messages'))
    counts = Counter(s['classification'] for s in messages if s['complete'])
    return dict(schemaVersion=1, readOnly=True, bytes=len(data), sha256=digest(data),
                framingClean=not issues, messageCount=len(messages),
                completeMessages=sum(s['complete'] for s in messages),
                duplicateMessages=sum('duplicateOfSegment' in s for s in messages),
                classifications=dict(counts), segments=segments, issues=issues,
                verifiedBank=False, transmitEligible=False,
                limitation='Framing/shape only; bank completeness, slot addresses and write eligibility are unverified.')


def inspect_file(path):
    path = Path(path)
    # Bound the read itself, including a file that grows between stat and read.
    with path.open('rb') as source:
        data = source.read(MAX_BYTES + 1)
    return dict(source=str(path.resolve()), **inspect_bytes(data))


def summary(report):
    lines = [f"Read-only SysEx inspection: {report['completeMessages']} complete / {report['messageCount']} framed messages",
             f"Bytes: {report['bytes']}; duplicate messages: {report['duplicateMessages']}",
             f"Framing: {'clean' if report['framingClean'] else 'ISSUES'}; SHA256: {report['sha256']}"]
    lines += [f"  {name}: {count}" for name, count in report['classifications'].items()]
    lines += [f"  byte {issue['offset']}: {issue['code']}" for issue in report['issues']]
    return '\n'.join(lines + [report['limitation']])
