"""Matched same-executable native/interpreter timing service proof; isolated fixtures only."""
from pathlib import Path
import argparse,json,re,subprocess,sys

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','manifest','route','config','seed-save-root','out'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--target',choices=('0081F7','0081BE','00899B','008985','01A4D6','01C726','01C643'),default='0081F7')
    p.add_argument('--frames',type=int,default=450)
    a=p.parse_args();out=a.out.resolve()
    if out.exists():p.error('Use a fresh private output directory')
    out.mkdir(parents=True)
    tools=Path(__file__).resolve().parent
    common=[sys.executable,'-P',str(tools/'qualify_native.py'),
        '--exe',str(a.exe.resolve()),'--manifest',str(a.manifest.resolve()),
        '--route',str(a.route.resolve()),'--config',str(a.config.resolve()),
        '--seed-save-root',str(a.seed_save_root.resolve()),'--frames',str(a.frames),
        '--features','camera_center,walk_speed','--env','LAGOON_NATIVE_WAIT_REPORT=1',
        '--env','SNESRECOMP_APURAM_TRACE_FILE=aram.log',
        '--env','SNESRECOMP_DSPREG_TRACE_FILE=dsp.log',
        '--env','SNESRECOMP_DSPOUT=dsp.s16']
    entries={}
    for mode in ('floor','native'):
        command=common+['--out',str(out/mode)]
        if mode=='floor':command+=['--env','SNESRECOMP_LLE_INTERP_TARGET='+a.target]
        subprocess.run(command,check=True)
        text=(out/mode/'stderr.log').read_text(encoding='utf-8',errors='replace')
        expression=rf'\[native-wait57\] pc={a.target} mode=M1X0 actual_aot_entries=(\d+)'
        match=re.search(expression,text)
        if not match:raise RuntimeError('Missing actual compiled-entry report')
        entries[mode]=int(match[1])
    result=subprocess.run([sys.executable,'-P',str(tools/'compare_native_runs.py'),
        str(out/'floor'),str(out/'native'),'--interp-control-target',a.target,
        '--out',str(out/'comparison.json')])
    proof={'target':a.target,'actual_aot_entries':entries,'strict_comparison_exit':result.returncode,
           'passed':result.returncode==0 and entries['floor']==0 and entries['native']>0,
           'scope':'Same EXE and manifest; selected original service control versus actual native entries. Ordered pictures, complete checkpoint state/clocks and per-frame ARAM/DSP snapshots and complete DSP sample stream remain strict. Assisted source fixtures, not a campaign.'}
    (out/'result.json').write_text(json.dumps(proof,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(proof));return 0 if proof['passed']else 1

if __name__=='__main__':raise SystemExit(main())
