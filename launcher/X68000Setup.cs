using Companion.Launcher.Core;
using System.Diagnostics;

internal static class X68000Setup
{
    internal static LaunchPlan BuildPlan(string project, string inputs, string? ipl = null,
        string? cgrom = null, string? bootstrapExecutable = null)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("X68000 Enhanced setup currently requires Windows.");
        if (string.IsNullOrWhiteSpace(inputs)) throw new FileNotFoundException("Choose your Lagoon X68000 disk ZIP or folder.");
        project = Path.GetFullPath(project);
        inputs = Path.GetFullPath(inputs.Trim().Trim('"'));
        if (!File.Exists(inputs) && !Directory.Exists(inputs)) throw new FileNotFoundException("Choose your Lagoon X68000 disk ZIP or folder.", inputs);
        var script = Path.Combine(project, "x68000", "tools", "setup_bootstrap.ps1");
        if (!File.Exists(script)) throw new FileNotFoundException("Extract the complete release: X68000 setup files are missing.", script);
        var bootstrap = bootstrapExecutable ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
            "WindowsPowerShell", "v1.0", "powershell.exe");
        if (!Path.IsPathFullyQualified(bootstrap) || !File.Exists(bootstrap))
            throw new FileNotFoundException("Windows PowerShell is unavailable. Extract the complete Windows release and try again.");
        var args = new List<string> { "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", script, "-Inputs", inputs };
        foreach (var entry in new[] { ("-Ipl", ipl), ("-Cgrom", cgrom) })
        {
            if (string.IsNullOrWhiteSpace(entry.Item2)) continue;
            var path = Path.GetFullPath(entry.Item2.Trim().Trim('"'));
            if (!File.Exists(path)) throw new FileNotFoundException("Choose an existing firmware file.", path);
            args.AddRange(new[] { entry.Item1, path });
        }
        return new(bootstrap, project, args);
    }

    internal static async Task RunAsync(LaunchPlan plan, Action<string> output, CancellationToken cancellation)
    {
        cancellation.ThrowIfCancellationRequested();
        var info = new ProcessStartInfo(plan.Executable) {
            WorkingDirectory = plan.WorkingDirectory, UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true,
        };
        foreach (var argument in plan.Arguments) info.ArgumentList.Add(argument);
        using var process = new Process { StartInfo = info };
        process.Start();
        var tail = new Queue<string>();
        async Task Drain(StreamReader reader)
        {
            while (await reader.ReadLineAsync(cancellation).ConfigureAwait(false) is string line)
            {
                lock (tail) { tail.Enqueue(line); if (tail.Count > 12) tail.Dequeue(); }
                output(line);
            }
        }
        var stdout = Drain(process.StandardOutput); var stderr = Drain(process.StandardError);
        try { await process.WaitForExitAsync(cancellation).ConfigureAwait(false); await Task.WhenAll(stdout, stderr).ConfigureAwait(false); }
        catch (OperationCanceledException)
        {
            try { if (!process.HasExited) process.Kill(entireProcessTree: true); } catch (InvalidOperationException) { }
            await process.WaitForExitAsync(CancellationToken.None).ConfigureAwait(false);
            try { await Task.WhenAll(stdout, stderr).ConfigureAwait(false); } catch (OperationCanceledException) { }
            throw;
        }
        if (process.ExitCode != 0) throw new InvalidOperationException("X68000 setup stopped.\n" + string.Join("\n", tail));
    }
}
