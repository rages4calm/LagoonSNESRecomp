"""Compile the actual read-only cameo module against an external source snapshot."""
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True);p.add_argument('--snapshot',type=Path,required=True)
p.add_argument('--vram',type=Path,required=True)
p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(a.rom)=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
assert not a.out.exists();a.out.mkdir(parents=True)
exe=a.out/'easter-egg-test.exe';sdk=repo/'snesrecomp/runner/src'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-I'+str(sdk),str(repo/'tests/lagoon_easter_egg_test.c'),str(repo/'src/lagoon_dialogue_font.c'),str(sdk/'snes_overlay_draw.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
run=subprocess.run([str(exe.resolve()),str(a.rom.resolve()),str(a.snapshot.resolve()),str(a.vram.resolve())],capture_output=True,text=True,cwd=a.out)
print(run.stdout,end='');print(run.stderr,end='')
paths=['src/lagoon_easter_egg.c','src/lagoon_easter_egg.h','src/lagoon_dialogue_font.c','src/lagoon_dialogue_font.h','assets/easter_egg/crono_portrait.generated.h','tests/lagoon_easter_egg_test.c','tools/test_easter_egg.py','assets/easter_egg/crono_dance.generated.h','assets/easter_egg/provenance.json']
record={'passed':run.returncode==0,'stdout':run.stdout,'stderr':run.stderr,'rom_sha256':sha(a.rom),'snapshot_sha256':sha(a.snapshot),'source_sha256':{x:sha(repo/x)for x in paths},'scope':'Actual module and full source snapshot; pure host rendering/interaction/raster guards and guest immutability. No actual host routing, in-game reachability or achievement persistence claim.'}
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n');assert record['passed']
