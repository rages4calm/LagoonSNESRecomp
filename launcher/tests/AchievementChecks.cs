using System.Text.Json;

internal static class AchievementChecks
{
    public static void Run(string root)
    {
        if (Directory.Exists(root)) throw new IOException("Use a fresh achievement-test directory.");
        Directory.CreateDirectory(root);
        var catalog = Path.Combine(root, "catalog.json");
        File.WriteAllText(catalog, """
            {"schemaVersion":1,"gameId":"lagoon","editionId":"native-qol","catalogVersion":1,"title":"Lagoon Achievements","achievements":[
              {"id":"goal","title":"A journey","description":"Finish a real action.","event":"goal.done","rule":"counter","target":2},
              {"id":"secret.wrong-game","title":"The dancing visitor","description":"A secret spoiler.","event":"secret.visit","rule":"counter","target":1}
            ]}
            """);
        var saveRoot = Path.Combine(root, "saves");
        var progress = Path.Combine(saveRoot, "Achievements", "lagoon", "native-qol", "unseeded", "progress.json");
        var locked = GameAdapter.ReadAchievementFiles(catalog, saveRoot);
        Assert(locked.Entries.Count == 2 && locked.Entries.All(x => x.UnlockedAt == 0), "Missing progress did not produce locked entries.");
        Assert(!Directory.Exists(saveRoot), "Read-only browser created a progress directory.");
        Assert(locked.Entries[1].Title == "A familiar stranger" && !locked.Entries[1].Description.Contains("spoiler"), "Locked secret exposed its reveal.");
        Directory.CreateDirectory(Path.GetDirectoryName(progress)!);
        const string valid = """
            {"schemaVersion":1,"gameId":"lagoon","editionId":"native-qol","seedId":null,"catalogVersion":1,"achievements":[
              {"id":"goal","progress":1,"unlockedAt":0},{"id":"secret.wrong-game","progress":1,"unlockedAt":1700000000}
            ]}
            """;
        File.WriteAllText(progress, valid);
        var saved = File.ReadAllBytes(progress);
        var earned = GameAdapter.ReadAchievementFiles(catalog, saveRoot);
        Assert(earned.Entries[0].Progress == 1 && earned.Entries[1].Title == "The dancing visitor", "Progress/earned secret did not load.");
        Assert(saved.SequenceEqual(File.ReadAllBytes(progress)), "Browser changed progress.");
        var second = GameAdapter.ReadAchievementFiles(catalog, Path.Combine(root, "other-save-root"));
        Assert(second.Entries.All(x => x.Progress == 0 && x.UnlockedAt == 0), "Separate save roots shared progress.");
        foreach (var bad in new[] { "{broken", valid.Replace("native-qol", "original"), valid.Replace("\"seedId\":null", "\"seedId\":\"test\""), valid.Replace("\"progress\":1,\"unlockedAt\":0", "\"progress\":3,\"unlockedAt\":0") })
        {
            File.WriteAllText(progress, bad);saved = File.ReadAllBytes(progress);
            try { GameAdapter.ReadAchievementFiles(catalog, saveRoot); throw new Exception("Malformed/foreign progress accepted."); }
            catch (Exception ex) when (ex is JsonException or InvalidDataException) { }
            Assert(saved.SequenceEqual(File.ReadAllBytes(progress)), "Browser rewrote malformed/foreign progress.");
        }
        Console.WriteLine("PASS achievement browser: locked/earned/progress, secret reveal, save-root isolation, read-only missing/corrupt/foreign progress.");
    }
    private static void Assert(bool condition, string message) { if (!condition) throw new Exception(message); }
}
