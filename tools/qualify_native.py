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


def campaign_setup_writes(data):
    """Record explicit WRAM cheats and refuse the observed invalid-HP fixture.

    Lagoon's player HP maximum is byte-sized (level growth at 01:E140).
    A nonzero high HP byte can overrun the original HUD's assumptions; it is
    not a useful invulnerability cheat. Enemy HP is deliberately not capped.
    """
    writes=[]
    for number,line in enumerate(data.decode('utf-8-sig').splitlines(),1):
        parts=line.split('#',1)[0].split()
        if not parts or parts[0] not in {'poke','pokefor','forcepoke'}:continue
        if len(parts)<3:raise ValueError(f'Malformed setup write on line {number}')
        try:address=int(parts[1],16);payload=bytes.fromhex(parts[2])
        except ValueError as e:raise ValueError(f'Malformed setup bytes on line {number}') from e
        if address<0 or not payload or address+len(payload)>0x20000:
            raise ValueError(f'Out-of-WRAM setup write on line {number}')
        if address<=0x521<address+len(payload) and payload[0x521-address]:
            raise ValueError(f'Player HP above 255 on line {number}; use valid HP and bounded defense instead')
        writes.append(dict(line=number,operation=parts[0],address=address,hex=payload.hex(),
                           frames=parts[3] if parts[0]=='pokefor' and len(parts)>3 else None))
    return writes


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--route',required=True,type=Path)
    parser.add_argument('--frames',type=int,default=6245)
    parser.add_argument('--out',required=True,type=Path)
    parser.add_argument('--coverage',action='store_true')
    parser.add_argument('--campaign-warps',type=Path,help='Development-only scheduled real-descriptor warps; frozen privately and recorded as cheats')
    parser.add_argument('--no-present-log',action='store_true',help='Performance-only: omit per-field CRC/luma logging and fflush; default qualification keeps the log')
    parser.add_argument('--live-input',action='store_true',help='Opt in to merging physical keyboard/gamepads into the script; default uses only scripted guest input')
    parser.add_argument('--input-trace',action='store_true',help='Log actual guest input words for isolation checks')
    parser.add_argument('--visible',action='store_true',help='Use the real SDL display for fullscreen screenshot checks; saves/config remain isolated')
    parser.add_argument('--offscreen',action='store_true',help='Real GPU in a hidden non-fullscreen window; no desktop display changes')
    parser.add_argument('--display-screenshot-range',nargs=2,type=int,metavar=('FIRST','LAST'),help='Read back a bounded range of actual GPU frames; requires visible or offscreen')
    parser.add_argument('--display-screenshot-frame',type=int,help='Capture the actual SDL output at this frame; requires --visible')
    parser.add_argument('--features',default='',help='Comma-separated lagoon.quality feature ids; creates a private mod catalog')
    parser.add_argument('--feature',action='append',default=[],help='Additional package_id:feature_id; repeatable')
    parser.add_argument('--env',action='append',default=[],help='Allowlisted diagnostic KEY=VALUE; relative output paths stay under --out')
    parser.add_argument('--msu-prefix',type=Path,help='Read-only private PCM pack prefix; never copies music into the source tree')
    parser.add_argument('--config',type=Path,help='Private input config; never modifies the owner configuration')
    parser.add_argument('--seed-save-root',type=Path,help='Copy existing save/state files into the private run; source files are read-only')
    parser.add_argument('--screenshot-frame',type=int,help='Capture the full expanded presentation at this frame')
    parser.add_argument('--screenshot-range',nargs=2,type=int,metavar=('FIRST','LAST'),help='Save every presentation in a bounded diagnostic frame range')
    parser.add_argument('--manifest',type=Path,help='Manifest belonging to this executable; required for old retained binaries')
    args=parser.parse_args()
    if args.visible and args.offscreen:parser.error('Choose visible OR offscreen')
    if args.campaign_warps and (args.visible or args.offscreen or args.live_input):
        parser.error('Campaign warps require an isolated headless scripted run')
    if args.no_present_log and args.screenshot_range:
        parser.error('--no-present-log cannot use --screenshot-range, which creates its own per-field CRC log')
    if (args.display_screenshot_frame or args.display_screenshot_range) and not (args.visible or args.offscreen):
        parser.error('Display capture requires --visible or --offscreen')
    repo=Path(__file__).resolve().parents[1];rom=repo.parent/'Lagoon (USA).sfc'
    exe=args.exe.resolve();route=args.route.resolve();out=args.out.resolve()
    out.mkdir(parents=True,exist_ok=False)
    # Freeze input bytes before launch: another investigation may edit its source
    # script while the game process is starting. Receipts must describe what ran.
    route_bytes=route.read_bytes()
    setup_writes=campaign_setup_writes(route_bytes) if args.campaign_warps else None
    frozen_route=out/'route.txt';frozen_route.write_bytes(route_bytes)
    for name in ('saves','dumps','coverage'):(out/name).mkdir()
    seeded=[]
    if args.seed_save_root:
        source=args.seed_save_root.resolve()
        if not source.is_dir() or out.is_relative_to(source):
            parser.error('Seed save root must be an existing directory outside this output')
        for original in sorted(source.iterdir()):
            if original.is_file() and original.suffix.lower() in {'.sav','.srm','.bak'}:
                target=out/'saves'/original.name
                shutil.copy2(original,target)
                digest=hashlib.sha256(original.read_bytes()).hexdigest()
                if hashlib.sha256(target.read_bytes()).hexdigest()!=digest:raise SystemExit('Seed copy hash mismatch')
                seeded.append(dict(source=str(original),copy=str(target),sha256=digest))
    # Every run gets its own catalog and state. An explicit --config lets the
    # native host keep this CWD instead of anchoring itself beside the executable.
    host=out/'host';host.mkdir()
    catalog=host/'mods/preloaded'
    shutil.copytree(repo/'mods/preloaded/packages',catalog/'packages')
    import tomllib
    enabled=set(filter(None,args.features.split(',')))
    selected={('lagoon.quality',feature) for feature in enabled}
    for spec in args.feature:
        package,sep,feature=spec.partition(':')
        if not sep or not package or not feature:raise SystemExit('Expected --feature package_id:feature_id')
        selected.add((package,feature))
    state='format_version = 1\n';available=set()
    for manifest_path in sorted((catalog/'packages').glob('*/*/manifest.toml')):
        package=tomllib.loads(manifest_path.read_text(encoding='utf-8'))
        package_id=package['id']
        state+='\n[[package]]\nid = '+json.dumps(package_id)+'\nversion = '+json.dumps(package['version'])+'\n'
        for feature in package.get('feature',[]):
            pair=(package_id,feature['id']);available.add(pair)
            state+='\n[[feature]]\npackage_id = '+json.dumps(package_id)+'\nid = '+json.dumps(feature['id'])+'\nenabled = '+str(pair in selected).lower()+'\n'
    if selected-available:raise SystemExit(f'Unknown package features: {sorted(selected-available)}')
    (catalog/'state.toml').write_text(state,encoding='utf-8')
    if (repo/'build/keybinds.ini').exists():shutil.copy2(repo/'build/keybinds.ini',host/'keybinds.ini')
    env=dict(os.environ)
    for key in list(env):
        if key.startswith('SNESRECOMP_') or key.startswith('LAGOON_'):env.pop(key)
    env.update(SDL_AUDIODRIVER='dummy',
               SNESRECOMP_RUN_FRAMES=str(args.frames),SNESRECOMP_DUMP_DIR=str(out/'dumps'))
    if not args.no_present_log:
        env['SNESRECOMP_PRESENT_LOG']=str(out/'presents.csv')
    if args.input_trace:env['SNESRECOMP_INPUT_TRACE']='1'
    if args.visible or args.offscreen:env.pop('SDL_VIDEODRIVER',None)
    else:env['SDL_VIDEODRIVER']='dummy'
    if args.offscreen:env['SNESRECOMP_HIDDEN_WINDOW']='1'
    if args.coverage:env.update(SNESRECOMP_TIER2_CAPTURE='1',SNESRECOMP_COVERAGE_DIR=str(out/'coverage'))
    campaign_plan=None
    if args.campaign_warps:
        plan=args.campaign_warps.resolve()
        frozen=out/'campaign-warps.txt';data=plan.read_bytes()
        frozen.write_bytes(data)
        env['LAGOON_CAMPAIGN_WARPS']=str(frozen)
        campaign_plan=dict(source=str(plan),frozen=str(frozen),sha256=hashlib.sha256(data).hexdigest(),
                           explicit_cheat=True,mechanism='Scheduled actual room loader using existing ROM door destination descriptors')
    if args.screenshot_frame:
        env.update(SNESRECOMP_SCREENSHOT=str(out/'presentation.ppm'),
                   SNESRECOMP_SCREENSHOT_FRAME=str(args.screenshot_frame))
    if args.screenshot_range:
        first,last=args.screenshot_range
        if first<1 or last<first or last-first>1500:
            parser.error('Screenshot range must contain 1..1501 frames')
        (out/'frames').mkdir()
        env.update(SNESRECOMP_SCREENSHOT_DIR=str(out/'frames'),
                   SNESRECOMP_SCREENSHOT_FROM=str(first),SNESRECOMP_SCREENSHOT_TO=str(last))
    if args.display_screenshot_frame:
        env.update(SNESRECOMP_DISPLAY_SCREENSHOT=str(out/'display.bmp'),
                   SNESRECOMP_DISPLAY_SCREENSHOT_FRAME=str(args.display_screenshot_frame))
    if args.display_screenshot_range:
        first,last=args.display_screenshot_range
        if first<1 or last<first or last-first>1500:parser.error('Display range must contain 1..1501 frames')
        (out/'display-frames').mkdir()
        env.update(SNESRECOMP_DISPLAY_SCREENSHOT_DIR=str(out/'display-frames'),SNESRECOMP_DISPLAY_SCREENSHOT_FROM=str(first),SNESRECOMP_DISPLAY_SCREENSHOT_TO=str(last))
    diagnostic_paths={'LAGOON_WIDE_ENTITY_REPORT','LAGOON_SOUNDTRACK_PROBE',
                      'LAGOON_SCENE_RENDERER_REPORT',
                      'SNESRECOMP_APURAM_TRACE_FILE','SNESRECOMP_DSPREG_TRACE_FILE',
                      'SNESRECOMP_FRAME_TIMING'}
    diagnostic_numbers={'SNESRECOMP_HOST_PROFILE','SNESRECOMP_HOST_PROFILE_START_FRAME'}
    overrides={}
    for spec in args.env:
        key,sep,value=spec.partition('=')
        if not sep or key not in diagnostic_paths|diagnostic_numbers|{'LAGOON_WIDE_ASPECT','LAGOON_SCENE_RENDERER'}:
            raise SystemExit('Unsupported diagnostic override: '+spec)
        if key in diagnostic_paths:
            p=Path(value);p=(p if p.is_absolute() else out/p).resolve()
            if not p.is_relative_to(out) or p.exists():raise SystemExit('Diagnostic outputs must be fresh paths within --out')
            p.parent.mkdir(parents=True,exist_ok=True);value=str(p)
        elif key in diagnostic_numbers:
            if not re.fullmatch(r'[0-9]+',value) or not 0<=int(value)<=1000000:
                raise SystemExit('Diagnostic numeric overrides must be unsigned integers <=1000000')
            if key=='SNESRECOMP_HOST_PROFILE' and value not in {'0','1'}:
                raise SystemExit('HOST_PROFILE must be 0 or 1')
        elif key=='LAGOON_SCENE_RENDERER':
            if value not in {'off','verify','auto'}:
                raise SystemExit('Unsupported scene renderer mode: '+value)
        elif value not in {'16:9','16:10','21:9','32:9','window'}:
            raise SystemExit('Unsupported widescreen aspect: '+value)
        env[key]=value;overrides[key]=value
    pack_receipt=None
    if args.msu_prefix:
        prefix=args.msu_prefix.resolve()
        data_file=Path(str(prefix)+'.msu')
        if not data_file.is_file():raise SystemExit('MSU prefix requires its .msu data file')
        env['SNESRECOMP_MSU1']=str(prefix)
        pack_receipt={'prefix':str(prefix),'data_sha256':hashlib.sha256(data_file.read_bytes()).hexdigest()}
        pack_manifest=prefix.parent/'pack.json'
        if pack_manifest.is_file():pack_receipt['manifest_sha256']=hashlib.sha256(pack_manifest.read_bytes()).hexdigest()
        # The framework resolves MSU through an enabled declared resource and
        # intentionally clears an unbound environment-only prefix on commit.
        state+='\n[[resource]]\npackage_id = "lagoon.soundtrack"\nfeature_id = "x68000"\nid = "pack"\npath = '+json.dumps(str(prefix.parent))+'\n'
        (catalog/'state.toml').write_text(state,encoding='utf-8')
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    receipt=dict(exe=str(exe),exe_sha256=sha(exe),rom_sha256=sha(rom),route=str(route),route_sha256=hashlib.sha256(route_bytes).hexdigest(),frozen_route=str(frozen_route),frames=args.frames)
    receipt['present_log_enabled']=not args.no_present_log
    receipt['present_log_path']=str(out/'presents.csv') if not args.no_present_log else None
    receipt.update(enabled_features=sorted(enabled),enabled_package_features=sorted(selected),
                   diagnostic_overrides=overrides,private_mod_state=state,private_working_directory=str(host))
    receipt['seeded_save_files']=seeded
    receipt['campaign_warps']=campaign_plan
    receipt['campaign_setup_writes']=setup_writes
    receipt['visible_display']=args.visible
    receipt['offscreen_gpu']=args.offscreen
    receipt['display_screenshot_range']=args.display_screenshot_range
    receipt['display_screenshot_frame']=args.display_screenshot_frame
    # The framework's current save-root buffer is only 96 bytes. A short path
    # relative to our isolated CWD avoids truncating long Windows test roots.
    receipt['private_save_root']=str(out/'saves')
    receipt['save_root_argument']='../saves'
    receipt['input_msu_pack']=pack_receipt
    for rel in ('config.ini','keybinds.ini','mods/preloaded/state.toml'):
        p=repo/'build'/rel
        if p.exists():receipt[rel]=dict(sha256=sha(p),text=p.read_text(encoding='utf-8'))
    manifest=args.manifest.resolve() if args.manifest else repo/'src/gen/program_manifest.json'
    if manifest.exists():
        (out/'program_manifest.json').write_bytes(manifest.read_bytes())
        receipt['program_manifest_sha256']=sha(manifest)
    # Headless checks remove wall-clock delay. Visible demonstrations keep normal
    # pacing so the owner can follow the route. Guest rendering stays enabled.
    config_path=args.config.resolve() if args.config else repo/'build/config.ini'
    config=config_path.read_text(encoding='utf-8')
    receipt['input_config_path']=str(config_path)
    receipt['input_config_sha256']=sha(config_path)
    delay=0 if (args.visible or args.offscreen) else 1
    config=re.sub(r'(?m)^DisableFrameDelay\s*=.*$', f'DisableFrameDelay = {delay}', config)
    if args.offscreen:
        config=re.sub(r'(?m)^Fullscreen\s*=.*$', 'Fullscreen = 0', config)
    receipt['effective_disable_frame_delay']=delay
    receipt['input_trace']=args.input_trace
    receipt['script_only_input']=not args.live_input
    if not args.live_input:
        # The host ORs physical input with script input even with dummy video.
        # Disable every human seat/device only in this run's copied config.
        # These existing settings work in retained binaries; no host patch or
        # owner settings change is required. Script controller masks remain.
        config+='\n# Qualification: physical input isolated from the replay.\n[Controller]\n'
        config+=''.join(f'SourceP{seat} = 0\n'for seat in range(1,9))
        config+='\n[GamepadMap]\n'
        config+=''.join(f'EnableGamepad{seat} = false\n'for seat in range(1,9))
    (out/'config.ini').write_text(config,encoding='utf-8')
    receipt['effective_config_sha256']=sha(out/'config.ini')
    started=time.monotonic()
    with (out/'stdout.log').open('w') as stdout,(out/'stderr.log').open('w') as stderr:
        result=subprocess.run([str(exe),'--no-launcher','--config',str(out/'config.ini'),'--rom',str(rom),'--save-root','../saves',
                               '--script',str(frozen_route)],cwd=host,env=env,stdout=stdout,stderr=stderr,timeout=240)
    receipt.update(exit_code=result.returncode,elapsed_seconds=round(time.monotonic()-started,2),
                   dump_tags=sorted(p.name.removesuffix('.wram.bin') for p in (out/'dumps').glob('*.wram.bin')))
    # A bounded guest can hit architectural BRK/step caps yet let the host exit
    # successfully. Such output is diagnostic evidence, never a passing replay.
    fatal_markers=('[brk]','[interp_cap]','[interp_low_exec]','[cpu_halt]','assertion failed')
    receipt['runtime_failures']=[line.strip() for line in (out/'stderr.log').read_text(encoding='utf-8',errors='replace').splitlines()
                                 if any(marker in line.lower() for marker in fatal_markers)]
    receipt['qualification_passed']=not result.returncode and bool(receipt['dump_tags']) and not receipt['runtime_failures']
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
    summary={k:receipt[k] for k in ('exe_sha256','exit_code','elapsed_seconds')}
    summary['dump_count']=len(receipt['dump_tags'])
    print(json.dumps(summary,indent=2))
    if not receipt['qualification_passed']:raise SystemExit('route failed, reported a guest execution failure, or produced no dump evidence')


if __name__=='__main__':main()
