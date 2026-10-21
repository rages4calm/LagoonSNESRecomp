"""Original selector18 dispatch instructions and separate real-JSL tail guards."""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1]
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
expected=next(x.split('=',1)[1].strip()for x in(repo/'rom_identity.txt').read_text().splitlines()if x.startswith('expected_sha256'))
assert sha(a.rom)==expected
source=repo/'snesrecomp/runner/src/snes/interp_bridge.c'
policy=source.read_text().split('/* BEGIN AOT_CONTINUATION_POLICY -- pure, bounded provenance matcher. */',1)[1].split('/* END AOT_CONTINUATION_POLICY */',1)[0]
a.out.mkdir(parents=True,exist_ok=True);(a.out/'endgame_policy_under_test.h').write_text(policy)
binary=a.out/'hud-dispatch-oracle.exe';runner=repo/'snesrecomp/runner/src'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-I'+str(runner),'-I'+str(a.out.resolve()),str(repo/'tests/lagoon_hud_dispatch_oracle_test.c'),str(runner/'snes/interp816.c'),'-o',str(binary)],check=True)
r=subprocess.run([str(binary.resolve()),str(a.rom.resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='')
files=['src/lagoon_native_endgame.c','src/lagoon_native_endgame.h','src/lagoon_native_recovery.c','snesrecomp/runner/src/snes/interp_bridge.c','snesrecomp/runner/src/snes/interp_bridge.h','snesrecomp/runner/src/snes/interp816.c','tests/lagoon_hud_dispatch_oracle_test.c','tests/lagoon_endgame_inline_oracle_test.c','tools/test_hud_dispatch_oracle.py']
(a.out/'result.json').write_text(json.dumps(dict(status='PASS'if r.returncode==0 else'FAIL',rom_sha256=expected,actual_policy_sha256=hashlib.sha256(policy.encode()).hexdigest(),source_sha256={f:sha(repo/f)for f in files},stdout=r.stdout,stderr=r.stderr,scope='Actual original selector18 JSL80B9/completepreparation/JSL80E0/DC80E6 and originalHUDbody/children/PLB/RTL execute with a unit constant bus. Actual SDK/title handoff reads unchanged CPU/RAM and preserves existing cross-bank3byte frame. Full source-span byte mutants and mode/stack/generation/cpu/ram/metadata/intervening-call/replay guards are separate synthetic refusal checks. Not a generated child or runtime scheduler/APU/DMA comparison.'),indent=2)+'\n')
r.check_returncode()
