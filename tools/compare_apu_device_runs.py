"""Separate device-domain evidence; never turns a raw strict failure into PASS.

CPU request timestamps belong to the CPU. Queued apply timestamps belong to
the APU port clock. The legacy trace's CPU beam and produced-sample counter
at deferred apply are observation fields, not an APU-cycle timestamp.
"""
import argparse
import csv
import importlib.util
import json
import re
from pathlib import Path

_spec = importlib.util.spec_from_file_location('compare_native_runs',
    Path(__file__).with_name('compare_native_runs.py'))
strict = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(strict)

QUEUE_ENV = 'SNESRECOMP_APU_QUEUE_TRACE'
MAX_QUEUE_EVENTS = 131072
AUDIO_ENVS = ('SNESRECOMP_APURAM_TRACE_FILE', 'SNESRECOMP_DSPREG_TRACE_FILE',
              'SNESRECOMP_DSPOUT')
QUEUE_COLUMNS = ['event', 'port', 'value', 'guest_cycle', 'target_cycle', 'port_clock']
PORT_COLUMNS = ['# frame', 'scanline', 'hclock', 'event', 'port', 'value', 'sample']


def exact_rows(left, right):
    mismatch = next((i for i in range(max(len(left), len(right)))
                     if i >= len(left) or i >= len(right) or left[i] != right[i]), None)
    return dict(exact=mismatch is None, baseline_count=len(left), candidate_count=len(right),
                first_mismatch=None if mismatch is None else dict(index=mismatch,
                    baseline=left[mismatch] if mismatch < len(left) else None,
                    candidate=right[mismatch] if mismatch < len(right) else None))


def load_queue(path):
    rows = {'Q': [], 'A': []}
    complete = False
    count = 0
    with path.open(encoding='utf-8', newline='') as stream:
        reader = csv.reader(stream)
        if next(reader, None) != QUEUE_COLUMNS:
            raise ValueError('unexpected APU queue columns')
        for index, cells in enumerate(reader, 2):
            if complete:
                raise ValueError('data after APU queue completion footer')
            if cells and cells[0] == 'END':
                if len(cells) != 3 or int(cells[1]) != count or int(cells[2]) != 0:
                    raise ValueError('incomplete or overflowed APU queue completion footer')
                complete = True
                continue
            if len(cells) != len(QUEUE_COLUMNS) or count >= MAX_QUEUE_EVENTS:
                raise ValueError(f'incomplete APU queue row {index}')
            row = dict(zip(QUEUE_COLUMNS, cells))
            kind = row['event']
            if kind not in rows:
                raise ValueError(f'unknown or overflow APU queue row {index}: {kind}')
            try:
                values = {key: int(row[key], 10) for key in QUEUE_COLUMNS[1:]}
            except ValueError as error:
                raise ValueError(f'non-numeric APU queue row {index}') from error
            if any(value < 0 or value > (1 << 64) - 1 for value in values.values()):
                raise ValueError(f'invalid APU queue integer at row {index}')
            if values['port'] > 3 or values['value'] > 255:
                raise ValueError(f'invalid APU queue port/value at row {index}')
            if kind == 'A' and (values['guest_cycle'] or
                               values['port_clock'] < values['target_cycle']):
                raise ValueError(f'invalid APU application row {index}')
            if kind == 'Q' and values['target_cycle'] < values['port_clock']:
                raise ValueError(f'APU request target precedes scheduler clock at row {index}')
            fields = ('guest_cycle', 'target_cycle', 'port', 'value') if kind == 'Q' else (
                'target_cycle', 'port_clock', 'port', 'value')
            rows[kind].append(tuple(values[key] for key in fields))
            count += 1
    if not complete or not rows['Q'] or not rows['A']:
        raise ValueError('APU queue capture needs actual requests/applications and completion footer')
    return rows


