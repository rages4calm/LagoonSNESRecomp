"""Original01:D3E9 item prefix/realRTS against the actual generated native candidate.

Stops before originalD409 JSL0080E9. Callee outcomes are never synthesized.
The physical2-byte ancestor frame is made by original0181A7 JSR;
source-opcode CPU/master/coprocessor clocks use a constant8-master byte bus.
Actual-MMIO latency/event clocks and source-owned live frames need live controls.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','gen','out'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text().splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==identity and not a.out.exists()
manifest=json.loads((a.gen/'program_manifest.json').read_text())
for pc in (0x01d3e9,0x01d45f):
 assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
 for mode in ('M0X0','M0X1','M1X1'):assert manifest['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
for pc in ('01D409','0080E9','0080B3','0080E6'):
 for mode in ('M0X0','M0X1','M1X0','M1X1'):assert manifest['nodes'].get(f'{pc}:{mode}',{}).get('disposition')!='aot_eligible'
functions={}
for f in a.gen.glob('bank*_v2.c'):
 t=f.read_text(encoding='utf-8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',t):
  body=t[m.start():t.index('\n}',m.end())+2]+'\n';pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',body)[1],16)
  functions[m[1]]=(pc,body,f)
selected={n for n,(pc,body,f)in functions.items()if pc in (0x01d3e9,0x01d45f)and n.endswith('_M1X0')}
assert len(selected)==2
for name in selected:
 assert set(re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[name][1]))<=selected
a.out.mkdir(parents=True)
(a.out/'item_prefix_under_test.h').write_text('\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected))+'\n'+'\n'.join(functions[n][1]for n in sorted(selected))+'\n#define ITEM_PREFIX '+next(n for n in selected if functions[n][0]==0x01d3e9)+'\n',encoding='utf-8')
test=repo/'tests/lagoon_item_prefix_oracle_test.c';exe=a.out/'item-prefix.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-Wno-unused-variable','-I'+str(sdk),'-I'+str(a.out.resolve()),str(test),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='')
record={'passed':run.returncode==0,'research_only':True,'production_promotion_qualified':False,'rom_sha256':identity,'manifest_sha256':sha(a.gen/'program_manifest.json'),'sources':{str(x.relative_to(repo)):sha(x)for x in(test,Path(__file__),sdk/'snes/interp816.c')},'generated_bodies':{n:{'pc':f'{functions[n][0]:06X}','sha256':hashlib.sha256(functions[n][1].encode()).hexdigest()}for n in sorted(selected)},'stdout':run.stdout,'stderr':run.stderr,'returncode':run.returncode,'scope':'Actual generated D3E9 request/selection prefix and existing D45F RTS versus original65816 source. Real outer frame is produced by original0181A7 JSR; stop before originalD409 JSL. FullWRAM/IO/deadstack/registers/flags/hiddenA and CPU/master/copro clocks, source deadlines. No helper synthesis, guessed handler effects, physical MMIO latency or live activation claim.'}
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8');assert record['passed']
