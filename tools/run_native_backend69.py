"""Launch an opt-in native69 build in strict mode without changing settings."""
import argparse
import os
from pathlib import Path
import subprocess


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('game_args', nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    exe = args.exe.resolve()
    if not exe.is_file():
        parser.error('Game executable does not exist: ' + str(exe))
    game_args = args.game_args
    if game_args[:1] == ['--']:
        game_args = game_args[1:]
    environment = os.environ.copy()
    environment.update(LAGOON_NATIVE_BACKEND69='1', SNESRECOMP_NATIVE_ONLY='1',
                       LAGOON_NATIVE_BOOT62='off')
    return subprocess.run([str(exe), *game_args], cwd=exe.parent, env=environment).returncode


if __name__ == '__main__':
    raise SystemExit(main())
