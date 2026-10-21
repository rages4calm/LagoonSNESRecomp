"""Qualify actual OpenGL timing/EndDraw code with a fake clock and GL call log.

This test proves diagnostic control flow, fresh-only deferred I/O and unchanged
GL command sequences. A separate actual GPU run must align its CSV sequence
count with the host's presentation count; these stubs do not measure a driver.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--compiler', default='clang')
args = parser.parse_args()
if args.out.exists():
    parser.error('Choose a fresh output directory')
repo = Path(__file__).resolve().parents[1]
source = repo / 'snesrecomp/runner/src/desktop/opengl.c'
harness = repo / 'tests/lagoon_gl_timing_test.c'
text = source.read_text(encoding='utf-8')
start = text.index('enum { kGlTimingCapacity =')
helpers = text[start:text.index('static void SaveScreenshotBmp(', start)]
start = text.index('static void OpenGLRenderer_EndDraw(void)')
end_draw = text[start:text.index('static const struct RendererFuncs', start)]
args.out.mkdir(parents=True)
(args.out / 'gl_timing_under_test.h').write_text(helpers + end_draw, encoding='utf-8')
executable = args.out / 'gl-timing-test.exe'
subprocess.run([args.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-parameter',
                '-I' + str(args.out.resolve()), str(harness), '-o', str(executable)], check=True)
cases = []
for mode in ('off', 'empty', 'zero', 'negative', 'long', 'alloc', 'register',
             'enabled', 'capacity', 'existing', 'missing-parent'):
    path = args.out / (mode + '.csv')
    if mode == 'missing-parent':
        path = args.out / 'absent/trace.csv'
    run = subprocess.run([str(executable.resolve()), mode, str(path.resolve())], capture_output=True, text=True)
    print(mode + ': ' + run.stdout.strip())
    cases.append(dict(mode=mode, returncode=run.returncode, stdout=run.stdout, stderr=run.stderr))
    if run.returncode:
        print(run.stderr)
        break
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
result = dict(passed=len(cases) == 11 and all(case['returncode'] == 0 for case in cases),
              sources={str(path.relative_to(repo)): sha(path) for path in (source, harness, Path(__file__))},
              cases=cases, scope='Actual source control flow with deterministic clock/GL call log. No actual driver, GPU, or game-runtime performance claim.')
(args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
if not result['passed']:
    raise SystemExit('GL timing source qualification failed')
