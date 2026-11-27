"""Exercise widescreen in real rooms loaded from isolated X68000 save slots."""

from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import csv
import json
import shutil
import struct
import subprocess

# These coordinates are on playable terrain in the pinned English disks.
# Each case uses normal load, camera, movement, hazards, and draw routines.
AREAS = (
    (4, "Gold cave", 1008, 1016),
    (8, "Elven meadow", 784, 528),
    (9, "Voloh", 528, 528),
    (15, "Dwarven desert", 1024, 1024),
    (17, "Dwarf cave", 672, 1024),
    (39, "The heavens", 1024, 1024),
    (40, "Lagoon Castle", 440, 552),
)

INPUT = """11050 11064 key 282 1
11145 11160 joy 8 1
11365 11380 joy 8 1
12000 12048 joy 7 1
12054 12102 joy 6 1
12112 12153 joy 4 1
12162 12203 joy 5 1
"""


def saved_room(source, area, x, y):
    sector = 6 * 1024
    if len(source) != 1232 * 1024 or source[sector:sector + 16] != b"SAVE DATA       ":
        raise ValueError("Widescreen check needs the verified slot from the virtual controller replay.")
    disk = bytearray(source)
    # The $BF86 loader restores the map request from B4 and player position
    # from A4 and CA. The copied fixture remains isolated from real saves.
    disk[sector + 0xb4] = area
    for offset in (0xa4, 0xca):
        struct.pack_into(">HH", disk, sector + offset, x, y)
    return disk


def replay_room(preview, controls_game, output, source, area, x, y, *,
                zoom=0, frames=12500, inputs=INPUT, capture=None, assisted=False, entry=None):
    output.mkdir()
    player = output / preview.name
    try:
        player.hardlink_to(preview)
    except OSError:
        shutil.copy2(preview, player)
    media = output / "datas/games" / controls_game.name / "media"
    media.mkdir(parents=True)
    disk = saved_room(source, area, x, y)
    sector = 6 * 1024
    if entry:
        descriptor = bytes.fromhex(entry)
        if len(descriptor) != 16 or descriptor[0] != area:
            raise ValueError("Expected a measured 16-byte entrance record for this room.")
        disk[sector + 0xb4:sector + 0xc4] = descriptor
    if assisted:
        # Isolated soak-test survival aid. The boss/enemy records and scripts
        # still load and run through the original game.
        # The load routine restores live player stats from the first copy;
        # the second copy is the guest's cached player-stat block.
        for stats in (0x11, 0x94):
            disk[sector + stats:sector + stats + 2] = bytes((127, 127))
            struct.pack_into(">H", disk, sector + stats + 12, 1000)
    (media / "disk-05.xdf").write_bytes(disk)
    (output / "input.tsv").write_text(inputs, encoding="ascii")
    args = ["--headless", "--mute", "--auto-disk", "--lagoon-guest-keys",
            "--lagoon-zoom", str(zoom), "--lagoon-frame-trace", "view.csv",
            "--frames", str(frames), "--input-events", "input.tsv", "--report", "run.json"]
    if capture:
        args += ["--lagoon-wide-picture", capture]
    result = subprocess.run([str(player), *args], cwd=output, capture_output=True, text=True,
                            creationflags=subprocess.CREATE_NO_WINDOW, timeout=150)
    if result.returncode:
        raise ValueError(f"{output.name} replay failed: {result.stderr[-1500:]}")
    report = json.loads((output / "run.json").read_text(encoding="utf-8"))
    if report["core_error"] or report["frames"] != frames:
        raise ValueError(f"{output.name} did not complete its replay.")
    with (output / "view.csv").open(encoding="ascii", newline="") as trace:
        # Drop hardware registers at columns 14-17. Remaining suffix is
        # output size, enemy/HUD pixels, first live enemy state, top/bottom ink.
        rows = [tuple(map(int, row[:14] + row[18:])) for row in csv.reader(trace)]
    return report, rows


def transient_flashes(rows, ink_columns):
    flashes = []
    for before, middle, after in zip(rows, rows[1:], rows[2:]):
        if before[0] + 2 != after[0] or before[1] != middle[1] or middle[1] != after[1]:
            continue
        if min(r[10] for r in (before, middle, after)) < 200:
            continue
        if any(min(before[i], after[i]) >= 200 and middle[i] * 10 < min(before[i], after[i])
               for i in ink_columns):
            flashes.append(middle[0])
    return flashes


