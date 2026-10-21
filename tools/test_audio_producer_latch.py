"""Test the actual profiled desktop consumer-progress stall latch, ROM-free.

Narrow backport of upstream SNESRecomp65b9387. This desktop already owns the
unlocked sleep checkpoint, so no shared callback API is required. SDL consumer,
clock and thread identity are deterministic stubs; no actual device claim.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--compiler',default='clang')
    a=p.parse_args()
    if a.out.exists():p.error('Choose fresh scratch output')
    repo=Path(__file__).resolve().parents[1]
    utility=repo/'tools/test_audio_load_resync.py'
    spec=importlib.util.spec_from_file_location('audio_source_extract',utility)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    source=repo/'snesrecomp/runner/src/desktop/host_main.c'
    text=source.read_text(encoding='utf-8')
    assert 'g_audio_callback_count' not in text and 'g_audio_stalled_callback' not in text
    assert 'bool paced_realtime = !g_turbo && !g_config.disable_frame_delay;' in text
    assert 'g_audio_producer_active = paced_realtime && g_audio_device != 0;' in text
    names=('static bool ProfileAudioProducer(void) {','static void LockApuMutexProfiled(void) {','void RtlApuLock(void) {','void RtlApuUnlock(void) {')
    bodies='\n'.join(module.block(text,name)for name in names)
    assert 'dsp_trimSamples' not in bodies and 'apu_cycle' not in bodies
    assert not re.search(r'->sample(?:Read|Write)\s*(?:=(?!=)|\+=|-=|\+\+|--)',bodies)
    constant=re.search(r'^#define HOST_AUDIO_HIGH_WATER\s+[^\n]+',text,re.M)[0]
    a.out.mkdir(parents=True)
    header=a.out/'audio_producer_under_test.h';header.write_text(constant+'\n'+bodies,encoding='utf-8')
    harness=repo/'tests/lagoon_audio_producer_latch_test.c'
    executable=a.out/'audio-producer-latch-test.exe'
    subprocess.run([a.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(a.out.resolve()),str(harness),'-o',str(executable)],check=True)
    run=subprocess.run([str(executable.resolve())],capture_output=True,text=True)
    print(run.stdout,end='');print(run.stderr,end='')
    sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest()
    result=dict(passed=run.returncode==0,returncode=run.returncode,upstream_policy_commit='65b9387',
        sources={str(x.relative_to(repo)):sha(x)for x in(source,harness,utility,Path(__file__))},
        extracted_sha256=sha(header),stdout=run.stdout,stderr=run.stderr,
        source_gates=['Existing turbo/device realtime producer guard unchanged','Callback counter and increment removed; actual native FIFO cursor is latch authority','Existing profiling source compiled, nested mutex release/sleep source exercised'],
        scope='Actual extracted production lock/unlock/profile code; deterministic SDL consumer and monotonic clock. No SPC/DSP producer writes, queue discard, new shared API or global threshold changes. No actual-device/campaign assertion.')
    (a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    if not result['passed']:raise SystemExit('Actual producer latch failed')


if __name__=='__main__':main()
