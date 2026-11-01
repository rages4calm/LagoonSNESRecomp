"""Strict same-executable guarded idle-poll control with positive replay evidence."""
from pathlib import Path
import argparse,json,re,subprocess,sys

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','manifest','route','config','seed-save-root','out'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--frames',type=int,default=450)
    a=p.parse_args();out=a.out.resolve()
    if out.exists():p.error('Use a fresh private output directory')
    out.mkdir(parents=True);tools=Path(__file__).resolve().parent
    common=[sys.executable,'-P',str(tools/'qualify_native.py'),
        '--exe',str(a.exe.resolve()),'--manifest',str(a.manifest.resolve()),
        '--route',str(a.route.resolve()),'--config',str(a.config.resolve()),
        '--seed-save-root',str(a.seed_save_root.resolve()),'--frames',str(a.frames),
        '--features','camera_center,walk_speed',
        '--env','SNESRECOMP_APURAM_TRACE_FILE=aram.log',
        '--env','SNESRECOMP_DSPREG_TRACE_FILE=dsp.log','--env','SNESRECOMP_DSPOUT=dsp.s16']
    modes={}
    for name,enabled in (('control','0'),('replay','1')):
        subprocess.run(common+['--env','LAGOON_IDLE_POLL_PROBE='+enabled,'--out',str(out/name)],check=True)
        log=(out/name/'stderr.log').read_text(encoding='utf-8',errors='replace')
        match=re.search(r'\[idle60\] declared=(\d+) interpreted_poll_ops=(\d+)',log)
        if not match:raise RuntimeError('Missing idle activation/opcode evidence')
        modes[name]={'declared':int(match[1]),'interpreted_poll_ops':int(match[2])}
    compare=subprocess.run([sys.executable,'-P',str(tools/'compare_native_runs.py'),
        str(out/'control'),str(out/'replay'),'--idle-poll-control','--out',str(out/'comparison.json')])
    active=(modes['control']['declared']==0 and modes['replay']['declared']==1 and
            0<modes['replay']['interpreted_poll_ops']<modes['control']['interpreted_poll_ops'])
    result={'passed':compare.returncode==0 and active,'modes':modes,
            'strict_comparison_exit':compare.returncode,
            'scope':'Guarded cycle replay, not AOT CPU coverage. Same executable/manifest; all outputs strict; positive reduction of actual interpreted poll opcodes required.'}
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result));return 0 if result['passed']else 1
if __name__=='__main__':raise SystemExit(main())
