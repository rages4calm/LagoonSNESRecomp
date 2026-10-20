"""Compile the actual camera token and extracted SDK owner matcher together."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1]
expected=next(x.split('=',1)[1].strip() for x in (repo/'rom_identity.txt').read_text().splitlines() if x.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest()==expected
source=repo/'snesrecomp/runner/src/snes/interp_bridge.c'
policy=source.read_text().split('/* BEGIN AOT_CONTINUATION_POLICY -- pure, bounded provenance matcher. */',1)[1].split('/* END AOT_CONTINUATION_POLICY */',1)[0]
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'camera_policy_under_test.h').write_text(policy,encoding='utf-8')
binary=a.out/'camera-tail-guards.exe'
harness=repo/'tests/lagoon_camera_tail_guard_test.c'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
    '-I'+str(repo/'snesrecomp/runner/src'),'-I'+str(a.out.resolve()),str(harness),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
files=['src/lagoon_native_camera.c','src/lagoon_native_camera.h','src/lagoon_native_recovery.c',
       'snesrecomp/runner/src/snes/interp_bridge.c','snesrecomp/runner/src/snes/interp_bridge.h',
       'tests/lagoon_camera_tail_guard_test.c','tools/test_camera_tail.py']
(a.out/'result.json').write_text(json.dumps(dict(status='PASS',rom_sha256=expected,
    actual_policy_sha256=hashlib.sha256(policy.encode()).hexdigest(),
    source_sha256={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in files},
    stdout=r.stdout,scope='Actual production title token and SDK owner matcher with synthetic calls/frames and complete guarded-source mutants. Guest CPU and RAM unchanged. Actual post-JMP interpreter wiring, generated dispatcher operations and live scheduler/wait behavior require separate opcode/live checks.'),indent=2)+'\n')
