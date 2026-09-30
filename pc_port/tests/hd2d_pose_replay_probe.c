/* PRIVATE-link TEST fixture. Never linked to production. Captures a naturally
 * observed complete UV keyset, then re-decodes that temporary pose for all modes.
 * Incoming OT is never rewritten; strict analyzer decides actual consistency. */
#include "common.h"
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "fei_hd2d.h"
#include "quick_checkpoint.h"
#include "psx_memory.h"
#include "hd2d_pose_replay_memory.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hd2d_pose_replay_target.h"
extern int g_GameSceneMapNum,g_PlayerActorIndex,g_FieldNumActors;
extern unsigned g_PcPortPresentedFrames;
extern void __real_func_8001E298(void*,void*);
extern void func_8001DAE8(void*,u16,u32);
extern int __real_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch*,void*,void*);
extern void __real_PcPort_FieldTestInputMerge(u16*,u16*,u16*);
extern int __real_PcPort_FeiHd2dTestCaptureWanted(unsigned);
static unsigned short rd16(const void*p){unsigned short n;memcpy(&n,p,2);return n;}
static unsigned rd32(const void*p){unsigned n;memcpy(&n,p,4);return n;}
static u8 *ram(uintptr_t p,size_t n){uintptr_t b=(uintptr_t)g_PsxRam;return n<=PSX_RAM_SIZE && p>=b && p-b<=PSX_RAM_SIZE-n?(u8*)p:NULL;}
static u8 pose_sprite[0xb4],pose_base[0x40],pose_prim[64*24],pose_dirs[64];
static short pose_animation;
static void *pose_sprite_ptr,*pose_base_ptr,*pose_prim_ptr,*pose_dirs_ptr;
static unsigned first_safe,latch_present;
static int latched,observing,offered,keys_ok,broken;
static int enabled(void){const char*p=getenv("HD2D_POSE_REPLAY");return p && !strcmp(p,"1");}
static void fail(const char *s){broken=1;fprintf(stderr,"[pose-replay] FAIL present=%u reason=%s\n",g_PcPortPresentedFrames+1,s);}
static int actor_table(void){return g_FieldNumActors>0 && g_FieldNumActors<=1536 && ram((uintptr_t)g_FieldActors,(size_t)g_FieldNumActors*sizeof(FieldActor))!=NULL;}
static int player(void *s){return PcPort_QuickCheckpointFieldIsActive() && (g_GameSceneMapNum&0x3fff)==pose_map && actor_table() && g_PlayerActorIndex==pose_actor && pose_actor>=0 && pose_actor<g_FieldNumActors && (uintptr_t)s==g_FieldActors[pose_actor].pSpriteData && PcPort_Hd2dSpriteIdentity(s)==pose_fnv;}
int __wrap_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch *b,void *packet,void *ot) {
 if(observing && b->sprite==pose_sprite_ptr){
  const POLY_FT4 *q=packet;int n=offered++;unsigned char uv[8]={q->u0,q->v0,q->u1,q->v1,q->u2,q->v2,q->u3,q->v3};
  if(n>=pose_parts || memcmp(uv,pose_keys[n].uv,8) || q->tpage!=pose_keys[n].tpage || q->clut!=pose_keys[n].clut)keys_ok=0;
 }
 return __real_PcPort_FeiHd2dCapture(b,packet,ot);
}
void __wrap_func_8001E298(void *ptr,void *ot) {
 u8*s=ptr,*base,*prim,*dirs;ActorData*a;unsigned count,current_count;int candidate;
 const char *isolate=getenv("HD2D_POSE_ISOLATE");
 if(enabled() && isolate && !strcmp(isolate,"1") && PcPort_QuickCheckpointFieldIsActive() && actor_table() && g_PlayerActorIndex>=0 && g_PlayerActorIndex<g_FieldNumActors && (uintptr_t)s!=g_FieldActors[g_PlayerActorIndex].pSpriteData){
  for(int i=0;i<g_FieldNumActors;i++)if((uintptr_t)s==g_FieldActors[i].pSpriteData)return;
 }
 if(!enabled() || broken || !ram((uintptr_t)s,0xb4) || !player(s)){__real_func_8001E298(ptr,ot);return;}
 base=ram(rd32(s+0x20),0x40);a=(ActorData*)ram(g_FieldActors[pose_actor].pActorData,sizeof(ActorData));
 if(!base || !a){if(latched)fail("invalid-live-base-actor");__real_func_8001E298(ptr,ot);return;}
 current_count=(rd32(s+0x40)>>2)&63;count=current_count>pose_parts?current_count:pose_parts;
 prim=ram(rd32(base+0x30),count*24);dirs=rd32(base+0x34)?ram(rd32(base+0x34),64):NULL;
 const u8 *pkg=ram(rd32(s+0x24),20),*table=pkg?ram(rd32(pkg),2):NULL;
 if(!pkg || !table || pose_frame>=(int)((rd16(table)&511)+1) || !ram((uintptr_t)table,((rd16(table)&511)+1)*2)){
  if(latched)fail("invalid-live-package-table");__real_func_8001E298(ptr,ot);return;
 }
 if(!prim || (rd32(base+0x34)&&!dirs)){if(latched)fail("invalid-live-descriptors");__real_func_8001E298(ptr,ot);return;}
 candidate=!latched && first_safe && g_PcPortPresentedFrames+1-first_safe>=120 && rd16(s+0x34)==pose_frame && (rd16(s+0x80)&4095)==pose_angle && current_count==pose_parts && PcPort_QuickCheckpointFieldIsSafe();
 if(candidate){
  memcpy(pose_sprite,s,sizeof pose_sprite);memcpy(pose_base,base,sizeof pose_base);memcpy(pose_prim,prim,pose_parts*24);if(dirs)memcpy(pose_dirs,dirs,64);
  pose_animation=a->curAnimationId;pose_sprite_ptr=s;pose_base_ptr=base;pose_prim_ptr=prim;pose_dirs_ptr=dirs;
 }
 if(latched && (s!=pose_sprite_ptr || base!=pose_base_ptr || prim!=pose_prim_ptr || dirs!=pose_dirs_ptr || rd32(s+0x24)!=rd32(pose_sprite+0x24))){fail("live-buffer-identity-changed");__real_func_8001E298(ptr,ot);return;}
 Hd2dPoseRange ranges[5];size_t nr=0;
 if(latched){
  /* Preserve current descriptor tail as well when current count exceeds target. */
  if(count>pose_parts)memcpy(pose_prim+pose_parts*24,prim+pose_parts*24,(count-pose_parts)*24);
  ranges[nr++]=(Hd2dPoseRange){s,pose_sprite,sizeof pose_sprite,{0}};
  ranges[nr++]=(Hd2dPoseRange){base,pose_base,sizeof pose_base,{0}};
  ranges[nr++]=(Hd2dPoseRange){prim,pose_prim,count*24,{0}};
  if(dirs)ranges[nr++]=(Hd2dPoseRange){dirs,pose_dirs,64,{0}};
  ranges[nr++]=(Hd2dPoseRange){&a->curAnimationId,&pose_animation,sizeof pose_animation,{0}};
  if(!Hd2dPoseApply(ranges,nr)){fail("transaction-invalid");__real_func_8001E298(ptr,ot);return;}
  func_8001DAE8(s,pose_frame,rd32(s+0x24)); /* ORIGINAL retail decoder */
 }
 observing=candidate||latched;offered=0;keys_ok=1;
 __real_func_8001E298(ptr,ot);
 observing=0;
 if(latched){Hd2dPoseRestore(ranges,nr);}
 if(candidate && offered==pose_parts && keys_ok){latched=1;latch_present=g_PcPortPresentedFrames+1;fprintf(stderr,"[pose-replay] latched present=%u map=%d actor=%d frame=%d angle=%d parts=%d native_observed=1\n",latch_present,pose_map,pose_actor,pose_frame,pose_angle,pose_parts);}
 else if(latched && (offered!=pose_parts || !keys_ok))fail("native-decoder-keyset-mismatch");
 if(latched)fprintf(stderr,"[pose-replay] rendered present=%u exact_keys=%d parts=%d restored=1 original_ot=1\n",g_PcPortPresentedFrames+1,keys_ok,offered);
}
void __wrap_PcPort_FieldTestInputMerge(u16*h,u16*p,u16*r) {
 if(!enabled()){__real_PcPort_FieldTestInputMerge(h,p,r);return;}
 if(latched || broken || (g_GameSceneMapNum&0x3fff)!=pose_map || !PcPort_QuickCheckpointFieldIsSafe())return;
 unsigned now=g_PcPortPresentedFrames+1;if(!first_safe)first_safe=now;
 if(now-first_safe>=120)__real_PcPort_FieldTestInputMerge(h,p,r);
}
int __wrap_PcPort_FeiHd2dTestCaptureWanted(unsigned p) {
 if(!enabled())return __real_PcPort_FeiHd2dTestCaptureWanted(p);
 if(broken || !latched || p-latch_present<600)return 0;
 return __real_PcPort_FeiHd2dTestCaptureWanted(p);
}
