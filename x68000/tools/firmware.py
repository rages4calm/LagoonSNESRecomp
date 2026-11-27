"""Pinned, redistributable X68000 firmware used by the qualified local build."""

import json
import shutil

from build_retrorecomp import checked_file, digest


COMMIT = "96abd36d33f8f8b78e114b4847d449c5c0684498"
BASE = "https://raw.githubusercontent.com/uraraworks/WebX68k/" + COMMIT + "/public/system/"
FILES = {
    "iplrom.dat": (131072, "8ead1d0f4ebb9c59a7fa118596f819e191c310442a00c56ab5ec5e9e7a189677"),
    "cgrom.dat": (786432, "e1669a796eb80ba9fd800c566584b0322e875da79abe25d9ff0597a7ecc317b3"),
}
PERMISSION_URL = BASE + "%E8%A8%B1%E8%AB%BE%E6%9D%A1%E4%BB%B6.txt"
PERMISSION_SHA256 = "21099eaab9417c5c8cea4331e51abc4a144062314ac1d7c110aefaab366b4794"


def resolve(files, ipl_override, cgrom_override, cache, fetch, find_file):
    directory = cache / "firmware-webx68k" / COMMIT
    directory.mkdir(parents=True, exist_ok=True)
    result = []
    for name, override in (("iplrom.dat", ipl_override), ("cgrom.dat", cgrom_override)):
        size, expected = FILES[name]
        if override:
            result.append(checked_file(override, size=size))
        elif files.get(name):
            result.append(find_file(files, name, size))
        else:
            target = directory / name
            if not target.is_file() or digest(target) != expected:
                print("Downloading compatible " + name + "...", flush=True)
                pending = directory / (name + ".download")
                fetch(BASE + name, pending, size + 1)
                checked_file(pending, expected, size)
                pending.replace(target)
            result.append(checked_file(target, expected, size))
    known = all(digest(path) == FILES[path_name][1]
                for path, path_name in zip(result, ("iplrom.dat", "cgrom.dat")))
    permission = None
    if known:
        permission = directory / "X68000-Sharp-Permission.txt"
        if not permission.is_file() or digest(permission) != PERMISSION_SHA256:
            pending = directory / "permission.download"
            fetch(PERMISSION_URL, pending, 6172)
            checked_file(pending, PERMISSION_SHA256, 6171)
            pending.replace(permission)
    return (*result, permission)


def install_notice(install, permission):
    if permission:
        destination = install / "licenses/X68000-Sharp-Permission.txt"
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(permission, destination)
        (destination.parent / "compatible-firmware.json").write_text(json.dumps({
            "repository": "uraraworks/WebX68k", "commit": COMMIT,
            "files": {name: sha for name, (_, sha) in FILES.items()},
            "permission_sha256": PERMISSION_SHA256,
            "font": "Public-domain Shinonome compatible CGROM from the same pinned repository.",
        }, indent=2))
