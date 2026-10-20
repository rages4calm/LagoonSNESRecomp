"""Compare actual generated boss animation code with original 65816 opcodes.

The original AE0C/B123/B016 children execute unchanged. Existing attack/damage
children stop at their real entry; this test never fabricates their return.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', type=Path, required=True)
p.add_argument('--gen', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--compiler', default='clang')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
expected = next(x.split('=', 1)[1].strip() for x in
                (repo/'rom_identity.txt').read_text().splitlines()
                if x.startswith('expected_sha256'))
assert hashlib.sha256(a.rom.read_bytes()).hexdigest() == expected
functions = {}
by_pc = {}
for file in a.gen.glob('bank*_v2.c'):
    source = file.read_text(encoding='utf-8')
    bank = int(re.search(r'bank([0-9a-fA-F]{2})', file.name)[1], 16)
    for m in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{', source):
        body = source[m.start():source.index('\n}', m.start())+2]+'\n'
        label = re.search(r'\bL_([0-9A-Fa-f]{4})_M[01]X[01]:', body)
        assert label, m[1]
        pc = (bank << 16) | int(label[1], 16)
        functions[m[1]] = (pc, body, file)
        if m[1].endswith('_M1X0'):
            by_pc[pc] = m[1]
starts = [0x01ada1, 0x01ae16, 0x01aeb9, 0x01b016,
          0x01b0d6, 0x01b10c, 0x01b123]
support = [0x01ae0c]
selected = {by_pc[pc] for pc in starts+support}
stubs = {}
bindings = {}
bodies = []
for pc in starts+support:
    name = by_pc[pc]
    _, body, file = functions[name]
    bindings[f'{pc:06X}'] = dict(name=name,
        body_sha256=hashlib.sha256(body.encode()).hexdigest(),
        source_sha256=hashlib.sha256(file.read_bytes()).hexdigest())
    for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)', body):
        assert child in functions, child
        if child not in selected:
            stubs[child] = functions[child][0]
    bodies.append(body)
header = '\n'.join('RecompReturn '+name+'(CpuState*c);' for name in sorted(selected))+'\n'
header += '\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}'
                    for name, pc in sorted(stubs.items()))+'\n'
header += '\n'.join(bodies)
header += '\n#define BOSS_TEST_COUNT '+str(len(starts))+'\n'
header += 'static const unsigned boss_starts[]={'+','.join(f'0x{pc&65535:x}' for pc in starts)+'};\n'
header += 'static RecompReturn boss_body(unsigned i,CpuState*c){switch(i){\n'
for i, pc in enumerate(starts):
    header += f'case {i}:return {by_pc[pc]}(c);\n'
header += 'default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True, exist_ok=True)
(a.out/'boss_animation_under_test.h').write_text(header, encoding='utf-8')
binary = a.out/'boss-animation-oracle.exe'
runner = repo/'snesrecomp/runner/src'
harness = repo/'tests/lagoon_boss_animation_oracle_test.c'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
    '-Wno-unused-label', '-Wno-unused-parameter', '-Wno-unused-variable',
    '-I'+str(runner), '-I'+str(a.out.resolve()), str(harness),
    str(runner/'snes/interp816.c'), '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())], capture_output=True, text=True)
print(r.stdout, end='')
print(r.stderr, end='')
r.check_returncode()
(a.out/'result.json').write_text(json.dumps(dict(status='PASS',
    generated_bodies=bindings, real_children=[f'{pc:06X}' for pc in support]+['01B016','01B123'],
    stopped_children=stubs, rom_sha256=expected,
    manifest_sha256=hashlib.sha256((a.gen/'program_manifest.json').read_bytes()).hexdigest(),
    harness_sha256=hashlib.sha256(harness.read_bytes()).hexdigest(),
    instrumented_header_sha256=hashlib.sha256(header.encode()).hexdigest(),
    stdout=r.stdout, limits='Original AE0C, B123 and B016 execute as actual generated code. '
    'Existing attack/damage bodies stop at the real original child entry without fabricated '
    'returns, actor creation, damage or gameplay outcomes. Full 128KiB RAM, CPU registers, '
    'flags, saved and dead stack bytes and CPU/master/coprocessor clocks are compared. '
    'Constant-eight-master-cycle bus fixture; actual MMIO/interrupt qualification is separate.'), indent=2)+'\n')
