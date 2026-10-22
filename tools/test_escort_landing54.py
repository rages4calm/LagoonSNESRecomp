"""Focused escort jump and cloud landing original-opcode tests; isolated fresh output only."""
import argparse,gzip,hashlib,json,re,shutil,subprocess
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--scratch-root',required=True,type=Path)
 p.add_argument('--rom',required=True,type=Path)
 p.add_argument('--philips-fixture',required=True,type=Path)
 p.add_argument('--cloud-fixture',required=True,type=Path)
 p.add_argument('--compiler',default='clang')
 a=p.parse_args();repo=Path(__file__).resolve().parents[1];workspace=repo.parent.resolve()
 raw=a.scratch_root.absolute();out=raw.resolve()
 if out.exists() or not out.is_relative_to(workspace):p.error('Use a fresh workspace scratch directory')
 parts=out.relative_to(workspace).parts
 if len(parts)<3 or not re.fullmatch(r'work-native-batch[0-9]+',parts[0]) or parts[1]!='follower':p.error('Use current work-native-batch<number>/follower/fresh-child')
 for ancestor in (raw,*raw.parents):
  if ancestor==workspace:break
  if ancestor.is_symlink() or getattr(ancestor,'is_junction',lambda:False)():p.error('Scratch ancestors must not be links')
 compiler=shutil.which(a.compiler)
 if not compiler:p.error('Compiler unavailable')
 sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
 if sha(a.rom)!='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb':p.error('Original Lagoon USA ROM identity differs')
 files=['src/lagoon_follower.c','src/lagoon_follower.h','src/lagoon_qol.c','src/lagoon_qol.h','tools/test_follower.c','tests/lagoon_qol_test.c','tests/lagoon_follower_jump54_test.c','tests/lagoon_cloud_landing54_test.c','tools/test_escort_landing54.py','snesrecomp/runner/src/snes/interp816.c']
 hashes={f:sha(repo/f)for f in files};out.mkdir(parents=True)
 inputs={}
 for name,fixture,room in [('philips',a.philips_fixture,10),('cloud',a.cloud_fixture,42)]:
  blob=gzip.decompress(fixture.read_bytes())
  if len(blob)!=16442 or int.from_bytes(blob[:2],'little')!=room:raise ValueError('Terrain fixture shape/room differs')
  original=a.rom.read_bytes();offset=0x2b2df+room*56
  if blob[2:58]!=original[offset:offset+56]:raise ValueError('Terrain descriptor differs from original ROM')
  ram=bytearray(0x20000);ram[0x48e:0x490]=blob[:2];ram[0x50:0x88]=blob[2:58];ram[0x14000:0x18000]=blob[58:]
  ram[0x88:0x8b]=bytes([0,0,127]);ram[0x8b:0x8e]=bytes([0,64,127])
  width=int.from_bytes(ram[0x53:0x55],'little')*8;height=int.from_bytes(ram[0x55:0x57],'little')*8
  ram[0x492:0x494]=(width-16).to_bytes(2,'little');ram[0x494:0x496]=(height-14).to_bytes(2,'little')
  path=out/(name+'.unit-wram.bin');path.write_bytes(ram);inputs[name]=path
 result={'passed':False,'sources':hashes,'rom_sha256':sha(a.rom),'fixtures':{n:{'path':str(f.resolve()),'sha256':sha(f)}for n,f in [('philips',a.philips_fixture),('cloud',a.cloud_fixture)]},'runs':[],'scope':'Original-opcode CPU source proof on a simple unit bus. Four synthetic jump directions plus real source-loader Philips collision map, NPC midair-generation recovery, exact guard refusals. Original cloud C7F6 through actual2B destination store for expanded margins/corners, outer misses and source mutations. Live gameplay/device, display and APU proof are separate. No owner input mutation or source save-schema change.'}
 try:
  for name,test,args in [('jump54','tests/lagoon_follower_jump54_test.c',[a.rom,inputs['philips']]),('cloud54','tests/lagoon_cloud_landing54_test.c',[a.rom,inputs['cloud']])]:
   exe=out/(name+'.exe')
   cmd=[compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(repo/'src'),'-I'+str(repo/'snesrecomp/runner/src'),str(repo/test),str(repo/'snesrecomp/runner/src/snes/interp816.c'),'-o',str(exe)]
   for command in (cmd,[str(exe),*[str(x.resolve())for x in args]]):
    r=subprocess.run(command,cwd=repo,capture_output=True,text=True);result['runs'].append({'command':command,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr});print(r.stdout,end='');print(r.stderr,end='')
    if r.returncode:raise RuntimeError('Focused original-source test failed')
  assert hashes=={f:sha(repo/f)for f in files},'Source changed during test'
  result['passed']=True
 finally:(out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
