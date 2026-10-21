"""Complete original display-transition services versus original opcodes."""
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
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
expected = next(s.split('=', 1)[1].strip() for s in
                (repo / 'rom_identity.txt').read_text().splitlines()
                if s.startswith('expected_sha256'))
assert sha(a.rom) == expected
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

roots = [0x0083F0, 0x008438, 0x008481, 0x0084CE, 0x00905F, 0x009068, 0x009071]
support = [0x008112]
manifest = json.loads((a.gen / 'program_manifest.json').read_text())
for pc in roots:
    assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition'] == 'aot_eligible'
    for mode in ('M0X0', 'M0X1', 'M1X1'):
        assert manifest['nodes'].get(f'{pc:06X}:{mode}', {}).get('disposition') != 'aot_eligible'
selected = {by_pc[pc] for pc in roots + support}
stubs = {}
bindings = {}
bodies = []
for name in sorted(selected):
    pc, body, file = functions[name]
    bindings[f'{pc:06X}'] = dict(name=name, body_sha256=hashlib.sha256(body.encode()).hexdigest(), source_sha256=sha(file))
    for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)', body):
        assert child in functions, child
        if child not in selected:
            stubs[child] = functions[child][0]
    bodies.append(body)
assert set(stubs.values()) == {0x0084DE, 0x0084E0}, stubs
header = '\n'.join('RecompReturn ' + n + '(CpuState*c);' for n in sorted(selected)) + '\n'
header += '\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}}' for name, pc in sorted(stubs.items())) + '\n'
header += '\n'.join(bodies)
header += '\nstatic const unsigned display_roots[]={' + ','.join(hex(pc) for pc in roots) + '};\n'
header += 'static RecompReturn display_body(unsigned pc,CpuState*c){switch(pc){\n'
for pc in roots:
    header += f'case 0x{pc:06X}:return {by_pc[pc]}(c);\n'
header += 'default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'display_services_under_test.h').write_text(header, encoding='utf8')
runner = repo / 'snesrecomp/runner/src'
harness = repo / 'tests/lagoon_display_services_oracle_test.c'
binary = a.out / 'display-services-oracle.exe'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Wno-unused-label', '-Wno-unused-parameter', '-Wno-unused-variable', '-I' + str(runner), '-I' + str(a.out.resolve()), str(harness), str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())], capture_output=True, text=True)
print(r.stdout, end='')
print(r.stderr, end='')
(a.out / 'result.json').write_text(json.dumps(dict(status='PASS' if r.returncode == 0 else 'FAIL', generated=bindings, stopped_children=stubs, rom_sha256=expected, manifest_sha256=sha(a.gen / 'program_manifest.json'), harness_sha256=sha(harness), tool_sha256=sha(Path(__file__)), stdout=r.stdout, stderr=r.stderr, limits='Seven complete display bodies and the existing8112 source child run original operations through normal returns or genuine84DE/84E0 wait/deadline entry, retaining actual three-byte JSL frames. No NMI completion, child outcome or return is fabricated. Full128KiBRAM+IO/registers/flags/Ahigh/saved-deadstack and CPU/master/coprocessor clocks match on constant8master unitbus; real hardware/NMI/IRQ/scheduling and live transitions remain separate.'), indent=2) + '\n')
r.check_returncode()
