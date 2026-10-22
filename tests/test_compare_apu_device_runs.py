import hashlib
import importlib.util
import json
from pathlib import Path
import unittest


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


HERE = Path(__file__).resolve().parent
fixtures = module('strict_fixtures', HERE / 'test_compare_native_runs.py')
device = module('compare_apu_device_runs', HERE.parent / 'tools/compare_apu_device_runs.py')


class DeviceDomainTests(unittest.TestCase):
    save = fixtures.StrictComparisonTests.save
    change_receipt = fixtures.StrictComparisonTests.change_receipt

    def setUp(self):
        fixtures.StrictComparisonTests.setUp(self)
        self.source = self.a.parent / 'apu.c'
        self.source.write_text('reviewed buffered diagnostic source fixture')
        self.proof = self.a.parent / 'source-proof.json'
        self.proof.write_text(json.dumps(dict(
            schema='lagoon read-only APU queue trace proof v1', read_only=True,
            disabled_by_default=True, buffered_until_exit=True,
            source=dict(path=str(self.source), sha256=device.strict.sha(self.source)),
            instrumented_executables=['a' * 64, 'b' * 64])))
        for root in (self.a, self.b):
            queue = root / 'queue.csv'
            queue.write_text('event,port,value,guest_cycle,target_cycle,port_clock\n'
                             'Q,1,61,100,200,190\nA,1,61,0,200,200\n'
                             'Q,2,62,110,210,200\nA,2,62,0,210,210\nEND,4,0\n')
            overrides = {device.QUEUE_ENV: str(queue)}
            for key, name in zip(device.AUDIO_ENVS, ('apuram.jsonl', 'dsp.jsonl', 'pcm.bin')):
                path = root / name
                path.write_bytes(b'\x01\x02\x03\x04')
                overrides[key] = str(path)
            self.change_receipt(root, diagnostic_overrides=overrides)
            (root / 'dumps/one.apu.tsv').write_text(
                '\t'.join(device.PORT_COLUMNS) + '\n'
                '1\t10\t20\tcpu_wr\t1\t3D\t5\n'
                '1\t10\t30\tcpu_ap\t1\t3D\t6\n'
                '1\t10\t40\tcpu_wr\t2\t3E\t6\n'
                '1\t10\t50\tcpu_ap\t2\t3E\t7\n')

    def compare(self, **options):
        return device.compare_runs(self.a, self.b, self.proof, **options)

    def test_batch_observation_difference_retains_raw_failure(self):
        path = self.b / 'dumps/one.apu.tsv'
        text = path.read_text().replace('cpu_wr\t1\t3D\t5', 'cpu_wr\t1\t3D\t6')
        path.write_text(text.replace('1\t10\t30\tcpu_ap', '1\t11\t60\tcpu_ap'))
        path = self.b / 'queue.csv'
        path.write_text(path.read_text().replace('100,200,190', '100,200,199'))
        result = self.compare()
        self.assertTrue(result['passed'])
        self.assertFalse(result['raw_strict_passed'])
        self.assertFalse(result['raw_strict']['passed'])
        self.assertTrue(result['queue_streams']['Q']['exact'])
        self.assertTrue(result['queue_streams']['A']['exact'])

    def test_actual_target_or_apply_clock_change_cannot_pass(self):
        path = self.b / 'queue.csv'
        original = path.read_text()
        for changed in (original.replace('100,200,190', '100,201,190'),
                        original.replace('0,200,200', '0,200,201')):
            with self.subTest(changed=changed):
                path.write_text(changed)
                self.assertFalse(self.compare()['passed'])

    def test_cpu_request_beam_and_payload_are_authoritative(self):
        path = self.b / 'dumps/one.apu.tsv'
        path.write_text(path.read_text().replace('1\t10\t20\tcpu_wr', '1\t10\t21\tcpu_wr'))
        self.assertFalse(self.compare()['passed'])

    def test_pcm_and_seed_or_feature_changes_cannot_pass(self):
        (self.b / 'pcm.bin').write_bytes(b'\x01\x02\x03\x05')
        self.assertFalse(self.compare()['passed'])
        (self.b / 'pcm.bin').write_bytes(b'\x01\x02\x03\x04')
        self.change_receipt(self.b, seeded_save_files=[dict(copy='save1.sav', sha256='c' * 64)])
        self.assertFalse(self.compare()['passed'])
        self.change_receipt(self.b, seeded_save_files=[], enabled_features=['magic_power'])
        self.assertFalse(self.compare()['passed'])

    def test_absent_historical_queue_is_explicitly_incomplete(self):
        receipt = json.loads((self.a / 'receipt.json').read_text())
        receipt['diagnostic_overrides'].pop(device.QUEUE_ENV)
        self.save(self.a, receipt)
        result = self.compare(allow_baseline_without_queue=True)
        self.assertTrue(result['valid_evidence'])
        self.assertTrue(result['request_and_audio_projection_passed'])
        self.assertFalse(result['device_queue_complete'])
        self.assertFalse(result['passed'])
        self.assertFalse(self.compare()['valid_evidence'])

    def test_corrupt_overflow_and_empty_queue_are_refused(self):
        path = self.b / 'queue.csv'
        original = path.read_text()
        for changed in ('', original.replace('END,4,0\n', ''),
                        original.replace('END,4,0', 'END,3,0'),
                        original.replace('END,4,0', 'END,4,1'),
                        original.replace('100,200,190', '100,180,190'),
                        original + 'OVERFLOW,1\n', original + 'A,4,61,0,200,200\n',
                        original + 'A,1,61,0,200,199\n', original + 'Q,1,61,no,200,200\n'):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                path.write_text(changed)
                self.compare()

    def test_missing_or_stale_source_review_proof_is_refused(self):
        self.source.write_text('unreviewed changed source')
        with self.assertRaises(ValueError):
            self.compare()


if __name__ == '__main__':
    unittest.main()
