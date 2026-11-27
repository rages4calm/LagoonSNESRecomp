"""Build a local English X68000 player with the pinned RetroRecomp converter."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lagoon_qol import enhance_program, patch_giles_rescue


PROJECT = Path(__file__).resolve().parents[2]
VERSION = "1.8.0"
WINDOWS_CONVERTER = "e7bb14e180dcedea5250e3d46c9192ab5a86d0dba13a85f29b78c99b44c9b9b0"
PLAYER = "Lagoon - English.exe"
USER_DISK_SHA256 = "dde8406ac12e0246878f65823b2294dabc44442c64de72ffc87e8bef59b8d3cf"
PROGRAM_SHA256 = "dc1fb23959dfb0915321f80125342fec53d616d08ead76c70cfad82aea366d43"
DIM_HEADER = 256
SECTOR = 1024
HU_HEADER = 64
DISK_PREFIX = "Lagoon (1990)(Zoom)(Disk "
SOURCE_SYSTEM = DISK_PREFIX + "1 of 4)(System).dim"
DISK_HASHES = {
    "System-EN.dim": "3470624f7a6f2eb3ee583c53a0c1a80390c7fefd013f04f94d86db798f3fcbac",
    DISK_PREFIX + "2 of 4)(Disk 1).dim": "028f63d0a7fc7e3d762344345c71428ffcb1b49454faa103aed2d3d69cf94ab2",
    DISK_PREFIX + "3 of 4)(Disk 2).dim": "587d5ec1aded8a904ccc0c4310382181e811153bedcab7d711f013e36c18bdf4",
    DISK_PREFIX + "4 of 4)(Disk 3).dim": "038c844490be9501a9515dd79c7ac401ab85c56773bc767d1bfdfea10656222e",
}


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def checked_file(path, sha256=None, size=None):
    path = path.resolve(strict=True)
    if not path.is_file() or (size is not None and path.stat().st_size != size):
        raise ValueError(f"Unexpected file size or type: {path}")
    if sha256 is not None and digest(path) != sha256:
        raise ValueError(f"File does not match the reviewed revision: {path}")
    return path


def make_user_disk(system_disk):
    # Lagoon's Make command copies the System disk, then writes the user boot
    # sector from LAGOON.X ($C731 in the loaded program) to sector zero.
    # This physical offset is specific to the SHA-256-checked English DIM.
    body = system_disk.read_bytes()[DIM_HEADER:]
    template_offset = 0x2EB71
    user = body[template_offset:template_offset + 1024] + body[1024:]
    if hashlib.sha256(user).hexdigest() != USER_DISK_SHA256:
        raise ValueError("Generated User disk does not match Lagoon's in-game Make output.")
    return user


def lagoon_program(image):
    root = DIM_HEADER + 5 * SECTOR
    entry_offset = next((offset for offset in range(root, root + 6 * SECTOR, 32)
                         if image[offset:offset + 11] == b"LAGOON  X  "), None)
    if entry_offset is None:
        raise ValueError("The System disk has no LAGOON.X root entry.")
    entry = image[entry_offset:entry_offset + 32]
    cluster = int.from_bytes(entry[26:28], "little")
    size = int.from_bytes(entry[28:32], "little")
    sectors = []
    for _ in range((size + SECTOR - 1) // SECTOR):
        if not 2 <= cluster < 1223:
            raise ValueError("LAGOON.X has an invalid FAT12 chain.")
        sectors.append(DIM_HEADER + (11 + cluster - 2) * SECTOR)
        fat = DIM_HEADER + SECTOR + cluster * 3 // 2
        pair = int.from_bytes(image[fat:fat + 2], "little")
        cluster = pair >> 4 if cluster & 1 else pair & 0xFFF
    if cluster < 0xFF8 or len(sectors) != len(set(sectors)):
        raise ValueError("LAGOON.X has an invalid FAT12 terminator or a repeated sector.")
    program = b"".join(image[start:start + SECTOR] for start in sectors)[:size]
    return program, sectors, entry_offset


def scale_combat_rewards(program, exp_rate, gold_rate):
    table = HU_HEADER + 0x11D90
    records = set()
    for species in range(62, 170):
        pointer = table + 2 * species
        relative = int.from_bytes(program[pointer:pointer + 2], "big", signed=True)
        record = table + relative
        if record == HU_HEADER + 0x10250:
            continue  # Unused species share a sentinel record.
        if (record % 2 or not HU_HEADER + 0xCF06 <= record <= HU_HEADER + 0x15882
                or program[record:record + 6] != bytes.fromhex("00e2800000e2")):
            raise ValueError(f"Unexpected combat record for species {species}.")
        records.add(record)
    if len(records) != 89:
        raise ValueError("Combat record count differs from the reviewed game.")
    scaled = bytearray(program)
    for record in sorted(records):
        for field, rate in ((0x12, exp_rate), (0x14, gold_rate)):
            offset = record + field
            value = int.from_bytes(program[offset:offset + 2], "big")
            scaled[offset:offset + 2] = min(value * rate, 0xFFFF).to_bytes(2, "big")
    return bytes(scaled)


def patch_system(system, exp_rate, gold_rate, enhanced, adventure, expedition=False):
    image = bytearray(system.read_bytes())
    program, sectors, entry = lagoon_program(image)
    if hashlib.sha256(program).hexdigest() != PROGRAM_SHA256:
        raise ValueError("Enhancements require the verified English 0.9b LAGOON.X.")
    if (exp_rate, gold_rate) != (1, 1):
        program = scale_combat_rewards(program, exp_rate, gold_rate)
    if enhanced or adventure:
        program = enhance_program(program, adventure, expedition)
    if len(program) > len(sectors) * SECTOR:
        raise ValueError("The enhanced LAGOON.X cannot fit its existing FAT12 chain.")
    for index, start in enumerate(sectors):
        sector = program[index * SECTOR:(index + 1) * SECTOR]
        image[start:start + len(sector)] = sector
    image[entry + 28:entry + 32] = len(program).to_bytes(4, "little")
    if expedition:
        image = patch_giles_rescue(image)
    system.write_bytes(image)


def stage_disks(args, work, use_user_disk=False, exp_rate=1, gold_rate=1, enhanced=False,
                adventure=False, expedition=False):
    directory = work / "disks"
    directory.mkdir()
    system = args.english_system
    if args.patch:
        system = work / "System-EN.dim"
        subprocess.run(
            [sys.executable, "-B", "-P", str(PROJECT / "x68000/tools/bps_apply.py"),
             str(args.disks / SOURCE_SYSTEM), str(args.patch), str(system)],
            check=True,
        )
    entries = []
    for index, (name, expected) in enumerate(DISK_HASHES.items(), 1):
        path = system if index == 1 else args.disks / name
        entry = f"disk-{index:02}.dim"
        shutil.copyfile(checked_file(path, expected, 1261824), directory / entry)
        entries.append(entry)
    staged_system = directory / entries[0]
    user_boot = make_user_disk(staged_system)[:SECTOR] if use_user_disk else None
    if enhanced or adventure or (exp_rate, gold_rate) != (1, 1):
        patch_system(staged_system, exp_rate, gold_rate, enhanced, adventure, expedition)
    if user_boot is not None:
        entry = "disk-05.xdf"
        (directory / entry).write_bytes(user_boot + staged_system.read_bytes()[DIM_HEADER + SECTOR:])
        entries.append(entry)
    playlist = directory / "Lagoon - English.m3u"
    playlist.write_text("\n".join(entries) + "\n", encoding="ascii")
    return playlist


def replace_file(source, destination):
    with tempfile.NamedTemporaryFile(dir=destination.parent, delete=False) as file:
        pending = Path(file.name)
    try:
        shutil.copyfile(source, pending)
        os.replace(pending, destination)
    finally:
        pending.unlink(missing_ok=True)


def expected_disks(use_user_disk):
    return [*DISK_HASHES.values(), USER_DISK_SHA256] if use_user_disk else list(DISK_HASHES.values())


def staged_hashes(playlist):
    return [digest(playlist.parent / entry) for entry in playlist.read_text(encoding="ascii").splitlines()]


def verified_export(report_path, ipl, cgrom, media_hashes, installed=None):
    report_path = checked_file(report_path)
    report = json.loads(report_path.read_text(encoding="utf-8"))
    machine = report["machine"]
    checks = report["final_checks"][0]
    if (
        report["version"] != VERSION
        or report["system"]["id"] != "x68"
        or report["target"] != "windows-x86_64"
        or report["status"] != "supported"
        or report["executable"] != PLAYER
        or [entry["sha256"] for entry in machine["media"]] != media_hashes
        or machine["firmware"]["ipl"]["sha256"] != digest(ipl)
        or machine["firmware"]["cgrom"]["sha256"] != digest(cgrom)
        or report["native_validation"]["passed"] is not True
        or checks["frames"] < 4000
        or checks["reference_match"] is not True
        or checks["packed_reference_match"] is not True
        or checks["state_restart_match"] is not True
        or checks["boot_state_restart_match"] is not True
    ):
        raise ValueError(f"The converter did not verify this English X68000 player: {report_path}")

    export = checked_file(installed or report_path.parents[3] / PLAYER, report["export_receipt"]["sha256"])
    return report, checks, export


def edition_directory(args):
    edition = "retrorecomp-save-disk" if args.user_disk else "retrorecomp"
    if args.expedition:
        edition = "retrorecomp-expedition"
    elif args.adventure:
        edition = "retrorecomp-adventure"
    elif args.enhanced:
        edition = "retrorecomp-enhanced"
    elif (args.exp_rate, args.gold_rate) != (1, 1):
        edition = "retrorecomp-rewards"
    return PROJECT / "local-x68000" / edition


def install_verified_export(report_path, ipl, cgrom, media_hashes, args):
    report, checks, export = verified_export(report_path, ipl, cgrom, media_hashes)
    sha256 = report["export_receipt"]["sha256"]
    destination = edition_directory(args)
    destination.mkdir(parents=True, exist_ok=True)
    installed = destination / PLAYER
    receipt = {
        "format": 1,
        "converter_version": VERSION,
        "system": "x68",
        "language": "en",
        "executable": PLAYER,
        "sha256": sha256,
        "reference_validated": True,
        "user_disk": args.user_disk,
        "exp_rate": args.exp_rate,
        "gold_rate": args.gold_rate,
        "enhanced": args.enhanced,
        "adventure": args.adventure,
        "expedition": args.expedition,
        "source_identity": report["source_identity"],
        "frames": checks["frames"],
        "interpreted_opcodes": checks["interpreted_opcodes"],
    }
    with tempfile.TemporaryDirectory(prefix="lagoonextra-receipt-") as directory:
        receipt_source = Path(directory) / "lagoon-extra-install.json"
        receipt_source.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        if not installed.is_file() or digest(installed) != sha256:
            replace_file(export, installed)
        replace_file(report_path, destination / "conversion-report.json")
        replace_file(receipt_source, destination / receipt_source.name)
    print(f"Verified local player: {installed}\nSHA-256: {sha256}")


def reusable_export(args, ipl, cgrom, media):
    destination = edition_directory(args)
    try:
        report, _, _ = verified_export(destination / "conversion-report.json", ipl, cgrom, media, destination / PLAYER)
        receipt = json.loads((destination / "lagoon-extra-install.json").read_text(encoding="utf-8"))
        if (receipt.get("sha256") != report["export_receipt"]["sha256"] or
                receipt.get("source_identity") != report["source_identity"]):
            return False
        if report.get("toolchain", {}).get("kind") != args.toolchain:
            return False
        built = checked_file(Path(report["build_executable"]))
        project = next((path for path in list(built.parents)[:6]
                        if path.name == receipt["source_identity"][:16]), None)
        if project is None:
            return False
        checked_file(project / "packed-game.exe", report["export_receipt"]["sha256"])
        return True
    except (OSError, ValueError, KeyError, IndexError, TypeError):
        return False


def conversion_error(work):
    summary = work / "Games/datas/Retro-Recomp-batch.json"
    log = work / "conversion.log"
    try:
        report = json.loads(summary.read_text(encoding="utf-8"))
        detail = next((game["message"] for game in report["games"]
                       if game["status"] == "error" and game.get("message")), "")
    except (OSError, ValueError, KeyError, TypeError):
        detail = ""
    if not detail:
        with log.open("rb") as source:
            source.seek(max(0, log.stat().st_size - 4096))
            detail = source.read().decode("utf-8", errors="replace").strip()
    excerpt = detail[:4096].rstrip() or "RetroRecomp returned an error before writing diagnostics."
    return f"RetroRecomp could not build the English base player.\n{excerpt}\nFull log: {log}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--converter", type=Path, help="Official portable Retro-Recomp.exe v1.8.0.")
    parser.add_argument("--from-report", type=Path, help="Install a completed export from this local conversion report.")
    parser.add_argument("--ipl", required=True, type=Path, help="X68000 IPL ROM, 128 KiB.")
    parser.add_argument("--cgrom", required=True, type=Path, help="X68000 font ROM, 768 KiB.")
    parser.add_argument("--disks", type=Path, default=PROJECT / "Lagoon_Sharp-X68000_EN")
    parser.add_argument("--english-system", type=Path, default=PROJECT / "x68000/patched/System-EN.dim")
    parser.add_argument("--patch", type=Path, help="Optional nleseul 0.9b BPS for the alternate DIM System disk.")
    parser.add_argument("--user-disk", action="store_true",
                        help="Build a separate player with a pre-initialized, writable User disk.")
    parser.add_argument("--exp-rate", type=int, choices=range(1, 9), default=1,
                        help="Combat EXP multiplier. Requires --user-disk for an isolated Reward Boost build.")
    parser.add_argument("--gold-rate", type=int, choices=range(1, 9), default=1,
                        help="Combat gold multiplier. Requires --user-disk for an isolated Reward Boost build.")
    parser.add_argument("--enhanced", action="store_true",
                        help="Longer sword reach, extra recovery, faster walking and a gentler opening.")
    parser.add_argument("--adventure", action="store_true",
                        help="Enhanced Combat plus lateral cleave, stronger magic and extra jump airtime.")
    parser.add_argument("--expedition", action="store_true",
                        help="Enhanced Adventure plus centered camera and faster Inventory/Equipment transitions.")
    parser.add_argument("--toolchain", choices=("llvm", "msvc"), default="msvc")
    parser.add_argument("--keep-build", action="store_true", help="Retain the private converter project for an Atlas build.")
    parser.add_argument("--resume", action="store_true", help="Reuse a verified install and its retained build if the inputs still match.")
    args = parser.parse_args()
    args.adventure = args.adventure or args.expedition
    args.enhanced = args.enhanced or args.adventure
    if os.name != "nt":
        parser.error("This recipe pins the verified Windows converter. Use the upstream Linux instructions for other hosts.")
    if (args.converter is None) == (args.from_report is None):
        parser.error("Supply either --converter or --from-report.")
    boosted = (args.exp_rate, args.gold_rate) != (1, 1)
    if (boosted or args.enhanced) and not args.user_disk:
        parser.error("Reward and enhanced profiles require --user-disk.")
    try:
        ipl = checked_file(args.ipl, size=131072)
        cgrom = checked_file(args.cgrom, size=786432)
        if args.from_report:
            media = expected_disks(args.user_disk)
            if boosted or args.enhanced:
                with tempfile.TemporaryDirectory(prefix="lagoonextra-reward-check-") as directory:
                    media = staged_hashes(stage_disks(
                        args, Path(directory), True, args.exp_rate, args.gold_rate, args.enhanced,
                        args.adventure, args.expedition))
            install_verified_export(args.from_report, ipl, cgrom, media, args)
            return
        if args.resume:
            with tempfile.TemporaryDirectory(prefix="lagoonextra-resume-") as directory:
                media = staged_hashes(stage_disks(args, Path(directory), args.user_disk, args.exp_rate,
                                                args.gold_rate, args.enhanced, args.adventure, args.expedition))
                if reusable_export(args, ipl, cgrom, media):
                    print("Verified base player and matching build cache found. Resuming at Atlas.")
                    return
        converter = checked_file(args.converter, WINDOWS_CONVERTER)
        work = Path(tempfile.mkdtemp(prefix="lagoonextra-english-")).resolve()
        print(f"Conversion work and exit report: {work}", flush=True)
        playlist = stage_disks(args, work, args.user_disk, args.exp_rate, args.gold_rate, args.enhanced,
                               args.adventure, args.expedition)
        media = staged_hashes(playlist)
        command = [
            str(converter), "batch", str(playlist), "--system", "x68", "--output", str(work / "Games"),
            "--x68-ipl", str(ipl), "--x68-cgrom", str(cgrom), "--x68-ram", "2",
            "--scan-level", "quick", "--jobs", "1", "--no-shared-base", "--no-cover",
            "--no-online-cover", "--no-compress",
        ]
        if args.keep_build:
            command.append("--keep-build")
        env = dict(os.environ, RETRO_RECOMP_TOOLCHAIN=args.toolchain)
        env.setdefault("CMAKE_BUILD_PARALLEL_LEVEL", "3")
        recipe = {
            "version": VERSION, "toolchain": args.toolchain, "command": command,
            "disk_sha256": media, "ipl_sha256": digest(ipl), "cgrom_sha256": digest(cgrom),
        }
        (work / "recipe.json").write_text(json.dumps(recipe, indent=2) + "\n", encoding="utf-8")
        try:
            with (work / "conversion.log").open("wb") as log:
                subprocess.run(command, cwd=converter.parent, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        except subprocess.CalledProcessError as error:
            raise ValueError(conversion_error(work)) from error
        reports = list((work / "Games").rglob("conversion-report.json"))
        if len(reports) != 1:
            raise ValueError("Expected one conversion report. Inspect conversion.log.")
        print(f"Conversion report: {reports[0]}")
        install_verified_export(reports[0], ipl, cgrom, media, args)
    except (OSError, ValueError, KeyError, IndexError, TypeError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
