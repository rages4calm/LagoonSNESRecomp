"""Strict comparison of isolated qualify_native.py replays; never launches a game.

Different executable/program identities are expected and recorded. Inputs,
ordered presentations, all twelve checkpoint files, CPU registers and clocks
must match, including audio, DMA and PPU-write event timing.
Exit 0 means exact, 1 means a difference, 2 means invalid/missing evidence.
"""
import argparse
import csv
import hashlib
import json
import re
from pathlib import Path


COMPONENTS = ('fb.bmp', 'fb.bgrx', 'wram.bin', 'vram.bin', 'oam.bin',
              'cgram.bin', 'sram.bin', 'regs.json', 'apu.tsv', 'dma.tsv', 'ppuw.tsv')
FATAL = ('[brk]', '[interp_cap]', '[interp_low_exec]', '[cpu_halt]', '[native_contract]', 'assertion failed')
INPUT_KEYS = ('rom_sha256', 'route_sha256', 'frames', 'input_config_sha256',
              'effective_config_sha256', 'enabled_features', 'enabled_package_features',
              'diagnostic_overrides', 'script_fixture_input', 'private_mod_state', 'script_only_input',
              'visible_display', 'offscreen_gpu', 'input_msu_pack')
REQUIRED = ('exe_sha256', 'program_manifest_sha256', 'rom_sha256', 'route_sha256',
            'frames', 'effective_config_sha256')
STRICT_CAPTURE_PATHS = {'SNESRECOMP_APURAM_TRACE_FILE', 'SNESRECOMP_DSPREG_TRACE_FILE',
                        'SNESRECOMP_DSPOUT', 'SNESRECOMP_APU_QUEUE_TRACE'}
ADVISORY_REPORT_PATHS = {'LAGOON_NATIVE_RECOVERY_REPORT', 'LAGOON_SCENE_RENDERER_REPORT',
                         'LAGOON_FOLLOWER_REPORT', 'LAGOON_FRAME_PROBE',
                         'LAGOON_TRANSITION_REPORT', 'LAGOON_SWORD_REPORT',
                         'LAGOON_WIDE_ENTITY_REPORT'}


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def json_diff(left, right, path='$'):
    """Return every semantic difference, including missing keys and list entries."""
    if type(left) is not type(right):
        return [dict(path=path, baseline=left, candidate=right)]
    if isinstance(left, dict):
        rows = []
        for key in sorted(left.keys() | right.keys()):
            if key not in left or key not in right:
                rows.append(dict(path=f'{path}.{key}', baseline_missing=key not in left,
                                 candidate_missing=key not in right,
                                 baseline=left.get(key), candidate=right.get(key)))
            else:
                rows.extend(json_diff(left[key], right[key], f'{path}.{key}'))
        return rows
    if isinstance(left, list):
        rows = []
        for index in range(max(len(left), len(right))):
            if index >= len(left) or index >= len(right):
                rows.append(dict(path=f'{path}[{index}]', baseline_missing=index >= len(left),
                                 candidate_missing=index >= len(right)))
            else:
                rows.extend(json_diff(left[index], right[index], f'{path}[{index}]'))
        return rows
    return [] if left == right else [dict(path=path, baseline=left, candidate=right)]


