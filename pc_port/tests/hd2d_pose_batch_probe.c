/* TEST ONLY: cache naturally observed exact poses; replay at current resting
 * player XYZ with native decoder/camera/original incoming OT. No game state is
 * persisted. Invalid/missing/lifetime-changed poses fail closed. */
#include "common.h"
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "fei_hd2d.h"
#include "quick_checkpoint.h"
#include "psx_memory.h"
#include "hd2d_pose_replay_memory.h"
#include "hd2d_pose_batch_clock.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hd2d_pose_batch_targets.h"
extern int g_GameSceneMapNum,g_PlayerActorIndex,g_FieldNumActors;
extern unsigned g_PcPortPresentedFrames;
extern void __real_func_8001E298(void*,void*);
extern void func_8001DAE8(void*,u16,u32);
extern int __real_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch*,void*,void*);
extern void __real_PcPort_FieldTestInputMerge(u16*,u16*,u16*);
extern int __real_PcPort_FeiHd2dTestCaptureWanted(unsigned);
static unsigned short r16(const void*p){unsigned short n;memcpy(&n,p,2);return n;}
static unsigned r32(const void*p){unsigned n;memcpy(&n,p,4);return n;}
static u8 *ram(uintptr_t p,size_t n){uintptr_t b=(uintptr_t)g_PsxRam;return n<=PSX_RAM_SIZE && p>=b && p-b<=PSX_RAM_SIZE-n?(u8*)p:NULL;}
typedef struct {u8 s[0xb4],base[0x40],prim[14*24],dirs[64];short animation;void *sp,*bp,*pp,*dp;int valid;} Snapshot;
static Snapshot poses[BATCH_TARGET_COUNT];
static int candidate=-1,active=-1,offered,keys_ok,broken,cache_done,last_logged=-2;
static unsigned first_safe,input_calls,cache_end,start,cached;
static int on(void){const char*p=getenv("HD2D_POSE_BATCH");return p && !strcmp(p,"1");}
static void fail(const char *why){broken=1;fprintf(stderr,"[pose-batch] FAIL present=%u reason=%s\n",g_PcPortPresentedFrames+1,why);}
static int table(void){return g_FieldNumActors>0 && g_FieldNumActors<=1536 && ram((uintptr_t)g_FieldActors,(size_t)g_FieldNumActors*sizeof(FieldActor));}
static int player(void *s){return PcPort_QuickCheckpointFieldIsActive() && (g_GameSceneMapNum&0x3fff)==batch_map && table() && g_PlayerActorIndex==batch_actor && batch_actor>=0 && batch_actor<g_FieldNumActors && (uintptr_t)s==g_FieldActors[batch_actor].pSpriteData && PcPort_Hd2dSpriteIdentity(s)==batch_fnv;}
/* Called by the separate strong-hook observer, solely during a replay draw. */
int PcPort_Hd2dBatchExpected(int *map,int *actor,unsigned *fnv,int *frame,int *parts){
 if(active<0 || broken)return 0;
 *map=batch_map;*actor=batch_actor;*fnv=batch_fnv;*frame=batch_targets[active].frame;*parts=batch_targets[active].parts;return 1;
}
int __wrap_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch*b,void*p,void*ot){
 int idx=active>=0?active:candidate;
 if(idx>=0 && b->sprite==poses[idx].sp){const POLY_FT4*q=p;int n=offered++;unsigned char uv[8]={q->u0,q->v0,q->u1,q->v1,q->u2,q->v2,q->u3,q->v3};
  if(n>=batch_targets[idx].parts || memcmp(uv,batch_targets[idx].keys[n].uv,8) || q->tpage!=batch_targets[idx].keys[n].tpage || q->clut!=batch_targets[idx].keys[n].clut)keys_ok=0;
 }
 return __real_PcPort_FeiHd2dCapture(b,p,ot);
}
static void end_cache(unsigned now){
 cache_done=1;cache_end=now;
 if(cached!=BATCH_TARGET_COUNT){for(int i=0;i<BATCH_TARGET_COUNT;i++)if(!poses[i].valid)fprintf(stderr,"[pose-batch] missing variant=%d\n",batch_targets[i].id);fail("missing-naturally-observed-variants");return;}
 start=Hd2dBatchStart(now+600);
 fprintf(stderr,"[pose-batch] cache_complete present=%u poses=%u replay_start=%u triples=%u relocated_current_player_xyz=1\n",now,cached,start,batch_triples);
}
void __wrap_func_8001E298(void *ptr,void *ot){
 u8*s=ptr,*base,*prim,*dirs;ActorData*a;unsigned now=g_PcPortPresentedFrames+1,count;int idx=-1;
 if(!on()){__real_func_8001E298(ptr,ot);return;}
 /* Same Fei-only isolation in every mode; explicit fixture, never organic. */
 if(PcPort_QuickCheckpointFieldIsActive() && table() && g_PlayerActorIndex>=0 && g_PlayerActorIndex<g_FieldNumActors && (uintptr_t)s!=g_FieldActors[g_PlayerActorIndex].pSpriteData){for(int i=0;i<g_FieldNumActors;i++)if((uintptr_t)s==g_FieldActors[i].pSpriteData)return;}
 if(broken || !ram((uintptr_t)s,0xb4) || !player(s)){__real_func_8001E298(ptr,ot);return;}
 if(cache_done)idx=Hd2dBatchIndex(now,start,BATCH_TARGET_COUNT,batch_triples);
 else if(first_safe && now-first_safe>=120 && PcPort_QuickCheckpointFieldIsSafe())for(int i=0;i<BATCH_TARGET_COUNT;i++)if(!poses[i].valid && r16(s+0x34)==batch_targets[i].frame && (r16(s+0x80)&4095)==batch_targets[i].angle){idx=i;break;}
 if(idx<0){__real_func_8001E298(ptr,ot);return;}
 Snapshot *q=&poses[idx];unsigned want=batch_targets[idx].parts;
 base=ram(r32(s+0x20),0x40);a=(ActorData*)ram(g_FieldActors[batch_actor].pActorData,sizeof(ActorData));count=(r32(s+0x40)>>2)&63;if(count<want)count=want;
 if(count>14 || !base || !a){fail("invalid-live-count-base-actor");__real_func_8001E298(ptr,ot);return;}
 prim=ram(r32(base+0x30),count*24);dirs=r32(base+0x34)?ram(r32(base+0x34),64):NULL;
 const u8*pkg=ram(r32(s+0x24),20),*ft=pkg?ram(r32(pkg),2):NULL;
 if(!prim || (r32(base+0x34)&&!dirs) || !ft || batch_targets[idx].frame>=(int)((r16(ft)&511)+1) || !ram((uintptr_t)ft,((r16(ft)&511)+1)*2)){fail("invalid-live-buffers-package");__real_func_8001E298(ptr,ot);return;}
 candidate=-1;active=-1;offered=0;keys_ok=1;
 Hd2dPoseRange ranges[5];size_t nr=0;
 if(!cache_done){
  if(((r32(s+0x40)>>2)&63)!=want){__real_func_8001E298(ptr,ot);return;}
  memcpy(q->s,s,sizeof q->s);memcpy(q->base,base,sizeof q->base);memcpy(q->prim,prim,want*24);if(dirs)memcpy(q->dirs,dirs,64);
  q->sp=s;q->bp=base;q->pp=prim;q->dp=dirs;q->animation=a->curAnimationId;candidate=idx;
 }else{
  if(!q->valid || s!=q->sp || base!=q->bp || prim!=q->pp || dirs!=q->dp || r32(s+0x24)!=r32(q->s+0x24)){fail("cached-pose-buffer-lifetime-changed");__real_func_8001E298(ptr,ot);return;}
  u8 relocated[0xb4];memcpy(relocated,q->s,sizeof relocated);memcpy(relocated,s,12); /* current actual XYZ only */
  if(count>want)memcpy(q->prim+want*24,prim+want*24,(count-want)*24);
  ranges[nr++]=(Hd2dPoseRange){s,relocated,sizeof relocated,{0}};ranges[nr++]=(Hd2dPoseRange){base,q->base,sizeof q->base,{0}};ranges[nr++]=(Hd2dPoseRange){prim,q->prim,count*24,{0}};
  if(dirs)ranges[nr++]=(Hd2dPoseRange){dirs,q->dirs,64,{0}};ranges[nr++]=(Hd2dPoseRange){&a->curAnimationId,&q->animation,sizeof q->animation,{0}};
  if(!Hd2dPoseApply(ranges,nr)){fail("invalid-restore-plan");__real_func_8001E298(ptr,ot);return;}
  active=idx;func_8001DAE8(s,batch_targets[idx].frame,r32(s+0x24));
  if(last_logged!=idx){last_logged=idx;fprintf(stderr,"[pose-batch] target_start present=%u slot=%d variant=%d frame=%d parts=%d\n",now,idx,batch_targets[idx].id,batch_targets[idx].frame,want);}
 }
 __real_func_8001E298(ptr,ot);
 if(cache_done){
  Hd2dPoseRestore(ranges,nr);int restored=1;for(size_t n=0;n<nr;n++)if(memcmp(ranges[n].live,ranges[n].saved,ranges[n].size))restored=0;
  if(!restored || offered!=(int)want || !keys_ok)fail("replay-keys-or-restoration-mismatch");
  fprintf(stderr,"[pose-batch] rendered present=%u variant=%d exact_keys=%d parts=%d restored=%d original_ot=1\n",now,batch_targets[idx].id,keys_ok,offered,restored);
 }else if(offered==(int)want && keys_ok){q->valid=1;cached++;fprintf(stderr,"[pose-batch] cached present=%u variant=%d frame=%d angle=%d parts=%u\n",now,batch_targets[idx].id,batch_targets[idx].frame,batch_targets[idx].angle,want);}
 active=-1;candidate=-1;
}
void __wrap_PcPort_FieldTestInputMerge(u16*h,u16*p,u16*r){
 if(!on()){__real_PcPort_FieldTestInputMerge(h,p,r);return;}
 if(broken || cache_done || (g_GameSceneMapNum&0x3fff)!=batch_map || !PcPort_QuickCheckpointFieldIsSafe())return;
 unsigned now=g_PcPortPresentedFrames+1;if(!first_safe)first_safe=now;if(now-first_safe<120)return;
 if(cached==BATCH_TARGET_COUNT || input_calls>=656){end_cache(now);return;}
 __real_PcPort_FieldTestInputMerge(h,p,r);input_calls++;
}
int __wrap_PcPort_FeiHd2dTestCaptureWanted(unsigned p){
 if(!on())return __real_PcPort_FeiHd2dTestCaptureWanted(p);
 if(broken || !cache_done || Hd2dBatchIndex(p,start,BATCH_TARGET_COUNT,batch_triples)<0)return 0;
 return __real_PcPort_FeiHd2dTestCaptureWanted(p);
}
