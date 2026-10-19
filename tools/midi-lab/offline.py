"""Offline research reports. Never imports a MIDI bridge or opens device ports."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess
from datetime import datetime, timezone


def git(root, *args):
    return subprocess.run(['git', '-C', str(root), *args], capture_output=True, text=True, check=True).stdout.strip()


def fingerprint(root):
    """Content identity of build/test inputs; independent of documentation-only commits."""
    root = Path(root)
    paths = [root/'CMakeLists.txt']
    for folder in ('Source', 'tests', 'tools'):
        paths.extend(p for p in (root/folder).rglob('*')
                     if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc')
    digest = hashlib.sha256()
    for path in sorted(paths):
        relative = path.relative_to(root).as_posix().encode()
        contents = path.read_bytes()
        digest.update(len(relative).to_bytes(8,'big')); digest.update(relative)
        digest.update(len(contents).to_bytes(8,'big')); digest.update(contents)
    return digest.hexdigest()


def validation_event(root, directory, action, label=None, verdict=None):
    directory = Path(directory)
    path = directory/'validation.json'
    if action == 'start':
        report = dict(schemaVersion=1, commit=git(root,'rev-parse','HEAD'),
                      started=datetime.now(timezone.utc).isoformat(),
                      initialFingerprint=fingerprint(root), platform=platform.platform(),
                      scope='Standalone build and offline CTest only', complete=False, stages={})
    else:
        report = json.loads(path.read_text())
        if action == 'stage': report['stages'][label] = verdict
        if action == 'finish' or verdict == 'failed':
            report['complete'] = True
            report['finished'] = datetime.now(timezone.utc).isoformat()
            report['finalFingerprint'] = fingerprint(root)
            import re
            logfile = directory/'tests.log'
            matches = re.findall(r'(\d+)% tests passed(?:, (\d+) tests failed)? out of (\d+)', logfile.read_text() if logfile.exists() else '')
            if matches:
                percent, failed_text, total_text = matches[-1]
                total = int(total_text)
                failed = int(failed_text) if failed_text else (0 if int(percent)==100 else total)
                report['tests'] = dict(total=total, failed=failed, passed=total-failed)
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(report,indent=2)+'\n')
    temporary.replace(path)
    return report


def status(root, controls):
    root = Path(root)
    current = fingerprint(root)
    paths = list((root/'validation-results').glob('*/validation.json'))
    validation = dict(status='unverified', reason='No fingerprinted validation report. Legacy summaries are not current-source proof.')
    if paths:
        latest = max(paths,key=lambda p:p.stat().st_mtime_ns)
        try:
            report = json.loads(latest.read_text())
            if not isinstance(report,dict) or not isinstance(report.get('stages',{}),dict) or not isinstance(report.get('tests',{}),dict):
                raise ValueError('Malformed report fields')
            validation = dict(status='unverified', report=str(latest.relative_to(root)), scope=report.get('scope'),
                              testedCommit=report.get('commit'), tests=report.get('tests'))
            stages = report.get('stages',{})
            if report.get('schemaVersion') != 1:
                validation['reason'] = 'Unsupported validation report schema.'
            elif report.get('initialFingerprint') != current:
                validation.update(status='stale',reason='Build/test inputs changed since this report started.')
            elif report.get('platform') != platform.platform():
                validation.update(status='stale',reason='Validation environment differs from this machine.')
            elif not report.get('complete'):
                validation.update(status='incomplete',reason='Validation has not recorded completion; it may have stopped.')
            elif report.get('finalFingerprint') != current:
                validation.update(status='stale',reason='Build/test inputs changed during validation.')
            elif any(value == 'failed' for value in stages.values()):
                validation.update(status='failed',reason='A recorded validation stage failed.')
            elif set(stages) == {'Configure','Build standalone and regression tests','Regression tests'} and all(v=='passed' for v in stages.values()) and report.get('tests',{}).get('total',0)>0 and report['tests']['failed']==0:
                validation.update(status='passed',reason='All scoped stages passed against identical current inputs.')
            else:
                validation['reason'] = 'Report does not establish all required stages and test counts.'
        except (ValueError,KeyError,TypeError,OSError):
            validation = dict(status='unverified',report=str(latest.relative_to(root)),reason='Malformed validation report.')
    return dict(commit=git(root,'rev-parse','--short','HEAD'), workingTreeClean=not bool(git(root,'status','--porcelain')),
                inputFingerprint=current,
                controls=dict(implemented=len(controls),patchMapped=sum(c['patchMapped'] for c in controls),
                              outsidePatch=[c['id'] for c in controls if not c['patchMapped']]),
                validation=validation,
                hardware='Not inferred from offline tests; see docs/remaining-work.md.',
                host='Native/AU/DAW and installation results are not certified by this offline summary.')


def status_text(report):
    controls,validation = report['controls'],report['validation']
    count = validation.get('tests') or {}
    counts = f" ({count.get('passed')}/{count.get('total')})" if count else ''
    return '\n'.join([
        f"BSII Control | {report['commit']} | working tree {'clean' if report['workingTreeClean'] else 'modified'}",
        f"Controls: {controls['implemented']} implemented; {controls['patchMapped']} patch-mapped; outside patch: {', '.join(controls['outsidePatch'])}",
        f"Local validation: {validation['status'].upper()}{counts}",
        validation.get('reason',''), 'Scope: standalone build + offline CTest; not hardware/DAW acceptance.',
        'Report: '+validation.get('report','none'), 'Next work: docs/remaining-work.md'])


def infer_bits(samples):
    """Candidate direct/permuted/inverted unsigned bitfields; no guessed decoder writes."""
    width = max(value for value,_ in samples).bit_length()
    train = [i for i in range(len(samples)) if len(samples)<8 or i%4!=3]
    held = [i for i in range(len(samples)) if i not in train]
    varying = [(byte,bit) for byte in range(len(samples[0][1])) for bit in range(7)
               if len({(data[byte]>>bit)&1 for _,data in samples})>1]
    matches = []
    for value_bit in range(width):
        expected = [(samples[i][0]>>value_bit)&1 for i in train]
        options = []
        if len(set(expected)) > 1:
            for byte,bit in varying:
                observed = [(samples[i][1][byte]>>bit)&1 for i in train]
                if observed == expected: options.append(dict(byte=byte,bit=bit,inverted=False))
                if observed == [1-x for x in expected]: options.append(dict(byte=byte,bit=bit,inverted=True))
        matches.append(dict(valueBit=value_bit,options=options))
    unique = width>0 and all(len(row['options'])==1 for row in matches)
    if unique:
        assigned = {(row['options'][0]['byte'],row['options'][0]['bit']) for row in matches}
        unique = len(assigned)==width and assigned==set(varying)
    candidate = None
    if unique:
        mapping = [dict(valueBit=row['valueBit'],**row['options'][0]) for row in matches]
        def reconstruct(data):
            return sum((((data[p['byte']]>>p['bit'])&1)^p['inverted'])<<p['valueBit'] for p in mapping)
        checks = [reconstruct(data)==value for value,data in samples]
        candidate = dict(bits=mapping,matches=sum(checks),samples=len(samples),
                         trainingSamples=len(train),heldOutSamples=len(held),heldOutMatches=sum(checks[i] for i in held),
                         fitsAllSamples=all(checks))
    return dict(candidate=candidate,bitCandidates=matches,
                limitation='Only unsigned bit placement/inversion is inferred. Clamping, offsets, interactions and causal hardware behaviour are not inferred.')


def analyse_sweep(directory, inspector):
    directory = Path(directory).resolve()
    manifest_file = directory/'experiment.json'
    if manifest_file.stat().st_size > 1048576: raise ValueError('Experiment manifest too large')
    manifest = json.loads(manifest_file.read_text())
    if not isinstance(manifest,dict) or manifest.get('schemaVersion')!=1 or manifest.get('stateDomain')!='edit-buffer':
        raise ValueError('Only schema-1 edit-buffer capture manifests are supported; globals/tuning/sequence recovery is not implied.')
    if not isinstance(manifest.get('id'),str) or not manifest['id'] or 'baseline' not in manifest:
        raise ValueError('Manifest requires an id and baseline filename')
    if manifest.get('comparisonField') is not None and not isinstance(manifest['comparisonField'],str):
        raise ValueError('comparisonField must be a string')
    rows = manifest.get('samples')
    if not isinstance(rows,list) or not 2<=len(rows)<=512: raise ValueError('Need 2..512 labelled capture samples')
    def captured(name):
        if not isinstance(name,str): raise ValueError('Capture filename must be a relative string')
        path = (directory/name).resolve()
        if Path(name).is_absolute() or not path.is_relative_to(directory): raise ValueError('Capture path escapes experiment directory')
        data = path.read_bytes()
        if len(data)!=154 or data[:6]!=bytes([240,0,32,41,0,51]) or any(data[6:13]) or data[-1]!=247 or any(b>=128 for b in data[1:-1]):
            raise ValueError('Capture is not a framed 154-byte current edit-buffer patch: '+name)
        return path,data
    baseline_path,baseline = captured(manifest['baseline'])
    after = captured(manifest['restored']) if manifest.get('restored') else None
    if after and after[0]==baseline_path: raise ValueError('Restored evidence must be a separate file from baseline')
    samples = []
    paths = []
    evidence = []
    for row in rows:
        if not isinstance(row,dict) or 'value' not in row or 'file' not in row:
            raise ValueError('Every sample requires value and file')
        value = row['value']
        if type(value) is not int or not 0<=value<=65535: raise ValueError('Input labels must be unsigned integers 0..65535')
        if 'expected' in row and type(row['expected']) is not int: raise ValueError('Expected decoder values must be integers')
        path,data = captured(row['file'])
        if path in paths: raise ValueError('A capture file cannot count as multiple independent samples')
        paths.append(path); samples.append((value,data))
        evidence.append(dict(file=row['file'],sha256=hashlib.sha256(data).hexdigest()))
    decode_paths = [baseline_path]+paths
    if after: decode_paths.append(after[0])
    decoded = []
    for start in range(0,len(decode_paths),64):
        result = subprocess.run([str(inspector),*map(str,decode_paths[start:start+64])],capture_output=True,text=True)
        if result.returncode: raise ValueError('Production decoder rejected capture: '+(result.stderr or result.stdout)[:200])
        decoded.extend(json.loads(line) for line in result.stdout.splitlines())
    if len(decoded)!=len(decode_paths) or any(not d.get('valid') for d in decoded): raise ValueError('Incomplete/invalid production decoder results')
    field = manifest.get('comparisonField')
    if field and any(field not in d['fields'] for d in decoded): raise ValueError('Comparison field is absent from production decoder')
    unrelated,mismatches = [],[]
    for index,(row,patch) in enumerate(zip(rows,decoded[1:1+len(rows)])):
        if field and patch['fields'][field]!=row.get('expected',row['value']):
            mismatches.append(dict(sample=index,expected=row.get('expected',row['value']),actual=patch['fields'][field]))
        changes = {k:dict(before=v,after=patch['fields'][k]) for k,v in decoded[0]['fields'].items()
                   if k!=field and patch['fields'][k]!=v}
        if changes: unrelated.append(dict(sample=index,fields=changes))
    changed = []
    for byte in range(154):
        mask = 0
        for _,data in samples: mask |= data[byte]^baseline[byte]
        if mask: changed.append(dict(byte=byte,mask=f'0x{mask:02X}'))
    inference = infer_bits(samples)
    restoration = dict(recorded=bool(after),byteIdentical=(after[1]==baseline if after else None),
                       baselineSha256=hashlib.sha256(baseline).hexdigest(),
                       afterSha256=hashlib.sha256(after[1]).hexdigest() if after else None,
                       scope='Saved before/after files only; no hardware restoration performed by this analysis.')
    expected_range = manifest.get('expectedRange')
    missing,outside = None,[]
    if expected_range is not None:
        if not isinstance(expected_range,list) or len(expected_range)!=2 or any(type(v)is not int for v in expected_range) or not 0<=expected_range[0]<=expected_range[1]<=65535:
            raise ValueError('Invalid expectedRange')
        missing = sorted(set(range(expected_range[0],expected_range[1]+1))-{v for v,_ in samples})
        outside = sorted({v for v,_ in samples if not expected_range[0]<=v<=expected_range[1]})
    candidate = inference['candidate']
    assigned = {}
    for bit in candidate['bits'] if candidate else []:
        assigned[bit['byte']] = assigned.get(bit['byte'],0) | (1 << bit['bit'])
    unexplained = [dict(byte=row['byte'],mask=f"0x{int(row['mask'],16) & ~assigned.get(row['byte'],0):02X}")
                   for row in changed if int(row['mask'],16) & ~assigned.get(row['byte'],0)]
    matched = bool(candidate and candidate['fitsAllSamples'] and not unexplained and field and not mismatches and not unrelated
                   and restoration['byteIdentical'] and missing==[] and not outside)
    return dict(schemaVersion=1,id=manifest['id'],firmware=manifest.get('firmware','unknown'),
                verdict='CAPTURED_MAPPING_MATCH' if matched else 'REVIEW_REQUIRED',samples=len(samples),
                changedBytes=changed,unexplainedChangedBits=unexplained,invariantBytes=154-len(changed),inference=inference,
                productionComparison=dict(field=field,matches=len(samples)-len(mismatches) if field else None,mismatches=mismatches),
                unexpectedDecodedChanges=unrelated,missingInputs=missing,outsideDeclaredRange=outside,restoration=restoration,evidence=evidence,
                scope='Offline captured-byte and actual decoder checks; not UI, audio, MIDI receive or general firmware verification.')


def sweep_text(report):
    candidate = report['inference']['candidate']
    lines = [f"# {report['id']}: {report['verdict']}", '',
             f"Samples: {report['samples']}; firmware recorded: {report['firmware']}",
             'Changed bytes (zero based): '+', '.join(f"{r['byte']} mask {r['mask']}" for r in report['changedBytes'])]
    if candidate:
        for bit in candidate['bits']:
            lines.append(f"- Value bit {bit['valueBit']} <- byte {bit['byte']} bit {bit['bit']}"+(' (inverted)' if bit['inverted'] else ''))
        lines.append(f"Candidate matches: {candidate['matches']}/{candidate['samples']}; held-out: {candidate['heldOutMatches']}/{candidate['heldOutSamples']}")
    else: lines.append('No unique supported bitfield candidate. See bitCandidates; do not guess.')
    lines += [f"Production decoder mismatches: {len(report['productionComparison']['mismatches'])}",
              f"Unexplained changed-byte masks: {report['unexplainedChangedBits']}",
              f"Samples with other decoded changes: {len(report['unexpectedDecodedChanges'])}",
              'Recorded before/after byte-identical: '+str(report['restoration']['byteIdentical']),
              'Missing declared input values: '+str(report['missingInputs']), '', report['scope'],
              report['inference']['limitation']]
    return '\n'.join(lines)+'\n'


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('action',choices=['start','stage','finish'])
    parser.add_argument('directory',type=Path)
    parser.add_argument('--label'); parser.add_argument('--verdict',choices=['passed','failed'])
    args=parser.parse_args()
    validation_event(Path(__file__).resolve().parents[2],args.directory,args.action,args.label,args.verdict)
