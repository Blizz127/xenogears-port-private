#!/usr/bin/env python3
"""Original text-designed Fei pixel clusters; no input bitmaps or retail pixels.
Only traced geometry defines output sizes. Standard-library PNG writer.
"""
import json
import struct
import zlib
from pathlib import Path

LABEL='fan-made, AI-generated replacement sprite art created for this project; not from the game; characters © Square Enix'
PALETTE=[(20,19,25),(42,29,28),(65,39,29),(92,55,34),(122,77,43),(155,106,60),
         (92,51,40),(139,76,49),(184,111,65),(218,155,99),(241,186,125),(255,212,155),
         (76,82,86),(113,119,120),(156,160,155),(193,194,177),(226,223,197),(250,244,218),
         (16,43,44),(24,69,64),(35,96,80),(58,121,92),(88,147,106),(135,173,126)]
# Each list is a deliberately authored polygon in a new 60x120 design canvas.
# Palette indices, not sampled image data. Hard pixel centers; no antialiasing.
def shapes(facing='front',phase=0):
    if facing in ('left','right'):return profile_shapes(facing,phase)
    out=[]
    def poly(c,*points):out.append((c,points))
    # Ponytail behind head/body. Compact silhouette, separated flat clusters.
    poly(0,(31,10),(42,14),(46,31),(43,55),(47,70),(39,66),(35,46))
    poly(2,(35,17),(41,21),(41,44),(44,61),(40,58),(38,36))
    poly(3,(39,23),(41,28),(40,46),(39,41))
    # Legs/boots, authored phase modifies foot placements, never runtime timing.
    step=(0,-3,0,3)[phase]
    poly(0,(17,74),(44,74),(44,89),(40+step,109),(29,110),(27,92),(24-step,110),(13,110),(14,90))
    poly(19,(19,76),(29,78),(27,92),(23-step,106),(15,106),(17,88))
    poly(20,(19,81),(25,80),(23,96),(19,103),(16,103))
    poly(21,(20,80),(23,80),(21,94),(19,96))
    poly(18,(29,77),(42,77),(41,89),(37+step,107),(29,107),(29,90))
    poly(20,(32,80),(40,80),(38,94),(35+step,103),(31,102))
    poly(22,(34,80),(37,80),(35,91),(33,94))
    poly(23,(34,81),(35,81),(34,86),(33,87))
    for x,shift in ((14,-step),(30,step)):
        poly(0,(x+shift,105),(x+10+shift,105),(x+11+shift,117),(x-4+shift,117),(x-4+shift,113),(x+shift,110))
        poly(2,(x+1+shift,107),(x+8+shift,107),(x+9+shift,114),(x-2+shift,114),(x+1+shift,111))
        poly(4,(x+2+shift,107),(x+7+shift,107),(x+7+shift,110),(x+2+shift,110))
        poly(3,(x-1+shift,113),(x+8+shift,113),(x+9+shift,115),(x-2+shift,115))
        poly(5,(x+2+shift,112),(x+5+shift,112),(x+5+shift,113),(x+2+shift,113))
    # Torso, bare arms and compact wrists/hands.
    poly(0,(20,38),(39,38),(43,47),(41,72),(16,72),(16,47))
    poly(15,(22,40),(29,43),(37,40),(40,48),(38,68),(19,68),(19,48))
    poly(17,(23,41),(29,46),(36,42),(38,47),(35,59),(21,62),(21,48))
    poly(16,(21,60),(35,56),(35,64),(20,67))
    poly(14,(20,48),(23,46),(23,53),(21,59),(19,64))
    poly(13,(35,49),(37,47),(37,61),(35,66),(33,66))
    poly(12,(29,46),(31,47),(24,60),(23,60))
    poly(0,(16,42),(20,46),(19,57),(15,71),(10,70),(11,57),(11,48))
    poly(8,(16,46),(18,48),(16,57),(13,64),(11,63),(13,52))
    poly(10,(15,47),(17,49),(15,55),(13,56),(13,52))
    poly(9,(13,58),(15,56),(14,64),(12,65))
    poly(0,(39,42),(44,47),(47,59),(46,70),(41,71),(40,57))
    poly(8,(41,47),(43,48),(45,59),(44,65),(42,63),(42,56))
    poly(10,(42,49),(43,51),(44,57),(42,56))
    for x in (10,41):
        poly(0,(x,63),(x+6,64),(x+6,73),(x+4,77),(x,75))
        poly(12,(x+1,64),(x+5,65),(x+5,68),(x+1,68))
        poly(13,(x+2,65),(x+4,65),(x+4,66),(x+2,66))
        poly(7,(x+1,70),(x+5,70),(x+4,74),(x+1,73))
        poly(9,(x+1,71),(x+3,71),(x+3,73),(x+1,72))
    poly(0,(17,69),(40,69),(41,75),(17,75))
    poly(3,(18,70),(39,70),(39,73),(18,73))
    poly(5,(28,70),(31,70),(31,73),(28,73))
    # Neck/head with polygonal original hairline.
    poly(0,(25,31),(34,31),(34,43),(30,47),(24,42))
    poly(8,(26,33),(33,33),(32,41),(29,43),(25,40))
    poly(10,(26,35),(30,35),(30,41),(27,40))
    poly(0,(19,14),(26,9),(35,11),(40,18),(39,31),(34,39),(26,39),(20,33))
    poly(8,(22,18),(29,14),(35,17),(37,24),(35,33),(31,36),(26,35),(22,29))
    poly(10,(24,18),(29,16),(34,18),(35,26),(32,32),(26,31),(23,26))
    poly(11,(25,20),(28,18),(30,19),(27,22),(24,23))
    poly(9,(31,29),(34,28),(33,33),(29,34),(26,32))
    poly(7,(28,34),(31,34),(31,35),(28,35))
    if facing=='back':
        poly(0,(20,14),(29,9),(37,13),(40,23),(36,38),(25,36),(19,26))
        poly(2,(23,15),(29,12),(35,15),(37,24),(34,33),(26,32),(22,25))
        poly(3,(26,16),(29,14),(29,30),(26,27))
        poly(4,(31,17),(34,19),(34,25),(32,24))
        poly(0,(29,24),(34,25),(37,47),(35,61),(30,56),(29,39))
        poly(2,(31,29),(33,31),(34,48),(33,54),(31,49))
    else:
        poly(0,(19,14),(24,8),(33,8),(39,13),(41,22),(37,29),(36,21),(33,17),(30,22),(29,17),(24,22),(21,24))
        poly(2,(21,15),(26,10),(33,11),(37,15),(38,20),(35,22),(32,15),(29,18),(27,14),(23,20),(21,21))
        poly(3,(24,14),(27,11),(29,12),(25,17),(23,19))
        poly(4,(32,12),(34,13),(35,16),(33,15))
        poly(0,(24,25),(28,24),(28,27),(24,27))
        poly(0,(32,24),(35,24),(35,27),(32,27))
        poly(17,(25,25),(26,25),(26,26),(25,26))
        poly(6,(29,27),(31,29),(29,30))
    poly(1,(19,18),(21,18),(21,25),(20,27),(19,24))
    return out

