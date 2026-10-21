"""Qualify sword readability using original source poses and the real PPU.

No ROM, extracted artwork or screenshots are distributed by this test. Supply
an owner-local original ROM. Output stays in a selected private scratch folder.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text(encoding='utf-8').splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==identity
if a.out.exists():raise SystemExit('Choose a fresh test output directory')
a.out.mkdir(parents=True)
source=repo/'src/lagoon_sword_visual.c';test=repo/'tests/lagoon_sword_visual_test.c'
# Keep strict warning treatment scoped to the new module, not upstream PPU C.
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(sdk),'-c',str(source),'-o',str(a.out/'sword-visual.obj')],check=True)
exe=a.out/'sword-visual-test.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-DSNESRECOMP_REVERSE_DEBUG=0','-I'+str(sdk),'-I'+str(sdk/'snes'),str(test),str(sdk/'snes/ppu.c'),str(sdk/'snes/ppu_legacy.c'),'-o',str(exe)],check=True)
r=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='')
files=[source,repo/'src/lagoon_sword_visual.h',test,Path(__file__),sdk/'snes/ppu.c',sdk/'snes/ppu_legacy.c']
result=dict(passed=r.returncode==0,rom_sha256=identity,sources={str(f.relative_to(repo)):sha(f)for f in files},stdout=r.stdout,stderr=r.stderr,scope='12 original sword staging/OAM poses, source/atlas/scene guards, real legacy+fast PPU background priority and windows. Synthetic pose construction, no live gameplay or display certification.')
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');r.check_returncode()
