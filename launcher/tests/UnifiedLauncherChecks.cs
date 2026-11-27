using Companion.Launcher.Core;
using System.Text.Json;

internal static class UnifiedLauncherChecks
{
    internal static void Run(string root, string project)
    {
        if (Directory.Exists(root)) throw new IOException("Use a fresh unified launcher fixture directory.");
        Directory.CreateDirectory(root);
        var snes = new GameAdapter(Path.Combine(root, "snes-data"));
        var x68 = new X68000Adapter(root, Path.Combine(root, "x68-data"));
        var x68Profile = x68.Profiles.Single();
        if (x68Profile.Id != "enhanced-expedition" || x68Profile.EditionId != "x68000-enhanced-expedition" ||
            x68Profile.Title != "X68000 Enhanced" || snes.Definition.Id == x68.Definition.Id ||
            snes.Profiles[0].EditionId == x68Profile.EditionId)
            throw new Exception("The one X68000 edition must retain existing Expedition data and remain separate from SNES.");
        if (X68000Adapter.LegacySaveSources.Count != 4 || x68.Definition.Options.Any(option => option.Section != "Display"))
            throw new Exception("Earlier releases are save import sources. The launcher exposes display settings only.");
        Directory.CreateDirectory(x68.ProfileDirectory(x68Profile));
        var storedSettings = Path.Combine(x68.ProfileDirectory(x68Profile), "settings.json");
        var originalStore = new JsonSettingsStore(storedSettings);
        originalStore.Write(new Dictionary<string, System.Text.Json.JsonElement> {
            ["engine.path"] = OptionValue.From(Path.Combine(root, "other-edition.exe")),
        }, originalStore.Read().Revision);
        var savedBefore = File.ReadAllBytes(storedSettings);
        var x68Values = x68.ReadSettings(x68Profile).Values;
        if (x68Values.ContainsKey("engine.path")) throw new Exception("Old settings exposed an executable override.");
        var runtime = Path.Combine(root, "local-x68000", "retrorecomp-expedition");
        var executable = Path.Combine(runtime, "Lagoon - English.exe");
        var atlas = Path.Combine(runtime, "Lagoon - Atlas.exe");
        var baseReceipt = Path.Combine(runtime, "lagoon-extra-install.json");
        var atlasReceipt = Path.Combine(runtime, "lagoon-atlas-install.json");
        var legacyProfile = new ProfileDescriptor("enhanced-adventure", "Enhanced Adventure", "x68000-enhanced-adventure");

        void RefuseLaunch(string message, ProfileDescriptor? otherProfile = null)
        {
            var checkedProfile = otherProfile ?? x68Profile;
            if (x68.CheckReady(checkedProfile, x68Values).Ready) throw new Exception(message);
            try { x68.BuildLaunchPlan(checkedProfile, x68Values, false); }
            catch (InvalidOperationException) { return; }
            throw new Exception(message);
        }
        RefuseLaunch("Missing export admitted.");
        Directory.CreateDirectory(runtime);
        File.WriteAllText(executable, "fixture: base executable");
        File.WriteAllText(atlas, "fixture: qualified Atlas executable");
        RefuseLaunch("Unidentified base export admitted.");

        string Hash(string path) => Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(File.ReadAllBytes(path)));
        var identity = new string('b', 64);
        var receipt = new {
            format = 1, converter_version = "1.8.0", system = "x68", language = "en",
            executable = Path.GetFileName(executable), sha256 = Hash(executable),
            reference_validated = true, user_disk = true, exp_rate = 2, gold_rate = 2,
            enhanced = true, adventure = true, expedition = true, source_identity = identity
        };
        var atlasNote = new {
            format = 2, source_identity = identity, sha256 = Hash(atlas), terrain_areas = 49,
            runtime_revision = "0.6.0", presentation_version = 2,
            opening_route = new {
                map_fit_checked = true, controller_menus_and_disk_roundtrip = true, controller_remaps_checked = true,
                controller_first_boot_checked = true, widescreen_checked = true, wide_crossing_sprite_checked = true,
                normal_door_pacing_checked = true, wide_extended_npc_checked = true,
                door_backdrop_frames = 1, door_reveal_frames = 1, dialogue_backdrop_frames = 10
            }
        };

