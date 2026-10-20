"""Compare complete original Nasir death animation and balanced selector with ROM opcodes."""
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
names = ['Lagoon_ActorLleBoundary_a7d6', 'Lagoon_Player_DeathAnimation']
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
assert 'case 0xa7fa:' in bodies[0] and 'case 0xaac1:' in bodies[0] and 'case 0xaaf6:' in bodies[0]
assert 'L_AB1D_M1X0:' in bodies[1] and 'L_AAF6_M1X0:' not in bodies[1]
a.out.mkdir(parents=True, exist_ok=True)
header = '\n'.join(f'RecompReturn {name}_M1X0(CpuState *cpu);' for name in names)
(a.out / 'player_death_under_test.h').write_text(header + '\n' + '\n'.join(bodies))
binary = a.out / 'player-death-oracle.exe'
runner = repo / 'snesrecomp/runner/src'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(repo / 'tests/lagoon_player_death_oracle_test.c'),
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
    'harness_sha256': hashlib.sha256((repo / 'tests/lagoon_player_death_oracle_test.c').read_bytes()).hexdigest(),
    'stdout': result.stdout,
    'limits': 'Actual generated AAC1 and its original AB1D draw tail run completely versus original opcodes, including real RTS. Actual generated A7D6 selector runs all four original low states and unknown high-byte pointer fallbacks, invoking actual native AAC1 or stopping at the original interpreted boundary. No HP/outcome writes or fake child returns. Live route/scheduler proof is separate.'
}, indent=2) + '\n')
