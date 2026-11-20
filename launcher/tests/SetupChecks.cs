using Companion.Launcher.Core;
using System.Diagnostics;

internal static class SetupChecks
{
    internal static async Task RunAsync(string root)
    {
        if (Directory.Exists(root)) throw new IOException("Use a fresh setup diagnostic directory.");
        Directory.CreateDirectory(root);
        var toolFolder = Path.Combine(root, "tools with spaces");
        Directory.CreateDirectory(toolFolder);
        var file = Path.Combine(toolFolder, OperatingSystem.IsWindows() ? "python3.exe" : "python3");
        File.WriteAllText(file, "synthetic executable resolution fixture");
        if (!OperatingSystem.IsWindows()) File.SetUnixFileMode(file, UnixFileMode.UserRead | UnixFileMode.UserExecute);
        Assert(SetupTools.ResolveExecutable("python3", toolFolder) == file, "Absolute tool resolution failed.");
        Assert(SetupTools.ResolveExecutable("python3", "." + Path.PathSeparator + toolFolder) == file,
            "Relative PATH entry must be ignored.");
        if (!OperatingSystem.IsWindows())
        {
            File.SetUnixFileMode(file, UnixFileMode.UserRead);
            AssertMissing(toolFolder);
            File.SetUnixFileMode(file, UnixFileMode.UserRead | UnixFileMode.UserExecute);
            var linkFolder = Path.Combine(root, "linked-tools"); Directory.CreateDirectory(linkFolder);
            File.CreateSymbolicLink(Path.Combine(linkFolder, "python3"), file);
            Assert(SetupTools.ResolveExecutable("python3", linkFolder) == Path.Combine(linkFolder, "python3"),
                "Normal system Python symlinks must work.");
        }
        AssertMissing(""); AssertMissing(".");

        // Reproduce the old failure against the actual pinned Companion runner.
        try
        {
            await SetupProcessRunner.RunAsync(new("python3", root, Array.Empty<string>()));
            throw new Exception("Pinned runner unexpectedly accepted a relative executable.");
        }
        catch (FileNotFoundException) { }

        if (!OperatingSystem.IsWindows())
        {
            var python = SetupTools.ResolveExecutable("python3");
            var probe = await SetupProcessRunner.RunAsync(new(python, root,
                new[] { "-c", "import sys; print(sys.argv[1])", "ROM path with spaces" }));
            Assert(probe.ExitCode == 0 && probe.StandardOutput.Trim() == "ROM path with spaces",
                "Resolved system Python did not run through the actual setup process runner.");
        }

        var executable = Environment.ProcessPath ?? throw new Exception("Test apphost path missing.");
        Assert(Path.IsPathFullyQualified(executable), "Test apphost must be absolute.");
        var child = new List<string>();
        if (Path.GetFileNameWithoutExtension(executable).Equals("dotnet", StringComparison.OrdinalIgnoreCase))
            child.Add(typeof(SetupChecks).Assembly.Location);
        child.Add("--setup-child");
        var output = await SetupProcessRunner.RunAsync(new(executable, root, child.Concat(new[] { "quoted path with spaces" }).ToArray()));
        Assert(output.ExitCode == 0 && output.StandardOutput.Trim() == "quoted path with spaces", "Absolute child invocation/arguments failed.");
        using (var cancellation = new CancellationTokenSource(TimeSpan.FromMilliseconds(250)))
        {
            var timer = Stopwatch.StartNew();
            try
            {
                await SetupProcessRunner.RunAsync(new(executable, root, child.Concat(new[] { "delay" }).ToArray()), cancellation.Token);
                throw new Exception("Cancelled child unexpectedly completed.");
            }
            catch (OperationCanceledException) { Assert(timer.Elapsed < TimeSpan.FromSeconds(10), "Cancelled setup child did not exit."); }
        }

        var previous = SynchronizationContext.Current;
        var ui = new QueuedContext();
        SynchronizationContext.SetSynchronizationContext(ui);
        try
        {
            string status = "";
            using (var progress = new SetupProgress(p => status = p.Detail))
            {
                progress.Report(new("Verifying your ROM", 0));
                ui.Drain(); Assert(status == "Verifying your ROM", "Active progress missing.");
                progress.Report(new("Stale verification message", 0));
                status = "Python is missing: actionable failure";
            }
            ui.Drain();
            Assert(status == "Python is missing: actionable failure", "Queued progress hid the setup failure.");
            using (var progress = new SetupProgress(p => status = p.Detail))
            {
                progress.Report(new("Retry is preparing", 0)); ui.Drain();
                Assert(status == "Retry is preparing", "A fresh attempt must still receive progress.");
            }
        }
        finally { SynchronizationContext.SetSynchronizationContext(previous); }
        Console.WriteLine("PASS: absolute setup tools, missing/non-executable diagnostics, quoted arguments, child cancellation and queued progress/error preservation.");
    }

    private static void AssertMissing(string path)
    {
        try { SetupTools.ResolveExecutable("python3", path); throw new Exception("Missing setup tool accepted."); }
        catch (FileNotFoundException error) { Assert(error.Message.Contains("3.11"), "Missing Python guidance is not actionable."); }
    }
    private static void Assert(bool condition, string message) { if (!condition) throw new Exception(message); }
    private sealed class QueuedContext : SynchronizationContext
    {
        private readonly Queue<(SendOrPostCallback Callback, object? State)> pending = new();
        public override void Post(SendOrPostCallback callback, object? state) => pending.Enqueue((callback, state));
        internal void Drain() { while (pending.TryDequeue(out var item)) item.Callback(item.State); }
    }
}
