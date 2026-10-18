"""Compare an exact generated B413 body with original ROM opcodes, locally."""
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
name = 'Lagoon_ActorLleBoundary_b413_M1X0'
needle = f'RecompReturn {name}(CpuState *cpu) {{'
matches = [(f, f.read_text()) for f in a.gen.glob('bank01*_v2.c')
           if needle in f.read_text()]
assert len(matches) == 1, 'Expected exactly one actual generated B413 body'
source, text = matches[0]
begin = text.index(needle)
end = text.index('\n}', begin) + 2
body = text[begin:end] + '\n'
assert 'L_B413_M1X0:' in body and 'L_B43D_M1X0:' in body
assert 'L_B43E_M1X0:' not in body, 'Special path must remain outside body'
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'death_animation_under_test.h').write_text(body)
binary = a.out / 'death-animation-oracle.exe'
runner = repo / 'snesrecomp/runner/src'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(repo / 'tests/lagoon_death_animation_oracle_test.c'),
                str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
print('Exact generated source:', source, flush=True)
print('Extracted body SHA256:', hashlib.sha256(body.encode()).hexdigest(), flush=True)
result = subprocess.run([str(binary.resolve()), str(a.rom.resolve())],
                        check=True, capture_output=True, text=True)
print(result.stdout, end='')
(a.out / 'result.json').write_text(json.dumps({
    'status': 'PASS',
    'generated_source': str(source.resolve()),
    'generated_source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'generated_body_sha256': hashlib.sha256(body.encode()).hexdigest(),
    'manifest_sha256': hashlib.sha256((a.gen / 'program_manifest.json').read_bytes()).hexdigest(),
    'rom_sha256': expected,
    'harness_sha256': hashlib.sha256((repo / 'tests/lagoon_death_animation_oracle_test.c').read_bytes()).hexdigest(),
    'stdout': result.stdout,
    'limits': 'Scheduling/dispatch are stubbed at original boundaries; this is source arithmetic/stack/clock proof, not live interrupt/audio or complete gameplay qualification.'
}, indent=2) + '\n')
