"""Original boss motion table: actual generated bodies versus ROM opcodes.

All fifteen newly compiled targets and the actual selector execute. The unchanged
first-landing body stops at its original entry. Ella RNG calls execute the
original009800/9848 instructions with a deterministic hardware-divider unit bus.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','gen','out'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1]
expected=next(line.split('=',1)[1].strip() for line in
              (repo/'rom_identity.txt').read_text().splitlines() if line.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest()==expected
by_pc={};functions={}
for file in a.gen.glob('bank*_v2.c'):
    text=file.read_text(encoding='utf-8')
    for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',text):
        body=text[m.start():text.index('\n}',m.end())+2]+'\n'
        entry=re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',body)
        assert entry,m[1]
        pc=int(entry[1],16);functions[m[1]]=(pc,body,file)
        if m[1].endswith('_M1X0'):by_pc[pc]=m[1]
starts=[0x0290a0,0x0290a1,0x0290bd,0x0290be,0x02911c,0x02914c,
        0x0291a2,0x0291a3,0x029242,0x029243,0x029244,0x029245,
        0x029312,0x0294af,0x029525,0x029036]
support=[]
selected={by_pc[pc] for pc in starts+support};stubs={};bindings={};bodies=[]
for pc in starts+support:
    name=by_pc[pc];_,body,file=functions[name]
    bindings[f'{pc:06X}']=dict(name=name,body_sha256=hashlib.sha256(body.encode()).hexdigest(),
                              source_sha256=hashlib.sha256(file.read_bytes()).hexdigest())
    for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',body):
        assert child in functions,child
        if child not in selected:stubs[child]=functions[child][0]
    bodies.append(body)
manifest=json.loads((a.gen/'program_manifest.json').read_text())
assert manifest['nodes'].get('009800:M1X0',{}).get('disposition')!='aot_eligible'
assert manifest['exit_modes']['009800:M1X0']=={'m':1,'x':0}
for pc in starts[:-1]:
    assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
    for mode in ('M0X0','M0X1','M1X1'):
        assert manifest['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
assert 'L_90EE_M0X0:' in bodies[4] and 'L_90F4_M0X0:' in bodies[4]
assert 'L_9116_M0X0:' in bodies[4] and 'L_911B_M1X0:' in bodies[4]
header='\n'.join('RecompReturn '+name+'(CpuState*c);' for name in sorted(selected))+'\n'
header+='\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}'
                  for name,pc in sorted(stubs.items()))+'\n'
header+='\n'.join(bodies)
header+='\nstatic const unsigned motion_starts[]={'+','.join(f'0x{pc:x}' for pc in starts)+'};\n'
header+='static RecompReturn motion_body(unsigned n,CpuState*c){switch(n){\n'
for n,pc in enumerate(starts):header+=f'case {n}:return {by_pc[pc]}(c);\n'
header+='default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'boss_motion_under_test.h').write_text(header,encoding='utf-8')
runner=repo/'snesrecomp/runner/src';harness=repo/'tests/lagoon_boss_motion_oracle_test.c'
binary=a.out/'boss-motion-oracle.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label',
    '-Wno-unused-parameter','-Wno-unused-variable','-I'+str(runner),'-I'+str(a.out.resolve()),
    str(harness),str(runner/'snes/interp816.c'),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
(a.out/'result.json').write_text(json.dumps(dict(status='PASS',generated_bodies=bindings,
    stopped_children=stubs,rom_sha256=expected,
    manifest_sha256=hashlib.sha256((a.gen/'program_manifest.json').read_bytes()).hexdigest(),
    harness_sha256=hashlib.sha256(harness.read_bytes()).hexdigest(),
    tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    interpreter_sha256=hashlib.sha256((runner/'snes/interp816.c').read_bytes()).hexdigest(),
    stdout=r.stdout,limits='All15 actual new motion bodies including seven RTS slots and the original9036 selector run. Existing906A and unknown pointers stop at actual entries; no fake actor/results. Original009800/9848 RNG instructions run through real3-byteJSL/RTL frames, with a constant-eight-cycle bus and deterministic divider model, not real-device timing proof. FullRAM/registers/flags/Ahigh/saved+deadstack and CPU/master/coprocessor clocks; live scheduler/APU/interrupt qualification separate.'),indent=2)+'\n')