def check_wide_areas(preview, controls_game, output):
    source = (controls_game / "media/disk-05.xdf").read_bytes()
    output.mkdir()

    def observe(entry):
        area, name, x, y = entry
        capture = "castle-wide.bmp" if area == 40 else None
        report, rows = replay_room(preview, controls_game, output / f"area-{area:02}", source,
                                   area, x, y, capture=capture)
        active = [row for row in rows if row[0] >= 11800 and row[1] == area
                  and not (row[6] or row[7] or row[13])]
        positions = {(row[2], row[3]) for row in active}
        if len(active) < 600 or len(positions) < 30 or active[0][2:4] != (x, y):
            raise ValueError(f"{name} missed the expected spawn or movement.")
        flashes = transient_flashes(active, (9, 11))
        missing = [row[0] for row in active if not row[8] and row[10] >= 200]
        geometry = {(row[4], row[5]) for row in active}
        changed = sum(row[12] for row in active)
        if missing or flashes or changed or geometry != {(256, 256)}:
            raise ValueError(f"{name} widescreen discontinuity: missing={missing}, flashes={flashes}, "
                             f"center changes={changed}, geometry={geometry}")
        return {"area": area, "room": name, "frames": len(active), "positions": len(positions),
                "flashes": len(flashes), "center_changes": changed,
                "staggered_frames": sum(row[8] == 3 for row in active),
                "ram_fnv64": report["ram_fnv64"], "pcm_fnv64": report["pcm_fnv64"],
                "final_video_fnv64": report["final_video_fnv64"]}

    with ThreadPoolExecutor(2) as workers:
        return list(workers.map(observe, AREAS))


ZOOM_ROUTES = (
    ("meadow", 8, 784, 528, 2, 0, "enemies"),
    ("phantom", 38, 1032, 1064, 1, 0, "enemies"),
    ("heavens", 39, 1024, 1024, 2, 0, ""),
    ("castle", 40, 440, 552, 2, 0, ""),
    ("samson", 7, 144, 144, 1, 0, "boss"),
    ("battler", 50, 144, 144, 2, 0, "boss"),
    ("gnome-door", 32, 264, 198, 2, 33, ""),
    ("castle-door", 40, 456, 252, 2, 41, ""),
    ("thor-door", 45, 1864, 894, 1, 46, ""),
)


def check_zoom_routes(preview, controls_game, output):
    source = (controls_game / "media/disk-05.xdf").read_bytes()
    output.mkdir()

    def observe(entry):
        name, area, x, y, zoom, destination, actor_check = entry
        inputs = INPUT.partition("12000")[0] + "12000 12045 joy 4 1\n" if destination else INPUT
        capture = "meadow-zoom.bmp" if name == "meadow" else None
        report, rows = replay_room(preview, controls_game, output / name, source, area, x, y,
                                   zoom=zoom, frames=12650, inputs=inputs, capture=capture)
        route = [row for row in rows if row[0] >= 11800]
        expected = (568, 320) if zoom == 1 else (684, 384)
        if len(route) < 800 or {row[14:16] for row in route} != {expected}:
            raise ValueError(f"{name} did not hold its {expected} canvas throughout the route.")
        seen = {row[1] for row in route}
        if route[0][1] != area or (destination and (route[-1][1] != destination or not any(r[6] for r in route))):
            raise ValueError(f"{name} did not take the measured doorway: {seen}")
        if not destination and seen != {area}:
            raise ValueError(f"{name} left its expected room: {seen}")
        stable = [row for row in route if row[1] and not (row[6] or row[7] or row[13])]
        # Check lit incoming and outgoing frames too. A full guest-authored
        # blackout has no lit center and cannot count as a border-only flash.
        flashes = transient_flashes([row for row in route if row[1] and not row[7]], (9, 11, 22, 23))
        missing = [row[0] for row in stable if not row[8] and row[10] >= 200]
        changed_center = sum(row[12] for row in stable)
        hud_frames = sum(row[17] > 0 for row in stable)
        enemy_frames = sum(row[16] > 0 for row in stable)
        actor_motion = len({row[18:20] for row in stable if row[20] > 0})
        if flashes or missing or changed_center or hud_frames < 300:
            raise ValueError(f"{name} unstable at zoom {zoom}: flashes={flashes}, missing={missing}, "
                             f"center changes={changed_center}, HUD frames={hud_frames}.")
        if actor_check and (actor_motion < 10 or enemy_frames < 4):
            raise ValueError(f"{name} did not draw a moving enemy outside the native view.")
        return {"route": name, "area": area, "destination": destination, "size": expected,
                "frames": len(route), "door_frames": sum(row[6] for row in route), "flashes": len(flashes),
                "hud_frames": hud_frames, "enemy_frames": enemy_frames, "actor_positions": actor_motion,
                "boss": actor_check == "boss", "center_changes": changed_center,
                "ram_fnv64": report["ram_fnv64"], "final_video_fnv64": report["final_video_fnv64"]}

    with ThreadPoolExecutor(2) as workers:
        return list(workers.map(observe, ZOOM_ROUTES))

@dataclass(frozen=True)
class LateRoute:
    name: str
    area: int
    x: int
    y: int
    destination: int = 0
    boss_hp: int = 0
    entry: str = ""
    movement: str = INPUT[INPUT.index("12000"):]
    cloud_jump: bool = False
    frames: int = 12650


