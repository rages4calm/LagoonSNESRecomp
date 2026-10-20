"""Keep exact death-flash modes and raw timing-selector input grammar."""
from pathlib import Path
import re
import tomllib
import unittest
ROOT=Path(__file__).resolve().parents[1]
NEW={'019346:M1X0','019368:M1X0','019371:M1X0','01A7D6:M1X0','01AAC1:M1X0'}
class DeathFlashConfigTests(unittest.TestCase):
 def test_timing_selectors_are_bare_keys(self):
  for name in ('precise-timing.txt','transfer-timing.txt'):
   tokens=(ROOT/'recomp'/name).read_text().split()
   self.assertTrue(all(re.fullmatch(r'[0-9A-Fa-f]{6}(?::M[01]X[01])?',v) for v in tokens),name)
   self.assertTrue(NEW.issubset(tokens),name)
   for key in NEW:self.assertEqual(tokens.count(key),1,name)
 def test_canonical_addresses_unique(self):
  rows=tomllib.loads((ROOT/'recomp/symbols.toml').read_text())['func']
  keys=[(row['bank'],int(row['addr'],16)) for row in rows]
  self.assertEqual(len(keys),len(set(keys)))
 def test_only_declared_native_widths_and_real_boundaries(self):
  cfg=(ROOT/'recomp/bank01.cfg').read_text().lower()
  self.assertNotIn('force_lle 019346',cfg)
  for key in ('9346','9368','9371'):
   for mode in ('m0x0','m0x1','m1x1'):
    self.assertIn('force_lle_variant 01'+key+':'+mode,cfg)
  for key in ('9364','936d','937c'):
   self.assertIn('force_lle 01'+key,cfg)
if __name__=='__main__':unittest.main()
