"""Compare actual generated item-consumption bodies with original ROM opcodes."""
import argparse
import hashlib
import json
from pathlib import Path
import re
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
wanted = {0x1CDB7: 'REVIVAL_CONSUME', 0x1CDE0: 'REVIVAL_RESTORE',
          0x1D8D4: 'REVIVAL_FINAL', 0x1D3CD: None}
found = {}
for path in a.gen.glob('bank01*_v2.c'):
    text = path.read_text(encoding='utf-8')
    for m in re.finditer(r'RecompReturn (\w+_M1X0)\(CpuState \*cpu\) \{', text):
        body = text[m.start():text.index('\n}', m.end()) + 2] + '\n'
        entry = re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)', body)
        pc = int(entry[1], 16) if entry else None
        if pc in wanted:
            assert pc not in found, hex(pc)
            found[pc] = (m[1], body, path)
assert set(found) == set(wanted), ('missing actual generated roots', set(wanted) - set(found))
a.out.mkdir(parents=True, exist_ok=True)
header = ''.join(f'RecompReturn {name}(CpuState *cpu);\n'
                 for name, body, path in found.values())
header += ''.join(f'#define {macro} {found[pc][0]}\n'
                  for pc, macro in wanted.items() if macro)
header += '\n'.join(found[pc][1] for pc in wanted)
(a.out / 'revival_under_test.h').write_text(header, encoding='utf-8')
binary = a.out / 'native-revival-oracle.exe'
runner = repo / 'snesrecomp/runner/src'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(repo / 'tests/lagoon_native_revival_oracle_test.c'),
                str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())], capture_output=True, text=True)
print(r.stdout, end='')
if r.returncode:
    print(r.stderr, end='')
r.check_returncode()
(a.out / 'result.json').write_text(json.dumps({
    'status': 'PASS', 'rom_sha256': expected, 'stdout': r.stdout,
    'manifest_sha256': hashlib.sha256((a.gen / 'program_manifest.json').read_bytes()).hexdigest(),
    'harness_sha256': hashlib.sha256((repo / 'tests/lagoon_native_revival_oracle_test.c').read_bytes()).hexdigest(),
    'generated': {f'{pc:06X}': {'name': name,
                  'body_sha256': hashlib.sha256(body.encode()).hexdigest(),
                  'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
                  for pc, (name, body, path) in found.items()}
}, indent=2) + '\n', encoding='utf-8')