def profile_shapes(facing,phase):
    out=[]
    def poly(c,*points):
        if facing=='right':points=tuple((60-x,y) for x,y in points)
        out.append((c,points))
    step=(0,-3,0,3)[phase]
    poly(0,(32,12),(41,17),(43,34),(40,60),(45,71),(38,68),(33,45))
    poly(2,(35,18),(39,23),(39,43),(41,61),(38,58),(36,38))
    poly(3,(38,27),(39,31),(38,46),(37,39))
    poly(0,(20,73),(38,73),(38,91),(33+step,109),(24+step,109),(26,89),(21-step,110),(12-step,110),(17,91))
    poly(19,(21,76),(28,76),(25,91),(19-step,106),(14-step,106),(19,90))
    poly(21,(22,77),(25,77),(22,91),(19-step,98),(18-step,98))
    poly(18,(29,76),(36,76),(34,92),(30+step,106),(25+step,106),(28,88))
    poly(20,(30,78),(34,78),(32,92),(29+step,103),(27+step,103))
    poly(22,(31,80),(32,80),(30,91),(29,92))
    for x,shift in ((13,-step),(25,step)):
        poly(0,(x+shift,105),(x+8+shift,105),(x+8+shift,116),(x-5+shift,116),(x-5+shift,113),(x+shift,110))
        poly(2,(x+1+shift,107),(x+6+shift,107),(x+6+shift,113),(x-3+shift,113),(x+1+shift,111))
        poly(4,(x+2+shift,108),(x+5+shift,108),(x+5+shift,110),(x+2+shift,110))
        poly(3,(x-2+shift,114),(x+6+shift,114),(x+6+shift,115),(x-3+shift,115))
    poly(0,(25,39),(35,41),(39,52),(36,72),(18,73),(20,50))
    poly(15,(26,42),(32,43),(36,52),(33,68),(21,69),(23,50))
    poly(17,(26,44),(30,45),(32,53),(28,64),(23,65),(24,52))
    poly(16,(22,65),(31,60),(31,67),(21,69))
    poly(13,(33,53),(34,56),(32,66),(31,65))
    poly(0,(26,43),(32,44),(35,53),(35,66),(31,74),(25,72),(28,63),(28,54),(23,49))
    poly(8,(26,46),(30,47),(32,54),(32,63),(29,68),(27,67),(29,58),(27,52))
    poly(10,(26,47),(28,48),(30,53),(29,55),(27,51))
    poly(9,(30,57),(32,57),(31,62),(30,62))
    poly(0,(26,65),(32,66),(32,72),(30,76),(25,74))
    poly(12,(27,66),(31,67),(30,70),(26,69))
    poly(7,(26,71),(30,71),(29,74),(26,73))
    poly(10,(27,71),(29,71),(28,73),(27,72))
    poly(0,(19,69),(35,68),(36,75),(19,76))
    poly(3,(20,70),(34,70),(34,73),(20,74))
    poly(5,(23,70),(25,70),(25,73),(23,73))
    poly(0,(26,30),(33,31),(32,42),(27,45),(23,41))
    poly(8,(27,33),(31,34),(30,40),(27,42),(25,39))
    poly(0,(21,14),(30,10),(36,16),(36,28),(30,37),(24,36),(20,31),(17,29),(20,25))
    poly(8,(23,18),(29,15),(33,19),(32,29),(28,34),(23,32),(20,29),(22,26))
    poly(10,(23,20),(27,18),(29,21),(28,27),(25,31),(22,29))
    poly(11,(23,21),(25,20),(25,22),(22,24))
    poly(9,(25,32),(29,30),(29,33),(26,34))
    poly(0,(23,25),(27,24),(27,27),(23,27))
    poly(17,(24,25),(25,25),(25,26),(24,26))
    poly(6,(20,30),(23,30),(23,31),(20,31))
    poly(0,(20,15),(25,9),(32,9),(37,15),(38,26),(34,31),(31,27),(32,18),(27,15),(23,21),(21,24),(19,21))
    poly(2,(22,15),(26,11),(31,12),(34,16),(35,24),(33,26),(30,16),(27,13),(23,19))
    poly(3,(24,14),(26,12),(28,12),(25,17),(23,19))
    poly(4,(32,14),(33,16),(33,20),(32,18))
    poly(1,(35,19),(36,21),(35,26),(34,26))
    return out

