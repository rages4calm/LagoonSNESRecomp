"""Independently audit newly widened cave fades using private per-field snapshots."""
import argparse,csv,json,struct,sys
from pathlib import Path
from PIL import Image,ImageChops

def same(a,b):
    d=ImageChops.difference(a,b).split();return ImageChops.lighter(ImageChops.lighter(d[0],d[1]),d[2]).histogram()[0]

def main():
    if sys.flags.optimize:raise SystemExit('Run without -O: strict audit guards must remain enabled')
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ['run','baseline','rom','out']:p.add_argument('--'+arg,required=True,type=Path)
    a=p.parse_args();rom=a.rom.read_bytes();rows=list(csv.DictReader((a.run/'presents.csv').open()));old={r['frame']:r for r in csv.DictReader((a.baseline/'presents.csv').open())}
    snapshots={json.loads(f.read_text())['frame']:f.name[:-10] for f in (a.run/'dumps').glob('*.info.json')}
    report=dict(presents=len(rows),native_center_mismatches=[],newly_wide=[],checked=0,exact=0,actor_pixel_observations=0)
    image=lambda directory,row:Image.open(directory/'frames'/('present_%06d.ppm'%int(row['present']))).convert('RGB')
    cache={}
    for row in rows:
        frame=int(row['frame']);im=image(a.run,row);before=image(a.baseline,old[row['frame']]);e=(im.width-256)//2
        if im.crop((e,0,e+256,224)).tobytes()!=before.crop((e,0,e+256,224)).tobytes():report['native_center_mismatches'].append(frame)
        margins=((0,0,e,224),(e+256,0,im.width,224))
        if any(before.crop(box).getbbox()for box in margins) or not any(im.crop(box).getbbox()for box in margins):continue
        assert frame in snapshots, 'Every newly visible field requires an actual source snapshot'
        prefix=a.run/'dumps'/snapshots[frame];read=lambda suffix:Path(str(prefix)+suffix).read_bytes()
        ram=read('.wram.bin');vr=read('.vram.bin');cg=read('.cgram.bin');regs=json.loads(read('.regs.json'))
        word=lambda q:int.from_bytes(ram[q:q+2],'little');ptr=lambda q:int.from_bytes(ram[q:q+3],'little')
        assert word(0x48e)in (3,4,5,6) and ptr(0x57)==0x128b77 and ptr(0x69)==ptr(0x75)==0x069298
        assert regs['cgadsub']==0 and regs['hscroll'][0]==regs['hscroll'][1] and regs['vscroll'][0]==regs['vscroll'][1]
        cx,cy=word(0x480),word(0x482);wx=cx+(((regs['hscroll'][0]&1023)-cx+512)&1023)-512;wy=cy+(((regs['vscroll'][0]&1023)-cy+512)&1023)-512+1
        oam=read('.oam.bin')
        actor_key=[]
        for q in range(word(0x48c)):
            off=word(0xec0+q*6)
            if off>0x800 or off&63:continue
            r=0x500+off
            actor_key.append((off,ram[0xec4+q*6],ram[r+1],ram[r+0x18],ram[r+0x1b],
                ram[r+0x1c],ram[r+0x1f],word(r+12),word(r+14)))
        key=(tuple(actor_key),oam,ram[0x52:0x90],ram[65536:65536+32768],vr[:0x37c0],vr[0x7600:0x76c0],cg,wx,wy)
        if key not in cache:
            vram=struct.unpack('<32768H',vr);pal=struct.unpack('<256H',cg)
            def byte(addr):
                bank,off=addr>>16,addr&65535
                if bank in (126,127):return ram[(bank-126)*65536+off]
                assert (bank&127)<32 and off>=32768
                return rom[(bank&31)*32768+(off&32767)]
            def pixel(layer,x,y):
                meta=byte(ptr(0x8b if layer else 0x88)+(y>>4)*128+(x>>4));q=ptr(0x75 if layer else 0x69)+meta*8+((x&8)>>2)+((y&8)>>1);tile=byte(q)|(byte(q+1)<<8)
                if layer:tile&=0xdfff
                sx=7-(x&7)if tile&0x4000 else x&7;sy=7-(y&7)if tile&0x8000 else y&7
                q=(tile&1023)*16+sy;planes=vram[q]|(vram[q+8]<<16);bit=7-sx;color=sum(((planes>>(bit+i*8))&1)<<i for i in range(4))
                return ((12 if tile&0x2000 else 8)-layer,((tile>>10)&7)*16+color)if color else (0,0)
            colors=[]
            for y in range(224):
                for x in range(im.width):
                    xx,yy=wx+x-e,wy+y
                    if not(0<=xx<word(0x53)*8 and 0<=yy<word(0x55)*8):colors.append((0,0,0));continue
                    index=max(pixel(0,xx,yy),pixel(1,xx,yy))[1];rgb=pal[index];colors.append(tuple((((rgb>>s)&31)<<3)|(((rgb>>s)&31)>>2)for s in (0,5,10)))
            # Ordinary queued cells must decode from the real ROM frame and
            # match complete uploaded OAM records before their pixels qualify.
            # Only resident object graphics are accepted; no stale parked OAM.
            signed=lambda v:v-65536 if v&32768 else v
            rw=lambda q:byte(q)|(byte(q+1)<<8)
            slots=[]
            for slot in range(128):
                q=slot*4;hi=(oam[512+slot//4]>>((slot%4)*2))&3
                slots.append((oam[q]|((hi&1)<<8),oam[q+1],int.from_bytes(oam[q+2:q+4],'little'),16 if hi&2 else 8))
            cells={}
            for q in range(word(0x48c)):
                off=word(0xec0+q*6);typ=ram[0xec4+q*6]
                if off>0x800 or off&63 or typ!=2:continue
                r=0x500+off;kind=ram[r+0x1b]
                if kind>1 or ram[r+1]&7:continue
                ox,oy=signed(rw(0x1f7e9+kind*4)),signed(rw(0x1f7eb+kind*4))
                xx,yy=signed(word(r+12))+ox,signed(word(r+14))+oy
                table=rw(0x999d+(word(0x48e)+1)*3)|(byte(0x999f+(word(0x48e)+1)*3)<<16)
                q=(table&0xff0000)|rw(table+ram[r+0x1c]*2);dims,flags=byte(q),byte(q+1);q+=2;decoded=[]
                for grid in range(2 if flags&128 else 1):
                    if grid:dims=byte(q);q+=1
                    step=16 if (flags^grid)&1 else 8
                    for dy in range(dims&15):
                        for dx in range(dims>>4):
                            tile,attr=byte(q),byte(q+1);q+=2;ex=ey=0
                            if tile==255:
                                if attr==255:continue
                                tile=attr;attr,ex,ey=byte(q),byte(q+1),byte(q+2);q+=3
                                ex=ex-256 if ex&128 else ex;ey=ey-256 if ey&128 else ey
                            t=tile|(attr<<8)
                            if ram[r+1]&128 or ram[r+0x1f]&2:t=(t&~0xe00)|0xe00
                            decoded.append((xx+dx*step+ex,yy+dy*step+ey,t,step))
                pending=[]
                for x,y,t,n in decoded:
                    if y< -n or y>=240:continue
                    hits=[i for i,c in enumerate(slots)if c==(x&511,y&255,t,n)]
                    if len(hits)!=1:pending=[];break
                    pending.append((hits[0],(x,y,t,n)))
                cells.update(pending)
            sprite={}
            for slot,(x,y,t,n)in sorted(cells.items()):
                base=0x5000 if t&0x100 else 0x4000
                for dy in range(n):
                    yy=(y+dy)&255
                    if yy>=224:continue
                    sy=n-1-dy if t&0x8000 else dy
                    for dx in range(n):
                        xx=x+dx+e
                        if not(0<=xx<im.width)or e<=xx<e+256:continue
                        sx=n-1-dx if t&0x4000 else dx
                        tile=((((t&255)>>4)+(sy>>3))<<4)|(((t&15)+(sx>>3))&15)
                        adr=base+tile*16
                        assert 0x4400<=adr and adr+16<=0x5900
                        adr+=sy&7;planes=vram[adr]|(vram[adr+8]<<16);bit=7-(sx&7)
                        color=sum(((planes>>(bit+i*8))&1)<<i for i in range(4))
                        if not color or (xx,yy)in sprite:continue
                        sprite[xx,yy]=(2+4*((t>>12)&3),128+((t>>9)&7)*16+color)
            actor_pixels=0
            for (x,y),(z,index)in sprite.items():
                bg=max(pixel(0,wx+x-e,wy+y),pixel(1,wx+x-e,wy+y))[0]
                if z<=bg:continue
                rgb=pal[index];colors[y*im.width+x]=tuple((((rgb>>s)&31)<<3)|(((rgb>>s)&31)>>2)for s in (0,5,10));actor_pixels+=1
            base=Image.new('RGB',im.size);base.putdata(colors);cache[key]=([base.point([i*b//15 for i in range(256)]*3)for b in range(16)],actor_pixels)
        fades,actor_pixels=cache[key];native=im.crop((e,0,e+256,184));scores=[same(f.crop((e,0,e+256,184)),native)for f in fades];b=scores.index(max(scores));assert scores.count(scores[b])==1 and scores[b]>256*184*0.9
        count=exact=0
        for box in ((0,0,e,224),(e+256,0,im.width,224)):
            count+=(box[2]-box[0])*224;exact+=same(im.crop(box),fades[b].crop(box))
        report['newly_wide'].append(dict(frame=frame,brightness=b,checked=count,exact=exact));report['checked']+=count;report['exact']+=exact;report['actor_pixel_observations']+=actor_pixels
    a.out.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:len(v)if isinstance(v,list)else v for k,v in report.items()}))
    assert report['checked'] and report['checked']==report['exact'] and not report['native_center_mismatches']
if __name__=='__main__':main()
