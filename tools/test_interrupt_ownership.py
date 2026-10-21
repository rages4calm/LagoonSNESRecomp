"""Run real interrupt/tail ownership fixtures with an explicitly selected C11 compiler.

The production bridge and upstream fixtures are linked unchanged. The legacy
ROM-free fixture supplies flat synthetic RAM and fake APU synchronization
counters; it does not run a game, physical device or real DSP/SPC. Two missing
local APU-policy API definitions are extracted from production verbatim.
The emitted-tail fixture uses the real emitter and production ancestor resolvers.
No pytest compiler detection, skip, install, ROM, assets or existing save inputs.
"""
from pathlib import Path
import argparse,ast,hashlib,json,re,shutil,subprocess,sys

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out',type=Path,required=True,help='Fresh private output directory')
parser.add_argument('--compiler',default='clang',help='Actual C11 compiler path/name; unavailable compiler is an error')
a=parser.parse_args();repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp'
compiler=shutil.which(a.compiler);assert compiler,'Requested compiler unavailable: '+a.compiler
out=a.out.resolve();assert not out.exists(),'Choose fresh private output; existing evidence is preserved'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected={
 'tests/test_interrupt_tail.py':'3a5abc0e4968a264af5e5adb7fb8cd863f3ce27199302c372858910b1388663c',
 'tests/interp816/interrupt_tail_test.c':'84ac0e6e3fc749b6b9351c3f21a27969dcae50b2cf458a94a9884f7e699e6a28',
 'tests/v2/test_tail_unwind_execution.py':'4e3db6c9221b5f86156d4bbc89bb5dce294c832e9bb99037f42bdf0e06d4767e',
 'tests/v2/tail_unwind_fixture.c':'b727f5c9194b64765d0b13289c52503742ad31b601f8b32f23c790130329f0c9'}
for name,digest in expected.items():assert sha(sdk/name)==digest,'Reviewed upstream fixture differs: '+name
out.mkdir(parents=True)
rtl=(sdk/'runner/src/common_rtl.c').read_text(encoding='utf-8')
def actual_function(source,name):
 m=re.search(r'(?m)^(?:static )?(?:bool|uint64_t) '+name+r'\([^;]*?\)\s*\{',source);assert m,name
 end=m.end();depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]
api_names=('rtl_apu_absolute_periodic_enabled','rtl_apu_periodic_flush_threshold')
apis='\n'.join(actual_function(rtl,n)for n in api_names)
common=['-std=c11','-D_POSIX_C_SOURCE=200809L','-O1','-DSNESRECOMP_TIER2_TEST=1','-Irunner/src','-Irunner/src/snes']
sources=['runner/src/snes/interp816.c','runner/src/snes/interp_bridge.c','runner/src/snes/tier2_capture.c','runner/src/sha256.c','runner/src/snes/cx4.c']
results=[]
for fixture in ('interrupt_tail_test.c','bridge_test.c'):
 wrapper=out/(fixture.removesuffix('.c')+'-compat.c');exe=out/(fixture.removesuffix('.c')+'.exe')
 wrapper.write_text('#include "'+(sdk/'tests/interp816'/fixture).as_posix()+'"\n#include "snes/cart.h"\n'+apis+'\n',encoding='utf-8')
 cmd=[compiler,*common,str(wrapper),*sources,'-lm','-o',str(exe)]
 build=subprocess.run(cmd,cwd=sdk,capture_output=True,text=True)
 (out/(fixture+'.build.log')).write_text(build.stdout+build.stderr,encoding='utf-8')
 row={'fixture':fixture,'build_exit':build.returncode,'compiler_command':cmd,'wrapper_sha256':sha(wrapper)}
 if not build.returncode:
  # Keep any synthetic recorder output in this same new private batch.
  import os
  env=os.environ.copy()
  for key in list(env):
   if key.startswith(('SNESRECOMP_','LAGOON_')):env.pop(key)
  env['SNESRECOMP_COVERAGE_DIR']=str(out/'coverage')
  run=subprocess.run([str(exe)],cwd=out,env=env,capture_output=True,text=True,timeout=60)
  row.update(returncode=run.returncode,stdout=run.stdout,stderr=run.stderr,exe_sha256=sha(exe))
 results.append(row)
 print(json.dumps({'fixture':fixture,'build_exit':row['build_exit'],'returncode':row.get('returncode'),'stdout':row.get('stdout','')}),flush=True)

sys.path[:0]=[str(sdk/'tests/v2'),str(sdk/'recompiler')]
fixture=sdk/'tests/v2/test_tail_unwind_execution.py'
tree=ast.parse(fixture.read_text(encoding='utf-8'),filename=str(fixture))
drops=[n for n in tree.body if isinstance(n,ast.Import)and any(x.name=='pytest'for x in n.names)]
assert len(drops)==1,'Expected only the reviewed unused pytest runner import'
original_harness=ast.dump(next(n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name=='_harness'))
tree.body=[n for n in tree.body if n not in drops]
assert ast.dump(next(n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name=='_harness'))==original_harness
namespace={'__file__':str(fixture)};exec(compile(tree,str(fixture),'exec'),namespace)
c=out/'tail_unwind.c';c.write_text(namespace['_harness'](),encoding='utf-8');exe=out/'tail_unwind.exe'
cmd=[compiler,'-std=c11','-O1','-Wall','-Wextra','-I'+str(sdk/'runner/src'),str(c),'-o',str(exe)]
build=subprocess.run(cmd,cwd=sdk,capture_output=True,text=True)
(out/'tail-unwind.build.log').write_text(build.stdout+build.stderr,encoding='utf-8')
row={'fixture':'actual emitted tail wrapper + production ancestor resolvers','build_exit':build.returncode,'compiler_command':cmd,'emitted_fixture_sha256':sha(c)}
if not build.returncode:
 run=subprocess.run([str(exe)],cwd=out,capture_output=True,text=True,timeout=10)
 row.update(returncode=run.returncode,stdout=run.stdout,stderr=run.stderr,exe_sha256=sha(exe))
results.append(row)
source_paths=[sdk/n for n in expected]+[sdk/'tests/interp816/bridge_test.c',sdk/'tests/v2/_helpers.py',sdk/'runner/src/common_rtl.c',sdk/'runner/src/common_cpu_infra.c']+[sdk/n for n in sources]
record={'passed':all(x['build_exit']==0 and x.get('returncode')==0 for x in results),'tool_sha256':sha(Path(__file__)),'sources_sha256':{str(p.relative_to(sdk)):sha(p)for p in source_paths},'extracted_APU_definitions_sha256':{n:hashlib.sha256(actual_function(rtl,n).encode()).hexdigest()for n in api_names},'results':results,'scope':'Actual current production interpreter/bridge with unchanged reviewed upstream direct/NMI/paired-frame and legacy bridge fixture bodies. Verbatim two production APU API definitions supply missing local link dependencies; no APU/SPC/DSP or physicalaudio claim. Exact upstream_harness AST/emitter/productionancestor resolvers, only unusedpytest import omitted. Explicit actual C11 compiler, no skipped tests. Legacybridge expected boundednegative prints[interp_cap]; its436checks must still pass. No game/ROM/assets/saves/campaign inputs.'}
(out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'passed':record['passed'],'fixtures':len(results),'tail_wrapper_output':row.get('stdout','')}))
assert record['passed']
