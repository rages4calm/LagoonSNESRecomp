"""Compile the actual readonly revival policy and its focused source/frame guards."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', required=True, type=Path)
p.add_argument('--out', required=True, type=Path)
p.add_argument('--compiler', default='clang')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
expected = next(x.split('=', 1)[1].strip() for x in
                (repo / 'rom_identity.txt').read_text().splitlines()
                if x.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest() == expected
a.out.mkdir(parents=True, exist_ok=True)
binary = a.out / 'native-revival-guards.exe'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                '-I' + str(repo / 'snesrecomp/runner/src'),
                str(repo / 'tests/lagoon_native_revival_guard_test.c'),
                '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())],
                   capture_output=True, text=True)
print(r.stdout, end='')
if r.returncode:
    print(r.stderr, end='')
r.check_returncode()
(a.out / 'result.json').write_text(json.dumps({
    'status': 'PASS', 'rom_sha256': expected, 'stdout': r.stdout,
    'sources': {name: hashlib.sha256((repo / name).read_bytes()).hexdigest()
                for name in ['src/lagoon_native_revival.c', 'src/lagoon_native_revival.h',
                             'tests/lagoon_native_revival_guard_test.c']}
}, indent=2) + '\n', encoding='utf-8')
