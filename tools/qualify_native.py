"""Run a native candidate with an isolated new save root and evidence directory."""
import argparse
import hashlib
import json
import os
import re
import shutil
from pathlib import Path
import subprocess
import time


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--route',required=True,type=Path)
    parser.add_argument('--frames',type=int,default=6245)
    parser.add_argument('--out',required=True,type=Path)
    parser.add_argument('--coverage',action='store_true')
    parser.add_argument('--features',default='',help='Comma-separated lagoon.quality feature ids; creates a private mod catalog')
    parser.add_argument('--config',type=Path,help='Private input config; never modifies the owner configuration')
    parser.add_argument('--screenshot-frame',type=int,help='Capture the full expanded presentation at this frame')
    parser.add_argument('--manifest',type=Path,help='Manifest belonging to this executable; required for old retained binaries')
    args=parser.parse_args()
    repo=Path(__file__).resolve().parents[1];rom=repo.parent/'Lagoon (USA).sfc'
    exe=args.exe.resolve();route=args.route.resolve();out=args.out.resolve()
    out.mkdir(parents=True,exist_ok=False)
    for name in ('saves','dumps','coverage'):(out/name).mkdir()
    # Every run gets its own catalog and state. An explicit --config lets the
    # native host keep this CWD instead of anchoring itself beside the executable.
    host=out/'host';host.mkdir()
    catalog=host/'mods/preloaded'
    shutil.copytree(repo/'mods/preloaded/packages',catalog/'packages')
    import tomllib
    quality=tomllib.loads((catalog/'packages/lagoon.quality/1.0.0/manifest.toml').read_text(encoding='utf-8'))
    available={f['id'] for f in quality['feature']}
    enabled=set(filter(None,args.features.split(',')))
    if enabled-available:raise SystemExit(f'Unknown QoL features: {sorted(enabled-available)}')
    state='format_version = 1\n\n[[package]]\nid = "lagoon.quality"\nversion = "1.0.0"\n'
    for feature in sorted(available):
        state+='\n[[feature]]\npackage_id = "lagoon.quality"\nid = '+json.dumps(feature)+'\nenabled = '+str(feature in enabled).lower()+'\n'
    (catalog/'state.toml').write_text(state,encoding='utf-8')
    if (repo/'build/keybinds.ini').exists():shutil.copy2(repo/'build/keybinds.ini',host/'keybinds.ini')
    env=dict(os.environ)
    for key in list(env):
        if key.startswith('SNESRECOMP_') or key.startswith('LAGOON_'):env.pop(key)
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',
               SNESRECOMP_RUN_FRAMES=str(args.frames),SNESRECOMP_DUMP_DIR=str(out/'dumps'),
               SNESRECOMP_PRESENT_LOG=str(out/'presents.csv'))
    if args.coverage:env.update(SNESRECOMP_TIER2_CAPTURE='1',SNESRECOMP_COVERAGE_DIR=str(out/'coverage'))
    if args.screenshot_frame:
        env.update(SNESRECOMP_SCREENSHOT=str(out/'presentation.ppm'),
                   SNESRECOMP_SCREENSHOT_FRAME=str(args.screenshot_frame))
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    receipt=dict(exe=str(exe),exe_sha256=sha(exe),rom_sha256=sha(rom),route=str(route),route_sha256=sha(route),frames=args.frames)
    receipt.update(enabled_features=sorted(enabled),private_mod_state=state,private_working_directory=str(host))
    for rel in ('config.ini','keybinds.ini','mods/preloaded/state.toml'):
        p=repo/'build'/rel
        if p.exists():receipt[rel]=dict(sha256=sha(p),text=p.read_text(encoding='utf-8'))
    manifest=args.manifest.resolve() if args.manifest else repo/'src/gen/program_manifest.json'
    if manifest.exists():
        (out/'program_manifest.json').write_bytes(manifest.read_bytes())
        receipt['program_manifest_sha256']=sha(manifest)
    # Use an isolated config and remove host wall-clock throttling. Guest frame
    # timing and rendering stay enabled; turbo would skip rendering, so avoid it.
    config_path=args.config.resolve() if args.config else repo/'build/config.ini'
    config=config_path.read_text(encoding='utf-8')
    receipt['input_config_path']=str(config_path)
    receipt['input_config_sha256']=sha(config_path)
    config=re.sub(r'(?m)^DisableFrameDelay\s*=.*$', 'DisableFrameDelay = 1', config)
    (out/'config.ini').write_text(config,encoding='utf-8')
    started=time.monotonic()
    with (out/'stdout.log').open('w') as stdout,(out/'stderr.log').open('w') as stderr:
        result=subprocess.run([str(exe),'--no-launcher','--config',str(out/'config.ini'),'--rom',str(rom),'--save-root',str(out/'saves'),
                               '--script',str(route)],cwd=host,env=env,stdout=stdout,stderr=stderr,timeout=240)
    receipt.update(exit_code=result.returncode,elapsed_seconds=round(time.monotonic()-started,2),
                   dump_tags=sorted(p.name.removesuffix('.wram.bin') for p in (out/'dumps').glob('*.wram.bin')))
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2))
    summary={k:receipt[k] for k in ('exe_sha256','exit_code','elapsed_seconds')}
    summary['dump_count']=len(receipt['dump_tags'])
    print(json.dumps(summary,indent=2))
    if result.returncode or not receipt['dump_tags']:raise SystemExit('route failed or produced no dump evidence')


if __name__=='__main__':main()
