#!/usr/bin/env python3
"""Audit explicit observed packet-key variants; no unobserved pose inference."""
import argparse,json
from collections import defaultdict
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('inventory',type=Path);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
d=json.loads(a.inventory.read_text());frames=defaultdict(list)
for v in d['variants']:
 parts=v['parts'];orientation=[]
 for r in parts:
  q=list(map(int,r['quad'].split(',')));orientation.append('reversed_x' if q[2]<q[0] else 'forward_x' if q[2]>q[0] else 'degenerate_x')
 frames[v['raw_frame']].append({'variant_id':v['variant_id'],'part_count':len(parts),'part_ids':[int(r['part']) for r in parts],'quad_orientation':sorted(set(orientation)), 'sample':v['stable_screenshot'],'first_present':min(v['observed_presents']),'last_present':max(v['observed_presents']), 'exact_keys':[{k:r[k] for k in ('map','actor','fnv','frame','part','uv','tpage','clut')} for r in parts]})
r={'source':str(a.inventory.resolve()),'raw_frame_count':len(frames),'variant_count':sum(map(len,frames.values())),'frames':dict(sorted(frames.items())), 'unknown':['actual curAnimationId','camera_relative_angle','sprite_direction','runtime_frame_table_count','diagonal/run/talk/gesture/jump coverage'], 'coverage_denominator':'Observed corpus only; not all available package frames or all opening route behaviors.','mirror_note':'Reversed quad X is observed packet orientation, not inferred atlas mirroring or semantic facing.'}
a.out.write_text(json.dumps(r,indent=2));print('frames',len(frames),'variants',r['variant_count']);print([(k,[(v['part_count'],v['quad_orientation']) for v in vs]) for k,vs in sorted(frames.items())])
