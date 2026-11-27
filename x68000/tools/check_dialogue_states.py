"""Cold-load real dialogue states in isolated X68000 preview runtimes.

Inputs are private, explicit paths; no ROM, state or owner save is bundled.
The diagnostic player must contain the same guest program as the playable build.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_wide_areas import INPUT, saved_room
from tas_route import Snapshot, view_summary


def check(player, identity, source, output, case):
    name, area, x, y, zoom = case
    work = output / f"{name}-zoom{zoom}"
    work.mkdir()
    game = work / "Game.exe"
    game.hardlink_to(player)
    media = work / "datas/games" / ("x68-" + identity) / "media"
    media.mkdir(parents=True)
    disk = media / "disk-05.xdf"
    disk.write_bytes(saved_room(source, area, x, y))
    route = INPUT.partition("12000")[0]
    route += "12000 12070 joy 4 1\n12085 12093 joy 8 1\n" if name == "house" else \
             "12000 12016 joy 4 1\n12030 12038 joy 8 1\n"
    (work / "input.tsv").write_text(route, encoding="ascii")

    def run(label, frames, extra):
        command = [str(game), "--headless", "--mute", "--auto-disk", "--lagoon-guest-keys",
                   "--lagoon-zoom", str(zoom), "--lagoon-frame-trace", label + ".csv",
                   "--frames", str(frames), "--input-events", "input.tsv",
                   "--report", label + ".json", *extra]
        result = subprocess.run(command, cwd=work, capture_output=True, text=True,
                                timeout=120, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (work / (label + ".stderr")).write_text(result.stderr)
        report = json.loads((work / (label + ".json")).read_text())
        if result.returncode or report["core_error"] or report["frames"] != frames:
            raise ValueError(f"{work.name}/{label}: {result.stderr[-1500:]}")
        return report, view_summary(work / (label + ".csv"))

    before, _ = run("save", 12400, ["--state-slot", "dialogue.rrstate", "--state-save-at", "12399"])
    state = Snapshot(work / "dialogue.rrstate")
    if not state.observe()["modal"]:
        raise ValueError(f"{work.name}: intended dialogue was not reached.")
    core_size = struct.unpack_from("<Q", state.data, 16)[0]
    aux = 168 + core_size
    media_size = struct.unpack_from("<Q", state.data, aux + 32)[0]
    view = aux + 64 + media_size
    header = struct.unpack_from("<8I3Q", state.data, view)
    if header[0] != 2 or header[1] != struct.calcsize("<8I3Q") or \
            header[4] != 1 or header[5] != area or not header[10]:
        raise ValueError(f"{work.name}: missing serialized backdrop: {header}")
    restored, picture = run("restore", 300, ["--state-slot", "dialogue.rrstate", "--state-load-start", "--state-save-at", "299",
                                             "--lagoon-wide-picture", "restored.bmp"])
    expected = ((456, 256), (568, 320), (684, 384))[zoom]
    if picture["canvases"] != [expected] or picture["center_mismatch_frames"] or \
            picture["dialogue_frames"] != 300 or picture["single_frame_margin_drop_candidates"]:
        raise ValueError(f"{work.name}: restored presentation differs: {picture}")
    # A cold startup frame initializes then restores the core. Match the actual
    # guest hardware frame count, rather than equating host and guest frames.
    reference_frames = 12400 + restored["hardware"]["frames"] - before["hardware"]["frames"]
    reference, _ = run("continuous", reference_frames, ["--state-slot", "continuous.rrstate",
                                                       "--state-save-at", str(reference_frames - 1)])
    a, b = Snapshot(work / "continuous.rrstate"), Snapshot(work / "dialogue.rrstate")
    # Full RAM includes inactive Human68k interrupt-stack remnants below the
    # game. Compare the entire pinned Lagoon image/state range byte-for-byte;
    # record full-RAM and peripheral phase differences separately.
    game_equal = a.data[a.ram:a.ram + 0x20000] == b.data[b.ram:b.ram + 0x20000]
    bus_a, bus_b = a.ram - 0x7EFC0, b.ram - 0x7EFC0
    differences = [i for i in range(0x200000) if a.data[bus_a + i] != b.data[bus_b + i]]
    phase = {key: restored["hardware"][key] - value for key, value in reference["hardware"].items()
             if restored["hardware"][key] != value}
    registers_match = reference["cpu"] == restored["cpu"]
    # Independently reproduced with the immutable pre-fix player. Pin only
    # these two observed 1.8.0 wait-loop phases, not an arbitrary tolerance.
    inherited_phase = phase == {"cpu_cycles": -8, "peripheral_ticks": -8} and registers_match and name == "town"
    inherited_phase |= name == "house" and phase == {"cpu_cycles": 6, "peripheral_ticks": 6} and \
        reference["cpu"][1:] == restored["cpu"][1:] and \
        (reference["cpu"][0], restored["cpu"][0]) == (0x84F3E, 0x84F36)
    def backdrop_data(state):
        core = struct.unpack_from("<Q", state.data, 16)[0]
        auxiliary = 168 + core
        disks = struct.unpack_from("<Q", state.data, auxiliary + 32)[0]
        offset = auxiliary + 64 + disks
        saved_header = struct.unpack_from("<8I3Q", state.data, offset)
        if saved_header[0] != 2 or saved_header[1] != struct.calcsize("<8I3Q"):
            raise ValueError(f"{work.name}: unexpected backdrop layout: {saved_header}")
        return state.data[offset + saved_header[1]:], saved_header[10]
    picture_a, palette_a = backdrop_data(a)
    picture_b, palette_b = backdrop_data(b)
    pixels_match = picture_a == picture_b and palette_a == palette_b
    if not game_equal or not pixels_match or reference["final_video_fnv64"] != restored["final_video_fnv64"] or \
            any(i >= 0x7EFC0 for i in differences) or not ((registers_match and not phase) or inherited_phase):
        raise ValueError(f"{work.name}: cold restore changed guest execution.")
    result = {"case": name, "area": area, "zoom": zoom, "view": picture,
              "snapshot_backdrop_valid": True, "matched_guest_frames": reference_frames,
              "snapshot_view_version": header[0], "snapshot_view_header_size": header[1],
              "palette_fingerprint_match": palette_a == palette_b,
              "palette_fingerprint": f"{palette_b:016x}",
              "game_ram_and_cached_picture_match": True, "cpu_registers_match": registers_match,
              "preexisting_wait_loop_phase": bool(inherited_phase),
              "inactive_system_ram_differences": len(differences), "hardware_phase_delta": phase,
              "ram_fnv64": restored["ram_fnv64"]}
    print(json.dumps(result), flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("player", type=Path)
    parser.add_argument("report", type=Path)
    parser.add_argument("seed_disk", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    player = args.player.resolve(strict=True)
    report = json.loads(args.report.read_text())
    source = args.seed_disk.read_bytes()
    source_hash = hashlib.sha256(source).hexdigest()
    output = args.output.resolve()
    output.mkdir(exist_ok=False)
    cases = [(name, area, x, y, zoom) for name, area, x, y in
             (("town", 9, 464, 890), ("house", 1, 864, 450)) for zoom in range(3)]
    with ThreadPoolExecutor(2) as workers:
        results = list(workers.map(lambda row: check(player, report["source_identity"], source, output, row), cases))
    if hashlib.sha256(args.seed_disk.read_bytes()).hexdigest() != source_hash:
        raise ValueError("The source save changed.")
    (output / "summary.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
