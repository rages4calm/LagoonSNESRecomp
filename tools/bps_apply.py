#!/usr/bin/env python3
"""Apply a BPS 1.0 patch and report exactly which bytes it changes.

Why this exists: a QoL ROM hack is other people's reverse engineering, paid for.
Applying it to a copy and diffing against the original turns "measure for an hour
to find a value" into "here is the offset somebody already found", and each offset
can then be checked against the engine's own code before any of it is trusted.

The format is small but easy to get subtly wrong: offsets are relative and signed,
source and output have separate cursors, and the two "same copy" modes are written
differently by different tools. So this file does not assert a reading of the
spec. `--solve` tries the plausible readings and keeps only the ones that produce
the header's target length AND match the patch's own CRC-32s, which is the
patch agreeing with the implementation rather than the other way round.

  python tools/bps_apply.py <patch.bps> <source-rom> [--out patched.sfc] [--diff]
  python tools/bps_apply.py <patch.bps> <source-rom> --solve

Diff output gives the file offset and the LoROM bus address, the form the rest of
this project's tools speak.
"""

import binascii
import itertools
import sys


def read_varint(data, off):
    """BPS 1.0 variable-length integer: base-128 with the high bit as terminator,
    and a shift added after every continuation byte -- not plain LEB128. A source
    length of 1 MiB decodes exactly, which is the first proof the convention is
    right."""
    shift = 1
    value = 0
    while True:
        b = data[off]
        off += 1
        value += (b & 0x7F) * shift
        if b & 0x80:
            break
        shift <<= 7
        value += shift
    return value, off


def read_signed(data, off):
    """Relative offsets are signed: low bit is the sign, the rest the magnitude."""
    value, off = read_varint(data, off)
    return (-(value >> 1) if value & 1 else value >> 1), off


def apply_patch(patch, source, delta2=False, delta3=False, advance23=True,
                signed=True, swap=False):
    """Apply the patch. The keyword flags are the readings that different BPS
    writers disagree on; --solve searches them. `swap` covers writers that pack
    the mode as flag bits -- bit 0 "same", bit 1 source/target -- rather than as
    four sequential codes, which changes which actions carry a delta."""
    if patch[:4] != b"BPS1":
        raise ValueError("not a BPS1 patch: %r" % patch[:4])
    off = 4
    source_len, off = read_varint(patch, off)
    target_len, off = read_varint(patch, off)
    meta_len, off = read_varint(patch, off)
    meta = patch[off:off + meta_len]
    off += meta_len
    if len(source) != source_len:
        raise ValueError("patch expects %d source bytes, file has %d"
                         % (source_len, len(source)))
    read_delta = read_signed if signed else read_varint
    # Two independent cursors, one into the source and one into the output built
    # so far; each mode advances only its own.
    end = len(patch) - 12
    target = bytearray(target_len)
    tp = src_off = tgt_off = 0
    while tp < target_len and off < end:
        action, off = read_varint(patch, off)
        mode = action & 3
        if swap:
            mode = (0, 2, 1, 3)[mode]  # flag-bit packing: 0 src, 1 srcSame, 2 tgt
        length = (action >> 2) + 1
        if mode == 0:
            d, off = read_delta(patch, off)
            src_off += d
            chunk = source[src_off:src_off + length]
            if len(chunk) != length:
                raise ValueError("source copy off the end at %d" % src_off)
            src_off += length
            target[tp:tp + length] = chunk
        elif mode == 1:
            d, off = read_delta(patch, off)
            tgt_off += d
            if tgt_off < 0 or tgt_off + length > target_len:
                raise ValueError("target copy off the end: tgt_off=%d len=%d"
                                 % (tgt_off, length))
            for k in range(length):  # may overlap forward
                target[tp + k] = target[tgt_off + k]
            tgt_off += length
        elif mode == 2:  # same source copy: the ordinary unchanged run, taken from
            # the source at the position being written now, cursors left alone.
            if delta2:
                d, off = read_delta(patch, off)
                src_off += d
                d, off = read_delta(patch, off)
                src_off += d
            chunk = source[tp:tp + length]
            if len(chunk) != length:
                raise ValueError("source copy off the end at %d" % tp)
            target[tp:tp + length] = chunk
        else:  # same target copy: likewise relative to the output position
            if delta3:
                d, off = read_delta(patch, off)
                tgt_off += d
            if tp + length > target_len:
                raise ValueError("target copy off the end: tp=%d len=%d"
                                 % (tp, length))
            for k in range(length):
                target[tp + k] = target[tp + k]
        tp += length
    if tp != target_len:
        raise ValueError("patch produced %d bytes, header says %d (stopped at %d)"
                         % (tp, target_len, off))
    if off != end:
        raise ValueError("action stream ended at %d, footer starts at %d" % (off, end))
    crc_src = int.from_bytes(patch[end:end + 4], "little")
    crc_tgt = int.from_bytes(patch[end + 4:end + 8], "little")
    got_src = binascii.crc32(source) & 0xFFFFFFFF
    got_tgt = binascii.crc32(bytes(target)) & 0xFFFFFFFF
    if crc_src != got_src:
        raise ValueError("source CRC mismatch: patch wants %08X, ROM is %08X"
                         % (crc_src, got_src))
    if crc_tgt != got_tgt:
        raise ValueError("result CRC mismatch under these semantics: patch wants "
                         "%08X, got %08X" % (crc_tgt, got_tgt))
    return bytes(target), meta.decode("utf-8", "replace")


