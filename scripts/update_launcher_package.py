"""Update only a managed launcher and help in an audited existing release.

No game rebuild: retain engine identity and record its original source separately.
Streams archives so the bundled, owner-authorized album need not be extracted.
"""
import argparse
import hashlib
import io
import json
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
    parser.add_argument('--version', required=True)
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
                    'docs/PLAYING.md': (root / 'docs/PLAYING.md').read_bytes()}
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
    for name, data in replacements.items():
        contract['files'][name] = sha(data)
    contract['engine_source_commit'] = contract.get('engine_source_commit', contract['source_commit'])
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
                      'game_sha256': contract['game_sha256'], 'scope': 'Audited input archive; unchanged native engine and runtime; managed launcher and setup/help update only.'}))


if __name__ == '__main__':
    main()