        void Receipt<T>(string path, T value) => File.WriteAllText(path, System.Text.Json.JsonSerializer.Serialize(value));
        Receipt(baseReceipt, receipt);
        RefuseLaunch("The bare enhanced export must never launch silently.");
        Receipt(atlasReceipt, atlasNote with { source_identity = new string('a', 64) });
        RefuseLaunch("Atlas for different media admitted.");
        Receipt(atlasReceipt, atlasNote with { opening_route = atlasNote.opening_route with { map_fit_checked = false } });
        RefuseLaunch("Older Atlas missing current gameplay features admitted.");
        Receipt(atlasReceipt, atlasNote with { opening_route = atlasNote.opening_route with { widescreen_checked = false } });
        RefuseLaunch("An older build without the wider world admitted.");
        Receipt(atlasReceipt, atlasNote with { opening_route = atlasNote.opening_route with { wide_extended_npc_checked = false } });
        RefuseLaunch("An older build without the NPC seam fix admitted.");
        var openingNote = new {
            format = 3, source_identity = identity, sha256 = Hash(atlas), terrain_areas = 49,
            runtime_revision = "0.6.0", presentation_version = 2,
            opening_route = new { frames = 11100, matched_reference = true }
        };
        Receipt(atlasReceipt, openingNote with { opening_route = openingNote.opening_route with { matched_reference = false } });
        RefuseLaunch("A preview without a matched opening was admitted.");
        Receipt(atlasReceipt, openingNote with { opening_route = openingNote.opening_route with { frames = 11099 } });
        RefuseLaunch("An incomplete preview opening was admitted.");
        Receipt(atlasReceipt, new { format = 3, source_identity = identity, sha256 = Hash(atlas), terrain_areas = 49,
            opening_route = new { frames = 11100, matched_reference = true } });
        RefuseLaunch("An old matching preview without runtime revision was admitted.");
        Receipt(atlasReceipt, openingNote with { runtime_revision = "0.5.3" });
        RefuseLaunch("An old runtime revision was admitted.");
        Receipt(atlasReceipt, openingNote with { presentation_version = 1 });
        RefuseLaunch("An old presentation format was admitted.");
        Receipt(atlasReceipt, openingNote);
        if (x68.BuildLaunchPlan(x68Profile, x68Values, false).Executable != atlas)
            throw new Exception("The locally verified preview did not select Atlas.");
        Receipt(atlasReceipt, openingNote with { runtime_revision = "0.6.1", presentation_version = 3 });
        if (x68.BuildLaunchPlan(x68Profile, x68Values, false).Executable != atlas)
            throw new Exception("The new runtime revision did not select Atlas.");
        Receipt(atlasReceipt, openingNote with { runtime_revision = "0.6.1", presentation_version = 2 });
        RefuseLaunch("An unmatched runtime/presentation pair was admitted.");
        Receipt(atlasReceipt, atlasNote);
        Receipt(baseReceipt, receipt with { exp_rate = 1 });
        RefuseLaunch("A different EXP preset admitted.");
        Receipt(baseReceipt, receipt with { adventure = false });
        RefuseLaunch("Base with incomplete gameplay improvements admitted.");
        Receipt(baseReceipt, receipt);
        RefuseLaunch("Legacy source was selectable for gameplay.", legacyProfile);
        var x68Plan = x68.BuildLaunchPlan(x68Profile, x68Values, false);
        if (x68Plan.Executable != atlas || x68Plan.WorkingDirectory != runtime || x68Plan.Arguments.Count != 0)
            throw new Exception("Enhanced did not open the verified game beside its existing Expedition saves.");
        var prefsPath = Path.Combine(runtime, "datas", "Retro-Recomp.ini");
        Directory.CreateDirectory(Path.GetDirectoryName(prefsPath)!);
        // The native Windows writer appends CRLF lines to the launcher's LF INI.
        const string mixedIni = "\n[X68000]\ndisplay_mode = 3\r\nautofire=0\r\nfilter=0\np0_key1=12\r\n" +
            "[X68000.Options]\naudio_volume=75\r\nfkeys_machine=1\n";
        File.WriteAllText(prefsPath, mixedIni);
        var prefs = x68.ReadSettings(x68Profile);
        if (prefs.Values["x68.display"].GetString() != "3" || File.ReadAllText(prefsPath) != mixedIni)
            throw new Exception("Reading X68000 settings failed to preserve the original mixed INI.");
        prefs.Values["x68.display"] = OptionValue.From("1");
        prefs.Values["x68.filter"] = OptionValue.From("2");
        x68.ApplySettings(x68Profile, prefs.Values, prefs.Revision);
        var applied = x68.ReadSettings(x68Profile);
        var appliedIni = File.ReadAllText(prefsPath);
        if (applied.Values["x68.display"].GetString() != "1" || applied.Values["x68.filter"].GetString() != "2" ||
            !appliedIni.Contains("p0_key1=12") || !appliedIni.Contains("audio_volume=75") ||
            !appliedIni.Contains("fkeys_machine=1") || !File.ReadAllBytes(storedSettings).SequenceEqual(savedBefore))
            throw new Exception("Display settings failed to round-trip or changed unrelated options.");
        var expectedIni = mixedIni.Replace("\r\n", "\n").Replace("display_mode = 3", "display_mode = 1")
            .Replace("filter=0", "filter = 2").Replace("\n", "\r\n");
        if (appliedIni != expectedIni)
            throw new Exception("Applying X68000 settings corrupted or failed to normalize the mixed INI.");
        try { x68.ApplySettings(x68Profile, applied.Values, prefs.Revision); throw new Exception("Stale X68000 settings were accepted."); }
        catch (SettingsConflictException) { }

