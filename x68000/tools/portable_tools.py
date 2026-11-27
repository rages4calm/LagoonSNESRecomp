"""Install the exact portable Windows tools pinned by RetroRecomp 1.8.0.

Tools remain in the converter's per-user cache. No administrator installation,
system PATH change or Visual Studio installation is required.
"""

import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
from zipfile import ZipFile


TOOLS = (
    ("cmake", "4.4.4", "https://github.com/Kitware/CMake/releases/download/v4.4.4/cmake-4.4.4-windows-x86_64.zip",
     "bace36e94b31c68ab6fa295f26dfa11219e0701cf7c94b0284a7d1cb13dac536", 54400562, "bin/cmake.exe"),
    ("ninja", "1.13.2", "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip",
     "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65", 291570, "ninja.exe"),
    ("llvm-mingw", "20261006", "https://github.com/mstorsjo/llvm-mingw/releases/download/20261006/llvm-mingw-20261006-ucrt-x86_64.zip",
     "317492c456aa27ee607a5919f1d2d38dcdc1112516a24d0bf4b00d078f52d17a", 190836485, "bin/clang.exe"),
    ("mingit", "2.56.0.3", "https://github.com/git-for-windows/git/releases/download/v2.56.0.windows.3/MinGit-2.56.0.3-64-bit.zip",
     "2e5619966e588fde5f8122c5557cad50c0fbafcaed736689f40ec1dfb1d78ea7", 39854943, "cmd/git.exe"),
)


def linked(path):
    return path.is_symlink() or (path.exists() and path.resolve() != path.absolute())


def keep_file(tool, name):
    pieces = name.split("/")
    if tool == "llvm-mingw":
        if pieces[0] in ("i686-w64-mingw32", "aarch64-w64-mingw32", "armv7-w64-mingw32", "python", "busybox", "share"):
            return False
        if name.startswith(("lib/libear/", "lib/libscanbuild/")) or "/lib/linux/" in "/" + name or "/lib/darwin/" in "/" + name:
            return False
        if pieces[0] == "bin":
            leaf = pieces[-1].lower()
            return not leaf.startswith(("lldb", "liblldb", "libpython", "clangd", "clang-tidy", "clang-format",
                                        "git-clang", "run-clang", "scan-build", "python")) and not leaf.endswith((".py", ".sh"))
    if tool == "cmake":
        if name.startswith(("doc/", "man/", "share/vim", "share/emacs", "share/bash-completion", "share/aclocal")):
            return False
        if name.startswith("share/cmake-") and "/Help/" in name:
            return False
        if name.lower() in ("bin/cmake-gui.exe", "bin/cpack.exe", "bin/ctest.exe"):
            return False
    return True


def unpack(archive, destination, tool):
    root = destination.resolve()
    with ZipFile(archive) as source:
        entries = source.infolist()
        names = [e.filename.replace("\\", "/") for e in entries]
        first = names[0].split("/", 1)[0] + "/" if names and "/" in names[0] else ""
        top = first if first and all(name.startswith(first) for name in names) else ""
        for entry, original in zip(entries, names):
            name = original[len(top):]
            if not name or entry.is_dir() or not keep_file(tool, name):
                continue
            target = (destination / name).resolve()
            if not target.is_relative_to(root) or stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError("Unsafe path in verified portable-tool archive.")
            target.parent.mkdir(parents=True, exist_ok=True)
            with source.open(entry) as incoming, target.open("wb") as output:
                shutil.copyfileobj(incoming, output, 1024 * 1024)


def installed(root, row):
    tool, version, _, expected, _, marker = row
    try:
        stamp = json.loads((root / tool / "retro-recomp-tool.json").read_text())
        return stamp.get("version") == version and stamp.get("sha256") == expected and (root / tool / marker).is_file()
    except (OSError, ValueError):
        return False


def ensure(cache, fetch):
    root = cache / "datas/tools"
    root.mkdir(parents=True, exist_ok=True)
    for row in TOOLS:
        if installed(root, row):
            continue
        tool, version, url, expected, size, marker = row
        destination = root / tool
        if destination.exists() and linked(destination):
            raise ValueError("A linked portable-tool cache cannot be used.")
        print(f"Downloading portable {tool} {version} ({size // 1048576} MB). No system installation is needed...", flush=True)
        with tempfile.TemporaryDirectory(prefix="lagoon-tool-", dir=root) as scratch:
            work = Path(scratch)
            archive = work / "tool.zip"
            fetch(url, archive, size + 1)
            if hashlib.sha256(archive.read_bytes()).hexdigest() != expected:
                raise ValueError(f"The {tool} download failed its SHA-256 check. Please run setup again.")
            stage = work / "tool"
            unpack(archive, stage, tool)
            if not (stage / marker).is_file():
                raise ValueError(f"The verified {tool} archive has an unexpected layout.")
            destination.mkdir(exist_ok=True)
            # Install only members of this verified archive. Unrelated cache
            # files are not deleted; a completed stamp is written last.
            for source in stage.rglob("*"):
                if not source.is_file():
                    continue
                target = destination / source.relative_to(stage)
                cursor = target.parent
                while cursor != root:
                    if cursor.exists() and linked(cursor):
                        raise ValueError("Linked portable-tool cache component refused.")
                    cursor = cursor.parent
                if linked(target):
                    raise ValueError("Linked portable-tool cache file refused.")
                target.parent.mkdir(parents=True, exist_ok=True)
                os.replace(source, target)
            (destination / "retro-recomp-tool.json").write_text(json.dumps({"version": version, "sha256": expected}))
    bins = [root / "llvm-mingw/bin", root / "cmake/bin", root / "ninja", root / "mingit/cmd"]
    os.environ["PATH"] = os.pathsep.join([*map(str, bins), os.environ.get("PATH", "")])
    os.environ["RETRO_RECOMP_TOOLCHAIN"] = "llvm"
    verify(root)


def verify(root):
    def run(command):
        result = subprocess.run(list(map(str, command)), capture_output=True, text=True,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0), timeout=120)
        if result.returncode:
            raise ValueError("Portable build-tool check failed: " + result.stderr[-800:])
        return result.stdout
    run([root / "cmake/bin/cmake.exe", "--version"])
    run([root / "ninja/ninja.exe", "--version"])
    run([root / "mingit/cmd/git.exe", "--version"])
    with tempfile.TemporaryDirectory(prefix="lagoon-compiler-check-") as directory:
        work = Path(directory)
        source, program = work / "check.cpp", work / "check.exe"
        source.write_text('#include <iostream>\nint main(){std::cout << "Lagoon portable tools ready";}\n')
        run([root / "llvm-mingw/bin/clang++.exe", "-static", source, "-o", program])
        if run([program]) != "Lagoon portable tools ready":
            raise ValueError("The portable compiler did not produce a working program.")
    print("Portable build tools verified.", flush=True)
