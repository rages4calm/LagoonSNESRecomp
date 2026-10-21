"""ROM-free tests of the extracted production deadline waiter, SDL3 and SDL2."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
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
    text=source.read_text(encoding='utf-8')
    body=module.block(text,'static void WaitUntil(double deadline) {')
    wait_decl=text[text.index('typedef struct ProfileWait {'):text.index('typedef struct ProfileStamp {')]
    timing_decl=text[text.index('enum { kFrameTimingCapacity ='):text.index('static ProfileStamp ProfileStart(void) {')]
    capacity=re.search(r'enum \{ kFrameTimingCapacity = [0-9]+ \};',text)[0]
    assert body.count('MonotonicSeconds()') == 2 and body.count('SDL_DelayPrecise(') == 1
    a.out.mkdir(parents=True)
    header=a.out/'wait_until_under_test.h';header.write_text(wait_decl+capacity+'\nstatic unsigned g_frame_timing_count;\n'+body+'\n',encoding='utf-8')
    harness=repo/'tests/lagoon_host_wait_test.c';results=[]
    for sdl3 in (1,0):
        executable=a.out/f'host-wait-sdl{3 if sdl3 else 2}.exe'
        subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror',f'-DSNESRECOMP_SDL3={sdl3}','-I'+str(a.out.resolve()),str(harness),'-o',str(executable)],check=True)
        run=subprocess.run([str(executable.resolve())],capture_output=True,text=True)
        print(run.stdout,end='');print(run.stderr,end='')
        results.append(dict(SDL=3 if sdl3 else 2,returncode=run.returncode,stdout=run.stdout,stderr=run.stderr))
    timing_header=a.out/'frame_timing_under_test.h'
    timing_header.write_text(wait_decl+timing_decl,encoding='utf-8')
    timing_harness=repo/'tests/lagoon_frame_timing_test.c'
    timing_executable=a.out/'frame-timing-test.exe'
    subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(a.out.resolve()),str(timing_harness),'-o',str(timing_executable)],check=True)
    run=subprocess.run([str(timing_executable.resolve())],capture_output=True,text=True)
    print(run.stdout,end='');print(run.stderr,end='')
    results.append(dict(scope='Actual RecordFrameTiming',returncode=run.returncode,stdout=run.stdout,stderr=run.stderr))
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    result=dict(passed=all(x['returncode']==0 for x in results),results=results,sources={str(x.relative_to(repo)):sha(x)for x in(source,harness,timing_harness,utility,Path(__file__))},extracted_sha256=sha(header),frame_timing_extracted_sha256=sha(timing_header),scope='Actual production WaitUntil body; deterministic SDL/monotonic/profile doubles. Whole remaining SDL3 budget, early return recheck, oversleep exit, short/subnanosecond/large clock, no wait and profiling; original SDL2 coarse/fine policy. Opt-in waitbudget/late-return accounting; actual presentation capture resets counters and respects off/capacity gates. No additional clocks/SDLcalls. Not an OS scheduler/real device timing guarantee.')
    (a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    if not result['passed']:raise SystemExit('Actual deadline waiter failed')


if __name__=='__main__':main()
