"""Compile title policy with the selected actual SDK core; never open a game."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
POLICIES = ('LAGOON_NATIVE_BACKEND69', 'SNESRECOMP_NATIVE_ONLY', 'LAGOON_NATIVE_BOOT62')


def run(sdk, compiler, out):
    sdk = sdk.resolve(); out = out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sources = [PROJECT/'tests/lagoon_native_release_policy71_test.c',
               PROJECT/'src/lagoon_native_backend69.c', sdk/'runner/src/snes/interp816.c']
    executions = []
    builds = []
    for default in (0, 1):
        exe = out/('release-policy-'+str(default)+('.exe' if os.name == 'nt' else ''))
        cmd = [str(compiler), '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror',
               '-DLAGOON_NATIVE_RELEASE_DEFAULT='+str(default),
               '-I'+str(PROJECT/'src'), '-I'+str(sdk/'runner/src'),
               *(str(p) for p in sources), '-o', str(exe)]
        subprocess.run(cmd, check=True)
        builds.append(cmd)
        # (mode, backend, strict, boot, expected status)
        cases = [('policy', None, None, None, 0), ('twice', None, None, None, 0),
                 ('step', None, None, None, 86 if default else 0),
                 ('step', '0', '0', 'auto', 0), ('step', '1', '0', 'off', 0),
                 ('step', '0', '1', 'off', 86), ('step', None, '0', None, 0),
                 ('step', '0', None, None, 86 if default else 0),
                 ('step', None, '1', None, 86),
                 ('policy', None, None, 'on', 0),
                 ('early', None, None, None, 87 if default else 0),
                 ('early', '0', '0', 'auto', 0),
                 ('flip', '0', '0', 'off', 87), ('flip', '1', '1', 'off', 87),
                 ('onrom', '0', '0', 'off', 87 if default else 0),
                 ('onrom', '1', '1', 'off', 87)]
        cases += [('policy', bad, None, None, 87) for bad in ('true', '01', '2', '-1', ' 1', '1 ')]
        cases += [('policy', None, bad, None, 87) for bad in ('true', '01', '2', '-1', ' 1', '1 ')]
        for index, (mode, backend, strict, boot, expected) in enumerate(cases):
            env = os.environ.copy()
            for name in POLICIES: env.pop(name, None)
            for name, value in zip(POLICIES, (backend, strict, boot)):
                if value is not None: env[name] = value
            marker = out/f'marker-{default}-{index}.txt'
            process = subprocess.run([str(exe), mode, str(marker)], env=env,
                                     capture_output=True, text=True)
            assert process.returncode == expected, (default, mode, backend, strict, boot, expected,
                                                   process.returncode, process.stdout, process.stderr)
            if expected:
                assert not marker.exists(), 'failed policy executed the synthetic save handler'
            else:
                assert marker.is_file()
                wanted_backend = backend if backend is not None else '1' if default else 'unset'
                wanted_strict = strict if strict is not None else '1' if default else 'unset'
                wanted_boot = boot if boot is not None else 'off' if default else 'unset'
                expected_policy = f'POLICY backend={wanted_backend} strict={wanted_strict} boot={wanted_boot} latched={int(wanted_strict == "1")}'
                assert expected_policy in process.stdout, (expected_policy, process.stdout)
            if expected == 86:
                assert 'STRICT before_fetch=1 bus_reads=0 bus_writes=0 dispatches=0 guest_unchanged=1' in process.stdout
                assert 'fetches=0 dispatches=0 completed=0 blocked=1' in process.stderr
            if not expected and mode == 'step':
                assert 'REFERENCE real_nop=1 bus_reads=1 dispatches=1' in process.stdout
            if mode == 'early' and default and backend is None:
                assert 'native-only-policy-already-latched' in process.stderr
            if mode == 'onrom' and default:
                assert 'native-release-program-not-built' in process.stderr
            executions.append(dict(default=default, mode=mode, backend=backend, strict=strict,
                                   boot=boot, expected_exit=expected, actual_exit=process.returncode,
                                   stdout=process.stdout, stderr=process.stderr))
    source = (PROJECT/'src/lagoon_native_backend69.c').read_text(encoding='utf-8')
    onrom = source[source.index('void LagoonNativeBackend69_OnRom('):]
    assert onrom.index('LagoonNativeBackend69_ConfigureExecution();') < onrom.index('interp_bridge_clear_native_program();')
    host = (sdk/'runner/src/desktop/host_main.c').read_text(encoding='utf-8')
    assert host.index('if (game->on_rom_loaded) game->on_rom_loaded(kRom, kRom_SIZE);') < host.index('Snes *snes = SnesInit(kRom, kRom_SIZE);')
    receipt = dict(passed=True, cases=len(executions), builds=builds, executions=executions,
                   sources={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
                   scope='Actual title policy and SDK 65816 backstop on this host; no generated game/source or gameplay qualification; POSIX target compilation belongs to platform builds.')
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(passed=True, cases=len(executions), receipt=str(out/'receipt.json'))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    run(args.sdk, args.compiler, args.out)


if __name__ == '__main__': main()
