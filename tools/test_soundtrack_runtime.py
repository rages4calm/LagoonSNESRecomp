"""Check source-bound music mappings and actual MSU playback using generated tones."""
from pathlib import Path
import argparse, hashlib, json, re, struct, subprocess
p=argparse.ArgumentParser(description=__doc__)
for name in ['rom','out']:p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--compiler',default='clang');a=p.parse_args()
repo=Path(__file__).resolve().parents[1];runner=repo/'snesrecomp/runner/src'
source=repo/'src/lagoon_soundtrack.c';test=repo/'tests/soundtrack_runtime_test.c'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected=next(x.split('=',1)[1].strip()for x in(repo/'rom_identity.txt').read_text(encoding='utf8').splitlines()if x.startswith('expected_sha256'))
assert sha(a.rom)==expected
rom=a.rom.read_bytes();text=source.read_text(encoding='utf8');mappings=[]
for m in re.finditer(r'\{0x([0-9a-f]+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{([\d, ]+)\}\}',text):
 event,track,b0,b1=(int(m[1],16),int(m[2]),int(m[3]),int(m[4]));payload=[int(x)for x in m[5].split(',')]
 row=bytes([b0,b1,7,*payload]);assert rom[0x679fc+6*event:0x679fc+6*event+6]==row
 mappings.append({'cue_hex':f'{event:02X}','archive_track':track,'source_row':row.hex()})
assert len(mappings)==17 and len({x['cue_hex']for x in mappings})==17
assert rom[0xd052:0xd056]==bytes([0x22]*4)
a.out.mkdir(parents=True,exist_ok=True);tones=a.out/'tones';tones.mkdir(exist_ok=True)
# Track2 is intentionally missing and4 malformed, preserving fallback cases.
for track in {1,*[x['archive_track']for x in mappings]}-{2,4}:
 (tones/f'lagoon-x68000-{track}.pcm').write_bytes(b'MSU1'+struct.pack('<I',0)+struct.pack('<hh',1000,-1000)*8192)
(tones/'lagoon-x68000-4.pcm').write_bytes(b'bad')
exe=a.out/'soundtrack-runtime.exe';command=[a.compiler,'-std=c11','-O2','-I'+str(runner),'-I'+str(repo/'src'),str(test),str(source),str(runner/'snes/msu1.c'),'-o',str(exe)]
subprocess.run(command,check=True)
r=subprocess.run([str(exe.resolve()),str((tones/'lagoon-x68000').resolve())],capture_output=True,text=True)
print(r.stdout,end='');print(r.stderr,end='')
result={'passed':r.returncode==0,'rom_sha256':expected,'source_sha256':sha(source),'test_sha256':sha(test),'tool_sha256':sha(Path(__file__)),'mappings':mappings,'stdout':r.stdout,'stderr':r.stderr,'scope':'All17 actual ROM6byte cue rows plus real MSU1 mixer/state/SFX/IPL/unknown/missing/invalid/source-mismatch tests with generated tones. Real listening/driver scheduling is separate live evidence. No original music is included in these fixtures.'}
(a.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8');r.check_returncode()
