"""Research candidate BB05 iteration against original opcodes and native closure.

The first build executes actual original child opcodes, never guessed results.
The second includes the complete generated native child closure. RAM executable
fixtures exercise unknown loaded-pointer returns in all four architectural modes.
Production BB05 remains interpreted after a live SPC beam-timing mismatch.
--candidate-gen must identify a separately generated research candidate with
only BB05:M1X0's production floor removed. A passing source oracle cannot
qualify production promotion; matched live MMIO/APU checks are also required.
No ROM, generated C, executable or private fixture data is distributed.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','out'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--candidate-gen',dest='gen',type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text(encoding='utf-8').splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==identity
assert not a.out.exists(),'Choose a fresh private output directory'
manifest=json.loads((a.gen/'program_manifest.json').read_text(encoding='utf-8'))
assert manifest['nodes']['01BB05:M1X0']['disposition']=='aot_eligible'
refused={}
for key in ['01BB05:M0X0','01BB05:M0X1','01BB05:M1X1','01BB4C:M1X1','01BD5B:M0X0','01BD5B:M0X1','01BD5B:M1X1','01BE79:M1X1']:
 disposition=manifest['nodes'].get(key,{}).get('disposition','not_generated');assert disposition!='aot_eligible',key;refused[key]=disposition
functions={}
for f in a.gen.glob('bank*_v2.c'):
 text=f.read_text(encoding='utf-8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',text):
  body=text[m.start():text.index('\n}',m.end())+2]+'\n'
  pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',body)[1],16)
  functions[m[1]]=(pc,body,f)
root='Lagoon_Projectile_UpdateAll_M1X0';body=functions[root][1]
assert 'cpu_unresolved_abandon_balanced(cpu, 0x01BB49' not in body
assert '0xbb4cu), 0x01BB49u, _entry_s, _hrv)' in body
children=sorted(set(re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',body)))
assert {functions[n][0]for n in children}=={0x01bb4b,0x01bd5b,0x01be52,0x01bcca,0x01be79,0x01be6d}
selected={root};pending=[root]
while pending:
 for n in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[pending.pop()][1]):
  assert n in functions and n.endswith('_M1X0'),n
  if n not in selected:selected.add(n);pending.append(n)
a.out.mkdir(parents=True)
header='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in children)+'\n'+body
for n in children:header+=f'RecompReturn {n}(CpuState*c){{return source_child(c,0x{functions[n][0]:06x},NULL);}}\n'
(a.out/'projectile_iteration_under_test.h').write_text(header,encoding='utf-8')
closure='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected))+'\n'+'\n'.join(functions[n][1]for n in sorted(selected))
(a.out/'projectile_iteration_closure_under_test.h').write_text(closure,encoding='utf-8')
harness=repo/'tests/lagoon_projectile_iteration_services_oracle_test.c';results=[]
for native in (False,True):
 exe=a.out/('native-closure.exe'if native else'original-children.exe')
 cmd=[a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-Wno-unused-variable','-I'+str(sdk),'-I'+str(a.out.resolve())]
 if native:cmd+=['-DWHOLE_NATIVE_CLOSURE']
 subprocess.run(cmd+[str(harness),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
 result=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
 print(result.stdout,end='');print(result.stderr,end='')
 results.append(dict(mode='complete-native-closure'if native else'original-source-children',exit=result.returncode,stdout=result.stdout,stderr=result.stderr))
 if result.returncode:break
record=dict(schema='Lagoon BB05 complete iteration candidate source oracle v2',research_only=True,production_promotion_qualified=False,production_floor_reason='Live spell13 SPC beam batch timing differs; original outer loop retained.',passed=len(results)==2 and all(not x['exit']for x in results),rom_sha256=identity,manifest_sha256=sha(a.gen/'program_manifest.json'),tool_sha256=sha(Path(__file__)),harness_sha256=sha(harness),closure={name:dict(pc=f'{functions[name][0]:06X}',body_sha256=hashlib.sha256(functions[name][1].encode()).hexdigest())for name in sorted(selected)},refused_variants=refused,results=results,scope='33 active kinds and empty pool, all16 source slots; sparse/full/mixed pools, retire/collision-free terrain, original fork allocation and same-pass later slots, hiddenA/carry/NV/decimal bounded controls, physicalstacks180/1FD0, six deadlines and unknown returned modes. Full128KiB RAM+IO/registers/physicalstack/CPU-master-copro clocks against original opcodes; constant8-master unitbus with shared multiplier model. Unknown loaded pointers stop at exact source destination. Deliberate source-table-to-WRAM handler fixtures execute actual width-changing opcodes, not real campaign data. Enemy-only title sampler HLE guard disabled in standalone room0 source oracle; separate math qualification required. Natural live-MMIO/pacing/parent integration are separate affected-route gates.')
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
assert record['passed'],'Research candidate source oracle failed'
