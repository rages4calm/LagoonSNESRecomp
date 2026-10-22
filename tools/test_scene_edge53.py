"""ROM-free Atland negative-edge fill checked against actual SDK PPU/shadow."""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--scratch-root',required=True,type=Path)
    p.add_argument('--compiler',default='clang')
    a=p.parse_args();repo=Path(__file__).resolve().parents[1];workspace=repo.parent.resolve()
    raw=a.scratch_root.absolute();out=raw.resolve()
    if not out.is_relative_to(workspace) or out.exists():p.error('Use a fresh workspace edge scratch directory')
    parts=out.relative_to(workspace).parts
    if len(parts)<3 or not re.fullmatch(r'work-native-batch[0-9]+',parts[0]) or parts[1]!='edge':
        p.error('Use a fresh directory under the current work-native-batch<number>/edge')
    for ancestor in (raw,*raw.parents):
        if ancestor==workspace:break
        if ancestor.is_symlink() or getattr(ancestor,'is_junction',lambda:False)():p.error('Scratch ancestors must not be links')
    compiler=shutil.which(a.compiler)
    if not compiler:p.error('Compiler unavailable')
    sources=['tests/lagoon_scene_edge53_test.c','tools/test_scene_edge53.py','src/lagoon_scene_renderer.c','src/lagoon_wide_entities.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c','snesrecomp/runner/src/snes/ws_shadow.c']
    sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
    hashes={f:sha(repo/f) for f in sources};out.mkdir(parents=True);exe=out/'scene-edge53.exe'
    cmd=[compiler,'-std=c11','-O2','-DSNESRECOMP_REVERSE_DEBUG=0','-I'+str(repo/'src'),'-I'+str(repo/'snesrecomp/runner/src'),'-I'+str(repo/'snesrecomp/runner/src/snes'),str(repo/'tests/lagoon_scene_edge53_test.c'),str(repo/'snesrecomp/runner/src/snes/ppu.c'),str(repo/'snesrecomp/runner/src/snes/ppu_legacy.c'),str(repo/'snesrecomp/runner/src/snes/ws_shadow.c'),'-o',str(exe)]
    result={'passed':False,'scope':'Synthetic source-qualified ordinary Atland packets, actual SDK fast-PPU plus real shadow lookup and independent whole-field gate. All8 fine phases,342/448/682/800 widths, two distinct fill metatiles and264/269 camera cases. Tests source/cache keys and negative controls; no ROM, gameplay, GPU, actor, timing or new-room coverage claimed. Actual original moving-route whole-field refusals are separate live evidence.','sources':hashes,'runs':[]}
    try:
        for command in (cmd,[str(exe)]):
            r=subprocess.run(command,cwd=repo,capture_output=True,text=True)
            result['runs'].append({'command':command,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
            print(r.stdout,end='');print(r.stderr,end='')
            if r.returncode:raise RuntimeError('Edge oracle command failed')
        assert hashes=={f:sha(repo/f) for f in sources},'Source changed during test'
        result['passed']=True
    finally:(out/'result.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
