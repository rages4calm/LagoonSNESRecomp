"""Prevent EXE-only deployments from silently shipping stale public sidecars."""
import importlib.util
from pathlib import Path
import tempfile
import unittest


spec = importlib.util.spec_from_file_location(
    "catalog", Path(__file__).resolve().parents[1] / "tools/check_runtime_catalog.py")
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)


class RuntimeCatalogTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.source, self.runtime, self.writable = [root / n for n in ("source", "game", "local")]
        for relative in catalog.SIDECARS:
            for directory in (self.source, self.runtime, self.writable):
                target = directory / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(("fixture " + relative + "\r\n").encode())
        (self.runtime / "LagoonSNESRecomp.exe").write_bytes(b"synthetic executable")

    def test_matching_runtime_and_setup_copies(self):
        result = catalog.check_runtime(self.source, self.runtime, self.writable)
        self.assertTrue(result["passed"])
        self.assertEqual(len(result["files"]), 8)
        self.assertEqual(result["game_sha256"], catalog.sha256(self.runtime / "LagoonSNESRecomp.exe"))

    def test_each_sidecar_missing_or_stale_in_each_location(self):
        for root in (self.runtime, self.writable):
            for relative in catalog.SIDECARS:
                with self.subTest(root=root.name, relative=relative):
                    target = root / relative
                    original = target.read_bytes()
                    target.write_bytes(original + b"old manifest or shader")
                    result = catalog.check_runtime(self.source, self.runtime, self.writable)
                    self.assertFalse(result["passed"])
                    self.assertEqual(len(result["errors"]), 1)
                    self.assertEqual(target.read_bytes(), original + b"old manifest or shader")
                    target.unlink()
                    self.assertFalse(catalog.check_runtime(self.source, self.runtime, self.writable)["passed"])
                    target.write_bytes(original)

    def test_missing_source_never_passes_even_if_both_missing(self):
        relative = catalog.SIDECARS[0]
        (self.source / relative).unlink()
        (self.runtime / relative).unlink()
        self.assertFalse(catalog.check_runtime(self.source, self.runtime)["passed"])

    def test_executable_required_and_owner_files_untouched(self):
        config = self.runtime / "config.ini"
        config.write_bytes(b"owner settings are not parsed")
        save = self.runtime / "save.srm"
        save.write_bytes(b"owner save is not parsed")
        (self.runtime / "LagoonSNESRecomp.exe").unlink()
        self.assertFalse(catalog.check_runtime(self.source, self.runtime)["passed"])
        self.assertEqual(config.read_bytes(), b"owner settings are not parsed")
        self.assertEqual(save.read_bytes(), b"owner save is not parsed")


if __name__ == "__main__":
    unittest.main()
