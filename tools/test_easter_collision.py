"""Source-qualified cameo footprint through actual original integration opcodes."""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest()
assert sha(a.rom)=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
assert not a.out.exists();a.out.mkdir(parents=True)
exe=(a.out/'cameo-collision.exe').resolve()
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-I'+str(sdk),str(repo/'tests/lagoon_easter_collision_test.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
result=subprocess.run([str(exe),str(a.rom.resolve())],capture_output=True,text=True)
print(result.stdout,end='');print(result.stderr,end='')
files=['src/lagoon_easter_collision.c','src/lagoon_easter_collision.h','tests/lagoon_easter_collision_test.c','tools/test_easter_collision.py']
(a.out/'result.json').write_text(json.dumps({'passed':result.returncode==0,'stdout':result.stdout,'stderr':result.stderr,'sources':{f:sha(repo/f)for f in files},'rom_sha256':sha(a.rom),'scope':'Actual original80C4/A4D3/A4EB opcodes plus production hook/readview, independent movement oracle. Title eligibility/bodyposition and generation services mocked. Actual runtime routing, animation and camera require separate live evidence.'},indent=2)+'\n')
raise SystemExit(result.returncode)
