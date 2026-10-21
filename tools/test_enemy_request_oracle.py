"""Original enemy attack request and finite projectile allocation/sound closure.

Both actual original-opcode children and the complete generated native closure
are compared with the real 65816 interpreter. Unit ports use deterministic IO;
ordered traffic is checked, while live SPC/APU timing needs matched game runs.
No ROM, generated code, private snapshots or assets are embedded/distributed.
"""
from pathlib import Path
import argparse,hashlib,json,re,subprocess

p=argparse.ArgumentParser(description=__doc__)
for k in ('rom','candidate-gen','out'):p.add_argument('--'+k,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=next(s.split('=',1)[1].strip() for s in (repo/'rom_identity.txt').read_text().splitlines() if s.startswith('expected_sha256'))
assert sha(a.rom)==identity and not a.out.exists()
pcs={0x01977f,0x01992c,0x0199ed,0x0199f5,0x0199ff,0x019a11,0x019a24,0x019a3c,0x019a42}
manifest=json.loads((a.candidate_gen/'program_manifest.json').read_text(encoding='utf8'))
for pc in pcs:
 for mode in ('M0X0','M0X1','M1X0','M1X1'):
  assert (manifest['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')=='aot_eligible')==(mode=='M1X0')
for pc in ('019814','0080B3','0080E6'):
 for mode in ('M0X0','M0X1','M1X0','M1X1'):assert manifest['nodes'].get(f'{pc}:{mode}',{}).get('disposition')!='aot_eligible'
functions={}
for f in a.candidate_gen.glob('bank*_v2.c'):
 t=f.read_text(encoding='utf8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',t):
  b=t[m.start():t.index('\n}',m.end())+2]+'\n';pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',b)[1],16)
  functions[m[1]]=(pc,b)
root=next(n for n,(pc,b) in functions.items() if pc==0x01977f and n.endswith('_M1X0'))
ancestor=next(n for n,(pc,b) in functions.items() if pc==0x019592 and n.endswith('_M1X0'))
new={n for n,(pc,b) in functions.items() if pc in pcs and n.endswith('_M1X0')};selected={root,ancestor};pending=[root,ancestor]
while pending:
 for n in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[pending.pop()][1]):
  assert n in functions and n.endswith('_M1X0'),n
  if n not in selected:selected.add(n);pending.append(n)
assert len(new)==9 and new<=selected and {0x019a58,0x0cf8fe}<={functions[n][0] for n in selected}
assert 'case 0x977f' in functions[ancestor][1].lower()
assert 'cpu_tailcall_inherit_return_context(_entry_s, _hrv)' in functions[ancestor][1]
assert 'interp_tier_dispatch_tail(cpu' in functions[ancestor][1]
a.out.mkdir(parents=True)
for whole in (False,True):
 names=selected if whole else new|{ancestor}
 body='\n'.join('RecompReturn '+n+'(CpuState*c);' for n in sorted(selected))+'\n'
 body+='\n'.join(functions[n][1] for n in sorted(names))
 if not whole:
  for n in sorted(selected-names):body+=f'\nRecompReturn {n}(CpuState*c){{return source_child(c,0x{functions[n][0]:06x},NULL);}}\n'
 (a.out/('enemy_request_closure.h' if whole else 'enemy_request_children.h')).write_text(body,encoding='utf8')
results=[];harness=repo/'tests/lagoon_enemy_request_oracle_test.c'
for whole in (False,True):
 executable=a.out/('native-closure.exe' if whole else 'original-children.exe')
 command=[a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-Wno-unused-variable','-I'+str(sdk),'-I'+str(a.out.resolve())]
 if whole:command+=['-DWHOLE_NATIVE_CLOSURE']
 subprocess.run(command+[str(harness),str(sdk/'snes/interp816.c'),'-o',str(executable)],check=True)
 run=subprocess.run([str(executable.resolve()),str(a.rom.resolve())],text=True,capture_output=True)
 print(run.stdout,end='');print(run.stderr,end='');results.append({'whole_native_closure':whole,'returncode':run.returncode,'stdout':run.stdout,'stderr':run.stderr})
 if run.returncode:break
record={'passed':len(results)==2 and all(not r['returncode'] for r in results),'research_only':True,'production_promotion_qualified':False,'rom_sha256':identity,'manifest_sha256':sha(a.candidate_gen/'program_manifest.json'),'sources':{str(f.relative_to(repo)):sha(f) for f in [Path(__file__),harness,sdk/'snes/interp816.c']},'closure':{n:{'pc':f'{functions[n][0]:06X}','body_sha256':hashlib.sha256(functions[n][1].encode()).hexdigest()} for n in sorted(selected)},'results':results,'scope':'Exact original request selection, real2-byte caller frame, PHA/PHX/PHY, firstfree projectile allocation and soundchildren. Full128KiBRAM/IO/physicalstack/registers/hiddenA/flags/clocks and ordered IO read/write traffic; constant8-master unitbus. Deterministic port storage does not model real SPC/DSP/time; strict sameEXE live APU-event controls remain required. Allother entrywidths and adjacent9814 interpreter floors verified; no callee outcomes guessed.'}
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf8');assert record['passed']
