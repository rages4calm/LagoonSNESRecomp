"""Qualify finite inactive render-slot iteration against actual ROM opcodes."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',required=True,type=Path)
p.add_argument('--scratch-root',required=True,type=Path)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
if sha(a.rom)!='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb':p.error('Original source ROM required')
out=a.scratch_root.resolve();allowed=(repo.parent/'work-native-batch52/native').resolve()
if not out.is_relative_to(allowed) or out.exists():p.error('Use a fresh directory under work-native-batch52/native')
compiler=shutil.which(a.compiler)
if not compiler:p.error('Compiler unavailable')
out.mkdir(parents=True);exe=out/'iterator52.exe'
command=[compiler,'-std=c11','-O2','-Wall','-Wextra','-I'+str(sdk),str(repo/'tests/lagoon_native_iterator52_test.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)]
runs=[]
for cmd in (command,[str(exe),str(a.rom.resolve())]):
 r=subprocess.run(cmd,capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='');runs.append({'command':cmd,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
 if r.returncode:break
files=['src/lagoon_native_iterator52.c','src/lagoon_native_iterator52.h','tests/lagoon_native_iterator52_test.c','tools/test_native_iterator52.py','snesrecomp/runner/src/snes/interp816.c']
(out/'result.json').write_text(json.dumps({'passed':r.returncode==0,'sources':{f:sha(repo/f)for f in files},'rom_sha256':sha(a.rom),'runs':runs,'scope':'Actual source iterator window and physical JSR prefix, constant8-master unit bus. Original active actor calls/final rendering stop at real entries; no child results or returns fabricated. Live bridge/APU/refresh/hardware cadence and runtime activation are separate gates.'},indent=2)+'\n')
raise SystemExit(r.returncode)
