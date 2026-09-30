#!/usr/bin/env python3
"""TEST ONLY cached native pose batch; explicit complete windows per target.
Snapshots stay in process RAM. All runtime artifacts must stay private/local.
"""
import argparse,json,os,shlex,shutil,subprocess,sys,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--corpus',required=True,type=Path);p.add_argument('--variants',required=True,help='comma-separated observed IDs, or all');p.add_argument('--profile-header',required=True,type=Path);p.add_argument('--checkpoint',required=True,type=Path);p.add_argument('--out',required=True,type=Path);p.add_argument('--triples',type=int,default=3);p.add_argument('--prepare-only',action='store_true');p.add_argument('--timeout',type=int,default=420);a=p.parse_args()
 if not 1<=a.triples<=8:p.error('triples must be1..8')
 d=json.loads(a.corpus.read_text());ids=[v['id'] for v in d['variants']] if a.variants=='all' else [int(n) for n in a.variants.split(',')]
 if len(set(ids))!=len(ids) or not 1<=len(ids)<=137:p.error('require1..137 unique observed IDs')
 vs=[next(v for v in d['variants'] if v['id']==i) for i in ids]
 if any(v['map']!=1 or v['fnv']!='db7d0105' or not 1<=len(v['parts'])<=14 for v in vs):p.error('only verified map1 Fei <=14part corpus currently supported')
 from hd2d_opening_evidence import checkpoint
 qs=checkpoint(a.checkpoint);assert qs['map']==1
 out=a.out.resolve();out.mkdir(exist_ok=False);inc=out/'private';inc.mkdir()
 text='#define BATCH_TARGET_COUNT '+str(len(vs))+'\nstatic const unsigned batch_fnv=0xdb7d0105u,batch_triples='+str(a.triples)+';\nstatic const int batch_map=1,batch_actor=1;\nstatic const struct {int id,frame,angle,parts;struct {unsigned char uv[8];unsigned short tpage,clut;} keys[14];} batch_targets[]={\n'
 for v in vs:
  rows=sorted(v['parts'],key=lambda r:int(r['part']));assert [int(r['part']) for r in rows]==list(range(len(rows)))
  state=max(v['states'],key=lambda s:len(s['presents']))
  text+='{'+','.join(map(str,(v['id'],v['frame'],int(state['angle']),len(rows))))+',{' + ','.join('{{'+r['uv']+'},'+r['tpage']+','+r['clut']+'}' for r in rows)+'}},\n'
 text+='};\n';(inc/'hd2d_pose_batch_targets.h').write_text(text);shutil.copyfile(a.profile_header,inc/'hd2d_sprite_profile.h')
 build=ROOT/'pc_port/build_native';flags=shlex.split((build/'port-source.cflags').read_text());env={k:v for k,v in os.environ.items() if not k.startswith(('XENO_','DISPLAY','WAYLAND_DISPLAY','XAUTHORITY'))}
 env.update(HD2D_POSE_BATCH='1',XENO_HD2D_OPENING_MAPS='1',XENO_HD2D_BATCH_STATS='1',XENO_FEI_HD2D='0',XENO_HD2D_FOOTPRINT_PART_LOG=str(out/'lane-parts.log'),SDL_VIDEODRIVER='offscreen')
 steps=['0:0'];t=16
 for run in (False,True):
  for mask in (0x2000,0x8000,0x4000,0x1000,0x3000,0xc000,0x6000,0x9000):steps.extend((f'{t}:{hex(mask|(0x40 if run else 0))}',f'{t+24}:0'));t+=40
 env['XENO_FIELD_TEST_INPUT']=','.join(steps)
 def run(cmd,**kw):return subprocess.run(cmd,cwd=ROOT,env=env,**kw)
 objs=[]
 for name in ('hd2d_pose_batch_probe','hd2d_pose_batch_footprint'):
  obj=out/(name+'.o');run(['gcc','-I'+str(inc),'-c',f'pc_port/tests/{name}.c',*flags,'-o',str(obj)],check=True);objs.append(str(obj))
 part=out/'parts.o';run(['gcc','-I'+str(inc),'-c','pc_port/src/hd2d/hd2d_sprite_parts.c',*flags,'-o',str(part)],check=True)
 args=(build/'xeno-port.link-args').read_text().splitlines();idx=[i for i,v in enumerate(args) if v.endswith('/hd2d__hd2d_sprite_parts.c.o')];assert len(idx)==1;args[idx[0]]=str(part)
 hooks=['PcPort_Hd2dSpriteBegin','PcPort_Hd2dSpriteDraw','PcPort_Hd2dSpriteEnd','PcPort_Hd2dPostProcess','XgHd2d_Present','PcPort_Hd2dSpriteTextureOverride','func_8001E298','PcPort_FeiHd2dCapture','PcPort_FieldTestInputMerge','PcPort_FeiHd2dTestCaptureWanted'];binary=out/'batch-replay';run(['gcc',*objs,*args,*['-Wl,--wrap='+h for h in hooks],'-o',str(binary)],check=True)
 manifest={'label':'TEST ONLY cached observed poses relocated to current resting player XYZ; native decoder/camera/incoming OT retained; all NPC sprites omitted in all modes; not organic gameplay','requested_variants':ids,'triples_per_variant':a.triples,'checkpoint':qs,'corpus':str(a.corpus.resolve()),'profile':str(a.profile_header.resolve()),'settle_after_cache':600};(out/'fixture.json').write_text(json.dumps(manifest,indent=2))
 if a.prepare_only:print('BATCH PREPARED',binary);return 0
 from PIL import Image
 with (out/'drive.txt').open('w') as f:rc=run([sys.executable,'pc_port/tests/fei_hd2d_footprint_drive.py',str(binary),str(a.checkpoint.resolve()),str(out),str(len(vs)*a.triples*3),str(a.timeout)],stdout=f,stderr=subprocess.STDOUT).returncode
 log=(out/'run.log').read_text();m=re.search(r'\[pose-batch\] cache_complete present=(\d+) poses=(\d+) replay_start=(\d+)',log)
 if rc or not m or int(m[2])!=len(vs) or '[pose-batch] FAIL ' in log or '[hd2d-footprint] FAIL ' in log:
  (out/'summary.json').write_text(json.dumps({'overall':'FAIL','reason':'cache/capture/runtime invariant','missing':re.findall(r'\[pose-batch\] missing variant=(\d+)',log)},indent=2));return 1
 start=int(m[3]);raw=[]
 for name in ('parts.log','lane-parts.log'):raw+=(out/name).read_text().splitlines()
 def present(l):return int(l.split('present=')[1].split()[0])
 rendered={}
 for line in log.splitlines():
  if line.startswith('[pose-batch] rendered '):
   x=dict(v.split('=',1) for v in line.split()[2:]);rendered[int(x['present'])]=x
 summaries=[];env['FEI_FOOTPRINT_STRICT_HORIZONTAL']='1'
 for slot,v in enumerate(vs):
  first=start+slot*24*a.triples;end=first+24*a.triples;dest=out/f'variant-{v["id"]:03d}';(dest/'cap').mkdir(parents=True)
  captures=list(range(first+6,end,8));required={n for c in captures for n in range(c//8*8,c+1)}
  gate=all(n in rendered and rendered[n]['variant']==str(v['id']) and rendered[n]['exact_keys']=='1' and rendered[n]['restored']=='1' for n in required)
  for c in captures:
   src=out/'cap'/f'fei-frame-{c:06d}.png'
   if not src.exists():gate=False
   else:(dest/'cap'/src.name).symlink_to(src)
  lines=sorted((l for l in raw if first<=present(l)<end),key=present);(dest/'merged-parts.log').write_text('\n'.join(lines)+'\n')
  with (dest/'analysis.txt').open('w') as f:strict=run([sys.executable,'pc_port/tests/fei_hd2d_footprint_analyze.py',str(dest/'merged-parts.log'),str(dest/'cap'),str(dest),'--tol','1'],stdout=f,stderr=subprocess.STDOUT).returncode
  gpu=sum('gpu_accepted=1' in l and 'mode=1 ' in l and present(l) in required for l in lines)
  changed=0
  if all((dest/f'full_{x}.png').exists() for x in ('retail','hd2d','hidden')):
   im=[Image.open(dest/f'full_{x}.png').convert('RGB') for x in ('retail','hd2d','hidden')];changed=sum(r!=h and (r!=x or h!=x) for r,h,x in zip(*(i.getdata() for i in im)))
  analysis=(dest/'analysis.txt').read_text();per=all(f'p{i}:' in analysis for i in range(len(v['parts']))) and any('source=runtime_retail_vram' in l for l in lines)
  result={'variant':v['id'],'frame':v['frame'],'window':[first,end],'replay_gate':gate,'expected_build_presents':len(required),'strict_returncode':strict,'gpu_acceptances':gpu,'changed_pixels':changed,'per_part_measured':per,'overall':'PASS' if gate and strict==0 and gpu>0 and changed>0 and per else 'FAIL'}
  (dest/'summary.json').write_text(json.dumps(result,indent=2));summaries.append(result)
 overall=all(s['overall']=='PASS' for s in summaries);(out/'summary.json').write_text(json.dumps({'fixture':manifest,'overall':'PASS' if overall else 'FAIL','variants':summaries},indent=2));print('BATCH', 'PASS' if overall else 'FAIL',summaries);return 0 if overall else 1
if __name__=='__main__':raise SystemExit(main())