def cpu_requests(path):
    rows = {'cpu_wr': [], 'cpu_rd': []}
    with path.open(encoding='utf-8', newline='') as stream:
        reader = csv.DictReader(stream, delimiter='\t')
        if reader.fieldnames != PORT_COLUMNS:
            raise ValueError(f'unexpected legacy port columns: {path.name}')
        for row in reader:
            if None in row or None in row.values():
                raise ValueError(f'incomplete legacy port row: {path.name}')
            if row['event'] not in ('cpu_wr', 'cpu_rd', 'cpu_ap', 'spc_rd', 'spc_wr'):
                raise ValueError(f'unknown legacy port event: {path.name}')
            values = tuple(int(row[key], 10) for key in PORT_COLUMNS[:3])
            port, value, sample = int(row['port'], 10), int(row['value'], 16), int(row['sample'], 10)
            if min(*values, port, value, sample) < 0 or port > 3 or value > 255:
                raise ValueError(f'invalid legacy port event: {path.name}')
            if row['event'] in rows:
                rows[row['event']].append((*values, port, value))
    return rows


def source_proof(path, runs):
    proof = json.loads(path.read_text(encoding='utf-8'))
    if proof.get('schema') != 'lagoon read-only APU queue trace proof v1' or not all(
            proof.get(key) is True for key in ('read_only', 'disabled_by_default', 'buffered_until_exit')):
        raise ValueError('missing explicit reviewed read-only instrumentation proof')
    source = Path(proof['source']['path'])
    if not source.is_absolute() or not re.fullmatch(r'[0-9a-f]{64}', proof['source']['sha256']):
        raise ValueError('instrumentation proof needs an absolute source path and SHA256')
    if not source.is_file() or strict.sha(source) != proof['source']['sha256']:
        raise ValueError('instrumentation source proof does not bind current source')
    if not isinstance(proof.get('instrumented_executables'), list) or not all(
            isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value)
            for value in proof['instrumented_executables']):
        raise ValueError('instrumentation proof has invalid executable identities')
    for run in runs:
        if QUEUE_ENV in run['captures'] and run['receipt']['exe_sha256'] not in proof.get('instrumented_executables', []):
            raise ValueError('queue capture executable is absent from instrumentation proof')
    return dict(path=str(path.resolve()), sha256=strict.sha(path), source=proof['source'],
                instrumented_executables=proof['instrumented_executables'])


