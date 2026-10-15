# Performance and frame pacing

The current build sustains about 60 simulated/presented frames per second in the bounded Atland checks below. Occasional frame-time spikes remain. These checks cover this machine and these routes; they are not a hard real-time or full-game performance guarantee.

![Actual 32:9 fullscreen OpenGL output with Enhanced pixels](screenshots/atland-32x9-epx-performance-1920x1080.png)

This is an actual 1920x1080 OpenGL fullscreen readback, with 32:9 scenery letterboxed on a 16:9 monitor and optional EPX enabled. The PNG was converted losslessly from the readback without resizing. Original artwork is retained.

## Measurements

Measured on Windows 11 Home build 26200, Intel Core Ultra 7 265F (20 cores/20 logical processors), NVIDIA GeForce RTX 5070, driver 32.0.16.1664, and a 75 Hz class 1920x1080 display (integer WMI reports 74 Hz). Private settings and a fresh version-3 state were used. The isolated harness uses a dummy audio device, so real audio-device pacing remains outside this measurement.

| Check | Measured interval | Rate | Frame-time p50 / p95 / p99 | Intervals >20 / >25 ms |
|---|---|---:|---|---:|
| Controlled prior build, long 21:9 SDL route | 4,673 intervals after field 1750 | 59.86 fps | 16.70 / 18.26 / 19.61 ms | 37 / 14 |
| Current visible 32:9 OpenGL + EPX | 2,892 intervals after field 150 | 59.79 fps | 16.67 / 18.56 / 23.78 ms | 109 / 13 |
| Current clean hidden 1920x1080 GPU, 32:9 OpenGL + EPX | 1,640 intervals, fields 150..1790 | 60.12 fps | 16.63 / 17.71 / 21.36 ms | 29 / 1 |

An interval above a threshold is not itself proof of a dropped display frame. Percentiles use the sorted lower sample. The native guest rate is approximately 60.1 fields per second; small differences from this rate arise from bounded measurement windows and timing phase.

The **visible** run included startup/state loading and completed 3,042 fields in 51.015 seconds: **59.63 fps overall**, using 11.359 CPU seconds. Two WMI hardware inventory queries ran during it and may have contributed to scheduling spikes; the single screenshot/readback and endpoint dumps also add overhead. The screenshot is useful visual evidence, but this run is not a clean proof of uninterrupted 60 fps.

A separate **hidden** GPU run avoided inventory queries, GPU readbacks and repeated dumps. Its final dump interval was excluded from the table. The moving portion measured 60.11 fps; the settled profile used about 1.45 ms guest simulation, 0.80 ms raster/capture, 1.12 ms composition and 0.22 ms upload/presentation per field, with about 13.01 ms in deadline waiting. It used 6.172 CPU seconds across the full 29.959-second, 1,792-field process interval. Guest timing stayed within 1.000000..1.000176 field periods per simulation. Hidden-window timing does not certify visible fullscreen scheduling.

The earlier fullscreen dialogue capture averaged 46.22 fps. Repeating its same route/configuration without overlapping heavy work produced 59.88 fps overall, with a 59.86 fps profiled interval. The earlier slow result was not reproduced under those controlled conditions; its precise cause was not established.

## What changed

The SDL3 deadline wait now uses its precise delay facility for the short tail instead of repeatedly yielding with `SDL_Delay(0)`. The SDL2 fallback remains available. This targets wait jitter while preserving guest clocks, simulation frequency and real multi-field loader work. It is not a new threaded renderer, GPU guest CPU, DirectX 12 backend or frame-skipping workaround. The measurements involve multiple runtime/route changes and do not isolate an exact speedup from this wait change alone.

A single same-route hidden A/B trial used precise waiting for the whole remaining deadline. It measured 60.08 fps, but p95/p99 were 19.11/23.58 ms versus 17.71/21.36 ms with the short-tail policy. CPU time rose from 6.172 to 7.047 seconds over the approximately 30-second run. The whole-deadline trial was rejected and reverted. It is kept as private experiment evidence, while the measured short-tail build remains selected. One A/B pair cannot eliminate OS scheduling variance.

The flashing defect was a separate state-restoration problem: the PPU's two static raster journals were missing from saved state. Cold loading could replay a cold-boot register baseline every other field even while guest memory, CPU and PPU writes were correct. The version-3 continuation saves both completed and pending journals, including writes after the V=225 boundary. Changing graphics backends or shaders would not restore that missing state.

After the fix, cold loading matched 64 consecutive warm-run snapshots and 62 uninterrupted snapshots across native framebuffer, WRAM, registers, VRAM, OAM, CGRAM and clock information. Hidden SDL and OpenGL/EPX readbacks each checked 141 consecutive frames: only the startup frame was black; all 140 frames following the load were nonblack. The single approved final fullscreen run detected no repeated black-frame pattern, and its actual GPU capture shows the expected world. States from the earlier incomplete format are intentionally rejected.

## Evidence scope

The private batch directory is `work-native-20261015-154136`, alongside this checkout. Receipts bind executable, route, state and configuration hashes:

- `performance-baseline-report.json`: prior-build controlled timing.
- `raster-flashing-verification.json`: cold/warm/uninterrupted state and hidden GPU verification.
- `performance-final-verification.json`: actual fullscreen capture and timing limitations.
- `performance-clean-verification.json`: separate clean hidden-GPU timing.
- `performance-precise-verification.json`: same-route whole-deadline wait trial.

The current measured executable SHA-256 is `03fcc1f6eb0fef35c9a3427bf36d1c6aa36adf8faed9103f465d0f670f35b2c2`. No owner save, settings or mod-state files were changed. Other rooms, later encounters, real audio devices, different drivers and other hardware need their own measurements.
