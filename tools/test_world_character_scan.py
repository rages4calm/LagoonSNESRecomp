"""Compare actual old/new read-only character discovery, including original maps.

The ROM remains external. Original02:C5C6..C64D constructs both map planes for
all75 descriptors. This is a resource scan proof, not live loader/APU timing.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def function(source, name):
    pos = source.index(name + '(')
    start = source.rfind('\n', 0, pos) + 1
    brace = source.index('{', pos)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def discovery(source, name):
    body = function(source, 'capture_world_characters')
    body = body[:body.index('    unsigned torch_bytes=')]
    return body.replace('static int capture_world_characters(const uint8_t *upload)',
                        'static void ' + name + '(void)') + '}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--compiler', default='clang')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    expected = '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
    if sha(args.rom) != expected or args.out.exists():
        parser.error('requires verified original ROM and fresh output directory')
    old = subprocess.check_output(['git', 'show',
        '01646735b418877afcdcca0fbe0ec261f4254cd6:src/lagoon_widescreen.c'],
        cwd=repo).decode('utf-8')
    current = (repo / 'src/lagoon_widescreen.c').read_text(encoding='utf-8')
    # Only map discovery changes. The exact acquisition/VRAM/torch ownership
    # checks and all mutations after it remain byte-for-byte unchanged.
    tail = lambda s: function(s, 'capture_world_characters').split('    unsigned torch_bytes=', 1)[1]
    assert tail(old) == tail(current)
    args.out.mkdir(parents=True)
    definitions = ''.join(function(current, n) for n in ('word', 'pointer'))
    reader = function(current, 'byte_at').replace('{',
        '{\n#ifndef WORLD_SCAN_BENCH\n  ++source_reads;\n#endif', 1)
    definitions += reader + function(current, 'map_tile')
    definitions += function(current, 'mark_map_characters')
    definitions += discovery(old, 'discover_reference') + discovery(current, 'discover_candidate')
    loop = old[old.index('      for(unsigned y=0;y<th;++y)for(unsigned x=0;x<tw;++x)'):]
    loop = loop[:loop.index('\n    }')]
    definitions += ('#ifndef WORLD_SCAN_BENCH\n'
                    'static void discover_layer_reference(unsigned layer,unsigned tw,unsigned th){\n'
                    + loop + '\n}\n#endif\n')
    header = args.out / 'world_character_scan_under_test.h'
    header.write_text(definitions, encoding='utf-8')
    test = repo / 'tests/lagoon_world_character_scan_test.c'
    interp = repo / 'snesrecomp/runner/src/snes/interp816.c'
    exe = args.out / 'world-character-scan.exe'
    subprocess.run([args.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-I' + str(args.out.resolve()), '-I' + str(repo / 'snesrecomp/runner/src'),
        str(test), str(interp), '-o', str(exe)], check=True)
    run = subprocess.run([str(exe.resolve()), str(args.rom.resolve())], text=True, capture_output=True)
    print(run.stdout, end='')
    print(run.stderr, end='')
    bench_exe = args.out / 'world-character-bench.exe'
    subprocess.run([args.compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-DWORLD_SCAN_BENCH', '-I' + str(args.out.resolve()),
        '-I' + str(repo / 'snesrecomp/runner/src'), str(test), str(interp),
        '-o', str(bench_exe)], check=True)
    bench = subprocess.run([str(bench_exe.resolve()), str(args.rom.resolve())],
        text=True, capture_output=True)
    print(bench.stdout, end='')
    print(bench.stderr, end='')
    result = dict(passed=run.returncode == 0 and bench.returncode == 0,
        stdout=run.stdout, stderr=run.stderr,
        microbench_stdout=bench.stdout, microbench_stderr=bench.stderr,
        microbench_scope='Uninstrumented same-source discovery only, actual14/2A maps; three alternating100-call QPC blocks. This is not frame timing or a total game speedup.',
        rom_sha256=expected, original_ref='01646735b418877afcdcca0fbe0ec261f4254cd6',
        unchanged_ownership_tail=True,
        source_sha256={str(p.relative_to(repo)): sha(p) for p in
            [Path(__file__), test, interp, repo / 'src/lagoon_widescreen.c']},
        extracted_sha256=sha(header),
        scope='Original75 ROM room descriptors/maps expanded by original02C5C6..C64D opcodes. Exact1024 used bits and read-only source state, plus synthetic odd/empty/capped extents, fill, aliases, all meta values and parallax. Ownership tail unchanged. No live pacing/APU/campaign claim.')
    (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    raise SystemExit(run.returncode or bench.returncode)


if __name__ == '__main__':
    main()
