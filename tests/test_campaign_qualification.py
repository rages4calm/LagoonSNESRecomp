"""Regression for the invalid oversized-HP campaign fixture discovered live."""
import importlib.util
from pathlib import Path
import unittest

spec=importlib.util.spec_from_file_location('qualify_native',Path(__file__).resolve().parents[1]/'tools/qualify_native.py')
qualify=importlib.util.module_from_spec(spec);spec.loader.exec_module(qualify)

class SetupTests(unittest.TestCase):
    def test_actual_valid_protection_and_disclosure(self):
        rows=qualify.campaign_setup_writes(b'forcepoke 0520 ff00\nforcepoke 0526 e803 # defense1000\npoke 0560 0005\n')
        self.assertEqual(len(rows),3)
        self.assertEqual(rows[0]['hex'],'ff00')
        self.assertEqual(rows[2]['address'],0x560)  # Enemy HP may exceed255.

    def test_observed_corrupting_hp_refused(self):
        with self.assertRaisesRegex(ValueError,'above 255'):
            qualify.campaign_setup_writes(b'forcepoke 0520 ff7f\n')

    def test_overlapping_player_record_and_high_byte_refused(self):
        for script in (b'poke 0521 01\n',b'poke 051f 000001\n'):
            with self.subTest(script=script),self.assertRaisesRegex(ValueError,'above 255'):
                qualify.campaign_setup_writes(script)

    def test_out_of_bounds_and_malformed_refused(self):
        for script in (b'poke 1ffff ffff\n',b'poke 0520 zzzz\n'):
            with self.subTest(script=script),self.assertRaises(ValueError):
                qualify.campaign_setup_writes(script)

class RuntimeFailureTests(unittest.TestCase):
    def test_requested_camera_cannot_silently_refuse(self):
        line='lagoon.qol: camera_center refused, affected camera code has an AOT entry'
        self.assertEqual(qualify.runtime_failure_lines(line),[line])
        self.assertEqual(qualify.runtime_failure_lines('[lagoon-qol] recovery_time refused: code differs'),
                         ['[lagoon-qol] recovery_time refused: code differs'])

    def test_successful_features_and_ordinary_words_are_not_failures(self):
        self.assertEqual(qualify.runtime_failure_lines('lagoon.qol: camera_center applied\nnot refused by a game patch'),[])

class DiagnosticRangeTests(unittest.TestCase):
    def test_native_bounce_control_is_explicit_and_boolean(self):
        for key in qualify.DIAGNOSTIC_BOOLEANS:
            for value in ('0','1'):
                qualify.validate_diagnostic_boolean(key,value)
            for value in ('2','-1','true','00','1\n',' 0',''):
                with self.subTest(key=key,value=value),self.assertRaises(SystemExit):
                    qualify.validate_diagnostic_boolean(key,value)

    def test_frame_range_decimal_endpoints_and_single_frame(self):
        for value in ('0-0','1-1','0-10000000','0001-0002','630-632'):
            with self.subTest(value=value):
                qualify.validate_diagnostic_range('SNESRECOMP_AOTBLK',value)

    def test_pc_range_hex_endpoints_and_case(self):
        for value in ('0-ffffff','FFFFFF-FFFFFF','00aBcD-00ABCE','1-1','0192AE-0192BF'):
            with self.subTest(value=value):
                qualify.validate_diagnostic_range('SNESRECOMP_CYC_WATCH',value)

    def test_frame_range_rejects_reversed_out_of_bounds_or_non_decimal(self):
        for value in ('2-1','0-10000001','-1-2','1','1-2-3','1.0-2','0x1-0x2',
                      '1-A','+1-2',' 1-2','1-2 ','1 -2','1-2\n','1-2;anything'):
            with self.subTest(value=value),self.assertRaises(SystemExit):
                qualify.validate_diagnostic_range('SNESRECOMP_AOTBLK',value)

    def test_pc_range_rejects_reversed_long_prefixed_or_malformed(self):
        for value in ('ffffff-0','0-1000000','0000000-0000001','-1-2','0x10-0x20',
                      '$10-$20','00:8000-00:9000','GG-FF','1','1-2-3',' 1-2','1-2\n'):
            with self.subTest(value=value),self.assertRaises(SystemExit):
                qualify.validate_diagnostic_range('SNESRECOMP_CYC_WATCH',value)


if __name__=='__main__':unittest.main()
