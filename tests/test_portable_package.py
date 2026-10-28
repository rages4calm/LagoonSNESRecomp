"""ROM-free package safety tests; synthetic files never qualify real gameplay."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))
import setup_portable as setup
import package_portable as package


class PortablePackageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Kept in the private build tree for owner-run cleanup; no owner data or
        # outside-workspace directory is deleted by the test runner.
        scratch = ROOT / 'build-port-tests'
        scratch.mkdir(exist_ok=True)
        cls.root = Path(tempfile.mkdtemp(prefix='package-', dir=scratch))
        cls.rom = cls.root / 'synthetic-rom.bin'
        cls.rom.write_bytes(bytes(1048576))

    def setUp(self):
        self.stage = Path(tempfile.mkdtemp(prefix='case-', dir=self.root))
        self.entries = {'game/LagoonSNESRecomp': b'fake executable for contract tests only',
                        'scripts/portable-default.ini': b'[Graphics]\nFullscreen = 0\n',
                        'game/mods/preloaded/packages/lagoon.quality/1.0.0/manifest.toml': b'id="lagoon.quality"\n'}
        for relative, content in self.entries.items():
            p = self.stage / relative
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(content)
        self.contract = {'format_version': 1, 'mode': 'prebuilt-own-rom', 'game_id': 'lagoon',
                         'playable': True, 'platform': 'linux-x64', 'package': 'unit-test-only',
                         'executable': 'game/LagoonSNESRecomp', 'generated_manifest_sha256': 'a' * 64,
                         'rom_identity': {'size': 1048576, 'sha256': '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'},
                         'files': {r: hashlib.sha256(b).hexdigest() for r, b in self.entries.items()}}
        self.save_contract()

    def save_contract(self):
        (self.stage / 'portable-package.json').write_text(json.dumps(self.contract), encoding='utf-8')

    def run_synthetic_setup(self):
        real = setup.digest
        with patch.object(setup, 'digest', side_effect=lambda p: self.contract['rom_identity']['sha256'] if p == self.rom else real(p)):
            return setup.setup(self.stage, self.rom)

    def test_initialization_does_not_copy_rom(self):
        self.run_synthetic_setup()
        local = self.stage / 'local'
        self.assertTrue((local / 'config.ini').is_file())
        self.assertTrue((local / 'saves').is_dir())
        self.assertTrue((local / 'mods/preloaded/packages/lagoon.quality/1.0.0/manifest.toml').is_file())
        self.assertFalse(any(p.suffix in ('.sfc', '.smc', '.bin') for p in self.stage.rglob('*')))

    def test_repeated_setup_preserves_user_progress_settings_and_catalog(self):
        self.run_synthetic_setup()
        files = ['local/config.ini', 'local/keybinds.ini', 'local/saves/save1.sav',
                 'local/mods/preloaded/state.toml', 'local/lagoon-achievements.json']
        for relative in files:
            (self.stage / relative).write_bytes(b'owner state preserved')
        self.run_synthetic_setup()
        for relative in files:
            self.assertEqual((self.stage / relative).read_bytes(), b'owner state preserved')

    def test_setup_host_contract_is_refused(self):
        self.contract['playable'] = False
        self.save_contract()
        with self.assertRaises(ValueError):
            self.run_synthetic_setup()
        self.assertFalse((self.stage / 'local').exists())

    def test_changed_payload_is_refused_before_local_writes(self):
        (self.stage / 'game/LagoonSNESRecomp').write_bytes(b'changed')
        with self.assertRaises(ValueError):
            self.run_synthetic_setup()
        self.assertFalse((self.stage / 'local').exists())

    def test_wrong_rom_is_refused(self):
        with self.assertRaises(ValueError):
            setup.setup(self.stage, self.rom)
        self.assertFalse((self.stage / 'local').exists())

    def test_path_traversal_and_absolute_paths_are_refused(self):
        for relative in ('../other', '/tmp/other', 'C:/other', 'game/../other', 'game\\other', 'game//other'):
            with self.subTest(path=relative), self.assertRaises(ValueError):
                setup.safe_path(self.stage, relative)

    def test_windows_binary_cannot_be_relabelled_linux(self):
        p = self.stage / 'windows.exe'
        p.write_bytes(b'MZ' + bytes(128))
        with self.assertRaises(ValueError):
            package.check_binary(p, 'linux-x64')

    def test_linux_binary_architecture_is_checked(self):
        p = self.stage / 'linux'
        b = bytearray(64)
        b[:6] = b'\x7fELF\x02\x01'
        struct.pack_into('<H', b, 18, 62)
        p.write_bytes(b)
        package.check_binary(p, 'linux-x64')
        with self.assertRaises(ValueError):
            package.check_binary(p, 'linux-arm64')

    def test_macos_binary_architecture_is_checked(self):
        p = self.stage / 'mac'
        b = bytearray(32)
        b[:4] = b'\xcf\xfa\xed\xfe'
        struct.pack_into('<I', b, 4, 0x100000c)
        p.write_bytes(b)
        package.check_binary(p, 'macos-arm64')
        with self.assertRaises(ValueError):
            package.check_binary(p, 'macos-x64')

    def test_shared_sdl_is_rejected(self):
        p = self.stage / 'game'
        response = SimpleNamespace(returncode=0, stdout='libSDL3.so.0 => /usr/lib/libSDL3.so.0 (0x123)\n', stderr='')
        with patch.object(package.platform, 'system', return_value='Linux'), patch.object(package.subprocess, 'run', return_value=response), self.assertRaises(ValueError):
            package.dependencies([p], 'linux-x64')

    def test_unresolved_or_developer_libraries_are_rejected(self):
        for text in ('libfoo.so => not found\n', 'libfoo.so => /home/developer/lib/libfoo.so (0x123)\n'):
            response = SimpleNamespace(returncode=0, stdout=text, stderr='')
            with self.subTest(text=text), patch.object(package.platform, 'system', return_value='Linux'), patch.object(package.subprocess, 'run', return_value=response), self.assertRaises(ValueError):
                package.dependencies([self.stage / 'game'], 'linux-x64')

    def test_system_library_and_supplied_native_sibling_are_accepted(self):
        p = self.stage / 'game'
        lib = self.stage / 'libcoreclr.so'
        response = SimpleNamespace(returncode=0, stdout='libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x123)\nlibcoreclr.so => ' + str(lib) + ' (0x456)\n', stderr='')
        with patch.object(package.platform, 'system', return_value='Linux'), patch.object(package.subprocess, 'run', return_value=response):
            report = package.dependencies([p, lib], 'linux-x64')
        self.assertNotIn(str(self.stage), json.dumps(report))

    def test_relative_inputs_accept_only_the_same_selected_absolute_sibling(self):
        publish = self.stage / 'publish'
        publish.mkdir()
        game = publish / 'lagoon.Companion'
        library = publish / 'libmscordaccore.so'
        # Build/package jobs naturally pass build-port/launcher relatively while
        # ldd reports this native sibling as an absolute resolved path.
        relative = Path(os.path.relpath(publish, Path.cwd()))
        text = 'libmscordaccore.so => ' + str(library.resolve()) + ' (0x123)\n'
        response = SimpleNamespace(returncode=0, stdout=text, stderr='')
        with patch.object(package.platform, 'system', return_value='Linux'), patch.object(package.subprocess, 'run', return_value=response):
            report = package.dependencies([relative / game.name, relative / library.name], 'linux-x64')
        self.assertNotIn(str(publish.resolve()), json.dumps(report))
        wrong = SimpleNamespace(returncode=0, stdout='libmscordaccore.so => /home/other/libmscordaccore.so (0x123)\n', stderr='')
        with patch.object(package.platform, 'system', return_value='Linux'), patch.object(package.subprocess, 'run', return_value=wrong), self.assertRaises(ValueError):
            package.dependencies([relative / game.name, relative / library.name], 'linux-x64')

    def test_macos_homebrew_dependency_is_rejected(self):
        response = SimpleNamespace(returncode=0, stdout='game:\n\t/opt/homebrew/lib/libSDL3.dylib (compatibility version 0.0.0)\n', stderr='')
        with patch.object(package.platform, 'system', return_value='Darwin'), patch.object(package.subprocess, 'run', return_value=response), self.assertRaises(ValueError):
            package.dependencies([self.stage / 'game'], 'macos-arm64')

    def test_macos_dylib_exact_self_install_name_can_differ_from_filename(self):
        p = self.stage / 'libAvaloniaNative.dylib'
        installed = '/usr/local/lib/libAvalonia.Native.OSX.dylib'
        def response(command, **kwargs):
            text = str(p) + ':\n' + (installed + '\n' if '-D' in command else
                    '\t' + installed + ' (compatibility version 1.0.0)\n\t/usr/lib/libSystem.B.dylib (compatibility version 1.0.0)\n')
            return SimpleNamespace(returncode=0, stdout=text, stderr='')
        with patch.object(package.platform, 'system', return_value='Darwin'), patch.object(package.subprocess, 'run', side_effect=response):
            package.dependencies([p], 'macos-arm64')

    def test_macos_exact_self_id_does_not_allow_an_external_usr_local_dependency(self):
        p = self.stage / 'libAvaloniaNative.dylib'
        installed = '/usr/local/lib/libAvalonia.Native.OSX.dylib'
        def response(command, **kwargs):
            text = str(p) + ':\n' + (installed + '\n' if '-D' in command else
                    '\t' + installed + ' (compatibility version 1.0.0)\n\t/usr/local/lib/libOther.dylib (compatibility version 1.0.0)\n')
            return SimpleNamespace(returncode=0, stdout=text, stderr='')
        with patch.object(package.platform, 'system', return_value='Darwin'), patch.object(package.subprocess, 'run', side_effect=response), self.assertRaises(ValueError):
            package.dependencies([p], 'macos-arm64')

    def test_macos_same_basename_is_not_a_self_id_exemption(self):
        p = self.stage / 'libAvaloniaNative.dylib'
        def response(command, **kwargs):
            text = str(p) + ':\n' + ('@rpath/actual-id.dylib\n' if '-D' in command else
                    '\t/usr/local/lib/libAvaloniaNative.dylib (compatibility version 1.0.0)\n')
            return SimpleNamespace(returncode=0, stdout=text, stderr='')
        with patch.object(package.platform, 'system', return_value='Darwin'), patch.object(package.subprocess, 'run', side_effect=response), self.assertRaises(ValueError):
            package.dependencies([p], 'macos-arm64')

    def test_macos_native_dependency_inspection_selects_target_slice(self):
        p = self.stage / 'libUniversal.dylib'
        for target, expected in [('macos-arm64', 'arm64'), ('macos-x64', 'x86_64')]:
            calls = []
            def response(command, **kwargs):
                calls.append(command)
                text = str(p) + ':\n' + ('@rpath/libUniversal.dylib\n' if '-D' in command else
                        '\t@rpath/libUniversal.dylib (compatibility version 1.0.0)\n\t/usr/lib/libSystem.B.dylib (compatibility version 1.0.0)\n')
                return SimpleNamespace(returncode=0, stdout=text, stderr='')
            with self.subTest(target=target), patch.object(package.platform, 'system', return_value='Darwin'), patch.object(package.subprocess, 'run', side_effect=response):
                package.dependencies([p], target)
            self.assertEqual(len(calls), 2)
            self.assertTrue(all(command[1:3] == ['-arch', expected] for command in calls))

    def test_macos_missing_target_slice_is_refused(self):
        response = SimpleNamespace(returncode=1, stdout='', stderr='does not contain the specified architecture')
        with patch.object(package.platform, 'system', return_value='Darwin'), patch.object(package.subprocess, 'run', return_value=response), self.assertRaises(ValueError):
            package.dependencies([self.stage / 'libUniversal.dylib'], 'macos-arm64')

    def test_launcher_payload_omits_only_the_optional_lttng_provider(self):
        publish = self.stage / 'publish'
        publish.mkdir()
        names = ['lagoon.Companion', 'libcoreclr.so', 'libhostfxr.so',
                 'libcoreclrtraceptprovider.so', 'libcoreclrtraceptprovider2.so',
                 'libUnrelated.so', 'app.dll', 'app.runtimeconfig.json', 'debug.pdb']
        for name in names:
            (publish / name).write_bytes(b'synthetic publish input')
        selected = package.launcher_files(publish)
        self.assertEqual({p.name for p in selected}, set(names) - {'libcoreclrtraceptprovider.so', 'debug.pdb'})
        # Any selected runtime with an unresolved dependency remains fatal.
        response = SimpleNamespace(returncode=0, stdout='libOther.so => not found\n', stderr='')
        with patch.object(package.platform, 'system', return_value='Linux'), patch.object(package.subprocess, 'run', return_value=response), self.assertRaises(ValueError):
            package.dependencies([publish / 'libUnrelated.so'], 'linux-x64')


if __name__ == '__main__':
    unittest.main()