def inside(x,y,points):
    yes=False
    j=len(points)-1
    for i,(xi,yi) in enumerate(points):
        xj,yj=points[j]
        if (yi>y)!=(yj>y) and x<(xj-xi)*(y-yi)/(yj-yi)+xi:yes=not yes
        j=i
    return yes

def raster(width,height,facing,phase,domain=(0,0,60,120),fill_geometry=False,left_envelope_pixels=0,right_envelope_pixels=0,canvas_right_pixels=0):
    pixels=bytearray(width*height*4); polygons=shapes(facing,phase)
    for y in range(height):
        # The original part design occupies a smaller authored canvas when
        # requested, while the transport cell retains its fixed integer grid.
        for x in range(width-canvas_right_pixels):
            authored_x=(x+.5-left_envelope_pixels)*width/(width-right_envelope_pixels)
            sx=domain[0]+authored_x*domain[2]/width
            sy=domain[1]+(y+.5)*domain[3]/height
            if fill_geometry:
                # Original shape re-authoring: expand own design bounds10..47,
                # y8..117 to the geometry-only domain. Never reads an alpha mask.
                sx=10+sx*37/60
                sy=8+sy*109/120
            if phase==2 and sy>78:
                sx=30+(sx-30)*1.12
            color=None
            for c,points in polygons:
                if inside(sx,sy,points):color=c
            if color is not None:
                offset=(y*width+x)*4;pixels[offset:offset+4]=bytes((*PALETTE[color],255))
    return bytes(pixels)

