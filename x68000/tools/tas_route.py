"""Run isolated, resumable X68000 TAS routes using the player's normal input API.

Teleports and boosts are explicit snapshot edits. Conversations, doors, combat
and scripts execute in the game. No owner save is opened or rewritten.
"""

import argparse
import ctypes
import csv
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess


BASE = 0x7EFC0
BUTTONS = {"up": 4, "down": 5, "left": 6, "right": 7, "jump": 8, "sword": 0}


class Snapshot:
    def __init__(self, path):
        self.path = path
        self.data = bytearray(path.read_bytes())
        if self.data[:8] != b"RRX68KS1":
            raise ValueError("Expected the pinned X68000 snapshot envelope.")
        position = self.data.index(b"RRX68MS1") + 18
        for _ in range(40):
            tag, version, size = struct.unpack_from("<4sHI", self.data, position)
            position += 10
            if tag == b"BUS " and version == 1 and size >= 0xC00000:
                self.ram = position + BASE
                break
            position += size
        else:
            raise ValueError("Missing X68000 RAM section.")
        if self.data[self.ram + 0xE968:self.ram + 0xE96E] != bytes.fromhex("427900e8002a"):
            raise ValueError("The pinned Lagoon game has not loaded.")
        core_size = struct.unpack_from("<Q", self.data, 16)[0]
        self.input_frame = struct.unpack_from("<Q", self.data, 168 + core_size + 24)[0]

    def byte(self, offset):
        return self.data[self.ram + offset]

    def word(self, offset):
        return struct.unpack_from(">H", self.data, self.ram + offset)[0]

    def write(self, offset, value, width=2):
        self.data[self.ram + offset:self.ram + offset + width] = value.to_bytes(width, "big")

    def save(self):
        library = self.path.parent / "tas_hash.dll"
        if library.exists():
            checksum = ctypes.CDLL(str(library)).tas_hash
            checksum.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint64]
            checksum.restype = ctypes.c_uint64
            buffer = (ctypes.c_ubyte * len(self.data)).from_buffer(self.data)
            value = checksum(buffer, 160, 14695981039346656037)
            value = checksum(ctypes.byref(buffer, 168), len(self.data) - 168, value)
        else:
            value = 14695981039346656037
            for span in (memoryview(self.data)[:160], memoryview(self.data)[168:]):
                for byte in span:
                    value = ((value ^ byte) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
        struct.pack_into("<Q", self.data, 160, value)
        self.path.write_bytes(self.data)

    def observe(self):
        actors = []
        for slot in range(1, 32):
            at = 0x402E + 16 * slot
            if self.word(at):
                actors.append({"slot": slot, "x": self.word(at), "y": self.word(at + 2),
                               "value": self.word(at + 4), "species": self.byte(at + 6),
                               "kind": self.byte(at + 7)})
        return {"area": self.byte(0xECCC), "x": self.word(0x402E), "y": self.word(0x4030),
                "hp": self.byte(0x4CFF), "level": self.byte(0x4CFD),
                "attack": self.word(0x4D08), "gold": self.word(0x4D06),
                "modal": self.byte(0x4D2F), "actors": actors,
                "choice": self.byte(0x4D30),
                "flags": self.data[self.ram + 0x4CAE:self.ram + 0x4CEE].hex(),
                "items": self.data[self.ram + 0x4D0E:self.ram + 0x4D18].hex(),
                "equipment_ui": self.data[self.ram + 0x162E0:self.ram + 0x16320].hex()}


def view_summary(path):
    """Compact observations, not a substitute for reviewing moving images."""
    if not path.exists():
        return None
    rows = []
    with path.open(encoding="ascii") as stream:
        for row in csv.reader(stream):
            if len(row) != 31:
                raise ValueError("Unexpected diagnostic view trace schema.")
            rows.append(tuple(int(value, 16 if 14 <= i < 18 else 10)
                              for i, value in enumerate(row)))
    flashes = []
    for a, b, c in zip(rows, rows[1:], rows[2:]):
        if a[0] + 2 != c[0] or len({a[1], b[1], c[1]}) != 1:
            continue
        if any(r[6] or r[7] or r[13] or r[10] < 200 for r in (a, b, c)):
            continue
        if any(min(a[i], c[i]) >= 200 and b[i] * 10 < min(a[i], c[i])
               for i in (9, 11, 26, 27)):
            flashes.append(b[0])
    enemies = [r for r in rows if r[25] != -1]
    return {"frames": len(rows), "areas": sorted({r[1] for r in rows}),
            "canvases": sorted({r[18:20] for r in rows}),
            "player_positions": len({r[2:4] for r in rows}),
            "door_frames": sum(bool(r[6]) for r in rows),
            "dialogue_frames": sum(bool(r[7]) for r in rows),
            "center_mismatch_frames": sum(bool(r[12]) for r in rows),
            "single_frame_margin_drop_candidates": flashes,
            "enemy_frames": len(enemies),
            "enemy_positions": len({r[22:24] for r in enemies}),
            "enemy_hp_min": min((r[24] for r in enemies), default=None),
            "enemy_zero_hp_frames": sum(r[24] == 0 for r in enemies)}


def advance(work, frames, events, label, cold=False):
    player = work / "Lagoon-TAS.exe"
    before = None if cold else Snapshot(work / "active.rrstate").observe()
    (work / "input.tsv").write_text(events, encoding="ascii")
    command = [str(player), "--headless", "--mute", "--auto-disk", "--frames", str(frames),
               "--input-events", "input.tsv", "--state-slot", "active.rrstate",
               "--state-save-at", str(frames - 1), "--report", "step.json", "--capture", "latest.ppm"]
    if not cold:
        command.append("--state-load-start")
    if label == "native-save" and not json.loads((work / "source.json").read_text()).get("diagnostic"):
        command.remove("--headless")
    if json.loads((work / "source.json").read_text()).get("diagnostic"):
        command.extend(["--lagoon-guest-keys", "--lagoon-zoom", "2",
                        "--lagoon-frame-trace", "view.csv"])
    result = subprocess.run(command, cwd=work, capture_output=True, text=True,
                            timeout=120, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    report = json.loads((work / "step.json").read_text())
    if result.returncode or report["core_error"] or report["frames"] != frames:
        raise RuntimeError(result.stderr[-1800:] or "TAS step did not complete.")
    after = Snapshot(work / "active.rrstate").observe()
    record = {"label": label, "frames": frames, "before": before, "after": after,
              "ram_hash": report["ram_fnv64"], "input": events}
    if json.loads((work / "source.json").read_text()).get("diagnostic"):
        record["view"] = view_summary(work / "view.csv")
    with (work / "route.jsonl").open("a", encoding="utf-8") as stream:
        stream.write(json.dumps(record) + "\n")
    print(json.dumps(after, indent=2))
    return after


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("work", type=Path)
    commands = parser.add_subparsers(dest="command", required=True)
    boot = commands.add_parser("boot")
    boot.add_argument("--player", type=Path, required=True)
    commands.add_parser("inspect")
    commands.add_parser("boost")
    transfer = commands.add_parser("transfer")
    transfer.add_argument("area", type=int)
    transfer.add_argument("x", type=int)
    transfer.add_argument("y", type=int)
    warp = commands.add_parser("warp")
    warp.add_argument("x", type=int)
    warp.add_argument("y", type=int)
    step = commands.add_parser("step")
    step.add_argument("--frames", type=int, default=240)
    step.add_argument("--hold", nargs="*", choices=BUTTONS, default=[])
    step.add_argument("--pulse", choices=BUTTONS)
    step.add_argument("--label", default="advance")
    talk = commands.add_parser("talk")
    talk.add_argument("slot", type=int)
    talk.add_argument("--frames", type=int, default=900)
    choice = commands.add_parser("choice")
    choice.add_argument("answer", choices=("yes", "no"))
    args = parser.parse_args()
    if hasattr(args, "frames") and not 1 <= args.frames <= 8000:
        parser.error("Each resumable step must be between 1 and 8000 frames.")
    work = args.work.resolve()
    if args.command == "boot":
        work.mkdir(parents=True, exist_ok=False)
        player = args.player.resolve(strict=True)
        try:
            (work / "Lagoon-TAS.exe").hardlink_to(player)
        except OSError:
            shutil.copyfile(player, work / "Lagoon-TAS.exe")
        (work / "source.json").write_text(json.dumps({"player": str(player),
            "sha256": hashlib.sha256(player.read_bytes()).hexdigest()}, indent=2))
        advance(work, 11100, "10975 11002 joy 6 1\n", "new-game", cold=True)
        return
    origin = json.loads((work / "source.json").read_text())
    if hashlib.sha256((work / "Lagoon-TAS.exe").read_bytes()).hexdigest() != origin["sha256"]:
        raise ValueError("The TAS executable changed since this route started.")
    state = Snapshot(work / "active.rrstate")
    if state.input_frame > 28000:
        core_size = struct.unpack_from("<Q", state.data, 16)[0]
        struct.pack_into("<Q", state.data, 168 + core_size + 24, 0)
        state.save()
        state.input_frame = 0
    if args.command == "transfer":
        if not json.loads((work / "source.json").read_text()).get("diagnostic"):
            raise ValueError("Map transfers require the matching diagnostic player's guest-key support.")
        if state.byte(0x4D2F) or not (1 <= args.area <= 50 and 0 < args.x < 2048 and 0 <= args.y < 2048):
            raise ValueError("Finish the current dialogue and choose valid destination coordinates.")
        f = state.input_frame
        advance(work, 1100, f"{f+2} {f+15} key 282 1\n{f+85} {f+99} joy 5 1\n"
                f"{f+160} {f+175} joy 8 1\n{f+420} {f+435} joy 8 1\n", "native-save")
        disk = next((work / "datas/games").glob("*/media/disk-05.xdf"))
        data = bytearray(disk.read_bytes())
        if data[6144:6160] != b"SAVE DATA       ":
            raise ValueError("Native save did not create slot 1.")
        data[6144 + 0xB4] = args.area
        for offset in (0xA4, 0xCA):
            struct.pack_into(">HH", data, 6144 + offset, args.x, args.y)
        disk.write_bytes(data)
        with (work / "cheats.jsonl").open("a") as stream:
            stream.write(json.dumps({"command": "assisted-map-transfer", "area": args.area,
                                     "x": args.x, "y": args.y}) + "\n")
        loaded = advance(work, 12000, "11050 11064 key 282 1\n11145 11160 joy 8 1\n"
                         "11365 11380 joy 8 1\n", "assisted-map-load", cold=True)
        if loaded["area"] != args.area or abs(loaded["x"] - args.x) > 16 or abs(loaded["y"] - args.y) > 16:
            raise ValueError("The assisted map load did not reach its requested entrance.")
        return
    if args.command == "inspect":
        print(json.dumps(state.observe(), indent=2))
        return
    if args.command == "boost":
        for offset in (0x4CFE, 0x4CFF, 0x4D00, 0x4D01):
            state.write(offset, 127, 1)
        for offset in (0x4CF8, 0x4D08):
            state.write(offset, 0x7800)
        for offset in (0x4CFA, 0x4D0A):
            state.write(offset, 0x7000)
        state.write(0x4CFD, 35, 1)
        state.write(0x4D06, 60000)
    elif args.command == "warp":
        if not (0 < args.x < 2048 and 0 <= args.y < 2048):
            raise ValueError("Teleport must stay within the current map.")
        state.write(0x402E, args.x)
        state.write(0x4030, args.y)
    elif args.command == "choice":
        if not state.byte(0x4D30):
            raise ValueError("There is no active yes/no choice.")
        # The original script defaults to No; Sword always cancels to No.
        events = f"{start+1} {start+9} joy 4 1\n"
        if args.answer == "yes":
            events += f"{start+30} {start+38} joy 5 1\n"
        events += f"{start+65} {start+73} joy 8 1\n"
        frames = 650
    elif args.command == "talk":
        if not 1 <= args.slot <= 31:
            raise ValueError("NPC slot must be between 1 and 31.")
        npc = 0x402E + 16 * args.slot
        if state.byte(npc + 7) != 3 or not state.word(npc):
            raise ValueError("The selected slot is not a live NPC.")
        state.write(0x402E, state.word(npc))
        state.write(0x4030, state.word(npc + 2) + 24)
    if args.command in ("boost", "warp", "talk"):
        if args.command in ("warp", "talk"):
            state.write(0xDE7D, state.word(0x402E) >> 4, 1)
            state.write(0xDE7E, state.word(0x4030) >> 4, 1)
        state.save()
        with (work / "cheats.jsonl").open("a") as stream:
            stream.write(json.dumps({"command": args.command, "arguments": vars(args),
                                     "before_input_frame": state.input_frame}, default=str) + "\n")
    start = state.input_frame
    frames = getattr(args, "frames", 4)
    events = ""
    if args.command == "step":
        for name in args.hold:
            events += f"{start} {start + frames} joy {BUTTONS[name]} 1\n"
        if args.pulse:
            for frame in range(start + 1, start + frames - 7, 24):
                events += f"{frame} {frame + 7} joy {BUTTONS[args.pulse]} 1\n"
    elif args.command == "talk":
        events = f"{start} {start + 16} joy 4 1\n"
        events += "".join(f"{frame} {frame + 8} joy 8 1\n"
                          for frame in range(start + 26, start + frames - 8, 24))
    after = advance(work, frames, events, getattr(args, "label", args.command))
    if args.command == "talk" and after["modal"] and not after["choice"]:
        # Some house triggers reopen immediately at the threshold. Move the
        # assisted player clear before acknowledging the last dialogue page.
        state = Snapshot(work / "active.rrstate")
        state.write(0x4030, min(2047, after["y"] + 40))
        state.save()
        start = state.input_frame
        events = "".join(f"{frame} {frame + 8} joy 8 1\n"
                         for frame in range(start + 1, start + 120, 24))
        advance(work, 150, events, "leave-interaction")


if __name__ == "__main__":
    main()
