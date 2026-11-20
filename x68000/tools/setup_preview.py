"""Build the Windows X68000 preview from the player's local inputs."""

import argparse
from contextlib import contextmanager
import msvcrt
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from urllib.request import Request, urlopen
from zipfile import ZipFile
import zlib

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from build_retrorecomp import (
    DISK_HASHES, PROJECT, WINDOWS_CONVERTER, checked_file, digest,
)


CONVERTER_URL = "https://github.com/ArthurReboulSalze/RetroRecomp/releases/download/v1.8.0/Retro-Recomp.exe"
PATCH_URL = "https://github.com/nleseul/lagoon_x68k_trans/releases/download/0.9b/Lagoon.X68000.-.EN.patch.0.9b.zip"
PATCH_FILE = "0.9b/Lagoon (1990)(Zoom)(Disk 1 of 4)(System)[EN].bps"
INSTALL = PROJECT / "local-x68000/retrorecomp-expedition"
CACHE = Path(os.environ["LOCALAPPDATA"]) / "L68"
DEFAULTS = b"[X68000]\r\ndisplay_mode=0\r\nfilter=0\r\n\r\n[X68000.Options]\r\nview_zoom=2\r\naudio_volume=80\r\n"


def fetch(url, output, limit):
    request = Request(url, headers={"User-Agent": "Lagoon-X68000-Preview"})
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
        print("Downloading nleseul's English 0.9b translation patch…", flush=True)
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
    print("Downloading RetroRecomp 1.8.0…", flush=True)
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", type=Path, default=PROJECT / "Inputs")
    args = parser.parse_args()
    files = files_by_name(args.inputs)
    try:
        ipl = find_file(files, "iplrom.dat", 131072)
        font = find_file(files, "cgrom.dat", 786432)
        # RetroRecomp builds beside its own executable. MSVC's file tracker can
        # exceed its Windows path limit under a deeply nested extraction folder.
        with build_lock(), tempfile.TemporaryDirectory(prefix="lagoon-x68000-setup-") as scratch:
            work = Path(scratch)
            system = prepare_disks(files, work)
            converter = converter_path(files, work)
            print("Building X68000 Enhanced. First setup runs the opening and widescreen checks.", flush=True)
            run("build_retrorecomp.py", "--converter", converter, "--ipl", ipl, "--cgrom", font,
                "--disks", work, "--english-system", system, "--user-disk", "--expedition",
                "--exp-rate", 2, "--gold-rate", 2, "--keep-build")
            run("build_atlas_player.py", INSTALL / "conversion-report.json", work / "atlas",
                "--disks", work, "--english-system", system, "--expedition", "--install")
        seed_defaults(INSTALL)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"X68000 setup stopped: {error}\n")
    print("Setup complete. Open Play X68000.cmd and select Play.")


if __name__ == "__main__":
    main()
