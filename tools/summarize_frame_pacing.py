"""Summarize buffered SNESRECOMP_FRAME_TIMING without hiding diagnostic stalls.

Capture with --no-present-log and no midroute dump/screenshots for a quiet timing
run. --checkpoint FRAME:TAG records a *declared* host-frame checkpoint boundary;
it does not remove that interval from any statistic. A matching quiet replay is
needed before attributing a coincident stall to checkpoint I/O. Scheduled-thread
cycles are raw counters, not nanoseconds. Audio wait columns overlap guest time
and must not be added to the exclusive host stages.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics

STAGES = ("guest", "raster", "acquire", "compose", "present", "trace", "hook", "events", "wait")
AUDIO = ("audio_highwater_wait_ms", "audio_sleep_ms", "audio_mutex_wait_ms", "audio_queue_peak")


def read_timing(path: Path) -> list[dict]:
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        required = {"frame", "present_seconds", *(name + "_ms" for name in STAGES)}
        missing = required - set(reader.fieldnames or ())
        if missing:
            raise ValueError("Missing timing columns: " + ", ".join(sorted(missing)))
        rows = []
        for line, row in enumerate(reader, 2):
            value = {}
            for key in required | (set(row) & set(AUDIO)):
                try:
                    number = float(row[key])
                except (ValueError, TypeError):
                    raise ValueError(f"Invalid {key} on line {line}") from None
                if not math.isfinite(number) or number < 0:
                    raise ValueError(f"Invalid {key} on line {line}")
                value[key] = number
            if value["frame"] != int(value["frame"]):
                raise ValueError(f"Nonintegral frame on line {line}")
            value["frame"] = int(value["frame"])
            for name in STAGES:
                key = name + "_thread_cycles"
                if key in row:
                    try:
                        value[key] = int(row[key])
                    except (ValueError, TypeError):
                        raise ValueError(f"Invalid {key} on line {line}") from None
                    if value[key] < 0:
                        raise ValueError(f"Invalid {key} on line {line}")
            if "processor" in row:
                try:
                    value["processor"] = int(row["processor"])
                except (ValueError, TypeError):
                    raise ValueError(f"Invalid processor on line {line}") from None
                if value["processor"] < 0:
                    raise ValueError(f"Invalid processor on line {line}")
            if rows and (value["frame"] < rows[-1]["frame"] or
                         value["present_seconds"] <= rows[-1]["present_seconds"]):
                raise ValueError(f"Nonmonotonic presentation on line {line}")
            rows.append(value)
    return rows


def summarize(rows: list[dict], after_frame: int = 0, threshold_ms: float = 20,
              checkpoints: dict[int, str] | None = None, largest: int = 20) -> dict:
    selected = [row for row in rows if row["frame"] > after_frame]
    if len(selected) < 2:
        raise ValueError("At least two presentations are required after the warmup boundary")
    points = []
    checkpoints = checkpoints or {}
    for previous, current in zip(selected, selected[1:]):
        point = dict(current)
        point["interval_ms"] = (current["present_seconds"] - previous["present_seconds"]) * 1000
        point["profiled_ms"] = sum(current[name + "_ms"] for name in STAGES)
        point["unprofiled_residual_ms"] = point["interval_ms"] - point["profiled_ms"]
        point["declared_checkpoint"] = checkpoints.get(current["frame"])
        points.append(point)
    values = sorted(point["interval_ms"] for point in points)
    spikes = [point for point in points if point["interval_ms"] > threshold_ms]
    return {
        "scope": "Completed-present wall time; all intervals retained, including declared checkpoints. Coincidence is not causal proof. Audio subcolumns overlap exclusive guest timing; raw thread cycles are not time units.",
        "after_frame": after_frame,
        "selected_presentations": len(selected),
        "intervals": len(points),
        "first_interval_ending_frame": points[0]["frame"],
        "last_interval_ending_frame": points[-1]["frame"],
        "mean_ms": statistics.mean(values),
        "p99_ms": values[min(len(values) - 1, int(len(values) * .99))],
        "percentile_method": "Upper empirical order statistic at floor(0.99 * interval_count)",
        "max_ms": max(values),
        "spike_threshold_ms": threshold_ms,
        "spikes": len(spikes),
        "spikes_at_declared_checkpoints": sum(point["declared_checkpoint"] is not None for point in spikes),
        "declared_checkpoints": {str(key): value for key, value in sorted(checkpoints.items())},
        "audio_highwater_wait_ms": sum(point.get("audio_highwater_wait_ms", 0) for point in points)
            if any("audio_highwater_wait_ms" in point for point in points) else None,
        "unprofiled_residual_max_ms": max(point["unprofiled_residual_ms"] for point in points),
        "unprofiled_residual_median_ms": statistics.median(point["unprofiled_residual_ms"] for point in points),
        "largest_intervals": sorted(points, key=lambda point: point["interval_ms"], reverse=True)[:largest],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timing", type=Path, required=True)
    parser.add_argument("--after-frame", type=int, default=0)
    parser.add_argument("--threshold-ms", type=float, default=20)
    parser.add_argument("--checkpoint", action="append", default=[], metavar="FRAME:TAG")
    parser.add_argument("--route", type=Path, help="Optional route identity only; no script timing is guessed")
    parser.add_argument("--largest", type=int, default=20)
    parser.add_argument("--out", type=Path, required=True, help="Fresh JSON result path")
    args = parser.parse_args()
    if args.out.exists():
        parser.error("Output must be fresh; existing evidence was preserved")
    if args.after_frame < 0 or args.largest < 1 or not math.isfinite(args.threshold_ms) or args.threshold_ms <= 0:
        parser.error("Warmup must be nonnegative, count positive and threshold finite/positive")
    checkpoints = {}
    for item in args.checkpoint:
        frame, separator, tag = item.partition(":")
        if not separator or not frame.isdecimal() or int(frame) <= 0 or not tag or int(frame) in checkpoints:
            parser.error("Each checkpoint requires a unique positive FRAME:TAG")
        checkpoints[int(frame)] = tag
    try:
        result = summarize(read_timing(args.timing), args.after_frame, args.threshold_ms, checkpoints, args.largest)
        result["timing_sha256"] = hashlib.sha256(args.timing.read_bytes()).hexdigest()
        if args.route:
            result["route_sha256"] = hashlib.sha256(args.route.read_bytes()).hexdigest()
        with args.out.open("x", encoding="utf-8") as stream:
            json.dump(result, stream, indent=2)
            stream.write("\n")
    except (OSError, ValueError) as error:
        parser.exit(2, str(error) + "\n")
    print(json.dumps({key: result[key] for key in ("intervals", "mean_ms", "max_ms", "spikes", "spikes_at_declared_checkpoints")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
