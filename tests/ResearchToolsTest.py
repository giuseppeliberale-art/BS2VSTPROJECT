"""Offline research-report regression checks; no MIDI transport."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('offline',ROOT/'tools/midi-lab/offline.py')
offline=importlib.util.module_from_spec(spec); spec.loader.exec_module(offline)

class ResearchToolsTests(unittest.TestCase):
    def synthetic(self):
        result=[]
        for value in range(16):
            data=bytearray(154)
            data[20]=((value&1)<<6)
            data[21]=(((value>>1)&1)^1)|(((value>>2)&1)<<5)
            data[27]=((value>>3)&1)<<2
            result.append((value,bytes(data)))
        return result

    def test_split_and_inverted_mapping_with_held_out_values(self):
        result=offline.infer_bits(self.synthetic())['candidate']
        self.assertTrue(result['fitsAllSamples'])
        self.assertEqual((result['matches'],result['heldOutSamples'],result['heldOutMatches']),(16,4,4))
        self.assertEqual([(p['byte'],p['bit'],p['inverted']) for p in result['bits']],[(20,6,False),(21,0,True),(21,5,False),(27,2,False)])

    def test_ambiguous_alias_does_not_produce_unique_candidate(self):
        samples=[]
        for value,data in self.synthetic():
            raw=bytearray(data);raw[30]=(value&1);samples.append((value,bytes(raw)))
        result=offline.infer_bits(samples)
        self.assertIsNone(result['candidate'])
        self.assertEqual(len(result['bitCandidates'][0]['options']),2)

    def test_held_out_failure_is_not_reported_as_fit(self):
        samples=self.synthetic();value,raw=samples[-1];raw=bytearray(raw);raw[27]^=4;samples[-1]=(value,bytes(raw))
        result=offline.infer_bits(samples)['candidate']
        self.assertIsNotNone(result)
        self.assertFalse(result['fitsAllSamples'])
        self.assertEqual(result['heldOutMatches'],3)

    def test_constant_values_do_not_establish_mapping(self):
        self.assertIsNone(offline.infer_bits([(0,bytes(154)),(0,bytes(154))])['candidate'])

    def root(self,path):
        (path/'Source').mkdir();(path/'tests').mkdir();(path/'tools').mkdir()
        (path/'CMakeLists.txt').write_text('project(test)')
        (path/'Source/test.cpp').write_text('original')
        return path

    def report(self,root):
        report=root/'validation-results/run';report.mkdir(parents=True)
        offline.validation_event(root,report,'start')
        for stage in ['Configure','Build standalone and regression tests','Regression tests']:
            offline.validation_event(root,report,'stage',stage,'passed')
        (report/'tests.log').write_text('100% tests passed, 0 tests failed out of 9')
        offline.validation_event(root,report,'finish')
        return report/'validation.json'

    def fake_git(self,root,*args): return 'abc123' if args[0]=='rev-parse' else ''

    def test_status_matches_current_inputs_and_ignores_docs_only_changes(self):
        with tempfile.TemporaryDirectory() as tmp,patch.object(offline,'git',side_effect=self.fake_git):
            root=self.root(Path(tmp));self.report(root)
            self.assertEqual(offline.status(root,[])['validation']['status'],'passed')
            (root/'README.md').write_text('documentation')
            self.assertEqual(offline.status(root,[])['validation']['status'],'passed')
            (root/'Source/test.cpp').write_text('new code')
            self.assertEqual(offline.status(root,[])['validation']['status'],'stale')

    def test_short_ctest_summary_records_counts(self):
        with tempfile.TemporaryDirectory() as tmp,patch.object(offline,'git',side_effect=self.fake_git):
            root=self.root(Path(tmp));path=self.report(root)
            (path.parent/'tests.log').write_text('100% tests passed out of 9')
            offline.validation_event(root,path.parent,'finish')
            self.assertEqual(offline.status(root,[])['validation']['status'],'passed')
            self.assertEqual(json.loads(path.read_text())['tests']['passed'],9)

    def test_changed_inputs_during_validation_cannot_pass(self):
        with tempfile.TemporaryDirectory() as tmp,patch.object(offline,'git',side_effect=self.fake_git):
            root=self.root(Path(tmp));path=self.report(root)
            report=json.loads(path.read_text());report['initialFingerprint']='old';path.write_text(json.dumps(report))
            self.assertEqual(offline.status(root,[])['validation']['status'],'stale')

    def test_failed_incomplete_and_malformed_reports(self):
        with tempfile.TemporaryDirectory() as tmp,patch.object(offline,'git',side_effect=self.fake_git):
            root=self.root(Path(tmp));path=self.report(root);original=json.loads(path.read_text())
            for changes,expected in [({'complete':False},'incomplete'),({'stages':{'Configure':'failed'}},'failed'),({'stages':{}},'unverified'),({'tests':None},'unverified')]:
                path.write_text(json.dumps(dict(original,**changes)))
                self.assertEqual(offline.status(root,[])['validation']['status'],expected)
            path.write_text('[]');self.assertEqual(offline.status(root,[])['validation']['status'],'unverified')

    def test_legacy_text_report_is_not_current_validation(self):
        with tempfile.TemporaryDirectory() as tmp,patch.object(offline,'git',side_effect=self.fake_git):
            root=self.root(Path(tmp));report=root/'validation-results/old';report.mkdir(parents=True)
            (report/'summary.txt').write_text('100% tests passed')
            self.assertEqual(offline.status(root,[])['validation']['status'],'unverified')

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'),'Actual decoder supplied by CTest')
    def test_glide_capture_proof_matches_actual_decoder(self):
        result=offline.analyse_sweep(ROOT/'tests/glide-divergence-fixtures',os.environ['BSII_PATCH_INSPECT'])
        self.assertEqual(result['verdict'],'CAPTURED_MAPPING_MATCH')
        self.assertEqual(result['changedBytes'],[dict(byte=118,mask='0x3C')])
        self.assertEqual(result['productionComparison']['matches'],16)
        self.assertEqual(result['inference']['candidate']['heldOutMatches'],4)
        self.assertTrue(result['restoration']['byteIdentical'])

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'),'Actual decoder supplied by CTest')
    def test_missing_captures_restoration_and_unrelated_changes_require_review(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)/'experiment';shutil.copytree(ROOT/'tests/glide-divergence-fixtures',folder)
            manifest_path=folder/'experiment.json';manifest=json.loads(manifest_path.read_text())
            original=manifest.copy();manifest['samples']=manifest['samples'][3:];manifest_path.write_text(json.dumps(manifest))
            result=offline.analyse_sweep(folder,os.environ['BSII_PATCH_INSPECT'])
            self.assertEqual(result['missingInputs'],[0,1,2]);self.assertEqual(result['verdict'],'REVIEW_REQUIRED')
            manifest_path.write_text(json.dumps(original))
            raw=bytearray((folder/'after.syx').read_bytes());raw[16]^=1;(folder/'after.syx').write_bytes(raw)
            result=offline.analyse_sweep(folder,os.environ['BSII_PATCH_INSPECT'])
            self.assertFalse(result['restoration']['byteIdentical']);self.assertEqual(result['verdict'],'REVIEW_REQUIRED')
            raw=bytearray((folder/'glide-divergence-0.syx').read_bytes());raw[16]^=1;(folder/'glide-divergence-0.syx').write_bytes(raw)
            result=offline.analyse_sweep(folder,os.environ['BSII_PATCH_INSPECT'])
            self.assertTrue(result['unexpectedDecodedChanges']);self.assertEqual(result['verdict'],'REVIEW_REQUIRED')

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'),'Actual decoder supplied by CTest')
    def test_constant_unmapped_change_is_not_hidden_by_decoder(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)/'experiment';shutil.copytree(ROOT/'tests/glide-divergence-fixtures',folder)
            for path in folder.glob('glide-divergence-*.syx'):
                raw=bytearray(path.read_bytes());raw[120]^=1;path.write_bytes(raw)
            result=offline.analyse_sweep(folder,os.environ['BSII_PATCH_INSPECT'])
            self.assertEqual(result['verdict'],'REVIEW_REQUIRED')
            self.assertEqual(result['unexplainedChangedBits'],[dict(byte=120,mask='0x01')])

    def test_bad_domains_paths_duplicate_samples_and_framing_rejected_before_decoder(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp)/'experiment';shutil.copytree(ROOT/'tests/glide-divergence-fixtures',folder)
            manifest_path=folder/'experiment.json';original=json.loads(manifest_path.read_text())
            for key,value in [('stateDomain','tuning-table'),('baseline','../outside.syx'),('restored','before.syx')]:
                manifest_path.write_text(json.dumps(dict(original,**{key:value})))
                with self.assertRaises(ValueError): offline.analyse_sweep(folder,'MUST_NOT_RUN')
            manifest=json.loads(json.dumps(original));manifest['samples'][1]['file']=manifest['samples'][0]['file'];manifest_path.write_text(json.dumps(manifest))
            with self.assertRaises(ValueError): offline.analyse_sweep(folder,'MUST_NOT_RUN')
            manifest_path.write_text(json.dumps(original));(folder/'glide-divergence-0.syx').write_bytes(b'bad')
            with self.assertRaises(ValueError): offline.analyse_sweep(folder,'MUST_NOT_RUN')

stream_spec=importlib.util.spec_from_file_location('sysex_stream',ROOT/'tools/midi-lab/sysex_stream.py')
stream=importlib.util.module_from_spec(stream_spec);stream_spec.loader.exec_module(stream)

class SysExStreamTests(unittest.TestCase):
    def fixture(self):
        return (ROOT/'tests/state-fixtures/baseline.syx').read_bytes()

    def reconstructed(self,report):
        result=bytearray()
        for entry in report['segments']:
            self.assertEqual(entry['offset'],len(result))
            raw=bytes.fromhex(entry['rawHex'])
            self.assertEqual(len(raw),entry['length'])
            result.extend(raw)
        return bytes(result)

    def test_actual_patch_and_synthetic_128_stream_do_not_authorize_bank(self):
        raw=self.fixture()*128
        r=stream.inspect_bytes(raw)
        self.assertTrue(r['framingClean'])
        self.assertEqual((r['completeMessages'],r['duplicateMessages']),(128,127))
        self.assertEqual(r['classifications'],{'bsii-edit-buffer-shape-candidate':128})
        self.assertFalse(r['verifiedBank']);self.assertFalse(r['transmitEligible'])
        self.assertEqual(r['segments'][-1]['duplicateOfSegment'],0)
        self.assertEqual(self.reconstructed(r),raw)
        changed=bytearray(self.fixture());changed[6]=1
        self.assertEqual(stream.inspect_bytes(changed)['classifications'],{'bsii-unknown-command':1})

    def test_unknown_and_realtime_preserve_exact_bytes_and_normalized_duplicates(self):
        message=bytes.fromhex('F0 7D 01 F7')
        raw=b'\xf8'+message+b'\xfa'+bytes.fromhex('F0 7D F8 01 FE F7')+b'\xfc'
        r=stream.inspect_bytes(raw)
        self.assertTrue(r['framingClean'])
        self.assertEqual(r['completeMessages'],2)
        self.assertEqual(r['duplicateMessages'],1)
        self.assertEqual(r['segments'][3]['realtimeBytes'],2)
        self.assertEqual(self.reconstructed(r),raw)

    def test_malformed_stream_resynchronizes_without_dropping_bytes(self):
        for raw,code in [(bytes.fromhex('F0 01'),'unterminated-sysex'),
                         (bytes.fromhex('F0 01 F0 02 F7'),'nested-start'),
                         (bytes.fromhex('F0 01 90 40 00 F0 02 F7'),'interrupted-sysex'),
                         (bytes.fromhex('F7 40 F0 01 F7'),'outside-sysex-data')]:
            with self.subTest(raw=raw):
                r=stream.inspect_bytes(raw)
                self.assertFalse(r['framingClean'])
                self.assertIn(code,[x['code'] for x in r['issues']])
                self.assertEqual(self.reconstructed(r),raw)

    def test_limits_empty_non_sysex_and_midi_files(self):
        for raw in [b'',b'x'*(stream.MAX_BYTES+1),b'MThd'+bytes(20),b'\xf0'* (stream.MAX_SEGMENTS+1)]:
            with self.assertRaises(ValueError):stream.inspect_bytes(raw)
        self.assertFalse(stream.inspect_bytes(b'\xf8')['framingClean'])
        self.assertFalse(stream.inspect_bytes(b'hello')['framingClean'])

    def test_cli_is_offline_does_not_modify_input_or_create_reports(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'capture.syx';path.write_bytes(self.fixture())
            args=[sys.executable,'-B',str(ROOT/'tools/midi-lab/lab.py'),'inspect-sysex',str(path),'--json']
            result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertEqual(result.returncode,0,result.stderr)
            r=json.loads(result.stdout)
            self.assertTrue(r['readOnly']);self.assertFalse(r['transmitEligible'])
            self.assertEqual(path.read_bytes(),self.fixture())
            self.assertEqual(list(Path(tmp).iterdir()),[path])
            path.write_bytes(bytes.fromhex('F0 01'))
            result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertEqual(result.returncode,1)
            self.assertFalse(json.loads(result.stdout)['framingClean'])

class BankReviewTests(unittest.TestCase):
    def bank(self):
        template=bytearray((ROOT/'tests/state-fixtures/baseline.syx').read_bytes())
        template[7]=1
        entries=[]
        for i in range(128):
            patch=bytearray(template);patch[8]=i;entries.append(bytes(patch))
        return entries

    def test_export_indices_order_independent_diff_and_no_write_eligibility(self):
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/'source.syx';reference=Path(tmp)/'reference.syx'
            entries=self.bank();reference.write_bytes(b''.join(entries))
            source.write_bytes(b''.join(reversed(entries)))
            r=stream.review_bank(source,reference)
            self.assertEqual(r['changedEntries'],0);self.assertFalse(r['byteIdentical'])
            self.assertEqual(r['patches'][0]['wireMessageIndex'],127)
            self.assertFalse(r['transmitEligible']);self.assertFalse(r['writeAddressingVerified'])
            changed=bytearray(entries[37]);changed[20]^=1;entries[37]=bytes(changed)
            source.write_bytes(b''.join(entries));r=stream.review_bank(source,reference)
            self.assertEqual(r['changedExportIndices'],[37]);self.assertEqual(r['unchangedEntries'],127)
            source.write_bytes(reference.read_bytes());self.assertTrue(stream.review_bank(source,reference)['byteIdentical'])

    def test_incomplete_duplicate_wrong_command_and_malformed_banks_rejected(self):
        entries=self.bank()
        cases=[b''.join(entries[:-1]),b''.join(entries+[entries[0]]),b''.join(entries[:-1]+[entries[0]])]
        for offset,value in [(5,0x34),(6,1),(7,0),(9,1),(12,1),(20,0x80),(30,0xf0),(153,0)]:
            altered=bytearray(entries[0]);altered[offset]=value
            cases.append(bytes(altered)+b''.join(entries[1:]))
        cases += [b'\xf8'+b''.join(entries),b''.join(entries)+b'\x00']
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'bad.syx'
            for raw in cases:
                path.write_bytes(raw)
                with self.subTest(prefix=raw[:14]),self.assertRaises(ValueError):stream.review_bank(path)
            good=Path(tmp)/'good.syx';good.write_bytes(b''.join(entries))
            with self.assertRaises(ValueError):stream.review_bank(good,path)

    def test_cli_preview_never_modifies_files_or_creates_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/'source.syx';reference=Path(tmp)/'reference.syx'
            raw=b''.join(self.bank());source.write_bytes(raw);reference.write_bytes(raw)
            args=[sys.executable,'-B',str(ROOT/'tools/midi-lab/lab.py'),'review-bank',str(source),'--against',str(reference),'--json']
            result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertEqual(result.returncode,0,result.stderr)
            r=json.loads(result.stdout);self.assertTrue(r['byteIdentical']);self.assertFalse(r['transmitEligible'])
            self.assertEqual(source.read_bytes(),raw);self.assertEqual(reference.read_bytes(),raw)
            self.assertEqual(set(Path(tmp).iterdir()),{source,reference})
            source.write_bytes(raw[:-1]);result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertEqual(result.returncode,1);self.assertIn('ERROR:',result.stderr)

class BankExtractionTests(unittest.TestCase):
    def bank(self):
        return BankReviewTests().bank()

    def test_all_indices_preserve_exact_payload_and_recover_original_fixture(self):
        entries=self.bank();report=stream.inspect_bytes(b''.join(reversed(entries)))
        original=(ROOT/'tests/state-fixtures/baseline.syx').read_bytes()
        for i in range(128):
            converted=stream.edit_buffer_entry(report,i)
            self.assertEqual(converted,original)
            self.assertEqual(converted[13:],entries[i][13:])
            self.assertEqual(converted[6:13],bytes(7))
        for index in [-1,128,1.5,True,'1']:
            with self.assertRaises(ValueError):stream.edit_buffer_entry(report,index)
        with self.assertRaises(ValueError):stream.edit_buffer_entry(stream.inspect_bytes(entries[0]),0)

    def test_distinct_payloads_remain_distinct_and_no_files_overwritten(self):
        entries=self.bank()
        for i in range(128):
            raw=bytearray(entries[i]);raw[20]=i;entries[i]=bytes(raw)
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/'bank.syx';source.write_bytes(b''.join(entries))
            output=Path(tmp)/'entry.syx';r=stream.extract_bank_entry(source,127,output)
            self.assertEqual(output.read_bytes(),entries[127][:6]+bytes(7)+entries[127][13:])
            self.assertFalse(r['midiSent']);self.assertFalse(r['storedWrite'])
            for path in [output,source]:
                before=path.read_bytes()
                with self.assertRaises(FileExistsError):stream.extract_bank_entry(source,0,path)
                self.assertEqual(path.read_bytes(),before)
            alias=Path(tmp)/'alias.syx';alias.symlink_to(source)
            with self.assertRaises(FileExistsError):stream.extract_bank_entry(source,0,alias)
            bad=Path(tmp)/'bad.syx';bad.write_bytes(b'bad')
            absent=Path(tmp)/'absent.syx'
            with self.assertRaises(ValueError):stream.extract_bank_entry(bad,0,absent)
            self.assertFalse(absent.exists())
            self.assertEqual(source.read_bytes(),b''.join(entries))

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'),'Actual production validator supplied by CTest')
    def test_outputs_pass_actual_production_gate_but_bank_messages_do_not(self):
        entries=self.bank();report=stream.inspect_bytes(b''.join(entries))
        with tempfile.TemporaryDirectory() as tmp:
            paths=[]
            for index in [0,1,11,127]:
                for name,data in [('stored',entries[index]),('edit',stream.edit_buffer_entry(report,index))]:
                    path=Path(tmp)/f'{name}-{index}.syx';path.write_bytes(data);paths.append(str(path))
            result=subprocess.run([os.environ['BSII_PATCH_INSPECT']]+paths,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            records=[json.loads(line) for line in result.stdout.splitlines()]
            self.assertEqual([r['editBufferEligible'] for r in records],[False,True]*4)
            self.assertTrue(all(r['valid'] for r in records))

    def test_cli_explicit_file_export_and_invalid_arguments(self):
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/'bank.syx';raw=b''.join(self.bank());source.write_bytes(raw)
            output=Path(tmp)/'patch.syx'
            args=[sys.executable,'-B',str(ROOT/'tools/midi-lab/lab.py'),'extract-bank-entry',str(source),'11',str(output),'--json']
            result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(json.loads(result.stdout)['bytes'],154)
            self.assertEqual(output.read_bytes(),(ROOT/'tests/state-fixtures/baseline.syx').read_bytes())
            result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertEqual(result.returncode,1)
            self.assertEqual(source.read_bytes(),raw)
            args[-3]='128'
            result=subprocess.run(args,capture_output=True,text=True,cwd=tmp)
            self.assertNotEqual(result.returncode,0)

if __name__=='__main__': unittest.main()
