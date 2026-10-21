"""Build the explored map data-model test against actual Lagoon USA metadata."""
from pathlib import Path
import argparse,subprocess,hashlib,json
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
r=Path(__file__).resolve().parents[1];assert hashlib.sha256(a.rom.read_bytes()).hexdigest()=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
a.out.mkdir(parents=True,exist_ok=True);stub=a.out/'stub';stub.mkdir(exist_ok=True);(stub/'common_rtl.h').write_text('const char *RtlSaveRoot(void);\nextern unsigned char g_ram[0x20000];\n')
saves=a.out/'saves';saves.mkdir(exist_ok=True);binary=a.out/'map-test.exe';runner=r/'snesrecomp/runner/src'
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(stub),'-I'+str(runner),str(r/'tests/lagoon_map_test.c'),str(runner/'snes_overlay_draw.c'),str(r/'src/lagoon_quest_guidance.c'),'-o',str(binary)],check=True)
q=subprocess.run([str(binary.resolve()),str(a.rom.resolve()),str(saves.resolve())],text=True,capture_output=True);print(q.stdout,q.stderr)
(a.out/'result.json').write_text(json.dumps(dict(passed=q.returncode==0,stdout=q.stdout,stderr=q.stderr,source_hashes={n:hashlib.sha256((r/n).read_bytes()).hexdigest() for n in ['src/lagoon_map.c','src/lagoon_quest_guidance.c','tests/lagoon_map_test.c','tools/test_map.py']},limitations='Isolated map model, original metadata and read-only tablet hint/marker integration only; runtime pause, extra-trigger input and real descriptor qualification checked separately.'),indent=2)+'\n');q.check_returncode()
