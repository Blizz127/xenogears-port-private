#!/usr/bin/env python3
"""Join runtime animation metadata with exact packet keys, keeping missing data explicit."""
import argparse,json,re
from collections import defaultdict,Counter
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('--out',type=Path,required=True);a=p.parse_args();run=a.run
log=(run/'TRACE/run.log').read_text();ready=re.search(r'\[capture-walk\] settled-active map=1 present=(\d+)',log)
if not ready:raise SystemExit('missing genuine safe-field readiness marker')
lo=int(ready[1]);caps=json.loads((run/'TRACE/captures.json').read_text());hi=max(int(n.rsplit('-',1)[1].split('.')[0]) for n in caps)
def parse(line):return dict(v.split('=',1) for v in line.split()[1:] if '=' in v)
metadata={};packages=[];topology=[]
for line in (run/'animation-metadata.log').read_text().splitlines():
 d=parse(line)
 if line.startswith('package '):packages.append(d)
 elif line.startswith('topology '):topology.append(d)
 elif line.startswith('sprite ') and lo<=int(d['present'])<=hi:metadata[(d['present'],d['map'],d['actor'],d['fnv'],d['frame'])]=d
parts=defaultdict(list)
for line in (run/'TRACE/probe.txt').read_text().splitlines():
 if not line.startswith('part '):continue
 d=parse(line)
 if lo<=int(d['present'])<=hi and d['actor']==d['player']:parts[(d['present'],d['map'],d['actor'],d['fnv'],d['frame'])].append(d)
variants={};groups=defaultdict(lambda:{'frames':set(),'presents':set(),'angles':set()});missing=0
for key,rows in sorted(parts.items(),key=lambda kv:int(kv[0][0])):
 m=metadata.get(key)
 if not m:missing+=len(rows);continue
 rows.sort(key=lambda r:int(r['part']))
 vk=tuple((r['map'],r['actor'],r['fnv'],r['frame'],r['part'],r['uv'],r['tpage'],r['clut']) for r in rows)
 v=variants.setdefault(vk,{'id':len(variants),'map':int(m['map']),'fnv':m['fnv'],'frame':int(m['frame']),'parts':rows,'metadata_first':m,'animations':set(),'angles':set(),'presents':[],'states':{}})
 sk=tuple(m[k] for k in ('animation','angle','direction','flags3c','flags40','flagsac','animation_flags'))
 state=v['states'].setdefault(sk,{k:m[k] for k in ('animation','angle','direction','flags3c','flags40','flagsac','animation_flags')})
 state.setdefault('presents',[]).append(int(m['present']))
 v['animations'].add(int(m['animation']));v['angles'].add(int(m['angle']));v['presents'].append(int(m['present']))
 g=groups[(m['map'],m['fnv'],m['animation'])];g['frames'].add(int(m['frame']));g['presents'].add(int(m['present']));g['angles'].add(int(m['angle']))
for v in variants.values():v['animations']=sorted(v['animations']);v['angles']=sorted(v['angles']);v['states']=list(v['states'].values())
summary=[{'map':int(k[0]),'fnv':k[1],'animation':int(k[2]),'frames':sorted(v['frames']),'angles':sorted(v['angles']),'present_count':len(v['presents'])} for k,v in sorted(groups.items())]
r={'run':str(run.resolve()),'window':[lo,hi],'packages':packages,'topology':topology,'animation_groups':summary,'variant_count':len(variants),'variants':list(variants.values()),'unmatched_part_records':missing,'metadata_present_count':len(metadata),'note':'Angles and animation IDs are observed runtime values; no inferred authored pose assignment. Topology indices are addressable package entries, not executed-route coverage.'}
a.out.write_text(json.dumps(r,indent=2));print(json.dumps({k:r[k] for k in ('window','packages','animation_groups','variant_count','unmatched_part_records')},indent=2))
