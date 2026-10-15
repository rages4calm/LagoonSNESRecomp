#!/usr/bin/env python3
"""Strict BPS1 applier for private comparisons. Never overwrites the input.

Mode order verified against https://github.com/Alcaro/Flips/blob/master/libbps.cpp
"""
import argparse
import json
import zlib
from pathlib import Path


def read_varint(data, off, end=None):
    end = len(data) if end is None else end
    shift, value = 1, 0
    for _ in range(10):
        if off >= end: raise ValueError('truncated BPS integer')
        b=data[off]; off+=1; value+=(b&127)*shift
        if b&128: return value,off
        shift<<=7; value+=shift
    raise ValueError('oversized BPS integer')


def apply_patch(patch, source):
    if len(patch)<19 or patch[:4]!=b'BPS1': raise ValueError('not BPS1')
    crc=lambda data:zlib.crc32(data)&0xffffffff
    if crc(patch[:-4])!=int.from_bytes(patch[-4:],'little'):raise ValueError('patch CRC mismatch')
    if crc(source)!=int.from_bytes(patch[-12:-8],'little'):raise ValueError('source CRC mismatch')
    end=len(patch)-12
    source_len,off=read_varint(patch,4,end)
    target_len,off=read_varint(patch,off,end)
    meta_len,off=read_varint(patch,off,end)
    if source_len!=len(source) or off+meta_len>end:raise ValueError('invalid source size or metadata')
    if target_len>64*1024*1024:raise ValueError('target exceeds SNES tool size limit')
    meta=patch[off:off+meta_len];off+=meta_len
    target=bytearray();src_pos=dst_pos=0
    while off<end and len(target)<target_len:
        action,off=read_varint(patch,off,end)
        mode,length=action&3,(action>>2)+1
        if len(target)+length>target_len:raise ValueError('action exceeds target size')
        if mode==0: # SourceRead, output-relative unchanged run
            pos=len(target)
            if pos+length>len(source):raise ValueError('SourceRead out of bounds')
            target.extend(source[pos:pos+length])
        elif mode==1: # TargetRead, literal patch bytes
            if off+length>end:raise ValueError('truncated TargetRead')
            target.extend(patch[off:off+length]);off+=length
        else:
            distance,off=read_varint(patch,off,end)
            delta=-(distance>>1) if distance&1 else distance>>1
            if mode==2: # SourceCopy, persistent source cursor
                src_pos+=delta
                if src_pos<0 or src_pos+length>len(source):raise ValueError('SourceCopy out of bounds')
                target.extend(source[src_pos:src_pos+length]);src_pos+=length
            else: # TargetCopy, permits overlapping repetition
                dst_pos+=delta
                if dst_pos<0 or dst_pos>=len(target):raise ValueError('TargetCopy references unwritten output')
                for _ in range(length):target.append(target[dst_pos]);dst_pos+=1
    if off!=end or len(target)!=target_len:raise ValueError('action stream size mismatch')
    if crc(target)!=int.from_bytes(patch[-8:-4],'little'):raise ValueError('target CRC mismatch')
    return bytes(target),meta.decode('utf-8','replace')


def lorom_bus(offset):return ((offset>>15)<<16)|0x8000|(offset&0x7fff)


def diff_runs(source,target):
    runs=[];i=0
    while i<max(len(source),len(target)):
        if i<min(len(source),len(target)) and source[i]==target[i]:i+=1;continue
        start=i;i+=1
        while i<max(len(source),len(target)) and (i>=min(len(source),len(target)) or source[i]!=target[i]):i+=1
        runs.append(dict(offset=f'{start:06X}',bus=f'{lorom_bus(start):06X}',length=i-start,
                         old=source[start:i].hex(),new=target[start:i].hex()))
    return runs


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('patch',type=Path);p.add_argument('source',type=Path)
    p.add_argument('--out',type=Path);p.add_argument('--diff',action='store_true');p.add_argument('--json',type=Path)
    a=p.parse_args();source=a.source.read_bytes();target,meta=apply_patch(a.patch.read_bytes(),source)
    if a.out:
        if a.out.resolve()==a.source.resolve():raise ValueError('refusing to overwrite source ROM')
        a.out.write_bytes(target)
    runs=diff_runs(source,target)
    print(f'All three CRCs verified; {len(source)} -> {len(target)} bytes; {len(runs)} runs, {sum(r["length"] for r in runs)} changed bytes')
    if meta:print('Metadata (untrusted patch data):',repr(meta))
    if a.diff:
        for r in runs:print(f'{r["offset"]} bus={r["bus"]} len={r["length"]} {r["old"][:40]} -> {r["new"][:40]}')
    if a.json:a.json.write_text(json.dumps(dict(metadata=meta,changes=runs),indent=2))


if __name__=='__main__':main()
