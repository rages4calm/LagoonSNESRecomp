"""Qualify room09's original common2 overlap from a fresh private snapshot.

Use the loaded09 checkpoint of the batch42 narrow Voloh route. This executes
original NPC predicates/decompression and checks source/resident mutations.
The ROM and captured bytes remain external private inputs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--snapshot-prefix", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--producer-prefix", type=Path)
    parser.add_argument("--sdl-include", type=Path)
    args = parser.parse_args()
    if bool(args.producer_prefix) != bool(args.sdl_include):
        parser.error("producer validation requires both --producer-prefix and --sdl-include")
    repo = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        parser.error("--out must be empty; use the active scratch batch")
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    rom = args.rom.resolve(strict=True)
    prefix = args.snapshot_prefix.resolve()
    exe = out / "loaded-common2.exe"
    sources = ["tools/test_wide_loaded_common2.c", "tools/test_wide_loaded_common2.py",
               "tools/test_wide_loaded_common2_producer.c", "tools/test_wide_additional_producer.c",
               "tools/test_wide_parallax_sources.c", "src/lagoon_wide_entities.c",
               "src/lagoon_wide_entities.h", "snesrecomp/runner/src/snes/interp816.c"]
    command = [str(args.compiler.resolve(strict=True)), "-std=c11", "-O2", "-Wall",
               "-Wextra", "-Werror", "-Wno-unused-function",
               "-DLAGOON_WIDE_ENTITIES_DECODER_ONLY", "-Isrc", "-Isnesrecomp/runner/src",
               "tools/test_wide_loaded_common2.c", "src/lagoon_wide_entities.c",
               "snesrecomp/runner/src/snes/interp816.c", "-o", str(exe)]
    build = subprocess.run(command, cwd=repo, text=True, capture_output=True)
    (out / "build.txt").write_text(build.stdout + build.stderr, encoding="utf-8")
    build.check_returncode()
    test = subprocess.run([str(exe), str(rom), str(prefix)], cwd=repo,
                          text=True, capture_output=True)
    (out / "test.txt").write_text(test.stdout + test.stderr, encoding="utf-8")
    test.check_returncode()
    receipt = {"passed": True, "rom_sha256": digest(rom),
               "snapshot_sha256": {s: digest(Path(str(prefix) + "." + s))
                                   for s in ("wram.bin", "vram.bin")},
               "sources_sha256": {p: digest(repo / p) for p in sources},
               "exe_sha256": digest(exe), "output": test.stdout.strip(),
               "scope": "Original source/actual resident fixture; no live added-margin pixel or campaign assertion"}
    if args.producer_prefix:
        producer = out / "loaded-common2-producer.exe"
        prefix = args.producer_prefix.resolve()
        command = [str(args.compiler.resolve(strict=True)), "-std=c11", "-O2",
                   "-DSDL_MAIN_HANDLED=1", "-DSNESRECOMP_SDL3=1", "-DSNESRECOMP_REVERSE_DEBUG=0",
                   "-Isrc", "-Isnesrecomp/runner/src", "-Isnesrecomp/runner/src/snes",
                   "-Isnesrecomp/runner/src/desktop", "-I" + str(args.sdl_include.resolve(strict=True)),
                   "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                   "tools/test_wide_loaded_common2_producer.c", "src/lagoon_wide_multipart.c",
                   "src/lagoon_wide_spell3.c", "src/lagoon_scene_renderer.c",
                   "snesrecomp/runner/src/snes/ppu.c", "snesrecomp/runner/src/snes/ppu_legacy.c",
                   "-o", str(producer)]
        build = subprocess.run(command, cwd=repo, text=True, capture_output=True)
        (out / "producer-build.txt").write_text(build.stdout + build.stderr, encoding="utf-8")
        build.check_returncode()
        pixels = subprocess.run([str(producer), str(rom), str(prefix)], cwd=repo,
                                text=True, capture_output=True)
        (out / "producer.txt").write_text(pixels.stdout + pixels.stderr, encoding="utf-8")
        pixels.check_returncode()
        receipt["producer"] = {"passed": True, "exe_sha256": digest(producer),
                               "output": pixels.stdout.strip(),
                               "snapshot_sha256": {s: digest(Path(str(prefix) + "." + s))
                                                   for s in ("wram.bin", "vram.bin", "cgram.bin")}}
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(test.stdout.strip())


if __name__ == "__main__":
    main()
