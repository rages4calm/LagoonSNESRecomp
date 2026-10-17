"""Synthetic export vectors; no ROM, game launch or owner configuration."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import campaign_coverage as cc


class CampaignCoverageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.root = Path(self.temp.name)
        self.plan = dict(schema='lagoon campaign coverage plan v1', rom_sha256='a'*64,
            expected_scenarios=[dict(id='boss0'), dict(id='boss1'), dict(id='boss2')], runs=[])

    def tearDown(self):
        self.temp.cleanup()

    def write(self, path, value):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value), encoding='utf-8'); return path

    def run_fixture(self, name='one', build='b'*64, session=None):
        directory = self.root/name; directory.mkdir(); (directory/'coverage').mkdir()
        program = json.dumps(dict(nodes={})).encode(); (directory/'program_manifest.json').write_bytes(program)
        route = b'wait 10\ndump boss\nquit\n'; (directory/'route.txt').write_bytes(route)
        receipt = dict(exit_code=0, dump_tags=['boss'], script_only_input=True, rom_sha256='a'*64,
            exe_sha256=build, program_manifest_sha256=hashlib.sha256(program).hexdigest(),
            route_sha256=hashlib.sha256(route).hexdigest())
        self.write(directory/'receipt.json', receipt)
        (directory/'stderr.log').write_text('clean runtime\n',encoding='utf-8')
        (directory/'stdout.log').write_text('',encoding='utf-8')
        for suffix in ('fb.bmp','wram.bin','regs.json','vram.bin','oam.bin','cgram.bin','info.json'):
            path = directory/'dumps'/f'boss.{suffix}'; path.parent.mkdir(exist_ok=True); path.write_bytes(b'fixture')
        row = dict(site_pc24='0x018000', target_pc24='0x018100', entry_mx='M1X0', emulation=0,
            site_kind='call_gap', fallback_reason='exact_entry_unavailable', observed_hits=1,
            completed_hits=9, pending_hits=0, sequence=3)
        record = dict(schema='snesrecomp tier2 coverage v2', capture_id=session or name, sequence=3,
            identity=dict(rom_sha256='a'*64, module_id='main', mapper='lorom',
                program_digest=receipt['program_manifest_sha256'], build_digest=build),
            checkpoint_complete=True, discoveries=[row], costs=[dict(record_kind='instruction',
                site_pc24=0,target_pc24='0x018100',entry_mx='M1X0',emulation=0,
                interpreted_instructions=4,guest_cycles=12)])
        self.write(directory/'coverage/checkpoint.json', record)
        self.plan['runs'].append(dict(id=name,directory=name,scenarios=['boss0'],context=dict(setup='declared cheats')))
        return directory, record

    def collect(self, **kwargs):
        path=self.write(self.root/'plan.json', self.plan)
        return cc.collect(path,self.root/'output',**kwargs)

    def test_bundle_deduplicates_checkpoint_journal_and_keeps_missing_scenarios(self):
        directory, record=self.run_fixture()
        self.write(directory/'coverage/duplicate.json',record)
        early=copy.deepcopy(record);early.update(schema='snesrecomp tier2 discovery v2',sequence=1)
        early['row']=dict(early['discoveries'][0],sequence=1,completed_hits=0,pending_hits=1)
        del early['discoveries'];del early['costs'];early.pop('checkpoint_complete')
        (directory/'coverage/journal.jsonl').write_text(json.dumps(early)+'\n',encoding='utf-8')
        summary=self.collect();self.assertTrue(summary['ingestion_ready'])
        self.assertEqual([s['status']for s in summary['scenarios']],['tagged_runs','absent','absent'])
        group=summary['groups'][0]
        profile=cc.load_profiles([self.root/'output'/group['ingest_manifest']])
        self.assertEqual(profile.discoveries[0]['completed_hits'],9)
        self.assertEqual(profile.discoveries[0]['pending_hits'],0)
        self.assertEqual(group['interpreted_instructions'],4)
        self.assertFalse(summary['native_completion_claim'])
        self.assertFalse(summary['scenarios'][0]['victory_or_complete_boss_coverage_proven'])

    def test_independent_sessions_sum_but_builds_partition(self):
        self.run_fixture('one');self.run_fixture('two');self.run_fixture('other','c'*64)
        summary=self.collect();self.assertEqual(len(summary['groups']),2)
        self.assertEqual(sorted(g['interpreted_instructions']for g in summary['groups']),[4,8])

    def test_missing_boss_registry_still_reports_absent_without_generation(self):
        result=self.collect();self.assertFalse(result['ingestion_ready'])
        self.assertTrue(all(x['status']=='absent'for x in result['scenarios']))

    def test_capture_does_not_match_run_receipt(self):
        directory, record=self.run_fixture();record['identity']['build_digest']='c'*64
        self.write(directory/'coverage/checkpoint.json',record)
        with self.assertRaisesRegex(ValueError,'does not belong'):self.collect()
        self.assertFalse((self.root/'output').exists())

    def test_overflow_rejects_by_default_and_salvage_is_not_ready(self):
        directory, record=self.run_fixture();record['dropped_cost_samples']=1
        self.write(directory/'coverage/checkpoint.json',record)
        with self.assertRaisesRegex(ValueError,'incomplete capture'):self.collect()
        result=self.collect(allow_incomplete=True);self.assertFalse(result['ingestion_ready'])
        self.assertNotIn('snesrecomp_cli.py generate',(self.root/'output/INGESTION.md').read_text())

    def test_counter_and_address_bounds(self):
        for field,value in [('observed_hits',1<<64),('observed_hits',-1),('observed_hits',True),('target_pc24','0x1000000')]:
            with self.subTest(field=field,value=value):
                record=dict(schema='snesrecomp tier2 discovery v2', capture_id='x',sequence=1,
                    identity=dict(rom_sha256='a'*64,module_id='main',mapper='lorom',program_digest='b'*64,build_digest='c'*64),
                    row=dict(site_pc24=0,target_pc24=0x8100,entry_mx='M1X0',emulation=0))
                record['row'][field]=value
                with self.assertRaises(ValueError):cc.validate(record)

    def test_torn_tail_salvage_preserves_warning(self):
        directory,_=self.run_fixture();(directory/'coverage/tail.jsonl').write_text('{"torn":',encoding='utf-8')
        with self.assertRaisesRegex(ValueError,'torn final'):self.collect()
        result=self.collect(allow_incomplete=True);self.assertFalse(result['runs'][0]['ingest_ready'])

    def test_conflicting_equal_sequence_rows_fail(self):
        directory,record=self.run_fixture();record['discoveries'][0]['completed_hits']=10
        self.write(directory/'coverage/conflict.json',record)
        with self.assertRaisesRegex(ValueError,'conflicting coverage row'):self.collect()

    def test_source_route_tampering_and_missing_dump_fail(self):
        directory,_=self.run_fixture();(directory/'route.txt').write_text('different',encoding='utf-8')
        with self.assertRaisesRegex(ValueError,'route differs'):self.collect()

    def test_output_overlap_and_input_traversal_fail(self):
        directory,_=self.run_fixture();registry=self.write(self.root/'plan.json',self.plan)
        with self.assertRaisesRegex(ValueError,'overlaps'):cc.collect(registry,directory/'output')
        receipt=json.loads((directory/'receipt.json').read_text());receipt['dump_tags']=['../../escape']
        self.write(directory/'receipt.json',receipt)
        with self.assertRaisesRegex(ValueError,'escapes'):self.collect()

    def test_readiness_does_not_authorize_aot_allowlists(self):
        _,record=self.run_fixture();record['qualified_aot_targets']=[]
        with self.assertRaisesRegex(ValueError,'qualification'):cc.validate(record)

    def test_merged_counter_overflow_is_not_unbounded_python_success(self):
        for name in ('one','two'):
            directory,record=self.run_fixture(name);record['costs'][0]['guest_cycles']=(1<<64)-1
            self.write(directory/'coverage/checkpoint.json',record)
        with self.assertRaisesRegex(ValueError,'merged guest_cycles'):self.collect()
        self.assertFalse((self.root/'output').exists())

    def test_reusing_one_capture_as_two_runs_is_explicitly_incomplete(self):
        self.run_fixture('one',session='same');self.run_fixture('two',session='same')
        with self.assertRaisesRegex(ValueError,'already attributed'):self.collect()

    def test_missing_dump_and_existing_output_are_not_overwritten(self):
        directory,_=self.run_fixture();(directory/'dumps/boss.oam.bin').unlink()
        with self.assertRaisesRegex(ValueError,'missing or oversized'):self.collect()
        (self.root/'output').mkdir();marker=self.root/'output/keep.txt';marker.write_text('keep')
        with self.assertRaisesRegex(ValueError,'fresh directory'):self.collect()
        self.assertEqual(marker.read_text(),'keep')

    def test_wrong_title_mapper_is_not_treated_as_another_build(self):
        directory,record=self.run_fixture();record['identity']['mapper']='hirom'
        self.write(directory/'coverage/checkpoint.json',record)
        with self.assertRaisesRegex(ValueError,'LoROM identity'):self.collect()

    def test_exit_zero_complete_capture_does_not_hide_fatal_logs(self):
        directory,_=self.run_fixture()
        for marker in ('[brk] stack garbage','[interp_cap] limit','[interp_low_exec] invalid PC',
                       'cpu_halt reached','Assertion x != 0 failed.','ASSERT FAILURE'):
            with self.subTest(marker=marker):
                (directory/'stderr.log').write_text(marker,encoding='utf-8')
                with self.assertRaisesRegex(ValueError,'fatal runtime marker'):self.collect()
                self.assertFalse((self.root/'output').exists())

    def test_receipt_runtime_failure_and_qualification_rejection(self):
        directory,_=self.run_fixture();path=directory/'receipt.json';original=json.loads(path.read_text())
        for changes in ({'runtime_failures':['[brk]']},{'qualification_passed':False}):
            self.write(path,dict(original,**changes))
            with self.assertRaisesRegex(ValueError,'runtime failure or rejected'):self.collect()

    def test_stdout_fatal_marker_also_rejects_salvage_mode(self):
        directory,_=self.run_fixture();(directory/'stdout.log').write_text('[BRK] fatal',encoding='utf-8')
        with self.assertRaisesRegex(ValueError,'fatal runtime marker'):self.collect(allow_incomplete=True)


if __name__=='__main__': unittest.main()
