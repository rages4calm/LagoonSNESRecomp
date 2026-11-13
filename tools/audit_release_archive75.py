"""Read-only independent release archive audit; never extract or run package files."""
from pathlib import Path, PurePosixPath
import argparse, hashlib, json, tarfile, zipfile

def audit(path,version):
    rows={};contracts={};modes={}
    def add(name,stream,mode=None):
        assert name not in rows and not name.startswith('/') and '..' not in PurePosixPath(name).parts,name
        h=hashlib.sha256();data=bytearray()
        while chunk:=stream.read(1048576):
            h.update(chunk)
            if name in ('windows-preview.json','portable-package.json'):data.extend(chunk)
        rows[name]=h.hexdigest()
        if data:contracts[name]=json.loads(data)
        if mode is not None:modes[name]=mode
    if path.suffix=='.zip':
        with zipfile.ZipFile(path)as z:
            for name in z.namelist():
                assert not name.endswith('/')
                with z.open(name)as stream:add(name,stream)
    else:
        with tarfile.open(path,'r|gz')as t:
            for member in t:
                if member.isdir():continue
                assert member.isfile(),member.name
                add(member.name.removeprefix('Lagoon/'),t.extractfile(member),member.mode)
    assert len(contracts)==1
    name,contract=next(iter(contracts.items()))
    assert contract['version']==version and contract['native_cpu'] is True and contract['native_only_default'] is True
    assert rows==dict(contract['files'],**{name:rows[name]})
    executable='game/LagoonSNESRecomp'+('.exe'if path.suffix=='.zip'else'')
    assert rows[executable]==contract['game_sha256']
    if modes:assert modes[executable]&0o111
    for name in rows:
        lower=name.lower()
        assert not lower.endswith(('.sfc','.smc','.srm','.sav','.wram','.pdb','.c','.cpp')),name
        assert not any(x in lower.split('/')for x in ('saves','validation','gen','native69')),name
    assert 'licenses/MMPX.txt'in rows
    checksum=hashlib.file_digest(path.open('rb'),'sha256').hexdigest()
    assert path.with_name(path.name+'.sha256').read_text().split()==[checksum,path.name]
    return dict(passed=True,file=path.name,bytes=path.stat().st_size,sha256=checksum,files=len(rows),game_sha256=contract['game_sha256'],version=version,contract_source_commit=contract.get('source_commit'))

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('archive',type=Path);parser.add_argument('--version',required=True);parser.add_argument('--out',type=Path)
    args=parser.parse_args();result=audit(args.archive,args.version);text=json.dumps(result,indent=2)+'\n'
    if args.out:args.out.write_text(text,encoding='utf-8')
    print(text)
if __name__=='__main__':main()
