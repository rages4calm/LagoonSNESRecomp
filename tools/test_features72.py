"""Focused map/menu/journal/return contracts, with an owner-supplied clean ROM.

These are isolated models and host adapter tests, not executed gameplay routes.
No ROM, decoded graphics, saves or generated game code are written to the output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--compiler', default='clang')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    rom, sdk, out = args.rom.resolve(), args.sdk.resolve(), args.out.resolve()
    assert hashlib.sha256(rom.read_bytes()).hexdigest() == '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
    out.mkdir(parents=True, exist_ok=True)
    flags = ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
             '-I'+str(root/'src'), '-I'+str(sdk/'runner/src')]
    cases = {
        'journal': ['tests/lagoon_journal72_test.c', 'src/lagoon_journal.c', 'src/lagoon_quest_guidance.c'],
        'items': ['tests/lagoon_item_guide72_test.c', 'src/lagoon_item_guide.c'],
        'return-model': ['tests/lagoon_return72_test.c'],
        'return-adapter': ['tests/lagoon_return_runtime72_test.c', 'src/lagoon_return.c', 'src/lagoon_return_runtime.c'],
        'ui': ['tests/lagoon_ui_test.c', 'src/lagoon_ui.c'],
        'settings': ['tests/lagoon_settings_test.c', 'src/lagoon_ui.c'],
        'achievements-ui': ['tests/lagoon_achievement_ui_test.c', 'src/lagoon_ui.c'],
    }
    runs = []
    for name, files in cases.items():
        binary = out/(name+('.exe' if __import__('os').name == 'nt' else ''))
        sources = [str(root/f) for f in files]
        if name in ('settings', 'achievements-ui'):
            sources.append(str(sdk/'runner/src/snes_overlay_draw.c'))
        subprocess.run([args.compiler, *flags, *sources, '-o', str(binary)], check=True)
        command = [str(binary), str(rom)] if name in ('journal', 'items', 'return-model', 'return-adapter') else [str(binary)]
        if name == 'settings':
            command.append(str(out/'settings-test.ini'))
        run = subprocess.run(command, capture_output=True, text=True, cwd=out)
        runs.append(dict(case=name, returncode=run.returncode, stdout=run.stdout, stderr=run.stderr))
        print(name, run.stdout.strip(), run.stderr.strip())
        run.check_returncode()
    binary = out/('return-adapter'+('.exe' if __import__('os').name == 'nt' else ''))
    for name in ('hook-capacity', 'terminator', 'incomplete', 'resume', 'host-change', 'reentry', 'source-lost', 'timeout'):
        run = subprocess.run([str(binary), str(rom), name], capture_output=True, text=True, cwd=out)
        runs.append(dict(case=name, returncode=run.returncode, expected_exit=87, stderr=run.stderr))
        assert run.returncode == 87, (name, run.returncode, run.stderr)
    files = {f for group in cases.values() for f in group}
    receipt = dict(passed=True, scope=__doc__.strip(), runs=runs,
                   sources={f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in sorted(files)})
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
