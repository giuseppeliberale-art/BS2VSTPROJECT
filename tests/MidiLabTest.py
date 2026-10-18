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
        self.assertEqual(ids - {'inputGain'}, mapped)
        self.assertEqual({c['id'] for c in lab.registry() if not c['patchMapped']}, {'inputGain'})
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
        self.assertEqual(rows[1]['controls'][0]['id'],'arpOn')
        self.assertEqual(rows[1]['controls'][0]['choice'],'On')
        d.feed([0xF0,0,0x20]); d.finish()
        self.assertEqual(d.warnings['unfinished SysEx'],1)

    def test_captured_arp_seq_routes_and_boundaries(self):
        rows=lab.Decoder().feed([0xB0,108,1,109,0,111,4,118,6,118,7,119,0,119,31,99,0,98,106,6,1])
        controls=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(c['id'],c['choice']) for c in controls], [
            ('arpOn','On'),('arpLatch','Off'),('arpOctaves','4'),
            ('arpMode','Play'),('arpMode','Record'),('arpRhythm','1'),
            ('arpRhythm','32'),('seqRetrig','On')])
        rows=lab.Decoder().feed([0xB0,111,0,119,32])
        self.assertTrue(all(not r['controls'][0]['inRange'] for r in rows))

    def test_lfo_sync_incoming_routes_and_labels(self):
        rows=lab.Decoder().feed([0xB0,99,0,98,88,6,1,98,92,6,0,98,87,6,25,98,91,6,34])
        controls=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(c['id'],c['choice']) for c in controls], [
            ('lfo1Sync','Sync'),('lfo2Sync','Speed'),('lfo1Division','1/4'),('lfo2Division','1/32 triplet')])
        rows=lab.Decoder().feed([0xB0,99,0,98,87,6,35])
        self.assertFalse([r for r in rows if 'controls' in r][0]['controls'][0]['inRange'])

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_all_lfo_sync_hardware_captures(self):
        directory=ROOT/'tests/lfo-sync-fixtures'
        for prefix, field, count in [('lfo1-sync','lfo1Sync',2), ('lfo2-sync','lfo2Sync',2),
                                     ('lfo1-division','lfo1Division',35), ('lfo2-division','lfo2Division',35)]:
            for value in range(count):
                with self.subTest(field=field, value=value):
                    result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],str(directory/f'{prefix}-{value}.syx')],capture_output=True,text=True,check=True)
                    decoded=json.loads(result.stdout)
                    self.assertTrue(decoded['valid'])
                    self.assertEqual(decoded['fields'][field],value)
        self.assertEqual((directory/'before.syx').read_bytes(),(directory/'after.syx').read_bytes())

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_all_modwheel_filter_captures(self):
        folder=ROOT/'tests/modwheel-filter-fixtures'
        files=[folder/f'{prefix}-{raw}.syx' for prefix in ('modwheel-filter','modwheel-lfo2-filter') for raw in range(128)]
        result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],*[str(p) for p in files]],capture_output=True,text=True,check=True)
        decoded=[json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(len(decoded),256)
        for index, patch in enumerate(decoded):
            field='modWhFilter' if index<128 else 'modWhLfo2Filter'
            self.assertTrue(patch['valid'])
            self.assertEqual(patch['fields'][field],index%128)
        self.assertEqual((folder/'before.syx').read_bytes(),(folder/'after.syx').read_bytes())

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_microtune_hardware_patch_captures(self):
        folder=ROOT/'tests/microtune-fixtures'
        files=[folder/'table-0-before.syx']+[folder/f'table-{v}.syx' for v in range(1,9)]
        result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],*[str(p) for p in files]],capture_output=True,text=True,check=True)
        decoded=[json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(len(decoded),9)
        self.assertTrue(all(p['valid'] for p in decoded))
        self.assertEqual([p['fields']['microTuneTable'] for p in decoded], list(range(9)))
        self.assertEqual((folder/'range-before.syx').read_bytes(),(folder/'range-restored.syx').read_bytes())

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_osc_extra_hardware_patch_captures(self):
        for prefix, field, count in [('osc-error','oscError',8),('glide-divergence','glideDivergence',16)]:
            folder=ROOT/f'tests/{prefix}-fixtures'
            files=[folder/f'{prefix}-{v}.syx' for v in range(count)]
            result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],*[str(p) for p in files]],capture_output=True,text=True,check=True)
            decoded=[json.loads(line) for line in result.stdout.splitlines()]
            self.assertEqual(len(decoded),count)
            self.assertTrue(all(p['valid'] for p in decoded))
            self.assertEqual([p['fields'][field] for p in decoded],list(range(count)))
            self.assertEqual((folder/'before.syx').read_bytes(),(folder/'after.syx').read_bytes())

    def test_osc_extra_emitted_nrpn(self):
        rows=lab.Decoder().feed([0xB0,99,0,98,111,6,7,6,0,6,8,98,113,6,15,6,0,6,16])
        values=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(v['id'],v['choice']) for v in values],[
            ('oscError','7'),('oscError','0'),('oscError',None),
            ('glideDivergence','15'),('glideDivergence','0'),('glideDivergence',None)])
        self.assertFalse(values[2]['inRange'])
        self.assertFalse(values[5]['inRange'])

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_sub_tuning_hardware_patch_captures(self):
        folder=ROOT/'tests/sub-tuning-fixtures'
        files=[folder/'before.syx']+[folder/f'sub-fine-{v}.syx' for v in range(27,229)]+[folder/f'sub-coarse-{v}.syx' for v in range(256)]
        result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],*[str(p) for p in files]],capture_output=True,text=True,check=True)
        decoded=[json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(len(decoded),459)
        self.assertTrue(all(p['valid'] for p in decoded))
        self.assertEqual([p['fields']['subFine'] for p in decoded[1:203]],list(range(27,229)))
        self.assertEqual([p['fields']['subCoarse'] for p in decoded[203:]],list(range(256)))
        for patch in decoded[1:]:
            for field in ('osc1Fine','osc1Coarse','osc2Fine','osc2Coarse'):
                self.assertEqual(patch['fields'][field],decoded[0]['fields'][field])
        self.assertEqual((folder/'before.syx').read_bytes(),(folder/'after.syx').read_bytes())

    def test_sub_tuning_paired_nrpn_isolation(self):
        d=lab.Decoder()
        rows=d.feed([0xB0,99,0,98,77,38,64,6,13])
        self.assertTrue(all('controls' not in r for r in rows))
        rows=d.feed([0xB1,99,0,98,84,6,127,0xB0,38,64,0xB1,38,64])
        values=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(v['id'],v['value']) for v in values],[('subFine',27),('subCoarse',255)])
        rows=d.feed([0xB0,98,84,38,64,100,3,6,10,38,0])
        self.assertTrue(all('controls' not in r for r in rows))

    def test_global_input_gain_msb_and_range(self):
        rows=lab.Decoder().feed([0xB0,99,64,98,28,6,0,6,31,6,32,99,0,6,1])
        values=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(v['id'],v['value']) for v in values],[('inputGain',0),('inputGain',31),('inputGain',32)])
        self.assertFalse(values[-1]['inRange'])
        self.assertNotIn('controls',rows[-1])

    def test_microtune_emitted_nrpn(self):
        rows=lab.Decoder().feed([0xB0,99,0,98,0,6,8,6,0,6,9])
        values=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(v['id'],v['choice']) for v in values[:2]], [('microTuneTable','8'),('microTuneTable','0')])
        self.assertFalse(values[-1]['inRange'])

    def test_modwheel_filter_signed_midi(self):
        rows=lab.Decoder().feed([0xB0,99,0,98,94,6,0,6,127,98,71,6,0,6,127])
        values=[r['controls'][0] for r in rows if 'controls' in r]
        self.assertEqual([(v['id'],v['display']) for v in values], [('modWhFilter',-64),('modWhFilter',63),('modWhLfo2Filter',-64),('modWhLfo2Filter',63)])

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
        self.assertEqual(set(decoded['fields']),{c['id'] for c in lab.registry() if c['patchMapped']})
        legacy={k:v for k,v in expected.items() if k in decoded['fields']}
        self.assertEqual(len(legacy),76) # Old XML also retains obsolete raw aliases.
        self.assertTrue(all(decoded['fields'][k]==v for k,v in legacy.items()))

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_actual_decoder_rejects_truncated_patch(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'bad.syx';p.write_bytes(b'\xf0\x00\xf7')
            result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],str(p)],capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0)
            self.assertFalse(json.loads(result.stdout)['valid'])

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_labelled_arp_seq_hardware_patch_captures(self):
        folder=ROOT/'tests/arp-seq-fixtures'
        manifest=json.loads((folder/'manifest.json').read_text())
        result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],*[str(folder/r['file']) for r in manifest]],capture_output=True,text=True,check=True)
        decoded=[json.loads(line) for line in result.stdout.splitlines()]
        self.assertEqual(len(decoded),len(manifest))
        coverage={}
        for row,patch in zip(manifest,decoded):
            self.assertTrue(patch['valid'])
            for ident,value in row['expected_choice_indices'].items():
                self.assertEqual(patch['fields'][ident],value,(row['file'],ident))
                coverage.setdefault(ident,set()).add(value)
        self.assertEqual(coverage,dict(arpOn={0,1},arpLatch={0,1},arpOctaves={0,1,2,3},arpMode=set(range(8)),arpRhythm={0,16,31},seqRetrig={0,1}))

    @unittest.skipUnless(os.environ.get('BSII_PATCH_INSPECT'), 'Inspector built by validation routine')
    def test_arp_fields_ignore_neighbours_and_reject_invalid_octaves(self):
        source=bytearray((ROOT/'tests/arp-seq-fixtures/seq-retrig-on.syx').read_bytes())
        ids=['arpOn','arpLatch','arpOctaves','arpMode','arpRhythm','seqRetrig']
        def decode(path):
            result=subprocess.run([os.environ['BSII_PATCH_INSPECT'],str(path)],capture_output=True,text=True,check=True)
            return json.loads(result.stdout)['fields']
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'test.syx';path.write_bytes(source);before=decode(path)
            source[77]^=0x47;source[78]^=0x63;source[79]^=0x71;source[80]^=0x60
            path.write_bytes(source);after=decode(path)
            self.assertEqual({k:before[k] for k in ids},{k:after[k] for k in ids})
            for raw in (0,5,6,7):
                source[78]=(source[78]&~0x1c)|(raw<<2);path.write_bytes(source)
                self.assertEqual(decode(path)['arpOctaves'],-1)

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
