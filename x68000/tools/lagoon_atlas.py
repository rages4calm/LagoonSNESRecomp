"""Generate compact terrain layers from the player's private Lagoon disks."""

from pathlib import Path
import json
import struct
import hashlib

from build_retrorecomp import checked_file, DISK_HASHES, DIM_HEADER, SECTOR, lagoon_program

BLOCK_SIZES = (16384, 2048, 2048, 16384, 16384, 2048, 32768, 1280)
END = bytes.fromhex("96b39045")
AREA_NAMES_SHA256 = "164a3e8c5d3aaddee50a3370def78d8f9a6aca98fa239e1677c2db2662d19a98"


def disk_files(path, sha256):
    body = checked_file(path, sha256, DIM_HEADER + 1232 * SECTOR).read_bytes()[DIM_HEADER:]

    def read_chain(cluster, size):
        seen = set()
        chunks = []
        while 2 <= cluster < 1223 and cluster not in seen:
            seen.add(cluster)
            offset = (cluster + 9) * SECTOR
            chunks.append(body[offset:offset + SECTOR])
            fat_offset = SECTOR + cluster * 3 // 2
            value = int.from_bytes(body[fat_offset:fat_offset + 2], "little")
            cluster = value >> 4 if cluster & 1 else value & 0xfff
        if cluster < 0xff8 or (size is not None and size > len(chunks) * SECTOR):
            raise ValueError("Invalid map disk cluster chain.")
        data = b"".join(chunks)
        return data if size is None else data[:size]

    def walk(directory, descend):
        for offset in range(0, len(directory), 32):
            entry = directory[offset:offset + 32]
            if entry[0] == 0:
                break
            if entry[0] in (0xe5, 0x2e) or entry[11] & 8:
                continue
            name = entry[:8].rstrip()
            extension = entry[8:11]
            cluster, size = struct.unpack_from("<HI", entry, 26)
            if entry[11] & 16:
                if descend:
                    yield from walk(read_chain(cluster, None), False)
            elif name.startswith(b"MAP_") and name[4:].isdigit() and extension in (b"DAT", b"PAL"):
                yield (int(name[4:]), extension), read_chain(cluster, size)

    return dict(walk(body[5 * SECTOR:11 * SECTOR], True))


