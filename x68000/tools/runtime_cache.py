"""Private, identity-bound Atlas build data for one-click host rebuilds."""

import hashlib
import json
from pathlib import Path
import shutil

from build_retrorecomp import DISK_HASHES, checked_file, digest
from lagoon_atlas import AREA_NAMES_SHA256, generate


class CacheUnavailable(ValueError):
    pass


def write_cache(directory, data, identity, base_sha256, areas):
    if areas != 49 or not 0 < len(data) < 32 * 1024 * 1024:
        raise ValueError("Unexpected Atlas cache data.")
    header = directory / "runtime-cache/atlas-data.inc"
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_bytes(data)
    (directory / "runtime-cache.json").write_text(json.dumps({
        "schema": 1, "source_identity": identity, "base_sha256": base_sha256,
        "atlas_data_sha256": hashlib.sha256(data).hexdigest(), "areas": areas,
        "english_area_names_sha256": AREA_NAMES_SHA256,
    }, indent=2))


def read_cache(directory, identity, base_sha256):
    try:
        info = json.loads((directory / "runtime-cache.json").read_text())
        header = directory / "runtime-cache/atlas-data.inc"
        if (info.get("schema") != 1 or info.get("source_identity") != identity or
                info.get("base_sha256") != base_sha256 or info.get("areas") != 49 or
                info.get("english_area_names_sha256") != AREA_NAMES_SHA256 or
                header.is_symlink() or not header.resolve().is_relative_to(directory.resolve()) or
                not 0 < header.stat().st_size < 32 * 1024 * 1024 or
                digest(header) != info.get("atlas_data_sha256")):
            raise ValueError("Atlas cache identity or checksum differs.")
        data = header.read_bytes()
        data.decode("ascii", errors="strict")
        return data, 49
    except (OSError, ValueError, KeyError) as error:
        raise CacheUnavailable("The cached X68000 build data is missing or changed. Choose the Lagoon ZIP once to rebuild it.") from error


def migrate_report(report, output):
    """Recover0.6's cache from verified retained converter inputs, not saves.

    The three data disks still use the original full-file pins. The enhanced
    System disk uses its verified conversion hash AND the exact original
    English0.9b area-name table. No relaxed or fabricated game identity is used.
    """
    try:
        media = report["machine"]["media"]
        if report["version"] != "1.8.0" or report["status"] != "supported" or len(media) != 5:
            raise ValueError("Unrecognized retained converter inputs.")
        output.mkdir(parents=True, exist_ok=True)
        for index, (name, expected) in enumerate(list(DISK_HASHES.items())[1:], 1):
            source = checked_file(Path(media[index]["path"]), expected, 1261824)
            if media[index]["sha256"] != expected:
                raise ValueError("Retained map-disk identity differs.")
            destination = output / name
            try:
                destination.hardlink_to(source)
            except OSError:
                shutil.copyfile(source, destination)
        system = checked_file(Path(media[0]["path"]), media[0]["sha256"], 1261824)
        header = output / "atlas-data.inc"
        areas = generate(output, system, header, system_sha256=media[0]["sha256"])
        write_cache(output, header.read_bytes(), report["source_identity"], report["export_receipt"]["sha256"], areas)
        return output
    except (OSError, ValueError, KeyError) as error:
        raise CacheUnavailable("The older X68000 build cache is unavailable. Select your Lagoon ZIP once; later engine updates can reuse its cache.") from error
