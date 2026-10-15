"""Apply Lagoon's reviewed compatibility/timing patch to the pinned upstream."""
from pathlib import Path
import subprocess

BASE='a6be9756285f26ccc74539c811d4d23d57d8c33d'
root=Path(__file__).resolve().parents[1]
framework=root/'snesrecomp'
patch=root/'patches/lagoon-framework.patch'


def git(*args):
    return subprocess.run(['git','-C',str(framework),*args],capture_output=True,text=True)


def main():
    if not patch.is_file():raise SystemExit('Missing patches/lagoon-framework.patch')
    if git('apply','--reverse','--check',str(patch)).returncode==0:
        print('Lagoon framework patch already applied');return
    head=git('rev-parse','HEAD')
    if head.returncode or head.stdout.strip()!=BASE:
        raise SystemExit('Framework must be at the pinned upstream revision. Do not overwrite a different checkout.')
    check=git('apply','--check',str(patch))
    if check.returncode:raise SystemExit('Patch check failed; no files changed:\n'+check.stderr)
    result=git('apply',str(patch))
    if result.returncode:raise SystemExit(result.stderr)
    print('Applied Lagoon compatibility/timing patch to upstream '+BASE)


if __name__=='__main__':main()
