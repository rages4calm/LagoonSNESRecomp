"""Actual SPC/DSP and native-port cadence/reference contract, without a game."""
import argparse, hashlib, json, os, re, struct, subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];sdk=repo/'snesrecomp/runner/src';a.out.mkdir(parents=True,exist_ok=True)
requests=['bool rtl_apu_frame_timeline_active(', 'bool rtl_apu_extended_frame_timing(', 'static uint64_t rtl_apu_guest_cycle(', 'void RtlApuWrite(', 'void rtl_sync_apu_to_cpu_locked(', 'uint64_t rtl_apu_periodic_flush_threshold(', 'bool rtl_apu_absolute_periodic_enabled(', 'void rtl_apu_pace_check(']
source=(sdk/'common_rtl.c').read_text(encoding='utf8');parts=[]
for sig in requests:
 start=source.index(sig);end=start+re.search(r'^}',source[start:],re.M).end();parts.append(source[start:end])
(a.out/'apu_device_under_test.h').write_text('\n\n'.join(parts)+'\n',encoding='utf8')
exe=a.out/'apu-cadence-device.exe';harness=repo/'tests/lagoon_apu_cadence_device_test.c'
files=[harness,sdk/'snes/apu.c',sdk/'snes/spc.c',sdk/'snes/dsp.c',sdk/'audio_trace.c']
subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-ffunction-sections','-fdata-sections','-I'+str(sdk),'-I'+str(sdk/'snes'),'-I'+str(a.out.resolve()),*map(str,files),'-Wl,--gc-sections','-o',str(exe)],check=True)
env=os.environ.copy()
for k in list(env):
 if k.startswith(('SNESRECOMP_','SNES_COSIM','LAGOON_')):env.pop(k)
runs={}
for mode in ['reference','shared','partitioned','coarse','restore']:
 out=a.out/mode;out.mkdir(exist_ok=True)
 r=subprocess.run([str(exe.resolve()),mode,str(out.resolve())],env=env,capture_output=True,text=True);print(r.stdout,end='');print(r.stderr,end='')
 runs[mode]={'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr,'files':{x.name:hashlib.sha256(x.read_bytes()).hexdigest()for x in out.iterdir()if x.is_file()}}
request_observations={}
for mode in ['reference','shared','partitioned','coarse']:
 data=(a.out/mode/'request-observations.bin').read_bytes();rows=[{'actual_apu_cursor':x[0],'trace_process_sample':x[1],'event_kind':x[2],'port':x[3],'value':x[4]}for x in struct.iter_unpack('<QQBBB5x',data)];request_observations[mode]={'count':len(rows),'first_three':rows[:3],'full_sha256':hashlib.sha256(data).hexdigest()}
compared=['targets.bin','device-events.bin','guest.bin','port-state.bin']
exact={mode:all(runs[mode]['files'].get(f)==runs['reference']['files'].get(f)for f in compared)for mode in ['shared','partitioned','coarse']}
result={'status':'PASS'if all(v['returncode']==0 for v in runs.values())and all(exact.values())else'FAIL','actual_device_files':compared,'reference_exact':exact,'request_observations':request_observations,'runs':runs,'source_hashes':{str(f.relative_to(repo)):hashlib.sha256(f.read_bytes()).hexdigest()for f in files+[sdk/'common_rtl.c',sdk/'snes/interp_bridge.h',sdk/'apu_frame_clock.h',Path(__file__)]},'extracted_function_hashes':{sig:hashlib.sha256(body.encode()).hexdigest()for sig,body in zip(requests,parts)},'scope':'Authored CPU costs and SPC echo loop plus audible DSP noise; actual native RtlApuWrite, guest-clock mapping, absolute sync/pacer, real SPC/DSP/port scheduler and actual audio_trace process-lifetime samplecounter. Device files compare actual target/apply/SPC-read/write portClocks+samples+payload and final guest/rollback port state. CPU request trace sample/cursor is separately observed backlog, not compared as a guest timestamp. Snapshot test uses actual apu_saveload and rollback sched restore, proving tracecounter excluded. Not real Lagoon driver or a full-game original-opcode reference; no production hook/trace edits.'}
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8');print(json.dumps({'status':result['status'],'reference_exact':exact}));raise SystemExit(result['status']!='PASS')
