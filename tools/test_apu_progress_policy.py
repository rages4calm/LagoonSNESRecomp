"""Exercise the actual interpreter progress-checkpoint block and APU selector.

No handwritten substitute for the production branch is tested: extract it from
interp_bridge.c verbatim. Periodic, port and bridge-exit synchronization are
separate existing guards/device/live gates. This fixture requires the proposed
absolute-periodic progress policy to have been applied to the local SDK.
"""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
assert not a.out.exists(),'Choose fresh scratch output';a.out.mkdir(parents=True)
bridge=(sdk/'snes/interp_bridge.c').read_text(encoding='utf-8');common=(sdk/'common_rtl.c').read_text(encoding='utf-8')
start=bridge.index('        if (auto_quiescent &&\n            (progress_write_epoch !=')
block=bridge[start:bridge.index('        /* WAI:',start)].rstrip()
sig='bool rtl_apu_absolute_periodic_enabled(';s=common.index(sig);e=s+re.search(r'^}',common[s:],re.M).end();selector=common[s:e]
wrapper='''static void progress_under_test(CpuState *cpu,int auto_quiescent,uint64_t *write,uint64_t *dynamic,unsigned *out_steps) {
uint64_t progress_write_epoch=*write,progress_dynamic_epoch=*dynamic;unsigned steps=*out_steps;
'''+block+'''
*write=progress_write_epoch;*dynamic=progress_dynamic_epoch;*out_steps=steps;
}
'''
(a.out/'apu_progress_under_test.h').write_text(selector+'\n'+wrapper,encoding='utf-8')
test=repo/'tests/lagoon_apu_progress_policy_test.c';exe=a.out/'apu-progress-policy.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(sdk),'-I'+str(a.out.resolve()),str(test),'-o',str(exe)],check=True)
runs=[]
for no_apu in (False,True):
 env=os.environ.copy();env.pop('SNESRECOMP_INTERP_NOAPU',None)
 if no_apu:env['SNESRECOMP_INTERP_NOAPU']='0' # presence, not value, is original policy.
 run=subprocess.run([str(exe.resolve()),str(int(no_apu))],env=env,capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='');runs.append({'no_apu':no_apu,'returncode':run.returncode,'stdout':run.stdout,'stderr':run.stderr})
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
result={'passed':all(not x['returncode']for x in runs),'runs':runs,'block_sha256':hashlib.sha256(block.encode()).hexdigest(),'selector_sha256':hashlib.sha256(selector.encode()).hexdigest(),'sources':{str(x.relative_to(repo)):sha(x)for x in (test,Path(__file__),sdk/'snes/interp_bridge.c',sdk/'common_rtl.c',sdk/'snes/interp_bridge.h')},'scope':'Actual source block/selector with mocked flush device. RealAPU state/nonzeroPCM and strict live event parity are separate mandatory gates. No native promotion follows from this test.'}
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');raise SystemExit(not result['passed'])
