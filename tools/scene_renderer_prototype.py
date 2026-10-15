"""Offline Lagoon scene renderer experiment. Private ROM/snapshots stay private.

Resolves world BG1/2 from retained room maps (never circular PPU tilemaps).
Uses actual uploaded native OAM; only ROM-decoded, queue/OAM-matched ordinary
NPC/enemy cells may extend outside native columns. No simulation or AI runs.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
from PIL import Image


def main():
    if sys.flags.optimize:raise SystemExit('Run without -O: strict snapshot guards must remain enabled')
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',required=True,type=Path,help='Dump prefix without suffix')
    p.add_argument('--rom',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path,help='Fresh private output directory')
    p.add_argument('--width',type=int,default=682)
    p.add_argument('--reference',type=Path,help='Optional expanded actual presentation')
    a=p.parse_args()
    assert 256<=a.width<=800 and a.width%2==0
    assert not a.out.exists(),'Use a fresh private output directory'
    prefix=str(a.snapshot)
    raw=lambda suffix:Path(prefix+suffix).read_bytes()
    ram,rom,oam=raw('.wram.bin'),a.rom.read_bytes(),raw('.oam.bin')
    vram=struct.unpack('<32768H',raw('.vram.bin'));pal=struct.unpack('<256H',raw('.cgram.bin'))
    regs=json.loads(raw('.regs.json'));word=lambda x:int.from_bytes(ram[x:x+2],'little')
    ptr=lambda x:int.from_bytes(ram[x:x+3],'little')
    assert word(0x48e)==0 and ptr(0x57)==0x138000 and ptr(0x5d)==0x198000
    assert ptr(0x69)==ptr(0x75)==0x068000 and ptr(0x88)==0x7f0000 and ptr(0x8b)==0x7f4000
    assert not ram[0x100e] and not ram[0x50] and regs['bgmode']==9 and regs['mosaic']==0
    assert regs['bgsc'][:3]==[0x71,0x79,0x3c] and regs['bg_tile_adr']==0x600
    assert regs['obsel']==2 and regs['setini']==0 and regs['cgadsub']==0
    assert not regs['windowsel'] and not regs['tmw'] and not regs['tsw'] and not regs['hdmaen_last']
    assert regs['hscroll'][0]==regs['hscroll'][1] and regs['vscroll'][0]==regs['vscroll'][1]
    assert regs['hscroll'][2]==regs['vscroll'][2]==0 and regs['tm']==31
    assert regs['inidisp']==15 and word(0x48c)<=64
    def byte(addr):
        bank,off=addr>>16,addr&65535
        if bank in (126,127):return ram[(bank-126)*65536+off]
        assert (bank&127)<32 and off>=32768
        return rom[(bank&31)*32768+(off&32767)]
    def rw(addr):return byte(addr)|(byte(addr+1)<<8)
    signed=lambda x:x-65536 if x&32768 else x
    left=(a.width-256)//2
    wx=word(0x480)+(((regs['hscroll'][0]&1023)-word(0x480)+512)&1023)-512
    wy=word(0x482)+(((regs['vscroll'][0]&1023)-word(0x482)+512)&1023)-512+1
    rgb=lambda index:tuple((((pal[index]>>s)&31)<<3)|(((pal[index]>>s)&31)>>2)for s in (0,5,10))
    def graphics(tile,x,y,base,bpp=4):
        sx=7-x if tile&0x4000 else x;sy=7-y if tile&0x8000 else y
        adr=(base+(tile&1023)*(8 if bpp==2 else 16)+sy)&32767
        planes=vram[adr]|(vram[(adr+8)&32767]<<16 if bpp==4 else 0)
        bit=7-sx
        return sum(((planes>>(bit+i*8))&1)<<i for i in range(bpp))
    def world(layer,x,y):
        if not(0<=x<word(0x53)*8 and 0<=y<word(0x55)*8):return (0,0)
        meta=byte(ptr(0x8b if layer else 0x88)+(y>>4)*128+(x>>4))
        tile=rw(ptr(0x75 if layer else 0x69)+meta*8+((x&8)>>2)+((y&8)>>1))
        if layer:tile&=0xdfff
        color=graphics(tile,x&7,y&7,0)
        return ((12 if tile&0x2000 else 8)-layer,((tile>>10)&7)*16+color) if color else (0,0)
    def decode(frame,x,y):
        table=rw(0x999d+3)|(byte(0x999f+3)<<16)
        q=(table&0xff0000)|rw(table+frame*2);dims,flags=byte(q),byte(q+1);q+=2;cells=[]
        for grid in range(2 if flags&128 else 1):
            if grid:dims=byte(q);q+=1
            step=16 if (flags^grid)&1 else 8
            assert dims>>4 and dims&15
            for yy in range(dims&15):
                for xx in range(dims>>4):
                    tile,attr=byte(q),byte(q+1);q+=2;dx=dy=0
                    if tile==255:
                        if attr==255:continue
                        tile=attr;attr,dx,dy=byte(q),byte(q+1),byte(q+2);q+=3
                        dx=dx-256 if dx&128 else dx;dy=dy-256 if dy&128 else dy
                    cells.append((x+xx*step+dx,y+yy*step+dy,tile|(attr<<8),step))
                    assert len(cells)<=64
        return cells
    slots=[]
    for slot in range(128):
        q=slot*4;high=(oam[512+slot//4]>>((slot%4)*2))&3
        x=oam[q]|((high&1)<<8);x=x-512 if x&256 else x
        slots.append((x,oam[q+1],int.from_bytes(oam[q+2:q+4],'little'),16 if high&2 else 8))
    margin_slots=set();actor_witnesses=[]
    for q in range(word(0x48c)):
        record=word(0xec0+q*6);typ=ram[0xec4+q*6]
        if record>0x800 or record&63:continue
        r=0x500+record;kind=ram[r+0x1b]
        if not(typ==1 and kind in (41,59) or typ==2 and kind in (0,1) and not ram[r+1]&7):continue
        offsets=0x1fb7f if typ==1 else 0x1f7e9
        cells=decode(ram[r+0x1c],signed(word(r+12))+signed(rw(offsets+kind*4)),signed(word(r+14))+signed(rw(offsets+kind*4+2)))
        if typ==2 and (ram[r+1]&128 or ram[r+0x1f]&2):cells=[(x,y,(t&~0xe00)|0xe00,n)for x,y,t,n in cells]
        matched=[]
        for cell in cells:
            if cell[1]<-cell[3] or cell[1]>=240:continue
            hits=[i for i,s in enumerate(slots)if s==(cell[0],cell[1]&255,cell[2],cell[3])]
            if len(hits)!=1:matched=[];break
            # All ordinary cells must use resident object character bytes;
            # dynamic player staging and stale parked objects cannot qualify.
            tile=cell[2];base=0x5000 if tile&0x100 else 0x4000
            owned=True
            for yy in range(0,cell[3],8):
                for xx in range(0,cell[3],8):
                    ch=((((tile&255)>>4)+(yy>>3))<<4)|(((tile&15)+(xx>>3))&15)
                    adr=((base+ch*16)&32767)*2
                    owned&=0x8800<=adr and adr+32<=0xb200
            if not owned:matched=[];break
            matched+=hits
        if matched:margin_slots.update(matched);actor_witnesses.append(dict(record=record,kind=kind,slots=matched))
    # The host's OBJ row is prepared one scanline ahead; captured output row0
    # corresponds to OAM Y0, while BG tiles sample scrollY+1.
    sprite=[None]*(a.width*224)
    # Lowest OAM slot owns each nontransparent object pixel, before BG priority.
    for slot,(x,y,tile,size) in enumerate(slots):
        for dy in range(size):
            out_y=(y+dy)&255
            if out_y>=224:continue
            sy=size-1-dy if tile&0x8000 else dy
            for dx in range(size):
                out_x=x+dx
                if not(0<=out_x<256) and slot not in margin_slots:continue
                ix=out_x+left
                if not(0<=ix<a.width):continue
                sx=size-1-dx if tile&0x4000 else dx
                ch=((((tile&255)>>4)+(sy>>3))<<4)|(((tile&15)+(sx>>3))&15)
                base=0x5000 if tile&0x100 else 0x4000
                color=graphics(ch,sx&7,sy&7,base)
                index=out_y*a.width+ix
                if color and sprite[index] is None:sprite[index]=(2+4*((tile>>12)&3),128+((tile>>9)&7)*16+color)
    pixels=[];bg_pixels=[]
    for y in range(224):
        for x in range(a.width):
            z,index=max(world(0,wx+x-left,wy+y),world(1,wx+x-left,wy+y));bg_pixels.append(rgb(index))
            obj=sprite[y*a.width+x]
            if obj and obj[0]>z:z,index=obj
            if left<=x<left+256:
                xx=x-left;yy=y+1
                tile=vram[0x3c00+(yy//8)*32+xx//8];color=graphics(tile,xx&7,yy&7,0x6000,2)
                priority=15 if tile&0x2000 else 1
                if color and priority>z:z,index=priority,((tile>>10)&7)*4+color
            pixels.append(rgb(index))
    out=Image.new('RGB',(a.width,224));out.putdata(pixels);bg=Image.new('RGB',out.size);bg.putdata(bg_pixels)
    native=Image.open(prefix+'.fb.bmp').convert('RGB')
    expected=out.crop((left,0,left+256,224));exact=sum(x==y for x,y in zip(native.getdata(),expected.getdata()))
    report=dict(experimental=True,mode='retained maps + uploaded OAM + native BG3',frame=regs['frame'],width=a.width,
                native_exact=exact,native_total=256*224,actor_witnesses=actor_witnesses,
                rom_sha256=hashlib.sha256(rom).hexdigest(),
                native_framebuffer_sha256=hashlib.sha256(raw('.fb.bmp')).hexdigest(),
                input_hashes={suffix:hashlib.sha256(raw(suffix)).hexdigest() for suffix in ['.wram.bin','.vram.bin','.oam.bin','.cgram.bin','.regs.json']},
                limits=['Atland room0 only','single stable snapshot; no raster or AI execution','far actor history unavailable; only queued/OAM-matched ordinary cells in margins','no color math, windows, HDMA, mosaic or unqualified layouts'])
    if a.reference:
        ref=Image.open(a.reference).convert('RGB');assert ref.size==out.size
        report['reference_sha256']=hashlib.sha256(a.reference.read_bytes()).hexdigest()
        report['expanded_exact']=sum(x==y for x,y in zip(ref.getdata(),out.getdata()));report['expanded_total']=a.width*224
    report['accepted']=report['native_exact']==report['native_total'] and ('expanded_exact' not in report or report['expanded_exact']==report['expanded_total'])
    a.out.mkdir(parents=True);out.save(a.out/'scene.png');bg.save(a.out/'scenery.png');(a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report))
    if not report['accepted']:raise SystemExit(1)


if __name__=='__main__':main()
