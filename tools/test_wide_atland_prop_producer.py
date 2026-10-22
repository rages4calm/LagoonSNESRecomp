"""Original Atland static-prop draw producer and finite phase-policy proof.

Requires the owner's original ROM and an externally captured exact Atland
checkpoint prefix. No ROM, graphics, state or generated sources are embedded.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom', type=Path, required=True)
p.add_argument('--fixture-prefix', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--compiler', default='clang')
p.add_argument('--sdl-include', type=Path, required=True)
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
assert sha(a.rom) == '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
a.out.mkdir(parents=True, exist_ok=True)
files = ['tools/test_wide_atland_prop_producer.c', 'src/lagoon_wide_multipart.c',
         'src/lagoon_wide_spell3.c', 'src/lagoon_scene_renderer.c',
         'snesrecomp/runner/src/snes/ppu.c',
         'snesrecomp/runner/src/snes/ppu_legacy.c',
         'snesrecomp/runner/src/snes/interp816.c']
flags = ['-std=c11', '-O2', '-DSDL_MAIN_HANDLED=1', '-DSNESRECOMP_SDL3=1',
         '-DSNESRECOMP_REVERSE_DEBUG=0', '-I'+str(repo/'src'),
         '-I'+str(repo/'snesrecomp/runner/src'),
         '-I'+str(repo/'snesrecomp/runner/src/snes'),
         '-I'+str(repo/'snesrecomp/runner/src/desktop'),
         '-I'+str(a.sdl_include), '-ffunction-sections', '-fdata-sections',
         '-Wl,--gc-sections']
exe = a.out/'atland-prop-producer-test.exe'
subprocess.run([a.compiler, *flags, *[str(repo/f) for f in files], '-o', str(exe)], check=True)
r = subprocess.run([str(exe.resolve()), str(a.rom.resolve()), str(a.fixture_prefix.resolve())],
                   capture_output=True, text=True)
print(r.stdout, end='')
print(r.stderr, end='')
r.check_returncode()
source_files = files + ['src/lagoon_wide_entities.c', 'src/lagoon_wide_entities.h',
                       'tools/test_wide_atland_prop_producer.py',
                       'tools/test_wide_multipart_producer.c',
                       'tools/test_wide_projectile_producer.c',
                       'tools/test_wide_additional_producer.c',
                       'snesrecomp/tests/ppu/ppu_extra_object_test.c']
receipt = dict(passed=True, scope=__doc__, output=r.stdout,
               rom_sha256=sha(a.rom),
               source_hashes={f: sha(repo/f) for f in source_files},
               external_fixture_hashes={suffix: sha(Path(str(a.fixture_prefix)+'.'+suffix))
                                        for suffix in ['wram.bin', 'vram.bin', 'cgram.bin']},
               limits='Actual original opcode/projection/draw source proof and modeled DMA. '
                      'The disclosed CPU-camera phase arrangements are finite policy cases; '
                      'continuous live walking and rendered pixels are qualified separately.')
(a.out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
