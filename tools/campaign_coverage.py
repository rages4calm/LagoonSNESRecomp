"""Bind completed replay coverage to receipts and prepare SDK observation bundles.

Never launches the game, edits cfgs, generates code, or certifies boss victories.
Scenario tags are declared test context, kept separate from measured CPU coverage.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / 'snesrecomp/recompiler'))
sys.path.insert(0, str(REPO / 'snesrecomp/tools'))
from v2.coverage_profile import COUNTERS, load_profiles, pc
from tier2_ingest import audit

U64 = (1 << 64) - 1
MAX_FILE = 128 * 1024 * 1024
IDENTITY = ('rom_sha256', 'module_id', 'mapper', 'program_digest', 'build_digest')
DROP_FIELDS = ('overflowed_tuples', 'dropped_cost_samples', 'journal_write_failures', 'ram_routines_overflow')
FATAL_RUNTIME = re.compile(
    r'\[(?:brk|interp_cap|interp_low_exec)\]|\bcpu_halt\b|'
    r'\bassert(?:ion)?[^\r\n]{0,512}\b(?:failed|failure)\b|'
    r'(?:lagoon\.qol:|\[lagoon-qol\])[^\r\n]*\brefused\b', re.IGNORECASE)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def ps_quote(value):
    return "'" + str(value).replace("'", "''") + "'"


def read_bytes(path):
    if not path.is_file() or path.stat().st_size > MAX_FILE:
        raise ValueError(f'missing or oversized input: {path}')
    return path.read_bytes()


def integer(value, name, maximum=U64):
    if type(value) is not int or not 0 <= value <= maximum:
        raise ValueError(f'invalid or overflowing {name}: {value!r}')


def address(value):
    if type(value) is not int and not isinstance(value, str):
        raise ValueError('PC must be an integer or address string')
    return pc(value)


def local(root, name):
    root = root.resolve()
    result = (root / name).resolve()
    if result == root or not result.is_relative_to(root):
        raise ValueError(f'input escapes run directory: {name}')
    return result


def validate(record):
    if not isinstance(record, dict) or record.get('schema') not in (
        'snesrecomp tier2 coverage v2', 'snesrecomp tier2 discovery v2'):
        raise ValueError('campaign inputs require original v2 checkpoint/journal records')
    identity = record.get('identity')
    if not isinstance(identity, dict):
        raise ValueError('missing capture identity')
    for field in ('rom_sha256', 'program_digest', 'build_digest'):
        if not re.fullmatch(r'[0-9a-f]{64}', str(identity.get(field, ''))):
            raise ValueError(f'missing/invalid {field}')
    for field in ('module_id', 'mapper'):
        if not isinstance(identity.get(field), str) or not identity[field]:
            raise ValueError(f'missing {field}')
    if identity.get('generation_digest') and not re.fullmatch(r'[0-9a-f]{64}', str(identity['generation_digest'])):
        raise ValueError('invalid generation digest')
    if not isinstance(record.get('capture_id'), str) or not record['capture_id']:
        raise ValueError('missing capture_id')
    integer(record.get('sequence'), 'sequence')
    for name in DROP_FIELDS:
        integer(record.get(name, 0), name)
    if 'qualified_aot_targets' in record:
        raise ValueError('runtime observations may not inject AOT qualification allowlists')
    for value in record.get('unsafe_aot_targets', []):
        address(value)
    for field in ('discoveries', 'costs', 'ram_routines'):
        if not isinstance(record.get(field, []), list):
            raise ValueError(f'{field} must be an array')
    rows = record.get('discoveries', []) + record.get('costs', [])
    if record['schema'] == 'snesrecomp tier2 discovery v2':
        rows = [record.get('row')]
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError('invalid coverage row')
        address(row.get('site_pc24', 0)); address(row['target_pc24'])
        if not re.fullmatch(r'M[01]X[01]', str(row.get('entry_mx', ''))):
            raise ValueError('missing/invalid exact entry width')
        integer(row.get('emulation'), 'emulation', 1)
        for field in COUNTERS:
            integer(row.get(field, 0), field)
        integer(row.get('sequence', record['sequence']), 'row sequence')
    for row in record.get('ram_routines', []):
        if not isinstance(row, dict):
            raise ValueError('invalid RAM snapshot')
        address(row['entry_pc24']); integer(row.get('hits', 0), 'RAM hits')
        raw = row.get('bytes', '')
        if not isinstance(raw, str) or not re.fullmatch(r'(?:[0-9a-fA-F]{2}){1,512}', raw):
            raise ValueError('invalid or oversized RAM bytes')
    return identity


def records(path, raw):
    text = raw.decode('utf-8')
    if path.suffix != '.jsonl':
        return [json.loads(text)], False
    result = []
    lines = text.splitlines(keepends=True)
    for index, line in enumerate(lines):
        try:
            result.append(json.loads(line))
        except json.JSONDecodeError:
            if index == len(lines)-1 and not line.endswith('\n'):
                return result, True
            raise ValueError(f'malformed complete journal record: {path}:{index+1}')
    return result, False


def frozen_profile(items, expected_rom, parent):
    # The SDK reader owns merging semantics. Feed it frozen records, not source
    # files that could change between validation and publishing the bundle.
    with tempfile.TemporaryDirectory(prefix='.campaign-coverage-', dir=parent) as temp:
        target = Path(temp).resolve()
        if not target.is_relative_to(parent.resolve()):
            raise ValueError('temporary audit path escaped output parent')
        bundle = target/'bundle.json'
        bundle.write_text(json.dumps(dict(schema='snesrecomp tier2 bundle v2', records=list(items))), encoding='utf-8')
        return load_profiles([bundle], expected_rom=expected_rom)


def collect(registry, out, allow_incomplete=False):
    registry = registry.resolve(); out = out.resolve()
    plan_raw = read_bytes(registry); plan = json.loads(plan_raw)
    if plan.get('schema') != 'lagoon campaign coverage plan v1':
        raise ValueError('unsupported campaign registry schema')
    expected_rom = plan.get('rom_sha256', '')
    if not re.fullmatch(r'[0-9a-f]{64}', str(expected_rom)):
        raise ValueError('registry requires verified ROM SHA256')
    expected = plan.get('expected_scenarios', [])
    if not isinstance(expected, list) or not expected:
        raise ValueError('expected_scenarios must be a nonempty list')
    ids = [r.get('id') for r in expected if isinstance(r, dict)]
    if len(ids) != len(expected) or any(not isinstance(x, str) or not x for x in ids) or len(set(ids)) != len(ids):
        raise ValueError('scenario IDs must be unique nonempty strings')
    runs = plan.get('runs', [])
    if not isinstance(runs, list):
        raise ValueError('runs must be a list')
    if out.exists():
        raise ValueError('output must be a fresh directory')
    if not out.parent.is_dir():
        raise ValueError('output parent must already exist')
    groups = {}; run_reports = []; used_ids = set(); capture_runs = {}
    for item in runs:
        rid = item['id']; tags = item.get('scenarios', [])
        if not isinstance(rid, str) or not rid or rid in used_ids:
            raise ValueError('run IDs must be unique nonempty strings')
        used_ids.add(rid)
        if not isinstance(tags, list) or any(tag not in ids for tag in tags) or len(set(tags)) != len(tags):
            raise ValueError(f'unknown/duplicate scenario tag: {rid}')
        directory = (registry.parent / item['directory']).resolve()
        if out == directory or out.is_relative_to(directory) or directory.is_relative_to(out):
            raise ValueError('output overlaps an input run')
        raw_receipt = read_bytes(directory/'receipt.json'); receipt = json.loads(raw_receipt)
        if receipt.get('exit_code') != 0 or not receipt.get('dump_tags') or not receipt.get('script_only_input'):
            raise ValueError(f'run must finish successfully with isolated script input and dumps: {rid}')
        if receipt.get('runtime_failures') or ('qualification_passed' in receipt and receipt['qualification_passed'] is not True):
            raise ValueError(f'runtime failure or rejected qualification receipt: {rid}')
        log_hashes = {}
        for filename in ('stderr.log', 'stdout.log'):
            raw_log = read_bytes(directory/filename)
            log_hashes[filename] = sha(raw_log)
            match = FATAL_RUNTIME.search(raw_log.decode('utf-8', errors='replace'))
            if match:
                raise ValueError(f'fatal runtime marker in {rid}/{filename}: {match.group(0)}')
        program_raw = read_bytes(directory/'program_manifest.json'); program_digest = sha(program_raw)
        if program_digest != receipt.get('program_manifest_sha256') or receipt.get('rom_sha256') != expected_rom:
            raise ValueError(f'run manifest/ROM identity mismatch: {rid}')
        route_raw = read_bytes(directory/'route.txt')
        if sha(route_raw) != receipt.get('route_sha256'):
            raise ValueError(f'run route differs from receipt: {rid}')
        dump_hashes = {}
        for tag in receipt['dump_tags']:
            if not isinstance(tag, str):
                raise ValueError('invalid dump tag')
            dump_hashes[tag] = {}
            for suffix in ('wram.bin', 'fb.bmp', 'regs.json', 'vram.bin', 'oam.bin', 'cgram.bin', 'info.json'):
                dump_hashes[tag][suffix] = sha(read_bytes(local(directory, f'dumps/{tag}.{suffix}')))
        coverage = local(directory, 'coverage')
        inputs = sorted(p for p in coverage.rglob('*') if p.suffix in ('.json', '.jsonl')) if coverage.is_dir() else []
        if not inputs:
            raise ValueError(f'no runtime coverage files: {rid}; collect with --coverage')
        run_records = []; evidence = []; warnings = []
        for path in inputs:
            path = local(directory, str(path.relative_to(directory)))
            raw = read_bytes(path); parsed, torn = records(path, raw)
            evidence.append(dict(path=str(path), sha256=sha(raw), bytes=len(raw), recovered_torn_tail=torn))
            if torn: warnings.append('torn final journal line; recent observations may be lost')
            for record in parsed:
                identity = validate(record)
                if identity['mapper'] != 'lorom' or identity['module_id'] != 'main':
                    raise ValueError('Lagoon campaign captures require main-module LoROM identity')
                if identity['rom_sha256'] != expected_rom or identity['program_digest'] != program_digest or identity['build_digest'] != receipt.get('exe_sha256'):
                    raise ValueError(f'capture does not belong to its replay executable/program/ROM: {path}')
                if any(record.get(k, 0) for k in DROP_FIELDS):
                    warnings.append('bounded capture overflow or journal write failure')
                key = tuple(identity[k] for k in IDENTITY)
                group = groups.setdefault(key, dict(identity=identity, records={}, paths=[], runs=[], program=program_raw))
                if group['identity'] != identity:
                    raise ValueError('contradictory identity metadata within one build group')
                group['records'][sha(json.dumps(record, sort_keys=True).encode())] = record
                group['paths'].append(path)
                if rid not in group['runs']: group['runs'].append(rid)
                session = record['capture_id']
                prior = capture_runs.setdefault(session, (key, rid))
                if prior[0] != key:
                    raise ValueError('capture ID reused across different build identities')
                if prior[1] != rid:
                    warnings.append(f'capture session is already attributed to run {prior[1]}')
                run_records.append(record)
        profile = frozen_profile(run_records, expected_rom, out.parent)
        warnings.extend(profile.warnings)
        if not run_records or not all(any(r.get('capture_id') == session and r.get('checkpoint_complete') is True for r in run_records) for session in profile.captures):
            warnings.append('missing complete checkpoint for a capture session')
        warnings = sorted(set(warnings))
        if warnings and not allow_incomplete:
            raise ValueError(f'incomplete capture {rid}: '+ '; '.join(warnings))
        run_reports.append(dict(id=rid, directory=str(directory), scenarios=tags,
            context=item.get('context', {}), context_is_declared_not_victory_proof=True,
            receipt_sha256=sha(raw_receipt), route_sha256=sha(route_raw),
            runtime_logs_sha256=log_hashes,
            exe_sha256=receipt['exe_sha256'], program_manifest_sha256=program_digest,
            enabled_package_features=receipt.get('enabled_package_features', []),
            input_config_sha256=receipt.get('input_config_sha256'),
            effective_config_sha256=receipt.get('effective_config_sha256'),
            seeded_save_files=receipt.get('seeded_save_files', []), dump_hashes=dump_hashes,
            capture_ids=sorted(profile.captures), warnings=warnings, ingest_ready=not warnings,
            interpreted_instructions=sum(r['interpreted_instructions'] for r in profile.costs),
            interpreted_guest_cycles=sum(r['guest_cycles'] for r in profile.costs),
            instruction_variants=len(profile.costs), transfer_variants=len(profile.discoveries),
            coverage_inputs=evidence))
    # Validate whole same-build groups before publishing any output. SDK keeps
    # latest cumulative rows per session before summing independent sessions.
    group_reports = []
    for key, group in sorted(groups.items()):
        profile = frozen_profile(group['records'].values(), expected_rom, out.parent)
        for row in profile.discoveries + profile.costs:
            for field in COUNTERS: integer(row[field], f'merged {field}')
        report = audit(profile, program=json.loads(group['program']))
        name = sha(json.dumps(key).encode())
        group['audit'] = report
        group_reports.append(dict(id=name, identity=group['identity'], runs=group['runs'],
            ingest_manifest=f'groups/{name}/coverage.bundle.json',
            capture_count=report['capture_count'], warnings=report['warnings'],
            interpreted_instructions=report['interpreted_instructions'], interpreted_guest_cycles=report['interpreted_guest_cycles'],
            instruction_variants=len(profile.costs),
            unique_instruction_addresses=len({(r.get('processor', 'snes_cpu'), pc(r['target_pc24'])) for r in profile.costs}),
            transfer_variants=len(report['discoveries']), candidate_status_counts=dict(Counter(r['candidate_status'] for r in report['discoveries'])),
            audit=f'groups/{name}/audit.json'))
    scenarios = []
    for definition in expected:
        matching = [r for r in run_reports if definition['id'] in r['scenarios']]
        scenarios.append(dict(definition, status='tagged_runs' if matching else 'absent',
            run_ids=[r['id'] for r in matching], clean_capture_run_ids=[r['id'] for r in matching if r['ingest_ready']],
            victory_or_complete_boss_coverage_proven=False))
    summary = dict(schema='lagoon campaign coverage summary v1', registry_sha256=sha(plan_raw),
        collector_sha256=sha(Path(__file__).read_bytes()),
        sdk_reader_sha256=sha((REPO/'snesrecomp/recompiler/v2/coverage_profile.py').read_bytes()),
        sdk_audit_sha256=sha((REPO/'snesrecomp/tools/tier2_ingest.py').read_bytes()),
        rom_sha256=expected_rom, runs=run_reports, groups=group_reports, scenarios=scenarios,
        native_completion_claim=False, generation_performed=False,
        ingestion_ready=bool(run_reports) and all(r['ingest_ready'] for r in run_reports),
        limits=['Scenario labels/context are declared setup, never proof of genuine boss behavior, damage, defeat or rewards.',
            'Coverage measures executed interpreter instructions/transfers, not whole ROM functions or full AOT gameplay.',
            'Build groups keep exclusive costs separate; captures do not authorize native roots, widths, timing or body semantics.'])
    out.mkdir(parents=True, exist_ok=False)
    def write(path, value):
        with path.open('x', encoding='utf-8', newline='\n') as f:json.dump(value, f, indent=2);f.write('\n')
    for metadata, (_, group) in zip(group_reports, sorted(groups.items())):
        directory = out/'groups'/metadata['id']; directory.mkdir(parents=True)
        write(directory/'coverage.bundle.json', dict(schema='snesrecomp tier2 bundle v2', records=list(group['records'].values())))
        (directory/'program_manifest.json').write_bytes(group['program'])
        write(directory/'audit.json', group['audit'])
    write(out/'summary.json', summary)
    recipe = ['# Campaign coverage: review before generation', '',
        'No generation was executed. Scenario tags are not boss completion proof.',
        'Use each same-build bundle separately. Audit against its original manifest:', '']
    for group in group_reports:
        relative = group['ingest_manifest']; original = str(Path(relative).parent/'program_manifest.json')
        recipe += [f'python -P snesrecomp/tools/tier2_ingest.py {ps_quote(out/relative)} --rom-sha256 {expected_rom} --program-manifest {ps_quote(out/original)} --cfg-dir recomp --json', '']
        if summary['ingestion_ready']:
            recipe += [f'python -P snesrecomp/snesrecomp_cli.py generate --rom \'<private-ROM>\' --cfg-dir \'<private-reviewed-cfg-copy>\' --out-dir \'<fresh-private-gen>\' --expected-sha256 {expected_rom} --profile-manifest {ps_quote(out/relative)}', '']
    recipe += ['These are candidate-generation recipes, NOT an accepted native build.',
        'Preserve existing roots/exclusions and timing settings. Profile seeds may emit newly eligible bodies.',
        'Inspect exact M/X/E, callers, bytes, exits, interpreter defaults and generated closure; replay before promotion.',
        'Older-build bundles may be passed with --historical-profile-manifest; never merge their costs into a current build.',
        'A journal-only/overflow capture is incomplete; recollect before treating costs as complete.']
    (out/'INGESTION.md').write_text('\n'.join(recipe)+'\n', encoding='utf-8')
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--registry', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--allow-incomplete', action='store_true', help='Salvage audit only; disables generation recipe when any capture is incomplete')
    args = parser.parse_args()
    try:
        report = collect(args.registry, args.out, args.allow_incomplete)
    except (ValueError, OSError, KeyError, TypeError) as exc:
        parser.error(str(exc))
    print(json.dumps(dict(runs=len(report['runs']), groups=len(report['groups']),
        absent_scenarios=[s['id'] for s in report['scenarios'] if s['status']=='absent'],
        ingestion_ready=report['ingestion_ready'], generation_performed=False)))


if __name__ == '__main__': main()
