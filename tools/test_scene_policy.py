"""ROM-free exact OAM policy-input reuse and previous-path benchmark. Use python -P."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--scratch-root',required=True,type=Path)
    p.add_argument('--compiler',default='clang')
    p.add_argument('--fields',type=int,default=1500)
    a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.scratch_root.resolve()
    if out.exists():p.error('Scratch root must be fresh')
    compiler=shutil.which(a.compiler)
    if not compiler:p.error('Compiler unavailable')
    if not 1<=a.fields<=20000:p.error('Fields must be1..20000')
    source=repo/'src/lagoon_scene_renderer.c';text=source.read_text()
    needle='if(!object_policy_inputs_equal(&s->policy_inputs,p)) {'
    if text.count(needle)!=1:p.error('Expected exactly one optimized policy gate')
    out.mkdir(parents=True)
    reference=out/'policy-reference.c';reference.write_text(text.replace(needle,'if(1) {'))
    common=[compiler,'-std=c11','-O2','-DSNESRECOMP_REVERSE_DEBUG=0',
            '-I'+str(repo/'src'),'-I'+str(repo/'snesrecomp/runner/src'),
            '-I'+str(repo/'snesrecomp/runner/src/snes')]
    suffix='.exe' if os.name=='nt' else ''
    result={'scope':'Exact raster eligibility and source-only observation microbenchmark; actual SDK policy/pixels. No ROM, runtime timing, GPU or whole-campaign guarantee.',
            'reference_change':'Only later-line policy gate forced to original full derivation; same current source otherwise.',
            'sources':{f.relative_to(repo).as_posix():hashlib.sha256(f.read_bytes()).hexdigest() for f in [source,repo/'tests/lagoon_scene_policy_test.c',repo/'tools/test_scene_policy.py',repo/'snesrecomp/runner/src/snes/ppu.c']},
            'reference_source_sha256':hashlib.sha256(reference.read_bytes()).hexdigest(),'runs':[],'passed':False}
    try:
        for name,flags in [('reference',['-DLAGOON_SCENE_POLICY_SOURCE="'+reference.as_posix()+'"']),('candidate',[])]:
            exe=out/(name+suffix)
            cmd=common+flags+[str(repo/'tests/lagoon_scene_policy_test.c'),str(repo/'snesrecomp/runner/src/snes/ppu_legacy.c'),'-o',str(exe)]
            build=subprocess.run(cmd,cwd=repo,capture_output=True,text=True)
            if build.returncode:raise RuntimeError(build.stderr)
            times=[];data=None
            for _ in range(3):
                run=subprocess.run([str(exe),str(a.fields)],cwd=repo,capture_output=True,text=True)
                if run.returncode:raise RuntimeError(run.stderr or run.stdout)
                data=json.loads(run.stdout.strip().splitlines()[-1]);times.append(data['benchmark_ms'])
            result['runs'].append({'name':name,'command':cmd,'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'result':data,'benchmark_ms':times})
        old,new=result['runs'];keys=['passed','metadata_cases','raster_cases','accepted','refused','decision_hash','actual_sdk_policy_vectors','actual_big_jump_pixels_preserved','benchmark_fields']
        assert all(old['result'][k]==new['result'][k] for k in keys)
        assert all(hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest for name,digest in result['sources'].items()),'Sources changed during qualification'
        median=lambda xs:sorted(xs)[len(xs)//2]
        result.update(passed=True,decision_equality=True,reference_median_ms=median(old['benchmark_ms']),candidate_median_ms=median(new['benchmark_ms']),observation_microbenchmark_ratio=median(new['benchmark_ms'])/median(old['benchmark_ms']))
        print(json.dumps(result))
    finally:(out/'result.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