def compare_runs(baseline, candidate, proof_path, interp_control_target=None,
                 allow_baseline_without_queue=False):
    raw = strict.compare_runs(baseline, candidate, interp_control_target)
    left, right = strict.load_run(baseline), strict.load_run(candidate)
    proof = source_proof(Path(proof_path), (left, right))
    errors = {name: list(run['errors']) for name, run in [('baseline', left), ('candidate', right)]}
    result = dict(schema='lagoon APU device-domain comparison v1', raw_strict=raw,
                  raw_strict_passed=raw['passed'], instrumentation_proof=proof,
                  invalid_evidence=errors)
    inputs = {}
    for key in (*strict.INPUT_KEYS, 'real_audio_device'):
        if key == 'diagnostic_overrides':
            continue
        a, b = left['receipt'].get(key), right['receipt'].get(key)
        if a != b:
            inputs[key] = dict(baseline=a, candidate=b)
    overrides = [dict(run['normalized_overrides'] or {}) for run in (left, right)]
    # Only reviewed diagnostic output destinations are removed from input
    # comparison. Their existence/content is checked in its own domain below.
    for values in overrides:
        for key in (*strict.STRICT_CAPTURE_PATHS, *strict.ADVISORY_REPORT_PATHS):
            values.pop(key, None)
    if interp_control_target is not None:
        if not raw.get('execution_control', {}).get('approved'):
            inputs['execution_control'] = raw.get('execution_control')
        else:
            overrides[0].pop('SNESRECOMP_LLE_INTERP_TARGET', None)
    if overrides[0] != overrides[1]:
        inputs['diagnostic_overrides'] = dict(baseline=overrides[0], candidate=overrides[1])
    seeds = [sorted((Path(row['copy']).name, row['sha256']) for row in
                    run['receipt'].get('seeded_save_files', [])) for run in (left, right)]
    if seeds[0] != seeds[1]:
        inputs['seeded_save_files'] = dict(baseline=seeds[0], candidate=seeds[1])
    warps = [(run['receipt'].get('campaign_warps') or {}).get('sha256') for run in (left, right)]
    if warps[0] != warps[1]:
        inputs['campaign_warps'] = dict(baseline=warps[0], candidate=warps[1])
    result['unapproved_input_differences'] = inputs
    result['ordered_presentations'] = raw['ordered_presentations']
    result['checkpoints'] = []
    for checkpoint in raw['checkpoints']:
        tag = checkpoint['tag']
        components = {key: value for key, value in checkpoint['components'].items() if key != 'apu.tsv'}
        request_streams = [cpu_requests(run['directory'] / 'dumps' / f'{tag}.apu.tsv')
                           for run in (left, right)]
        requests = {kind: exact_rows(request_streams[0][kind], request_streams[1][kind])
                    for kind in ('cpu_wr', 'cpu_rd')}
        result['checkpoints'].append(dict(tag=tag, non_apu_components=components,
            clock_exact=checkpoint['clock_exact'], cpu_request_streams=requests,
            exact=checkpoint['clock_exact'] and all(row['exact'] for row in components.values())
                  and all(row['exact'] for row in requests.values())))
    result['queue_streams'] = {}
    queues = []
    for name, run in [('baseline', left), ('candidate', right)]:
        capture = run['captures'].get(QUEUE_ENV)
        if not capture:
            if name != 'baseline' or not allow_baseline_without_queue:
                errors[name].append('missing authoritative APU queue capture')
            queues.append(None)
        else:
            queues.append(load_queue(run['directory'] / capture['relative_path']))
    complete = all(queue is not None for queue in queues)
    if complete:
        result['queue_streams'] = {kind: exact_rows(queues[0][kind], queues[1][kind]) for kind in ('Q', 'A')}
    result['device_queue_complete'] = complete
    result['audio_captures'] = {}
    for key in AUDIO_ENVS:
        captures = [run['captures'].get(key) for run in (left, right)]
        if any(captures):
            result['audio_captures'][key] = dict(baseline=captures[0], candidate=captures[1],
                exact=all(captures) and captures[0]['sha256'] == captures[1]['sha256'] and
                      captures[0]['size'] == captures[1]['size'])
    result['valid_evidence'] = not any(errors.values())
    result['request_and_audio_projection_passed'] = bool(result['valid_evidence'] and not inputs and
        raw['ordered_presentations']['exact'] and not raw['missing_candidate_tags'] and
        not raw['additional_candidate_tags'] and result['checkpoints'] and
        all(row['exact'] for row in result['checkpoints']) and
        all(row['exact'] for row in result['audio_captures'].values()))
    result['passed'] = bool(result['request_and_audio_projection_passed'] and complete and
                            all(row['exact'] for row in result['queue_streams'].values()))
    result['limitations'] = ('This separate device-domain result never changes raw strict status. '
        'Legacy deferred-apply CPU beam/current-produced-sample observations remain in raw_strict. '
        'Missing historical queue evidence cannot pass the device queue gate; optional absent '
        'APU-RAM/DSP-register/PCM streams are not claimed as verified.')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--source-proof', type=Path, required=True)
    parser.add_argument('--interp-control-target')
    parser.add_argument('--allow-baseline-without-queue', action='store_true')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = compare_runs(args.baseline, args.candidate, args.source_proof,
                              args.interp_control_target, args.allow_baseline_without_queue)
    except (OSError, ValueError, KeyError, TypeError) as error:
        result = dict(schema='lagoon APU device-domain comparison v1', passed=False,
                      valid_evidence=False, error=str(error))
    args.out.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: result.get(key) for key in
                     ('passed', 'valid_evidence', 'raw_strict_passed', 'device_queue_complete', 'error')}))
    raise SystemExit(0 if result['passed'] else 1 if result['valid_evidence'] else 2)


if __name__ == '__main__':
    main()
