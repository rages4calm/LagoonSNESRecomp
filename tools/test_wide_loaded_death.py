"""Check original common0 loading and optional actual staged death-packet pixels.

ROM and snapshot bytes are private external inputs. The optional producer models
one completed original OAM DMA from captured staged bytes; live timing, actual
latched OAM and center comparisons remain separate replay evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
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
    compiler = args.compiler.resolve(strict=True)
    sources = ["tools/test_wide_loaded_death.py", "tools/test_wide_loaded_death_source.c",
               "tools/test_wide_loaded_death_producer.c", "tools/test_wide_additional_producer.c",
               "tools/test_wide_parallax_sources.c", "src/lagoon_wide_entities.c",
               "src/lagoon_wide_entities.h", "src/lagoon_wide_multipart.c",
               "src/lagoon_wide_spell3.c", "src/lagoon_scene_renderer.c",
               "snesrecomp/runner/src/snes/interp816.c",
               "snesrecomp/runner/src/snes/ppu.c", "snesrecomp/runner/src/snes/ppu_legacy.c"]

    def run(command, name):
        result = subprocess.run(command, cwd=repo, text=True, capture_output=True)
        (out / (name + ".txt")).write_text(result.stdout + result.stderr, encoding="utf-8")
        result.check_returncode()
        return result

    exe = out / "loaded-death-source.exe"
    run([str(compiler), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
         "-Wno-unused-function", "-DLAGOON_WIDE_ENTITIES_DECODER_ONLY",
         "-Isrc", "-Isnesrecomp/runner/src", "tools/test_wide_loaded_death_source.c",
         "src/lagoon_wide_entities.c", "snesrecomp/runner/src/snes/interp816.c",
         "-o", str(exe)], "source-build")
    test = run([str(exe), str(rom)], "source")
    receipt = {"schema": "lagoon loaded common0 focused source and producer v1",
               "passed": True, "rom_sha256": digest(rom),
               "sources_sha256": {p: digest(repo / p) for p in sources},
               "exe_sha256": digest(exe), "output": test.stdout.strip(),
               "scope": "Original loader/source and ownership contracts; no campaign assertion"}
    if args.producer_prefix:
        producer = out / "loaded-death-producer.exe"
        prefix = args.producer_prefix.resolve()
        run([str(compiler), "-std=c11", "-O2", "-DSDL_MAIN_HANDLED=1",
             "-DSNESRECOMP_SDL3=1", "-DSNESRECOMP_REVERSE_DEBUG=0", "-Isrc",
             "-Isnesrecomp/runner/src", "-Isnesrecomp/runner/src/snes",
             "-Isnesrecomp/runner/src/desktop", "-I" + str(args.sdl_include.resolve(strict=True)),
             "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
             "tools/test_wide_loaded_death_producer.c", "src/lagoon_wide_multipart.c",
             "src/lagoon_wide_spell3.c", "src/lagoon_scene_renderer.c",
             "snesrecomp/runner/src/snes/ppu.c", "snesrecomp/runner/src/snes/ppu_legacy.c",
             "-o", str(producer)], "producer-build")
        pixels = run([str(producer), str(rom), str(prefix)], "producer")
        receipt["producer"] = {"passed": True, "exe_sha256": digest(producer),
                               "output": pixels.stdout.strip(),
                               "snapshot_sha256": {s: digest(Path(str(prefix) + "." + s))
                                                   for s in ("wram.bin", "vram.bin", "cgram.bin")},
                               "scope": "Actual captured staged OAM packet at modeled completed DMA boundary; independent planar/palette decoder, not live latched-OAM timing proof"}
        print(pixels.stdout.strip())
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(test.stdout.strip())


if __name__ == "__main__":
    main()
