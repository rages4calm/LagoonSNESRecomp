"""Compare real saved/restored guest checkpoints from isolated runtime runs."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--saved-run',required=True,type=Path)
    p.add_argument('--loaded-run',required=True,type=Path)
    p.add_argument('--saved-tag',default='first-pass')
    p.add_argument('--loaded-tag',default='cold-pass')
    p.add_argument('--out',required=True,type=Path)
    a=p.parse_args()
    left=json.loads((a.saved_run/'receipt.json').read_text())
    right=json.loads((a.loaded_run/'receipt.json').read_text())
    assert left['exe_sha256']==right['exe_sha256'],'Different executables'
    assert left['program_manifest_sha256']==right['program_manifest_sha256'],'Different manifests'
    assert left['enabled_package_features']==right['enabled_package_features'],'Different feature sets'
    assert left['script_only_input'] and right['script_only_input'],'Physical input must be isolated'
    suffixes=['.wram.bin','.regs.json','.vram.bin','.oam.bin','.cgram.bin','.fb.bmp','.info.json']
    results={}
    for suffix in suffixes:
        x=(a.saved_run/'dumps'/(a.saved_tag+suffix)).read_bytes()
        y=(a.loaded_run/'dumps'/(a.loaded_tag+suffix)).read_bytes()
        results[suffix]={'equal':x==y,'saved_sha256':hashlib.sha256(x).hexdigest(),
                         'loaded_sha256':hashlib.sha256(y).hexdigest()}
    report={'exe_sha256':left['exe_sha256'],'manifest_sha256':left['program_manifest_sha256'],
            'saved_run':str(a.saved_run.resolve()),'loaded_run':str(a.loaded_run.resolve()),
            'components':results,'all_equal':all(x['equal'] for x in results.values()),
            'scope':'Bounded guest checkpoint and clock equality, not a campaign or mid-hold QoL guarantee.'}
    a.out.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'all_equal':report['all_equal'],'components':len(results)}))
    if not report['all_equal']:raise SystemExit(1)


if __name__=='__main__':main()
