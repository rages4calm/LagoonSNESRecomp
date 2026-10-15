import importlib.util
from pathlib import Path
import unittest
import zlib

spec=importlib.util.spec_from_file_location('bps_apply',Path(__file__).resolve().parents[1]/'tools/bps_apply.py')
bps=importlib.util.module_from_spec(spec);spec.loader.exec_module(bps)


def number(value):
    out=bytearray()
    while True:
        byte=value&127;value>>=7
        if not value:
            out.append(byte|128)
            return bytes(out)
        out.append(byte);value-=1


def patch(source,target,actions):
    payload=b'BPS1'+number(len(source))+number(len(target))+number(0)+actions
    for data in (source,target):payload+=(zlib.crc32(data)&0xffffffff).to_bytes(4,'little')
    return payload+(zlib.crc32(payload)&0xffffffff).to_bytes(4,'little')


class BpsChecks(unittest.TestCase):
    def test_all_four_actions(self):
        source=b'ABCDEFGH';target=b'ABxyCDEABxy'
        actions=number(4)+number(5)+b'xy'+number(10)+number(4)+number(15)+number(0)
        self.assertEqual(bps.apply_patch(patch(source,target,actions),source),(target,''))

    def test_target_copy_can_overlap(self):
        source=b'A';target=b'AAAAAA'
        actions=number(0)+number(19)+number(0)
        self.assertEqual(bps.apply_patch(patch(source,target,actions),source)[0],target)

    def test_negative_source_cursor_delta(self):
        source=b'ABCDEFGH';target=b'FGBC'
        actions=number(6)+number(10)+number(6)+number(13)
        self.assertEqual(bps.apply_patch(patch(source,target,actions),source)[0],target)

    def test_corrupt_patch_and_wrong_source_rejected(self):
        encoded=patch(b'AB',b'AB',number(4))
        with self.assertRaisesRegex(ValueError,'patch CRC'):bps.apply_patch(encoded[:-1]+bytes([encoded[-1]^1]),b'AB')
        with self.assertRaisesRegex(ValueError,'source CRC'):bps.apply_patch(encoded,b'AC')

    def test_lo_rom_address_includes_upper_half(self):
        self.assertEqual(bps.lorom_bus(0xb6f2),0x01b6f2)


if __name__=='__main__':unittest.main()
