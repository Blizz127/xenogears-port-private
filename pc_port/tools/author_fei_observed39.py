#!/usr/bin/env python3
"""Original vector-cluster authoring for observed geometry variants only.
No bitmap/mask inputs. Private provisional association; no production approval.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
from author_fei_pixel_v3 import LABEL, PALETTE, raster, png, stats, shapes, inside

FACING_BY_ANGLE={0:'right',512:'front_right',1024:'front',1536:'front_left',
                 2048:'left',2560:'back_left',3072:'back',3584:'back_right'}
def association(variant):
    states=variant.get('states',[])
    if not states:
        return 'front',0,'No joined animation state; semantic approval pending',None
    chosen=max(states,key=lambda state:len(state.get('presents',[])))
    angle=int(chosen['angle']) & 4095
    if angle not in FACING_BY_ANGLE:
        return 'front',0,'Non-eight-way observed angle; dedicated art orientation pending',chosen
    facing=FACING_BY_ANGLE[angle]
    # Neutral authored source is projected through actual geometry. It is NOT
    # labeled a proven gait phase, and no frame-number arithmetic selects one.
    return facing,0,'Observed dominant-state direction; no exact gait phase proven; transition states retained',chosen

def walking_profile_raster(width,height,facing,phase,domain,running=False,progress=None,vector_only=False):
    # Original forward arm and trailing calf/boot clusters, authored from text.
    # They reach geometry regions absent in the first profile design. No masks.
    additions=[
        (0,((27,44),(24,50),(17,57),(10,61),(9,70),(15,75),(21,71),(21,65),(29,56))),
        (8,((25,48),(22,54),(16,60),(12,63),(12,68),(16,69),(19,65),(26,56))),
        (10,((23,51),(21,55),(15,61),(14,64),(16,64),(23,56))),
        (12,((10,65),(18,65),(19,69),(15,72),(10,70))),
        (7,((11,70),(17,71),(17,74),(12,74))),
        (9,((12,71),(15,72),(15,73),(12,73))),
        (0,((32,78),(37,82),(41,93),(46,101),(47,108),(42,113),(36,108),(35,99),(30,89))),
        (19,((33,82),(36,84),(39,95),(44,102),(43,106),(39,105),(36,97),(32,89))),
        (21,((34,84),(35,85),(37,94),(39,98),(37,98),(34,91))),
        (0,((40,103),(47,105),(47,115),(36,115),(35,112),(39,109))),
        (2,((41,106),(45,107),(45,113),(38,113),(39,110))),
        (4,((41,107),(44,108),(44,110),(41,110))),
        (3,((37,113),(45,113),(46,114),(36,114))),
        (0,((12,105),(20,105),(20,116),(8,116),(8,113),(12,110))),
        (2,((13,108),(18,108),(18,113),(10,113),(13,111))),
        (4,((14,108),(17,108),(17,110),(14,110))),
        (3,((10,114),(18,114),(19,115),(9,115)))]
    additions += [
        (0,((35,13),(43,16),(47,20),(48,37),(46,51),(41,55),(39,44),(42,30),(38,24))),
        (2,((39,19),(44,21),(46,30),(45,44),(42,49),(42,36),(43,27))),
        (3,((43,26),(45,27),(45,35),(44,40),(43,36))),
        (0,((39,47),(44,51),(48,62),(48,78),(45,87),(40,84),(43,74),(43,62),(37,54))),
        (8,((41,52),(43,54),(46,65),(45,76),(43,79),(44,69),(41,59))),
        (12,((43,77),(47,77),(46,82),(42,82))),
        (7,((43,82),(46,82),(45,86),(42,84)))]
    if running:
        additions += [
            (0,((24,43),(18,38),(14,29),(15,18),(12,14),(8,17),(8,31),(12,43),(20,51))),
            (8,((21,43),(17,37),(12,29),(12,21),(10,20),(10,30),(14,40),(20,47))),
            (10,((11,23),(12,24),(12,29),(11,29))),
            (12,((8,17),(14,17),(15,22),(9,23),(8,21))),
            (7,((9,14),(12,13),(14,16),(13,19),(9,18))),
            (9,((10,15),(12,15),(12,17),(10,17)))]
    if facing=='right':additions=[(c,tuple((60-x,y) for x,y in points)) for c,points in additions]
    polygons=additions+shapes(facing,phase)
    if progress is not None:polygons=animate_polygons(polygons,progress,2 if running else 1)
    if vector_only:return polygons
    raw=bytearray(width*height*4)
    for y in range(height):
        for x in range(width):
            sx=10+(domain[0]+(x+.5)*domain[2]/width)*37/60
            sy=8+(domain[1]+(y+.5)*domain[3]/height)*109/120
            color=None
            for c,points in polygons:
                if inside(sx,sy,points):color=c
            if color is not None:
                off=(y*width+x)*4;raw[off:off+4]=bytes((*PALETTE[color],255))
    return bytes(raw)

def observed_raster(width,height,facing,animation,domain,progress=None):
    if facing in ('left','right') and animation in (1,2):
        return walking_profile_raster(width,height,facing,0,domain,running=animation==2,progress=progress)
    # Dedicated text-designed 3/4 projection of original vector clusters;
    # not an edit, rescale, or quantization of any bitmap.
    base='back' if facing.startswith('back') else facing
    if facing.startswith('front'):base='front'
    polygons=shapes(base,0)
    if '_' in facing:
        right=facing.endswith('right')
        projected=[]
        for c,points in polygons:
            transformed=[]
            for x,y in points:
                xx=30+(x-30)*.9
                if y<40:xx-=3
                if right:xx=60-xx
                transformed.append((xx,y))
            projected.append((c,tuple(transformed)))
        polygons=projected
    if '_' in facing and animation in (1,2):
        # Original long ponytail, counter-swinging wrist and loose trouser
        # clusters for diagonal locomotion. Geometry regions guide bounds only.
        outer=[
            (0,((21,12),(15,14),(10,22),(9,38),(12,51),(18,55),(19,45),(15,32),(20,23))),
            (2,((17,18),(13,23),(12,35),(14,47),(17,49),(15,35),(17,28))),
            (3,((13,27),(14,27),(14,34),(13,37),(12,34))),
            (0,((17,50),(12,56),(9,66),(10,82),(14,88),(19,83),(15,76),(15,66),(21,57))),
            (8,((16,56),(13,60),(12,68),(13,78),(16,80),(14,68),(18,61))),
            (12,((10,77),(16,78),(17,83),(12,85),(10,83))),
            (7,((11,83),(16,84),(15,88),(11,86))),
            (0,((19,77),(17,91),(12,105),(10,117),(19,117),(21,108),(25,97),(26,84))),
            (19,((20,82),(19,92),(15,104),(13,112),(17,112),(19,104),(23,94))),
            (21,((19,88),(20,89),(18,98),(16,101),(16,98))),
            (0,((12,107),(18,108),(18,117),(7,117),(7,114),(11,112))),
            (2,((13,110),(16,111),(16,114),(9,115),(11,113))),
            (4,((13,110),(15,111),(15,112),(13,112)))]
        if facing.endswith('left'):outer=[(c,tuple((60-x,y) for x,y in points)) for c,points in outer]
        polygons=outer+polygons
    if animation==2:
        # Original running posture: forward torso lean, same fixed palette.
        sign=1 if 'right' in facing else -1 if 'left' in facing else 0
        polygons=[(c,tuple((x+sign*3*max(0,1-y/100),y) for x,y in points)) for c,points in polygons]
    if progress is not None:polygons=animate_polygons(polygons,progress,animation)
    raw=bytearray(width*height*4)
    for y in range(height):
        for x in range(width):
            sx=10+(domain[0]+(x+.5)*domain[2]/width)*37/60
            sy=8+(domain[1]+(y+.5)*domain[3]/height)*109/120
            color=None
            for c,points in polygons:
                if inside(sx,sy,points):color=c
            if color is not None:
                off=(y*width+x)*4;raw[off:off+4]=bytes((*PALETTE[color],255))
    return bytes(raw)

def animate_polygons(polygons,progress,animation):
    # Artist-selected ORIGINAL walk/run stride, sampled in proven frame order.
    # This is not a claim of retail anatomical contact or decoded gait phase.
    wave=math.sin(progress*2*math.pi)
    arm_wave=math.cos(progress*2*math.pi)
    amplitude=4.5 if animation==2 else 3.0
    result=[]
    for color,points in polygons:
        moved=[]
        for x,y in points:
            if y>=77:
                weight=min(1,(y-77)/28)
                side=-1 if x<29 else 1
                x+=side*wave*amplitude*weight
                y-=max(0,side*wave)*1.4*weight
            elif 43<=y<=76 and (x<21 or x>38):
                side=-1 if x<29 else 1
                x-=side*wave*1.5
                y+=side*arm_wave*(3 if animation==2 else 2)
            moved.append((x,y))
        result.append((color,tuple(moved)))
    return result

def clip_polygon(points,axis,bound,greater):
    result=[]
    if not points:return result
    previous=points[-1]
    previous_inside=previous[axis]>=bound if greater else previous[axis]<=bound
    for point in points:
        current_inside=point[axis]>=bound if greater else point[axis]<=bound
        if current_inside!=previous_inside:
            delta=point[axis]-previous[axis]
            t=(bound-previous[axis])/delta
            crossing=tuple(previous[k]+t*(point[k]-previous[k]) for k in (0,1))
            result.append(crossing)
        if current_inside:result.append(point)
        previous,previous_inside=point,current_inside
    return result

def frame42_original_envelope(width,height,part,domain,progress):
    # New original polygon authoring fitted to reported scalar edge errors.
    # No pixels/bitmaps/masks are input. Same source art, same2xpart grid.
    polygons=walking_profile_raster(width,height,'right',0,domain,running=False,
                                   progress=progress,vector_only=True)
    authored=[]
    top,right,bottom=(1,3,1) if part==3 else (0,5,0) if part in (4,6,8) else (0,3,0) if part==7 else (0,0,0)
    for color,points in polygons:
        local=[(((x-10)*60/37-domain[0])*width/domain[2],
                ((y-8)*120/109-domain[1])*height/domain[3]) for x,y in points]
        # Clip source VECTOR geometry to the original authored cell domain.
        for axis,bound,greater in ((0,0,True),(0,width,False),(1,0,True),(1,height,False)):
            local=clip_polygon(local,axis,bound,greater)
        if part==1:
            local=[(x*(width+3)/width-3,y) for x,y in local]
        else:
            local=[(x*(width-right)/width,top+y*(height-top-bottom)/height) for x,y in local]
        if local:authored.append((color,local))
    raw=bytearray(width*height*4)
    for y in range(height):
        for x in range(width):
            color=None
            for c,points in authored:
                if inside(x+.5,y+.5,points):color=c
            if color is not None:
                off=(y*width+x)*4;raw[off:off+4]=bytes((*PALETTE[color],255))
    return bytes(raw)

def order_lookup(path):
    if path is None:return {},None
    raw=path.read_bytes();segments=json.loads(raw)
    best={}
    for segment in segments:
        key=(int(segment['animation']),int(segment['angle']))
        runs=segment['runs']
        if key[0] not in (1,2) or len(runs)<4:continue
        if key not in best or len(runs)>len(best[key]):best[key]=runs
    lookup={}
    for (animation,angle),runs in best.items():
        elapsed=0;total=sum(int(r['observed_presents']) for r in runs)
        for index,run in enumerate(runs):
            lookup[(animation,angle,int(run['raw_frame']))]=dict(
                observed_order_index=index,observed_sequence_length=len(runs),
                observed_duration_presents=int(run['observed_presents']),
                authored_cycle_progress=elapsed/total,
                anatomical_contact='artist-selected; not retail contact proof',
                observed_start_present=run['start_present'],observed_end_present=run['end_present'])
            elapsed+=int(run['observed_presents'])
    return lookup,hashlib.sha256(raw).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inventory',type=Path);parser.add_argument('output',type=Path)
    parser.add_argument('--order',type=Path)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    phase_lookup,order_hash=order_lookup(args.order)
    raw_source=args.inventory.read_bytes();inventory=json.loads(raw_source)
    for variant in inventory['variants']:
        variant.setdefault('raw_frame',int(variant.get('frame',0)))
        variant.setdefault('variant_id',variant.get('id',0))
        variant.setdefault('observed_presents',variant.get('presents',[]))
    cells=[];mappings=[];variants=[];unique={};pixel_data=[]
    atlas_width=512;cursor_x=cursor_y=row_height=0
    for variant in inventory['variants']:
        parts=variant['parts']
        quads=[[int(x) for x in p['quad'].split(',')] for p in parts]
        xs=[x for q in quads for x in q[::2]];ys=[y for q in quads for y in q[1::2]]
        x0,x1,y0,y1=min(xs),max(xs),min(ys),max(ys)
        if x1<=x0 or y1<=y0:raise ValueError('empty frame geometry')
        mirror_votes=sum(q[2]<q[0] for q in quads)
        facing,phase,status,state=association(variant)
        animation=int(state['animation']) if state else None
        phase_evidence=phase_lookup.get((animation,int(state['angle']),variant['raw_frame'])) if state else None
        progress=phase_evidence['authored_cycle_progress'] if phase_evidence else None
        variant_record=dict(variant_id=variant['variant_id'],raw_frame=variant['raw_frame'],
                            family_art=facing,authored_source_pose=('original moving limbs sampled in observed order' if phase_evidence else 'provisional neutral original clusters'),
                            authored_phase=phase_evidence,observed_animation=animation,selected_observed_state=state,
                            observed_states=variant.get('states',[]),association_status=status,
                            strict_gate='PENDING',geometry_bounds=[x0,y0,x1,y1],parts=[])
        for p,q in zip(parts,quads):
            uv=[int(x) for x in p['uv'].split(',')];u=uv[::2];v=uv[1::2]
            width,height=2*(abs(u[1]-u[0])+1),2*(abs(v[2]-v[0])+1)
            # Signed domain follows original UV vertex order. The native quad
            # projects those same vertices; no second renderer-side art flip.
            domain=((q[0]-x0)*60/(x1-x0),(q[1]-y0)*120/(y1-y0),
                    (q[2]-q[0])*60/(x1-x0),(q[5]-q[1])*120/(y1-y0))
            if variant['variant_id']==0 and variant['raw_frame']==190 and int(p['part'])==2:
                # Re-author from approved ORIGINAL v3e vector/scalar recipe.
                # Width16 keeps14 authored columns; no existing bitmap input.
                rgba=raster(width,height,'front',0,domain,fill_geometry=True,
                            right_envelope_pixels=2,canvas_right_pixels=2)
            else:
                rgba=(frame42_original_envelope(width,height,int(p['part']),domain,progress) if variant['variant_id']==1 and variant['raw_frame']==42 and int(p['part']) in (1,3,4,6,7,8) else observed_raster(width,height,facing,animation,domain,progress=progress))
            key=(width,height,rgba)
            if key not in unique:
                if cursor_x+width>atlas_width:cursor_x=0;cursor_y+=row_height;row_height=0
                idx=len(cells);unique[key]=idx
                cells.append(dict(cell=idx,x=cursor_x,y=cursor_y,width=width,height=height,
                                  retail_texel_grid=[width//2,height//2],**stats(rgba)))
                pixel_data.append(rgba);cursor_x+=width;row_height=max(row_height,height)
            cell=cells[unique[key]]
            mapping=dict(identity=int(p['fnv'],16),map=int(p['map']),actor=int(p['actor']),
                         frame=int(p['frame']),part=int(p['part']),tpage=int(p['tpage']),clut=int(p['clut']),
                         u=u,v=v,rect=[cell[k] for k in ('x','y','width','height')],
                         trace=f"{args.inventory}: variant{variant['variant_id']} present{p['present']}; geometry only; ORIGINAL art; strict/semantic approval pending")
            mappings.append(mapping)
            variant_record['parts'].append(dict(part=int(p['part']),cell=cell['cell'],
                                                source_quad=q,source_uv=uv,signed_authoring_domain=domain,
                                                horizontal_mirror=(q[2]-q[0])*(u[1]-u[0])<0))
        variants.append(variant_record)
    atlas_height=cursor_y+row_height
    if atlas_height>4096:raise ValueError('atlas exceeds renderer bounds')
    atlas=bytearray(atlas_width*atlas_height*4)
    for cell,rgba in zip(cells,pixel_data):
        for y in range(cell['height']):
            dst=((cell['y']+y)*atlas_width+cell['x'])*4
            src=y*cell['width']*4;atlas[dst:dst+cell['width']*4]=rgba[src:src+cell['width']*4]
    # Same observed runtime key may repeat; reject inconsistent art decisions.
    bykey={}
    for m in mappings:
        key=tuple(m[k] for k in ('identity','map','actor','frame','part','tpage','clut'))+tuple(m['u'])+tuple(m['v'])
        if key in bykey and bykey[key]['rect']!=m['rect']:
            raise ValueError('same exact source key assigned inconsistent art; needs geometry/phase proof')
        bykey[key]=m
    manifest=dict(schema=1,label=LABEL,width=atlas_width,height=atlas_height,
                  rgba_path='pc_port/assets/hd2d/fei-observed137-animated-v4.rgba',parts=list(bykey.values()),
                  production_status='PRIVATE PROVISIONAL; not strict-gated; not production mapping',
                  runtime_pose_approval=False,observed_order_sha256=order_hash,variant_count=len(variants),raw_frame_count=len({v['raw_frame'] for v in variants}),
                  original_authoring='New code-authored polygon clusters; no bitmap or retail-alpha inputs',
                  author_source='pc_port/tools/author_fei_observed39.py',palette=PALETTE,
                  source_inventory_sha256=hashlib.sha256(raw_source).hexdigest(),
                  variants=variants,cells=cells,aggregate_palette=stats(atlas))
    png(args.output/'fei-observed137-animated-v4.png',atlas_width,atlas_height,atlas)
    (args.output/'fei-observed137-animated-v4.rgba').write_bytes(atlas)
    (args.output/'manifest.provisional.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    private=args.output/'private';private.mkdir(exist_ok=True)
    rows=[]
    for m in manifest['parts']:
        nums=[m[k] for k in ('identity','map','actor','frame','part','tpage','clut')]
        rows.append('    {'+', '.join(map(str,nums))+', {'+', '.join(map(str,m['u']))+'}, {'+', '.join(map(str,m['v']))+'}, '+', '.join(map(str,m['rect']))+'}')
    source='/* '+LABEL+'; PRIVATE PROVISIONAL strict gate pending */\n#include "hd2d_sprite_parts.h"\nstatic const PcPortHd2dPartMapping hd2d_parts[] = {\n'+',\n'.join(rows)+'\n};\n'
    source+=f'static const PcPortHd2dPartProfile hd2d_sprite_profile = {{{atlas_width}, {atlas_height}, hd2d_parts, {len(rows)}, "{manifest["rgba_path"]}"}};\n'
    (private/'hd2d_sprite_profile.h').write_text(source,encoding='utf-8')
    print(json.dumps(dict(variants=len(variants),raw_frames=manifest['raw_frame_count'],
                         unique_cells=len(cells),exact_keys=len(bykey),size=[atlas_width,atlas_height],
                         palette=manifest['aggregate_palette'],strict_gate='PENDING'),indent=2))
if __name__=='__main__':main()
