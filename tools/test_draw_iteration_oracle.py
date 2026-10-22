"""Compare the generated actor sort/iteration loop with original CPU opcodes.

Dynamic drawing children run their actual opcodes on both sides. This isolates
the newly compiled loop; live replay checks cover the production bridge/MMIO.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
for key in ('rom', 'gen', 'out'):
    parser.add_argument('--' + key, type=Path, required=True)
parser.add_argument('--compiler', default='clang')
args = parser.parse_args()
repo = Path(__file__).resolve().parents[1]
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
expected = next(line.split('=', 1)[1].strip() for line in
                (repo / 'rom_identity.txt').read_text().splitlines()
                if line.startswith('expected_sha256'))
assert sha(args.rom) == expected and not args.out.exists()
manifest = json.loads((args.gen / 'program_manifest.json').read_text())
assert manifest['nodes']['01C026:M1X0']['disposition'] == 'aot_eligible'
for mode in ('M0X0', 'M0X1', 'M1X1'):
    assert manifest['nodes'].get('01C026:' + mode, {}).get('disposition') != 'aot_eligible'
for mode in ('M0X0', 'M0X1', 'M1X0', 'M1X1'):
    assert len(manifest['exit_mode_sets']['01C0AA:' + mode]) == 4

bodies = []
for file in args.gen.glob('bank*_v2.c'):
    text = file.read_text()
    for match in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{', text):
        body = text[match.start():text.index('\n}', match.end()) + 2]
        if 'cpu_trace_func_entry(cpu, 0x01C026,' in body and match[1].endswith('_M1X0'):
            bodies.append((match[1], body))
assert len(bodies) == 1
name, body = bodies[0]
children = set(re.findall(r'\b(\w+_M[01]X[01])\(cpu\)', body))
assert len(children) == 4 and all('C0AA' in child for child in children)
header = '\n'.join('RecompReturn ' + child + '(CpuState *cpu) { return original_child(cpu); }'
                   for child in sorted(children))
header += '\n' + body + '\n#define DRAW_ITERATION ' + name + '\n'
args.out.mkdir(parents=True)
(args.out / 'draw_iteration_under_test.h').write_text(header)
sdk = repo / 'snesrecomp/runner/src'
test = repo / 'tests/lagoon_draw_iteration_oracle_test.c'
binary = args.out / 'draw-iteration.exe'
subprocess.run([args.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter', '-Wno-unused-variable',
                '-I' + str(sdk), '-I' + str(args.out.resolve()), str(test),
                str(sdk / 'snes/interp816.c'), '-o', str(binary)], check=True)
run = subprocess.run([str(binary.resolve()), str(args.rom.resolve())], capture_output=True, text=True)
print(run.stdout, end='')
print(run.stderr, end='')
record = dict(passed=run.returncode == 0, rom_sha256=expected,
              manifest_sha256=sha(args.gen / 'program_manifest.json'),
              generated_body_sha256=hashlib.sha256(body.encode()).hexdigest(),
              sources={str(path.relative_to(repo)): sha(path) for path in
                       (Path(__file__), test, sdk / 'snes/interp816.c')},
              stdout=run.stdout, stderr=run.stderr, returncode=run.returncode,
              scope='Actual generated C026 sort and complete iteration; actual original C0AA dynamic children, no child outcome injection. Physical original 0180F1 JSR frame, complete WRAM/IO/registers/flags and CPU/master/coprocessor clocks on a constant eight-master byte bus. Unit arithmetic models immediate multiply/divide results; production latency, bridge ownership, APU and presentation require live matched controls.')
(args.out / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
run.check_returncode()
