"""Finite original34-slot projectile animation table: complete leaf handlers and shared RTS tail versus original opcodes."""
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

roots = [0x01BBA3,0x01BBAD,0x01BBCE,0x01BBE6,0x01BBEC,0x01BC0D,0x01BC17,0x01BC23,0x01BC4C,0x01BC59,0x01BC5E,0x01BC63,0x01BC68,0x01BC6D,0x01BC79,0x01BC85,0x01BC9B,0x01BCA0,0x01BCA5,0x01BCAA,0x01BCAF]
support = [0x01BCB2, 0x01BB4B]
manifest = json.loads((a.gen / 'program_manifest.json').read_text())
refused_variants = ['01BB05:M1X0', '01BB4B:M0X0', '01BB4B:M0X1', '01BB4B:M1X1',
                    '018385:M1X1', '01BCCA:M1X1', '01BE52:M0X0',
                    '01BE52:M0X1', '01BE52:M1X1', '01BE79:M1X1']
for key in refused_variants:
    assert manifest['nodes'].get(key, {}).get('disposition') != 'aot_eligible', key
for pc in roots + support:
    assert manifest['nodes'][f'{pc:06X}:M1X0']['disposition'] == 'aot_eligible'
    if pc in roots or pc == 0x01BB4B:
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
assert not stubs, stubs
header = '\n'.join('RecompReturn ' + n + '(CpuState*c);' for n in sorted(selected)) + '\n'
header += '\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}}' for name, pc in sorted(stubs.items())) + '\n'
header += '\n'.join(bodies)
header += '\nstatic const unsigned animation_handlers_roots[]={' + ','.join(hex(pc) for pc in roots) + '};\n'
header += 'static RecompReturn animation_handlers_body(unsigned pc,CpuState*c){switch(pc){\n'
for pc in roots + support:
    header += f'case 0x{pc:06X}:return {by_pc[pc]}(c);\n'
header += 'default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
rom = a.rom.read_bytes()
slots = [int.from_bytes(rom[0xBB5F+i*2:0xBB61+i*2], 'little') | 0x010000 for i in range(34)]
assert set(slots) == set(roots)
header += 'static const unsigned animation_table_targets[]={' + ','.join(hex(pc) for pc in slots) + '};\n'
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'animation_handlers_services_under_test.h').write_text(header, encoding='utf8')
runner = repo / 'snesrecomp/runner/src'
harness = repo / 'tests/lagoon_animation_handlers_services_oracle_test.c'
binary = a.out / 'animation_handlers-services-oracle.exe'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Wno-unused-label', '-Wno-unused-parameter', '-Wno-unused-variable', '-I' + str(runner), '-I' + str(a.out.resolve()), str(harness), str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())], capture_output=True, text=True)
print(r.stdout, end='')
print(r.stderr, end='')
(a.out / 'result.json').write_text(json.dumps(dict(status='PASS' if r.returncode == 0 else 'FAIL', generated=bindings, stopped_children=stubs, refused_variants={key:manifest['nodes'].get(key, {}).get('disposition', 'not_generated') for key in refused_variants}, rom_sha256=expected, manifest_sha256=sha(a.gen / 'program_manifest.json'), harness_sha256=sha(harness), tool_sha256=sha(Path(__file__)), stdout=r.stdout, stderr=r.stderr, limits='All34 original table slots and21 complete M1X0 handlers plus original sharedBCB2 RTS tail; no child results or returns stubbed. Original table/data bytes unchanged, 256frame values, binaryflags/hiddenA/2stacks/physical inherited return and entry/internal deadline cases; full128KiBRAM+IO/registers/CPU-master-copro clocks match under constant8master unitbus. Full original BB4B computes its pointer/PHY/PLY; finite known pointers enter exact M1X0 handlers with inherited frame. All256 byte selectors including128..161 aliases and unknown targets run to their real original JMP boundary; unknown modes stay on floor. BB05 remains explicitly interpreted; no global return-mode assertion. Actual hardware, casting, IRQ and live handler reach are separate route evidence.'), indent=2) + '\n')
r.check_returncode()
