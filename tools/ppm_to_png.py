#!/usr/bin/env python3
"""Convert the host's PPM frame dumps to PNG so they can be looked at.

Usage: python tools/ppm_to_png.py <dir> [frame ...]

With no frames, converts every present_*.ppm in the directory. Reads the
binary P6 header by hand, so it works without Pillow.
"""
import glob
import os
import struct
import sys
import zlib


def read_ppm(path):
    with open(path, "rb") as f:
        magic = f.readline().strip()
        if magic != b"P6":
            raise ValueError("%s is not a binary PPM (%r)" % (path, magic))
        line = f.readline()
        while line.startswith(b"#"):
            line = f.readline()
        w, h = (int(x) for x in line.split())
        maxv = int(f.readline().strip())
        assert maxv == 255, maxv
        data = f.read(w * h * 3)
    return w, h, data


def write_png(path, w, h, rgb):
    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    raw = bytearray()
    stride = w * 3
    for y in range(h):
        raw.append(0)
        raw += rgb[y * stride:(y + 1) * stride]
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw), 6))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def main(argv):
    d = argv[0]
    want = [int(x) for x in argv[1:]]
    files = sorted(glob.glob(os.path.join(d, "present_*.ppm")))
    if want:
        files = [p for p in files
                 if any(("%06d" % n) in os.path.basename(p) for n in want)] or files
    if not files:
        print("no present_*.ppm in %s" % d)
        return 1
    for p in files:
        w, h, rgb = read_ppm(p)
        out = p[:-4] + ".png"
        write_png(out, w, h, rgb)
        print("%s -> %s (%dx%d)" % (os.path.basename(p), os.path.basename(out), w, h))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
