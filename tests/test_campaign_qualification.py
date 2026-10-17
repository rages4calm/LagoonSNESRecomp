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

if __name__=='__main__':unittest.main()
