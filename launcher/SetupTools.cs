using Companion.Launcher.Core;

// SetupProcessRunner deliberately accepts only absolute executable paths.
// Resolve our named tool without invoking a shell or searching the current dir.
internal static class SetupTools
{
    internal static string ResolveExecutable(string name, string? searchPath = null)
    {
        if (string.IsNullOrWhiteSpace(name) || Path.GetFileName(name) != name)
            throw new ArgumentException("Expected a setup tool name, not a relative path.", nameof(name));
        var names = OperatingSystem.IsWindows() && Path.GetExtension(name).Length == 0
            ? new[] { name + ".exe", name } : new[] { name };
        foreach (var entry in (searchPath ?? Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator))
        {
            var directory = OperatingSystem.IsWindows() ? entry.Trim().Trim('"') : entry;
            if (!Path.IsPathFullyQualified(directory)) continue;
            foreach (var file in names)
            {
                try
                {
                    var candidate = Path.GetFullPath(Path.Combine(directory, file));
                    if (!File.Exists(candidate)) continue;
                    if (!OperatingSystem.IsWindows() && (File.GetUnixFileMode(candidate) &
                        (UnixFileMode.UserExecute | UnixFileMode.GroupExecute | UnixFileMode.OtherExecute)) == 0) continue;
                    return candidate;
                }
                catch (Exception error) when (error is IOException or UnauthorizedAccessException or ArgumentException) { }
            }
        }
        throw new FileNotFoundException(name == "python3"
            ? "Python 3 was not found on PATH. Install Python 3.11 or newer, then reopen Lagoon Companion and choose your ROM again. No compiler is needed."
            : $"The setup tool '{name}' was not found on PATH.");
    }
}

// Progress<T> posts asynchronously to the UI context. A queued status from a
// completed/failed import must not hide its error or overwrite a later attempt.
internal sealed class SetupProgress : IProgress<OperationProgress>, IDisposable
{
    private readonly IProgress<OperationProgress> queued;
    private int active = 1;

    internal SetupProgress(Action<OperationProgress> update) =>
        queued = new Progress<OperationProgress>(value => { if (Volatile.Read(ref active) != 0) update(value); });

    public void Report(OperationProgress value) => queued.Report(value);
    public void Dispose() => Interlocked.Exchange(ref active, 0);
}
