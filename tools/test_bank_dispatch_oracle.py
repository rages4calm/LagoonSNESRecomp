"""Actual bank1/2 wrappers, long-pointer tail, physical cleanup and retirement.

Only command19's finite original spawn child is closed natively in this oracle;
other known/unknown target entries remain explicit source-boundary handoffs.
Private WRAM CPU-bytecode fixtures exercise all cleanup widths and a nonlocal
return by executing actual interp816 instructions, never fabricated outcomes.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for k in ('rom','gen','out'):p.add_argument('--'+k,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text().splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==identity and not a.out.exists()
m=json.loads((a.gen/'program_manifest.json').read_text(encoding='utf8'))['nodes']
core={0x0080b3,0x0080b9,0x0080e6,0x02824a}
for pc in core:
 assert m[f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
 for mode in ('M0X0','M0X1','M1X1'):assert m.get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
for mode in ('M0X0','M0X1','M1X0','M1X1'):assert m[f'0080E4:{mode}']['disposition']=='aot_eligible'
functions={}
for f in a.gen.glob('bank*_v2.c'):
 t=f.read_text(encoding='utf8')
 for found in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',t):
  body=t[found.start():t.index('\n}',found.end())+2]+'\n';pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',body)[1],16);functions[found[1]]=(pc,body)
name=lambda pc,mode='M1X0':next(n for n,(v,b)in functions.items()if v==pc and n.endswith('_'+mode))
selected={name(pc)for pc in core|{0x0281de,0x028255}}
selected.update(name(0x0080e4,mode)for mode in ('M0X0','M0X1','M1X0','M1X1'))
child={name(0x018c54)};todo=list(child)
while todo:
 for n in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[todo.pop()][1]):
  assert n in functions
  if n not in child:child.add(n);todo.append(n)
selected|=child
calls=set()
for n in selected:calls.update(re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[n][1]))
assert calls<=functions.keys()
a.out.mkdir(parents=True)
body='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected|calls))+'\n'
body+='\n'.join(functions[n][1]for n in sorted(selected))
for n in sorted(calls-selected):body+='\nRecompReturn '+n+'(CpuState*c){return source_boundary(c,0x'+f'{functions[n][0]:06x}'+');}\n'
for label,pc in [('BANK_B3',0x0080b3),('BANK_B9',0x0080b9),('BANK_RETIRE',0x0281de)]:body+='\n#define '+label+' '+name(pc)+'\n'
body+='\nstatic RecompReturn (*const bank_cleanup[4])(CpuState*)={'+','.join(name(0x0080e4,mode)for mode in ('M0X0','M0X1','M1X0','M1X1'))+'};\n'
(a.out/'bank_dispatch_under_test.h').write_text(body,encoding='utf8')
source=(sdk/'snes/snes.c').read_text(encoding='utf8');read=source[source.index('    case 0x4216:'):source.index('    case 0x4016:',source.index('    case 0x4216:'))];write=source[source.index('    case 0x4202: {'):source.index('    case 0x4204: {')]
(a.out/'bank_dispatch_math_under_test.h').write_text('static uint8 original_math_read(TestSnes*snes,unsigned adr){switch(adr){\n'+read+'default:CHECK(0);return 0;}}\nstatic void original_math_write(TestSnes*snes,unsigned adr,uint8 val){switch(adr){\n'+write+'default:CHECK(0);}}\n',encoding='utf8')
timing_source=(sdk/'cpu_state.c').read_text(encoding='utf8')
region=timing_source[timing_source.index('uint32_t cpu_region_speed(uint32_t adr) {'):];region=region[:region.index('\n}')+2]
(a.out/'bank_dispatch_bus_under_test.h').write_text(region+'\n',encoding='utf8')
harness=repo/'tests/lagoon_bank_dispatch_oracle_test.c';exe=a.out/'bank-dispatch.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-variable','-Wno-unused-parameter','-I'+str(sdk),'-I'+str(a.out.resolve()),str(harness),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='')
r={'passed':run.returncode==0,'returncode':run.returncode,'stdout':run.stdout,'stderr':run.stderr,'manifest_sha256':sha(a.gen/'program_manifest.json'),'rom_sha256':identity,'sources':{str(x.relative_to(repo)):sha(x)for x in(harness,Path(__file__),sdk/'snes/interp816.c',sdk/'snes/snes.c',sdk/'cpu_state.c')},'actual_generated_bodies':{n:{'pc':f'{functions[n][0]:06X}','sha256':hashlib.sha256(functions[n][1].encode()).hexdigest()}for n in sorted(selected)},'other_native_targets_stopped_at_actual_entry':{n:f'{functions[n][0]:06X}'for n in sorted(calls-selected)},'scope':'Full generated bank dispatch/physical cleanup and81DE/824A retirement with finite actual018C54 child closure. Every other target is an explicit interpreter-entry boundary, no handler effects synthesized. All4 cleanup widths and nonlocal return use real65816 WRAM CPU-bytecode fixtures, not authored Lagoon coverage. Actual production region-speed bytebus with extracted runtime multiplier model; commit/refresh/APU effects require live controls, fullRAM/IO/physicalstack/registers/flags/CPU/master/copro clocks. Live admission, source/SPC clocks and APU events require matched controls.'}
(a.out/'result.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf8');assert r['passed']
