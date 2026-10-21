"""Focused, private-ROM validation of load-only actor atlas recovery.

The optional producer fixture is the south checkpoint of the assisted
validation/inputs/batch40/field-cadence.txt route, not arbitrary save data.
No game assets or captured bytes are included in this tool.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--snapshot-prefix", type=Path)
    parser.add_argument("--sdl-include", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    args.rom = args.rom.resolve(strict=True)
    args.compiler = args.compiler.resolve(strict=True)
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    if any(args.out.iterdir()):
        parser.error("--out must be empty; use the active scratch batch")
    if bool(args.snapshot_prefix) != bool(args.sdl_include):
        parser.error("producer validation requires both --snapshot-prefix and --sdl-include")
    receipt = {"rom_sha256": digest(args.rom), "cases": {}, "sources": {}}
    sources = ["src/lagoon_wide_entities.c", "src/lagoon_wide_entities.h",
               "tools/test_wide_loaded_atlas_source.c",
               "tools/test_wide_loaded_atlas_producer.c",
               "tools/test_wide_parallax_sources.c",
               "tools/test_wide_additional_producer.c", "tools/test_wide_loaded_atlas.py"]
    receipt["sources"] = {p: digest(repo / p) for p in sources}

    def run(name, flags, files, inputs):
        exe = args.out / (name + ".exe")
        command = [str(args.compiler), "-std=c11", "-O2", *flags,
                   *files, "-o", str(exe)]
        build = subprocess.run(command, cwd=repo, text=True, capture_output=True)
        (args.out / (name + ".build.txt")).write_text(
            build.stdout + build.stderr, encoding="utf-8")
        build.check_returncode()
        test = subprocess.run([str(exe), *map(str, inputs)], cwd=repo,
                              text=True, capture_output=True)
        (args.out / (name + ".txt")).write_text(test.stdout + test.stderr, encoding="utf-8")
        test.check_returncode()
        receipt["cases"][name] = {"passed": True, "exe_sha256": digest(exe),
                                   "output": test.stdout.strip()}

    run("loaded-atlas-source", ["-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
        "-DLAGOON_WIDE_ENTITIES_DECODER_ONLY", "-Isrc", "-Isnesrecomp/runner/src"],
        ["tools/test_wide_loaded_atlas_source.c", "src/lagoon_wide_entities.c",
         "snesrecomp/runner/src/snes/interp816.c"], [args.rom])
    if args.snapshot_prefix:
        prefix = args.snapshot_prefix.resolve()
        receipt["snapshot_sha256"] = {s: digest(Path(str(prefix) + "." + s))
                                     for s in ("wram.bin", "vram.bin", "cgram.bin")}
        run("loaded-atlas-producer", ["-DSDL_MAIN_HANDLED=1", "-DSNESRECOMP_SDL3=1",
            "-DSNESRECOMP_REVERSE_DEBUG=0", "-Isrc", "-Isnesrecomp/runner/src",
            "-Isnesrecomp/runner/src/snes", "-Isnesrecomp/runner/src/desktop",
            "-I" + str(args.sdl_include.resolve(strict=True)), "-ffunction-sections",
            "-fdata-sections", "-Wl,--gc-sections"],
            ["tools/test_wide_loaded_atlas_producer.c", "src/lagoon_wide_multipart.c",
             "src/lagoon_wide_spell3.c", "src/lagoon_scene_renderer.c",
             "snesrecomp/runner/src/snes/ppu.c", "snesrecomp/runner/src/snes/ppu_legacy.c"],
            [args.rom, prefix])
    (args.out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt["cases"], indent=2))


if __name__ == "__main__":
    main()
