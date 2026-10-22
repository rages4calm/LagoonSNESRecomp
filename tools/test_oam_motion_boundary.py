"""ROM-free actual SDK OAM history and sprite regressions. Run with python -P."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scratch-root', required=True, type=Path,
                        help='New directory; existing paths are always refused.')
    parser.add_argument('--compiler', default='clang')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    out = args.scratch_root.resolve()
    if out.exists():
        parser.error('--scratch-root must not already exist')
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error('Compiler not found: ' + args.compiler)
    sdk = repo / 'snesrecomp/runner/src'
    exe = out / ('oam-motion.exe' if os.name == 'nt' else 'oam-motion')
    command = [compiler, '-std=c11', '-O1', '-DSNESRECOMP_REVERSE_DEBUG=0',
               '-I' + str(sdk), '-I' + str(sdk / 'snes'),
               str(repo / 'tests/lagoon_oam_motion_boundary_test.c'),
               str(sdk / 'snes/ppu_legacy.c'), '-o', str(exe)]
    out.mkdir(parents=True, exist_ok=False)
    files = ['tests/lagoon_oam_motion_boundary_test.c',
             'tools/test_oam_motion_boundary.py',
             'snesrecomp/runner/src/snes/ppu.c',
             'snesrecomp/runner/src/snes/ppu.h',
             'snesrecomp/runner/src/snes/ppu_legacy.c',
             'snesrecomp/runner/src/snes/ppu_legacy.h',
             'snesrecomp/runner/src/snes/ws_shadow.h',
             'snesrecomp/tests/ppu/ppu_sprite_limit_test.c']
    result = {
        'scope': 'ROM-free synthetic actual SDK history/evaluator pixels and '
                 'existing SDK sprite suite; no live route or campaign claim.',
        'sources': {f: hashlib.sha256((repo / f).read_bytes()).hexdigest()
                    for f in files},
        'runs': [], 'passed': False,
    }
    try:
        for cmd in [command, [str(exe)], [str(exe), '--sdk-suite']]:
            run = subprocess.run(cmd, cwd=repo, capture_output=True, text=True)
            result['runs'].append({'command': cmd, 'exit_code': run.returncode,
                                   'stdout': run.stdout, 'stderr': run.stderr})
            print(run.stdout, end='')
            print(run.stderr, end='')
            if run.returncode:
                return run.returncode
        result['executable_sha256'] = hashlib.sha256(exe.read_bytes()).hexdigest()
        result['passed'] = True
        return 0
    finally:
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n',
                                         encoding='utf-8')


if __name__ == '__main__':
    raise SystemExit(main())
