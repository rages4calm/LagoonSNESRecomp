"""ROM-free tests of the extracted production deadline waiter, SDL3 and SDL2."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--compiler',default='clang')
    a=p.parse_args()
    if a.out.exists():p.error('Choose fresh scratch output')
    repo=Path(__file__).resolve().parents[1]
    utility=repo/'tools/test_audio_load_resync.py'
    spec=importlib.util.spec_from_file_location('host_source_extract',utility)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    source=repo/'snesrecomp/runner/src/desktop/host_main.c'
    body=module.block(source.read_text(encoding='utf-8'),'static void WaitUntil(double deadline) {')
    a.out.mkdir(parents=True)
    header=a.out/'wait_until_under_test.h';header.write_text(body+'\n',encoding='utf-8')
    harness=repo/'tests/lagoon_host_wait_test.c';results=[]
    for sdl3 in (1,0):
        executable=a.out/f'host-wait-sdl{3 if sdl3 else 2}.exe'
        subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror',f'-DSNESRECOMP_SDL3={sdl3}','-I'+str(a.out.resolve()),str(harness),'-o',str(executable)],check=True)
        run=subprocess.run([str(executable.resolve())],capture_output=True,text=True)
        print(run.stdout,end='');print(run.stderr,end='')
        results.append(dict(SDL=3 if sdl3 else 2,returncode=run.returncode,stdout=run.stdout,stderr=run.stderr))
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    result=dict(passed=all(x['returncode']==0 for x in results),results=results,sources={str(x.relative_to(repo)):sha(x)for x in(source,harness,utility,Path(__file__))},extracted_sha256=sha(header),scope='Actual production WaitUntil body; deterministic SDL/monotonic/profile doubles. Whole remaining SDL3 budget, early return recheck, oversleep exit, short/subnanosecond/large clock, no wait and profiling; original SDL2 coarse/fine policy. Not an OS scheduler/real device timing guarantee.')
    (a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    if not result['passed']:raise SystemExit('Actual deadline waiter failed')


if __name__=='__main__':main()
