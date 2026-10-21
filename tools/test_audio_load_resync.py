"""Test the actual successful-load audio helper and real APU/SPC/DSP invariants.

Extract current production helper/desktop bodies and load-success conditions;
link the real APU, SPC and DSP. No ROM, save or game executable is used. SDL and
load IO are stubs, so live-device latency, listening and complete load lifecycle
still require separate focused qualification. Serialized output-ring differences
are explicit; no replay comparator is modified or normalized.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def mask_literals(text):
    pattern = r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: ''.join('\n' if x == '\n' else ' ' for x in m[0]), text, flags=re.S)


def block(text, signature):
    masked = mask_literals(text)
    start = masked.index(signature)
    opening = masked.index('{', start)
    level = 1
    for end in range(opening + 1, len(text)):
        level += (masked[end] == '{') - (masked[end] == '}')
        if not level:
            return text[start:end + 1]
    raise ValueError('Unterminated source block: ' + signature)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--compiler', default='clang')
    args = parser.parse_args()
    if args.out.exists():
        parser.error('Choose fresh scratch output')
    repo = Path(__file__).resolve().parents[1]
    sdk = repo / 'snesrecomp/runner/src'
    common = sdk / 'common_rtl.c'
    desktop = sdk / 'desktop/host_main.c'
    c = common.read_text(encoding='utf-8')
    h = desktop.read_text(encoding='utf-8')
    delivery = block(c, 'static void rtl_reset_audio_delivery(void) {')
    loaded = block(c, 'static void rtl_reset_loaded_audio(void) {')
    timeline = block(h, 'static void ResetAudioTimeline(void) {')
    slot = block(h, 'static bool LoadStateSlot(int slot) {')
    gates = []
    checks = []
    for signature, state, label in (
            ('bool RtlLoadSnapshot(const char *filename) {', 'fs', 'file'),
            ('bool RtlLoadSnapshotFromMemory(const void *data, size_t size) {', 'memory', 'memory')):
        source = block(c, signature)
        gate = re.search(r'if\s*\(!' + state + r'\.error\)\s*rtl_reset_audio_delivery\(\);', source)
        if not gate:
            raise AssertionError('Missing original success delivery gate: ' + label)
        callback = source.index('g_rtl_game_info->on_state_loaded(hdr[1]);')
        post_start = source.index('RtlApuLock();', callback)
        post_end = source.index('RtlApuUnlock();', post_start) + len('RtlApuUnlock();')
        post = source[post_start:post_end]
        assert post.count('rtl_reset_loaded_audio();') == 1
        assert source.count('rtl_reset_loaded_audio();') == 1
        assert source.index('RtlApuLock();') < source.index('snes_saveload(g_snes,') < gate.start() < source.index('RtlApuUnlock();') < callback < post_start
        assert source.index('state_load_extra(&') < gate.start()
        assert source.index('if (' + state + '.error)') < callback
        gates.append('static void actual_' + label + '_gate(bool error) {\n'
                     '  struct { bool error; } ' + state + ' = {error};\n'
                     '  RtlApuLock();\n  ' + gate[0] + '\n  RtlApuUnlock();\n'
                     '  if (error) return;\n  restore_execution_stub();\n' + post + '\n}\n')
        checks.append(label + ': original success delivery reset remains locked; new locked helper follows successful execution reconciliation')
    rollback = block(c, 'bool RtlRollbackLoadFromMemory(const void *data, size_t size) {')
    assert rollback.index('s_rollback_load=true;') < rollback.index('RtlLoadSnapshotFromMemory(') < rollback.index('s_rollback_load=previous_rollback_load;')
    assert rollback.index('dsp_output_ring_save(') < rollback.index('RtlLoadSnapshotFromMemory(') < rollback.index('dsp_output_ring_restore(')
    assert 's_state_generation = generation;' in rollback
    checks.append('rollback retains live ring and generation around nested memory load')
    assert timeline.index('RtlApuUnlock();') < timeline.index('SDL_ClearAudioStream(')
    checks.append('desktop SDL stream clear follows APU unlock; scratch reset stays locked')
    resume = block(h, 'if (args.resume_state) {')
    resume_gate = block(resume, 'if (resumed) {')
    assert resume_gate.index('GameReset();') < resume_gate.index('ResetAudioTimeline();')
    assert 'ResetAudioTimeline' not in block(h, 'static void GameReset(void) {')
    checks.append('resume-state success resets timeline; generic GameReset has no new clamp')
    constants = []
    for name in ('RTL_AUDIO_FADE_FRAMES', 'RTL_AUDIO_TARGET_NATIVES'):
        constants.append(re.search(r'^#define\s+' + name + r'\s+[^\n]+', c, re.M)[0])
    generation = block(h, 'if (g_state_generation != RtlStateGeneration()) {')
    assert generation.index('if (g_audio_timeline_generation != RtlStateGeneration())') < generation.index('ResetAudioTimeline();') < generation.index('snes_rewind_shutdown();') < generation.index('snes_rewind_configure();')
    assert 'g_audio_timeline_generation = RtlStateGeneration();' in timeline
    checks.append('audio generation avoids second reset while rewind history is invalidated')
    header = '\n'.join(constants) + '\n' + delivery + '\n' + loaded + '\n' + '\n'.join(gates) + timeline + '\n' + slot
    header += '\nstatic void actual_resume_gate(bool resumed) {\n' + resume_gate + '\n}\n'
    header += '\nstatic void actual_generation_gate(void) {\n' + generation + '\n}\n'
    args.out.mkdir(parents=True)
    extracted = args.out / 'audio_load_under_test.h'
    extracted.write_text(header, encoding='utf-8')
    harness = repo / 'tests/lagoon_audio_load_resync_test.c'
    sources = [harness, *[sdk / ('snes/' + name + '.c') for name in ('apu', 'spc', 'dsp')]]
    executable = args.out / 'audio-load-resync-test.exe'
    command = [args.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-comment',
               '-I' + str(sdk), '-I' + str(sdk / 'snes'), '-I' + str(args.out.resolve()),
               *[str(x) for x in sources], '-o', str(executable)]
    subprocess.run(command, check=True)
    run = subprocess.run([str(executable.resolve())], capture_output=True, text=True)
    print(run.stdout, end='')
    print(run.stderr, end='')
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    result = dict(passed=run.returncode == 0, returncode=run.returncode,
                  sources={str(x.relative_to(repo)): sha(x) for x in (common, desktop, *sources, Path(__file__))},
                  extracted_sha256=sha(extracted), source_gate_checks=checks, stdout=run.stdout, stderr=run.stderr,
                  domains=dict(exact='Complete actual serialized synthesis state except existing output-ring tail; actual future nonzero generated PCM; port state/events/clock; input snapshot bytes untouched.',
                               intentional_difference='Live sampleRead and queued presentation history; future ring enqueue/drop bookkeeping. Complete raw APU blob/delivered PCM equality is not asserted.',
                               mocked='Selector return, SDL, lock sentinels and IO result. Actual source helper/conditions/desktop bodies are extracted; whole file IO/preflight/rollback/game routes are not executed.'),
                  limits='No game/owner-save/actual-device run, no MSU/music cue or audible continuity claim; no comparator changes.')
    (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if not result['passed']:
        raise SystemExit('Actual audio load resync helper failed')


if __name__ == '__main__':
    main()
