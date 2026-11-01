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


DIAGNOSTIC_RANGES={'SNESRECOMP_AOTBLK': (10,10000000),
                   'SNESRECOMP_CYC_WATCH': (16,0xffffff)}
DIAGNOSTIC_BOOLEANS={'SNESRECOMP_HOST_PROFILE','SNESRECOMP_LLE_BOUNCE','SNESRECOMP_DMA_LOG','LAGOON_ACTOR_COMPUTE_REPORT','LAGOON_NATIVE_MUSIC_TRACE','LAGOON_NATIVE_ITERATOR52_REPORT','LAGOON_NATIVE_SCRIPTS53_REPORT','LAGOON_NATIVE_WAIT_REPORT','LAGOON_IDLE_POLL_PROBE','LAGOON_FRAME_WAIT62','LAGOON_TIMING64','LAGOON_TIMING65','LAGOON_TIMING66','LAGOON_TIMING67','LAGOON_TIMING68','LAGOON_TIMING65_SCENE','LAGOON_TIMING65_VRAM','SNESRECOMP_DSPOUT_DRAIN'}
DIAGNOSTIC_BOOLEANS.add('SNESRECOMP_NATIVE_ONLY')
DIAGNOSTIC_BOOLEANS.add('LAGOON_NATIVE_BACKEND69')


def native_only_evidence(text,requested):
    """Require an armed runtime and an unambiguous zero-decoder report.

    Environment variables alone prove nothing: retained executables can ignore
    unknown options. A missing/ambiguous capability or summary cannot pass.
    Bypassed accelerations did no guest work and are distinct from replay.
    """
    if not requested:return None
    errors=[]
    armed=re.findall(r'^\[native-only\] armed schema=1 processor=main65816 strict=1$',text,re.M)
    if len(armed)!=1:errors.append('expected exactly one main65816 strict guard acknowledgement')
    reports=re.findall(r'^\[native-only\] summary (.*)$',text,re.M)
    counters={}
    if len(reports)!=1:errors.append('expected exactly one strict execution summary')
    else:
        for token in reports[0].split():
            key,sep,value=token.partition('=')
            if not sep or not re.fullmatch(r'[a-z_]+',key) or not re.fullmatch(r'0|[1-9][0-9]{0,19}',value) or key in counters:
                errors.append('malformed or duplicate strict execution counter');continue
            number=int(value)
            if number>0xffffffffffffffff:errors.append('strict execution counter exceeds uint64');continue
            counters[key]=number
    required={'schema','strict','interpreted_opcodes','completed_opcodes','runtime_invocations',
              'unqualified_replay_activations','generic_accelerations_bypassed','blocked_events',
              'bridge_uncovered_attempts','source_native_opcodes','interrupt_native_opcodes'}
    if required-counters.keys():errors.append('missing strict execution counters: '+','.join(sorted(required-counters.keys())))
    if counters.get('schema')!=1 or counters.get('strict')!=1:errors.append('unsupported or inactive strict execution report')
    for key in ('interpreted_opcodes','completed_opcodes','runtime_invocations',
                'unqualified_replay_activations','blocked_events','bridge_uncovered_attempts'):
        if counters.get(key,0)!=0:errors.append('nonzero '+key)
    if '[native-only] first-uncovered' in text:errors.append('runtime reported an uncovered instruction or replay')
    return dict(requested=True,passed=not errors,errors=errors,counters=counters)


def native_backend_evidence(text,requested,strict_evidence):
    """A requested generated backend needs runtime identity and actual execution.

    This is narrower than whole-game native qualification; the strict execution
    report must independently pass. A registered but unused program proves no
    execution, and an old executable silently ignoring the option cannot pass.
    """
    if not requested:return None
    errors=[]
    registrations=re.findall(r'^\[native-blocks69\] registered program=([A-Za-z_]\w*) profile=(0|[1-9][0-9]*) entries=([1-9][0-9]*) source_sha256=([0-9a-f]{64})$',text,re.M)
    bindings=re.findall(r'^\[native-blocks69\] bound generation=(0|[1-9][0-9]*) profile=(0|[1-9][0-9]*)$',text,re.M)
    if len(registrations)!=1:errors.append('expected exactly one generated program registration')
    if not bindings:errors.append('missing generated program runtime binding')
    if registrations and any(row[1]!=registrations[0][1] for row in bindings):errors.append('generated program profile binding mismatch')
    if not strict_evidence or not strict_evidence.get('passed'):errors.append('generated backend requires successful strict execution evidence')
    count=(strict_evidence or {}).get('counters',{}).get('generated_native_opcodes')
    if type(count) is not int or count<=0:errors.append('no generated native opcode execution evidenced')
    if '[native-blocks69] contract-refused' in text:errors.append('generated backend contract refused')
    return dict(requested=True,passed=not errors,errors=errors,registrations=registrations,bindings=bindings,generated_native_opcodes=count)


