"""Set up X68000 Enhanced from the player's own disks and firmware."""

import argparse
from contextlib import contextmanager, ExitStack
import msvcrt
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile
from urllib.request import Request, urlopen
from zipfile import BadZipFile, ZipFile
import zlib

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from build_retrorecomp import (
    DISK_HASHES, PROJECT, WINDOWS_CONVERTER, checked_file, digest,
)
from portable_tools import ensure as ensure_portable_tools
from portable_tools import installed as tool_installed, TOOLS as PORTABLE_TOOLS
from firmware import resolve as resolve_firmware, install_notice
from runtime_cache import CacheUnavailable, read_cache, migrate_report


CONVERTER_URL = "https://github.com/ArthurReboulSalze/RetroRecomp/releases/download/v1.8.0/Retro-Recomp.exe"
PATCH_URL = "https://github.com/nleseul/lagoon_x68k_trans/releases/download/0.9b/Lagoon.X68000.-.EN.patch.0.9b.zip"
PATCH_FILE = "0.9b/Lagoon (1990)(Zoom)(Disk 1 of 4)(System)[EN].bps"
INSTALL = PROJECT / "local-x68000/retrorecomp-expedition"
CACHE = Path(os.environ["LOCALAPPDATA"]) / "L68"
DEFAULTS = b"[X68000]\r\ndisplay_mode=0\r\nfilter=0\r\n\r\n[X68000.Options]\r\nview_zoom=1\r\naudio_volume=80\r\n"


def fetch(url, output, limit):
    request = Request(url, headers={"User-Agent": "Lagoon-X68000-Setup"})
    with urlopen(request, timeout=60) as response, output.open("wb") as file:
        remaining = limit
        for block in iter(lambda: response.read(1024 * 1024), b""):
            remaining -= len(block)
            if remaining < 0:
                raise ValueError(f"Download exceeded the size limit for {output.name}.")
            file.write(block)


def run(tool, *args):
    subprocess.run([sys.executable, "-B", "-P", str(TOOLS / tool), *map(str, args)],
                   cwd=PROJECT, check=True)


def files_by_name(inputs):
    files = {}
    for item in sorted(inputs.rglob("*")):
        if item.is_file():
            files.setdefault(item.name.lower(), []).append(item)
    return files


def input_files(inputs, directory):
    """Read a folder or bounded ZIP without writing into the player's inputs."""
    inputs = inputs.resolve(strict=True)
    if inputs.is_dir():
        files = files_by_name(inputs)
        archives = [p for paths in files.values() for p in paths if p.suffix.lower() == ".zip"]
    elif inputs.is_file() and inputs.suffix.lower() == ".zip":
        files, archives = {}, [inputs]
    else:
        raise ValueError("Choose a folder containing the Lagoon disks, or the Lagoon disk ZIP.")
    for number, archive in enumerate(archives):
        with ZipFile(archive) as source:
            entries = source.infolist()
            if len(entries) > 512:
                raise ValueError("The input ZIP has too many entries. Use the four Lagoon DIM disks.")
            total = 0
            for index, entry in enumerate(entries):
                if entry.is_dir():
                    continue
                name = entry.filename.replace("\\", "/").rsplit("/", 1)[-1]
                if Path(name).suffix.lower() not in (".dim", ".bps", ".dat"):
                    continue
                if entry.flag_bits & 1 or stat.S_ISLNK(entry.external_attr >> 16):
                    raise ValueError("Encrypted files and links are not supported in the input ZIP.")
                total += entry.file_size
                if entry.file_size > 8 * 1024 * 1024 or total > 64 * 1024 * 1024:
                    raise ValueError("The input ZIP is larger than the supported Lagoon disk set.")
                # Ignore archive directory names, so ../ and absolute paths can
                # never escape this private temporary import directory.
                target = directory / "imported" / f"{number}-{index}" / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.read(entry))
                files.setdefault(name.lower(), []).append(target)
    return files


def find_file(files, name, size):
    matches = [path for path in files.get(name, ()) if path.stat().st_size == size]
    if len({digest(path) for path in matches}) != 1:
        raise ValueError(f"Place one valid {name} ({size:,} bytes) in the Inputs folder.")
    return matches[0]


