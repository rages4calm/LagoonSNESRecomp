"""Audit actual outdoor presentation bounds, native center and source colors.

All ROM/dump/capture inputs remain private. A background source witness may be
occluded by an actor; it is reported separately from strict bounds/center tests.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--run',required=True,type=Path)
    p.add_argument('--bounds',required=True,type=Path)
    p.add_argument('--control',type=Path)
    p.add_argument('--rom',type=Path)
    p.add_argument('--texture-tag',action='append',default=[])
    p.add_argument('--fallback-range',type=int,nargs=2,action='append',default=[],metavar=('FIRST','LAST'),help='Require native fallback throughout a known resource transition')
    p.add_argument('--out',required=True,type=Path)
    a=p.parse_args()
    presents=list(csv.DictReader((a.run/'presents.csv').open()))
    bounds={int(r['frame']):{k:int(v)for k,v in r.items()}for r in csv.DictReader(a.bounds.open(),delimiter='\t')}
    controls=list(csv.DictReader((a.control/'presents.csv').open())) if a.control else None
    if controls:assert [r['frame']for r in presents]==[r['frame']for r in controls]
    result={'presents':len(presents),'masked_frames':0,'fallback_frames':0,'outside_checked':0,'outside_nonblack':0,'inside_nonblack':0,'native_center_mismatches':[],'errors':[],'texture':[]}
    frame_images={}
    for i,r in enumerate(presents):
        frame=int(r['frame']);im=Image.open(a.run/'frames'/('present_%06d.ppm'%int(r['present']))).convert('RGB');e=(im.width-256)//2
        frame_images[frame]=r
        if controls:
            c=controls[i];ci=Image.open(a.control/'frames'/('present_%06d.ppm'%int(c['present']))).convert('RGB');ce=(ci.width-256)//2
            if im.crop((e,0,e+256,224)).tobytes()!=ci.crop((ce,0,ce+256,224)).tobytes():result['native_center_mismatches'].append(frame)
        b=bounds.get(frame)
        if any(first<=frame<=last for first,last in a.fallback_range):
            assert not b or not b['mask'], 'Unexpected room expansion inside required fallback interval'
            b={'fallback':1,'mask':0}
        if not b:continue
        if b['mask']:
            assert b['known']==224 and b['xchanges']==b['ychanges']==0
            result['masked_frames']+=1
        elif b['fallback']:result['fallback_frames']+=1
        else:continue
        def nonblack(box):
            x0,y0,x1,y1=box
            if x1<=x0 or y1<=y0:return 0
            area=(x1-x0)*(y1-y0)
            colors=im.crop(box).getcolors(maxcolors=area)
            return area-next((n for n,c in colors if c==(0,0,0)),0)
        for mx0,mx1 in [(0,e),(e+256,im.width)]:
            inside=(0,0,0,0)
            if b['mask']:
                inside=(max(mx0,e-b['x0']),max(0,-b['y0']),
                        min(mx1,e-b['x0']+b['width']),min(im.height,-b['y0']+b['height']))
            ix0,iy0,ix1,iy1=inside
            inside_area=max(0,ix1-ix0)*max(0,iy1-iy0)
            inside_nonblack=nonblack(inside)
            outside_nonblack=nonblack((mx0,0,mx1,im.height))-inside_nonblack
            result['outside_checked']+=(mx1-mx0)*im.height-inside_area
            result['outside_nonblack']+=outside_nonblack
            result['inside_nonblack']+=inside_nonblack
            if outside_nonblack and len(result['errors'])<20:
                result['errors'].append([frame,mx0,mx1,outside_nonblack])
    for tag in a.texture_tag:
        assert a.rom
        prefix=a.run/'dumps'/tag
        read=lambda suffix:Path(str(prefix)+suffix).read_bytes()
        ram=read('.wram.bin');vram=struct.unpack('<32768H',read('.vram.bin'));pal=struct.unpack('<256H',read('.cgram.bin'));regs=json.loads(read('.regs.json'));rom=a.rom.read_bytes()
        assert regs['inidisp']==15 and regs['cgadsub']==0 and regs['tm']&3==3
        b=bounds[regs['frame']];assert b['mask'] and b['xchanges']==b['ychanges']==0
        entry=frame_images[regs['frame']];im=Image.open(a.run/'frames'/('present_%06d.ppm'%int(entry['present']))).convert('RGB');e=(im.width-256)//2
        ptr=lambda q:int.from_bytes(ram[q:q+3],'little')
        def byte(addr):
            bank=addr>>16;off=addr&65535
            if bank in (126,127):return ram[(bank-126)*65536+off]
            assert (bank&127)<32 and off>=32768
            return rom[(bank&31)*32768+(off&32767)]
        def pixel(layer,wx,wy):
            meta=byte(ptr(0x8b if layer else 0x88)+(wy>>4)*128+(wx>>4))
            q=ptr(0x75 if layer else 0x69)+meta*8+((wx&8)>>2)+((wy&8)>>1)
            tile=byte(q)|byte(q+1)<<8
            if layer:tile&=0xdfff
            sx=7-(wx&7) if tile&0x4000 else wx&7;sy=7-(wy&7) if tile&0x8000 else wy&7
            q=(((regs['bg_tile_adr']>>(layer*4))&15)<<12)+(tile&1023)*16+sy;q&=32767
            planes=vram[q]|vram[(q+8)&32767]<<16;bit=7-sx
            color=((planes>>bit)&1)|((planes>>(bit+7))&2)|((planes>>(bit+14))&4)|((planes>>(bit+21))&8)
            return ((12 if tile&0x2000 else 8)-layer,((tile>>10)&7)*16+color) if color else (0,0)
        other_pixels=[]
        report={'tag':tag,'frame':regs['frame'],'inside_margin_pixels':0,'exact_source_colors':0,'first_other_pixels':[]}
        for y in range(im.height):
            for x in list(range(e))+list(range(e+256,im.width)):
                wx=b['x0']+x-e;wy=b['y0']+y
                if not(0<=wx<b['width'] and 0<=wy<b['height']):continue
                z,index=max(pixel(0,wx,wy),pixel(1,wx,wy))
                rgb15=pal[index];rgb=tuple((((rgb15>>s)&31)<<3)|(((rgb15>>s)&31)>>2)for s in [0,5,10])
                report['inside_margin_pixels']+=1
                if im.getpixel((x,y))==rgb:report['exact_source_colors']+=1
                else:
                    other_pixels.append([x-e,y])
                    if len(report['first_other_pixels'])<10:report['first_other_pixels'].append([x,y])
        report['other_pixel_coordinates_sha256']=hashlib.sha256(json.dumps(other_pixels,separators=(',',':')).encode()).hexdigest()
        result['texture'].append(report)
    a.out.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    if result['outside_nonblack'] or result['native_center_mismatches']:raise SystemExit(1)


if __name__=='__main__':main()
