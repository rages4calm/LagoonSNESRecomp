#!/usr/bin/env python3
"""Convert an owner-supplied Lagoon VGM/VGZ ZIP into a private PCM pack.

Calls the separately built libvgm vgm2wav executable; no emulator library is
linked into Lagoon. This tool never downloads music or changes game settings.
"""
from __future__ import annotations

import sys
from pathlib import Path

# tools/dis.py is an existing ROM utility and shadows Python's standard dis
# module. This standalone importer uses only stdlib dependencies, so exclude
# its own directory before argparse -> inspect imports that standard module.
sys.path = [entry for entry in sys.path if Path(entry).resolve() != Path(__file__).resolve().parent]

import argparse
import array
import gzip
import hashlib
import io
import json
import os
import re
import struct
import subprocess
import uuid
import wave
import zipfile

REFERENCE_LIBVGM_REVISION = "a6eed945ecf4e5a25cf82004779c6f7e4fd5c319"
MAX_SOURCE_BYTES = 64 * 1024 * 1024
MAX_FRAMES = 44100 * 30 * 60


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_vgm(data: bytes) -> tuple[bytes, dict]:
    if data.startswith(b"\x1f\x8b"):
        with gzip.GzipFile(fileobj=io.BytesIO(data)) as stream:
            data = stream.read(MAX_SOURCE_BYTES + 1)
    if len(data) > MAX_SOURCE_BYTES or len(data) < 0x94 or data[:4] != b"Vgm ":
        raise ValueError("Invalid, truncated or oversized VGM source")
    u32 = lambda offset: struct.unpack_from("<I", data, offset)[0]
    total = u32(0x18)
    loop = u32(0x20)
    relative_loop = u32(0x1C)
    if not 0 < total <= MAX_FRAMES or loop > total:
        raise ValueError("Invalid VGM duration or loop sample count")
    if bool(relative_loop) != bool(loop):
        raise ValueError("VGM loop pointer/sample metadata disagree")
    if relative_loop and relative_loop + 0x1C >= len(data):
        raise ValueError("VGM loop pointer lies outside the source")
    if not u32(0x30) or not u32(0x90):
        raise ValueError("Expected the Lagoon X68000 YM2151 + OKIM6258 source")
    return data, {
        "vgm_version": f"{u32(8):x}",
        "total_frames": total,
        "loop_frames": loop,
        "loop_start_frame": total - loop if loop else None,
        "loop_vgm_offset": relative_loop + 0x1C if relative_loop else None,
        "ym2151_clock": u32(0x30) & 0x3FFFFFFF,
        "okim6258_clock": u32(0x90) & 0x3FFFFFFF,
    }


