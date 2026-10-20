"""Original shared OAM, queue and resource services versus emitted instructions.

The optional OAM probe selects the existing native8C24 body's actual8211 label;
it is a test-only source entry, never a production reentry authorization.
"""
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
p.add_argument('--probe-oam', action='store_true')
p.add_argument('--probe-caller', action='store_true', help='Explicit legacy coarse8C24 full-entry state/frame diagnostic; clock failure remains visible.')
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

roots = [0x8211] if a.probe_oam else [0x8C24] if a.probe_caller else [0x8C24, 0x8C93, 0x8211, 0x8C96, 0x89EC, 0x8AA4, 0x8DFE, 0x8D82, 0x8DCE, 0x8E2C, 0x8291, 0x8B68, 0x8D0B, 0x01C136, 0x01C156, 0x01C17B, 0x02B2C1]
# These source-closed children must execute, including genuine nested frames.
support = [0x8291] if a.probe_oam else [0x8211, 0x8291] if a.probe_caller else [0x8291, 0x8B68, 0x8D0B, 0x8C24]
selected = {by_pc[pc] for pc in roots + support if pc in by_pc}
if a.probe_oam and 0x8211 not in by_pc:
    selected.add(by_pc[0x8C24])
    assert 'L_8211_M1X0:' in functions[by_pc[0x8C24]][1]
else:
    manifest = json.loads((a.gen / 'program_manifest.json').read_text())
    for pc in roots:
        assert manifest['nodes'][f'{(0x8C24 if pc == 0x8C93 else pc):06X}:M1X0']['disposition'] == 'aot_eligible'

stubs = {}
bodies = []
bindings = {}
for name in sorted(selected):
    pc, body, file = functions[name]
    bindings[f'{pc:06X}'] = dict(name=name, body_sha256=hashlib.sha256(body.encode()).hexdigest(), source_sha256=sha(file))
    for child in re.findall(r'\b(\w+_M[01]X[01])\(cpu\)', body):
        assert child in functions, child
        if child not in selected:
            stubs[child] = functions[child][0]
    if a.probe_oam and 0x8211 not in by_pc and pc == 0x8C24:
        first = re.search(r'  L_[0-9A-F]+_M[01]X[01]:', body)
        body = body[:first.start()] + '  goto L_8211_M1X0;\n' + body[first.start():]
    if not a.probe_oam and pc == 0x8C24:
        first = re.search(r'  L_[0-9A-F]+_M[01]X[01]:', body)
        body = body[:first.start()] + '  if(service_entry==0x8C93)goto L_8C93_M1X0;\n' + body[first.start():]
    bodies.append(body)
header = '\n'.join('RecompReturn ' + n + '(CpuState*c);' for n in sorted(selected)) + '\n'
header += '\n'.join(f'RecompReturn {name}(CpuState*c){{(void)c;pending=1;boundary=0x{pc:06X};return (RecompReturn)1;}}' for name, pc in sorted(stubs.items())) + '\n'
header += '\n'.join(bodies)
header += '\nstatic const unsigned service_roots[]={' + ','.join(hex(pc) for pc in roots) + '};\n'
header += 'static RecompReturn service_body(unsigned pc,CpuState*c){switch(pc){\n'
for pc in roots:
    name = by_pc[0x8C24] if (a.probe_oam and 0x8211 not in by_pc and pc == 0x8211) or pc == 0x8C93 else by_pc[pc]
    header += f'case 0x{pc:04X}:return {name}(c);\n'
header += 'default:assert(0);return RECOMP_RETURN_NORMAL;}}\n'
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'shared_services_under_test.h').write_text(header, encoding='utf8')
runner = repo / 'snesrecomp/runner/src'
harness = repo / 'tests/lagoon_shared_services_oracle_test.c'
binary = a.out / 'shared-services-oracle.exe'
subprocess.run([a.compiler, '-DSERVICE_OAM_PROBE=' + str(int(a.probe_oam)), '-DSERVICE_LEGACY_CALLER=' + str(int(a.probe_caller)), '-std=c11', '-O2', '-Wall', '-Wextra', '-Wno-unused-label', '-Wno-unused-parameter', '-Wno-unused-variable', '-I' + str(runner), '-I' + str(a.out.resolve()), str(harness), str(runner / 'snes/interp816.c'), '-o', str(binary)], check=True)
r = subprocess.run([str(binary.resolve()), str(a.rom.resolve())], capture_output=True, text=True)
print(r.stdout, end='')
print(r.stderr, end='')
(a.out / 'result.json').write_text(json.dumps(dict(status='PASS' if r.returncode == 0 else 'FAIL', probe_oam=a.probe_oam, probe_caller=a.probe_caller, generated=bindings, stopped_children=stubs, rom_sha256=expected, manifest_sha256=sha(a.gen / 'program_manifest.json'), harness_sha256=sha(harness), tool_sha256=sha(Path(__file__)), stdout=r.stdout, stderr=r.stderr, limits='Actual source bodies, full128KiBRAM+IO/registers/flags/hiddenA/saved+deadstack/CPU/master/copro clocks. Existing8291/8B68/8D0B child bodies execute. Unknown children and the genuine8985 wait stop at actual entries; no fabricated return. Constant8master unitbus; resource wait fixtures either retain original polling until deadline or model an external23A-bit20 release on every fourth poll read, identically on both buses. This is not an executedNMI/device-latency claim. Root8C93 is a test-only actual-instruction entry into old8C24, preserving the real incoming owner; it grants no production reentry authority. Real-device scheduling is separate.'), indent=2) + '\n')
r.check_returncode()
