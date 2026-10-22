"""Verbatim modal-audio API with the actual SPC, DSP and port scheduler."""
import argparse,hashlib,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',required=True,type=Path)
p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1]
sdk=repo/'snesrecomp/runner/src';a.out.mkdir(parents=True,exist_ok=True)
source=(sdk/'common_rtl.c').read_text(encoding='utf8')
block=source.split('/* BEGIN RTL_MODAL_AUDIO:',1)[1].split('*/',1)[1].split('/* END RTL_MODAL_AUDIO */',1)[0]
(a.out/'modal_audio_under_test.h').write_text(block,encoding='utf8')
files=[repo/'tests/lagoon_dialogue_audio_test.c',sdk/'snes/apu.c',sdk/'snes/spc.c',sdk/'snes/dsp.c',sdk/'audio_trace.c']
exe=(a.out/'dialogue-audio.exe').resolve()
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-ffunction-sections','-fdata-sections','-I'+str(sdk),'-I'+str(a.out.resolve()),*map(str,files),'-Wl,--gc-sections','-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='')
result={'passed':r.returncode==0,'returncode':r.returncode,'output':r.stdout,'error':r.stderr,'block_sha256':hashlib.sha256(block.encode()).hexdigest(),'sources':{str(f.relative_to(repo)):hashlib.sha256(f.read_bytes()).hexdigest()for f in files+[sdk/'common_rtl.c',sdk/'common_rtl.h',Path(__file__)]},'scope':'Actual SPC/DSP/scheduler with authored echo program and audible noise, frozen mock CPU/WRAM/PPU. Not original Lagoon music listening or SDL device integration.'}
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
raise SystemExit(r.returncode)
