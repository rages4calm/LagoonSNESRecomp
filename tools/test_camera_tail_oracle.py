"""Original patched-camera opcodes and actual native dispatch through real frames."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True)
p.add_argument('--gen',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1]
expected=next(x.split('=',1)[1].strip() for x in (repo/'rom_identity.txt').read_text().splitlines() if x.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest()==expected
functions={};by_pc={}
for file in a.gen.glob('bank*_v2.c'):
 source=file.read_text(encoding='utf-8');bank=int(re.search(r'bank([0-9a-fA-F]{2})',file.name)[1],16)
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',source):
  body=source[m.start():source.index('\n}',m.start())+2]+'\n'
  label=re.search(r'\bL_([0-9A-Fa-f]{4})_M[01]X[01]:',body);assert label,m[1]
  pc=(bank<<16)|int(label[1],16);functions[m[1]]=(pc,body,file)
  if m[1].endswith('_M1X0'):by_pc[pc]=m[1]
pcs=[0x01a7b5,0x01a7d6,0x01acaa,0x01ada1,0x01ae16,0x01aeb9,
     0x01b016,0x01b0d6,0x01b10c,0x01b123,0x01ae0c]
selected={by_pc[pc] for pc in pcs};stubs={};bindings={};bodies=[]
for pc in pcs:
 name=by_pc[pc];_,body,file=functions[name]
 bindings[f'{pc:06X}']=dict(name=name,body_sha256=hashlib.sha256(body.encode()).hexdigest(),source_sha256=hashlib.sha256(file.read_bytes()).hexdigest())
 for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',body):
  assert child in functions,child
  if child not in selected:stubs[child]=functions[child][0]
 bodies.append(body)
header='\n'.join('RecompReturn '+name+'(CpuState*c);' for name in sorted(selected))+'\n'
header+='\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}' for name,pc in sorted(stubs.items()))+'\n'
header+='\n'.join(bodies)
header+=f'\nstatic RecompReturn camera_body(CpuState*c){{return {by_pc[0x01a7b5]}(c);}}\n'
bridge=repo/'snesrecomp/runner/src/snes/interp_bridge.c'
policy=bridge.read_text().split('/* BEGIN AOT_CONTINUATION_POLICY -- pure, bounded provenance matcher. */',1)[1].split('/* END AOT_CONTINUATION_POLICY */',1)[0]
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'camera_actor_under_test.h').write_text(header,encoding='utf-8')
(a.out/'camera_policy_under_test.h').write_text(policy,encoding='utf-8')
binary=a.out/'camera-tail-opcodes.exe';runner=repo/'snesrecomp/runner/src';harness=repo/'tests/lagoon_camera_tail_oracle_test.c'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-function',
 '-Wno-unused-label','-Wno-unused-parameter','-Wno-unused-variable','-I'+str(runner),
 '-I'+str(a.out.resolve()),str(harness),str(runner/'snes/interp816.c'),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
(a.out/'result.json').write_text(json.dumps(dict(status='PASS',rom_sha256=expected,
 manifest_sha256=hashlib.sha256((a.gen/'program_manifest.json').read_bytes()).hexdigest(),
 generated_bodies=bindings,stopped_children=stubs,actual_policy_sha256=hashlib.sha256(policy.encode()).hexdigest(),
 source_sha256={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in [
  'src/lagoon_native_camera.c','src/lagoon_native_camera.h',
  'snesrecomp/runner/src/snes/interp_bridge.c','snesrecomp/runner/src/snes/interp_bridge.h',
  'snesrecomp/runner/src/snes/interp816.c','snesrecomp/runner/src/cpu_state.h']},
 harness_sha256=hashlib.sha256(harness.read_bytes()).hexdigest(),stdout=r.stdout,
 scope='Original patched 80CA JSR/camera opcodes execute in interp816 through actual83F3 JMP. Actual SDK call notification arms production title token, actual SDK tail matcher owns unchanged CC80 stack frame, then actual generated A7B5/A7D6/ACAA and six boss handlers/AE0C/B123 execute. Independent original-opcode execution compares full RAM/registers/flags/stack and CPU/master/coprocessor clocks. Existing unsupported child services STOP at their original entries, with no fabricated return or outcome. Unit hardware bus is constant8master cycles; actual interpreter scheduler glue/APU/yields and whole-path waits require separate live checks.'),indent=2)+'\n')