def solve(patch, source):
    """Find the semantics this writer used, judged by the patch's own CRCs."""
    winners = []
    for delta2, delta3, advance23, signed, swap in itertools.product(
            (False, True), (False, True), (False, True), (True, False),
            (False, True)):
        try:
            target, meta = apply_patch(patch, source, delta2, delta3, advance23,
                                       signed, swap)
        except Exception as exc:
            # Which stage a reading dies at is the diagnosis: "stopped at"/
            # "off the end" means the byte stream desynced, a CRC line means the
            # structure was right and only the check disagrees.
            print("  delta2=%d delta3=%d advance23=%d signed=%d swap=%d -> %s"
                  % (delta2, delta3, advance23, signed, swap,
                     str(exc).split("\n")[0]))
            continue
        winners.append(((delta2, delta3, advance23, signed, swap), target, meta))
        print("MATCH delta2=%d delta3=%d advance23=%d signed=%d swap=%d"
              % (delta2, delta3, advance23, signed, swap))
    if not winners:
        raise ValueError("no reading of the format reproduced the patch's CRCs")
    if len(winners) > 1:
        print("note: %d readings agree (they differ only where the patch never "
              "exercises the difference)" % len(winners))
    return winners[0]


def lorom_bus(file_off):
    """File offset -> LoROM bus address, to match dis.py and friends."""
    return ((file_off >> 15) << 16) | (file_off & 0x7FFF)


def report_diff(source, target):
    runs = []
    i = 0
    n = min(len(source), len(target))
    while i < n:
        if source[i] != target[i]:
            start = i
            while i < n and source[i] != target[i]:
                i += 1
            runs.append((start, i - start))
        else:
            i += 1
    print("%d changed run(s), %d byte(s) total"
          % (len(runs), sum(r[1] for r in runs)))
    for start, length in runs:
        print("  file %06X  bus %06X  len %-4d old %-14s new %s"
              % (start, lorom_bus(start), length,
                 source[start:start + min(length, 7)].hex(" "),
                 target[start:start + min(length, 7)].hex(" ")))
    return runs


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    with open(argv[0], "rb") as f:
        patch = f.read()
    with open(argv[1], "rb") as f:
        source = f.read()
    if "--probe" in argv:
        crc = binascii.crc32(source) & 0xFFFFFFFF
        print("source crc %08X  source len %d  patch len %d"
              % (crc, len(source), len(patch)))
        print("source crc found little-endian at %d, big-endian at %d"
              % (patch.rfind(crc.to_bytes(4, "little")),
                 patch.rfind(crc.to_bytes(4, "big"))))
        print("last 32 bytes: %s" % patch[-32:].hex(" "))
        return 0
    if "--solve" in argv:
        (flags, target, meta) = solve(patch, source)
        print("using delta2=%d delta3=%d advance23=%d signed=%d swap=%d" % flags)
    else:
        target, meta = apply_patch(patch, source)
    print("patch ok: source %d -> target %d bytes" % (len(source), len(target)))
    if meta.strip():
        print("metadata: %s" % meta.strip().replace("\n", " | ")[:400])
    if "--out" in argv:
        with open(argv[argv.index("--out") + 1], "wb") as f:
            f.write(target)
        print("wrote %s" % argv[argv.index("--out") + 1])
    if "--diff" in argv:
        report_diff(source, target)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
