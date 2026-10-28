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
    parser.add_argument('--reuse-generated', action='store_true', help='Use the existing locally generated C after checking its manifest exists')
    parser.add_argument('--jobs', type=int, default=min(os.cpu_count() or 2, 4))
    parser.add_argument('--cmake-arg', action='append', default=[])
    args = parser.parse_args()
    if platform.system() not in ('Linux', 'Darwin'):
        parser.error('Run on Linux or macOS; this is not a cross-compiler or Windows build wrapper')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if args.setup_host and (args.rom or args.reuse_generated):
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
    command = ['cmake', '-S', str(ROOT), '-B', str(build), '-G', 'Ninja',
               '-DCMAKE_BUILD_TYPE=Release', '-DLAGOON_ENABLE_LEGACY_LAUNCHER=OFF',
               '-DLAGOON_PORTABLE_STATIC_SDL3=ON',
               '-DSNESRECOMP_FRAME_IMPL=HLE', '-DSNESRECOMP_EXECUTION_DEFAULT=off',
               '-DSNESRECOMP_SETUP_HOST=' + ('ON' if args.setup_host else 'OFF'),
               '-DLAGOON_GENERATED_DIR=' + str(generated), *args.cmake_arg]
    if platform.system() == 'Darwin':
        command += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0']
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run(['cmake', '--build', str(build), '--target', 'LagoonSNESRecomp', '-j', str(args.jobs)], check=True)
    print('Compile-only setup host built; not playable.' if args.setup_host else
          'Game built. Test on this OS with your ROM before packaging. Interpreter fallback retained.')


if __name__ == '__main__':
    main()
