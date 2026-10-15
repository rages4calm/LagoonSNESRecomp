"""Compare live scene-renderer output with the same executable's off-mode control."""
import argparse
import csv
import hashlib
import json
from pathlib import Path


def verify(reference, candidate, report_path):
    read=lambda p:json.loads(p.read_text(encoding='utf-8'))
    a,b=read(reference/'receipt.json'),read(candidate/'receipt.json')
    for key in ('exe_sha256','program_manifest_sha256','route_sha256','enabled_package_features','dump_tags','input_config_sha256','effective_disable_frame_delay'):
        assert a[key]==b[key],key
    assert a['script_only_input'] and b['script_only_input']
    assert a['diagnostic_overrides'].get('LAGOON_SCENE_RENDERER')=='off'
    assert b['diagnostic_overrides'].get('LAGOON_SCENE_RENDERER')=='auto'
    assert a.get('present_log_enabled',True) and b.get('present_log_enabled',True)
    seed=lambda r:sorted((Path(x['copy']).name,x['sha256'])for x in r['seeded_save_files'])
    assert seed(a)==seed(b),'Different seed states'
    for tag in a['dump_tags']:
        for suffix in ('fb.bmp','wram.bin','regs.json','vram.bin','oam.bin','cgram.bin','info.json'):
            assert (reference/'dumps'/f'{tag}.{suffix}').read_bytes()==(candidate/'dumps'/f'{tag}.{suffix}').read_bytes(),(tag,suffix)
    def stream(root):
        return [(r['frame'],r['crc32'])for r in csv.DictReader((root/'presents.csv').open(encoding='utf-8'))]
    x,y=stream(reference),stream(candidate)
    assert x==y and len(x)>0,'Presentation stream differs'
    stats=read(report_path)
    assert stats['mode']=='auto' and stats['replaced_presents']>0
    assert stats['independent_pixels']>0 and stats['renders']>0
    assert stats['mismatches']==0,{'mismatches':stats['mismatches'],'first_cases':stats.get('mismatch_details',[])[:2]}
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    return dict(exe_sha256=a['exe_sha256'],manifest_sha256=a['program_manifest_sha256'],route_sha256=a['route_sha256'],checkpoints=len(a['dump_tags']),complete_ordered_presentations=len(x),exact_seven_components_and_presents=True,renderer=stats,receipt_hashes=[sha(reference/'receipt.json'),sha(candidate/'receipt.json')],renderer_report_sha256=sha(report_path),scope='Same-executable live/off rendering comparison with script-only input; no complete-campaign or performance claim.')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference',required=True,type=Path)
    p.add_argument('--candidate',required=True,type=Path)
    p.add_argument('--report',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path)
    a=p.parse_args();result=verify(a.reference,a.candidate,a.report)
    a.out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result))


if __name__=='__main__':main()
