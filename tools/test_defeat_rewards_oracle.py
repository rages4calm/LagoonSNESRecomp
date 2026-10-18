"""Compare actual generated bounded reward code and children with ROM opcodes."""
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
names = ['Lagoon_Combat_GrantDefeatRewards', 'Lagoon_Combat_MarkDefeated',
         'Lagoon_Progress_AddGold', 'Lagoon_Progress_AwardExperience',
         'Lagoon_Progress_RecalculateStrength', 'Lagoon_Progress_RecalculateDefense']
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
assert 'L_B7F7_M1X0:' in bodies[0] and 'L_B7FE_M1X0:' not in bodies[0]
a.out.mkdir(parents=True, exist_ok=True)
header = '\n'.join(f'RecompReturn {name}_M1X0(CpuState *cpu);' for name in names)
(a.out / 'defeat_rewards_under_test.h').write_text(header + '\n' + '\n'.join(bodies))
binary = a.out / 'defeat-rewards-oracle.exe'
runner = repo / 'snesrecomp/runner/src'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(repo / 'tests/lagoon_defeat_rewards_oracle_test.c'),
                str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
result = subprocess.run([str(binary.resolve()), str(a.rom.resolve())],
                        capture_output=True, text=True)
print(result.stdout, end='')
if result.returncode:
    print(result.stderr, end='')
result.check_returncode()
(a.out / 'result.json').write_text(json.dumps({
    'status': 'PASS', 'generated_sources': sources,
    'manifest_sha256': hashlib.sha256((a.gen / 'program_manifest.json').read_bytes()).hexdigest(),
    'rom_sha256': expected,
    'harness_sha256': hashlib.sha256((repo / 'tests/lagoon_defeat_rewards_oracle_test.c').read_bytes()).hexdigest(),
    'stdout': result.stdout,
    'limits': 'Actual generated prefix and five actual generated arithmetic children versus ROM interpreter. Host scheduling is stubbed; boss refill/render paths remain outside both sides at B7FE/B819. Live affected-route qualification is separate.'
}, indent=2) + '\n')
