"""Compare exact generated death-flash instruction ranges with original ROM opcodes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', required=True, type=Path)
p.add_argument('--gen', required=True, type=Path)
p.add_argument('--out', required=True, type=Path)
p.add_argument('--compiler', default='clang')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
expected = next(x.split('=', 1)[1].strip() for x in
                (repo / 'rom_identity.txt').read_text().splitlines()
                if x.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest() == expected
names = ['Lagoon_Death_FlashSetup', 'Lagoon_Death_FlashQueue', 'Lagoon_Death_FlashStep']
texts = [(f, f.read_text()) for f in a.gen.glob('bank01*_v2.c')]
bodies, sources = [], {}
for name in names:
    needle = f'RecompReturn {name}_M1X0(CpuState *cpu) {{'
    matches = [(f, text) for f, text in texts if needle in text]
    assert len(matches) == 1, name
    source, text = matches[0]
    begin = text.index(needle)
    body = text[begin:text.index('\n}', begin) + 2] + '\n'
    bodies.append(body)
    sources[name] = {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                     'body_sha256': hashlib.sha256(body.encode()).hexdigest()}
assert all('L_'+pc+'_M1X0:' in body for pc,body in zip(['9346','9368','9371'],bodies))
# Test-only entry selection reaches each original instruction range around
# real sound calls. ALL generated guest operations and clocks remain verbatim.
# The two child sentinels stop at real callee entry and fabricate no results.
selector = "  if(flash_test_entry==0x934c)goto L_934C_M1X0;\n  if(flash_test_entry==0x9352)goto L_9352_M1X0;\n"
needle = '  L_9346_M1X0:'
assert bodies[0].count(needle)==1
bodies[0]=bodies[0].replace(needle,selector+needle,1)
a.out.mkdir(parents=True, exist_ok=True)
header = '\n'.join(f'RecompReturn {name}_M1X0(CpuState *cpu);' for name in names)
(a.out / 'death_flash_under_test.h').write_text(header + '\n' + '\n'.join(bodies))
binary = a.out / 'death-flash-oracle.exe'
runner = repo / 'snesrecomp/runner/src'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(repo / 'tests/lagoon_death_flash_oracle_test.c'),
                str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
result = subprocess.run([str(binary.resolve()), str(a.rom.resolve())],
                        capture_output=True, text=True)
print(result.stdout, end='')
if result.returncode:
    print(result.stderr, end='')
result.check_returncode()
(a.out / 'result.json').write_text(json.dumps({
    'status': 'PASS', 'generated_sources': sources,
    'test_only_entry_selector': selector, 'selector_sha256': hashlib.sha256(selector.encode()).hexdigest(),
    'instrumented_setup_body_sha256': hashlib.sha256(bodies[0].encode()).hexdigest(),
    'manifest_sha256': hashlib.sha256((a.gen / 'program_manifest.json').read_bytes()).hexdigest(),
    'rom_sha256': expected,
    'harness_sha256': hashlib.sha256((repo / 'tests/lagoon_death_flash_oracle_test.c').read_bytes()).hexdigest(),
    'stdout': result.stdout,
    'limits': 'Actual three generated bodies versus original opcodes across every instruction range. Test-only entry selector starts setup at9346/934C/9352 around two original sound calls; child sentinels STOP at real callee entry, not fake child returns. No sound results or guest waits manufactured. Declared CPU/stack states are isolated unit fixtures; real saved-byte ownership/six actual waits and matched live sound/timing paths are separate proofs.'
}, indent=2) + '\n')
