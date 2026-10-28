#!/usr/bin/env python3
"""Stage an actual Linux/macOS game + self-contained Companion; no ROM or generated C.

This does not publish. A setup-host compilation is deliberately refused.
Use a fresh --stage and audit/test it on its target OS before making a release.
"""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parent.parent
_spec = importlib.util.spec_from_file_location('lagoon_windows_package_inputs', ROOT / 'scripts/package_windows_preview.py')
_inputs = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_inputs)
achievement_cameo_inputs = _inputs.achievement_cameo_inputs
PLATFORMS = {'linux-x64': ('Linux', 62), 'linux-arm64': ('Linux', 183),
             'macos-x64': ('Darwin', 0x1000007), 'macos-arm64': ('Darwin', 0x100000c)}


def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def check_binary(path: Path, target: str) -> None:
    with path.open('rb') as stream:
        header = stream.read(32)
    system, machine = PLATFORMS[target]
    if system == 'Linux':
        if len(header) < 20 or header[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', header, 18)[0] != machine:
            raise ValueError('Expected a 64-bit ELF executable for ' + target + ': ' + str(path))
    elif len(header) < 8 or header[:4] != b'\xcf\xfa\xed\xfe' or struct.unpack_from('<I', header, 4)[0] != machine:
        raise ValueError('Expected a target-architecture 64-bit Mach-O executable: ' + str(path))


def launcher_files(directory: Path) -> list[Path]:
    selected = []
    for source in directory.iterdir():
        if source.is_symlink():
            raise ValueError('Linked publish inputs are unsupported: ' + str(source))
        # Optional Linux LTTng diagnostic provider, loaded with a nonfatal
        # dlopen by CoreCLR (PAL_InitializeTracing in tracepointprovider.cpp).
        # Omit this provider rather than require an obsolete tracing-library ABI
        # for an ordinary game launcher. Do not omit any other runtime library.
        if source.name == 'libcoreclrtraceptprovider.so':
            continue
        if source.is_file() and (source.suffix in ('.dll', '.so', '.dylib', '.json', '.dat')
                                 or source.name in ('lagoon.Companion', 'createdump')):
            selected.append(source)
    return selected


def dependencies(paths: list[Path], target: str) -> dict:
    """Record real loader dependencies and reject undeclared developer libraries."""
    # ldd resolves dependencies to absolute paths even when the publish directory
    # was passed relatively. Compare canonical parents on both sides so only
    # actual selected siblings qualify, never arbitrary developer directories.
    paths = [p.resolve() for p in paths]
    expected_system = PLATFORMS[target][0]
    if platform.system() != expected_system:
        raise ValueError('Package/audit binaries on their target OS, not by relabelling a Windows build')
    report = {}
    supplied = {p.name for p in paths}
    for path in paths:
        # A self-contained launcher may contain universal native libraries.
        # Inspect the target CPU slice explicitly: inspecting every fat section
        # would mix install IDs/dependencies from different architectures.
        apple_arch = 'arm64' if target == 'macos-arm64' else 'x86_64'
        tool = ['ldd', str(path)] if expected_system == 'Linux' else ['otool', '-arch', apple_arch, '-L', str(path)]
        result = subprocess.run(tool, capture_output=True, text=True, check=False)
        if result.returncode:
            raise ValueError('Dependency inspection failed for ' + str(path) + ': ' + result.stderr)
        lines = result.stdout.splitlines()
        own_install_name = None
        if expected_system == 'Darwin' and path.suffix == '.dylib':
            # otool -L includes LC_ID_DYLIB as its first entry. Its historical
            # install name need not match the redistributed filename (Avalonia
            # is one example), and is not an external load command. Obtain the
            # exact ID separately; never exempt arbitrary /usr/local libraries.
            identity = subprocess.run(['otool', '-arch', apple_arch, '-D', str(path)], capture_output=True, text=True, check=False)
            if identity.returncode:
                raise ValueError('Install-name inspection failed for ' + str(path) + ': ' + identity.stderr)
            names = [line.strip() for line in identity.stdout.splitlines()[1:] if line.strip()]
            if len(names) > 1:
                raise ValueError('Expected one Mach-O install name for target slice: ' + str(path))
            own_install_name = names[0] if names else None
        public_lines = []
        for line in lines:
            for parent in sorted({str(p.parent) for p in paths}, key=len, reverse=True):
                line = line.replace(parent + '/', '<package-native>/')
            public_lines.append(line)
        report[path.name] = public_lines
        for number, line in enumerate(lines if expected_system == 'Linux' else lines[1:]):
            line = line.strip()
            if not line:
                continue
            if 'not found' in line:
                raise ValueError('Unresolved target dependency: ' + line)
            if expected_system == 'Linux':
                if 'libSDL' in line:
                    raise ValueError('Portable game must use pinned static SDL3, not an external SDL shared library')
                resolved = line.split('=>', 1)[-1].strip().split(' ', 1)[0]
                if resolved.startswith('/') and not resolved.startswith(('/lib/', '/lib64/', '/usr/lib/', '/usr/lib64/')):
                    # Launcher-native libraries can resolve each other beside the
                    # fresh publish executable. Everything else must be explicit.
                    if Path(resolved).name not in supplied or Path(resolved).resolve().parent != path.parent:
                        raise ValueError('Undeclared non-system shared library: ' + resolved)
            else:
                dependency = line.split(' (', 1)[0]
                if number == 0 and own_install_name is not None and dependency == own_install_name:
                    continue
                if dependency.startswith(('/usr/lib/', '/System/Library/')):
                    continue
                if dependency.startswith(('@rpath/', '@loader_path/', '@executable_path/')) and Path(dependency).name in supplied:
                    continue
                raise ValueError('Undeclared non-system Mach-O dependency: ' + dependency)
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--launcher', type=Path, required=True, help='Fresh self-contained Avalonia publish output')
    parser.add_argument('--build-metadata', type=Path, required=True, help='lagoon-build.json emitted by CMake')
    parser.add_argument('--generated-manifest', type=Path, required=True, help='Local input; only its digest is packaged')
    parser.add_argument('--platform', choices=PLATFORMS, required=True)
    parser.add_argument('--companion-root', type=Path, required=True)
    parser.add_argument('--sdl-license', type=Path, required=True, help='LICENSE.txt from the exact SDL3 build/source used')
    parser.add_argument('--zlib-license', type=Path, required=True, help='zlib license/notice from the build dependency')
    parser.add_argument('--nuget-packages', type=Path, default=Path(os.environ.get('NUGET_PACKAGES', Path.home() / '.nuget/packages')))
    parser.add_argument('--soundtrack-root', type=Path, help='Optional owner-authorized complete X68000 PCM album')
    args = parser.parse_args()
    stage = args.stage.absolute()
    if stage.exists() or stage.is_symlink():
        raise SystemExit('Output must be fresh; no existing data is overwritten')
    metadata = json.loads(args.build_metadata.read_text(encoding='utf-8'))
    manifest_sha = digest(args.generated_manifest)
    patch_sha = digest(ROOT / 'patches/lagoon-framework.patch')
    if (metadata.get('format_version') != 1 or metadata.get('playable') is not True
            or metadata.get('system') != PLATFORMS[args.platform][0]
            or metadata.get('generated_manifest_sha256') != manifest_sha
            or metadata.get('framework_patch_sha256') != patch_sha
            or metadata.get('version') != (ROOT / 'VERSION').read_text().strip()
            or metadata.get('executable') != args.game.name):
        raise SystemExit('Build metadata does not qualify this as the current playable target; setup hosts cannot be packaged')
    check_binary(args.game, args.platform)
    launcher = args.launcher / 'lagoon.Companion'
    if not launcher.is_file() or not (args.launcher / 'libcoreclr.so').is_file() and not (args.launcher / 'libcoreclr.dylib').is_file():
        raise SystemExit('A self-contained Avalonia Companion publish is required')
    check_binary(launcher, args.platform)
    selected = {Path(source): relative for source, relative in achievement_cameo_inputs().items()}
    selected[args.game] = 'game/LagoonSNESRecomp'
    published = launcher_files(args.launcher)
    for source in published:
        selected[source] = source.name
    for name in ('lagoon-epx.glsl', 'lagoon-epx.glslp'):
        selected[ROOT / 'assets/shaders' / name] = 'game/assets/shaders/' + name
    for package in ('lagoon.quality', 'lagoon.soundtrack'):
        rel = f'mods/preloaded/packages/{package}/1.0.0/manifest.toml'
        selected[ROOT / rel] = 'game/' + rel
    for name in ('lagoon-companion.png', 'lagoon-game.png'):
        selected[ROOT / 'assets/icons' / name] = 'icons/' + name
    for name in ('setup_portable.py', 'portable-default.ini'):
        selected[ROOT / 'scripts' / name] = 'scripts/' + name
    for source, relative in [(ROOT / 'LICENSE', 'licenses/Lagoon-LICENSE'),
                             (ROOT / 'CREDITS.md', 'CREDITS.md'),
                             (ROOT / 'launcher/LICENSE', 'licenses/Lagoon-Companion-LICENSE'),
                             (args.companion_root / 'LICENSE', 'licenses/CompanionLauncher-LICENSE'),
                             (args.sdl_license, 'licenses/SDL3-LICENSE.txt'),
                             (args.zlib_license, 'licenses/zlib-LICENSE.txt'),
                             (ROOT / 'snesrecomp/LICENSE', 'licenses/SNESRecomp-LICENSE'),
                             (ROOT / 'snesrecomp/THIRD_PARTY_ATTRIBUTION.md', 'licenses/SNESRecomp-THIRD_PARTY_ATTRIBUTION.md')]:
        selected[source] = relative
    # Include exact framework notices from the target runtime pack, not a Windows
    # runtime or a presumed machine-wide installation.
    runtime = json.loads((args.launcher / 'lagoon.Companion.runtimeconfig.json').read_text())['runtimeOptions']
    frameworks = runtime.get('includedFrameworks', [])
    if not frameworks:
        raise SystemExit('Launcher runtimeconfig must identify self-contained framework versions')
    rid = args.platform.replace('macos-', 'osx-')
    for framework in frameworks:
        folder = args.nuget_packages / (framework['name'].lower() + '.runtime.' + rid) / framework['version']
        if not folder.is_dir():
            raise SystemExit('Missing target runtime license inputs: ' + str(folder))
        notices = [p for p in folder.iterdir() if p.is_file() and p.name.upper().startswith(('LICENSE', 'THIRD-PARTY-NOTICES'))]
        if not notices:
            raise SystemExit('No runtime license notices in ' + str(folder))
        for source in notices:
            selected[source] = 'licenses/' + framework['name'] + '/' + source.name
    # Additional publish notices (for example Avalonia native dependencies).
    for source in args.launcher.iterdir():
        if source.is_file() and source.name.upper().startswith(('LICENSE', 'THIRD-PARTY-NOTICES')):
            selected[source] = 'licenses/dotnet/' + source.name
    if args.soundtrack_root:
        pack = json.loads((args.soundtrack_root / 'pack.json').read_text(encoding='utf-8'))
        tracks = sorted(args.soundtrack_root.glob('lagoon-x68000-*.pcm'))
        if len(tracks) != 40 or not (args.soundtrack_root / 'lagoon-x68000.msu').is_file():
            raise SystemExit('Expected the reviewed complete 40-track X68000 album')
        # Require source import hashes to match, never package arbitrary local audio.
        rows = pack.get('tracks', [])
        recorded = {row.get('pcm_file', row.get('file', '')): row for row in rows}
        for track in tracks:
            row = recorded.get(track.name)
            if row is None or digest(track) not in [row.get('pcm_sha256'), row.get('sha256')]:
                raise SystemExit('Soundtrack track missing from reviewed import hashes: ' + track.name)
        for source in [*tracks, args.soundtrack_root / 'lagoon-x68000.msu', args.soundtrack_root / 'pack.json']:
            selected[source] = 'soundtrack/x68000/' + source.name
    for source in selected:
        if not source.is_file() or source.is_symlink():
            raise SystemExit('Missing/nonregular package input: ' + str(source))
    # Inspect exactly the native payload selected above, not omitted optional
    # diagnostics or other files that happen to remain in a publish directory.
    native_inputs = [args.game, launcher] + [p for p in published if p.suffix in ('.so', '.dylib')]
    loader_dependencies = dependencies(native_inputs, args.platform)
    stage.mkdir(parents=True)
    hashes = {}
    for source, relative in sorted(selected.items(), key=lambda item: item[1]):
        dest = stage / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest)
        hashes[relative] = digest(dest)
    for name in ('LagoonSNESRecomp',):
        (stage / 'game' / name).chmod(0o755)
    (stage / 'lagoon.Companion').chmod(0o755)
    start = stage / 'Start Lagoon.command'
    start.write_text('#!/bin/sh\nset -eu\ncd -- "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"\nexec ./lagoon.Companion "$@"\n', encoding='utf-8')
    start.chmod(0o755)
    hashes[start.name] = digest(start)
    package = f"lagoon-{metadata['version']}-{args.platform}"
    (stage / 'README.txt').write_text('''LAGOON COMPANION — LINUX / macOS

Bring your own clean, unheadered USA ROM. No ROM or generated C is included.
Run Start Lagoon.command (or ./lagoon.Companion). Choose your ROM, run setup,
then Play. Python 3.11+ is required only for first-run setup; no compiler is needed
for this prebuilt package. Settings/progress live in local/. Do not copy them
between running game processes. Linux desktop runtime dependencies and macOS
Gatekeeper/testing limits are documented in docs/PORTING.md in the source repo.
The interpreter safety net is intentionally retained. Native routines are not a
complete matching decompilation. Platform qualification is recorded separately
from package creation; packaging alone is not a successful gameplay test.
The optional Linux LTTng tracing provider is not included; ordinary launcher
operation does not require LTTng or its legacy tracing-library ABI.
''', encoding='utf-8')
    hashes['README.txt'] = digest(stage / 'README.txt')
    (stage / 'dependency-report.json').write_text(json.dumps(loader_dependencies, indent=2) + '\n', encoding='utf-8')
    hashes['dependency-report.json'] = digest(stage / 'dependency-report.json')
    contract = {'format_version': 1, 'mode': 'prebuilt-own-rom', 'game_id': 'lagoon',
                'version': metadata['version'], 'platform': args.platform, 'package': package,
                'playable': True, 'executable': 'game/LagoonSNESRecomp',
                'rom_identity': {'size': 1048576, 'sha256': '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'},
                'generated_manifest_sha256': manifest_sha, 'framework_patch_sha256': patch_sha,
                'game_sha256': hashes['game/LagoonSNESRecomp'], 'files': dict(sorted(hashes.items()))}
    (stage / 'portable-package.json').write_text(json.dumps(contract, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'stage': str(stage), 'platform': args.platform, 'files': len(hashes) + 1,
                      'package': package, 'published': False}))


if __name__ == '__main__':
    main()