def native_block_coverage_evidence(path,text,strict_evidence):
    """Validate an optional actual generated-block capture, never a game percentage."""
    if path is None:return None
    errors=[];document={};raw=b'';programs=[]
    def pairs(items):
        out={}
        for key,value in items:
            if key in out:raise ValueError('duplicate JSON field')
            out[key]=value
        return out
    def no_float(value):raise ValueError('non-integer JSON number')
    def uint(value,maximum=0xffffffffffffffff):
        if type(value) is not int or not 0<=value<=maximum:raise ValueError('invalid uint counter')
        return value
    try:
        file=Path(path)
        if file.stat().st_size>32*1024*1024:raise ValueError('coverage manifest exceeds32MiB')
        raw=file.read_bytes();document=json.loads(raw,object_pairs_hook=pairs,parse_float=no_float,parse_constant=no_float)
        if type(document) is not dict:raise ValueError('expected manifest object')
        if uint(document['schema'])!=1 or document['processor']!='main65816' or document['kind']!='generated-native-blocks':raise ValueError('unsupported coverage schema/scope')
        if document['finalization']!='normal-exit' or document['counter_consistent'] is not True or uint(document['errors'])!=0:raise ValueError('partial or inconsistent coverage')
        pid=uint(document['pid']);total=uint(document['hits_total']);generated=uint(document['generated_native_opcodes'])
        armed=re.findall(r'^\[native-block-coverage\] armed schema=1 processor=main65816 kind=generated-native-blocks pid=([1-9][0-9]{0,19})$',text,re.M)
        finalized=re.findall(r'^\[native-block-coverage\] finalized schema=1 finalization=normal-exit generated_native_opcodes=([0-9]{1,20}) hits_total=([0-9]{1,20}) programs=([0-9]{1,20}) nonzero_keys=([0-9]{1,20}) errors=([0-9]{1,20}) pid=([0-9]{1,20})$',text,re.M)
        if len(armed)!=1 or len(finalized)!=1:raise ValueError('missing or ambiguous coverage acknowledgements')
        if uint(int(armed[0]))!=pid:raise ValueError('coverage armed pid mismatch')
        entries=document['programs']
        if type(entries) is not list or uint(document['program_count'])!=len(entries):raise ValueError('coverage program inventory mismatch')
        counted=nonzero=0
        for program in entries:
            profile=uint(program['profile'],0xffffffff);compiled=uint(program['compiled_key_count'])
            if not isinstance(program['name'],str) or not re.fullmatch('[0-9a-f]{64}',program['original_source_sha256']):raise ValueError('invalid coverage program identity')
            if not compiled or type(program['keys']) is not list or len(program['keys'])>compiled:raise ValueError('invalid key inventory')
            hits=0;seen=set()
            for row in program['keys']:
                key=tuple(uint(row[k],limit)for k,limit in [('pc24',0xffffff),('m',1),('x',1),('e',1),('profile',0xffffffff)])
                if key in seen or key[4]!=profile or (key[3] and key[1:3]!=(1,1)):raise ValueError('duplicate or invalid native key')
                seen.add(key);n=uint(row['hits'])
                if not n:raise ValueError('zero hit row in nonzero inventory')
                hits+=n
            if hits!=uint(program['hits_total']):raise ValueError('program hit sum mismatch')
            counted+=hits;nonzero+=len(seen)
            programs.append(dict(name=program['name'],profile=profile,original_source_sha256=program['original_source_sha256'],compiled_key_count=compiled,hits_total=hits,nonzero_keys=len(seen)))
        if counted!=total or total!=generated or nonzero!=uint(document['nonzero_key_count']):raise ValueError('global native hit sum mismatch')
        if tuple(map(int,finalized[0]))!=(generated,total,len(entries),nonzero,0,pid):raise ValueError('coverage final acknowledgement mismatch')
        expected=(strict_evidence or {}).get('counters',{}).get('generated_native_opcodes')
        if strict_evidence is not None and (type(expected) is not int or expected!=generated):raise ValueError('strict execution counter mismatch')
    except (OSError,ValueError,KeyError,TypeError) as error:
        errors.append(str(error))
        if type(document) is not dict:document={}
    return dict(requested=True,passed=not errors,errors=errors,path=str(path),sha256=hashlib.sha256(raw).hexdigest() if raw else None,
                hits_total=document.get('hits_total'),nonzero_key_count=document.get('nonzero_key_count'),programs=programs,
                scope='Actual generated blocks only; excludes legacy AOT/HLE/source-native and is not a game completion percentage')


