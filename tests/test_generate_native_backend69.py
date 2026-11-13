"""Public-source metadata checks; no ROM, saves or private validation required."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('generate69', PROJECT / 'tools/generate_native_backend69.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class NativeBuildMetadata(unittest.TestCase):
    def test_reviewed_input_integrity_and_architectural_modes(self):
        manifest = generator.read_json(generator.METADATA / 'manifest.json')
        for name, expected in manifest['metadata_files'].items():
            self.assertEqual(generator.text_digest((generator.METADATA / name).read_bytes()), expected)
        self.assertEqual(manifest['text_pin_policy'], generator.TEXT_PIN_POLICY)
        for profile in manifest['profiles']:
            keys = generator.read_json(generator.METADATA / profile['keys'])
            self.assertEqual(len(keys), profile['count'])
            self.assertEqual(len({tuple(k) for k in keys}), len(keys))
            self.assertTrue(all(len(k) == 4 and all(type(v) is int for v in k) for k in keys))
            self.assertTrue(all(not k[3] or k[1:3] == [1, 1] for k in keys))
        self.assertEqual([p['count'] for p in manifest['profiles']], [28507, 28493])

    def test_camera_contract_matches_actual_patch_writer_and_follower(self):
        patches = generator.read_json(generator.METADATA / 'camera.patches.json')
        generator.check_camera_contract(patches)
        patches[2]['runtime'] = 'ea' * 35
        with self.assertRaisesRegex(ValueError, 'differs'):
            generator.check_camera_contract(patches)

    def test_unrecognized_rom_refuses_before_sdk_or_output(self):
        with tempfile.TemporaryDirectory() as directory:
            rom = Path(directory) / 'unknown.sfc'
            rom.write_bytes(bytes(1024))
            output = Path(directory) / 'output'
            with self.assertRaisesRegex(ValueError, 'unmodified Lagoon'):
                generator.generate(rom, Path(directory) / 'missing-sdk', output)
            self.assertFalse(output.exists())

    def test_metadata_has_no_private_validation_dependency(self):
        manifest = generator.read_json(generator.METADATA / 'manifest.json')
        text = json.dumps(manifest)
        self.assertNotIn('validation/', text)
        self.assertNotIn('C:\\Users', text)
        self.assertEqual(len(generator.read_json(generator.METADATA / 'camera.patches.json')), 8)

    def test_lf_and_crlf_checkouts_validate_the_same_source_and_metadata(self):
        # Tiny independent SDK text fixtures; no real SDK/ROM copy or mutation.
        # Run the actual complete admission path under both checkout policies.
        with tempfile.TemporaryDirectory() as directory:
            sdk = Path(directory) / 'sdk'
            sdk.mkdir()
            rom = Path(directory) / 'fixture.rom'
            rom.write_bytes(b'\x01')
            source = sdk / 'fixture.py'
            source.write_bytes('value = "é"  \n# unchanged bytes\n'.encode('utf-8'))
            manifest = generator.read_json(generator.METADATA / 'manifest.json')
            manifest.update(original_rom_size=1, original_rom_sha256=generator.digest(b'\x01'),
                            sdk_files={'fixture.py': generator.text_digest(source.read_bytes())})
            real_read_json = generator.read_json
            real_read_bytes = Path.read_bytes
            def fixture_manifest(path):
                if path == generator.METADATA / 'manifest.json':
                    return manifest
                return real_read_json(path)
            for crlf in (False, True):
                def checkout_bytes(path):
                    data = real_read_bytes(path)
                    if path == source or path.parent == generator.METADATA:
                        data = data.replace(b'\r\n', b'\n')
                        return data.replace(b'\n', b'\r\n') if crlf else data
                    return data
                with patch.object(generator, 'read_json', fixture_manifest), patch.object(Path, 'read_bytes', checkout_bytes):
                    accepted, _, profiles = generator.checked_inputs(rom, sdk)
                    self.assertEqual(accepted['sdk_files'], manifest['sdk_files'])
                    self.assertEqual([len(keys) for _, keys, _ in profiles], [28507, 28493])

    def test_text_pins_preserve_every_non_crlf_byte(self):
        lf = 'é = 1  \n# lone carriage return\rEND\n'.encode('utf-8')
        self.assertEqual(generator.text_digest(lf), generator.text_digest(lf.replace(b'\n', b'\r\n')))
        for changed in (lf.replace(b'  ', b' '), lf.replace(b'\rEND', b'END'), lf + b' '):
            self.assertNotEqual(generator.text_digest(lf), generator.text_digest(changed))
        with self.assertRaises(UnicodeDecodeError):
            generator.text_digest(b'\xff\r\n')


if __name__ == '__main__':
    unittest.main()
