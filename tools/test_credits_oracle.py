"""Actual24 full credits services versus original opcode source-entry fixtures.

Every emitted instruction label has an explicit CPU/saved-stack unit fixture;
post-service fixtures grant no production reentry authority. Actual ED03/ED0A
execute, other children STOP at real source entries without fabricated returns.
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
p.add_argument('--include-existing-palette-child',action='store_true',
 help='Diagnostic: also execute prior coarse008E2C; current known unit master-clock mismatch is expected to fail, never masked.')
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
required=[0x02E9BF,0x02E9D6,0x02E9E3,0x02EA45,0x02EA6E,0x02EAC7,
  0x02EAEB,0x02EB26,0x02EB52,0x02EB9C,0x02EBB2,0x02EC19,0x02EC69,
  0x02ECC3,0x02ECA9,0x02ECEF,0x02ECF8,0x02ED03,0x02ED0A,0x02ED53,
  0x02ED57,0x02ED6C,0x02ED81,0x02ED8F]
assert all(pc in by_pc for pc in required),[hex(pc) for pc in required if pc not in by_pc]
support=[0x02ED0A,0x02ED03]+([0x008E2C] if a.include_existing_palette_child else [])
support_names={by_pc[pc][0] for pc in support}
bodies=[];bindings={};stubs={};selectors={};fixtures=[]
def saved(pc):
 if pc in (0xEACA,0xED57):return 0
 if 0xEACB<=pc<=0xEAD7:return 2
 if pc in (0xED54,0xED58,0xED90):return 2
 if pc in (0xED66,0xEDB9):return 2
 if 0xED55<=pc<=0xED65 or 0xED59<=pc<=0xED65 or 0xED91<=pc<=0xEDB8:return 4
 if 0xED82<=pc<=0xED8D:return 2
 return 0
for pos,pc in enumerate(required+support):
 name,body,file=by_pc[pc]
 bindings[f'{pc:06X}']={'name':name,'body_sha256':hashlib.sha256(body.encode()).hexdigest(),'source_sha256':hashlib.sha256(file.read_bytes()).hexdigest()}
 for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',body):
  assert child in all_functions,child
  if child not in support_names:stubs[child]=all_functions[child][0]
 if pos<len(required):
  # Required definitions are independently selected at their ACTUAL generated
  # instruction labels. This is test-only source-entry fixture instrumentation,
  # not a production reentry or a fabricated child return.
  labels=re.findall(r'  L_([0-9A-F]+)_M([01])X([01]):',body)
  selector='  switch(credits_entry){\n'+''.join(f'    case0x{q}:goto L_{q}_M{m}X{x};\n'.replace('case0x','case 0x') for q,m,x in labels)+'  }\n'
  first='  L_'+labels[0][0]+'_M'+labels[0][1]+'X'+labels[0][2]+':'
  body=body.replace(first,selector+first,1)
  selectors[f'{pc:06X}']=selector
  body=body.replace(f'RecompReturn {name}(',f'RecompReturn UnderTest_{pc:06X}(',1)
  for q,m,x in labels:fixtures.append((pc,int(q,16),int(m),int(x),saved(int(q,16)),0))
  fixtures.append((pc,pc&65535,1,0,0,1)) # actual ordinary root entry deadline
 bodies.append(body)
header='\n'.join('RecompReturn '+name+'(CpuState*c);' for name in support_names)+'\n'
header+='\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}' for name,pc in sorted(stubs.items()))+'\n'
header+='\n'.join(bodies)
header+='\nstatic const struct {unsigned body,pc,m,x,saved,expired;} credit_cases[]={\n'
header+=''.join('{'+','.join(str(v) for v in row)+'},\n' for row in fixtures)+'};\n'
header+='static RecompReturn credit_body(unsigned body,CpuState*c){switch(body){\n'
for pc in required:header+=f'case 0x{pc:06X}:return UnderTest_{pc:06X}(c);\n'
header+='default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'credits_under_test.h').write_text(header,encoding='utf-8')
binary=a.out/'credits-opcodes.exe';runner=repo/'snesrecomp/runner/src'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-I'+str(runner),'-I'+str(a.out.resolve()),str(repo/'tests/lagoon_credits_oracle_test.c'),str(runner/'snes/interp816.c'),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
(a.out/'result.json').write_text(json.dumps({'status':'PASS','generated_bodies':bindings,'rom_sha256':expected,'manifest_sha256':hashlib.sha256((a.gen/'program_manifest.json').read_bytes()).hexdigest(),'harness_sha256':hashlib.sha256((repo/'tests/lagoon_credits_oracle_test.c').read_bytes()).hexdigest(),'test_only_selectors':selectors,'instruction_fixtures':len(fixtures),'existing_child_limit':{'pc24':'008E2C','finding':'Actual old coarse native child preserves CPU/RAM but is+6master/copro versus original under constant8master unitbus. It remains explicit genuine-entry stop in this new-service oracle; clock assertions unchanged. Connected old/new live clocks must independently match.'},'instrumented_header_sha256':hashlib.sha256(header.encode()).hexdigest(),'stdout':r.stdout,'limits':'Actual generated bodies with all original operations through first real child boundary, or complete RTS path. Native children STOP at their real original entry; no fake sound/render/script result or return. All24 full generated credits services tested at their original instruction labels using explicit source-entry CPU/stack fixtures (not runtime reentry authority). ActualED0A/ED03 helper operations execute unchanged; every other child STOPs at its genuine source entry with no fabricated return/outcome. Full128KiBRAM/CPU/flags/real saved/deadstack and CPU/master/coprocessor clocks match original instructions under a constant8master bus, unmapped reads0. Root-entry deadlines test actual original top guard. Real scheduling/devices and seven-scene flow require the separate connected live pair.'},indent=2)+'\n')
