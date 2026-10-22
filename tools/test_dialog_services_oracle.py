"""Actual generated dialogue cuts versus original instructions and physical frames.

External calls stop at their real entry with the original frame intact. This
checks the new bodies and typed continuations, not NMI/SPC completion or cadence.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','gen','out'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest()
expected=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text().splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==expected and not a.out.exists();a.out.mkdir(parents=True)
cfg=(repo/'recomp/bank02.cfg').read_text(encoding='utf8');tail=cfg.split('# Batch48:',1)[1]
roots={0x020000|int(x,16)for x in re.findall(r'^end_at ([0-9a-fA-F]+) ',tail,re.M)}
offsets={}
for line in cfg.splitlines():
 if line.startswith('func '):
  q=line.split();pc=0x020000|int(q[2],16)
  offsets[pc]=next((int(x.split(':')[1])for x in q if x.startswith('entry_s_offset:')),0)
functions={};by_pc={}
for file in a.gen.glob('bank*_v2.c'):
 text=file.read_text(encoding='utf8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',text):
  body=text[m.start():text.index('\n}',m.end())+2]+'\n'
  pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',body)[1],16)
  functions[m[1]]=(pc,body)
  if m[1].endswith('_M1X0'):by_pc[pc]=m[1]
assert roots<=by_pc.keys(),[hex(x)for x in roots-by_pc.keys()]
manifest=json.loads((a.gen/'program_manifest.json').read_text())
for pc in roots:
 assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
 for mode in ('M0X0','M0X1','M1X1'):
  assert manifest['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
selected={by_pc[pc]for pc in roots};stopped={}
for name in selected:
 for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[name][1]):
  assert child in functions,child
  if child not in selected:stopped[child]=functions[child][0]
header='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected|stopped.keys()))+'\n'
header+='\n'.join(f'RecompReturn {n}(CpuState*c){{return dialog_stop(c,0x{pc:06x});}}'for n,pc in sorted(stopped.items()))+'\n'
header+='\n'.join(functions[n][1]for n in sorted(selected))
header+='\nstatic const unsigned dialog_roots[]={'+','.join(hex(x)for x in sorted(roots))+'};\n'
header+='static unsigned dialog_offset(unsigned p){switch(p){'+''.join(f'case 0x{x:06x}:return {offsets.get(x,0)};'for x in sorted(roots))+'default:CHECK(0);return 0;}}\n'
header+='static RecompReturn dialog_body(CpuState*c,unsigned p){switch(p){'+''.join(f'case 0x{x:06x}:return {by_pc[x]}(c);'for x in sorted(roots))+'default:CHECK(0);return RECOMP_RETURN_NORMAL;}}\n'
(a.out/'dialog_services_under_test.h').write_text(header,encoding='utf8')
(a.out/'inline_script_closure_under_test.h').write_text('static RecompReturn native_script(CpuState*c,uint32 p){(void)c;(void)p;CHECK(0);return RECOMP_RETURN_NORMAL;}\n')
timing=(sdk/'cpu_state.c').read_text(encoding='utf8');timing=timing[timing.index('uint32_t cpu_region_speed(uint32_t adr) {'):];timing=timing[:timing.index('\n}')+2]
(a.out/'script_bus_under_test.h').write_text(timing+'\n')
test=repo/'tests/lagoon_dialog_services_oracle_test.c';exe=a.out/'dialog-source.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-function','-Wno-unused-label','-Wno-unused-variable','-Wno-unused-parameter','-I'+str(sdk),'-I'+str(a.out.resolve()),str(test),str(repo/'src/lagoon_native_music.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='')
record=dict(passed=run.returncode==0,stdout=run.stdout,stderr=run.stderr,roots=len(roots),generated_bodies={n:dict(pc=f'{functions[n][0]:06X}',sha256=hashlib.sha256(functions[n][1].encode()).hexdigest())for n in sorted(selected)},external_entry_boundaries=stopped,manifest_sha256=sha(a.gen/'program_manifest.json'),rom_sha256=expected,source_sha256={str(x.relative_to(repo)):sha(x)for x in [Path(__file__),test,repo/'tests/lagoon_inline_dispatch_oracle_test.c',repo/'recomp/bank02.cfg',sdk/'cpu_state.c',sdk/'snes/interp816.c']},scope='Actual source instructions through normal physical returns or first genuine external-call/deadline entry. All RAM/IO/registers/flags/physicalstack and CPU/master/copro clocks; actual production region costs. No fake return, input, NMI or SPC acknowledgment. Live device and variable child services are separate gates.')
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf8');assert record['passed']