def load_run(directory):
    directory = Path(directory).resolve()
    receipt_path = directory / 'receipt.json'
    receipt = json.loads(receipt_path.read_text(encoding='utf-8'))
    errors = []
    for key in REQUIRED:
        if not receipt.get(key):
            errors.append(f'missing receipt field {key}')
    if receipt.get('exit_code') != 0 or receipt.get('runtime_failures'):
        errors.append('unsuccessful runtime receipt')
    if receipt.get('qualification_passed') is False:
        errors.append('qualification receipt explicitly failed')
    if not receipt.get('script_only_input'):
        errors.append('physical input was not isolated')
    bindings = {}
    frozen = [('route.txt', receipt.get('route_sha256')),
              ('config.ini', receipt.get('effective_config_sha256')),
              ('program_manifest.json', receipt.get('program_manifest_sha256'))]
    warp = receipt.get('campaign_warps')
    if warp:
        frozen.append(('campaign-warps.txt', warp.get('sha256')))
    for name, expected in frozen:
        path = directory / name
        digest = sha(path) if path.is_file() else None
        bindings[name] = dict(sha256=digest, expected_sha256=expected)
        if not digest or digest != expected:
            errors.append(f'frozen identity mismatch: {name}')
    # An original executable may have been retired. Its recorded digest remains
    # bound by the hashed receipt; report availability rather than silently
    # identifying today's executable as the historical one.
    exe = Path(receipt.get('exe', ''))
    bindings['executable'] = dict(recorded_sha256=receipt.get('exe_sha256'),
                                  current_sha256=sha(exe) if exe.is_file() else None)
    log_hashes = {}
    for name in ('stdout.log', 'stderr.log'):
        path = directory / name
        if not path.is_file():
            errors.append(f'missing {name}')
            continue
        log_hashes[name] = sha(path)
        for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
            if (any(marker in line.lower() for marker in FATAL) or
                re.search(r'(?:lagoon\.qol:|\[lagoon-qol\])[^\r\n]*\brefused\b',line,re.IGNORECASE)):
                errors.append(f'{name}: {line.strip()}')
    present = directory / 'presents.csv'
    with present.open(encoding='utf-8', newline='') as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != ['present', 'frame', 'alpha', 'crc32', 'luma']:
            errors.append('unexpected presentation columns')
        presentations = list(reader)
    if not presentations:
        errors.append('empty presentation stream')
    for index, row in enumerate(presentations):
        if row.get('present') != str(index):
            errors.append(f'presentation ordinal discontinuity at {index}')
            break
    dumps = directory / 'dumps'
    tags = sorted(p.name.removesuffix('.wram.bin') for p in dumps.glob('*.wram.bin'))
    if not tags or tags != sorted(receipt.get('dump_tags', [])):
        errors.append('checkpoint tags missing or disagree with receipt')
    # Inventory every component's tag set, so a stray/missing component cannot
    # hide outside the WRAM-derived checkpoint list.
    for component in (*COMPONENTS, 'info.json'):
        component_tags = sorted(p.name.removesuffix('.' + component)
                                for p in dumps.glob('*.' + component))
        if component_tags != tags:
            errors.append(f'checkpoint inventory mismatch: {component}')
    overrides = receipt.get('diagnostic_overrides')
    normalized = dict(overrides) if overrides is not None else None
    captures = {}
    # Candidate-only instrumentation may be ignored by a historical executable.
    # Its destination is not guest input, but must remain inside the run even
    # when no file was produced. Its content is deliberately not parity evidence.
    for key in ADVISORY_REPORT_PATHS & set(overrides or {}):
        path = Path(overrides[key]).resolve()
        if not path.is_relative_to(directory):
            errors.append(f'out-of-run advisory report: {key}')
            continue
        normalized[key] = path.relative_to(directory).as_posix()
    for key in STRICT_CAPTURE_PATHS & set(overrides or {}):
        path = Path(overrides[key]).resolve()
        if not path.is_relative_to(directory) or not path.is_file():
            errors.append(f'missing or out-of-run diagnostic capture: {key}')
            continue
        normalized[key] = path.relative_to(directory).as_posix()
        size = path.stat().st_size
        if not size or (key == 'SNESRECOMP_DSPOUT' and size % 4):
            errors.append(f'empty or incomplete diagnostic capture: {key}')
        captures[key] = dict(relative_path=normalized[key], size=size, sha256=sha(path))
    return dict(directory=directory, receipt=receipt, errors=errors, tags=tags,
                normalized_overrides=normalized, captures=captures,
                presentations=presentations, identity=dict(directory=str(directory),
                    receipt_sha256=sha(receipt_path), executable_sha256=receipt.get('exe_sha256'),
                    manifest_sha256=receipt.get('program_manifest_sha256'), bindings=bindings,
                    log_hashes=log_hashes, presents_sha256=sha(present),
                    diagnostic_captures=captures))


