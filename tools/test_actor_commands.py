"""Compare ordinary enemy host commands with the complete original opcodes.

This checks the original 0080B9 dispatcher plus bank02 direction/velocity
commands through the real final RTL. It does not validate live cadence or MMIO;
the host prefix deliberately consumes no guest clocks. Keep the ROM and this
tool's small executable/result in a fresh private scratch directory.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--compiler', default='clang')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
sdk = repo / 'snesrecomp/runner/src'
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
identity = next(line.split('=', 1)[1].strip() for line in
                (repo / 'rom_identity.txt').read_text(encoding='utf-8').splitlines()
                if line.startswith('expected_sha256'))
assert sha(a.rom) == identity, 'Wrong ROM identity'
assert not a.out.exists(), 'Choose a fresh private output directory'
a.out.mkdir(parents=True)
sources = [repo / 'tests/lagoon_native_actor_commands_test.c',
           repo / 'src/lagoon_native_actor_commands.c', sdk / 'snes/interp816.c']
exe = a.out / 'actor-commands-test.exe'
command = [a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
           '-DLAGOON_NATIVE_ACTOR_COMMANDS_TEST', '-I' + str(repo / 'src'),
           '-I' + str(sdk)] + [str(path) for path in sources] + ['-o', str(exe)]
subprocess.run(command, check=True)
result = subprocess.run([str(exe.resolve()), str(a.rom.resolve())],
                        text=True, capture_output=True)
print(result.stdout, end='')
print(result.stderr, end='')
record = dict(schema='Lagoon ordinary enemy commands source oracle v1',
              passed=result.returncode == 0, rom_sha256=identity,
              source_sha256={str(path.relative_to(repo)): sha(path) for path in
                             sources + [repo / 'src/lagoon_native_actor_commands.h',
                                        Path(__file__)]},
              exit=result.returncode, stdout=result.stdout, stderr=result.stderr,
              scope='Exhaustive 256x256 direction flags/intents and velocity '
                    'tiers/directions; full128KiB RAM, registers, flags, '
                    'openbus and dead physical stack. 64 realJSLframes plus '
                    'readonly mode/source/context refusals. Original opcode '
                    'oracle stops before actual80E5RTL and then executes that '
                    'RTL with untouched physicalcallerframe. Native math '
                    'preserves entry clocks intentionally; original opcode '
                    'clocks measured separately. Live normal-world guards, '
                    'cadence, NMI/audio, rendering and pacing require paired '
                    'runtime controls; this is not cycle-equivalent execution.')
(a.out / 'result.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
assert record['passed'], 'Actor command source oracle failed'