def convert(source: Path, converter: Path, output: Path, selected: set[int] | None) -> dict:
    if output.exists():
        raise FileExistsError("Output must be a fresh directory; existing packs are never replaced")
    if output == source or source in output.parents:
        raise ValueError("Output cannot replace or live inside the source archive")
    source_digest = sha256(source.read_bytes())
    converter_digest = sha256(converter.read_bytes())
    output.parent.mkdir(parents=True, exist_ok=True)
    staging = output.parent / (output.name + ".partial-" + uuid.uuid4().hex)
    staging.mkdir()
    success = False
    try:
        manifest = {
            "schema": "lagoon-x68000-pack-v1",
            "source_archive": source.name,
            "source_sha256": source_digest,
            "converter": converter.name,
            "converter_sha256": converter_digest,
            "reference_libvgm_revision": REFERENCE_LIBVGM_REVISION,
            "reference_revision_note": "Build recipe reference; binary identity is the SHA256 above.",
            "source_credits": "Music: Hideki Suzuki / ZOOM. VGMRips package: GTheGuardian; optimization: Valley Bell.",
            "sample_rate": 44100,
            "channels": 2,
            "sample_bits": 16,
            "base": "lagoon-x68000",
            "game_mapping_status": "not yet qualified; archive track numbers are NOT SNES music command IDs",
            "tracks": [],
        }
        seen: set[int] = set()
        with zipfile.ZipFile(source) as archive:
            members = sorted((m for m in archive.infolist() if m.filename.lower().endswith((".vgz", ".vgm"))), key=lambda m: m.filename)
            for member in members:
                match = re.fullmatch(r"(\d{2}) (.+)\.vg[zm]", Path(member.filename).name, re.IGNORECASE)
                if not match:
                    raise ValueError("Unrecognized Lagoon track filename: " + member.filename)
                number = int(match[1])
                if number in seen or not 1 <= number <= 40:
                    raise ValueError("Duplicate or out-of-range archive track number")
                seen.add(number)
                if selected is not None and number not in selected:
                    continue
                if member.file_size > MAX_SOURCE_BYTES:
                    raise ValueError("Oversized ZIP member")
                packed = archive.read(member)
                _, metadata = read_vgm(packed)
                input_path = staging / f"source-{number:02}.vgz"
                wav_path = staging / f"render-{number:02}.wav"
                log_path = staging / f"render-{number:02}.log"
                input_path.write_bytes(packed)
                command = [str(converter), "--samplerate", "44100", "--bps", "16", "--loops", "1", "--fade", "0", str(input_path), str(wav_path)]
                result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300, check=False)
                log_path.write_bytes(result.stdout)
                if result.returncode:
                    raise RuntimeError(f"vgm2wav failed for track {number}; see {log_path}")
                # A binary silently built without the OKIM6258 core would omit
                # the drums. Require evidence that BOTH devices were created.
                log = result.stdout.decode("utf-8", errors="replace")
                if "Type 0x03" not in log or "Type 0x17" not in log or "devices: 2" not in log:
                    raise RuntimeError("Converter did not initialize both required sound chips")
                with wave.open(str(wav_path), "rb") as wav:
                    if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) != (2, 2, 44100, "NONE"):
                        raise ValueError("Converter emitted an unsupported WAV format")
                    if wav.getnframes() != metadata["total_frames"]:
                        raise ValueError(f"Rendered length disagrees with VGM header for track {number}")
                    pcm = wav.readframes(wav.getnframes())
                if len(pcm) != metadata["total_frames"] * 4:
                    raise ValueError("Truncated converted PCM")
                samples = array.array("h", pcm)
                if sys.byteorder != "little":
                    samples.byteswap()
                peak = max(abs(value) for value in samples)
                if peak == 0:
                    raise ValueError("Converter produced a silent track")
                name = f"lagoon-x68000-{number}.pcm"
                contents = b"MSU1" + struct.pack("<I", metadata["loop_start_frame"] or 0) + pcm
                (staging / name).write_bytes(contents)
                manifest["tracks"].append({
                    "archive_track": number, "title": match[2],
                    "source_member": member.filename, "source_sha256": sha256(packed),
                    "file": name, "sha256": sha256(contents),
                    "loop": bool(metadata["loop_frames"]),
                    "peak_s16": peak,
                    "full_scale_samples": sum(value == -32768 or value == 32767 for value in samples),
                    **metadata,
                })
                input_path.unlink(); wav_path.unlink()
                print(f"Converted {number:02}: {match[2]} ({metadata['total_frames']} frames)", flush=True)
            if seen != set(range(1, 41)):
                raise ValueError("Expected all 40 source tracks in the supplied Lagoon archive")
        if selected is not None and selected - seen:
            raise ValueError("Requested track is absent")
        if sha256(source.read_bytes()) != source_digest:
            raise RuntimeError("Source ZIP changed during conversion")
        # Audio-only MSU packs still carry the empty data file. No ROM is made.
        (staging / "lagoon-x68000.msu").write_bytes(b"")
        (staging / "pack.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        os.rename(staging, output)
        success = True
        return manifest
    finally:
        # Retain a failed private staging directory for diagnostics; it never
        # becomes an active pack and no existing output is touched.
        if not success and staging.exists():
            print(f"Incomplete private conversion retained at {staging}", file=sys.stderr)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, help="Owner-provided Lagoon_(Sharp_X68000).zip")
    parser.add_argument("--vgm2wav", type=Path, required=True, help="External libvgm converter with YM2151 and MSM6258 enabled")
    parser.add_argument("--output", type=Path, required=True, help="Fresh private directory outside the source/public checkout")
    parser.add_argument("--track", type=int, action="append", help="Convert only these archive track numbers, for a bounded probe")
    args = parser.parse_args()
    source = args.archive.resolve(strict=True)
    converter = args.vgm2wav.resolve(strict=True)
    output = args.output.resolve()
    repo = Path(__file__).resolve().parents[1]
    if output == repo or repo in output.parents:
        parser.error("Choose a private output directory outside the public source checkout")
    selected = set(args.track) if args.track else None
    if selected is not None and not selected <= set(range(1, 41)):
        parser.error("--track must be between 1 and 40")
    result = convert(source, converter, output, selected)
    print(f"Private pack complete: {len(result['tracks'])} tracks; source ZIP unchanged. Game integration is not yet qualified.")


if __name__ == "__main__":
    main()
