"""Real patch application/idempotence and wrong-revision refusal on tiny fixtures."""
import importlib.util
from pathlib import Path
import subprocess,sys,tempfile,unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]

class FrameworkPatchTests(unittest.TestCase):
    def test_revision_guard_idempotence_and_unpackaged_sdk(self):
        spec=importlib.util.spec_from_file_location('framework_patch',ROOT/'tools/apply_framework_patch.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory(prefix='lagoon-patch-') as directory:
            root=Path(directory);sdk=root/'sdk';sdk.mkdir()
            def git(*args):return subprocess.check_output(['git','-C',str(sdk),*args],stderr=subprocess.DEVNULL)
            git('init');git('config','core.autocrlf','false')
            (sdk/'sample.txt').write_bytes(b'old\n');git('add','sample.txt')
            git('-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','-m','synthetic base')
            module.BASE=git('rev-parse','HEAD').decode().strip();module.framework=sdk
            module.patch=root/'change.patch';module.patch.write_bytes(b'diff --git a/sample.txt b/sample.txt\n--- a/sample.txt\n+++ b/sample.txt\n@@ -1 +1 @@\n-old\n+new\n')
            with patch.object(sys,'argv',['apply_framework_patch.py']):
                module.main();self.assertEqual((sdk/'sample.txt').read_bytes(),b'new\n')
                module.main();self.assertEqual((sdk/'sample.txt').read_bytes(),b'new\n')
                (sdk/'sample.txt').write_bytes(b'old\n');module.BASE='0'*40
                with self.assertRaises(SystemExit):module.main()
                self.assertEqual((sdk/'sample.txt').read_bytes(),b'old\n')
                # An already patched distribution does not require a Git checkout.
                unpacked=root/'unpacked';unpacked.mkdir();(unpacked/'sample.txt').write_bytes(b'new\n')
                module.framework=unpacked;module.main()
                self.assertEqual((unpacked/'sample.txt').read_bytes(),b'new\n')

if __name__=='__main__':unittest.main()
