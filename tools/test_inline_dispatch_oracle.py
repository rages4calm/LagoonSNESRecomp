"""Six source-owned inline table families against actual original opcodes.

A constant eight-master unit bus checks complete helper state and instruction
clocks. Actual paired scheduler/native-child and APU behavior require separate
matched live controls; this tool does not certify those domains.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','out'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--compiler',default='clang')
p.add_argument('--magic-gen',type=Path,help='Additionally execute the complete current native magic-handler closure.')
p.add_argument('--script-gen',type=Path,help='Execute all27 exact available CCDB script handlers and their generated closure.')
a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest()
expected=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text(encoding='utf-8').splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==expected and not a.out.exists()
a.out.mkdir(parents=True);exe=a.out/'inline-dispatch.exe';test=repo/'tests/lagoon_inline_dispatch_oracle_test.c'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-variable','-I'+str(sdk),str(test),str(repo/'src/lagoon_native_music.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
r=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='')
bridge=sdk/'snes/interp_bridge.c';bridge_text=bridge.read_text(encoding='utf-8')
start=bridge_text.index('RecompReturn interp_tier_dispatch_rewritten_return(CpuState *cpu,')
body=bridge_text[start:bridge_text.index('\n}',start)+2]
(a.out/'inline_rewrite_under_test.h').write_text(body+'\n',encoding='utf-8')
policy_test=repo/'tests/lagoon_inline_rewrite_policy_test.c';policy_exe=a.out/'rewrite-policy.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(sdk),'-I'+str(a.out.resolve()),str(policy_test),'-o',str(policy_exe)],check=True)
policy_run=subprocess.run([str(policy_exe.resolve())],capture_output=True,text=True);print(policy_run.stdout,end='');print(policy_run.stderr,end='')
magic_run=None;magic_bodies={}
if a.magic_gen:
 import re
 functions={}
 for f in a.magic_gen.glob('bank*_v2.c'):
  text=f.read_text(encoding='utf-8')
  for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',text):
   content=text[m.start():text.index('\n}',m.end())+2]+'\n'
   pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',content)[1],16)
   functions[m[1]]=(pc,content)
 pcs={0x195ee,0x19600,0x19619,0x1962f,0x19647,0x1965f,0x19677,0x19690}
 roots={n for n,(pc,body)in functions.items()if pc in pcs and n.endswith('_M1X0')}
 assert len(roots)==8
 selected=set(roots);pending=list(roots)
 while pending:
  for n in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[pending.pop()][1]):
   assert n in functions,n
   if n not in selected:selected.add(n);pending.append(n)
 header='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected))+'\n'+'\n'.join(functions[n][1]for n in sorted(selected))
 header+='\nstatic RecompReturn native_magic(CpuState*c,uint32 p){switch(p){\n'+''.join(f'case 0x{functions[n][0]:06x}:return {n}(c);\n'for n in sorted(roots))+'default:CHECK(0);return RECOMP_RETURN_NORMAL;}}\n'
 (a.out/'inline_magic_closure_under_test.h').write_text(header,encoding='utf-8')
 exe=a.out/'native-magic.exe'
 subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-variable','-Wno-unused-label','-Wno-unused-function','-Wno-unused-parameter','-DNATIVE_MAGIC_CLOSURE','-I'+str(sdk),'-I'+str(a.out.resolve()),str(test),str(repo/'src/lagoon_native_music.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
 magic_run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(magic_run.stdout,end='');print(magic_run.stderr,end='')
 magic_bodies={n:dict(pc=f'{functions[n][0]:06X}',sha256=hashlib.sha256(functions[n][1].encode()).hexdigest())for n in sorted(selected)}
script_run=None;script_bodies={}
if a.script_gen:
 import re
 functions={}
 for f in a.script_gen.glob('bank*_v2.c'):
  text=f.read_text(encoding='utf8')
  for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{',text):
   content=text[m.start():text.index('\n}',m.end())+2]+'\n'
   pc=int(re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)',content)[1],16)
   functions[m[1]]=(pc,content)
 pcs={0x1cd65,0x1cd79,0x1cd8d,0x1cda0,0x1cde3,0x1cdf6,0x1ce0a,0x1ce1e,0x1d17a,0x1d193,0x1d199,0x1d20c,0x1d27d,0x1d2bb,0x1d2c5,0x1d2cb,0x1d2d1,0x1d2e1,0x1d2e9,0x1d2f0,0x1d31d,0x1d341,0x1d352,0x1d3b4,0x1d3bb,0x1d3c1,0x1d3c8}
 roots={n for n,(pc,body)in functions.items()if pc in pcs and n.endswith('_M1X0')}
 assert len(roots)==27
 selected=set(roots);pending=list(roots)
 while pending:
  for n in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)',functions[pending.pop()][1]):
   assert n in functions,n
   if n not in selected:selected.add(n);pending.append(n)
 header='\n'.join('RecompReturn '+n+'(CpuState*c);'for n in sorted(selected))+'\n'+'\n'.join(functions[n][1]for n in sorted(selected))
 header+='\nstatic RecompReturn native_script(CpuState*c,uint32 p){switch(p){\n'+''.join(f'case 0x{functions[n][0]:06x}:return {n}(c);\n'for n in sorted(roots))+'default:CHECK(0);return RECOMP_RETURN_NORMAL;}}\n'
 (a.out/'inline_script_closure_under_test.h').write_text(header,encoding='utf8')
 timing=(sdk/'cpu_state.c').read_text(encoding='utf8');region=timing[timing.index('uint32_t cpu_region_speed(uint32_t adr) {'):];region=region[:region.index('\n}')+2]
 (a.out/'script_bus_under_test.h').write_text(region+'\n',encoding='utf8')
 exe=a.out/'native-script.exe'
 subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Wno-unused-variable','-Wno-unused-label','-Wno-unused-function','-Wno-unused-parameter','-DNATIVE_SCRIPT_CLOSURE','-I'+str(sdk),'-I'+str(a.out.resolve()),str(test),str(repo/'src/lagoon_native_music.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
 script_run=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(script_run.stdout,end='');print(script_run.stderr,end='')
 script_bodies={n:dict(pc=f'{functions[n][0]:06X}',sha256=hashlib.sha256(functions[n][1].encode()).hexdigest())for n in sorted(selected)}
files=[Path(__file__),test,policy_test,bridge,repo/'src/lagoon_native_inline.c',repo/'src/lagoon_native_inline.h',repo/'src/lagoon_native_music.c',repo/'src/lagoon_native_music.h',sdk/'cpu_state.c',sdk/'snes/interp816.c']
record=dict(passed=r.returncode==0 and policy_run.returncode==0 and (magic_run is None or magic_run.returncode==0) and (script_run is None or script_run.returncode==0),script_scope='All27 exact available CCDB handlers via realoriginalCCD5JSR/CCDBJSL plus fullgenerated closure; actualproductionregion-speed bus. Only currentavailable generatedbodies included; flooredmusicupload3 stayoriginal. ActualmusicRunwrapper declines standaloneunregisteredhostcontext anddispatches realoriginalleafsource; newnative3writer execution/devicepolicy has separate test_native_music.py/livegates. Unknown tails execute originalopcodechildren with their actualphysicalowner; actualNMI/SPCwaits retain realdeadline boundaries, no fabricatedack/NMI/return. No live APU/device cadence claim.',script_closure=script_bodies,script_stdout=script_run.stdout if script_run else None,script_stderr=script_run.stderr if script_run else None,script_manifest_sha256=sha(a.script_gen/'program_manifest.json') if a.script_gen else None,magic_scope='All eight actual95CA targets via realancestorJSR/helperJSL+nativehelper+fullavailablegeneratedclosure; realoriginal-opcode gap calls. Original8985 NMIwait stops at firstcompleted8993pollbackedge, neverfabricatesNMI/return. Guardedactor/QoLoptimizationsrefuse in standaloneoriginal-sourcecontext. No actualAPU/device/MMIOcadence claim; matchedlivegate required.',magic_manifest_sha256=sha(a.magic_gen/'program_manifest.json') if a.magic_gen else None,magic_closure=magic_bodies,magic_stdout=magic_run.stdout if magic_run else None,magic_stderr=magic_run.stderr if magic_run else None,rewrite_policy_body_sha256=hashlib.sha256(body.encode()).hexdigest(),rewrite_policy_stdout=policy_run.stdout,rewrite_policy_stderr=policy_run.stderr,rom_sha256=expected,source_sha256={str(x.relative_to(repo)):sha(x)for x in files},stdout=r.stdout,stderr=r.stderr,returncode=r.returncode,scope='Six authored bank1 tables (CCDB admits27 exact available handlers), actual physical parent JSRs and helper JSL, full original80E9..8108 instructions, hiddenA/unknown selectors/flags/deadlines, pure guard refusal mutations. FullWRAM/IO/deadstack/registers/flags/CPU/master/copro clocks against original opcodes withconstant8-master bus. Existing rewritten-return API is a recorded boundary stub here; live nativechild/bridge/APU qualification separate.')
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8');assert record['passed']
