"""Stage the reviewed playable Windows preview, with no ROM/private game data.

Explicit runtime inputs only. Never recursively copy a developer build or SDK.
Output must be fresh. Audit and clean-unpacked test before publishing a ZIP.
"""
from __future__ import annotations
import argparse,hashlib,json,os
from pathlib import Path
import shutil,subprocess
import re

ROOT=Path(__file__).resolve().parents[1]
def digest(path:Path)->str: return hashlib.sha256(path.read_bytes()).hexdigest()

def achievement_cameo_inputs(root:Path=ROOT)->dict[str,str]:
    """Audit the small, explicit public additions before creating a stage.

    Progress, profiles, ROMs and whole sprite sheets can never enter this list.
    The embedded artwork is separately copyrighted; preserve its provenance.
    """
    catalog=root/'assets/achievements/catalog.json'
    if catalog.stat().st_size>32768: raise ValueError('Oversized achievement catalog')
    data=json.loads(catalog.read_text(encoding='utf-8'))
    if not isinstance(data,dict) or type(data.get('schemaVersion')) is not int or data['schemaVersion']!=1 or data.get('gameId')!='lagoon' or data.get('editionId')!='native-qol' or type(data.get('catalogVersion')) is not int or data['catalogVersion']!=1:
        raise ValueError('Unsupported Lagoon achievement catalog identity/schema')
    rows=data.get('achievements');ids=set()
    if not isinstance(rows,list) or len(rows)!=26: raise ValueError('Expected the reviewed 26-entry achievement catalog')
    for row in rows:
        identifier=row.get('id') if isinstance(row,dict) else None
        if not isinstance(identifier,str) or not re.fullmatch(r'[a-z0-9._-]{1,80}',identifier) or identifier in ids:
            raise ValueError('Invalid or duplicate achievement ID')
        ids.add(identifier)
        if any(not isinstance(row.get(k),str) or not row[k] for k in ('title','description','event')) or row.get('rule') not in ('milestone','counter') or type(row.get('target')) is not int or not 0<row['target']<=0xffffffff:
            raise ValueError('Invalid achievement definition: '+identifier)
    if not {'story.finished','secret.wrong-game'}.issubset(ids): raise ValueError('Missing reviewed ending/cameo achievement')
    selected={str(catalog):'game/assets/achievements/catalog.json'}
    provenance=root/'third_party/companion_achievements/provenance.json'
    record=json.loads(provenance.read_text(encoding='utf-8'))
    for folder,key,names in [('companion_achievements','core_files',{'companion_achievements.c','companion_achievements.h','LICENSE'}),('cjson','cjson_files',{'cJSON.c','cJSON.h','LICENSE'})]:
        hashes=record.get(key)
        if not isinstance(hashes,dict) or set(hashes)!=names: raise ValueError('Incomplete achievement dependency provenance: '+folder)
        for name,expected in hashes.items():
            if digest(root/'third_party'/folder/name)!=expected: raise ValueError('Achievement dependency differs from provenance: '+folder+'/'+name)
    selected[str(provenance)]='licenses/CompanionAchievements-provenance.json'
    selected[str(root/'third_party/companion_achievements/LICENSE')]='licenses/CompanionAchievements-LICENSE'
    selected[str(root/'third_party/cjson/LICENSE')]='licenses/cJSON-LICENSE'
    assets=root/'assets/easter_egg';provenance=assets/'provenance.json'
    record=json.loads(provenance.read_text(encoding='utf-8'))
    names={'Crono - Left Arm (Front).gif','Crono - Right Arm (Front).gif','Crono - Arms Up (Front).gif','Crono - Status.gif'}
    sources=record.get('source_files',[])
    if not isinstance(sources,list) or len(sources)!=4 or {p.get('file') for p in sources if isinstance(p,dict)}!=names:
        raise ValueError('Only the four reviewed Crono source GIFs may be packaged')
    for source in sources:
        path=assets/source['file']
        if not 0<path.stat().st_size<=4096 or path.stat().st_size!=source.get('bytes') or digest(path)!=source.get('sha256'): raise ValueError('Cameo asset differs from provenance: '+path.name)
        selected[str(path)]='game/assets/easter_egg/'+path.name
    header=assets/'crono_dance.generated.h'
    if not 0<header.stat().st_size<=16384 or digest(header)!=record.get('generated_header_sha256'): raise ValueError('Cameo header differs from provenance')
    selected[str(header)]='game/assets/easter_egg/'+header.name
    portrait=assets/'crono_portrait.generated.h'
    if not 0<portrait.stat().st_size<=16384 or digest(portrait)!=record.get('portrait_header_sha256'): raise ValueError('Cameo portrait differs from provenance')
    selected[str(portrait)]='game/assets/easter_egg/'+portrait.name
    sound=assets/'B1.wav'; sound_record=record.get('completion_sound',{})
    if sound_record.get('file')!='B1.wav' or sound.stat().st_size!=495964 or sound_record.get('bytes')!=495964 or digest(sound)!=sound_record.get('sha256') or digest(sound)!='0dc7bc26a8547bb5c1e533afe363823bc89de74df3effa229315dce0e0901d6c': raise ValueError('Cameo sound differs from reviewed owner input')
    selected[str(sound)]='game/assets/easter_egg/B1.wav'
    selected[str(provenance)]='game/assets/easter_egg/provenance.json'
    return selected

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--stage',type=Path,required=True)
    ap.add_argument('--launcher',type=Path,required=True,help='Self-contained win-x64 Companion publish directory')
    ap.add_argument('--game-exe',type=Path,required=True,help='Final qualified native game EXE; no ROM is embedded')
    ap.add_argument('--generated-manifest',type=Path,required=True,help='LOCAL qualification manifest; only SHA is distributed')
    ap.add_argument('--package-id',required=True)
    ap.add_argument('--companion-root',type=Path,required=True)
    ap.add_argument('--toolchain',type=Path,required=True,help='Qualification toolchain for static dependency license notices only')
    ap.add_argument('--soundtrack-root',type=Path,required=True,help='Immutable complete 40-track X68000 MSU1 PCM pack; staged by hardlink')
    ap.add_argument('--nuget-packages',type=Path,default=Path(os.environ.get('NUGET_PACKAGES',Path.home()/'.nuget/packages')))
    a=ap.parse_args();stage=a.stage.resolve()
    if stage.exists(): raise SystemExit('Stage must not exist. Existing work is preserved.')
    if not a.game_exe.is_file() or not (a.launcher/'lagoon.Companion.exe').is_file() or not (a.launcher/'coreclr.dll').is_file(): raise SystemExit('Qualified game EXE and self-contained Companion are required.')
    selected={str(ROOT/n):n for n in ['Setup Lagoon.cmd','scripts/setup_windows.ps1','scripts/preview-default.ini','rom_identity.txt','LICENSE','CREDITS.md','docs/PLAYING.md','docs/SOUNDTRACK.md']}
    selected.update(achievement_cameo_inputs())
    selected[str(a.game_exe)]='game/LagoonSNESRecomp.exe'
    audio=set()
    for name in ['lagoon-x68000.msu','pack.json']+[f'lagoon-x68000-{n}.pcm' for n in range(1,41)]:
        source=str(a.soundtrack_root/name);selected[source]='soundtrack/x68000/'+name
        if name!='pack.json': audio.add(source)
    for n in ['lagoon-game.ico','lagoon-companion.ico']:
        selected[str(ROOT/'assets/icons'/n)]='icons/'+n
    for n in ['lagoon-epx.glsl','lagoon-epx.glslp']:
        selected[str(ROOT/'assets/shaders'/n)]='game/assets/shaders/'+n
    for package in ['lagoon.quality','lagoon.soundtrack']:
        rel=f'mods/preloaded/packages/{package}/1.0.0/manifest.toml'
        selected[str(ROOT/rel)]='game/'+rel
    # Only public redistributables from this fresh publish output; no PDBs,
    # settings, saves, downloaded content, arbitrary EXEs or developer paths.
    for p in a.launcher.iterdir():
        if p.is_file() and (p.suffix.lower()=='.dll' or p.name in {'lagoon.Companion.exe','createdump.exe','lagoon.Companion.deps.json','lagoon.Companion.runtimeconfig.json'}): selected[str(p)]=p.name
    for source,rel in [(ROOT/'launcher/LICENSE','licenses/Lagoon-Companion-LICENSE'),(a.companion_root/'LICENSE','licenses/CompanionLauncher-LICENSE'),(ROOT/'snesrecomp/LICENSE','licenses/SNESRecomp-LICENSE'),(ROOT/'snesrecomp/THIRD_PARTY_ATTRIBUTION.md','licenses/SNESRecomp-THIRD_PARTY_ATTRIBUTION.md'),(a.toolchain/'deps/share/licenses/SDL3/LICENSE.txt','licenses/SDL3-LICENSE.txt')]: selected[str(source)]=rel
    selected[str(a.toolchain/'LICENSE.TXT')]='licenses/LLVM-LICENSE.TXT'
    for p in (a.toolchain/'x86_64-w64-mingw32/share/mingw32').glob('COPYING*'):
        if p.is_file(): selected[str(p)]='licenses/MinGW/'+p.name
    sdk_files=subprocess.check_output(['git','-C',str(ROOT/'snesrecomp'),'ls-files','--recurse-submodules','-z']).decode().split('\0')
    for name in sdk_files:
        p=Path(name)
        if p.name.upper().startswith(('LICENSE','COPYING','NOTICE')) and (ROOT/'snesrecomp'/p).is_file(): selected[str(ROOT/'snesrecomp'/p)]='licenses/framework/'+p.as_posix()
    runtime=json.loads((a.launcher/'lagoon.Companion.runtimeconfig.json').read_text())['runtimeOptions']['includedFrameworks']
    for framework in runtime:
        folder=a.nuget_packages/(framework['name'].lower()+'.runtime.win-x64')/framework['version']
        notices=[p for p in folder.iterdir() if p.is_file() and p.name.upper().startswith(('LICENSE','THIRD-PARTY-NOTICES'))]
        if not notices: raise SystemExit(f'Missing runtime license notices: {folder}')
        for p in notices: selected[str(p)]=f"licenses/{framework['name']}/{p.name}"
    for p in selected:
        if not Path(p).is_file(): raise SystemExit('Missing release input: '+p)
    stage.mkdir(parents=True);hashes={}
    for source,relative in sorted(selected.items(),key=lambda x:x[1]):
        target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True)
        if source in audio: os.link(source,target) # Immutable inputs; never alter staged music.
        else: shutil.copy2(source,target)
        hashes[relative]=digest(target)
    # Retain every imported track/hash/loop field, while distinguishing current
    # runtime cue qualification from the original import's historical status.
    metadata=json.loads((a.soundtrack_root/'pack.json').read_text(encoding='utf-8'))
    metadata['original_import_metadata_sha256']=digest(a.soundtrack_root/'pack.json')
    metadata['game_mapping_status']='17 source-qualified named SNES music cues; unknown cues retain SNES audio. All 40 imported tracks are included; not all tracks have a known game cue.'
    metadata['cue_mapping_source_sha256']=digest(ROOT/'src/lagoon_soundtrack.c')
    (stage/'soundtrack/x68000/pack.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    hashes['soundtrack/x68000/pack.json']=digest(stage/'soundtrack/x68000/pack.json')
    # The provider puts zlib's complete notice in its public header.
    zlib=(a.toolchain/'deps/include/zlib.h').read_text(encoding='utf-8').split('*/',1)[0]+'*/\n'
    (stage/'licenses/zlib-LICENSE.txt').write_text(zlib,encoding='utf-8');hashes['licenses/zlib-LICENSE.txt']=digest(stage/'licenses/zlib-LICENSE.txt')
    text='''LAGOON COMPANION — WINDOWS PREVIEW

This playable work in progress requires Windows 10/11 x64 and your own clean,
unheadered Lagoon USA ROM (1 MiB). No ROM, extracted Lagoon graphics, saves or
ROM-derived generated C source are included. The native game and self-contained Companion
EXEs are included. The interpreter safety net remains enabled.

1. Extract the entire ZIP into a writable folder.
2. Open Setup Lagoon.cmd (or lagoon.Companion.exe).
3. On Game, enter your ROM's complete path and click Apply.
4. Open Game setup and Run setup, then press Play.

No compiler, Python, .NET installation, first-run compilation or download is
needed. Your ROM is checked and read in place; it is never copied into the pack.
The game EXE is game/LagoonSNESRecomp.exe. Companion supplies its ROM, settings
and save paths. Keep local/ for your settings, mod preferences, saves and map.
Extract a new preview into a separate folder and keep your previous saves.

M or controller L2 opens the exploration map. Esc/B closes it; left/right select
visited areas. Early tablet objectives include next-step hints and map markers.
R2 or Ctrl+comma opens quick settings: master volume and X68000 music volume.
Select Achievements there and press A/Y or Enter, or use Ctrl+J directly.
The 26-entry achievement browser is also available in Companion. Progress is
offline and local to this package's saves, independent of game saves/states.
Unlocks have a short notification and original chime, following master volume.
Loading a save does not retroactively award achievements; assisted diagnostic runs
do not award progress. A hidden traveler has a spoiler-safe locked entry.
The game pauses while these panels are open. No return to the launcher is needed
to adjust those volumes. Companion also edits keyboard/controller/system hotkeys.
Alt+Enter toggles fullscreen. F1 loads slot1; Shift+F1 saves slot1.
Quality of life has independent EXP and gold reward selectors: Original or 2x through 8x;
EXP also retains the previous 1.5x option. Multipliers do not stack and do not alter
shop prices, quest rewards or your existing saved totals.
Some widescreen/special actor paths retain the original view. Audio's Soundtrack
dropdown switches between Original SNES and the bundled X68000 music. Choose
one, Apply and restart the game. No folder selection or conversion is needed.
Complete campaign playtesting is still pending. Report room, actions/settings
and whether a new game reproduces a problem.

Source and updates: https://github.com/rages4calm/LagoonSNESRecomp
See docs/PLAYING.md for controls, CREDITS.md and licenses/ for notices.
The tiny credited Chrono Trigger cameo artwork has separate original copyright;
it is not covered by the MIT licenses for Companion/CompanionAchievements.
'''
    (stage/'READ ME.txt').write_text(text,encoding='utf-8');hashes['READ ME.txt']=digest(stage/'READ ME.txt')
    contract={'format':1,'mode':'prebuilt-own-rom','package':a.package_id,'game_sha256':digest(a.game_exe),'generated_manifest_sha256':digest(a.generated_manifest),'source_commit':subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip(),'framework_patch_sha256':digest(ROOT/'patches/lagoon-framework.patch'),'files':dict(sorted(hashes.items()))}
    (stage/'windows-preview.json').write_text(json.dumps(contract,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'stage':str(stage),'files':len(hashes)+1,'bytes':sum(p.stat().st_size for p in stage.rglob('*') if p.is_file()),'game_sha256':contract['game_sha256'],'generated_manifest_sha256':contract['generated_manifest_sha256']}))
if __name__=='__main__': main()
