import importlib.util
import json
import os
import subprocess
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('lab', ROOT / 'tools/midi-lab/lab.py')
lab = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lab)

class MidiLabTests(unittest.TestCase):
    def test_registry_matches_shared_decoder_fields(self):
        ids = {item['id'] for item in lab.registry()}
        import re
        mapped = set(re.findall(r'\{"([^"]+)"', (ROOT/'Source/BSIIPatchFields.h').read_text()))
        self.assertEqual(ids, mapped)
        self.assertEqual(next(c for c in lab.registry() if c['id']=='bendAmount')['minimum'], -24)

    def test_running_status_fragmentation_clock_and_program(self):
        d = lab.Decoder()
        self.assertEqual(d.feed([0xB0,107]), [])
        rows = d.feed([0xF8,40,107,63,107,64,0xC0,0,127])
        self.assertEqual([r['controls'][0]['value'] for r in rows[:3]], [-24,-1,0])
        self.assertEqual([r['program'] for r in rows[3:]], [0,127])
        self.assertNotIn('controls', rows[3])

    def test_nrpn_channel_isolation_and_rpn_cancellation(self):
        d=lab.Decoder()
        rows=d.feed([0xB0,99,0,98,76,0xB1,6,20,0xB0,6,0])
        self.assertNotIn('controls',rows[2])
        self.assertEqual(rows[-1]['controls'][0]['id'],'aftertouchLfo2Spd')
        self.assertEqual(rows[-1]['controls'][0]['display'],-64)
        rows=d.feed([101,0,100,0,6,127])
        self.assertNotIn('controls',rows[-1])

    def test_paired_cc_requires_msb_and_preserves_eight_bits(self):
        d=lab.Decoder()
        rows=d.feed([0xB0,58,64])
        self.assertNotIn('controls',rows[0])
        rows=d.feed([26,127,58,64])
        self.assertEqual(rows[-1]['controls'][0]['value'],255)
        self.assertEqual(d.warnings['paired LSB without preceding MSB'],1)

    def test_sysex_packet_splits_and_realtime(self):
        d=lab.Decoder()
        self.assertEqual(d.feed([0xF0,0,0x20]),[])
        rows=d.feed([0xF8,0x29,0xF7,0xB0,108,1])
        self.assertEqual(rows[0]['bytes'],[0xF0,0,0x20,0x29,0xF7])
        self.assertEqual(rows[1]['discovery'],'arpOn')
        self.assertNotIn('controls',rows[1])
        d.feed([0xF0,0,0x20]); d.finish()
        self.assertEqual(d.warnings['unfinished SysEx'],1)

    def test_report_does_not_claim_hardware_or_ui_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp); source=path/'capture.txt'
            source.write_text('Connected: Bass Station II\nB0 63 00 B0 62 49 B0 06 01 B0 63 00 B0 62 69 B0 06 01\nC0 04\n')
            result=lab.analyse(source,path,'ampTrigger')
            self.assertEqual(result['verdict'],'OBSERVED')
            self.assertEqual(result['controls']['modTrigger']['last'],1)
            self.assertEqual(result['programs'],[{'channel':1,'raw':4}])
            result=lab.analyse(source,path,'osc2Fine')
            self.assertEqual(result['verdict'],'NOT OBSERVED')

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_actual_patch_decoder_matches_saved_state(self):
        result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],str(ROOT/'tests/state-fixtures/baseline.syx')],capture_output=True,text=True,check=True)
        decoded=json.loads(result.stdout)
        import xml.etree.ElementTree as ET
        expected={p.attrib['id']:float(p.attrib['value']) for p in ET.parse(ROOT/'tests/state-fixtures/baseline.bsiistate').getroot() if 'value' in p.attrib}
        self.assertTrue(decoded['valid'])
        self.assertEqual(set(decoded['fields']),{c['id'] for c in lab.registry()})
        self.assertTrue(all(expected[k]==v for k,v in decoded['fields'].items()))

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_actual_decoder_rejects_truncated_patch(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'bad.syx';p.write_bytes(b'\xf0\x00\xf7')
            result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],str(p)],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertFalse(json.loads(result.stdout)['valid'])

    def test_channel_filter_cannot_pass_on_wrong_channel(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp); capture=p/'capture.txt';capture.write_text('B1 6B 28\n')
            result=lab.analyse(capture,p,'bendAmount',channel=1)
            self.assertEqual(result['verdict'],'NOT OBSERVED')

    def test_failed_sweep_restores_its_own_backup(self):
        class FailedProcess:
            def wait(self): return 1
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp); (path/'before.syx').write_bytes(b'backup')
            calls=[]
            def execute(args,log=None):
                calls.append(args)
                if args[1]=='request': Path(args[2]).write_bytes(b'backup')
            with patch.object(lab.subprocess,'Popen',return_value=FailedProcess()), patch.object(lab,'bridge',return_value='bridge'), patch.object(lab,'execute',side_effect=execute), patch.object(lab,'compare') as compare:
                with self.assertRaises(RuntimeError): lab.sweep('bend-amount',path)
                self.assertEqual(calls[0],['bridge','restore',path/'before.syx'])
                self.assertEqual(calls[1][1],'request')
                compare.assert_called_once_with(path/'before.syx',path/'recovered.syx',path)

    def test_interrupted_sweep_stops_process_group_before_recovery(self):
        from unittest.mock import MagicMock
        process=MagicMock();process.pid=12345;process.poll.return_value=None
        process.wait.side_effect=[KeyboardInterrupt(),0]
        with tempfile.TemporaryDirectory() as tmp:
            directory=Path(tmp);(directory/'before.syx').write_bytes(b'original')
            calls=[]
            def execute(args,log=None):
                calls.append(args)
                if args[1]=='request': Path(args[2]).write_bytes(b'original')
            with patch.object(lab.subprocess,'Popen',return_value=process), patch.object(lab.os,'killpg') as kill, patch.object(lab,'bridge',return_value='bridge'), patch.object(lab,'execute',side_effect=execute), patch.object(lab,'compare'):
                with self.assertRaisesRegex(RuntimeError,'Interrupted'): lab.sweep('bend-amount',directory)
                kill.assert_called_once_with(12345,lab.signal.SIGTERM)
                self.assertEqual(calls[0][1],'restore')

    def test_failed_recovery_is_not_reported_as_success(self):
        class FailedProcess:
            def wait(self): return 1
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp); (path/'before.syx').write_bytes(b'backup')
            with patch.object(lab.subprocess,'Popen',return_value=FailedProcess()), patch.object(lab,'bridge',return_value='bridge'), patch.object(lab,'execute',side_effect=RuntimeError('restore failed')), patch.object(lab,'compare') as compare:
                with self.assertRaisesRegex(RuntimeError,'restore failed'): lab.sweep('bend-amount',path)
                compare.assert_not_called()

if __name__=='__main__': unittest.main()