        var data = Path.Combine(runtime, "datas", "games", "x68-" + identity);
        using (var gameLock = new FileStream(Path.Combine(data, "session.lock"), FileMode.Open, FileAccess.ReadWrite, FileShare.None))
        {
            applied.Values["x68.filter"] = OptionValue.From("0");
            try { x68.ApplySettings(x68Profile, applied.Values, applied.Revision); throw new Exception("An open X68000 session admitted edits."); }
            catch (InvalidOperationException error) when (error.Message == "Close Lagoon before applying X68000 settings.") { }
        }
        if (x68.ReadSettings(x68Profile).Values["x68.filter"].GetString() != "2")
            throw new Exception("A refused X68000 settings change modified the filter.");

        foreach (var malformed in new[] { "[X68000]\rdisplay_mode=1\n", "[X68000]\r\nfilter=2\0\n" })
        {
            File.WriteAllText(prefsPath, malformed);
            try { _ = x68.ReadSettings(x68Profile); throw new Exception("Malformed X68000 settings were accepted."); }
            catch (InvalidDataException) { }
            if (File.ReadAllText(prefsPath) != malformed)
                throw new Exception("Reading malformed X68000 settings changed the INI.");
        }
        File.WriteAllText(prefsPath, appliedIni);

        {
            var catalog = Path.Combine(root, "x68000");
            Directory.CreateDirectory(catalog);
            File.Copy(Path.Combine(project, "x68000", "achievements.json"), Path.Combine(catalog, "achievements.json"));
            File.WriteAllText(Path.Combine(data, "achievements.ini"), "[Unlocked]\ntravel.first-area=1795194005\n");
            var achievements = x68.ReadAchievements(x68Profile);
            if (achievements.Entries.Count != 8 || achievements.Entries.Count(entry => entry.UnlockedAt > 0) != 1 ||
                achievements.Entries.Single(entry => entry.Id == "travel.first-area").UnlockedAt != 1795194005)
                throw new Exception("Enhanced did not retain existing achievement progress.");
        }
        ExternalSaveImport(root, x68, executable, identity, data);
        File.AppendAllText(atlas, " modified");
        RefuseLaunch("Changed Atlas admitted.");
        Receipt(atlasReceipt, atlasNote with { sha256 = Hash(atlas) });
        File.AppendAllText(executable, " modified");
        RefuseLaunch("Changed base export admitted.");

