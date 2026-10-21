"""Qualify enemy RAM computations against original opcodes using a local ROM.

The production callbacks keep source/scene/mode/real-stack guards. Pure body
semantics are compared separately from their intentional host-computation
timing policy: only the genuine RTS is charged after a qualified host body.
No ROM, generated code, copied SDK or save files are written to the output.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
identity='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
if sha(a.rom)!=identity:raise SystemExit('Expected original unheadered Lagoon USA ROM')
if a.out.exists():raise SystemExit('Choose a fresh output directory')
a.out.mkdir(parents=True)
base=[a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(repo/'src'),'-I'+str(sdk)]
source=repo/'src/lagoon_actor_compute.c';obj=a.out/'actor-compute.obj'
subprocess.run(base+['-DLAGOON_ACTOR_COMPUTE_TEST','-c',str(source),'-o',str(obj)],check=True)
results=[];tests=[]
for name in ['compute','sample','integrate','offscreen','cell']:
 test=repo/f'tests/lagoon_actor_{name}_test.c';tests.append(test);exe=a.out/(name+'-test.exe')
 subprocess.run(base+[str(test),str(obj),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
 run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
 (a.out/(name+'.log')).write_text(run.stdout+run.stderr,encoding='utf-8')
 results.append(dict(test=name,exit=run.returncode,stdout=run.stdout,stderr=run.stderr))
 print(run.stdout,end='');print(run.stderr,end='')
 run.check_returncode()
files=[source,repo/'src/lagoon_actor_compute.h',Path(__file__),sdk/'snes/interp816.c',sdk/'cpu_state.h',*tests]
result=dict(passed=True,rom_sha256=identity,tests=results,sources={str(f.relative_to(repo)):sha(f)for f in files},scope='Whole-RAM/register/flags/dead-stack source semantics; real guarded RTS return, finite63ordinary room descriptors; no boss/ending ownership or live-game/frame-pacing claim.')
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
