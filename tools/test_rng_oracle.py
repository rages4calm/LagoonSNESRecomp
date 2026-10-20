"""Complete native009800/9848 versus original opcodes; owner ROM stays external."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
p=argparse.ArgumentParser(description=__doc__)
for name in ('rom','gen','out'):p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args();repo=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text().splitlines()if s.startswith('expected_sha256'));assert sha(a.rom)==expected
allfuncs={}
for file in a.gen.glob('bank*_v2.c'):
 s=file.read_text(encoding='utf8')
 for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',s):
  body=s[m.start():s.index('\n}',m.end())+2]+'\n';entry=re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9A-Fa-f]+)',body);assert entry;allfuncs[m[1]]=(int(entry[1],16),body,file)
names=[next(n for n,(pc,b,f)in allfuncs.items()if pc==target and n.endswith('_M1X0'))for target in [0x9800,0x9848]]
m=json.loads((a.gen/'program_manifest.json').read_text());bindings={}
for n in names:
 pc,b,file=allfuncs[n];assert m['nodes'][f'{pc:06X}:M1X0']['disposition']=='aot_eligible'
 for mode in ['M0X0','M0X1','M1X1']:assert m['nodes'].get(f'{pc:06X}:{mode}',{}).get('disposition')!='aot_eligible'
 assert all(c in names for c in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',b));bindings[f'{pc:06X}']=dict(name=n,body_sha256=hashlib.sha256(b.encode()).hexdigest(),source_sha256=sha(file))
a.out.mkdir(parents=True,exist_ok=True)
header='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in names)+'\n'+'\n'.join(allfuncs[n][1]for n in names)
header+='\nstatic RecompReturn rng_body(unsigned n,CpuState*c){return n?'+names[1]+'(c):'+names[0]+'(c);}'
(a.out/'rng_under_test.h').write_text(header,encoding='utf8');runner=repo/'snesrecomp/runner/src';harness=repo/'tests/lagoon_rng_oracle_test.c';binary=a.out/'rng-oracle.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-label','-Wno-unused-parameter','-Wno-unused-variable','-I'+str(runner),'-I'+str(a.out.resolve()),str(harness),str(runner/'snes/interp816.c'),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='');r.check_returncode()
(a.out/'result.json').write_text(json.dumps(dict(status='PASS',generated=bindings,rom_sha256=expected,manifest_sha256=sha(a.gen/'program_manifest.json'),harness_sha256=sha(harness),tool_sha256=sha(Path(__file__)),interpreter_sha256=sha(runner/'snes/interp816.c'),stdout=r.stdout,scope='Full original RNG/seed bodies with same-bank2/3-byte native host return frames (cross-bank native caller ABI separately tested by actualElla motion); unitdivider immediate andconstant8-cyclebus, separate live hardware timing required; zero-flag spin loop also compared through real instruction-deadline unwind'),indent=2)+'\n',encoding='utf8')
