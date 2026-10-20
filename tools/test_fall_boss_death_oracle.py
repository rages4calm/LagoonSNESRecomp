"""Actual alternate-death/boss-death bodies and selectors versus ROM opcodes.

The B936/0080E9 child runs original opcodes for its 16 source-owned selectors.
Unknown high-accumulator/selector contexts stop at the real child entry. Rewards
and unsupported actor paths stop at genuine entries, never fake a result.
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
starts=[0x01aaf6,0x01b573,0x01a7d6,0x01acaa]
support=[0x01aac1]
selected={by_pc[pc] for pc in starts+support};stubs={};bindings={};bodies=[]
for pc in starts+support:
    name=by_pc[pc];_,body,file=functions[name]
    bindings[f'{pc:06X}']=dict(name=name,body_sha256=hashlib.sha256(body.encode()).hexdigest(),
                              source_sha256=hashlib.sha256(file.read_bytes()).hexdigest())
    for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',body):
        assert child in functions,child
        if child not in selected:stubs[child]=functions[child][0]
    bodies.append(body)
assert 'L_AB28_M1X0:' in bodies[0] and 'L_AB29_M1X0:' not in bodies[0]
assert 'L_B5ED_M1X0:' in bodies[1] and 'L_B936_M1X0:' not in bodies[1]
manifest=json.loads((a.gen/'program_manifest.json').read_text())
assert manifest['nodes']['01B936:M1X0']['disposition']=='lle_only'
assert manifest['exit_modes']['01B936:M1X0']=={'m':1,'x':0}
for pc in starts[:2]:
    assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
    for mode in ('M0X0','M0X1','M1X1'):
        assert manifest['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
header='\n'.join('RecompReturn '+name+'(CpuState*c);' for name in sorted(selected))+'\n'
header+='\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}'
                  for name,pc in sorted(stubs.items()))+'\n'
header+='\n'.join(bodies)
header+='\nstatic const unsigned death_starts[]={'+','.join(f'0x{pc:x}' for pc in starts)+'};\n'
header+='static RecompReturn death_body(unsigned n,CpuState*c){switch(n){\n'
for n,pc in enumerate(starts):header+=f'case {n}:return {by_pc[pc]}(c);\n'
header+='default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'fall_boss_death_under_test.h').write_text(header,encoding='utf-8')
runner=repo/'snesrecomp/runner/src';harness=repo/'tests/lagoon_fall_boss_death_oracle_test.c'
binary=a.out/'fall-boss-death-oracle.exe'
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
    stdout=r.stdout,limits='Actual source-closed B936/0080E9 executes original opcodes through the existing JSR frame for16 selectors with zero hiddenAhigh. Unknown selectors/highA stop at B936, not fabricated destinations/results. Existing reward/special services stop at their actual entry. FullRAM/registers/flags/saved+deadstack and CPU/master/coprocessor clocks; constant-eight-master-cycle unitbus. Live scheduler/APU/interrupt qualification separate.'),indent=2)+'\n')