WALK_NORTH = "12000 12042 joy 4 1\n"
JUMP_ACTIONS = (3, 4)  # Guest $403C ascent and airborne pose on the cloud crossing.
ADVANCE_DIALOGUE = "".join(f"{frame} {frame + 8} joy 8 1\n" for frame in range(12350, 13150, 24))
EARDON_INTERACT = "".join(
    f"{frame} {frame + 9} joy {0 if (frame // 24) % 2 else 8} 1\n"
    for frame in range(12340, 13142, 24)
)
BOSS_APPROACH = (
    "11900 11916 joy 4 1\n11990 12004 joy 7 1\n12020 12042 joy 6 1\n"
    "12090 12104 joy 5 1\n12120 12134 joy 7 1\n"
)
LATE_ROUTES = (
    LateRoute("natela", 13, 1016, 872, destination=14, boss_hp=100, movement=WALK_NORTH),
    LateRoute("eardon", 19, 1320, 424, destination=21, boss_hp=150,
              movement=WALK_NORTH + "12280 12332 joy 4 1\n" + EARDON_INTERACT, frames=13350),
    LateRoute("duma", 29, 504, 359, destination=31, boss_hp=240,
              movement=WALK_NORTH + "12290 12320 joy 4 1\n" + ADVANCE_DIALOGUE, frames=13350),
    LateRoute("thimale", 36, 128, 464, boss_hp=300, entry="241f06081d0000102000000000ffff00",
              movement=BOSS_APPROACH),
    LateRoute("nymph-spring", 37, 272, 320),
    LateRoute("clouds", 39, 1024, 1024,
              movement=INPUT[INPUT.index("12000"):] +
              "12300 12328 joy 4 1\n12300 12312 joy 0 1\n12370 12408 joy 7 1\n12370 12382 joy 8 1\n"),
    LateRoute("cloud-landing", 39, 260, 210, destination=40,
              movement="11980 12012 joy 4 1\n11980 11995 joy 8 1\n", cloud_jump=True),
    LateRoute("castle-enemies", 42, 1024, 1024),
    LateRoute("castle-stairs", 42, 104, 89, destination=43, movement=WALK_NORTH),
    LateRoute("upper-castle", 44, 1024, 1024),
    LateRoute("thor-room", 46, 128, 208, boss_hp=650, entry="2e7436080e0000101000000000ffff00",
              movement=BOSS_APPROACH),
    LateRoute("u-t", 48, 136, 584, destination=49, boss_hp=410, movement=WALK_NORTH),
)


def check_late_routes(preview, controls_game, output):
    source = (controls_game / "media/disk-05.xdf").read_bytes()
    output.mkdir()

    def observe(case):
        capture = "clouds.bmp" if case.name == "clouds" else None
        _, rows = replay_room(preview, controls_game, output / case.name, source,
                              case.area, case.x, case.y, zoom=2, frames=case.frames,
                              inputs=INPUT.partition("12000")[0] + case.movement,
                              capture=capture, entry=case.entry, assisted=True)
        route = [row for row in rows if row[0] >= 11800]
        if len(route) != case.frames - 11799 or {row[14:16] for row in route} != {(684, 384)}:
            raise ValueError(f"{case.name}: lost the 1.5x canvas.")
        area = case.destination or case.area
        door_frames = sum(row[6] for row in route)
        if route[0][1] != case.area or route[-1][1] != area or (case.destination and not door_frames):
            raise ValueError(f"{case.name}: did not complete the intended room change.")
        stable = [row for row in route if row[1] and not (row[6] or row[7] or row[13])]
        flashes = transient_flashes([row for row in route if row[1] and not row[7]], (9, 11, 22, 23))
        missing = [row[0] for row in stable if not row[8] and row[10] >= 200]
        changed_center = sum(row[12] for row in stable)
        hud_frames = sum(row[17] > 0 for row in stable)
        max_hp = stable[0][26] if stable else 0
        if flashes or missing or changed_center or hud_frames < 250 or max_hp != 127:
            raise ValueError(f"{case.name}: flashes={flashes}, missing={missing}, changed center={changed_center}, "
                             f"HUD frames={hud_frames}, fixture HP={max_hp}.")
        boss = [row for row in stable if row[1] == area and row[20] == case.boss_hp and row[10] >= 200]
        boss_poses = len({row[21] for row in boss})
        if case.boss_hp and (len(boss) < 100 or boss_poses < 2):
            raise ValueError(f"{case.name}: boss coverage too short ({len(boss)} frames, {boss_poses} poses).")
        jump_frames = sum(row[1] == case.area and not row[6] and row[25] in JUMP_ACTIONS for row in route)
        if case.cloud_jump and jump_frames < 8:
            raise ValueError(f"{case.name}: the original jump action never ran.")
        restored = sum(row[24] for row in stable)
        enemy_frames = sum(row[16] > 0 for row in stable)
        if case.name == "castle-enemies" and (enemy_frames < 100 or not restored):
            raise ValueError(f"{case.name}: missing enemy or HUD overlap coverage.")
        return {"route": case.name, "area": case.area, "destination": case.destination,
                "frames": len(route), "door_frames": door_frames, "flashes": len(flashes),
                "boss_frames": len(boss) if case.boss_hp else 0, "boss_poses": boss_poses if case.boss_hp else 0,
                "jump_frames": jump_frames, "hud_frames": hud_frames,
                "enemy_frames": enemy_frames, "restored_sprite_pixels": restored}

    with ThreadPoolExecutor(2) as workers:
        return list(workers.map(observe, LATE_ROUTES))
