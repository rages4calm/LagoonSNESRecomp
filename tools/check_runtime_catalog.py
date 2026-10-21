"""Check that an installed/staged game has its exact matching public sidecars.

Run before handing off a local build or publishing a Windows preview. This
checks both mod manifests and both EPX files against the source checkout; it
does not inspect or change settings, saves, ROMs or soundtrack data. For a
configured preview, also pass --writable-root <preview>/local to check the
catalog/shader copies made by setup. Bind the reported EXE hash to the final
qualified build separately: matching sidecars alone do not qualify a game EXE.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


SIDECARS = (
    "mods/preloaded/packages/lagoon.quality/1.0.0/manifest.toml",
    "mods/preloaded/packages/lagoon.soundtrack/1.0.0/manifest.toml",
    "assets/shaders/lagoon-epx.glsl",
    "assets/shaders/lagoon-epx.glslp",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_runtime(source: Path, runtime: Path, writable: Path | None = None) -> dict:
    source, runtime = source.resolve(), runtime.resolve()
    roots = {"runtime": runtime}
    if writable is not None:
        roots["writable"] = writable.resolve()
    errors = []
    files = []
    executable = runtime / "LagoonSNESRecomp.exe"
    game_hash = sha256(executable) if executable.is_file() else None
    if game_hash is None:
        errors.append("Missing runtime executable: " + str(executable))
    for relative in SIDECARS:
        expected = source / relative
        expected_hash = sha256(expected) if expected.is_file() else None
        if expected_hash is None:
            errors.append("Missing source sidecar: " + str(expected))
        for role, root in roots.items():
            actual = root / relative
            actual_hash = sha256(actual) if actual.is_file() else None
            matches = expected_hash is not None and actual_hash == expected_hash
            files.append(dict(root=role, relative=relative,
                              source_sha256=expected_hash, actual_sha256=actual_hash,
                              matched=matches))
            if not matches:
                errors.append(("Missing" if actual_hash is None else "Stale or changed")
                              + " " + role + " sidecar: " + str(actual))
    return dict(passed=not errors, source_root=str(source),
                runtime_root=str(runtime),
                writable_root=str(roots["writable"]) if "writable" in roots else None,
                game_sha256=game_hash, files=files, errors=errors,
                scope="Exact public sidecar bytes only; EXE qualification is separate. No preferences, saves or private game data inspected.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime-root", type=Path, required=True,
                        help="Directory containing LagoonSNESRecomp.exe (build/ or preview/game/)")
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--writable-root", type=Path,
                        help="Optional preview/local/ after setup")
    parser.add_argument("--report", type=Path,
                        help="Optional fresh JSON receipt path; otherwise stdout only")
    args = parser.parse_args()
    if args.report is not None and args.report.exists():
        parser.error("Report must be fresh; existing file was preserved.")
    try:
        result = check_runtime(args.source_root, args.runtime_root, args.writable_root)
        text = json.dumps(result, indent=2) + "\n"
        if args.report is not None:
            with args.report.open("x", encoding="utf-8") as output:
                output.write(text)
        print(text, end="")
        return 0 if result["passed"] else 1
    except OSError as error:
        parser.exit(2, "Catalog check could not read inputs/write receipt: " + str(error) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
