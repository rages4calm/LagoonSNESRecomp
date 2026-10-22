"""Build the boss HUD's independent PPU oracle against private original dumps.

Requires the owner's legal USA ROM and a qualify_native checkpoint prefix.
No game data, saves, generated code or graphics are embedded in this test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

repo = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', required=True, type=Path)
p.add_argument('--fixture', required=True, type=Path)
p.add_argument('--out', required=True, type=Path)
p.add_argument('--cc', default=os.environ.get('CC', 'clang'))
p.add_argument('--refuse', action='store_true', help='Expect an unchanged unsupported boss phase')
a = p.parse_args()
out = a.out.resolve()
if out.exists():
    p.error('Use a fresh output directory')
rom, fixture = a.rom.resolve(), a.fixture.resolve()
suffixes = ('wram.bin', 'vram.bin', 'cgram.bin', 'oam.bin', 'fb.bgrx')
inputs = [rom] + [Path(str(fixture) + '.' + s) for s in suffixes]
if not all(f.is_file() for f in inputs):
    p.error('ROM or required checkpoint component is missing')
sources = [repo/'tests/lagoon_boss_hud_test.c', repo/'src/lagoon_boss_hud.c',
           repo/'src/lagoon_boss_hud.h',
           repo/'snesrecomp/tests/ppu/ppu_extra_object_test.c',
           repo/'snesrecomp/runner/src/snes/ppu.c',
           repo/'snesrecomp/runner/src/snes/ppu_legacy.c']
out.mkdir(parents=True)
exe = out/'boss-hud-test.exe'
command = [a.cc, '-std=c11', '-O2', '-I'+str(repo/'src'),
           '-I'+str(repo/'snesrecomp/runner/src'), '-ffunction-sections',
           '-fdata-sections', '-Wl,--gc-sections', str(sources[0]),
           str(sources[-2]), str(sources[-1]), '-o', str(exe)]
subprocess.run(command, check=True)
run = subprocess.run([str(exe), str(rom), str(fixture)] +
                     (['--refuse'] if a.refuse else []), capture_output=True, text=True)
print(run.stdout, end='')
if run.stderr:
    print(run.stderr, end='')
digest = lambda f: hashlib.sha256(f.read_bytes()).hexdigest()
receipt = dict(schema='lagoon boss HUD original PPU oracle v1',
               passed=run.returncode == 0, expected_refusal=a.refuse,
               input_sha256={str(f): digest(f) for f in inputs},
               source_sha256={str(f.relative_to(repo)): digest(f) for f in sources},
               compile_command=command, stdout=run.stdout, stderr=run.stderr,
               executable_sha256=digest(exe), exit_code=run.returncode)
(out/'result.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
raise SystemExit(run.returncode)