def validate_offline_dsp_drain(real_audio,config_text,overrides):
    sections=re.findall(r'(?ims)^\[Sound\]\s*$(.*?)(?=^\[|\Z)',config_text or '')
    lines=re.findall(r'(?im)^EnableAudio\s*=\s*([^\r\n]+)',sections[0]) if len(sections)==1 else []
    if real_audio or len(lines)!=1 or lines[0].strip()!='0' or not any(x.startswith('SNESRECOMP_DSPOUT=') and len(x.split('=',1)[1]) for x in overrides):
        raise SystemExit('Offline DSP drain requires explicit disabled host audio and a DSP output capture')


def validate_diagnostic_boolean(key,value):
    if key not in DIAGNOSTIC_BOOLEANS or value not in {'0','1'}:
        raise SystemExit('Diagnostic boolean must be 0 or 1: '+key+'='+value)


def validate_diagnostic_number(value):
    if not re.fullmatch(r'[0-9]+',value) or not 0<=int(value)<=1000000:
        raise SystemExit('Diagnostic numeric overrides must be unsigned integers <=1000000')


def validate_diagnostic_range(key,value):
    """Validate the framework's bounded decimal-frame or hex-PC trace pair."""
    base,maximum=DIAGNOSTIC_RANGES[key]
    pattern=r'[0-9]+-[0-9]+' if base==10 else r'[0-9a-fA-F]{1,6}-[0-9a-fA-F]{1,6}'
    if not re.fullmatch(pattern,value):
        raise SystemExit('Diagnostic range must be an ordered lo-hi pair: '+key+'='+value)
    lo,hi=(int(v,base) for v in value.split('-'))
    if not 0<=lo<=hi<=maximum:
        raise SystemExit('Diagnostic range outside supported bounds: '+key+'='+value)


def validate_fixture_room(value,*,campaign_warps,visible,live_input,real_audio,frames):
    """Only the explicitly isolated3F resource fixture may override a room."""
    if value not in {'3f','3F'} or not campaign_warps or visible or live_input or real_audio or not 1<=frames<=10000000:
        raise SystemExit('Room fixture requires literal3f, campaign plan, bounded dummy-audio scripted hidden run')


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


