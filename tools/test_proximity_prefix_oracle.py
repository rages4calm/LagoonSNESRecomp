"""Original02:81DE prefix/realRTL against the actual generated native candidate.

Stops before original824A JSL0080B3. Callee outcomes are never synthesized.
The existing runtime's immediate multiplier cases are extracted verbatim;
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
for pc in (0x0281de,0x028255):
 assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
 for mode in ('M0X0','M0X1','M1X1'):assert manifest['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
for pc in ():
 for mode in ('M0X0','M0X1','M1X0','M1X1'):assert manifest['nodes'].get(f'{pc}:{mode}',{}).get('disposition')!='aot_eligible'
# Batch46 qualifies these exactM1X0 services; unknown widths remain original.
for pc in ('02824A', '0080B3', '0080E6'):
 for mode in ('M0X0','M0X1','M1X0','M1X1'):
  assert (manifest['nodes'].get(f'{pc}:{mode}',{}).get('disposition')=='aot_eligible')==(mode=='M1X0')
functions={}
for f in a.gen.glob('bank*_v2.c'):
 t=f.read_text(encoding='utf-8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',t):
  body=t[m.start():t.index('\n}',m.end())+2]+'\n';pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',body)[1],16)
  functions[m[1]]=(pc,body,f)
selected={n for n,(pc,body,f)in functions.items()if pc in (0x0281de,0x028255)and n.endswith('_M1X0')}
assert len(selected)==2
# The production824A continuation is now native; this prefix-only oracle
# still stops at its actual source entry and executes no callee outcome.
next_name=next(n for n,(pc,body,f)in functions.items()if pc==0x02824a and n.endswith('_M1X0'))
for name in selected:
 assert set(re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[name][1]))<=selected|{next_name}
a.out.mkdir(parents=True)
(a.out/'proximity_prefix_under_test.h').write_text('\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected))+'\nRecompReturn '+next_name+'(CpuState*c){return interp_bridge_lle_yield_unwind(c,0x02824a); }\n'+'\n'.join(functions[n][1]for n in sorted(selected)),encoding='utf-8')
production=(sdk/'snes/snes.c').read_text(encoding='utf-8')
read=production[production.index('    case 0x4216:'):production.index('    case 0x4016:',production.index('    case 0x4216:'))]
write=production[production.index('    case 0x4202: {'):production.index('    case 0x4204: {')]
(a.out/'proximity_math_under_test.h').write_text('static uint8 original_math_read(TestSnes*snes,unsigned adr){switch(adr){\n'+read+'default:CHECK(0);return 0;}}\nstatic void original_math_write(TestSnes*snes,unsigned adr,uint8 val){switch(adr){\n'+write+'default:CHECK(0);}}\n',encoding='utf-8')
test=repo/'tests/lagoon_proximity_prefix_oracle_test.c';exe=a.out/'proximity-prefix.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-Wno-unused-variable','-I'+str(sdk),'-I'+str(a.out.resolve()),str(test),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='')
record={'passed':run.returncode==0,'research_only':True,'production_promotion_qualified':False,'rom_sha256':identity,'manifest_sha256':sha(a.gen/'program_manifest.json'),'sources':{str(x.relative_to(repo)):sha(x)for x in(test,Path(__file__),sdk/'snes/snes.c')},'generated_bodies':{n:{'pc':f'{functions[n][0]:06X}','sha256':hashlib.sha256(functions[n][1].encode()).hexdigest()}for n in sorted(selected)},'stdout':run.stdout,'stderr':run.stderr,'returncode':run.returncode,'scope':'Actual generated prefix and original65816 source until realRTL or before original824AJSL; no originalcallee effects guessed. FullWRAM/IO/physicalstack/registers/flags/hiddenA and CPU/master/copro clocks, six deadlines,33 actor slots plus adjacentX0840 boundary, eligibility/kind/near/far/wrapping coordinates. Explicit existing paired-host caller PB completion is applied afterNORMAL; pending source-boundary/deadline states are unchanged. Runtime immediate multiplier cases extracted; ordered register traffic checked. No physicalhardware latency or live MMIO/IRQ/parent/promotion claim.'}
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8');assert record['passed']
