"""Read-only sword cue/PPU qualification using an owner-local clean ROM."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    repo = Path(__file__).resolve().parents[1]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--sdk', type=Path, default=repo / 'snesrecomp')
    p.add_argument('--compiler', default='clang')
    a = p.parse_args()
    expected = '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
    assert sha(a.rom) == expected
    a.out.mkdir(parents=True, exist_ok=False)
    sdk = a.sdk.resolve() / 'runner/src'
    modules = [repo / 'src/lagoon_sword_range.c', repo / 'src/lagoon_sword_visual.c']
    test = repo / 'tests/lagoon_sword_range72_test.c'
    for source in modules:
        subprocess.run([a.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(sdk), '-c', str(source), '-o', str(a.out / (source.stem + '.obj'))], check=True)
    engine = [sdk / 'snes/ppu.c', sdk / 'snes/ppu_legacy.c']
    if (sdk / 'snes/hd_jobs.c').exists():
        engine.append(sdk / 'snes/hd_jobs.c')
    exe = a.out / ('sword-range72.exe' if os.name == 'nt' else 'sword-range72')
    # The test includes the actual visual implementation, so only its range
    # dependency is linked again; no generated game C or emulation driver.
    command = [a.compiler, '-std=c11', '-O2', '-DSNESRECOMP_REVERSE_DEBUG=0',
               '-I' + str(sdk), '-I' + str(sdk / 'snes'), str(test), str(modules[0]),
               *map(str, engine), '-o', str(exe)]
    if os.name != 'nt':
        command.append('-lm')
    subprocess.run(command, check=True)
    run = subprocess.run([str(exe.resolve()), str(a.rom.resolve())], text=True, capture_output=True)
    files = modules + [test, repo / 'src/lagoon_sword_range.h', repo / 'src/lagoon_sword_visual.h',
                      repo / 'tests/lagoon_sword_visual_test.c', Path(__file__), *engine]
    result = dict(passed=run.returncode == 0, rom_sha256=expected,
                  sources={str(f): sha(f) for f in files}, stdout=run.stdout, stderr=run.stderr,
                  scope='Actual ROM source poses constructed as fixtures, original and QoL rectangles, no RAM/guest PPU writes, copied-list/disable/refusal cleanup, capped extra objects, real legacy/fast PPU clipping/foreground/windows. Not live gameplay/display qualification.')
    (a.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(run.stdout, end='')
    print(run.stderr, end='')
    run.check_returncode()


if __name__ == '__main__':
    main()
