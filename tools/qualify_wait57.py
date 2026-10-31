"""Matched same-executable81F7 native/interpreter proof; isolated fixtures only."""
from pathlib import Path
import argparse,json,re,subprocess,sys

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','manifest','route','config','seed-save-root','out'):
        p.add_argument('--'+name,type=Path,required=True)
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
        '--env','SNESRECOMP_DSPREG_TRACE_FILE=dsp.log']
    entries={}
    for mode in ('floor','native'):
        command=common+['--out',str(out/mode)]
        if mode=='floor':command+=['--env','SNESRECOMP_LLE_INTERP_TARGET=0081F7']
        subprocess.run(command,check=True)
        text=(out/mode/'stderr.log').read_text(encoding='utf-8',errors='replace')
        expression=r'\[native-wait57\] pc=0081F7 mode=M1X0 actual_aot_entries=(\d+)'
        match=re.search(expression,text)
        if not match:raise RuntimeError('Missing actual compiled-entry report')
        entries[mode]=int(match[1])
    result=subprocess.run([sys.executable,'-P',str(tools/'compare_native_runs.py'),
        str(out/'floor'),str(out/'native'),'--interp-control-target','0081F7',
        '--out',str(out/'comparison.json')])
    proof={'actual_aot_entries':entries,'strict_comparison_exit':result.returncode,
           'passed':result.returncode==0 and entries['floor']==0 and entries['native']>0,
           'scope':'Same EXE and manifest; original81F7 control versus actual native entries. Ordered pictures, complete checkpoint state/clocks and ARAM/DSP-write streams remain strict. Assisted source fixtures, not a campaign.'}
    (out/'result.json').write_text(json.dumps(proof,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(proof));return 0 if proof['passed']else 1

if __name__=='__main__':raise SystemExit(main())
