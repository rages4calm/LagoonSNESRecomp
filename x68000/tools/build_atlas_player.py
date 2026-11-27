"""Build the Lagoon map into a kept RetroRecomp 1.8.0 project."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_retrorecomp import PROJECT, digest, replace_file
from lagoon_atlas import generate
from check_wide_areas import check_late_routes, check_wide_areas, check_zoom_routes


def write_patch(path, replacements):
    text = path.read_text(encoding="utf-8")
    for before, after in replacements:
        if text.count(before) != 1:
            raise ValueError(f"Unexpected pinned SDK source at {path.name}: {before[:54]}")
        text = text.replace(before, after)
    path.write_text(text, encoding="utf-8")


def write_award_catalog(path):
    source = PROJECT / "x68000/achievements.json"
    items = json.loads(source.read_text(encoding="utf-8"))["achievements"]
    rules = {"atlas.areas": 0, "atlas.routes": 1, "player.level": 2}
    if len(items) != 8 or len({item["id"] for item in items}) != 8:
        raise ValueError("The in-game achievement page requires eight unique entries.")
    lines = []
    for item in items:
        fields = [json.dumps(item[key]) for key in ("id", "title", "description")]
        fields += [str(rules[item["event"]]), str(item["target"])]
        lines.append("    {" + ", ".join(fields) + "},")
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def configure_atlas(cmake, cache, destination, log):
    command = [cmake, "-S", cache["CMAKE_HOME_DIRECTORY"], "-B", str(destination),
               "-G", cache["CMAKE_GENERATOR"]]
    for key, flag in (("CMAKE_GENERATOR_PLATFORM", "-A"), ("CMAKE_GENERATOR_TOOLSET", "-T")):
        if cache.get(key):
            command.extend([flag, cache[key]])
    settings = {key: value for key, value in cache.items() if key.startswith("RR_") or key in {
        "CMAKE_PREFIX_PATH", "PX68K_DIR", "CMAKE_GENERATOR_INSTANCE", "CMAKE_TOOLCHAIN_FILE", "CMAKE_BUILD_TYPE",
        "CMAKE_RC_COMPILER", "CMAKE_EXE_LINKER_FLAGS",
    }}
    if not cache["CMAKE_GENERATOR"].startswith("Visual Studio"):
        settings.update({key: cache[key] for key in ("CMAKE_C_COMPILER", "CMAKE_CXX_COMPILER", "CMAKE_MAKE_PROGRAM") if key in cache})
    command.extend(f"-D{key}={value}" for key, value in settings.items() if value)
    subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)


def build(report, destination, disk_dir, system, keep_diagnostics=False, opening_only=False):
    receipt = json.loads(report.read_text(encoding="utf-8"))
    if (receipt["version"], receipt["target"], receipt["status"]) != ("1.8.0", "windows-x86_64", "supported"):
        raise ValueError("The atlas build requires a retained, verified RetroRecomp 1.8.0 Windows conversion.")
    game = Path(receipt["build_executable"]).resolve(strict=True)
    # MSVC adds reference/Release; portable Ninja uses reference directly.
    # Locate the actual cache rather than relying on either directory depth.
    build_dir = next((path for path in list(game.parents)[:4] if (path / "CMakeCache.txt").is_file()), None)
    if build_dir is None:
        raise ValueError("Retain the converter's verified CMake build before adding the atlas.")
    project = build_dir.parent.parent
    sdk = project.parent / "sdk" / "native"
    stock = project / "packed-game.exe"
    if (project.name != receipt["source_identity"][:16] or game.name != "RetroRecomp-X68000-Native.exe" or
            receipt["export_receipt"]["sha256"] != digest(stock)):
        raise ValueError("Retain the original verified build before adding the atlas.")
    cache = {}
    for line in (build_dir / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        if ":" in line and "=" in line and not line.startswith(("#", "//")):
            key, _, setting = line.partition(":")
            cache[key] = setting.partition("=")[2]
    cmake = cache["CMAKE_COMMAND"]
    adapter = sdk / "hostkit/adapters/rrhk_x68k.inc"
    host = sdk / "x68000_reference/reference_host.c"
    options = sdk / "hostkit/shell/rrhk_options.c"
    additions = [adapter.parent / name for name in
                 ("lagoon_atlas.inc", "lagoon_exploration.inc", "lagoon_atlas_data.inc", "lagoon_ui_font.generated.h",
                  "lagoon_journal.inc", "lagoon_achievements_data.inc", "lagoon_controls.inc",
                  "lagoon_boot.inc", "lagoon_world_layers.inc", "lagoon_wide.inc", "lagoon_door.inc",
                  "lagoon_extended_actors.inc", "lagoon_scene_view.inc", "lagoon_view_state.inc")]
    original = {file: file.read_bytes() for file in (adapter, host, options)}
    if any(file.exists() for file in additions):
        raise ValueError("The private SDK already contains atlas build files.")
    destination.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copyfile(PROJECT / "x68000/runtime/lagoon_atlas.inc", additions[0])
        shutil.copyfile(PROJECT / "x68000/runtime/lagoon_exploration.inc", additions[1])
        areas = generate(disk_dir, system, additions[2])
        shutil.copyfile(PROJECT / "src/lagoon_ui_font.generated.h", additions[3])
        shutil.copyfile(PROJECT / "x68000/runtime/lagoon_journal.inc", additions[4])
        write_award_catalog(additions[5])
        for source in additions[6:]:
            shutil.copyfile(PROJECT / "x68000/runtime" / source.name, source)
        write_patch(adapter, (
            ("X_ENTER, X_PAUSE, X_ACTIONS", "X_ENTER, X_PAUSE, X_ITEMS, X_EQUIPMENT, X_SAVE_LOAD, X_ATLAS, X_ACTIONS"),
            ('{"Button A", L"Button A",', '{"Hold sword", L"Button A",'),
            ('{"Button B", L"Button B",', '{"Jump", L"Button B",'),
            ("{X_NO_PAD, X_NO_PAD}}};",
             "{X_NO_PAD, X_NO_PAD}},\n"
             '   {"Items", L"Items", 1u << X_ITEMS, RRHK_SYSTEM, 0, {0, 0}, {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, X_NO_PAD}},\n'
             '   {"Equipment", L"Equipment", 1u << X_EQUIPMENT, RRHK_SYSTEM, 0, {0, 0}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, X_NO_PAD}},\n'
             '   {"Save or load", L"Save or load", 1u << X_SAVE_LOAD, RRHK_SYSTEM, 0, {0, 0}, {SDL_CONTROLLER_BUTTON_Y, X_NO_PAD}},\n'
             '   {"Atlas", L"Atlas", 0, RRHK_SYSTEM, 0, {0, 0}, {SDL_CONTROLLER_BUTTON_RIGHTSTICK, X_NO_PAD}}};'),
            ('#include "hostkit/adapters/rrhk_computer.inc"',
             '#include "hostkit/adapters/rrhk_computer.inc"\n#include "lagoon_atlas.inc"\n#include "lagoon_controls.inc"'
             '\n#include "lagoon_boot.inc"\n#include "lagoon_world_layers.inc"'
             '\n#include "lagoon_door.inc"\n#include "lagoon_scene_view.inc"'
             '\n#include "lagoon_extended_actors.inc"\n#include "lagoon_wide.inc"\n#include "lagoon_view_state.inc"'),
            ("static bool x_run_frame(uint16_t p1, uint16_t p2) {",
             "static bool x_run_frame(uint16_t p1, uint16_t p2) {\n"
             "   lagoon_prepare_menu_input(p1);\n"
             "   if (lagoon_chart.open) return true;\n"
             "   if (lagoon_chart.consume_pad_release) {\n"
             "      if (!(p1 | p2)) lagoon_chart.consume_pad_release = false;\n"
             "      p1 = p2 = 0;\n"
             "   }"),
            ("   fast_forward = x_fast_forward();",
             "   lagoon_boot_prepare(p1);\n   fast_forward = lagoon_game_fast_forward();"),
            ("   if (!host_frame() || quit_requested || core_error) return false;",
             "   lagoon_capture_world_epoch();\n   if (!host_frame() || quit_requested || core_error) return false;"),
            ("   return !frame_limit || frame_number < frame_limit;",
             "   if (lagoon_menu_input.remaining) --lagoon_menu_input.remaining;\n"
             "   lagoon_note_position();\n   return !frame_limit || frame_number < frame_limit;"),
            ("static void x_reset(void) {", "static void x_reset(void) {\n"
             "   lagoon_release_menu_input();\n   lagoon_boot_reset();\n   lagoon_invalidate_view();\n"
             "   if (lagoon_chart.open) lagoon_close_atlas();\n"
             "   lagoon_save_history();\n   memset(&lagoon_chart, 0, sizeof lagoon_chart);"),
            ("{ (void)path; return snapshot(load) == RR_STATE_OK; }",
             "{ (void)path; if (load) { lagoon_release_menu_input();"
             " lagoon_chart.area = lagoon_chart.previous = lagoon_chart.pending = 0; }"
             " bool ok = snapshot(load) == RR_STATE_OK; if (load && ok) lagoon_boot_stop(); return ok; }"),
            ("   status = rrhk_main(machine, title, 1);",
             "   lagoon_boot_reset();\n   core.set_input_state(lagoon_boot_input_state);\n"
             "   lagoon_open_history();\n   lagoon_open_awards();\n   status = rrhk_main(machine, title, 1);"
             "\n   lagoon_save_history();\n   lagoon_save_awards();"),
            ("   xa.audio_taken += n;",
             "   lagoon_mix_chime(pcm, n);\n   xa.audio_taken += n;"),
            (".event = x_event,", ".event = lagoon_control_event, .before_present = lagoon_draw_overlays,"),
            (".frame_start = x_frame_start,", ".frame_start = lagoon_frame_start,"),
            (".pixels = x_pixels, .visible_width = x_width, .visible_height = x_height,",
             ".pixels = lagoon_picture, .visible_width = lagoon_picture_width, .visible_height = lagoon_picture_height,"),
            (".fast_forward = x_fast_forward, .game_rect = x_game_rect,",
             ".fast_forward = lagoon_game_fast_forward, .game_rect = lagoon_picture_rect,"),
        ))
        # Extend the SDK's transactional auxiliary payload, rather than a
        # sidecar file. Its checksum/identity/compatibility gates and rollback
        # cover the presentation cache together with the guest and disks.
        write_patch(host, (
            ("typedef struct {\n   uint32_t version,header_size,width,height,pixel_format,autofire;",
             "static size_t lagoon_view_state_size(void);\n"
             "static bool lagoon_capture_view_state(void *,size_t);\n"
             "static bool lagoon_validate_view_state(const void *,size_t);\n"
             "static bool lagoon_apply_view_state(const void *,size_t);\n"
             "typedef struct {\n   uint32_t version,header_size,width,height,pixel_format,autofire;"),
            ("size!=sizeof(header)+bytes)", "size!=sizeof(header)+bytes+lagoon_view_state_size())"),
            ("header.version=1;header.header_size=sizeof(header);",
             "header.version=2;header.header_size=sizeof(header);"),
            ("memcpy(data,&header,sizeof(header));memcpy((uint8_t*)data+sizeof(header),disks,bytes);free(disks);return true;",
             "memcpy(data,&header,sizeof(header));memcpy((uint8_t*)data+sizeof(header),disks,bytes);free(disks);"
             "return lagoon_capture_view_state((uint8_t*)data+sizeof(header)+bytes,lagoon_view_state_size());"),
            ("header.version!=1||header.header_size!=sizeof(header)||header.media_size!=size-sizeof(header)||",
             "header.version!=2||header.header_size!=sizeof(header)||header.media_size>size-sizeof(header)||"
             "size-sizeof(header)-(size_t)header.media_size!=lagoon_view_state_size()||"),
            ("return rr_media_validate(&media,(const uint8_t*)data+sizeof(header),(size_t)header.media_size)!=0;",
             "return rr_media_validate(&media,(const uint8_t*)data+sizeof(header),(size_t)header.media_size)!=0 && "
             "lagoon_validate_view_state((const uint8_t*)data+sizeof(header)+(size_t)header.media_size,lagoon_view_state_size());"),
            ("width=header.width;height=header.height;rrhk_ui()->autofire=header.autofire!=0;\n   return true;",
             "width=header.width;height=header.height;rrhk_ui()->autofire=header.autofire!=0;\n"
             "   return lagoon_apply_view_state((const uint8_t*)data+sizeof(header)+(size_t)header.media_size,lagoon_view_state_size());"),
            ("io.core_size=core.serialize_size();io.auxiliary_size=sizeof(HostAuxiliary)+bytes;",
             "io.core_size=core.serialize_size();io.auxiliary_size=sizeof(HostAuxiliary)+bytes+lagoon_view_state_size();"),
        ))
        write_patch(options, (
            ("bool rrhk_sound_toggle_mute(void)", "int lagoon_sound_volume(void) { return rr_sound_volume(); }\n"
             "void lagoon_set_sound_volume(int volume) { rr_sound_set_volume(volume); }\n"
             "int lagoon_load_zoom(void) { return SDL_clamp(rr_ini_int(rr_options.section, L\"view_zoom\", 0, rr_options.file), 0, 2); }\n"
             "void lagoon_save_zoom(int zoom) { rr_options_store(L\"view_zoom\", zoom); }\n"
             "bool rrhk_sound_toggle_mute(void)"),
        ))
        write_patch(host, ((
            'if(changed){auto_disk_changes+=(unsigned)changed;notify("Next disk inserted automatically");}',
            'if(changed) auto_disk_changes+=(unsigned)changed;',
        ),))
        normal_host = host.read_bytes()
        command = [cmake, "--build", str(build_dir), "--config", "Release",
                   "--target", "x68000_reference_host", "--parallel", "3"]
        built = destination / "Lagoon - Atlas.exe"
        if opening_only:
            # The converter must never see object files compiled with Atlas headers.
            with tempfile.TemporaryDirectory(prefix="a-", dir=project) as atlas_build:
                atlas_dir = Path(atlas_build)
                with (destination / "atlas-build.log").open("wb") as log:
                    configure_atlas(cmake, cache, atlas_dir, log)
                    subprocess.run([cmake, "--build", str(atlas_dir), *command[3:]],
                                   check=True, stdout=log, stderr=subprocess.STDOUT)
                replace_file(atlas_dir / game.relative_to(build_dir), built)
            with tempfile.TemporaryDirectory(prefix="lagoon-atlas-opening-") as checks:
                proof = qualify(built, stock, None, Path(checks))
            print(f"Opening matched after {proof['frames']} frames. SHA-256: {digest(built)}")
            return built, receipt["source_identity"], areas, proof

        write_patch(host, (
            ('#include "hostkit/adapters/rrhk_x68k.inc"',
             '#define LAGOON_ATLAS_PREVIEW 1\n#include "hostkit/adapters/rrhk_x68k.inc"'),
            ("   char core_path[PATH_CAP]", "   const char *atlas_preview = NULL, *lagoon_wide_path = NULL;\n"
             "   bool atlas_tour=false, lagoon_controls_check=false, lagoon_boot_check=false, lagoon_door_check=false, lagoon_crossing_check=false, lagoon_motion_check=false;\n"
             "   bool lagoon_extended_check=false, lagoon_scene_check=false;\n"
             "   bool lagoon_guest_keys=false;\n   unsigned lagoon_door_frames=0;\n   FILE *lagoon_trace=NULL;\n   char core_path[PATH_CAP]"),
            ('      else if(!strcmp(argv[i],"--auto-disk"))',
             '      else if(!strcmp(argv[i],"--lagoon-controls-check")) lagoon_controls_check=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-guest-keys")) lagoon_guest_keys=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-boot-check")) lagoon_boot_check=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-door-check")) lagoon_door_check=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-crossing-check")) lagoon_crossing_check=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-extended-check")) lagoon_extended_check=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-scene-check")) lagoon_scene_check=true;\n'
             '      else if(!strcmp(argv[i],"--lagoon-motion-check")) lagoon_motion_check=true;\n'
             '      else if(!strcmp(argv[i],"--auto-disk"))'),
            ('         else if(!strcmp(arg,"--input-events"))',
             '         else if(!strcmp(arg,"--lagoon-frame-trace")) { lagoon_trace=fopen(argv[i],"wb");'
             ' if(!lagoon_trace) { fail("Could not open view trace"); goto cleanup; } }\n'
             '         else if(!strcmp(arg,"--lagoon-zoom")) {'
             ' if(strlen(argv[i]) != 1 || argv[i][0] < \'0\' || argv[i][0] > \'2\') { fail("Invalid Lagoon zoom"); goto cleanup; }'
             ' lagoon_zoom_mode=argv[i][0]-\'0\'; }\n'
             '         else if(!strcmp(arg,"--lagoon-wide-picture")) lagoon_wide_path=argv[i];\n'
             '         else if(!strcmp(arg,"--atlas-preview")||!strcmp(arg,"--atlas-tour")) {'
             ' atlas_preview=argv[i]; atlas_tour=!strcmp(arg,"--atlas-tour"); }\n         else if(!strcmp(arg,"--input-events"))'),
            ("   if(headless){\n      while(!quit_requested",
             "   if(headless){\n      if(lagoon_guest_keys) guest_keyboard=true;\n      if(lagoon_boot_check) {\n"
             '         if(!lagoon_check_boot()) { fail("Controller boot qualification failed"); goto cleanup; }\n'
             "         frame_limit=frame_number;\n      }\n"
             "      if(atlas_preview) { lagoon_open_history(); lagoon_open_awards(); }\n"
             "      if(atlas_tour) { rrhk_bind(rrhk_adapter()); rrhk_options_init(L\"X68000\"); }\n"
             "      while(!quit_requested"),
            ("         if(!host_frame())break;",
             "         if(lagoon_wide_path || lagoon_motion_check || lagoon_scene_check || lagoon_trace) lagoon_capture_world_epoch();\n"
             "         if(!host_frame())break;\n         if(lagoon_trace) lagoon_trace_view(lagoon_trace);\n         if(atlas_preview) lagoon_note_position();\n"
             "         if(lagoon_door_check && lagoon_door_waiting()) lagoon_door_frames++;\n"
             "         if(lagoon_motion_check) lagoon_track_wide_motion();\n"
             "         if(lagoon_scene_check) lagoon_track_wide_scene();"),
            ("cleanup:", "cleanup:\n   if(lagoon_trace) fclose(lagoon_trace);"),
            ('   if(capture_path[0] && !capture(capture_path))',
             '   if(lagoon_door_check) fprintf(stderr,"Normal door pacing gated %u frames\\n",lagoon_door_frames);\n'
             '   if(lagoon_motion_check) fprintf(stderr,"Wide motion qualified %u/%u lit frames (%u moving)\\n",'
             ' lagoon_motion_qualified,lagoon_motion_lit,lagoon_motion_moved);\n'
             '   if(lagoon_motion_check && lagoon_motion_earlier_checked) fprintf(stderr,"Earlier epoch continuity verified\\n");\n'
             '   if(lagoon_scene_check) fprintf(stderr,"Wide scene carried %u doors %u dialogue, center differences %u, reveals %u\\n",'
             ' lagoon_scene_held_doors,lagoon_scene_held_dialogue,lagoon_scene_center_mismatches,lagoon_scene_reveal_frames);\n'
             '   if(lagoon_wide_path && (!headless || !lagoon_capture_wide(lagoon_wide_path, lagoon_crossing_check, lagoon_extended_check))) {'
             ' fail("Widescreen qualification failed"); goto cleanup; }\n'
             '   if(lagoon_controls_check && (!headless || !lagoon_check_controls())) {'
             ' fail("Controller qualification failed"); goto cleanup; }\n'
             '   if(atlas_preview && (!headless || !lagoon_chart_preview(atlas_preview,atlas_tour))) {'
             ' fail("Atlas preview could not verify this route"); goto cleanup; }\n   if(capture_path[0] && !capture(capture_path))'),
        ))
        with tempfile.TemporaryDirectory(prefix="lagoon-atlas-preview-") as scratch:
            preview = Path(scratch) / "Lagoon - Atlas Preview.exe"
            with (destination / "atlas-build.log").open("wb") as log:
                subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
                replace_file(game, preview)
                # The SDK includes reference_host.c in its snapshot ABI stamp.
                # The delivered player only changes the presentation adapter.
                host.write_bytes(normal_host)
                subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
            replace_file(game, built)
            with tempfile.TemporaryDirectory(prefix="lagoon-atlas-check-") as checks:
                if keep_diagnostics:
                    replace_file(preview, destination / preview.name)
                proof = qualify(built, stock, preview, Path(checks))
                replace_file(Path(checks) / "late/clouds/clouds.bmp", destination / "clouds-zoom.bmp")
                replace_file(Path(checks) / "zoom/meadow/meadow-zoom.bmp", destination / "meadow-zoom.bmp")
                replace_file(Path(checks) / "areas/area-40/castle-wide.bmp", destination / "castle-wide.bmp")
                replace_file(Path(checks) / "preview/atlas.bmp", destination / "atlas-preview.bmp")
                replace_file(Path(checks) / "preview/atland-wide.bmp", destination / "atland-wide.bmp")
                replace_file(Path(checks) / "tour/church-wide.bmp", destination / "church-wide.bmp")
                replace_file(Path(checks) / "crossing/crossing-wide.bmp", destination / "crossing-wide.bmp")
                replace_file(Path(checks) / "extended/extended-wide.bmp", destination / "extended-wide.bmp")
                for scene in ("dialogue", "mayor", "shop"):
                    replace_file(Path(checks) / scene / f"{scene}-wide.bmp", destination / f"{scene}-wide.bmp")
                for name in ("routes.bmp", "items.bmp", "journal.bmp", "awards.bmp",
                             "church.bmp", "samson-atlas-fixture.bmp", "controls.bmp", "settings.bmp",
                        "settings-narrow.bmp", "settings-ultrawide.bmp", "quit.bmp"):
                    replace_file(Path(checks) / "tour" / name, destination / name)
        print(f"Built {areas} terrain maps in {built}")
        print(f"SHA-256 {digest(built)}")
        return built, receipt["source_identity"], areas, proof
    finally:
        for file, content in original.items():
            file.write_bytes(content)
        for file in additions:
            file.unlink(missing_ok=True)


def qualify(executable, stock, preview, output):
    def replay(label, player, snapshot=None):
        directory = output / ("tour" if label == "restart-tour" else label)
        directory.mkdir(exist_ok=label == "restart-tour")
        isolated = directory / player.name
        if label != "restart-tour":
            shutil.copyfile(player, isolated)
        route = "10975 11002 joy 6 1\n"
        frames = 11100
        if label == "tour":
            # Walk into Atland and enter the Area 03 doorway from the street.
            route = "11101 11651 joy 4 1\n11681 11709 joy 6 1\n11719 12179 joy 4 1\n"
            frames = 12208
        if label == "crossing":
            route += "11100 11103 joy 6 1\n"
            frames = 11106
        if label == "extended":
            route += "11100 11120 joy 6 1\n"
            frames = 11135
        if label == "dialogue":
            route = ("11101 11651 joy 4 1\n11681 11709 joy 6 1\n11719 11749 joy 4 1\n"
                     "12190 12342 joy 4 1\n12360 12371 joy 8 1\n")
            frames = 12450
        if label == "mayor":
            route = ("11101 11157 joy 4 1\n11164 11292 joy 6 1\n11300 11345 joy 4 1\n")
            frames = 11460
        if label == "shop":
            route = ("11101 11651 joy 4 1\n11660 11737 joy 4 1\n11745 11855 joy 6 1\n"
                     "11861 11891 joy 4 1\n12020 12069 joy 4 1\n12074 12095 joy 6 1\n12101 12128 joy 4 1\n")
            frames = 12259
        if label == "motion":
            # Walk the street in both directions without entering a doorway.
            route = ("11101 11651 joy 4 1\n11700 12190 joy 5 1\n"
                     "12250 12400 joy 6 1\n12440 12600 joy 7 1\n")
            frames = 12710
        if label == "doors":
            route = ("11101 11651 joy 4 1\n11681 11709 joy 6 1\n11719 11749 joy 4 1\n"
                     "12211 12236 joy 6 1\n12254 12279 joy 7 1\n12308 12351 joy 5 1\n")
            frames = 12710
        (directory / "input.tsv").write_text(route, encoding="ascii")
        report_name = f"{label}.json"
        args = ["--headless", "--mute", "--frames", str(frames), "--input-events", "input.tsv", "--report", report_name]
        if label in ("preview", "restart-tour"):
            args += ["--atlas-preview", "atlas.bmp"]
            if label == "preview":
                args += ["--lagoon-wide-picture", "atland-wide.bmp"]
        elif label == "controls":
            args += ["--lagoon-controls-check"]
        elif label == "boot":
            args += ["--lagoon-boot-check"]
        elif label == "motion":
            args += ["--lagoon-motion-check"]
        elif label == "doors":
            args += ["--lagoon-door-check", "--lagoon-scene-check"]
        elif label in ("dialogue", "mayor", "shop"):
            args += ["--lagoon-scene-check", "--lagoon-wide-picture", f"{label}-wide.bmp"]
        elif label == "extended":
            args += ["--lagoon-wide-picture", "extended-wide.bmp", "--lagoon-extended-check"]
        elif label == "crossing":
            args += ["--lagoon-wide-picture", "crossing-wide.bmp", "--lagoon-crossing-check"]
        elif label == "tour":
            args += ["--atlas-tour", "tour.bmp", "--lagoon-wide-picture", "church-wide.bmp"]
        elif snapshot:
            args = ["--headless", "--mute", "--frames", "120", "--report", report_name,
                    "--state-slot", str(snapshot), "--state-load-start"]
        elif preview is not None:
            args += ["--state-slot", "quick.rrstate", "--state-save-at", "11080"]
        result = subprocess.run([str(isolated), *args], cwd=directory, capture_output=True, text=True,
                                creationflags=subprocess.CREATE_NO_WINDOW,
                                timeout=120 if preview is None else None,
                                env=dict(os.environ, SDL_VIDEODRIVER="dummy") if label in ("boot", "controls") else None)
        if result.returncode:
            raise ValueError(f"{label} exited {result.returncode}: {result.stderr.strip()}")
        proof = {}
        if label == "boot":
            timing = next((line for line in result.stderr.splitlines() if line.startswith("Controller boot ready at frame ")), "")
            if not timing:
                raise ValueError("The controller did not complete the accelerated opening.")
            proof["controller_boot_ready_frame"] = int(timing.split()[5].rstrip(","))
        if label == "doors":
            timing = next((line for line in result.stderr.splitlines() if line.startswith("Normal door pacing gated ")), "")
            if not timing or not (int(timing.split()[4]) == 0 or 140 <= int(timing.split()[4]) <= 180):
                raise ValueError(f"The two normal door transitions did not have the expected pacing gates: {timing}")
            proof["normal_door_pacing_frames"] = int(timing.split()[4])
        if label in ("doors", "dialogue", "mayor", "shop"):
            continuity = next((line for line in result.stderr.splitlines() if line.startswith("Wide scene carried ")), "")
            if not continuity:
                raise ValueError("Missing doorway or dialogue frame evidence.")
            words = continuity.split()
            doors, dialogue = int(words[3]), int(words[5])
            changed_center, reveals = int(words[9].rstrip(",")), int(words[11])
            if changed_center or (label == "doors" and (doors < 1 or reveals < 1)) or (label != "doors" and dialogue < 4):
                raise ValueError(f"Native scene continuity check failed on {label}: {continuity}")
            if label == "doors":
                proof["door_backdrop_frames"] = doors
                proof["door_reveal_frames"] = reveals
            else:
                proof["dialogue_backdrop_frames"] = dialogue
        if label == "motion":
            motion = next((line for line in result.stderr.splitlines() if line.startswith("Wide motion qualified ")), "")
            if not motion:
                raise ValueError("Missing motion evidence.")
            admitted, lit = map(int, motion.split()[3].split("/"))
            moved = int(motion.split()[6].lstrip("("))
            if lit < 1000 or moved < 300 or admitted * 100 < lit * 97:
                raise ValueError(f"Moving widescreen guard rejected too much gameplay: {motion}")
            if "Earlier epoch continuity verified" not in result.stderr:
                raise ValueError("A register change hid a still-valid earlier world view.")
            proof["wide_motion_earlier_checked"] = True
            proof["wide_motion_frames"] = [admitted, lit]
            proof["wide_motion_moved_frames"] = moved
        if label == "controls":
            if "Controller Items, Equipment, Save and Load verified" not in result.stderr:
                raise ValueError("The virtual controller did not complete the in-game save and load.")
            if "Controller Atlas tabs and room browsing verified" not in result.stderr:
                raise ValueError("The virtual controller did not navigate every Atlas tab and browse rooms.")
            if "Controller Settings, modal focus, and audio persistence verified" not in result.stderr:
                raise ValueError("The virtual controller did not finish Settings qualification.")
        if label in ("preview", "restart-tour") and ("Atlas area=1 x=552 y=1385" not in result.stderr
                                                   or not (directory / "atlas.bmp").is_file()):
            raise ValueError("The atlas did not render the scripted Atland position.")
        if label == "restart-tour" and "routes=1 awards=2 pending=0" not in result.stderr:
            raise ValueError("The cold player did not load its recorded route and achievements.")
        if label == "tour":
            captures = ("tour.bmp", "routes.bmp", "items.bmp", "journal.bmp", "awards.bmp",
                        "church.bmp", "samson-atlas-fixture.bmp", "controls.bmp", "settings.bmp",
                        "settings-narrow.bmp", "settings-ultrawide.bmp", "quit.bmp")
            missing = [name for name in captures if not (directory / name).is_file()]
            if missing:
                raise ValueError(f"The area crossing missed these captures: {', '.join(missing)}.")
        report = json.loads((directory / report_name).read_text(encoding="utf-8"))
        if (report["core_error"] or (preview is None and report["frames"] != frames) or
                report["media"] != {"selected": [4, 1], "ready_mask": 3} or
                (snapshot and (report["session"]["loaded"] != 1 or report["session"]["rejected"]))):
            raise ValueError("Opening or User Disk qualification failed.")
        return {**{key: report[key] for key in ("frames", "cpu", "ram_fnv64", "pcm_fnv64", "final_video_fnv64")}, **proof}

    with ThreadPoolExecutor(2) as runs:
        atlas = runs.submit(replay, "atlas", executable)
        control = runs.submit(replay, "control", stock)
        hashes = atlas.result()
        if hashes != control.result():
            raise ValueError("Atlas and unmodified player differ on the opening route.")
        if preview is None:
            return {**hashes, "matched_reference": True}
        old_state = output / "control/quick.rrstate"
        new_state = output / "atlas/quick.rrstate"
        resume = runs.submit(replay, "resume", executable, old_state)
        reverse = runs.submit(replay, "reverse", stock, new_state)
        if resume.result() != reverse.result():
            raise ValueError("The old and new players resumed different state.")
    with ThreadPoolExecutor(3) as runs:
        checks = {name: runs.submit(replay, name, preview) for name in ("boot", "doors", "dialogue", "extended", "motion", "preview", "tour", "controls", "crossing", "mayor", "shop")}
        failure = [str(future.exception()) for future in checks.values() if future.exception()]
        if failure:
            raise ValueError("\n".join(failure))
        if hashes != checks["preview"].result():
            raise ValueError("The atlas capture build differed from normal gameplay.")
        boot_ready = checks["boot"].result()["controller_boot_ready_frame"]
        door_frames = checks["doors"].result()["normal_door_pacing_frames"]
        wide_motion = checks["motion"].result()["wide_motion_frames"]
        moved_frames = checks["motion"].result()["wide_motion_moved_frames"]
        door_backdrop = checks["doors"].result()["door_backdrop_frames"]
        door_reveals = checks["doors"].result()["door_reveal_frames"]
        dialogue_backdrop = {scene: checks[scene].result()["dialogue_backdrop_frames"]
                             for scene in ("dialogue", "mayor", "shop")}

    controls_game = next((output / "controls/datas/games").glob("x68-*"))
    area_proof = check_wide_areas(preview, controls_game, output / "areas")
    zoom_proof = check_zoom_routes(preview, controls_game, output / "zoom")
    late_proof = check_late_routes(preview, controls_game, output / "late")
    history = next((output / "tour/datas/games").glob("x68-*/atlas-history.bin"))
    saved = history.read_bytes()
    if saved[:9] != b"LGAATLS1\x01" or saved[-6:-4] != bytes((1, 3)):
        raise ValueError("The observed Atland doorway was not saved in the atlas.")
    awards = history.with_name("achievements.ini")
    unlocked = awards.read_bytes()
    replay("restart-tour", preview)
    if history.read_bytes()[-6:] != saved[-6:]:
        raise ValueError("The cold player changed the saved crossing.")
    if awards.read_bytes() != unlocked:
        raise ValueError("The two earned achievements changed on a cold restart.")
    return {**hashes, "atlas_route": [1, 3], "cold_history_loaded": True, "item_guide_checked": True,
            "journal_level": 1, "unlocks": 2, "achievement_threshold_fixture": 8, "chime_checked": True,
            "controller_menus_and_disk_roundtrip": True, "controller_remaps_checked": True,
            "controller_settings_checked": True,
            "controller_atlas_tabs_checked": True,
            "map_fit_checked": True, "controller_first_boot_checked": True,
            "widescreen_checked": True, "wide_area_routes": area_proof, "zoom_routes": zoom_proof,
            "late_routes": late_proof, "atlas_doorways_checked": True,
            "normal_door_pacing_checked": True,
            "controller_boot_ready_frame": boot_ready, "normal_door_pacing_frames": door_frames,
            "wide_motion_frames": wide_motion, "wide_motion_moved_frames": moved_frames,
            "wide_motion_earlier_checked": True,
            "wide_crossing_sprite_checked": True, "wide_extended_npc_checked": True,
            "door_backdrop_frames": door_backdrop, "door_reveal_frames": door_reveals,
            "dialogue_backdrop_frames": dialogue_backdrop["dialogue"],
            "mayor_backdrop_frames": dialogue_backdrop["mayor"],
            "shop_backdrop_frames": dialogue_backdrop["shop"]}


def install(executable, source_identity, entries, proof, expedition):
    edition = "retrorecomp-expedition" if expedition else "retrorecomp-adventure"
    target = PROJECT / "local-x68000" / edition
    previous = json.loads((target / "lagoon-extra-install.json").read_text(encoding="utf-8"))
    if (previous.get("adventure") is not True or previous.get("expedition", False) != expedition
            or previous.get("source_identity") != source_identity
            or digest(target / "Lagoon - English.exe") != previous.get("sha256")):
        raise ValueError("The installed profile must match the atlas gameplay data.")
    atlas = target / executable.name
    if not atlas.exists() or digest(atlas) != digest(executable):
        replace_file(executable, atlas)
    receipt = {
        "format": 3 if proof.get("matched_reference") else 2,
        "source_identity": source_identity, "sha256": digest(executable),
        "terrain_areas": entries, "opening_route": proof,
        "runtime_revision": "0.6.0", "presentation_version": 2,
    }
    with tempfile.TemporaryDirectory(prefix="lagoon-atlas-receipt-") as directory:
        local = Path(directory) / "lagoon-atlas-install.json"
        local.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        replace_file(local, target / local.name)
    print(f"Installed qualified atlas player in {target}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--disks", type=Path, default=PROJECT / "Lagoon_Sharp-X68000_EN")
    parser.add_argument("--english-system", type=Path, default=PROJECT / "x68000/patched/System-EN.dim")
    parser.add_argument("--install", action="store_true", help="Install the qualified atlas in the matching local profile.")
    parser.add_argument("--keep-diagnostics", action="store_true", help="Retain the local preview player for scripted diagnostics.")
    parser.add_argument("--opening-only", action="store_true", help="Build the playable host and compare the opening to the validated base player.")
    parser.add_argument("--expedition", action="store_true", help="Install into Enhanced Expedition instead of Enhanced Adventure.")
    args = parser.parse_args()
    if args.opening_only and args.keep_diagnostics:
        parser.error("--opening-only builds only the playable host.")
    output = args.output.resolve()
    executable, source_identity, entries, proof = build(args.report.resolve(), output, args.disks, args.english_system,
                                                        args.keep_diagnostics, args.opening_only)
    if args.install:
        install(executable, source_identity, entries, proof, args.expedition)


if __name__ == "__main__":
    main()
