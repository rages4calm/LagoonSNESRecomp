using System.Security.Cryptography;

// English Lagoon stores its three saves in one 1 KiB sector each on the User Disk.
internal sealed class X68000SaveDisk(string root, string templateHash, bool canImport)
{
    private const int SectorSize = 1024;
    private const int DiskSize = 1232 * SectorSize;
    private const int FirstSlotSector = 6;
    internal const int SlotCount = 3;
    private readonly bool importDestination = canImport;
    private readonly string diskPath = Path.Combine(root, "media", "disk-05.xdf");

    internal sealed record Slot(int Number, int? Level)
    {
        public override string ToString() => Level is int level
            ? $"Slot {Number}  ·  Level {level}"
            : $"Slot {Number}  ·  Empty";
    }

    internal IReadOnlyList<Slot> Inspect()
    {
        using var session = Lock();
        var disk = Read();
        return Enumerable.Range(1, SlotCount).Select(number =>
        {
            var sector = SlotBytes(disk, number);
            return new Slot(number, sector[0] == 0 ? null : sector[16]);
        }).ToArray();
    }

    internal void CopySlotTo(int from, X68000SaveDisk destination, int to)
    {
        if (!destination.importDestination) throw new InvalidOperationException("Choose a slot in X68000 Enhanced.");
        if (Path.GetFullPath(diskPath).Equals(Path.GetFullPath(destination.diskPath), StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Choose a different X68000 User Disk as the destination.");
        using var sourceSession = Lock();
        using var destinationSession = destination.Lock();
        var source = Read();
        var target = destination.Read();
        var data = SlotBytes(source, from);
        var slot = SlotBytes(target, to);
        if (data[0] == 0) throw new InvalidOperationException("The source slot is empty.");
        if (slot[0] != 0) throw new InvalidOperationException("The destination slot already contains a save.");
        data.CopyTo(slot);
        destination.Write(target);
    }

    private static Span<byte> SlotBytes(byte[] disk, int number)
    {
        if (number is < 1 or > SlotCount) throw new ArgumentOutOfRangeException(nameof(number));
        return disk.AsSpan((FirstSlotSector + number - 1) * SectorSize, SectorSize);
    }

    private static void RejectLink(string path)
    {
        if (File.GetAttributes(path).HasFlag(FileAttributes.ReparsePoint))
            throw new InvalidDataException("A linked X68000 data directory or save file cannot be used.");
    }

    private FileStream Lock()
    {
        if (!File.Exists(diskPath))
            throw new InvalidOperationException("A User Disk is required. Start the corresponding game once, then close it.");
        for (var directory = new DirectoryInfo(Path.GetDirectoryName(diskPath)!); directory != null; directory = directory.Parent)
            RejectLink(directory.FullName);
        RejectLink(diskPath);
        var lockPath = Path.Combine(root, "session.lock");
        RejectLink(lockPath);
        try
        {
            // Same exclusive Windows lock as the installed RetroRecomp player.
            return new FileStream(lockPath, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
        }
        catch (IOException error)
        {
            throw new InvalidOperationException("Close the X68000 game before reading or copying its in-game saves.", error);
        }
    }

    private byte[] Read()
    {
        using var stream = File.OpenRead(diskPath);
        if (stream.Length != DiskSize) throw new InvalidDataException("This User Disk has an unexpected size.");
        var disk = new byte[DiskSize];
        stream.ReadExactly(disk);
        var content = (byte[])disk.Clone();
        for (var slot = 1; slot <= SlotCount; slot++)
        {
            var sector = SlotBytes(content, slot);
            if (sector.ContainsAnyExcept((byte)0) &&
                (!sector.StartsWith("SAVE DATA       "u8) || sector[16] == 0))
                throw new InvalidDataException($"Slot {slot} is not a recognized English Lagoon save.");
            sector.Clear();
        }
        if (!Convert.ToHexString(SHA256.HashData(content)).Equals(templateHash, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("This User Disk differs from the verified LagoonExtra format.");
        return disk;
    }

    private void Write(byte[] disk)
    {
        var pending = Path.Combine(Path.GetDirectoryName(diskPath)!, $".save-transfer-{Guid.NewGuid():N}");
        try
        {
            using (var stream = new FileStream(pending, FileMode.CreateNew, FileAccess.Write, FileShare.None))
            {
                stream.Write(disk);
                stream.Flush(flushToDisk: true);
            }
            File.Move(pending, diskPath, overwrite: true);
        }
        finally { File.Delete(pending); }
    }
}
