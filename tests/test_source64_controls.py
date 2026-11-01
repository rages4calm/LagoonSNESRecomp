import importlib.util,json,unittest
from pathlib import Path

def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
root=Path(__file__).resolve().parents[1]
compare=module(root/'tools/compare_native_runs.py','compare64')
qualify=module(root/'tools/qualify_native.py','qualify64')
fixtures=module(root/'tests/test_compare_native_runs.py','fixtures64')
class Source64Controls(unittest.TestCase):
 def test_exact_control_and_output_tamper(self):
  f=fixtures.StrictComparisonTests();f.setUp();self.addCleanup(f.doCleanups)
  for path,value in [(f.a,'0'),(f.b,'1')]:
   (path/'dsp.s16').write_bytes(b'\x00\x01'*40)
   f.change_receipt(path,exe_sha256='a'*64,diagnostic_overrides={'LAGOON_TIMING64':value,'SNESRECOMP_DSPOUT_DRAIN':'1','SNESRECOMP_DSPOUT':str(path/'dsp.s16')})
  self.assertFalse(compare.compare_runs(f.a,f.b)['passed'])
  self.assertTrue(compare.compare_runs(f.a,f.b,source64=True)['passed'])
  (f.b/'dsp.s16').write_bytes(b'changed')
  self.assertFalse(compare.compare_runs(f.a,f.b,source64=True)['passed'])
 def test_control_refusals(self):
  for n in range(8):
   a={'exe_sha256':'a'*64,'program_manifest_sha256':'b'*64};b=dict(a);left={'LAGOON_TIMING64':'0','capture':'same'};right={'LAGOON_TIMING64':'1','capture':'same'}
   if n==0:left.pop('LAGOON_TIMING64')
   if n==1:right['LAGOON_TIMING64']='auto'
   if n==2:left['LAGOON_TIMING64']='1'
   if n==3:b['exe_sha256']='c'*64
   if n==4:b['program_manifest_sha256']='c'*64
   if n==5:right['capture']='different'
   if n==6:right['unexpected']='1'
   if n==7:right.pop('LAGOON_TIMING64')
   self.assertFalse(compare.source64_control(a,b,left,right)['approved'])
 def test_offline_guard(self):
  good='[Sound]\nEnableAudio = 0\n';outputs=['SNESRECOMP_DSPOUT=file']
  qualify.validate_offline_dsp_drain(False,good,outputs)
  for real,text,env in [(True,good,outputs),(False,'',outputs),(False,'[Other]\nEnableAudio=0\n',outputs),(False,good.replace('0','1'),outputs),(False,good+'EnableAudio=1\n',outputs),(False,good,[]),(False,good,['SNESRECOMP_DSPOUT='])]:
   with self.assertRaises(SystemExit):qualify.validate_offline_dsp_drain(real,text,env)
if __name__=='__main__':unittest.main()
