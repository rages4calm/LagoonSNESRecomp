"""Original and approved centered camera dispatcher source oracle."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','out'):p.add_argument('--'+key,required=True,type=Path)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=Path(os.environ.get('SNESRECOMP_ROOT',repo/'snesrecomp'))/'runner/src'
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
if sha(a.rom)!='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb':p.error('Original source ROM required')
a.out.mkdir(parents=True,exist_ok=True);exe=a.out/'camera68.exe'
cmd=[a.compiler,'-std=c11','-O2','-Wall','-Wextra','-I'+str(sdk),str(repo/'tests/lagoon_native_camera68_test.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)]
subprocess.run(cmd,check=True);r=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='')
files=[repo/'src/lagoon_native_camera68.c',repo/'src/lagoon_native_camera68.h',repo/'tests/lagoon_native_camera68_test.c',Path(__file__),sdk/'snes/interp816.c']
(a.out/'result.json').write_text(json.dumps(dict(passed=r.returncode==0,stdout=r.stdout,stderr=r.stderr,rom_sha256=sha(a.rom),sources={str(f):sha(f)for f in files},scope='Original and approved camera dispatcher scalars, actual source JSR ownership, real helpers/JSR/RTS, stock table data, exact memory/bus/width/clock parity and immutable refusals. Existing centered83F3 tail/ancestor remains separate and authoritative. Live route activation remains separate.'),indent=2)+'\n',encoding='utf-8');raise SystemExit(r.returncode)
