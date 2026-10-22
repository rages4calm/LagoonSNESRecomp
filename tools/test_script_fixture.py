"""Original-source-bound diagnostic NPC script input; no game-state outcomes synthesized."""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected=next(s.split('=',1)[1].strip()for s in(repo/'rom_identity.txt').read_text(encoding='utf8').splitlines()if s.startswith('expected_sha256'))
assert sha(a.rom)==expected and not a.out.exists();a.out.mkdir(parents=True)
exe=a.out/'script-fixture.exe';test=repo/'tests/lagoon_script_fixture_test.c'
subprocess.run([a.compiler,'-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(repo/'snesrecomp/runner/src'),str(test),'-o',str(exe)],check=True)
r=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='')
record=dict(passed=r.returncode==0,rom_sha256=expected,stdout=r.stdout,stderr=r.stderr,source_sha256={str(p.relative_to(repo)):sha(p)for p in [Path(__file__),test,repo/'src/lagoon_script_fixture.h',repo/'src/lagoon_native_endgame.c',repo/'tools/qualify_native.py']},scope='Exact source01CC9A after original CC87..CC98 pointer loads and original NPC interaction809A/9B34/CC7B. Existing ROM aliases05C6AF=1EFF and05CAB1=27FF. Only A1/A2 rewritten once per state generation; all original stack/PC/flags/request/outcome untouched. Default off; actual hook/runtime activation is separate.')
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf8');assert record['passed']