def png(path,width,height,rgba):
    def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data)&0xffffffff)
    rows=b''.join(b'\0'+rgba[y*width*4:(y+1)*width*4] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))

def stats(raw):
    colors={tuple(raw[i:i+3]) for i in range(0,len(raw),4) if raw[i+3]}
    return dict(opaque_colors=len(colors),alpha_values=sorted(set(raw[3::4])),opaque_pixels=sum(a==255 for a in raw[3::4]))

# Geometry-only traced frame190/variant0. No source images or alpha masks.
FRONT_PARTS=[(0,24,12,(159,99,10,5)),(1,16,12,(159,94,7,5)),
             (2,16,16,(159,87,7,7)),(3,56,26,(148,104,23,11)),
             (4,40,36,(151,115,17,15)),(5,24,40,(150,87,10,17))]
def main():
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);p.add_argument('--version',choices=('v3','v3b','v3c','v3d','v3e'),default='v3c');args=p.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    manifest=dict(schema=1,kind='original_pixel_parts',label=LABEL,palette=PALETTE,
                  authoring='Original code-authored polygons and pixel clusters from text; no bitmap input, sampling, resizing, quantization or retail pixels',
                  production_status='draft_pending_visual_and_strict_footprint_gates',parts=[],authored_poses=[],verified_runtime_poses=['frame190/variant0 geometry only'])
    for part,w,h,q in FRONT_PARTS:
        # New original full-character design projected through geometry-only domains.
        domain=((q[0]-148)*60/23,(q[1]-87)*120/43,q[2]*60/23,q[3]*120/43)
        raw=raster(w,h,'front',0,domain,fill_geometry=args.version!='v3',left_envelope_pixels=2 if args.version in ('v3c','v3d','v3e') and part==5 else 0,right_envelope_pixels=2 if args.version in ('v3d','v3e') and part==2 else 0,canvas_right_pixels=2 if args.version=='v3e' and part==2 else 0);name=f'front190-part{part}'
        png(args.output/(name+'.png'),w,h,raw);(args.output/(name+'.rgba')).write_bytes(raw)
        manifest['parts'].append(dict(part=part,png=name+'.png',rgba=name+'.rgba',retail_texel_grid=[w//2,h//2],exact_2x_grid=[w,h],screen_quad_domain=list(q),**stats(raw)))
    atlas=bytearray(240*480*4)
    for row,facing in enumerate(('front','back','left','right')):
        for phase,label in enumerate(('idle','left_step','passing','right_step')):
            raw=raster(60,120,facing,phase,fill_geometry=args.version!='v3');name=f'{facing}-{label}'
            png(args.output/(name+'.png'),60,120,raw)
            manifest['authored_poses'].append(dict(name=name,png=name+'.png',width=60,height=120,runtime_key=None,status='authored only; gait and facing mapping unverified',**stats(raw)))
            for y in range(120):
                dst=((row*120+y)*240+phase*60)*4
                atlas[dst:dst+240]=raw[y*240:(y+1)*240]
    png(args.output/'fei-v3-authored-16poses.png',240,480,atlas)
    (args.output/'fei-v3-authored-16poses.rgba').write_bytes(atlas)
    manifest['aggregate_pose_palette']=stats(atlas)
    (args.output/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    pack_front(args.output,args.version)
    print(json.dumps(dict(front_parts=manifest['parts'],aggregate_pose_palette=manifest['aggregate_pose_palette']),indent=2))
def pack_front(output,version='v3c'):
    width,height=96,128
    packed=bytearray(width*height*4)
    placements=[(0,0),(24,0),(40,0),(0,16),(0,42),(56,0)]
    uv=[([4,15,4,15],[55,55,60,60]),([20,27,20,27],[43,43,48,48]),
        ([20,27,20,27],[34,34,41,41]),([4,31,4,31],[1,1,13,13]),
        ([4,23,4,23],[15,15,32,32]),([4,15,4,15],[34,34,53,53])]
    cells=[]
    mappings=[]
    for part,w,h,q in FRONT_PARTS:
        x,y=placements[part]
        domain=((q[0]-148)*60/23,(q[1]-87)*120/43,q[2]*60/23,q[3]*120/43)
        raw=raster(w,h,'front',0,domain,fill_geometry=version!='v3',left_envelope_pixels=2 if version in ('v3c','v3d','v3e') and part==5 else 0,right_envelope_pixels=2 if version in ('v3d','v3e') and part==2 else 0,canvas_right_pixels=2 if version=='v3e' and part==2 else 0)
        for row in range(h):
            dst=((y+row)*width+x)*4
            packed[dst:dst+w*4]=raw[row*w*4:(row+1)*w*4]
        cells.append(dict(part=part,x=x,y=y,width=w,height=h,retail_texel_grid=[w//2,h//2],**stats(raw)))
        for map_id in (14,1):
            mappings.append(dict(identity=3682402565,map=map_id,actor=1,frame=190,part=part,tpage=26,clut=14353,
                                 u=uv[part][0],v=uv[part][1],rect=[x,y,w,h],
                                 trace='Geometry-only front190 from existing opening profile; ORIGINAL pixel-cluster art; strict footprint gate pending'))
    rgba_path=f'pc_port/assets/hd2d/fei-octopath-{version}-frontparts.rgba'
    png(output/f'fei-octopath-{version}-frontparts.png',width,height,packed)
    (output/f'fei-octopath-{version}-frontparts.rgba').write_bytes(packed)
    asset_path=Path(rgba_path)
    asset_path.parent.mkdir(parents=True,exist_ok=True)
    asset_path.write_bytes(packed)
    png(asset_path.with_suffix('.png'),width,height,packed)
    profile=dict(schema=1,label=LABEL,width=width,height=height,rgba_path=rgba_path,parts=mappings,
                 production_status='PRIVATE DRAFT; unmapped production; strict foot/geometry gating pending',
                 original_authoring_source='pc_port/tools/author_fei_pixel_v3.py',cells=cells,pack_palette=stats(packed))
    (output/'frontparts-profile.draft.json').write_text(json.dumps(profile,ensure_ascii=False,indent=2)+'\n')
    private=output/'private';private.mkdir(exist_ok=True)
    rows=[]
    for m in mappings:
        nums=[m[k] for k in ('identity','map','actor','frame','part','tpage','clut')]
        row='    {'+', '.join(map(str,nums))+', {'+', '.join(map(str,m['u']))+'}, {'+', '.join(map(str,m['v']))+'}, '+', '.join(map(str,m['rect']))+'}'
        rows.append(row)
    source='/* '+LABEL+' */\n#include "hd2d_sprite_parts.h"\nstatic const PcPortHd2dPartMapping hd2d_parts[] = {\n'+',\n'.join(rows)+'\n};\n'
    source+=f'static const PcPortHd2dPartProfile hd2d_sprite_profile = {{{width}, {height}, hd2d_parts, {len(mappings)}, "{rgba_path}"}};\n'
    (private/'hd2d_sprite_profile.h').write_text(source,encoding='utf-8')

if __name__=='__main__':main()
