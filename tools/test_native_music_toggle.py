"""Original finite sound toggle writer versus source-owned opcode-start port observation."""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
r=Path(__file__).resolve().parents[1];sdk=r/'snesrecomp/runner/src';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(a.rom)=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb' and not a.out.exists();a.out.mkdir(parents=True)
s=(sdk/'cpu_state.c').read_text(encoding='utf8');s=s[s.index('uint32_t cpu_region_speed(uint32_t adr) {'):];s=s[:s.index('\n}')+2];(a.out/'music_bus_under_test.h').write_text(s+'\n',encoding='utf8')
exe=a.out/'toggle-test.exe';cmd=[a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-unused-parameter','-I'+str(sdk),'-I'+str(a.out.resolve()),str(r/'tests/lagoon_native_music_toggle_test.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)]
c=subprocess.run(cmd,capture_output=True,text=True);(a.out/'compile.txt').write_text(c.stdout+c.stderr,encoding='utf8');assert c.returncode==0,c.stderr
v=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(v.stdout,v.stderr)
files=['src/lagoon_native_music.c','src/lagoon_native_music.h','src/lagoon_native_music_upload.h','tests/lagoon_native_music_test.c','tests/lagoon_native_music_toggle_test.c','tools/test_native_music_toggle.py']
result=dict(passed=v.returncode==0,scope=__doc__,source_sha256={f:sha(r/f)for f in files},rom_sha256=sha(a.rom),sdk_source_sha256={f:sha(sdk/f)for f in ['cpu_state.c','cpu_state.h','snes/interp816.c']},region_function_sha256=hashlib.sha256(s.encode()).hexdigest(),stdout=v.stdout,stderr=v.stderr,returncode=v.returncode)
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8');assert result['passed']