def compare_component(left, right):
    a, b = left.read_bytes(), right.read_bytes()
    result = dict(exact=a == b, baseline_sha256=hashlib.sha256(a).hexdigest(),
                  candidate_sha256=hashlib.sha256(b).hexdigest(),
                  baseline_size=len(a), candidate_size=len(b))
    if a != b:
        differences = [i for i in range(max(len(a), len(b)))
                       if i >= len(a) or i >= len(b) or a[i] != b[i]]
        result['different_bytes'] = len(differences)
        result['first_differing_bytes'] = [dict(offset=i, hex_offset=f'{i:05X}',
            baseline=a[i] if i < len(a) else None, candidate=b[i] if i < len(b) else None)
            for i in differences[:64]]
    if left.suffix == '.json':
        result['json_differences'] = json_diff(json.loads(a), json.loads(b))
    return result


def iterator52_control(left, right, old_overrides, new_overrides):
    old,new=dict(old_overrides or {}),dict(new_overrides or {})
    baseline_mode=old.pop('LAGOON_NATIVE_ITERATOR52',None)
    candidate_mode=new.pop('LAGOON_NATIVE_ITERATOR52',None)
    identities_equal=all(isinstance(left.get(k),str) and re.fullmatch(r'[0-9a-f]{64}',left[k])
        and left[k]==right.get(k) for k in ('exe_sha256','program_manifest_sha256'))
    return dict(kind='inactive_actor_iterator52',baseline_mode=baseline_mode,
        candidate_mode=candidate_mode,same_executable_and_manifest=identities_equal,
        approved=bool(identities_equal and baseline_mode=='off' and candidate_mode in {'auto','on'} and old==new))


