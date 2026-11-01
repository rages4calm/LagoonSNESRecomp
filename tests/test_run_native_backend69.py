"""Exercise a harmless child interpreter, never open the game or owner saves."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[1]
WRAPPER = PROJECT / 'tools/run_native_backend69.py'


class NativeLauncher(unittest.TestCase):
    def test_child_environment_arguments_cwd_and_failure_status(self):
        # Python stands in for the executable so this tests real process
        # inheritance/argument handling without depending on the game build.
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'child.json'
            child = Path(directory) / 'child.py'
            child.write_text('import json,os,sys\nfrom pathlib import Path\n'
                             'Path(sys.argv[1]).write_text(json.dumps(dict(args=sys.argv[2:],cwd=os.getcwd(),'
                             'env={k:os.environ.get(k) for k in ["LAGOON_NATIVE_BACKEND69",'
                             '"SNESRECOMP_NATIVE_ONLY","LAGOON_NATIVE_BOOT62"]})),encoding="utf-8")\n'
                             'raise SystemExit(87)\n', encoding='utf-8')
            environment = os.environ.copy()
            environment.update(LAGOON_NATIVE_BACKEND69='0', SNESRECOMP_NATIVE_ONLY='0', LAGOON_NATIVE_BOOT62='auto')
            original = dict(os.environ)
            arguments = ['two words', '--unknown-game-option', 'é', 'quote"literal']
            result = subprocess.run([sys.executable, str(WRAPPER), '--exe', sys.executable,
                                     '--', str(child), str(output), *arguments], env=environment)
            self.assertEqual(result.returncode, 87)
            observed = json.loads(output.read_text(encoding='utf-8'))
            self.assertEqual(observed['args'], arguments)
            self.assertEqual(Path(observed['cwd']), Path(sys.executable).resolve().parent)
            self.assertEqual(observed['env'], dict(LAGOON_NATIVE_BACKEND69='1', SNESRECOMP_NATIVE_ONLY='1',
                                                  LAGOON_NATIVE_BOOT62='off'))
            self.assertEqual(dict(os.environ), original)
            self.assertEqual(environment['SNESRECOMP_NATIVE_ONLY'], '0')

    def test_plain_script_help_does_not_import_local_dis(self):
        result = subprocess.run([sys.executable, str(WRAPPER), '--help'],
                                cwd=PROJECT, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--exe', result.stdout)
        self.assertNotIn('Traceback', result.stderr)

    def test_missing_executable_refuses(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([sys.executable, '-P', str(WRAPPER), '--exe',
                                     str(Path(directory) / 'missing.exe')], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn('does not exist', result.stderr)


if __name__ == '__main__':
    unittest.main()
