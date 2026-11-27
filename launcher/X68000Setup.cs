using Companion.Launcher.Core;
using System.Diagnostics;
using System.Text.Json;

internal static class X68000Setup
{
    internal static bool RuntimeCurrent(string project)
    {
        var marker = Path.Combine(project, "x68000", "runtime-version.json");
        if (!File.Exists(marker) || new FileInfo(marker).Length > 16384) throw new InvalidDataException("The X68000 runtime version file is missing. Extract the complete current package.");
        using var desired = JsonDocument.Parse(File.ReadAllBytes(marker));
        var receipt = Path.Combine(project, "local-x68000", "retrorecomp-expedition", "lagoon-atlas-install.json");
        if (!File.Exists(receipt) || new FileInfo(receipt).Length > 16384) return false;
        using var installed = JsonDocument.Parse(File.ReadAllBytes(receipt));
        return installed.RootElement.TryGetProperty("runtime_revision", out var rev) && installed.RootElement.TryGetProperty("presentation_version", out var view) &&
            rev.GetString() == desired.RootElement.GetProperty("runtime_revision").GetString() && view.GetInt32() == desired.RootElement.GetProperty("presentation_version").GetInt32();
    }
    internal static LaunchPlan CachedPlan(string project)
    {
        var script = Path.Combine(Path.GetFullPath(project), "x68000", "tools", "setup_bootstrap.ps1");
        if (!File.Exists(script)) throw new FileNotFoundException("Extract the complete release: X68000 setup files are missing.", script);
        return new(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe"),
            Path.GetFullPath(project), new[] { "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", script, "-RebuildCached" });
    }
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
        if (process.ExitCode != 0) throw new X68000SetupException(process.ExitCode, "X68000 setup stopped.\n" + string.Join("\n", tail));
    }
}

internal sealed class X68000SetupException(int exitCode, string message) : InvalidOperationException(message)
{
    internal int ExitCode { get; } = exitCode;
}
