"""Bind actual endgame windows and actor initializer to original opcodes.

Original children are STOP sentinels at their real entry, never fake returns.
Actual018DC6 actor initialization uses a deterministic immediate multiply model;
hardware latency, IRQs and live scheduling are separate.
"""
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
all_functions={};by_pc={}
for file in a.gen.glob('bank*_v2.c'):
 text=file.read_text(encoding='utf-8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',text):
  body=text[m.start():text.index('\n}',m.start())+2]+'\n'
  label=re.search(r'\bL_([0-9A-Fa-f]{4})_M[01]X[01]:',body)
  assert label,m[1]
  bank=int(re.search(r'bank([0-9a-fA-F]{2})',file.name)[1],16)
  pc=(bank<<16)|int(label[1],16)
  all_functions[m[1]]=(pc,body,file)
  if m[1].endswith('_M1X0'):by_pc[pc]=(m[1],body,file)
required=[0x019382,0x01938D,0x0193BB,0x0193C1,0x0193C7,0x0193CD,0x0193D3,
          0x0193DC,0x0193E7,0x0193ED,0x0193F3,0x0193F4,0x0193FA,0x019400,
          0x019406,0x01940C,0x01B88E,0x01B8E2]
starts=required
assert all(pc in by_pc for pc in required),[hex(pc) for pc in required if pc not in by_pc]
bodies=[];bindings={};stubs={};selectors={}
support=[0x018DC6]
support_names={by_pc[pc][0] for pc in support}
for pc in starts+support:
 name,body,file=by_pc[pc]
 bindings[f'{pc:06X}']={'name':name,'body_sha256':hashlib.sha256(body.encode()).hexdigest(),'source_sha256':hashlib.sha256(file.read_bytes()).hexdigest()}
 # Rename only the definition. Every direct native child, including another
 # tested body, stops at its actual original callee entry and fabricates nothing.
 for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',body):
  assert child in all_functions,child
  if child not in support_names:stubs[child]=all_functions[child][0]
 if pc not in support:body=body.replace(f'RecompReturn {name}(',f'RecompReturn UnderTest_{pc:06X}(',1)
 bodies.append(body)
header='\n'.join('RecompReturn '+name+'(CpuState*c);' for name in support_names)+'\n'
header+='\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}' for name,pc in sorted(stubs.items()))+'\n'
header+='\n'.join(bodies)
actual_starts=starts
header+='\n#define BOSS_TEST_COUNT '+str(len(actual_starts))+'\n'
header+='static const unsigned boss_starts[]={'+','.join(f'0x{pc&65535:x}' for pc in actual_starts)+'};\n'
header+='static RecompReturn boss_body(unsigned i,CpuState*c){switch(i){\n'
for i,pc in enumerate(actual_starts):header+=f'case {i}:return UnderTest_{pc:06X}(c);\n'
header+='default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'endgame_windows_under_test.h').write_text(header,encoding='utf-8')
binary=a.out/'endgame-windows-opcodes.exe';runner=repo/'snesrecomp/runner/src'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-I'+str(runner),'-I'+str(a.out.resolve()),str(repo/'tests/lagoon_endgame_windows_oracle_test.c'),str(runner/'snes/interp816.c'),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
(a.out/'result.json').write_text(json.dumps({'status':'PASS','generated_bodies':bindings,'rom_sha256':expected,'manifest_sha256':hashlib.sha256((a.gen/'program_manifest.json').read_bytes()).hexdigest(),'harness_sha256':hashlib.sha256((repo/'tests/lagoon_endgame_windows_oracle_test.c').read_bytes()).hexdigest(),'test_only_selectors':selectors,'instrumented_header_sha256':hashlib.sha256(header.encode()).hexdigest(),'stdout':r.stdout,'limits':'Actual generated bodies with all original operations through first real child boundary, or complete RTS path. Native children STOP at their real original entry; no fake sound/render/script result or return. All18 new windows tested with actual018DC6 initializer and deterministic immediate multiply-register bus model (not hardware IRQ/latency proof). Entry-deadline yields stop before anyopcode. Real services and waits stop at their actual entries; no fabricated returns/outcomes. Genuine authority requires separate opcode/live checks.'},indent=2)+'\n')
