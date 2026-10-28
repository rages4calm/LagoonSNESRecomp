"""Audit public runtime artifacts, then upload to an existing ports-preview draft.
Temporary artifact URLs grant only the reviewed runtime payload, never source.
"""
from pathlib import Path,PurePosixPath
import argparse,subprocess,json,os,urllib.request,urllib.parse,zipfile,tarfile,hashlib,struct
p=argparse.ArgumentParser();p.add_argument('--platform',required=True,choices=['linux-x64','macos-x64','macos-arm64']);p.add_argument('--release-id',type=int,required=True);p.add_argument('--manifest-sha',required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
repo=os.environ['GITHUB_REPOSITORY'];root=a.out.absolute();root.mkdir(parents=True,exist_ok=False)
def api(path):return json.loads(subprocess.check_output(['gh','api',path],text=True))
release=api(f'repos/{repo}/releases/{a.release_id}')
assert release['draft'] and release['prerelease'] and '-ports-preview.' in release['tag_name']
url=os.environ['PORT_ARTIFACT_URL'];parsed=urllib.parse.urlsplit(url)
assert parsed.scheme=='https' and parsed.hostname.endswith('.blob.core.windows.net')
outer=root/'artifact.zip'
try:
 with urllib.request.urlopen(url,timeout=120) as source,outer.open('xb') as output:
  total=0
  while block:=source.read(1024*1024):
   total+=len(block);assert total<512*1024**2;output.write(block)
except Exception:raise SystemExit('Runtime artifact download failed; refresh the short-lived runtime link.') from None
z=zipfile.ZipFile(outer);names=z.namelist();candidates=[n for n in names if n.endswith('.tar.gz')];assert len(candidates)==1
name=candidates[0];base=PurePosixPath(name).name;assert a.platform in base
checkname=next(n for n in names if PurePosixPath(n).name==base+'.sha256');expected=z.read(checkname).decode().split()[0]
archive=root/base;sha=hashlib.sha256()
with z.open(name) as source,archive.open('xb') as output:
 while block:=source.read(1024*1024):sha.update(block);output.write(block)
assert sha.hexdigest()==expected
actual={};modes={};headers={};contract=None
with tarfile.open(archive,'r|gz') as tar:
 for m in tar:
  path=PurePosixPath(m.name);assert path.parts[0]=='Lagoon' and '..' not in path.parts and not path.is_absolute()
  if m.isdir():continue
  assert m.isfile()
  rel=PurePosixPath(*path.parts[1:]).as_posix();assert rel not in actual
  assert path.suffix.lower() not in {'.sfc','.smc','.sav','.srm','.state','.vgz','.vgm','.bps','.c'}
  assert not rel.startswith(('local/','src/','recomp/')) and path.name!='program_manifest.json'
  if path.suffix=='.h':assert rel.startswith('game/assets/easter_egg/')
  source=tar.extractfile(m);h=hashlib.sha256();first=b'';data=bytearray()
  while chunk:=source.read(1024*1024):
   h.update(chunk)
   if len(first)<32:first=(first+chunk)[:32]
   if rel=='portable-package.json':data.extend(chunk);assert len(data)<2*1024**2
  actual[rel]=h.hexdigest();modes[rel]=m.mode;headers[rel]=first
  if rel=='portable-package.json':contract=json.loads(data)
assert contract and contract['playable'] is True and contract['platform']==a.platform
assert contract['generated_manifest_sha256']==a.manifest_sha
assert actual==dict(contract['files'],**{'portable-package.json':actual['portable-package.json']})
assert contract['game_sha256']==actual['game/LagoonSNESRecomp']
for file in ['game/LagoonSNESRecomp','lagoon.Companion','Start Lagoon.command']:assert modes[file]&0o111
for file in ['game/LagoonSNESRecomp','lagoon.Companion']:
 b=headers[file]
 if a.platform=='linux-x64':assert b[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',b,18)[0]==62
 else:assert b[:4]==b'\xcf\xfa\xed\xfe' and struct.unpack_from('<I',b,4)[0]==(0x100000c if a.platform.endswith('arm64') else 0x1000007)
assert sum(n.startswith('soundtrack/x68000/') and n.endswith('.pcm') for n in actual)==40
checksum=root/(base+'.sha256');checksum.write_text(expected+'  '+base+'\n')
for n in names:
 if n.endswith('launcher.png'):(root/'launcher.png').write_bytes(z.read(n))
for file,digest in [(archive,expected),(checksum,hashlib.sha256(checksum.read_bytes()).hexdigest())]:
 current=api(f'repos/{repo}/releases/{a.release_id}')
 assert current['draft'] and current['prerelease']
 existing=next((x for x in current['assets'] if x['name']==file.name),None)
 if existing:
  if existing['state']=='uploaded':
   assert existing['size']==file.stat().st_size and existing.get('digest')=='sha256:'+digest
   continue
  assert existing['state']=='starter'
  subprocess.run(['gh','api','--method','DELETE',f'repos/{repo}/releases/assets/{existing["id"]}'],check=True)
 subprocess.run(['gh','release','upload',release['tag_name'],str(file),'--repo',repo],check=True)
 uploaded=next(x for x in api(f'repos/{repo}/releases/{a.release_id}')['assets'] if x['name']==file.name)
 assert uploaded['state']=='uploaded' and uploaded['size']==file.stat().st_size and uploaded.get('digest')=='sha256:'+digest
record={'passed':True,'platform':a.platform,'archive':base,'archive_sha256':expected,'archive_bytes':archive.stat().st_size,'public_files':len(actual),'game_sha256':actual['game/LagoonSNESRecomp'],'launcher_sha256':actual['lagoon.Companion'],'generated_manifest_sha256':a.manifest_sha,'source_commit':os.environ.get('GITHUB_SHA'),'workflow_run':os.environ.get('GITHUB_RUN_ID'),'scope':'Native build/GL/persistence/settings and packaged UI verified separately; no target gameplay playthrough claimed.'}
(root/'qualification.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
