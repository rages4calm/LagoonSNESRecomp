"""Original BE79 six-effect pointer tail and actual first-free allocator closure."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser(description=__doc__)
for key in ('rom', 'gen', 'out'):
    p.add_argument('--' + key, type=Path, required=True)
p.add_argument('--compiler', default='clang')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
identity = next(s.split('=', 1)[1].strip() for s in
                (repo / 'rom_identity.txt').read_text().splitlines()
                if s.startswith('expected_sha256'))
assert sha(a.rom) == identity
functions = {}
by_pc = {}
for file in a.gen.glob('bank*_v2.c'):
    text = file.read_text(encoding='utf8')
    for match in re.finditer(r'RecompReturn (\w+_M[01]X[01])\(CpuState \*cpu\) \{', text):
        body = text[match.start():text.index('\n}', match.end()) + 2] + '\n'
        entry = re.search(r'cpu_trace_func_entry\(cpu, 0x([0-9A-Fa-f]+)', body)
        assert entry
        functions[match[1]] = (int(entry[1], 16), body, file)
        if match[1].endswith('_M1X0'):
            by_pc[int(entry[1], 16)] = match[1]
roots = [0x01BE79, 0x01BED1, 0x01BF1F, 0x01BF20, 0x01BF83, 0x01BF9C, 0x01BFF5]
allocator = 0x019A70
manifest = json.loads((a.gen / 'program_manifest.json').read_text())
for pc in roots + [allocator]:
    assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition'] == 'aot_eligible'
refused = {}
fixed_refusals = ['01BB05:M0X0', '01BB05:M0X1', '01BB05:M1X1', '01BB4C:M1X1', '01BD5B:M0X0',
                  '018385:M1X1', '01BCCA:M1X1', '01BE52:M0X0',
                  '01BE52:M0X1', '01BE52:M1X1']
for key in fixed_refusals:
    disposition = manifest['nodes'].get(key, {}).get('disposition', 'not_generated')
    assert disposition != 'aot_eligible', key
    refused[key] = disposition
for pc in roots:
    for mode in ('M0X0', 'M0X1', 'M1X1'):
        key = f'{pc:06X}:{mode}'
        disposition = manifest['nodes'].get(key, {}).get('disposition', 'not_generated')
        assert disposition != 'aot_eligible', key
        refused[key] = disposition
selected = {by_pc[pc] for pc in roots + [allocator]}
pending = list(selected)
while pending:
    for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)', functions[pending.pop()][1]):
        assert child in functions, child
        assert child.endswith('_M1X0'), child
        if child not in selected:
            selected.add(child)
            pending.append(child)
bindings = {f'{functions[name][0]:06X}': dict(name=name,
    body_sha256=hashlib.sha256(functions[name][1].encode()).hexdigest(),
    source_sha256=sha(functions[name][2])) for name in sorted(selected)}
header = '\n'.join('RecompReturn ' + n + '(CpuState*c);' for n in sorted(selected)) + '\n'
header += '\n'.join(functions[name][1] for name in sorted(selected))
header += '\nstatic RecompReturn effect_body(unsigned pc,CpuState*c){switch(pc){\n'
for pc in roots + [allocator]:
    header += f'case 0x{pc:06X}:return {by_pc[pc]}(c);\n'
header += 'default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
rom = a.rom.read_bytes()
slots = [int.from_bytes(rom[0xBE8D+i*2:0xBE8F+i*2], 'little') | 0x010000 for i in range(34)]
assert set(slots) == set(roots) - {0x01BE79}
header += 'static const unsigned effect_targets[]={' + ','.join(hex(pc) for pc in slots) + '};\n'
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'projectile_effect_services_under_test.h').write_text(header, encoding='utf8')
runner = repo / 'snesrecomp/runner/src'
harness = repo / 'tests/lagoon_projectile_effect_services_oracle_test.c'
binary = a.out / 'projectile-effect-oracle.exe'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Wno-unused-label',
    '-Wno-unused-parameter', '-Wno-unused-variable', '-I'+str(runner), '-I'+str(a.out.resolve()),
    str(harness), str(runner/'snes/interp816.c'), '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())], capture_output=True, text=True)
print(r.stdout, end='')
print(r.stderr, end='')
result = dict(schema='Lagoon complete projectile effect source oracle v1',
    status='PASS' if r.returncode == 0 else 'FAIL', generated=bindings, stopped_children={},
    refused_variants=refused, rom_sha256=identity, manifest_sha256=sha(a.gen/'program_manifest.json'),
    harness_sha256=sha(harness), tool_sha256=sha(Path(__file__)), stdout=r.stdout, stderr=r.stderr,
    limits='Actual original BE79 pointer/Y/PHY/PLY plus six complete effects and real9A70 allocation; '
    'no child return/result stubs. FullRAM+IO/register/flags/physicalRTS and CPU/master/copro clocks '
    'match original opcodes under constant8master unitbus. All34slots/256byte selectors/aliases, '
    'full/partial pool and spawned slot contents, actor-intent alias, target velocities/sinusoid/gravity, '
    'hiddenA, flags and deadlines. Unknown pointer stops at exact original JMP target; unknown modes '
    'are separately asserted absent/floored in manifest. Same-BB05-pass processing of new higher '
    'slots and actual MMIO/runtime timing belong to the separate whole-loop/live qualification.')
(a.out/'result.json').write_text(json.dumps(result, indent=2)+'\n')
r.check_returncode()
