"""Prove legacy port-trace beam labels depend on batching, not SPC event time.

The actual SPC/APU/DSP executes a synthetic CPU-port echo/noise fixture. Full
serialized state, port scheduler and non-silent PCM must remain byte-exact
across chunk sizes; port sample indices/types/values must also match. Legacy
CPU beam labels are required to differ, documenting their observation domain.
This does not qualify a Lagoon native service or relax any replay comparator.
"""
import argparse,csv,hashlib,json,subprocess,wave
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang')
a=p.parse_args();repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src'
assert not a.out.exists(),'Choose fresh scratch output';a.out.mkdir(parents=True)
test=repo/'tests/lagoon_apu_batch_observer_test.c';exe=a.out/'apu-batch-observer.exe'
sources=[test,*[sdk/f'snes/{n}.c'for n in ('apu','spc','dsp')],sdk/'audio_trace.c']
cmd=[a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-comment','-I'+str(sdk),'-I'+str(sdk/'snes'),*[str(x)for x in sources],'-o',str(exe)]
subprocess.run(cmd,check=True);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
reference=None;cases=[]
for chunk in (1,7,31,64,257,1024,4096,32768):
 out=a.out/str(chunk);out.mkdir()
 run=subprocess.run([str(exe.resolve()),str(chunk),str(out.resolve())],capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='');run.check_returncode()
 events=list(csv.DictReader((out/'events.tsv').open(encoding='utf-8'),delimiter='\t'))
 semantic=[{k:r[k]for k in ('type','port','value','sample')}for r in events]
 hashes={n:sha(out/n)for n in ('apu-state.bin','port-scheduler.bin','pcm.wav')}
 with wave.open(str(out/'pcm.wav'),'rb')as f:pcm=f.readframes(f.getnframes());assert any(pcm),'PCM equality must be non-silent'
 if reference is None:reference=(hashes,events,semantic)
 assert hashes==reference[0],f'APU/SPC/DSP/scheduler/PCM changed with chunk{chunk}'
 assert semantic==reference[2],f'Actual event values/order/sample positions changed with chunk{chunk}'
 changed=sum(x!=y for x,y in zip(events,reference[1]))
 if chunk!=1:assert changed,'Expected old observer CPU beam labels to depend on batching'
 cases.append({'chunk':chunk,'events':len(events),'changed_legacy_beam_labels':changed,'semantic_hashes':hashes,'stdout':run.stdout})
record={'passed':True,'source_sha256':{str(x.relative_to(repo)):sha(x)for x in sources},'cases':cases,'scope':'Synthetic actual SPC/APU/DSP fixture proves legacy beam-label batching dependence only; not a live Lagoon BB05 proof or native promotion, no comparator relaxation.'}
(a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
