"""Compare complete original Nasir death animation and balanced selector with ROM opcodes."""
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
p.add_argument('--compile-only', action='store_true',
               help='Verify actual-body extraction and compilation without replaying opcode cases')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
expected = next(x.split('=', 1)[1].strip() for x in
                (repo / 'rom_identity.txt').read_text().splitlines()
                if x.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest() == expected
names = ['Lagoon_ActorLleBoundary_a7d6', 'Lagoon_Player_DeathAnimation']
texts = [(f, f.read_text(encoding='utf-8')) for f in a.gen.glob('bank01*_v2.c')]
bodies, sources = [], {}
actual_names = []
for name, pc in zip(names, [0x01A7D6, 0x01AAC1]):
    matches = []
    for source, text in texts:
        for m in re.finditer(r'RecompReturn (\w+_M1X0)\(CpuState \*cpu\) \{', text):
            body = text[m.start():text.index('\n}', m.end()) + 2] + '\n'
            entry = re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9a-fA-F]+)', body)
            if entry and int(entry[1], 16) == pc:
                matches.append((source, body, m[1]))
    assert len(matches) == 1, (name, hex(pc))
    source, body, actual_name = matches[0]
    actual_names.append(actual_name)
    bodies.append(body)
    sources[name] = {'actual_name': actual_name, 'pc24': f'{pc:06X}',
                     'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                     'body_sha256': hashlib.sha256(body.encode()).hexdigest()}
assert 'case 0xa7fa:' in bodies[0] and 'case 0xaac1:' in bodies[0] and 'case 0xaaf6:' in bodies[0]
assert 'L_AB1D_M1X0:' in bodies[1] and 'L_AAF6_M1X0:' not in bodies[1]
a.out.mkdir(parents=True, exist_ok=True)
header = '\n'.join(f'#define {name}_M1X0 {actual}'
                   for name, actual in zip(names, actual_names) if name + '_M1X0' != actual)
header += '\n' + '\n'.join(f'RecompReturn {name}(CpuState *cpu);' for name in actual_names)
(a.out / 'player_death_under_test.h').write_text(header + '\n' + '\n'.join(bodies), encoding='utf-8')
binary = a.out / 'player-death-oracle.exe'
runner = repo / 'snesrecomp/runner/src'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Wno-unused-label', '-Wno-unused-parameter',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(repo / 'tests/lagoon_player_death_oracle_test.c'),
                str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
if a.compile_only:
    (a.out / 'compile-only.json').write_text(json.dumps({
        'status': 'EXTRACT_AND_COMPILE_ONLY', 'generated_sources': sources,
        'manifest_sha256': hashlib.sha256((a.gen / 'program_manifest.json').read_bytes()).hexdigest(),
        'opcode_cases_executed': 0
    }, indent=2) + '\n', encoding='utf-8')
    print('PASS actual A7D6/AAC1 trace-PC extraction and compilation; no opcode cases replayed')
    raise SystemExit(0)
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
