"""Read-only EBU R128 audit for the bundled X68000 music gain table.

Requires ffmpeg on PATH. Never rewrites audio, changes settings or downloads
music. Output is a fresh compact JSON receipt; test audio remains owner-local.
"""
from pathlib import Path
import argparse, hashlib, json, math, re, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--pack',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--ffmpeg',default='ffmpeg')
a=p.parse_args()
if a.out.exists():raise SystemExit('Choose a fresh receipt output')
source=Path(__file__).resolve().parents[1]/'src/lagoon_soundtrack.c'
text=source.read_text(encoding='utf-8')
block=text.split('static const uint8_t balanced_gain[41] = {',1)[1].split('};',1)[0]
gains={int(k):int(v) for k,v in re.findall(r'\[(\d+)\] = (\d+)',block)}
tracks={int(m[2]) for m in re.finditer(r'\{0x([0-9a-f]+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{([\d, ]+)\}\}',text)}
assert len(tracks)==17 and set(gains)==tracks
rows=[]
for n in sorted(tracks):
 f=a.pack/f'lagoon-x68000-{n}.pcm'
 with f.open('rb') as stream:assert stream.read(4)==b'MSU1'
 result=subprocess.run([a.ffmpeg,'-hide_banner','-nostats','-f','s16le','-ar','44100','-ac','2','-skip_initial_bytes','8','-i',str(f),'-af','ebur128=peak=true','-f','null','-'],capture_output=True,text=True,timeout=120,check=True)
 tail=result.stderr.rsplit('Summary:',1)[-1]
 loudness=float(re.search(r'I:\s*([-0-9.]+) LUFS',tail)[1])
 peak=float(re.search(r'Peak:\s*([-0-9.]+) dBFS',tail)[1])
 measured=min(255,math.floor(255*10**((-18-loudness)/20)))
 assert measured==gains[n],f'Track {n}: gain table requires a new reviewed measurement'
 adjusted=loudness+20*math.log10(gains[n]/255)
 assert -18.1<=adjusted<=-18.0
 rows.append(dict(track=n,sha256=hashlib.sha256(f.read_bytes()).hexdigest(),integrated_lufs=loudness,true_peak_dbfs=peak,msu_gain=gains[n],predicted_output_lufs=adjusted))
a.out.parent.mkdir(parents=True,exist_ok=True)
a.out.write_text(json.dumps(dict(passed=True,target_lufs=-18,method='ffmpeg EBU R128 fulltrack, attenuation only; no PCM rewrite',source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),tracks=rows),indent=2)+'\n',encoding='utf-8')
print(f'PASS {len(rows)} mapped album levels; estimated output within0.1LU of -18LUFS, no amplification')
