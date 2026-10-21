"""Independent original-ROM treasure catalogue and discovered-marker unit test."""
from pathlib import Path
import argparse,hashlib,json,struct,subprocess
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
rom=a.rom.read_bytes();assert hashlib.sha256(rom).hexdigest()=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
repo=Path(__file__).resolve().parents[1];a.out.mkdir(parents=True,exist_ok=True);rows=[]
for room,pointer in enumerate(struct.unpack_from('<75H',rom,0x2903c)):
 offset=0x20000+pointer
 while rom[offset]!=255:
  x,y,flag,reward,lo,hi=struct.unpack_from('6B',rom,offset);assert flag<128 and reward in [1,2,3]
  rows.append((room,x,y,flag));offset+=6
assert len(rows)==33 and len({r[3] for r in rows})==33
catalogue=a.out/'treasure-catalogue.txt';catalogue.write_text(''.join(' '.join(map(str,r))+'\n' for r in rows),encoding='utf-8')
stub=a.out/'stub';stub.mkdir(exist_ok=True);(stub/'common_rtl.h').write_text('const char *RtlSaveRoot(void);\nextern unsigned char g_ram[0x20000];\n')
saves=a.out/'saves';saves.mkdir(exist_ok=True);runner=repo/'snesrecomp/runner/src';binary=a.out/'map-treasure-test.exe'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(stub),'-I'+str(runner),str(repo/'tests/lagoon_map_treasure_test.c'),str(runner/'snes_overlay_draw.c'),str(repo/'src/lagoon_quest_guidance.c'),'-o',str(binary)],check=True)
q=subprocess.run([str(binary.resolve()),str(a.rom.resolve()),str(catalogue.resolve()),str(saves.resolve())],text=True,capture_output=True);print(q.stdout,q.stderr)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
(a.out/'result.json').write_text(json.dumps(dict(passed=q.returncode==0,stdout=q.stdout,stderr=q.stderr,source_hashes={n:sha(repo/n) for n in ['src/lagoon_map.c','src/lagoon_map.h','src/lagoon_quest_guidance.c','tests/lagoon_map_treasure_test.c','tools/test_map_treasure.py']},catalogue_sha256=sha(catalogue),scope='Original ROM metadata and synthetic observed discovery/open flags only; no live chest or campaign claim.'),indent=2)+'\n');q.check_returncode()
