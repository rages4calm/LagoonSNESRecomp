"""Qualify original NMI request/DMA-register preparation; no DMA replacement."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for key in ('rom','out'):p.add_argument('--'+key,required=True,type=Path)
p.add_argument('--compiler',default='clang');a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=Path(os.environ.get('SNESRECOMP_ROOT',repo/'snesrecomp'))/'runner/src'
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
if sha(a.rom)!='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb':p.error('Original source ROM required')
a.out.mkdir(parents=True,exist_ok=True);exe=a.out/'nmi_prep68.exe';subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-I'+str(sdk),str(repo/'tests/lagoon_native_nmi_prep68_test.c'),str(sdk/'snes/interp816.c'),'-o',str(exe)],check=True)
r=subprocess.run([str(exe.resolve()),str(a.rom.resolve())],capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='')
files=[repo/'src/lagoon_native_nmi_prep68.c',repo/'src/lagoon_native_nmi_prep68.h',repo/'tests/lagoon_native_nmi_prep68_test.c',repo/'tests/lagoon_native_interrupt66_test.c',repo/'src/lagoon_native_interrupt66.c',sdk/'snes/interp816.c',Path(__file__)]
(a.out/'result.json').write_text(json.dumps(dict(passed=r.returncode==0,stdout=r.stdout,stderr=r.stderr,sources={str(f):sha(f)for f in files},scope='Finite original NMI request-control and DMA/PPU register preparation. Complete hardware-frame/prologue and all256 request-mask CPU/bus fixtures; real original DMA-trigger instructions execute in byte-bus fixture, not physical DMA/device stalls. Actual hardware/ARAM/DSP live proof separate. Existing66 matrix not rerun.'),indent=2)+'\n',encoding='utf-8');raise SystemExit(r.returncode)
