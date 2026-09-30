#!/usr/bin/env python3
"""Read-only signed edge diagnostic for triples already chosen by strict analyzer.
Uses exactly its runtime alpha-box calibration; changes no pass criteria.
"""
import argparse,json,re
from pathlib import Path
from fei_hd2d_footprint_analyze import read_bmp,parse_log,diff_mask,bbox,point_in_quad
p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);a=p.parse_args();d=a.run
frames,meta=parse_log(str(d/'merged-parts.log'));results=[]
for match in re.finditer(r'FEI FOOTPRINT triple (\d+)/(\d+)/(\d+):',(d/'analysis.txt').read_text()):
 pr,ph,px=map(int,match.groups());w,h,R=read_bmp(str(d/'cap'/f'fei-frame-{pr:06d}.png'));_,_,H=read_bmp(str(d/'cap'/f'fei-frame-{ph:06d}.png'));_,_,X=read_bmp(str(d/'cap'/f'fei-frame-{px:06d}.png'))
 mr=diff_mask(R,X,w,(0,0,w-1,h-1));mh=diff_mask(H,X,w,(0,0,w-1,h-1));br,bh=bbox(list(mr)),bbox(list(mh))
 rb=max(n for n in frames if pr//8*8<=n<=pr);hb=max(n for n in frames if ph//8*8<=n<=ph);box=meta.get(('box',hb));parts=[]
 if box and bh:
  sx=(bh[2]+1-bh[0])/max(1e-6,box[2]-box[0]);sy=(bh[3]+1-bh[1])/max(1e-6,box[3]-box[1]);ox=bh[0]-sx*box[0];oy=bh[1]-sy*box[1]
  for idx,ot,q in sorted(frames[rb]):
   cq=tuple(q[i]*sx+ox if i%2==0 else q[i]*sy+oy for i in range(8));r=bbox([(x,y) for x,y in mr if point_in_quad(x+.5,y+.5,cq)]);v=bbox([(x,y) for x,y in mh if point_in_quad(x+.5,y+.5,cq)])
   parts.append({'part':idx,'quad_native':q,'relative_ot':ot,'retail_bounds':r,'hd_bounds':v,'signed_hd_minus_retail_ltrb':[v[i]-r[i] for i in range(4)] if r and v else None})
 results.append({'triple':[pr,ph,px],'retail_bounds':br,'hd_bounds':bh,'signed_hd_minus_retail_ltrb':[bh[i]-br[i] for i in range(4)] if br and bh else None,'runtime_alpha_box':box,'parts':parts})
(d/'signed-edges.json').write_text(json.dumps({'note':'Diagnostic only. Same calibration as unchanged strict analyzer; signed HD minus retail edges [left,top,right,bottom]. No pixel/mask export or art edits.','triples':results},indent=2));print('signed diagnostics for',len(results),'strict-measured triples')
