"""Minimal BPS (Floating IPS) patch applier.

Usage: python bps_apply.py <source> <patch.bps> <dest>
Verifies source CRC32 against the patch footer, applies, and requires the
target CRC32 from the footer to match. Varints use byuu's BPS spec encoding
(see read_varint).
"""
import struct, sys, zlib

def read_varint(buf, pos):
    # byuu BPS spec, verbatim:
    #   data = 0; shift = 1
    #   loop: x = read(); data += (x & 0x7f) * shift
    #         if x & 0x80: break
    #         shift <<= 7; data += shift
    data = 0
    shift = 1
    while True:
        x = buf[pos]
        pos += 1
        data += (x & 0x7F) * shift
        if x & 0x80:
            break
        shift <<= 7
        data += shift
    return data, pos

def read_signed(buf, pos):
    data, pos = read_varint(buf, pos)
    value = data >> 1
    if data & 1:
        value = -value
    return value, pos

def main():
    src = open(sys.argv[1], "rb").read()
    patch = open(sys.argv[2], "rb").read()
    assert patch[:4] == b"BPS1", "not a BPS patch"
    src_crc, tgt_crc, data_crc = struct.unpack("<III", patch[-12:])
    have = zlib.crc32(src) & 0xFFFFFFFF
    if have != src_crc:
        raise SystemExit(f"source CRC mismatch: have {have:08X}, patch wants {src_crc:08X}")
    p = len(b"BPS1")
    src_len, p = read_varint(patch, p)
    tgt_len, p = read_varint(patch, p)
    meta_len, p = read_varint(patch, p)
    p += meta_len
    print(f"header: srcLen={src_len} tgtLen={tgt_len} meta={meta_len}")
    out = bytearray()
    src_pos = 0
    tgt_pos = 0
    end = len(patch) - 12
    actions = 0
    while p < end:
        action, p = read_varint(patch, p)
        length = (action >> 2) + 1
        mode = action & 3
        actions += 1
        if mode == 0:      # SourceRead: source at same offset as output
            out += src[len(out):len(out) + length]
        elif mode == 1:    # TargetRead: literal bytes
            out += patch[p:p + length]
            p += length
        elif mode == 2:    # SourceCopy: signed relative source pointer
            delta, p = read_signed(patch, p)
            src_pos += delta
            out += src[src_pos:src_pos + length]
            src_pos += length
        else:              # TargetCopy: signed relative target pointer
            delta, p = read_signed(patch, p)
            tgt_pos += delta
            for i in range(length):
                out.append(out[tgt_pos + i])
            tgt_pos += length
    got_tgt = zlib.crc32(out) & 0xFFFFFFFF
    print(f"source len={len(src)} crc={have:08X} (verified)")
    print(f"actions={actions} target len={len(out)} crc={got_tgt:08X} "
          f"expect={tgt_crc:08X} MATCH={got_tgt == tgt_crc}")
    if got_tgt != tgt_crc:
        raise SystemExit("target CRC mismatch")
    open(sys.argv[3], "wb").write(bytes(out))
    print(f"wrote {sys.argv[3]}")

main()
