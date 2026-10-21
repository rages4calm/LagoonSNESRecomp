"""Compile the actual APU cadence, opcode commit and paired-return functions."""
import argparse
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--compiler', default='clang')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
runner = repo / 'snesrecomp/runner/src'
requests = [
    ('common_rtl.c', 'uint64_t rtl_apu_periodic_flush_threshold('),
    ('common_rtl.c', 'bool rtl_apu_absolute_periodic_enabled('),
    ('common_rtl.c', 'void rtl_apu_pace_check('),
    ('snes/interp_bridge.c', 'static uint64_t bridge_bounce_flush_thresh('),
    ('snes/interp_bridge.c', 'static int bridge_periodic_apu_flush_due('),
    ('snes/interp_bridge.c', 'static void bridge_apu_flush('),
    ('common_cpu_infra.c', 'void snes_refresh_charge('),
    ('cpu_state.c', 'void cpu_instruction_timing_commit('),
    ('cpu_state.c', 'RecompReturn cpu_dispatch_pc_paired('),
]
parts = []
for relative, signature in requests:
    source = (runner / relative).read_text(encoding='utf8')
    start = source.index(signature)
    end = re.search(r'^}', source[start:], re.M).end() + start
    parts.append((relative, signature, source[start:end]))
a.out.mkdir(parents=True, exist_ok=True)
(a.out / 'apu_cadence_under_test.h').write_text(
    '\n\n'.join(part[2] for part in parts) + '\n', encoding='utf8')
harness = repo / 'tests/lagoon_apu_cadence_guard_test.c'
binary = a.out / 'apu-cadence-guard.exe'
subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra',
                '-I' + str(runner), '-I' + str(a.out.resolve()),
                str(harness), '-o', str(binary)], check=True)
runs = []
for value in (None, '0', '1', '1024', '4096', '-7', '0x800'):
    for no_apu in (False, True):
        env = os.environ.copy()
        env.pop('SNESRECOMP_LLE_APU_FLUSH_THRESH', None)
        env.pop('SNESRECOMP_INTERP_NOAPU', None)
        if value is not None:
            env['SNESRECOMP_LLE_APU_FLUSH_THRESH'] = value
        if no_apu:
            env['SNESRECOMP_INTERP_NOAPU'] = '0'
        expected = 1024 if value is None else max(0, int(value, 0))
        r = subprocess.run([str(binary.resolve()), str(expected), str(int(no_apu))],
                           env=env, capture_output=True, text=True)
        print(r.stdout, end='')
        print(r.stderr, end='')
        runs.append(dict(threshold_env=value, no_apu_present=no_apu,
                         returncode=r.returncode, stdout=r.stdout, stderr=r.stderr))
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
result = dict(
    status='PASS' if all(r['returncode'] == 0 for r in runs) else 'FAIL',
    source_sha256={relative: sha(runner / relative) for relative, _, _ in parts},
    header_sha256=sha(runner / 'snes/interp_bridge.h'),
    function_sha256={signature: hashlib.sha256(source.encode()).hexdigest()
                     for _, signature, source in parts},
    harness_sha256=sha(harness), tool_sha256=sha(Path(__file__)), runs=runs,
    limits='Actual SDK functions with mocked device/lock/lookup plumbing. '
           'Includes actual DRAM-refresh accounting; periodic predicate, selector, '
           'threshold cache, commit ordering, consumed-return guards and mixed-tier '
           'completed-opcode partition invariance. No real SPC/PPU/device claim. '
           'Mandatory port/exit/IRQ flushes and event timestamps remain unchanged. '
           'Fresh same-host native/floor all12 runtime parity is required.')
(a.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf8')
if result['status'] != 'PASS':
    raise SystemExit(1)
