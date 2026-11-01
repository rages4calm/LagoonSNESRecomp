#!/usr/bin/env python3
"""Build Lagoon on the current Linux/macOS host without touching build/.

Generated game sources stay local. --setup-host is compile qualification only,
and produces metadata with playable=false so the packager will refuse it.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import platform
import subprocess

ROOT = Path(__file__).resolve().parent.parent


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, help='Your clean Lagoon USA ROM; never copied or uploaded')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--setup-host', action='store_true', help='ROM-free compile check; cannot play')
    parser.add_argument('--reuse-generated', action='store_true', help='Reuse the existing legacy generated C; native profiles are still required')
    parser.add_argument('--native-generated-dir', type=Path, help='Private reviewed native profile directory; no ROM is needed to compile already-generated inputs')
    parser.add_argument('--jobs', type=int, default=min(os.cpu_count() or 2, 4))
    parser.add_argument('--cmake-arg', action='append', default=[])
    args = parser.parse_args()
    if platform.system() not in ('Linux', 'Darwin'):
        parser.error('Run on Linux or macOS; this is not a cross-compiler or Windows build wrapper')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if args.setup_host and (args.rom or args.reuse_generated or args.native_generated_dir):
        parser.error('A ROM-free setup-host check cannot request game generation')
    build = (args.build_dir or ROOT / ('build-port-setup' if args.setup_host else 'build-port')).absolute()
    if build == ROOT or build == ROOT / 'build' or build.is_symlink():
        parser.error('Choose a separate build directory; the working game is protected')
    subprocess.run(['python3', '-P', str(ROOT / 'tools/apply_framework_patch.py')], cwd=ROOT, check=True)
    generated = ROOT / 'src/gen'
    if args.setup_host:
        generated = build / 'no-generated-code'
    elif args.reuse_generated:
        if not (generated / 'program_manifest.json').is_file() or not any(generated.glob('*.c')):
            parser.error('No complete locally generated program exists')
    else:
        command = ['bash', str(ROOT / 'tools/regen.sh')]
        if args.rom:
            command += ['--rom', str(args.rom.expanduser().absolute())]
        subprocess.run(command, cwd=ROOT, check=True)
    native_args = []
    if not args.setup_host:
        if args.native_generated_dir:
            native = args.native_generated_dir.expanduser().absolute()
            sources = [native / name for name in ('ordinary69.generated.c', 'camera69.generated.c')]
            if not all(p.is_file() for p in sources):
                parser.error('Both reviewed private native profile sources are required')
            native_args = ['-DLAGOON_NATIVE_BLOCKS69_SOURCE=' + str(sources[0]),
                           '-DLAGOON_NATIVE_BLOCKS69_CAMERA_SOURCE=' + str(sources[1])]
        else:
            rom = args.rom.expanduser().absolute() if args.rom else None
            if rom is None and (ROOT / 'rom.cfg').is_file():
                rom = Path((ROOT / 'rom.cfg').read_text(encoding='utf-8').strip()).expanduser().absolute()
            if rom is None or not rom.is_file():
                parser.error('Native profiles require --rom (or a valid rom.cfg), or --native-generated-dir for reviewed frozen inputs')
            native_args = ['-DLAGOON_GENERATE_NATIVE69_FROM_ROM=ON', '-DLAGOON_NATIVE69_ROM=' + str(rom)]
        native_args += ['-DLAGOON_NATIVE_RELEASE_DEFAULT=ON', '-DLAGOON_REQUIRE_NATIVE_CPU=ON']
    command = ['cmake', '-S', str(ROOT), '-B', str(build), '-G', 'Ninja',
               '-DCMAKE_BUILD_TYPE=Release', '-DLAGOON_ENABLE_LEGACY_LAUNCHER=OFF',
               '-DLAGOON_PORTABLE_STATIC_SDL3=ON',
               '-DSNESRECOMP_FRAME_IMPL=HLE', '-DSNESRECOMP_EXECUTION_DEFAULT=off',
               '-DSNESRECOMP_SETUP_HOST=' + ('ON' if args.setup_host else 'OFF'),
               '-DLAGOON_GENERATED_DIR=' + str(generated), *native_args, *args.cmake_arg]
    if platform.system() == 'Darwin':
        command += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0']
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run(['cmake', '--build', str(build), '--target', 'LagoonSNESRecomp', '-j', str(args.jobs)], check=True)
    print('Compile-only setup host built; not playable.' if args.setup_host else
          'Strict-native CPU game built. Test on this OS with your ROM before packaging; hardware graphics/audio runtime remains.')


if __name__ == '__main__':
    main()
