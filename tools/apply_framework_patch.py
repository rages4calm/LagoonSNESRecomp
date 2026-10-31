"""Apply Lagoon's reviewed compatibility/timing patch to the pinned upstream."""
from pathlib import Path
import argparse
import os
import subprocess

BASE='98f41838443a7cd7f9d56bec41387a6ebfc3bcbc'
root=Path(__file__).resolve().parents[1]
local=root/'framework.local'
selection=os.environ.get('LAGOON_FRAMEWORK_ROOT') or (local.read_text(encoding='utf-8').strip() if local.is_file() else 'snesrecomp')
framework=Path(selection)
if not framework.is_absolute():framework=root/framework
framework=framework.resolve()
patch=root/'patches/lagoon-framework.patch'


def git(*args):
    return subprocess.run(['git','-C',str(framework),*args],capture_output=True,text=True)


def main():
    global framework
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--framework',type=Path,help='Explicit SDK worktree; never reset an existing dirty SDK')
    parser.add_argument('--print-root',action='store_true',help='Print the selected SDK without applying anything')
    args=parser.parse_args()
    if args.framework:framework=args.framework.resolve()
    if args.print_root:print(framework.as_posix());return
    if not patch.is_file():raise SystemExit('Missing patches/lagoon-framework.patch')
    # Published source SDKs have no Git metadata. An already applied exact
    # patch is a read-only success there; unpatched inputs still require BASE.
    if git('apply','--reverse','--check',str(patch)).returncode==0:
        print('Lagoon framework patch already applied');return
    head=git('rev-parse','HEAD')
    if head.returncode or head.stdout.strip()!=BASE:
        raise SystemExit('Selected framework must be at '+BASE+'. Preserve dirty SDKs; select an isolated worktree with framework.local or --framework.')
    check=git('apply','--check',str(patch))
    if check.returncode:raise SystemExit('Patch check failed; no files changed:\n'+check.stderr)
    result=git('apply',str(patch))
    if result.returncode:raise SystemExit(result.stderr)
    print('Applied Lagoon compatibility/timing patch to upstream '+BASE)


if __name__=='__main__':main()
