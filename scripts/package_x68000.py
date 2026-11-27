"""Explicit public X68000 setup payload; never includes a built game or inputs."""

import hashlib
import json
from pathlib import Path


RUNTIME = (
    "lagoon_atlas.inc", "lagoon_boot.inc", "lagoon_controls.inc", "lagoon_door.inc",
    "lagoon_exploration.inc", "lagoon_extended_actors.inc", "lagoon_journal.inc",
    "lagoon_scene_view.inc", "lagoon_view_state.inc", "lagoon_wide.inc", "lagoon_world_layers.inc",
)
TOOLS = (
    "bps_apply.py", "build_atlas_player.py", "build_retrorecomp.py", "check_wide_areas.py",
    "lagoon_atlas.py", "lagoon_qol.py", "portable_tools.py", "firmware.py", "runtime_cache.py", "setup_preview.py", "setup_bootstrap.ps1",
)


def public_inputs(root: Path) -> dict[str, str]:
    root = Path(root).resolve(strict=True)
    names = ["Setup X68000.cmd", "x68000/achievements.json", "x68000/PLAYING.md", "x68000/runtime-version.json",
             "src/lagoon_ui_font.generated.h", "assets/fonts/atkinson-hyperlegible/OFL.txt",
             "assets/fonts/atkinson-hyperlegible/provenance.json", "licenses/X68000-Sharp-Permission.txt"]
    names += ["x68000/runtime/" + name for name in RUNTIME]
    names += ["x68000/tools/" + name for name in TOOLS]
    result = {}
    for name in names:
        source = root / name
        if not source.is_file() or source.is_symlink() or not source.resolve().is_relative_to(root):
            raise ValueError("Missing or linked X68000 setup source: " + name)
        if not 0 < source.stat().st_size < 1024 * 1024:
            raise ValueError("Unexpected X68000 setup source size: " + name)
        source.read_text(encoding="utf-8")  # All public additions must be text.
        result[str(source)] = name
    font = root / "assets/fonts/atkinson-hyperlegible/provenance.json"
    if hashlib.sha256((root / "licenses/X68000-Sharp-Permission.txt").read_bytes()).hexdigest() != \
            "21099eaab9417c5c8cea4331e51abc4a144062314ac1d7c110aefaab366b4794":
        raise ValueError("Compatible firmware permission text must remain unchanged.")
    record = json.loads(font.read_text())
    if record.get("license") != "SIL Open Font License 1.1":
        raise ValueError("Missing X68000 UI font license provenance.")
    for name, raw, canonical in (
        ("src/lagoon_ui_font.generated.h", record.get("generated_header_sha256"), record.get("canonical_generated_header_sha256")),
        ("assets/fonts/atkinson-hyperlegible/OFL.txt", record["files"]["OFL.txt"].get("sha256"), record["files"]["OFL.txt"].get("canonical_sha256")),
    ):
        data = (root / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != raw and hashlib.sha256(data.replace(b"\r\n", b"\n")).hexdigest() != canonical:
            raise ValueError("X68000 UI font source differs from provenance: " + name)
    return result
