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


def bank_entries(report):
    """Recognize the observed export profile, not a verified write protocol."""
    if (not report['framingClean'] or report['bytes'] != 128 * 154
            or report['completeMessages'] != 128 or len(report['segments']) != 128):
        raise ValueError('Requires a clean 128-message, 19712-byte BSII export; no outside or real-time bytes.')
    entries = {}
    for wire_index, segment in enumerate(report['segments']):
        message = bytes.fromhex(segment['rawHex'])
        if (len(message) != 154 or not message.startswith(BSII_PREFIX)
                or message[6:8] != bytes([0, 1]) or message[9:13] != bytes(4)):
            raise ValueError(f'Message {wire_index} does not match the observed BSII export header.')
        index = message[8]
        if index in entries:
            raise ValueError(f'Duplicate export index {index}; bank is not complete.')
        entries[index] = dict(exportIndex=index, wireMessageIndex=wire_index,
                              messageSha256=digest(message), payloadSha256=digest(message[13:-1]))
    if set(entries) != set(range(128)):
        raise ValueError('Bank must contain every export index 0..127 exactly once.')
    return entries


def review_bank(path, against=None):
    source = inspect_file(path)
    entries = bank_entries(source)
    result = dict(schemaVersion=1, readOnly=True, source=source['source'],
                  sourceSha256=source['sha256'], bytes=source['bytes'], entries=128,
                  observedExportProfile=True, writeAddressingVerified=False,
                  transmitEligible=False, restorationVerified=False,
                  patches=[entries[i] for i in range(128)],
                  limitation='Export-index review only; no MIDI, restore commands or verified hardware destination mapping.')
    if against is not None:
        reference = inspect_file(against)
        previous = bank_entries(reference)
        changed = [i for i in range(128) if entries[i]['payloadSha256'] != previous[i]['payloadSha256']]
        result.update(reference=reference['source'], referenceSha256=reference['sha256'],
                      byteIdentical=source['sha256'] == reference['sha256'],
                      changedExportIndices=changed, changedEntries=len(changed), unchangedEntries=128-len(changed))
    return result


def bank_summary(report):
    lines = ['Read-only BSII export review: all 128 export indices present exactly once.',
             f"Bytes: {report['bytes']}; SHA256: {report['sourceSha256']}"]
    if 'changedEntries' in report:
        lines += [f"Compared by export index: {report['changedEntries']} changed, {report['unchangedEntries']} unchanged.",
                  'Changed export indices: '+(', '.join(map(str, report['changedExportIndices'])) or 'none'),
                  f"Files byte-identical: {report['byteIdentical']}"]
    return '\n'.join(lines + [report['limitation']])
