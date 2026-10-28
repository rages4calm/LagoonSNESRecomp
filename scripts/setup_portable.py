#!/usr/bin/env python3
"""Verify a ROM-free Linux/macOS package and initialize only its writable local/.

The ROM is read in place, never copied. Existing saves/settings are preserved.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath


def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def safe_path(root: Path, relative: str) -> Path:
    rel = PurePosixPath(relative)
    if not relative or '\\' in relative or rel.is_absolute() or any(p in ('', '.', '..') for p in relative.split('/')) or ':' in relative:
        raise ValueError('Invalid package-relative path: ' + relative)
    path = root.joinpath(*rel.parts)
    for candidate in (root, *path.parents, path):
        if candidate.is_symlink():
            raise ValueError('Redirected package path: ' + str(candidate))
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError('Package path escapes its root')
    return path


def setup(root: Path, rom: Path) -> dict:
    root = root.absolute()
    if root.is_symlink():
        raise ValueError('Package root must not be a link')
    contract_path = safe_path(root, 'portable-package.json')
    contract = json.loads(contract_path.read_text(encoding='utf-8'))
    if (contract.get('format_version') != 1 or contract.get('mode') != 'prebuilt-own-rom'
            or contract.get('game_id') != 'lagoon' or contract.get('playable') is not True
            or contract.get('platform') not in ('linux-x64', 'linux-arm64', 'macos-x64', 'macos-arm64')
            or contract.get('executable') != 'game/LagoonSNESRecomp'):
        raise ValueError('Not a playable Lagoon Linux/macOS package')
    files = contract.get('files')
    if not isinstance(files, dict) or not files or contract['executable'] not in files:
        raise ValueError('Incomplete package contract')
    for relative, expected in files.items():
        path = safe_path(root, relative)
        if not path.is_file() or digest(path) != expected:
            raise ValueError('Package file missing or changed: ' + relative)
    identity = contract.get('rom_identity', {})
    # Fixed title identity: a modified contract cannot accept another game/patch.
    expected_rom = '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
    if identity != {'size': 1048576, 'sha256': expected_rom}:
        raise ValueError('Unsupported ROM identity')
    rom = rom.expanduser().resolve(strict=True)
    if not rom.is_file() or rom.stat().st_size != 1048576 or digest(rom) != expected_rom:
        raise ValueError('Select the clean, unheadered 1 MiB Lagoon USA ROM')
    local = safe_path(root, 'local')
    local.mkdir(exist_ok=True)
    config = safe_path(root, 'local/config.ini')
    ready_path = safe_path(root, 'local/setup-ready.json')
    # Validate all writable ancestors before creating or modifying anything below.
    for relative in ('local/saves', 'local/keybinds.ini', 'local/rom.cfg', 'local/setup-ready.tmp'):
        safe_path(root, relative)
    catalog = [(relative[5:], safe_path(root, relative)) for relative in files
               if relative.startswith('game/mods/preloaded/packages/')]
    for relative, _ in catalog:
        safe_path(root, 'local/' + relative)
    if not config.exists():
        default = safe_path(root, 'scripts/portable-default.ini')
        if 'scripts/portable-default.ini' not in files:
            raise ValueError('Package defaults missing from contract')
        with config.open('x', encoding='utf-8') as stream:
            stream.write(default.read_text(encoding='utf-8'))
    keybinds = local / 'keybinds.ini'
    if not keybinds.exists():
        keybinds.write_text('# Unspecified bindings use the game defaults.\n', encoding='utf-8')
    for relative, source in catalog:
        target = safe_path(root, 'local/' + relative)
        if not target.exists():
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write(source.read_bytes())
    (local / 'saves').mkdir(exist_ok=True)
    # The selected ROM path may change; game progress and config never do here.
    (local / 'rom.cfg').write_text(str(rom) + '\n', encoding='utf-8')
    ready = {'package': contract['package'], 'platform': contract['platform'],
             'game_sha256': files[contract['executable']],
             'generated_manifest_sha256': contract['generated_manifest_sha256'],
             'executable': str(safe_path(root, contract['executable'])),
             'config': str(config), 'rom': str(rom)}
    temporary = local / 'setup-ready.tmp'
    with temporary.open('w', encoding='utf-8') as stream:
        json.dump(ready, stream, indent=2)
        stream.write('\n')
    temporary.replace(ready_path)
    return ready


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).absolute().parent.parent)
    parser.add_argument('--rom', type=Path, required=True)
    args = parser.parse_args()
    try:
        setup(args.root, args.rom)
    except (OSError, ValueError, KeyError) as error:
        raise SystemExit('Setup stopped: ' + str(error))
    print('Setup complete. Existing settings and saves preserved. Return to Companion and press Play.')


if __name__ == '__main__':
    main()