def compare_runs(baseline, candidate, interp_control_target=None, native_iterator_control=False):
    if interp_control_target is not None and native_iterator_control:
        raise ValueError('Declare exactly one execution control')
    left, right = load_run(baseline), load_run(candidate)
    result = dict(schema='lagoon strict native replay comparison v2',
                  baseline=left['identity'], candidate=right['identity'],
                  invalid_evidence={'baseline': left['errors'], 'candidate': right['errors']})
    inputs = {key: (left['receipt'].get(key), right['receipt'].get(key)) for key in INPUT_KEYS}
    # Capture env values name outputs, not guest input files. Compare their
    # paths relative to each isolated run AND require identical captured bytes.
    # The single advisory report path is also relative, without a content check.
    # Every other diagnostic override retains its original exact input contract.
    inputs['diagnostic_overrides'] = (left['normalized_overrides'], right['normalized_overrides'])
    result['diagnostic_outputs'] = {}
    for key in sorted(left['captures'].keys() | right['captures'].keys()):
        a, b = left['captures'].get(key), right['captures'].get(key)
        result['diagnostic_outputs'][key] = dict(baseline=a, candidate=b, exact=a is not None and a == b)
    for key, run in [('baseline', left), ('candidate', right)]:
        # Paths differ in isolated roots; seed identity is filename plus digest.
        run['seeds'] = sorted((Path(row['copy']).name, row['sha256'])
                              for row in run['receipt'].get('seeded_save_files', []))
    inputs['seeded_save_files'] = (left['seeds'], right['seeds'])
    inputs['campaign_warps_sha256'] = tuple((r['receipt'].get('campaign_warps') or {}).get('sha256')
                                           for r in (left, right))
    result['input_differences'] = {k: dict(baseline=a, candidate=b)
                                   for k, (a, b) in inputs.items() if a != b}
    unexpected_inputs = dict(result['input_differences'])
    if interp_control_target is not None:
        if not re.fullmatch(r'[0-9A-F]{6}', interp_control_target):
            raise ValueError('Interpreter control target must be six uppercase hex digits')
        old = dict(left['normalized_overrides'] or {})
        new = dict(right['normalized_overrides'] or {})
        actual = old.pop('SNESRECOMP_LLE_INTERP_TARGET', None)
        identities_equal = all(left['receipt'].get(k) == right['receipt'].get(k)
            for k in ('exe_sha256', 'program_manifest_sha256'))
        approved = identities_equal and actual == interp_control_target and old == new
        result['execution_control'] = dict(target=interp_control_target,
            baseline_interpreted=True, same_executable_and_manifest=identities_equal,
            approved=approved)
        if approved:
            unexpected_inputs.pop('diagnostic_overrides', None)
        else:
            unexpected_inputs['execution_control'] = dict(expected=interp_control_target,
                                                          actual=actual)
    if native_iterator_control:
        control=iterator52_control(left['receipt'],right['receipt'],left['normalized_overrides'],right['normalized_overrides'])
        result['execution_control']=control
        if control['approved']:
            unexpected_inputs.pop('diagnostic_overrides',None)
        else:
            unexpected_inputs['execution_control']=control
    result['unapproved_input_differences'] = unexpected_inputs
    a, b = left['presentations'], right['presentations']
    mismatches = [i for i in range(max(len(a), len(b)))
                  if i >= len(a) or i >= len(b) or a[i] != b[i]]
    first = mismatches[0] if mismatches else None
    result['ordered_presentations'] = dict(exact=not mismatches, baseline_count=len(a),
        candidate_count=len(b), mismatch_count=len(mismatches),
        first_mismatch=None if first is None else dict(index=first,
            baseline=a[first] if first < len(a) else None,
            candidate=b[first] if first < len(b) else None))
    result['missing_candidate_tags'] = sorted(set(left['tags']) - set(right['tags']))
    result['additional_candidate_tags'] = sorted(set(right['tags']) - set(left['tags']))
    result['checkpoints'] = []
    for tag in sorted(set(left['tags']) & set(right['tags'])):
        row = dict(tag=tag, components={})
        for component in (*COMPONENTS, 'info.json'):
            paths = [r['directory'] / 'dumps' / f'{tag}.{component}' for r in (left, right)]
            row['components'][component] = (compare_component(*paths) if all(p.is_file() for p in paths)
                                             else dict(exact=False, missing=True))
        info = [json.loads((r['directory'] / 'dumps' / f'{tag}.info.json').read_text())
                for r in (left, right) if (r['directory'] / 'dumps' / f'{tag}.info.json').is_file()]
        row['clock_exact'] = len(info) == 2 and all(k in v for v in info
            for k in ('frame', 'master_clock', 'vpos', 'hpos')) and all(
            info[0][k] == info[1][k] for k in ('frame', 'master_clock', 'vpos', 'hpos'))
        row['exact'] = row['clock_exact'] and all(c['exact'] for c in row['components'].values())
        result['checkpoints'].append(row)
    result['valid_evidence'] = not left['errors'] and not right['errors']
    result['passed'] = bool(result['valid_evidence'] and not unexpected_inputs
        and all(row['exact'] for row in result['diagnostic_outputs'].values())
        and not mismatches and not result['missing_candidate_tags']
        and not result['additional_candidate_tags'] and result['checkpoints']
        and all(row['exact'] for row in result['checkpoints']))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--interp-control-target', help='Declare the sole baseline-only interpreter target; requires the same EXE/manifest. All observed differences remain reported.')
    parser.add_argument('--native-iterator-control',action='store_true',help='Declare only iterator52 off versus auto/on with the same EXE/manifest. All guest, clock, audio and picture comparisons remain strict.')
    args = parser.parse_args()
    try:
        result = compare_runs(args.baseline, args.candidate, args.interp_control_target,args.native_iterator_control)
    except (OSError, ValueError, KeyError, TypeError) as error:
        result = dict(schema='lagoon strict native replay comparison v2', passed=False,
                      valid_evidence=False, error=str(error))
    args.out.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: result.get(k) for k in ('passed', 'valid_evidence', 'error')}))
    raise SystemExit(0 if result['passed'] else 1 if result['valid_evidence'] else 2)


if __name__ == '__main__':
    main()
