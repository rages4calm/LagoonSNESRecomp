"""Focused presentation protocols; optional private real panel/mosaic inputs.

Only the named production function bodies are compiled into the small protocol
tests. Resource callback vectors are synthetic; optional actual panels and native
map comparisons are separate evidence. No ROM, snapshots or art are embedded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def function(source, name):
    start = source.index(name + "(")
    start = source.rfind("\n", 0, start) + 1
    pos = source.index("{", start)
    depth = 1
    pos += 1
    while depth:
        depth += (source[pos] == "{") - (source[pos] == "}")
        pos += 1
    return source[start:pos] + "\n"


def phase_map(rom_path, prefix, settled_prefix):
    rom = rom_path.read_bytes()
    assert hashlib.sha256(rom).hexdigest() == "46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb"
    ram = Path(str(prefix) + ".wram.bin").read_bytes()
    vram = Path(str(prefix) + ".vram.bin").read_bytes()
    final_vram = Path(str(settled_prefix) + ".vram.bin").read_bytes()
    regs = json.loads(Path(str(prefix) + ".regs.json").read_text(encoding="utf-8"))
    assert len(ram) == 0x20000 and len(vram) == len(final_vram) == 65536
    word = lambda a: int.from_bytes(ram[a:a+2], "little")
    ptr = lambda a: int.from_bytes(ram[a:a+3], "little")

    def read(address):
        bank, low = address >> 16, address & 65535
        if bank in (0x7e, 0x7f):
            return ram[address - 0x7e0000]
        assert (bank & 127) < 32 and low >= 0x8000
        return rom[((bank & 31) << 15) | (low & 32767)]

    def tile(x, y):
        meta = read(ptr(0x88) + (y >> 4) * 128 + (x >> 4))
        address = ptr(0x69) + meta * 8 + ((x & 8) >> 2) + ((y & 8) >> 1)
        return read(address) | read(address+1) << 8

    assert word(0x48e) == 0x2a and ptr(0x57) == 0x14edd5
    assert regs["mosaic"] == 255 and regs["bgmode"] == 9
    assert regs["bgsc"][:2] == [0x71, 0x7a] and regs["bg_tile_adr"] == 0x600
    camera = [word(a) for a in range(0x480, 0x488, 2)]
    assert camera[2:] == [camera[0] >> 2, camera[1] >> 2]
    assert regs["hscroll"][:2] == [camera[0] & 1023, camera[2]]
    assert regs["vscroll"][:2] == [camera[1] & 1023, camera[3]]
    tx, ty = camera[0] // 8 + 1, camera[1] // 8 + 1
    for y in range(24):
        for x in range(30):
            xx, yy = tx+x, ty+y
            address = (0x7000 + ((xx & 32) << 5) + (yy & 31)*32 + (xx & 31))*2
            assert int.from_bytes(vram[address:address+2], "little") == tile(xx*8, yy*8)
    # The exact source-proven static parallax pages do not change at mosaic-clear.
    assert vram[0xf000:0x10000] == final_vram[0xf000:0x10000]
    return dict(room="2a", native_bg1_source_cells=720, bg2_unchanged_bytes=4096,
                mosaic=255, camera=camera, rom_sha256=sha(rom_path),
                inputs={str(p): sha(p) for p in [Path(str(prefix)+s) for s in
                        (".wram.bin", ".vram.bin", ".regs.json")] +
                        [Path(str(settled_prefix)+".vram.bin")]})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--sdl-include", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--menu-prefix", type=Path, action="append", default=[])
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--mosaic-prefix", type=Path)
    parser.add_argument("--settled-prefix", type=Path)
    args = parser.parse_args()
    if any([args.rom, args.mosaic_prefix, args.settled_prefix]) and not all(
            [args.rom, args.mosaic_prefix, args.settled_prefix]):
        parser.error("phase source check requires ROM, mosaic and settled prefixes")
    repo = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        parser.error("--out must be empty")
    source = (repo / "src/lagoon_widescreen.c").read_text(encoding="utf-8")
    entities = (repo / "src/lagoon_wide_entities.c").read_text(encoding="utf-8")
    (out / "lagoon_draw_frame_under_test.inc").write_text(
        function(source, "LagoonWide_DrawFrame"), encoding="utf-8")
    (out / "lagoon_readiness_under_test.inc").write_text("".join(function(source, n) for n in
        ["world_map_geometry", "world_scene_geometry", "initial_world_source_ready"]) +
        function(entities, "LagoonWideEntities_SceneSettled"), encoding="utf-8")
    flags = [str(args.compiler.resolve()), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
             "-Wno-int-in-bool-context", "-I"+str(out), "-Isrc", "-Isnesrecomp/runner/src"]

    def run(command, tag):
        result = subprocess.run(command, cwd=repo, text=True, capture_output=True)
        (out / (tag + ".txt")).write_text(result.stdout+result.stderr, encoding="utf-8")
        result.check_returncode()
        return result.stdout.strip()

    priority = out / "menu-priority.exe"
    run(flags + ["-DSNESRECOMP_SDL3=1", "-DSDL_MAIN_HANDLED", "-Isnesrecomp/runner/src/desktop",
        "-I"+str(args.sdl_include.resolve()), "tests/lagoon_menu_world_priority_test.c",
        "src/lagoon_transition_present.c", "-o", str(priority)], "priority-build")
    results = {"priority": run([str(priority)], "priority")}
    readiness = out / "mosaic-readiness.exe"
    run(flags + ["tests/lagoon_mosaic_readiness_test.c", "-o", str(readiness)], "readiness-build")
    results["readiness"] = run([str(readiness)], "readiness")
    if args.menu_prefix:
        panel = out / "actual-menu.exe"
        run(flags + ["tests/lagoon_menu_presentation_test.c", "-o", str(panel)], "panel-build")
        for i, prefix in enumerate(args.menu_prefix):
            images = out / ("panel"+str(i))
            images.mkdir()
            results["panel"+str(i)] = run([str(panel), str(prefix.resolve()), str(images)], "panel"+str(i))
    if args.rom:
        results["phase_map"] = phase_map(args.rom.resolve(), args.mosaic_prefix.resolve(),
                                          args.settled_prefix.resolve())
        mosaic = out / "incoming-mosaic.exe"
        ppu_flags = [x for x in flags if x not in ("-Wall", "-Wextra", "-Werror")]
        run(ppu_flags + ["tests/lagoon_incoming_mosaic_test.c", "snesrecomp/runner/src/snes/ppu.c",
                     "snesrecomp/runner/src/snes/ppu_legacy.c", "-o", str(mosaic)], "mosaic-build")
        results["original_ppu_mosaic"] = run([str(mosaic), str(args.rom.resolve()),
                                               str(args.mosaic_prefix.resolve())], "mosaic")
    sources = ["src/lagoon_widescreen.c", "src/lagoon_transition_present.c", "src/lagoon_menu_presentation.c",
               "src/lagoon_wide_entities.c", "tests/lagoon_menu_world_priority_test.c",
               "tests/lagoon_mosaic_readiness_test.c", "tests/lagoon_menu_presentation_test.c",
               "tests/lagoon_incoming_mosaic_test.c", "src/lagoon_scene_renderer.c", "src/lagoon_scene_renderer.h",
               "snesrecomp/runner/src/snes/ppu.c", "snesrecomp/runner/src/snes/ppu_legacy.c",
               "tools/test_wide_transition_protocol.py"]
    receipt = dict(passed=True, scope="source-extracted protocol; resource callbacks synthetic; actual panels/map optional",
                   sources={name: sha(repo/name) for name in sources}, results=results)
    (out/"receipt.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