def prepare_disks(files, directory):
    images = {digest(path): path for name, paths in files.items() if name.endswith(".dim")
              for path in paths if path.stat().st_size == 1261824}
    for name, sha in list(DISK_HASHES.items())[1:]:
        if sha not in images:
            raise ValueError(f"Missing the supported Lagoon disk: {name}")
        shutil.copyfile(images[sha], directory / name)
    english = directory / "System-EN.dim"
    expected = DISK_HASHES[english.name]
    if expected in images:
        shutil.copyfile(images[expected], english)
        return english
    system = next((path for path in images.values() if zlib.crc32(path.read_bytes()) == 0x684A43EF), None)
    if system is None:
        raise ValueError("Add the supported Japanese System DIM or English 0.9b System DIM to Inputs.")
    patches = [path for name, paths in files.items() if name.endswith(".bps") for path in paths]
    if len(patches) > 1:
        raise ValueError("Keep only the English 0.9b System BPS in Inputs, or let setup download it.")
    if patches:
        patch = patches[0]
    else:
        print("Downloading nleseul's English 0.9b translation patch...", flush=True)
        archive = directory / "translation.zip"
        fetch(PATCH_URL, archive, 8 * 1024 * 1024)
        with ZipFile(archive) as source:
            items = [info for info in source.infolist() if info.filename == PATCH_FILE
                     and info.file_size <= 2 * 1024 * 1024]
            if len(items) != 1:
                raise ValueError("The translation archive has an unexpected layout. Place its English BPS in Inputs.")
            patch = directory / "english.bps"
            patch.write_bytes(source.read(items[0]))
    run("bps_apply.py", system, patch, english)
    return checked_file(english, expected, 1261824)


def converter_path(files, directory):
    cached = CACHE / "Retro-Recomp.exe"
    for path in [cached, PROJECT / "local-x68000/converter/Retro-Recomp.exe", *files.get("retro-recomp.exe", ())]:
        if path.is_file() and digest(path) == WINDOWS_CONVERTER:
            if path != cached:
                shutil.copyfile(path, cached)
            return cached
    print("Downloading RetroRecomp 1.8.0...", flush=True)
    downloaded = directory / "Retro-Recomp.exe"
    fetch(CONVERTER_URL, downloaded, 512 * 1024 * 1024)
    checked_file(downloaded, WINDOWS_CONVERTER)
    shutil.copyfile(downloaded, cached)
    return cached


@contextmanager
def build_lock():
    CACHE.mkdir(parents=True, exist_ok=True)
    with (CACHE / "setup.lock").open("a+b") as lock:
        lock.seek(0)
        try:
            msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        except OSError as error:
            raise ValueError("Another X68000 setup is running. Wait for it to finish before starting one again.") from error
        try:
            yield
        finally:
            lock.seek(0)
            msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)


def seed_defaults(install):
    ini = install / "datas/Retro-Recomp.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    try:
        with ini.open("xb") as output:
            output.write(DEFAULTS)
    except FileExistsError:
        pass


def repair_legacy_build_objects():
    root = CACHE / ".build/x68/win"
    if not root.is_dir():
        return
    for obj in root.glob("*/*/native/build/reference/x68000_reference_host.dir/Release/reference_host.obj"):
        tool, game, *_ = obj.relative_to(root).parts
        if (not re.fullmatch(r"[0-9a-f]{12}", tool) or not re.fullmatch(r"[0-9a-f]{16}", game)
                or obj.resolve() != obj.absolute()):
            continue
        # Preview 1 used this object directory for the base and patched hosts.
        # Force MSBuild to compile the source again before it uses the cache.
        if b"lagoon_load_zoom" in obj.read_bytes():
            obj.unlink(missing_ok=True)


def data_fingerprints():
    return {str(path): (digest(path), path.stat().st_size, path.stat().st_mtime_ns)
            for path in (INSTALL / "datas").rglob("*") if path.is_file() and path.name != "session.lock"}