def runtime_failure_lines(text):
    fatal_markers=('[brk]','[interp_cap]','[interp_low_exec]','[cpu_halt]','[native_contract]', '[dsp_capture]','assertion failed','[native-only] first-uncovered','[native-blocks69] contract-refused')
    return [line.strip() for line in text.splitlines()
            if any(marker in line.lower() for marker in fatal_markers)
            or re.search(r'(?:lagoon\.qol:|\[lagoon-qol\])[^\r\n]*\brefused\b',line,re.IGNORECASE)]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--route',required=True,type=Path)
    parser.add_argument('--frames',type=int,default=6245)
    parser.add_argument('--out',required=True,type=Path)
    parser.add_argument('--coverage',action='store_true')
    parser.add_argument('--coverage-summary',action='store_true',help='Export coverage snapshots without a raw event journal; useful for bounded gap scouting.')
    parser.add_argument('--native-only',action='store_true',help='Require an armed strict runtime, zero interpreter/replay work and a positive execution summary; old executables cannot qualify.')
    parser.add_argument('--campaign-warps',type=Path,help='Development-only scheduled real-descriptor warps; frozen privately and recorded as cheats')
    parser.add_argument('--no-present-log',action='store_true',help='Performance-only: omit per-field CRC/luma logging and fflush; default qualification keeps the log')
    parser.add_argument('--live-input',action='store_true',help='Opt in to merging physical keyboard/gamepads into the script; default uses only scripted guest input')
    parser.add_argument('--input-trace',action='store_true',help='Log actual guest input words for isolation checks')
    parser.add_argument('--visible',action='store_true',help='Use the real SDL display for fullscreen screenshot checks; saves/config remain isolated')
    parser.add_argument('--offscreen',action='store_true',help='Real GPU in a hidden non-fullscreen window; no desktop display changes')
    parser.add_argument('--real-audio',action='store_true',help='Use the actual audio device for an isolated visible/offscreen performance check instead of dummy audio')
    parser.add_argument('--display-screenshot-range',nargs=2,type=int,metavar=('FIRST','LAST'),help='Read back a bounded range of actual GPU frames; requires visible or offscreen')
    parser.add_argument('--display-screenshot-frame',type=int,help='Capture the actual SDL output at this frame; requires --visible or --offscreen')
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
    native_overrides=[x.partition('=')[2]for x in args.env if x.partition('=')[0]=='SNESRECOMP_NATIVE_ONLY']
    if len(native_overrides)>1:parser.error('Specify SNESRECOMP_NATIVE_ONLY only once')
    if args.native_only:
        if native_overrides and native_overrides!=['1']:parser.error('--native-only conflicts with a disabled native-only override')
        if not native_overrides:args.env.append('SNESRECOMP_NATIVE_ONLY=1')
    if args.coverage and args.coverage_summary:parser.error('Choose coverage journal OR summary-only capture')
    if args.visible and args.offscreen:parser.error('Choose visible OR offscreen')
    if args.real_audio and not (args.visible or args.offscreen):parser.error('--real-audio requires --visible or --offscreen')
    if args.campaign_warps and (args.visible or args.live_input):
        parser.error('Campaign warps require an isolated scripted run without a visible window or physical input')
    if args.no_present_log and args.screenshot_range:
        parser.error('--no-present-log cannot use --screenshot-range, which creates its own per-field CRC log')
    if (args.display_screenshot_frame or args.display_screenshot_range) and not (args.visible or args.offscreen):
        parser.error('Display capture requires --visible or --offscreen')
    repo=Path(__file__).resolve().parents[1];rom=repo.parent/'Lagoon (USA).sfc'
    exe=args.exe.resolve();route=args.route.resolve();out=args.out.resolve()
    manifest=args.manifest.resolve() if args.manifest else repo/'src/gen/program_manifest.json'
    if not manifest.is_file():
        parser.error('Matching program manifest does not exist: '+str(manifest))
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
            if original.is_file() and (original.suffix.lower() in {'.sav','.srm','.bak'}
                                       or original.name == 'lagoon-exploration.map'):
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
    if args.real_audio:env.pop('SDL_AUDIODRIVER',None)
    if not args.no_present_log:
        env['SNESRECOMP_PRESENT_LOG']=str(out/'presents.csv')
    if args.input_trace:env['SNESRECOMP_INPUT_TRACE']='1'
    if args.visible or args.offscreen:env.pop('SDL_VIDEODRIVER',None)
    else:env['SDL_VIDEODRIVER']='dummy'
    if args.offscreen:env['SNESRECOMP_HIDDEN_WINDOW']='1'
    if args.coverage:env.update(SNESRECOMP_TIER2_CAPTURE='1',SNESRECOMP_COVERAGE_DIR=str(out/'coverage'))
    if args.coverage_summary:
        env.update(SNESRECOMP_TIER2_CAPTURE='1',SNESRECOMP_COVERAGE_DIR=str(out/'coverage'),
                   SNESRECOMP_TIER2_MANIFEST=str(out/'coverage/final.json'),
                   SNESRECOMP_TIER2_JOURNAL=os.devnull)
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
    diagnostic_paths={'SNESRECOMP_NATIVE_BLOCK_COVERAGE','LAGOON_NATIVE_TRACE69','LAGOON_NATIVE_ENDPOINT69','LAGOON_WIDE_ENTITY_REPORT','LAGOON_SOUNDTRACK_PROBE','LAGOON_FRAME_PROBE',
                      'LAGOON_SCENE_RENDERER_REPORT','LAGOON_TRANSITION_REPORT','LAGOON_SWORD_REPORT',
                      'LAGOON_NATIVE_RECOVERY_REPORT','LAGOON_FOLLOWER_REPORT',
                      'SNESRECOMP_APURAM_TRACE_FILE','SNESRECOMP_DSPREG_TRACE_FILE','SNESRECOMP_DSPOUT',
                      'SNESRECOMP_APU_QUEUE_TRACE',
                      'SNESRECOMP_FRAME_TIMING','SNESRECOMP_GL_TIMING','SNESRECOMP_TITLE_OVERLAY_DUMP',
                      'SNESRECOMP_TITLE_OVERLAY_DISPLAY_SCREENSHOT'}
    diagnostic_numbers={'SNESRECOMP_HOST_PROFILE_START_FRAME','SNESRECOMP_PPU_HEARTBEAT','LAGOON_MAP_SELFTEST_FRAME'}
    diagnostic_booleans=DIAGNOSTIC_BOOLEANS
    diagnostic_ranges=DIAGNOSTIC_RANGES
    overrides={}
    for spec in args.env:
        key,sep,value=spec.partition('=')
        if not sep or key not in diagnostic_paths|diagnostic_numbers|diagnostic_booleans|diagnostic_ranges.keys()|{'LAGOON_WIDE_ASPECT','LAGOON_SCENE_RENDERER','LAGOON_ACTOR_COMPUTE','LAGOON_ACTOR_COMMANDS','LAGOON_ACTOR_COMMANDS_REPORT','LAGOON_NATIVE_ITERATOR52','LAGOON_NATIVE_SCRIPTS53','LAGOON_NATIVE_BOOT62','LAGOON_NATIVE_DECODE63','LAGOON_NATIVE_SPC63','LAGOON_NATIVE_RESET63','LAGOON_CAMPAIGN_FIXTURE_ROOM','LAGOON_SCRIPT_FIXTURE','SNESRECOMP_LLE_INTERP_TARGET','LAGOON_NATIVE_ENDPOINT69_PC'}:
            raise SystemExit('Unsupported diagnostic override: '+spec)
        if key=='SNESRECOMP_DSPOUT_DRAIN' and value=='1':
            validate_offline_dsp_drain(args.real_audio,args.config.read_text(encoding='utf-8-sig') if args.config else '',args.env)
        if key in diagnostic_paths:
            p=Path(value);p=(p if p.is_absolute() else out/p).resolve()
            if not p.is_relative_to(out) or p.exists():raise SystemExit('Diagnostic outputs must be fresh paths within --out')
            p.parent.mkdir(parents=True,exist_ok=True);value=str(p)
        elif key in diagnostic_booleans:
            validate_diagnostic_boolean(key,value)
        elif key in diagnostic_numbers:
            validate_diagnostic_number(value)
        elif key in diagnostic_ranges:
            validate_diagnostic_range(key,value)
        elif key in {'SNESRECOMP_LLE_INTERP_TARGET','LAGOON_NATIVE_ENDPOINT69_PC'}:
            if not re.fullmatch(r'[0-9a-fA-F]{6}',value):
                raise SystemExit('Interpreter control target must be one exact six-digit hexadecimal PC')
            value=value.upper()
        elif key=='LAGOON_SCRIPT_FIXTURE':
            if value not in {'30','39','text'}:raise SystemExit('Script fixture must be exactly 30, 39 or text')
        elif key=='LAGOON_CAMPAIGN_FIXTURE_ROOM':
            validate_fixture_room(value,campaign_warps=args.campaign_warps,visible=args.visible,
                live_input=args.live_input,real_audio=args.real_audio,frames=args.frames)
        elif key=='LAGOON_ACTOR_COMMANDS_REPORT':
            if value not in {'0','1'}:raise SystemExit('Report must be 0 or 1')
        elif key in {'LAGOON_SCENE_RENDERER','LAGOON_ACTOR_COMPUTE','LAGOON_ACTOR_COMMANDS','LAGOON_NATIVE_ITERATOR52','LAGOON_NATIVE_SCRIPTS53','LAGOON_NATIVE_BOOT62','LAGOON_NATIVE_DECODE63','LAGOON_NATIVE_SPC63','LAGOON_NATIVE_RESET63'}:
            choices={'off','verify','auto'} if key=='LAGOON_SCENE_RENDERER' else {'off','on'} if key in {'LAGOON_NATIVE_DECODE63','LAGOON_NATIVE_SPC63','LAGOON_NATIVE_RESET63'} else {'off','on','auto'}
            if value not in choices:
                raise SystemExit('Unsupported mode for '+key+': '+value)
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
    receipt['coverage_mode']='summary'if args.coverage_summary else 'journal'if args.coverage else None
    receipt['present_log_enabled']=not args.no_present_log
    receipt['present_log_path']=str(out/'presents.csv') if not args.no_present_log else None
    receipt.update(enabled_features=sorted(enabled),enabled_package_features=sorted(selected),
                   diagnostic_overrides=overrides,private_mod_state=state,private_working_directory=str(host))
    receipt['script_fixture_input']=({'source_pc':'01CC9A','selector':overrides['LAGOON_SCRIPT_FIXTURE'],'script_alias':{'30':'05C6AF','39':'05CAB1','text':'05C45B'}[overrides['LAGOON_SCRIPT_FIXTURE']],'writes':'A1/A2 only; once per state generation after real NPC interaction; original bank/dispatcher/handlers unchanged'} if 'LAGOON_SCRIPT_FIXTURE' in overrides else None)
    receipt['seeded_save_files']=seeded
    receipt['campaign_warps']=campaign_plan
    receipt['campaign_setup_writes']=setup_writes
    receipt['visible_display']=args.visible
    receipt['offscreen_gpu']=args.offscreen
    receipt['real_audio_device']=args.real_audio
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
    stderr_text=(out/'stderr.log').read_text(encoding='utf-8',errors='replace')
    receipt['runtime_failures']=runtime_failure_lines(stderr_text)
    receipt['native_only']=native_only_evidence(stderr_text,overrides.get('SNESRECOMP_NATIVE_ONLY')=='1')
    receipt['native_backend']=native_backend_evidence(stderr_text,overrides.get('LAGOON_NATIVE_BACKEND69')=='1',receipt['native_only'])
    receipt['native_block_coverage']=native_block_coverage_evidence(overrides.get('SNESRECOMP_NATIVE_BLOCK_COVERAGE'),stderr_text,receipt['native_only'])
    capture=receipt['native_block_coverage'];backend=receipt['native_backend']
    if capture and capture['passed'] and backend and backend['passed']:
        name,profile,entries,source=backend['registrations'][0]
        if not any(p['name']==name and p['profile']==int(profile) and p['compiled_key_count']==int(entries) and p['original_source_sha256']==source and p['hits_total']>0 for p in capture['programs']):
            capture['passed']=False;capture['errors'].append('selected native program is absent from coverage')
    receipt['qualification_passed']=not result.returncode and bool(receipt['dump_tags']) and not receipt['runtime_failures'] and (receipt['native_only'] is None or receipt['native_only']['passed']) and (receipt['native_backend'] is None or receipt['native_backend']['passed'])
    if receipt['native_block_coverage'] is not None:receipt['qualification_passed']=receipt['qualification_passed'] and receipt['native_block_coverage']['passed']
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
    summary={k:receipt[k] for k in ('exe_sha256','exit_code','elapsed_seconds')}
    summary['dump_count']=len(receipt['dump_tags'])
    print(json.dumps(summary,indent=2))
    if not receipt['qualification_passed']:raise SystemExit('route failed, reported a guest execution failure, or produced no dump evidence')


if __name__=='__main__':main()
