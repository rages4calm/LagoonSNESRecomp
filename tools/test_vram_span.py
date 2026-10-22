"""Source-domain oracle for the finite circular VRAM DMA overlap classifier."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];a.out.mkdir(parents=True,exist_ok=True)
exe=a.out.resolve()/'vram-span-test.exe'
cmd=[os.environ.get('CC','clang'),'-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(repo/'src'),str(repo/'tests/lagoon_vram_span_test.c'),'-o',str(exe)]
subprocess.run(cmd,check=True,cwd=repo)
run=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
record=json.loads(run.stdout)
record['sources']={str(f.relative_to(repo)):hashlib.sha256(f.read_bytes()).hexdigest() for f in (repo/'src/lagoon_vram_span.h',repo/'src/lagoon_wide_spell3.c',repo/'tests/lagoon_vram_span_test.c',Path(__file__))}
record['scope']='Pure finite classifier oracle; every byte start, bounded lengths and endpoint boundaries, 8192 actual byte-loop controls; not a whole-game performance result.'
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