        var originalRoot = Environment.GetEnvironmentVariable("COMPANION_DATA_ROOT");
        try
        {
            Environment.SetEnvironmentVariable("COMPANION_DATA_ROOT", Path.Combine(root, "edition-store"));
            if (GameChoice.Load(Array.Empty<string>()) != GameChoice.Ask) throw new Exception("Fresh setup must offer both editions.");
            GameChoice.Save(GameChoice.X68000);
            if (GameChoice.Load(Array.Empty<string>()) != GameChoice.X68000 ||
                GameChoice.Load(new[] { "--game", "snes" }) != GameChoice.Snes ||
                GameChoice.Load(new[] { "--game", "ask" }) != GameChoice.Ask ||
                GameChoice.Load(new[] { "--rom-setup" }) != GameChoice.Snes)
                throw new Exception("Edition selection or SNES ROM setup override failed.");
            try { GameChoice.Load(new[] { "--game", "unknown" }); throw new Exception("Unknown edition admitted."); }
            catch (ArgumentException) { }
        }
        finally { Environment.SetEnvironmentVariable("COMPANION_DATA_ROOT", originalRoot); }
        var bootstrapDir = Path.Combine(root, "x68000", "tools");
        Directory.CreateDirectory(bootstrapDir);
        File.WriteAllText(Path.Combine(bootstrapDir, "setup_bootstrap.ps1"), "# plan fixture; not executed");
        var input = Path.Combine(root, "Disk ZIP with spaces.zip"); File.WriteAllText(input, "fixture");
        var firmware = Path.Combine(root, "firmware with spaces.dat"); File.WriteAllText(firmware, "fixture");
        var setupPlan = X68000Setup.BuildPlan(root, input, firmware, firmware);
        if (!Path.IsPathFullyQualified(setupPlan.Executable) || setupPlan.WorkingDirectory != root ||
            !setupPlan.Arguments.Contains(input) || !setupPlan.Arguments.Contains(firmware) ||
            !setupPlan.Arguments.Contains("-Inputs") || !setupPlan.Arguments.Contains("-Ipl") ||
            !setupPlan.Arguments.Contains("-Cgrom")) throw new Exception("Portable X68000 bootstrap did not preserve structured path arguments.");
        Console.WriteLine("PASS: unified edition choice; guarded X68000 receipts, settings and achievements; bootstrap paths with spaces; SNES adapter retained.");
    }

    private static void ExternalSaveImport(string root, X68000Adapter current, string currentPlayer, string identity, string data)
    {
        string Hash(byte[] bytes) => Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(bytes));
        void WriteJson(string path, object value) => File.WriteAllText(path, JsonSerializer.Serialize(value));
        var previous = Path.Combine(root, "previous installation with spaces");
        var oldRuntime = Path.Combine(previous, "local-x68000", "retrorecomp-expedition");
        Directory.CreateDirectory(oldRuntime);
        var oldPlayer = Path.Combine(oldRuntime, "Lagoon - English.exe");
        File.WriteAllText(oldPlayer, "fixture: previous base player");
        var oldIdentity = new string('c', 64);
        WriteJson(Path.Combine(oldRuntime,"lagoon-extra-install.json"),new {
            format=1,converter_version="1.8.0",system="x68",language="en",executable=Path.GetFileName(oldPlayer),sha256=Hash(File.ReadAllBytes(oldPlayer)),
            reference_validated=true,user_disk=true,source_identity=oldIdentity
        });
        var oldTemplate = new byte[1232*1024]; oldTemplate[2048] = 0x11;
        var newTemplate = new byte[1232*1024]; newTemplate[2048] = 0x22;
        void Report(string runtime,string player,string id,byte[] template) => WriteJson(Path.Combine(runtime,"conversion-report.json"),new {
            export_receipt=new {sha256=Hash(File.ReadAllBytes(player))},source_identity=id,
            machine=new { media=Enumerable.Range(0,5).Select(i=>new {format=i==4?"xdf":"dim",sha256=i==4?Hash(template):new string('0',64)}).ToArray() }
        });
        Report(oldRuntime,oldPlayer,oldIdentity,oldTemplate);
        Report(Path.GetDirectoryName(currentPlayer)!,currentPlayer,identity,newTemplate);
        var oldData = Path.Combine(oldRuntime,"datas","games","x68-"+oldIdentity);
        Directory.CreateDirectory(Path.Combine(oldData,"media")); File.WriteAllText(Path.Combine(oldData,"session.lock"),"");
        var oldDisk=(byte[])oldTemplate.Clone();
        System.Text.Encoding.ASCII.GetBytes("SAVE DATA       ").CopyTo(oldDisk,6144);oldDisk[6144+0x20]=35;oldDisk[6144+0x10]=6;oldDisk[6144+0x100]=0x99;
        var oldPath=Path.Combine(oldData,"media","disk-05.xdf");File.WriteAllBytes(oldPath,oldDisk);
        Directory.CreateDirectory(Path.Combine(data,"media"));
        var destination=Path.Combine(data,"media","disk-05.xdf");File.WriteAllBytes(destination,newTemplate);
        var original=Hash(File.ReadAllBytes(oldPath));
        var source=current.OpenExternalImportSource(previous);
        if(source.Inspect()[0].Level!=35 || current.OpenExternalImportSource(oldRuntime).Inspect()[0].Level!=35)
            throw new Exception("Previous installation root/runtime resolution failed.");
        source.CopySlotTo(1,current.OpenEnhancedSaveDisk(),1);
        var copied=File.ReadAllBytes(destination);
        if(!copied.AsSpan(6144,1024).SequenceEqual(oldDisk.AsSpan(6144,1024)) || copied[2048]!=0x22 || Hash(File.ReadAllBytes(oldPath))!=original)
            throw new Exception("External import overwrote engine data or source save.");
        try { source.CopySlotTo(1,current.OpenEnhancedSaveDisk(),1); throw new Exception("An occupied destination was overwritten."); }
        catch(InvalidOperationException error) when(error.Message.Contains("already contains")) {}
        if(!File.ReadAllBytes(destination).SequenceEqual(copied)) throw new Exception("Refused occupied import changed destination.");
        var malformed=(byte[])oldDisk.Clone();malformed[2048]^=1;File.WriteAllBytes(oldPath,malformed);
        try { source.Inspect(); throw new Exception("Unrecognized external disk accepted."); }
        catch(InvalidDataException) {}
        Console.WriteLine("PASS: previous-installation browse roots, old read-only source validation, sector-only import, new engine preservation and occupied/malformed refusal.");
    }

    internal static async Task ProcessChecks()
    {
        var dotnet = SetupTools.ResolveExecutable("dotnet");
        var assembly = typeof(UnifiedLauncherChecks).Assembly.Location;
        var lines = new System.Collections.Concurrent.ConcurrentQueue<string>();
        LaunchPlan Plan(string value) => new(dotnet, Path.GetDirectoryName(assembly)!, new[] { assembly, "--setup-child", value });
        await X68000Setup.RunAsync(Plan("path with spaces"), lines.Enqueue, CancellationToken.None);
        if (!lines.Contains("path with spaces")) throw new Exception("Setup output streaming or path arguments failed.");
        try { await X68000Setup.RunAsync(Plan("failure"), lines.Enqueue, CancellationToken.None); throw new Exception("Setup failure was ignored."); }
        catch (InvalidOperationException error) when (error.Message.Contains("Controlled setup failure")) { }
        using var cancel = new CancellationTokenSource(TimeSpan.FromMilliseconds(250));
        var timer = System.Diagnostics.Stopwatch.StartNew();
        try { await X68000Setup.RunAsync(Plan("delay"), lines.Enqueue, cancel.Token); throw new Exception("Setup cancellation was ignored."); }
        catch (OperationCanceledException) { }
        if (timer.Elapsed > TimeSpan.FromSeconds(5)) throw new Exception("Cancelled setup child did not exit promptly.");
        Console.WriteLine("PASS: streamed setup output, propagated tool errors and cancellation/reaping of setup child.");
    }

    internal static void Installed(string project, string output)
    {
        var adapter = new X68000Adapter(project, output);
        var controller = new LauncherController(adapter);
        var readiness = controller.GetReadiness();
        if (!readiness.Ready) throw new Exception(readiness.Summary);
        var plan = controller.BuildLaunchPlan();
        if (Path.GetFileName(plan.Executable) != "Lagoon - Atlas.exe") throw new Exception("Main launcher did not select qualified Atlas.");
        Console.WriteLine(JsonSerializer.Serialize(new { ready = true, executable = plan.Executable, working_directory = plan.WorkingDirectory }));
    }
}