@contextmanager
def game_session_lock(path):
    """Hold the same share-none Windows guard as the running player."""
    import ctypes
    from ctypes import wintypes
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    create = kernel.CreateFileW
    create.argtypes = (wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p,
                       wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE)
    create.restype = wintypes.HANDLE
    close = kernel.CloseHandle
    close.argtypes = (wintypes.HANDLE,)
    close.restype = wintypes.BOOL
    handle = create(str(path), 0x80000000 | 0x40000000, 0, None, 4, 0x80, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ValueError("Close the X68000 game before updating it.")
    try:
        yield
    finally:
        close(handle)


def rebuild_cached():
    """Compile only the host, using validated retained base and private data."""
    from build_atlas_player import build, install
    import json
    report_path = INSTALL / "conversion-report.json"
    try:
        report = json.loads(report_path.read_text())
        base = json.loads((INSTALL / "lagoon-extra-install.json").read_text())
        if (report.get("version") != "1.8.0" or report.get("status") != "supported" or
                report.get("toolchain", {}).get("kind") != "llvm" or
                not base.get("reference_validated") or not base.get("expedition") or
                base.get("source_identity") != report.get("source_identity") or
                base.get("sha256") != report["export_receipt"]["sha256"]):
            raise ValueError("The retained enhanced base receipt differs.")
        checked_file(INSTALL / "Lagoon - English.exe", base["sha256"])
        built = checked_file(Path(report["build_executable"]))
        build_dir = next((path for path in list(built.parents)[:4] if (path / "CMakeCache.txt").is_file()), None)
        if build_dir is None:
            raise ValueError("The retained build project is missing.")
        project = build_dir.parent.parent
        if project.name != report["source_identity"][:16]:
            raise ValueError("The retained build source identity differs.")
        checked_file(project / "packed-game.exe", base["sha256"])
        if not all(tool_installed(CACHE / "datas/tools", row) for row in PORTABLE_TOOLS):
            raise ValueError("The portable build tools are missing.")
    except (OSError, ValueError, KeyError) as error:
        raise CacheUnavailable("X68000's original build cache is unavailable. Choose the Lagoon ZIP once to prepare it again.") from error
    before = data_fingerprints()
    with build_lock(), ExitStack() as sessions, tempfile.TemporaryDirectory(prefix="lagoon-x68000-rebuild-") as scratch:
        for game in (INSTALL / "datas/games").glob("x68-*"):
            if game.is_dir():
                sessions.enter_context(game_session_lock(game / "session.lock"))
        # Holding locks can create a previously absent empty lock file. This
        # is synchronization metadata, never a save/settings mutation.
        before = data_fingerprints()
        work = Path(scratch)
        if (INSTALL / "runtime-cache.json").is_file():
            read_cache(INSTALL, report["source_identity"], base["sha256"])
            cached = INSTALL
        else:
            print("Preparing the existing0.6 game data for future one-click updates...", flush=True)
            cached = migrate_report(report, work / "migrated")
        # All tools were verified present above, so this only validates their
        # existing install and puts it on this process's PATH. No downloads.
        ensure_portable_tools(CACHE, fetch)
        print("Updating X68000 from its saved build data. Your saves and settings stay in place...", flush=True)
        executable, identity, areas, proof = build(report_path, work / "atlas", None, None,
                                                  opening_only=True, cached_atlas=cached)
        if data_fingerprints() != before:
            raise ValueError("Game data changed during compilation; the current engine was not replaced.")
        install(executable, identity, areas, proof, expedition=True)
        if data_fingerprints() != before:
            raise ValueError("Save/settings preservation check failed after rebuilding.")
    print("X68000 game updated. All saves and settings were preserved.", flush=True)


def main():
    if sys.version_info < (3, 11):
        raise SystemExit("X68000 setup needs Python 3.11 or newer. Install it, then run Setup X68000.cmd again.")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, default=PROJECT / "Inputs/X68000",
                        help="Folder containing the four Lagoon DIM disks, or their ZIP.")
    parser.add_argument("--ipl", type=Path, help="Your 128 KiB iplrom.dat firmware file.")
    parser.add_argument("--cgrom", type=Path, help="Your 768 KiB cgrom.dat font firmware file.")
    parser.add_argument("--check-inputs", action="store_true", help="Verify inputs without compiling or installing.")
    parser.add_argument("--rebuild-cached", action="store_true", help="Update only the host from its existing verified private build cache.")
    args = parser.parse_args()
    try:
        if args.rebuild_cached:
            rebuild_cached()
            return
        # RetroRecomp builds beside its own executable. MSVC's file tracker can
        # exceed its Windows path limit under a deeply nested extraction folder.
        with build_lock(), tempfile.TemporaryDirectory(prefix="lagoon-x68000-setup-") as scratch:
            work = Path(scratch)
            files = input_files(args.inputs, work)
            ipl, font, permission = resolve_firmware(files, args.ipl, args.cgrom, CACHE, fetch, find_file)
            system = prepare_disks(files, work)
            if args.check_inputs:
                print("Inputs verified: four supported disks, English System disk, IPL and font firmware. No game was built or installed.")
                return
            converter = converter_path(files, work)
            ensure_portable_tools(CACHE, fetch)
            repair_legacy_build_objects()
            print("Building and validating the English base player. This can take several minutes...", flush=True)
            run("build_retrorecomp.py", "--converter", converter, "--ipl", ipl, "--cgrom", font,
                "--disks", work, "--english-system", system, "--user-disk", "--expedition",
                "--exp-rate", 2, "--gold-rate", 2, "--keep-build", "--resume", "--toolchain", "llvm")
            print("Building the playable Atlas and comparing its opening route...", flush=True)
            run("build_atlas_player.py", INSTALL / "conversion-report.json", work / "atlas",
                "--disks", work, "--english-system", system, "--expedition", "--opening-only", "--install")
            install_notice(INSTALL, permission)
        seed_defaults(INSTALL)
    except CacheUnavailable as error:
        parser.exit(3, f"X68000 setup needs its game ZIP once: {error}\n")
    except (OSError, ValueError, BadZipFile, subprocess.CalledProcessError) as error:
        parser.exit(1, f"X68000 setup stopped: {error}\n")
    print("Setup complete. Open Lagoon Companion, select X68000 Enhanced and press Play.")


if __name__ == "__main__":
    main()
