"""Actual authored bank-dispatch parent scripts and state query, source boundaries."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for k in ('rom','gen','out'):p.add_argument('--'+k,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text().splitlines()if s.startswith('expected_sha256'));assert sha(a.rom)==identity and not a.out.exists()
functions={}
for f in a.gen.glob('bank*_v2.c'):
 s=f.read_text(encoding='utf8')
 for x in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',s):
  b=s[x.start():s.index('\n}',x.end())+2]+'\n';functions[x[1]]=(int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',b)[1],16),b)
name=lambda pc:next(n for n,(v,b)in functions.items()if v==pc and n.endswith('_M1X0'))
roots=[0x0090db,0x01d31d,0x01d3c8,0x028c0d,0x01806f,0x0090ed,0x009114,0x00912a]
selected={name(pc)for pc in roots}|{name(0x01cd90),name(0x01d3cd),name(0x008e2c)};calls=set()
for n in selected:calls.update(re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[n][1]))
assert calls<=functions.keys();a.out.mkdir(parents=True)
header='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected|calls))+'\n'
header+='\n'.join(functions[n][1]for n in sorted(selected))
for n in sorted(calls-selected):header+='\nRecompReturn '+n+'(CpuState*c){return source_boundary(c,0x'+f'{functions[n][0]:06x}'+');}\n'
header+='\nstatic unsigned parent_pcs[]={'+','.join(hex(pc)for pc in roots)+'};\nstatic RecompReturn parent_body(unsigned p,CpuState*c){switch(p){\n'+''.join('case '+hex(pc)+':return '+name(pc)+'(c);\n'for pc in roots)+'default:CHECK(0);return 0;}}\n'
(a.out/'parent_under_test.h').write_text(header,encoding='utf8')
# Reuse the production bus and multiplier extraction from the family driver output.
source=(sdk/'snes/snes.c').read_text(encoding='utf8');read=source[source.index('    case 0x4216:'):source.index('    case 0x4016:',source.index('    case 0x4216:'))];write=source[source.index('    case 0x4202: {'):source.index('    case 0x4204: {')]
(a.out/'bank_dispatch_math_under_test.h').write_text('static uint8 original_math_read(TestSnes*snes,unsigned adr){switch(adr){\n'+read+'default:CHECK(0);return 0;}}\nstatic void original_math_write(TestSnes*snes,unsigned adr,uint8 val){switch(adr){\n'+write+'default:CHECK(0);}}\n',encoding='utf8')
s=(sdk/'cpu_state.c').read_text(encoding='utf8');s=s[s.index('uint32_t cpu_region_speed(uint32_t adr) {'):];s=s[:s.index('\n}')+2];(a.out/'bank_dispatch_bus_under_test.h').write_text(s+'\n',encoding='utf8')
# Use existing family infrastructure without including its generated bodies or campaign.
s=(repo/'tests/lagoon_bank_dispatch_oracle_test.c').read_text(encoding='utf8');s=s[:s.index('static void compare(')];s=s.replace('#include "bank_dispatch_under_test.h"','')
(a.out/'parent_infrastructure.h').write_text(s,encoding='utf8');h=repo/'tests/lagoon_bank_parent_oracle_test.c';exe=a.out/'bank-parent.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-function','-Wno-unused-variable','-Wno-unused-parameter','-Wno-unused-label','-I'+str(sdk),'-I'+str(a.out.resolve()),str(h),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
r=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='');(a.out/'result.json').write_text(json.dumps({'passed':r.returncode==0,'stdout':r.stdout,'stderr':r.stderr,'manifest_sha256':sha(a.gen/'program_manifest.json'),'bodies':{n:{'pc':hex(functions[n][0]),'sha256':hashlib.sha256(functions[n][1].encode()).hexdigest()}for n in sorted(selected)},'stopped_at_actual_child_entries':{n:hex(functions[n][0])for n in sorted(calls-selected)},'sources':{str(x.relative_to(repo)):sha(x)for x in [Path(__file__),h,repo/'tests/lagoon_bank_dispatch_oracle_test.c',sdk/'snes/interp816.c',sdk/'cpu_state.c',sdk/'snes/snes.c']},'scope':'Original CCD5 JSR plus actualCCDB/80E9 table helper establish D31D/D3C8 physical2-byte owners; actualJSL/JSR caller opcodes for other entries. FullRAM/register/deadstack/flags and production-region bytebus clocks. Original008E2C request leaf runs its real LDA/TSB/RTL; external music/wait/HUD children stop at actual entry, no effects or returns synthesized. Actual runtime activation/refresh/APU requires live controls.'},indent=2)+'\n',encoding='utf8');assert r.returncode==0
