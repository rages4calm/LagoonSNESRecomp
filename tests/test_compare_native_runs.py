import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('compare_native_runs',
    Path(__file__).resolve().parents[1] / 'tools/compare_native_runs.py')
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)


class StrictComparisonTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.a = Path(self.temp.name) / 'a'
        self.b = Path(self.temp.name) / 'b'
        for root in (self.a, self.b):
            root.mkdir()
            (root / 'dumps').mkdir()
            for name in ('route.txt', 'config.ini', 'program_manifest.json'):
                (root / name).write_bytes(b'fixture')
            for component in compare.COMPONENTS:
                (root / 'dumps' / f'one.{component}').write_bytes(
                    b'{"cpu":{"sp":511}}' if component == 'regs.json' else b'\x00\x01')
            (root / 'dumps/one.info.json').write_text(json.dumps(
                dict(frame=3, master_clock=123, vpos=225, hpos=4)))
            (root / 'presents.csv').write_text(
                'present,frame,alpha,crc32,luma\n0,1,1.0000,abc12345,1.000\n')
            for name in ('stderr.log', 'stdout.log'):
                (root / name).write_text('')
            digest = hashlib.sha256(b'fixture').hexdigest()
            self.save(root, dict(exe=str(root / 'retired.exe'), exe_sha256=root.name * 64,
                program_manifest_sha256=digest, rom_sha256='r' * 64,
                route_sha256=digest, frames=10, input_config_sha256=digest,
                effective_config_sha256=digest, script_only_input=True,
                exit_code=0, runtime_failures=[], qualification_passed=True,
                dump_tags=['one'], seeded_save_files=[]))

    def save(self, root, receipt):
        (root / 'receipt.json').write_text(json.dumps(receipt))

    def change_receipt(self, root, **values):
        receipt = json.loads((root / 'receipt.json').read_text())
        self.save(root, dict(receipt, **values))

    def test_requested_feature_refusal_invalidates_historical_receipt(self):
        (self.b/'stderr.log').write_text('lagoon.qol: camera_center refused, affected camera code has an AOT entry')
        result=compare.compare_runs(self.a,self.b)
        self.assertFalse(result['valid_evidence'])
        self.assertFalse(result['passed'])

    def test_exact_and_distinct_executable_identity(self):
        result = compare.compare_runs(self.a, self.b)
        self.assertTrue(result['passed'])
        self.assertNotEqual(result['baseline']['executable_sha256'],
                            result['candidate']['executable_sha256'])

    def test_stack_byte_cannot_be_ignored(self):
        (self.b / 'dumps/one.wram.bin').write_bytes(b'\x00\x02')
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['passed'])
        self.assertEqual(result['checkpoints'][0]['components']['wram.bin']['different_bytes'], 1)

    def test_sram_change_fails_comparison(self):
        (self.b / 'dumps/one.sram.bin').write_bytes(b'\x01\x01')
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['passed'])
        self.assertEqual(result['checkpoints'][0]['components']['sram.bin']['different_bytes'], 1)

    def test_register_semantic_and_raw_differences_reported(self):
        (self.b / 'dumps/one.regs.json').write_text('{"cpu":{"sp":510}}')
        result = compare.compare_runs(self.a, self.b)
        component = result['checkpoints'][0]['components']['regs.json']
        self.assertFalse(result['passed'])
        self.assertEqual(component['json_differences'][0]['path'], '$.cpu.sp')

    def test_clock_delta_fails_even_if_six_components_match(self):
        path = self.b / 'dumps/one.info.json'
        value = json.loads(path.read_text()); value['master_clock'] += 1
        path.write_text(json.dumps(value))
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['passed'])
        self.assertFalse(result['checkpoints'][0]['clock_exact'])

    def test_transient_picture_between_checkpoints_fails(self):
        (self.b / 'presents.csv').write_text(
            'present,frame,alpha,crc32,luma\n0,1,1.0000,deadbeef,1.000\n')
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['passed'])
        self.assertEqual(result['ordered_presentations']['first_mismatch']['candidate']['frame'], '1')

    def test_missing_additional_presentations_and_tags(self):
        with (self.b / 'presents.csv').open('a') as stream:
            stream.write('1,2,1.0000,abc12345,1.000\n')
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['passed'])
        self.assertIsNone(result['ordered_presentations']['first_mismatch']['baseline'])
        for path in (self.b / 'dumps').glob('one.*'):
            path.rename(path.with_name(path.name.replace('one.', 'two.', 1)))
        self.change_receipt(self.b, dump_tags=['two'])
        result = compare.compare_runs(self.a, self.b)
        self.assertEqual(result['missing_candidate_tags'], ['one'])
        self.assertEqual(result['additional_candidate_tags'], ['two'])

    def test_forged_route_hash_or_missing_component_is_invalid(self):
        (self.b / 'route.txt').write_text('different')
        (self.b / 'dumps/one.oam.bin').unlink()
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['valid_evidence'])
        self.assertFalse(result['passed'])

    def test_different_inputs_fail_even_with_exact_outputs(self):
        self.change_receipt(self.b, diagnostic_overrides={'LAGOON_WIDE_ASPECT': '32:9'})
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['passed'])
        self.assertIn('diagnostic_overrides', result['input_differences'])

    def test_fatal_log_overrides_exit_zero_and_success_claim(self):
        (self.b / 'stderr.log').write_text('[brk] invalid guest\n')
        result = compare.compare_runs(self.a, self.b)
        self.assertFalse(result['valid_evidence'])

    def test_missing_clock_is_not_exact(self):
        for root in (self.a, self.b):
            (root / 'dumps/one.info.json').write_text('{"frame":3}')
        self.assertFalse(compare.compare_runs(self.a, self.b)['passed'])


if __name__ == '__main__':
    unittest.main()
