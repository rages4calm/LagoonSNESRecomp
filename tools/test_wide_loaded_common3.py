"""Qualify original room09 common2-then-common3 residency and actual NPC pixels.

The ROM and snapshot bytes remain external private inputs. Source predicates and
decompressors run as original instructions. Optional producer independently runs
C1C6 and C026, then models completed OAM DMA; live runtime remains separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('rom', 'snapshot-prefix', 'compiler', 'out'):
        parser.add_argument('--' + key, type=Path, required=True)
    parser.add_argument('--producer-prefix', type=Path)
    parser.add_argument('--sdl-include', type=Path)
    args = parser.parse_args()
    if bool(args.producer_prefix) != bool(args.sdl_include):
        parser.error('producer validation requires both --producer-prefix and --sdl-include')
    repo = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        parser.error('--out must be empty')
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    compiler, rom = args.compiler.resolve(strict=True), args.rom.resolve(strict=True)
    source_files = ['tools/test_wide_loaded_common3_source.c', 'tools/test_wide_loaded_common3_producer.c',
                    'tools/test_wide_loaded_common3.py', 'tools/test_wide_parallax_sources.c',
                    'tools/test_wide_additional_producer.c', 'src/lagoon_wide_entities.c',
                    'src/lagoon_wide_entities.h', 'src/lagoon_wide_multipart.c',
                    'src/lagoon_wide_spell3.c', 'src/lagoon_scene_renderer.c',
                    'snesrecomp/runner/src/snes/interp816.c', 'snesrecomp/runner/src/snes/ppu.c',
                    'snesrecomp/runner/src/snes/ppu_legacy.c']

    def run(command, tag):
        result = subprocess.run(list(map(str, command)), cwd=repo, text=True, capture_output=True)
        (out / (tag + '.txt')).write_text(result.stdout + result.stderr, encoding='utf-8')
        result.check_returncode()
        return result.stdout.strip()

    source_exe = out / 'common3-source.exe'
    run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
         '-DLAGOON_WIDE_ENTITIES_DECODER_ONLY', '-Isrc', '-Isnesrecomp/runner/src',
         'tools/test_wide_loaded_common3_source.c', 'src/lagoon_wide_entities.c',
         'snesrecomp/runner/src/snes/interp816.c', '-o', source_exe], 'source-build')
    source_result = run([source_exe, rom, args.snapshot_prefix.resolve()], 'source')
    snapshot = lambda prefix: {s: digest(Path(str(prefix.resolve()) + '.' + s))
                               for s in ('wram.bin', 'vram.bin', 'cgram.bin')}
    receipt = dict(passed=True, rom_sha256=digest(rom), sources_sha256={p: digest(repo / p) for p in source_files},
                   source=dict(exe_sha256=digest(source_exe), output=source_result,
                               snapshot_sha256=snapshot(args.snapshot_prefix)),
                   scope='Source-qualified original common2-then-common3 family only; no campaign or live-frame timing assertion')
    if args.producer_prefix:
        producer = out / 'common3-producer.exe'
        run([compiler, '-std=c11', '-O2', '-DSDL_MAIN_HANDLED=1', '-DSNESRECOMP_SDL3=1',
             '-DSNESRECOMP_REVERSE_DEBUG=0', '-Isrc', '-Isnesrecomp/runner/src',
             '-Isnesrecomp/runner/src/snes', '-Isnesrecomp/runner/src/desktop',
             '-I' + str(args.sdl_include.resolve(strict=True)), '-ffunction-sections',
             '-fdata-sections', '-Wl,--gc-sections', 'tools/test_wide_loaded_common3_producer.c',
             'src/lagoon_wide_multipart.c', 'src/lagoon_wide_spell3.c', 'src/lagoon_scene_renderer.c',
             'snesrecomp/runner/src/snes/ppu.c', 'snesrecomp/runner/src/snes/ppu_legacy.c',
             'snesrecomp/runner/src/snes/interp816.c', '-o', producer], 'producer-build')
        result = run([producer, rom, args.producer_prefix.resolve()], 'producer')
        receipt['producer'] = dict(exe_sha256=digest(producer), output=result,
                                   snapshot_sha256=snapshot(args.producer_prefix),
                                   scope='Existing NPC47 with disclosed position-only assist; original C1C6 source/OAM cells and C026 queued groups, modeled DMA, independent scalar margin pixels; not a natural escort test')
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