def map_blocks(data):
    """Lagoon's $6FE6 decoder uses a bitmap of run-coded byte values per block."""
    blocks = []
    pos = 0
    for expected in BLOCK_SIZES:
        bitmap = data[pos:pos + 32]
        pos += 32
        block = bytearray()
        while data[pos:pos + 4] != END:
            if len(bitmap) != 32 or pos >= len(data):
                raise ValueError("Incomplete Lagoon map block.")
            token = data[pos]
            pos += 1
            count = 1
            if bitmap[token // 8] & (0x80 >> (token % 8)):
                if pos >= len(data):
                    raise ValueError("Incomplete Lagoon map run.")
                count = data[pos] or 256
                pos += 1
            if len(block) + count > expected + 1:
                raise ValueError("Unexpected Lagoon map block size.")
            block.extend(bytes([token]) * count)
        pos += len(END)
        if len(block) not in (expected, expected + 1):
            raise ValueError("Unexpected Lagoon map block size.")
        blocks.append(bytes(block[:expected]))
    if pos != len(data):
        raise ValueError("Unexpected Lagoon map trailer.")
    return blocks


def visible_bounds(tiles, metatiles, grid, palette):
    colors = struct.unpack_from(">256H", palette)
    painted = set()
    for cell in set(grid):
        for descriptor in struct.unpack_from(">4H", metatiles, cell * 8):
            start = (descriptor & 0x1ff) * 32
            group = descriptor >> 8 & 0xf0
            # Classify the full original bitmap. A black preview sample or a
            # zero cell ID alone does not establish that the tile is blank.
            if any(colors[((group | (value >> 4)) + 16) & 0xff] or
                   colors[((group | (value & 15)) + 16) & 0xff]
                   for value in tiles[start:start + 32]):
                painted.add(cell)
                break
    visible = [divmod(index, 128) for index, cell in enumerate(grid) if cell in painted]
    if not visible:
        return (0, 0, 512, 512)
    rows, columns = zip(*visible)
    # Preserve two metatiles of context next to the outermost visible artwork.
    x = max(0, min(columns) - 2) * 4
    y = max(0, min(rows) - 2) * 4
    right = min(128, max(columns) + 3) * 4
    bottom = min(128, max(rows) + 3) * 4
    return (x, y, right - x, bottom - y)


def terrain(data, palette):
    tiles, foreground, metatiles, foreground_grid, grid, *_ = map_blocks(data)
    if len(palette) < 512:
        raise ValueError("Incomplete Lagoon palette.")
    # The $EBEA map loader increments the PCG palette group in each pair of
    # foreground descriptors. Retain both terrain planes and the packed patterns.
    foreground = b"".join(struct.pack(">I", (pair + 0x01000100) & 0xffffffff)
                          for pair, in struct.iter_unpack(">I", foreground))
    pixels = palette[:512] + tiles + metatiles + grid + foreground + foreground_grid
    return pixels, visible_bounds(tiles, metatiles, grid, palette)


def area_names(system, system_sha256=None):
    image = checked_file(system, system_sha256 or next(iter(DISK_HASHES.values())), DIM_HEADER + 1232 * SECTOR)
    program, _, _ = lagoon_program(image.read_bytes())
    # The guest's save UI indexes the area string table by area number minus one.
    table = program[0x194D8:0x19C35]
    if hashlib.sha256(table).hexdigest() != AREA_NAMES_SHA256:
        raise ValueError("The English0.9b area-name table differs from its pinned original.")
    records = table.split(b"\x00\x1c", 50)
    names = {}
    for area, record in enumerate(records[:50], 1):
        if not record.startswith(b"p") or not (2 <= len(record) <= 23):
            raise ValueError(f"Invalid English area name for area {area}.")
        name = record[1:].decode("ascii")
        if any(not 32 <= ord(character) < 127 for character in name):
            raise ValueError(f"Invalid English area text for area {area}.")
        names[area] = name
    if len(names) != 50 or names[1] != "Atland":
        raise ValueError("Unrecognized English area table.")
    return names


def generate(disk_dir, system, output, *, system_sha256=None):
    files = {}
    for name, sha in list(DISK_HASHES.items())[1:]:
        for key, data in disk_files(Path(disk_dir) / name, sha).items():
            if key in files and data != files[key]:
                raise ValueError(f"Conflicting map file for {key}.")
            files[key] = data
    maps = {}
    for (area, extension), data in sorted(files.items()):
        if extension == b"DAT":
            maps[area] = terrain(data, files[(area, b"PAL")])
    names = area_names(system, system_sha256)
    if not maps.keys() <= names.keys():
        raise ValueError("Missing English names for the generated atlas.")
    with Path(output).open("w", encoding="ascii") as out:
        for area, (data, _) in maps.items():
            out.write(f"static const unsigned char lagoon_area_{area}[] = {{\n")
            for start in range(0, len(data), 32):
                out.write(",".join(str(value) for value in data[start:start+32])+",\n")
            out.write("};\n")
        out.write("static const unsigned char *const lagoon_areas[] = {\n")
        for area in range(max(maps) + 1):
            out.write(f"lagoon_area_{area},\n" if area in maps else "NULL,\n")
        out.write("};\n")
        out.write("static const SDL_Rect lagoon_area_bounds[] = {\n")
        for area in range(max(maps) + 1):
            bounds = maps[area][1] if area in maps else (0, 0, 0, 0)
            out.write("{" + ",".join(str(value) for value in bounds) + "},\n")
        out.write("};\n")
        out.write("static const char *const lagoon_area_labels[] = {\n")
        for area in range(max(maps) + 1):
            out.write(json.dumps(names[area]) + ",\n" if area in maps else "NULL,\n")
        out.write("};\n")
    return len(maps)
