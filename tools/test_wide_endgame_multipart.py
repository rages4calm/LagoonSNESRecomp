"""External-ROM source oracle and external-checkpoint multipart producer proof.

This tests room47 Thor11/orbs13/Bird12. Synthetic seam policy is explicit;
actual arena clamps keep these actors inside the original viewport.
"""
import argparse, hashlib, json, subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True)
p.add_argument('--thor-prefix',type=Path,required=True)
p.add_argument('--bird-prefix',type=Path,required=True)
p.add_argument('--moving-prefix',type=Path)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--compiler',default='clang')
p.add_argument('--sdl-include',type=Path,required=True)
a=p.parse_args(); repo=Path(__file__).resolve().parents[1]
def sha(f): return hashlib.sha256(f.read_bytes()).hexdigest()
assert sha(a.rom)=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
a.out.mkdir(parents=True,exist_ok=True)
common=['-std=c11','-O2','-I'+str(repo/'src'),'-I'+str(repo/'snesrecomp/runner/src')]
src=a.out/'endgame-source.exe'
subprocess.run([a.compiler,*common,'-DLAGOON_WIDE_ENTITIES_DECODER_ONLY',str(repo/'tools/test_wide_endgame_multipart_source.c'),str(repo/'src/lagoon_wide_multipart.c'),str(repo/'src/lagoon_wide_entities.c'),str(repo/'snesrecomp/runner/src/snes/interp816.c'),'-o',str(src)],check=True)
flags=['-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-I'+str(repo/'snesrecomp/runner/src/snes'),'-I'+str(repo/'snesrecomp/runner/src/desktop'),'-I'+str(a.sdl_include),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
prod=a.out/'endgame-producer.exe'
files=['tools/test_wide_endgame_multipart_producer.c','src/lagoon_wide_multipart.c','src/lagoon_wide_spell3.c','src/lagoon_scene_renderer.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c','snesrecomp/runner/src/snes/interp816.c']
subprocess.run([a.compiler,*common,*flags,*[str(repo/f) for f in files],'-o',str(prod)],check=True)
def run(args):
    r=subprocess.run([str(x.resolve()) if isinstance(x,Path) else x for x in args],capture_output=True,text=True)
    print(r.stdout,end='');print(r.stderr,end='');r.check_returncode();return r.stdout
outputs={'source':run([src,a.rom]),'Thor11_orbs13':run([prod,a.rom,a.thor_prefix,'11']),'Bird12':run([prod,a.rom,a.bird_prefix,'12'])}
if a.moving_prefix:
    moving=a.out/'draw-producer.exe'
    moving_files=['tools/test_wide_draw_producer_pose.c',*files[1:]]
    subprocess.run([a.compiler,*common,*flags,*[str(repo/f) for f in moving_files],'-o',str(moving)],check=True)
    outputs['moving_NPC_producer']=run([moving,a.rom,a.moving_prefix])
source_files=['src/lagoon_wide_multipart.c','src/lagoon_wide_multipart.h','src/lagoon_wide_entities.c','src/lagoon_wide_entities.h','tools/test_wide_endgame_multipart_source.c','tools/test_wide_endgame_multipart_producer.c','tools/test_wide_endgame_multipart.py','tools/test_wide_multipart_source.c','tools/test_wide_multipart_producer.c','tools/test_wide_projectile_producer.c','tools/test_wide_additional_producer.c','snesrecomp/runner/src/snes/interp816.c']
if a.moving_prefix:source_files.append('tools/test_wide_draw_producer_pose.c')
fixtures={name:{suffix:sha(Path(str(prefix)+'.'+suffix)) for suffix in ['wram.bin','vram.bin','cgram.bin']} for name,prefix in [('Thor11_orbs13',a.thor_prefix),('Bird12',a.bird_prefix)]}
if a.moving_prefix:fixtures['moving_NPC']={suffix:sha(Path(str(a.moving_prefix)+'.'+suffix)) for suffix in ['wram.bin','vram.bin','cgram.bin']}
receipt={'passed':True,'scope':__doc__,'outputs':outputs,'rom_sha256':sha(a.rom),'source_hashes':{f:sha(repo/f) for f in source_files},'external_fixture_hashes':fixtures,'limits':'Actual opcode/interpreter and modeled DMA/OAM policy proofs; not live phase/hardware scheduling or natural campaign. All synthetic X252 policy positions are separate from actual arena margin0. No graphics/state assets embedded.'}
(a.out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
