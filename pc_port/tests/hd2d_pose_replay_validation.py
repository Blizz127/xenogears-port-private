#!/usr/bin/env python3
"""Private-link native observed-pose replay. No production/asset modifications.
Wraps stock strict driver invocation, retaining its unchanged analyzer/criteria.
"""
import argparse,json,os,shlex,subprocess,sys
from pathlib import Path
import hd2d_footprint_validation as strict
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--corpus',type=Path,required=True);p.add_argument('--variant',type=int,required=True);p.add_argument('--profile-header',type=Path,required=True);p.add_argument('--checkpoint',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--prepare-only',action='store_true');a=p.parse_args()
 corpus=json.loads(a.corpus.read_text());v=next(v for v in corpus['variants'] if v['id']==a.variant);rows=v['parts'];state=max(v['states'],key=lambda s:len(s['presents']));angle=int(state['angle'])
 out=a.out.resolve();prep=out.parent/(out.name+'-prep');prep.mkdir(exist_ok=False);flags=shlex.split((ROOT/'pc_port/build_native/port-source.cflags').read_text())
 if sorted(int(r['part']) for r in rows)!=list(range(len(rows))) or not 1<=len(rows)<=63:raise ValueError('incomplete expected part set')
 text='#include <stdint.h>\nstatic const unsigned pose_fnv=0x'+v['fnv']+'u;\n'
 text+=f'static const int pose_map={v["map"]},pose_actor={int(rows[0]["actor"])},pose_frame={v["frame"]},pose_angle={angle},pose_parts={len(rows)};\n'
 text+='static const struct {unsigned char uv[8]; unsigned short tpage,clut;} pose_keys[]={\n'
 for r in rows:text+='{{'+r['uv']+'},'+r['tpage']+','+r['clut']+'},\n'
 text+='};\n';(prep/'hd2d_pose_replay_target.h').write_text(text)
 obj=prep/'pose.o';subprocess.run(['gcc','-I'+str(prep),'-c','pc_port/tests/hd2d_pose_replay_probe.c',*flags,'-o',str(obj)],cwd=ROOT,check=True)
 # Exact original observed input sequence; stop scheduling only after full-key latch.
 steps=['0:0'];t=16
 for run in (False,True):
  for mask in (0x2000,0x8000,0x4000,0x1000,0x3000,0xc000,0x6000,0x9000):steps.extend((f'{t}:{hex(mask|(0x40 if run else 0))}',f'{t+24}:0'));t+=40
 manifest={'label':'TEST ONLY observed render-input replay; NPC sprites omitted in all3 modes; not organic gameplay','corpus':str(a.corpus.resolve()),'variant':a.variant,'angle':angle,'keys':rows,'state':state,'settle_after_live_key_latch':600,'native_decoder':'func_8001DAE8','incoming_OT':'unchanged','profile':str(a.profile_header.resolve())};(prep/'replay-fixture.json').write_text(json.dumps(manifest,indent=2))
 original_run=subprocess.run
 def wrapped(cmd,*args,**kwargs):
  cmd=list(cmd)
  if 'env' in kwargs:
   kwargs['env']=dict(kwargs['env'],HD2D_POSE_REPLAY='1',HD2D_POSE_ISOLATE='1',XENO_FIELD_TEST_INPUT=','.join(steps))
  if cmd[0]=='gcc' and '-o' in cmd and str(out/'lane-footprint') in cmd:
   cmd+= [str(obj)]+['-Wl,--wrap='+h for h in ('func_8001E298','PcPort_FeiHd2dCapture','PcPort_FieldTestInputMerge','PcPort_FeiHd2dTestCaptureWanted')]
  return original_run(cmd,*args,**kwargs)
 strict.subprocess.run=wrapped
 oldargv=sys.argv
 sys.argv=['strict','--out',str(out),'--checkpoint',str(a.checkpoint.resolve()),'--profile-header',str(a.profile_header.resolve()),'--map',str(v['map']),'--actor',rows[0]['actor'],'--fnv','0x'+v['fnv'],'--frame',str(v['frame']),'--parts',str(len(rows)),'--timeout','180']+(['--prepare-only'] if a.prepare_only else [])
 try:rc=strict.main()
 finally:strict.subprocess.run=original_run;sys.argv=oldargv
 # Stock driver does not know this separately linked replay/scene fixture.
 # Correct provenance only; retain its strict measurements and return code.
 for name in ('fixture.json','summary.json'):
  file=out/name
  if file.exists():
   result=json.loads(file.read_text());provenance=result.get('fixture',result)
   provenance['scene_scope']=manifest['label'];provenance['replay_variant']=a.variant
   file.write_text(json.dumps(result,indent=2))
 if a.prepare_only:return rc
 log=(out/'run.log').read_text();caps=sorted(int(f.stem.rsplit('-',1)[1]) for f in (out/'cap').glob('fei-frame-*.png'))
 rendered={int(line.split('present=')[1].split()[0]) for line in log.splitlines() if line.startswith('[pose-replay] rendered ') and 'exact_keys=1 ' in line and 'restored=1 ' in line}
 required={n for c in caps for n in range(c//8*8,c+1)}
 good=bool(caps) and '[pose-replay] latched ' in log and '[pose-replay] FAIL ' not in log and required.issubset(rendered)
 (out/'replay-gate.json').write_text(json.dumps({'pass':good,'required_presents':len(required),'verified_presents':len(required&rendered),'original_strict_returncode':rc,'fixture':manifest},indent=2))
 return rc if good else 1
if __name__=='__main__':raise SystemExit(main())
