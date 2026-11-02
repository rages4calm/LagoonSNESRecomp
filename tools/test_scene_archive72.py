"""Qualify the bounded visited-scenery archive with an owner-local clean ROM.

Offline synthetic graphics/room fixtures; not gameplay or GPU qualification.
The emitted maps contain private owner-ROM terrain: keep --out local.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess


def main():
    repo=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--sdk',type=Path,default=repo/'snesrecomp')
    parser.add_argument('--compiler',default='clang')
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();rom=args.rom.resolve();sdk=args.sdk.resolve()/'runner/src';out=args.out.resolve()
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assert sha(rom)=='46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
    out.mkdir(parents=True,exist_ok=False)
    # The C fixture supplies an explicit UTF8 filename, independent of Windows'
    # legacy narrow argv conversion, to exercise real wide-file / POSIX APIs.
    profile=out/'profile';profile.mkdir()
    exe=out/('scene-archive72.exe' if os.name=='nt' else 'scene-archive72')
    files=[repo/'tests/lagoon_scene_archive72_test.c',sdk/'snes/interp816.c',sdk/'sha256.c',sdk/'crc32.c']
    command=[args.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-missing-field-initializers','-I'+str(repo/'src'),'-I'+str(sdk),*map(str,files),'-o',str(exe)]
    if os.name!='nt':command.append('-lm')
    built=subprocess.run(command,capture_output=True,text=True)
    (out/'build.log').write_text(built.stdout+built.stderr);built.check_returncode()
    run=subprocess.run([str(exe),str(rom),str(profile)],capture_output=True,text=True)
    (out/'test.log').write_text(run.stdout+run.stderr)
    bindings=files+[repo/'src/lagoon_scene_renderer.c',repo/'src/lagoon_scene_renderer.h',repo/'tests/lagoon_scene_renderer_test.c',repo/'tests/lagoon_scene_map72_test.c',repo/'src/lagoon_wide_entities.c',repo/'tools/test_wide_parallax_sources.c',Path(__file__)]
    receipt=dict(passed=run.returncode==0,returncode=run.returncode,rom_sha256=sha(rom),sources={str(p):sha(p)for p in bindings},stdout=run.stdout,stderr=run.stderr,
      format=dict(schema=1,header_bytes=80,record_bytes=74024,maximum_rooms=75,maximum_file_bytes=5551880,byte_order='little',identity='Clean USA SHA256 + original resource descriptors and definition SHA256; per-record/payload/header IEEE CRC32'),
      scope='All75 exact room-descriptor fixture captures, original static-page source loads, synthetic BG art; explicit committed-only capture, all fresh/offline pixels equal, independent live/offline row caches, generation retention, lazy disk merge and latest-seen updates, bounded codec, corruption/truncation/resource/foreign/late-record refusal without partial import, preserved foreign/concurrent/temp files, Unicode path, no guest/livepacket/presentation writes by archive image API.',
      limits='Offline fixture proof, not all-room gameplay/GPU/source-art qualification or target campaigns. ArchiveCache never authorizes the worldrenderer. Live host gating and user browsing/restart are verified separately.')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(run.stdout,end='');print(run.stderr,end='');run.check_returncode()


if __name__=='__main__':main()
