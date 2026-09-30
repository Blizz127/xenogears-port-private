#!/usr/bin/env python3
"""Read-only trace geometry audit; never reads or emits retail image pixels.
Reports exact doubled inclusive PSX UV grids, mirror state, and observed keys.
This is geometry evidence, not artwork or an animation/pose approval.
"""
import argparse
import hashlib
import json
from pathlib import Path

LABEL = 'fan-made, AI-generated replacement sprite art created for this project; not from the game; characters © Square Enix'

def inspect(part):
    uv = [int(x) for x in part['uv'].split(',')]
    quad = [int(x) for x in part['quad'].split(',')]
    if len(uv) != 8 or len(quad) != 8 or any(not 0 <= x <= 255 for x in uv):
        raise ValueError('invalid quad or UV tuple')
    u, v = uv[::2], uv[1::2]
    if u[0] != u[2] or u[1] != u[3] or v[0] != v[1] or v[2] != v[3]:
        raise ValueError('nonrectangular UV needs dedicated proof')
    width, height = abs(u[1]-u[0])+1, abs(v[2]-v[0])+1
    return dict(part=int(part['part']), tpage=int(part['tpage']), clut=int(part['clut']),
                u=u, v=v, retail_texel_grid=[width, height], exact_2x_grid=[width*2, height*2],
                screen_quad=quad,
                horizontal_mirror=(quad[2]-quad[0])*(u[1]-u[0]) < 0,
                vertical_mirror=(quad[5]-quad[1])*(v[2]-v[0]) < 0,
                evidence_present=int(part['present']))

def audit(path):
    raw = path.read_bytes()
    inventory = json.loads(raw)
    variants = []
    for item in inventory['variants']:
        parts = [inspect(p) for p in item['parts']]
        if sorted(p['part'] for p in parts) != list(range(len(parts))):
            raise ValueError('incomplete/noncontiguous part inventory')
        variants.append(dict(raw_frame=item['raw_frame'], variant_id=item['variant_id'],
                             observed_presents=item['observed_presents'],
                             authored_pose=None, pose_status='unverified', parts=parts))
    return dict(schema=1, kind='geometry_only_not_artwork', label=LABEL,
                source=str(path), source_sha256=hashlib.sha256(raw).hexdigest(),
                map=inventory['map'], actor=inventory['actor'], identity=inventory['fnv'],
                variant_count=len(variants), raw_frame_count=len({v['raw_frame'] for v in variants}),
                verified_16_pose_associations=0,
                sizing_rule='2*(abs(U1-U0)+1) by 2*(abs(V2-V0)+1), inclusive UV texel grid; never screen size',
                limitations=['No retail pixels read/copied', 'No semantic gait phase inferred',
                             'Per-part original alpha shape requires separate strict footprint validation',
                             'Screen quad/mirror and exact UV tuple must remain distinct'],
                variants=variants)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inventory', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = audit(args.inventory)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f"{result['variant_count']} variants, {result['raw_frame_count']} frames; 16-pose associations unverified")

if __name__ == '__main__':
    main()
