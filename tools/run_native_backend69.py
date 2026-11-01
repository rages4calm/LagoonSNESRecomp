"""Launch an opt-in native69 build in strict mode without changing settings."""
import sys

# Direct script launches otherwise put tools/dis.py ahead of Python's dis.
# Keep stdlib imports safe even when a caller omits Python's -P option.
if __name__ == '__main__' and not getattr(sys.flags, 'safe_path', False) and sys.path:
    sys.path.pop(0)

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
