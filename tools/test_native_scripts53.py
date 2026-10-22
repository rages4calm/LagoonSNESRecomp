"""Qualify finite original outer-script windows against actual interp816 opcodes."""
import argparse, hashlib, json, re, shutil, subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',required=True,type=Path)
p.add_argument('--scratch-root',required=True,type=Path)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
if sha(a.rom)!='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb':p.error('Original source ROM required')
raw=a.scratch_root.absolute();out=raw.resolve();workspace=repo.parent.resolve()
if not out.is_relative_to(workspace) or out.exists():p.error('Use a fresh workspace scratch directory')
relative=out.relative_to(workspace)
if len(relative.parts)<3 or not re.fullmatch(r'work-native-batch[0-9]+',relative.parts[0]) or relative.parts[1]!='native':
    p.error('Use a fresh directory under the current work-native-batch<number>/native')
for ancestor in (raw,*raw.parents):
    if ancestor==workspace:break
    if ancestor.is_symlink() or getattr(ancestor,'is_junction',lambda:False)():
        p.error('Scratch ancestors must not be symlinks or junctions')
compiler=shutil.which(a.compiler)
if not compiler:p.error('Compiler unavailable')
out.mkdir(parents=True);exe=out/'scripts53.exe'
command=[compiler,'-std=c11','-O2','-Wall','-Wextra','-I'+str(sdk),str(repo/'tests/lagoon_native_scripts53_test.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)]
runs=[]
for cmd in (command,[str(exe),str(a.rom.resolve())]):
    r=subprocess.run(cmd,capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='');runs.append({'command':cmd,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
    if r.returncode:break
files=['src/lagoon_native_scripts53.c','src/lagoon_native_scripts53.h','tests/lagoon_native_scripts53_test.c','tools/test_native_scripts53.py','snesrecomp/runner/src/snes/interp816.c']
(out/'result.json').write_text(json.dumps({'passed':r.returncode==0,'sources':{f:sha(repo/f)for f in files},'rom_sha256':sha(a.rom),'runs':runs,'scope':'Actual source header/command-fetch windows and real JSR/80E9 dispatch prefixes, constant8-master unit bus. NPC/event inputs before shared tail and post-child Y are explicit preconditions, not claims of executing those bodies. CC9A, command child JSR and terminal RTS are original boundaries. All179 authored pointer indices,40 commands and FF terminator tested. Live bridge/APU/refresh/hardware cadence and positive runtime activation are separate gates.'},indent=2)+'\n')
raise SystemExit(r.returncode)
