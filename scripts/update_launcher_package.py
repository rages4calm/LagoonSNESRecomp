"""Update only a managed launcher and help in an audited existing release.

No game rebuild: retain engine identity and record its original source separately.
Streams archives so the bundled, owner-authorized album need not be extracted.
"""
import argparse
import hashlib
import io
import json
import sys
from pathlib import Path, PurePosixPath
import tarfile
import zipfile


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--launcher', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source-commit', required=True)
    parser.add_argument('--engine-source-commit', required=True,
                        help='Original engine source from the verified input release manifest')
    parser.add_argument('--version', required=True)
    parser.add_argument('--include-x68000', action='store_true',
                        help='Include the ROM-free X68000 setup/runtime sources in the Windows release')
    parser.add_argument('--launcher-exe', type=Path,
                        help='Matching newly published Windows Companion apphost')
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Output must be fresh')
    root = Path(__file__).resolve().parents[1]
    windows = args.archive.suffix == '.zip'
    contract_name = 'windows-preview.json' if windows else 'portable-package.json'
    prefix = '' if windows else 'Lagoon/'
    actual = {}
    contract = None
    forbidden = {'.sfc', '.smc', '.srm', '.sav', '.state', '.maps', '.vgz', '.vgm', '.bps', '.c', '.pdb'}

    def admit(name, stream):
        nonlocal contract
        if not name.startswith(prefix):
            raise ValueError('Unexpected archive root')
        name = name[len(prefix):]
        path = PurePosixPath(name)
        if '\\' in name or path.is_absolute() or '..' in path.parts or name in actual or path.suffix.lower() in forbidden or name.startswith(('local/', 'src/gen/')):
            raise ValueError('Private or unsafe archive entry: ' + name)
        digest = hashlib.sha256()
        data = bytearray()
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
            if name == contract_name:
                data.extend(chunk)
                if len(data) > 2 * 1024 * 1024:
                    raise ValueError('Oversized contract')
        actual[name] = digest.hexdigest()
        if name == contract_name:
            contract = json.loads(data)

    if windows:
        with zipfile.ZipFile(args.archive) as source:
            for entry in source.infolist():
                if not entry.is_dir():
                    with source.open(entry) as stream:
                        admit(entry.filename, stream)
    else:
        with tarfile.open(args.archive, 'r|gz') as source:
            for entry in source:
                if entry.isdir():
                    continue
                if not entry.isfile():
                    raise ValueError('Links are not release inputs')
                admit(entry.name, source.extractfile(entry))
    assert contract and contract['playable'] and contract['native_cpu'] and contract['native_only_default']
    assert actual == dict(contract['files'], **{contract_name: actual[contract_name]})
    assert actual['game/LagoonSNESRecomp' + ('.exe' if windows else '')] == contract['game_sha256']
    assert 'lagoon.Companion.dll' in actual
    dll = args.launcher.read_bytes()
    assert dll[:2] == b'MZ' and len(dll) < 2 * 1024 * 1024
    replacements = {'lagoon.Companion.dll': dll,
                    'docs/RECORDING.md': (root / 'docs/RECORDING.md').read_bytes(),
                    'docs/PLAYING.md': (root / 'docs/PLAYING.md').read_bytes(),
                    'docs/BUILDING.md': (root / 'docs/BUILDING.md').read_bytes(),
                    'CREDITS.md': (root / 'CREDITS.md').read_bytes()}
    help_text = ('Lagoon Companion ' + args.version + '\n\n'
                 'Open the Companion launcher. On first launch choose your original USA\n'
                 '.sfc/.smc ROM FILE with Choose ROM file. Verification and setup are automatic.\n'
                 'Extract ZIP archives first; do not choose a folder. Your ROM stays where it is.\n'
                 'Choose ROM.cmd / Choose ROM.command reopens the picker later.\n\n'
                 'Display > Window mode > Borderless fullscreen fills your monitor.\n'
                 'OBS: Game Capture > Capture specific window > LagoonSNESRecomp.exe.\n'
                 'See docs/RECORDING.md and docs/PLAYING.md.\n\n'
                 'This launcher update retains the qualified v0.4.0 game engine.\n'
                 'Existing saves and settings remain in local/. No ROM is bundled.\n'
                 'https://github.com/rages4calm/LagoonSNESRecomp\n')
    replacements['README.txt'] = help_text.encode()
    chooser = 'Choose ROM.cmd' if windows else 'Choose ROM.command'
    replacements[chooser] = (('@echo off\r\nstart "" "%~dp0lagoon.Companion.exe" --rom-setup\r\n') if windows else
        '#!/bin/sh\ncd -- "$(dirname -- "$0")" || exit 1\nexec ./lagoon.Companion --rom-setup\n').encode()
    if args.include_x68000:
        if not windows or args.launcher_exe is None:
            raise ValueError('The combined release requires Windows and its matching launcher apphost')
        # Reuse the qualified .NET/runtime payload only when the new publish has
        # exactly the same dependency/runtime contracts. Never silently omit a
        # newly required runtime or dependency.
        with zipfile.ZipFile(args.archive) as original:
            for name in ('lagoon.Companion.deps.json', 'lagoon.Companion.runtimeconfig.json'):
                if original.read(name) != (args.launcher.parent / name).read_bytes():
                    raise ValueError('Managed dependency contract changed; publish the complete runtime: ' + name)
        apphost = args.launcher_exe.read_bytes()
        if apphost[:2] != b'MZ' or len(apphost) > 2 * 1024 * 1024 or b'lagoon.Companion.dll' not in apphost:
            raise ValueError('Expected the matching ROM-free Companion apphost')
        replacements['lagoon.Companion.exe'] = apphost
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        from package_x68000 import public_inputs
        for source, relative in public_inputs(root).items():
            path = PurePosixPath(relative)
            if path.is_absolute() or '..' in path.parts or '\\' in relative or path.suffix.lower() not in {'.py', '.inc', '.h', '.json', '.md', '.txt', '.cmd', '.ps1'}:
                raise ValueError('Unexpected X68000 public setup file: ' + relative)
            source = Path(source)
            if not source.resolve().is_relative_to(root.resolve()) or source.is_symlink():
                raise ValueError('X68000 setup input must be a regular public source file')
            replacements[relative] = source.read_bytes()
        replacements['Setup Lagoon.cmd'] = b'@echo off\r\nstart "" "%~dp0lagoon.Companion.exe" --game ask\r\n'
        replacements.pop('README.txt', None)
        replacements['READ ME.txt'] = ('''LAGOON COMPANION ''' + args.version + ''' - SNES + X68000 (Windows)

1. Extract the entire ZIP into a writable folder.
2. Open lagoon.Companion.exe (or Setup Lagoon.cmd).
3. Choose SNES Enhanced or X68000 Enhanced.

SNES: choose your clean Lagoon USA .sfc/.smc ROM file. Setup is automatic;
no compiler, Python or download is needed. The qualified SNES engine is included.

X68000: choose your Lagoon game ZIP (or extracted disk folder). Click Set up
game, then Play. Compatible firmware is downloaded automatically; Advanced
also accepts your own firmware. You do not need to extract the game ZIP,
install Python/build tools, type commands or use an administrator account.
Setup downloads verified portable tools and builds
the game on your PC. Internet is needed for the first setup; keep the window
open until setup finishes. Later launches do not rebuild the game.

Supply your own game files. No game disks, game ROM or prebuilt X68000
executable containing game disks is included in this download. The compatible
IPL and replacement font are downloaded with their separate notices/terms.
This release includes the fully native SNES CPU port and the enhanced X68000
edition using RetroRecomp's runtime. Their stories and controls differ.

SNES defaults: Z / controller A sword; X / B jump; S / Y talk.
M / L2 map; Ctrl+comma / R2 Options; Ctrl+Z / R3 zoom.
F1-F10 load state; Shift+F1-F10 save state. F11 state browser. Alt+Enter fullscreen.

X68000 defaults: hold S / A and move into enemies to attack; D / B jump,
talk and confirm original menus. I / LB Items; E / RB Equipment; Y Save/Load.
Ctrl+M / R3 Atlas; Ctrl+Z / LT zoom; Ctrl+comma / RT Settings.
F8 save state; F9 load state (Ctrl+F8/F9 when guest function keys are enabled).
Giles no longer needs a Potion; there is no escort/tablet hunt from the SNES game.

See docs/PLAYING.md for the full controls, features and controller navigation.
Keep local/ (SNES) and local-x68000/ (X68000) for your saves/settings.
Use Check for updates in Companion for future Windows package updates.
Close both games first. The updater preserves your generated X68000 game,
ROM selections, settings and saves. It does not rebuild the game for a
launcher-only update. Update X68000 game is a separate action when its
runtime changes; it reuses verified local build inputs where available.
If an older installation no longer has those inputs, select your game ZIP
once to restore them. Source checkouts use the documented developer workflow.

Source, downloads and help: https://github.com/rages4calm/LagoonSNESRecomp
''').encode('utf-8')
        contract['package'] = 'Lagoon-Companion-' + args.version + '-win-x64'
        contract['editions'] = {
            'snes': {'setup': 'import-own-rom', 'prebuilt_engine': True},
            'x68000': {'setup': 'automatic-portable-build-from-own-inputs',
                        'prebuilt_engine': False, 'auto_download_tools': True,
                        'administrator_required': False}}
        help_text = replacements['READ ME.txt'].decode('utf-8')
    for name, data in replacements.items():
        contract['files'][name] = sha(data)
    recorded_engine = contract.get('engine_source_commit', contract.get('source_commit'))
    if recorded_engine is not None and recorded_engine != args.engine_source_commit:
        raise ValueError('Engine source differs from the input release manifest')
    if len(args.engine_source_commit) != 40 or any(c not in '0123456789abcdef' for c in args.engine_source_commit):
        raise ValueError('Engine source must be a full commit identity')
    contract['engine_source_commit'] = args.engine_source_commit
    contract['engine_version'] = contract.get('engine_version', contract['version'])
    contract['source_commit'] = args.source_commit
    contract['version'] = args.version
    replacements[contract_name] = (json.dumps(contract, indent=2) + '\n').encode()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if windows:
        with zipfile.ZipFile(args.archive) as source, zipfile.ZipFile(args.output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as target:
            for entry in source.infolist():
                if entry.is_dir() or entry.filename in replacements:
                    continue
                with source.open(entry) as incoming, target.open(entry.filename, 'w') as outgoing:
                    while chunk := incoming.read(1024 * 1024):
                        outgoing.write(chunk)
            for name, data in replacements.items():
                target.writestr(name, data)
    else:
        with tarfile.open(args.archive, 'r|gz') as source, tarfile.open(args.output, 'w|gz', compresslevel=6) as target:
            for entry in source:
                if entry.isdir() or entry.name[len(prefix):] in replacements:
                    continue
                target.addfile(entry, source.extractfile(entry))
            for name, data in replacements.items():
                entry = tarfile.TarInfo(prefix + name)
                entry.mode = 0o755 if name == chooser else 0o644
                entry.size = len(data)
                target.addfile(entry, io.BytesIO(data))
    digest = hashlib.file_digest(args.output.open('rb'), 'sha256').hexdigest()
    args.output.with_name(args.output.name + '.sha256').write_text(digest + '  ' + args.output.name + '\n')
    print(json.dumps({'passed': True, 'version': args.version, 'archive': args.output.name,
                      'sha256': digest, 'bytes': args.output.stat().st_size,
                      'source_commit': args.source_commit, 'engine_source_commit': contract['engine_source_commit'],
                      'game_sha256': contract['game_sha256'], 'scope': 'Audited input archive; unchanged native SNES engine and bundled .NET runtime; managed launcher, generic X68000 sources and setup/help updated.'}))


if __name__ == '__main__':
    main()
