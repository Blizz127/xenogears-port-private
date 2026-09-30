/* Stubbed safety/cache gates; generated private2-target header required. */
#include "hd2d_pose_batch_probe.c"
#include <assert.h>
unsigned char g_PsxRam[PSX_RAM_SIZE];
FieldActor* volatile g_FieldActors;
int g_GameSceneMapNum=1,g_PlayerActorIndex=1,g_FieldNumActors=0;
unsigned g_PcPortPresentedFrames;
static int renders,inputs,wanted,captures,decodes,live;
static void *last_sprite,*last_ot;
int PcPort_QuickCheckpointFieldIsActive(void){return live;}
int PcPort_QuickCheckpointFieldIsSafe(void){return live;}
unsigned PcPort_Hd2dSpriteIdentity(const void*p){(void)p;return batch_fnv;}
void __real_func_8001E298(void*s,void*o){renders++;last_sprite=s;last_ot=o;}
void func_8001DAE8(void*s,u16 f,u32 p){(void)s;(void)f;(void)p;decodes++;}
int __real_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch*b,void*p,void*o){(void)b;(void)p;(void)o;captures++;return 17;}
void __real_PcPort_FieldTestInputMerge(u16*h,u16*p,u16*r){(void)h;(void)p;(void)r;inputs++;}
int __real_PcPort_FeiHd2dTestCaptureWanted(unsigned p){(void)p;wanted++;return 1;}
int main(void){
 u16 h=0,p=0,r=0;char sprite=9,ot=3;int m,a,f,n;unsigned id;
 unsetenv("HD2D_POSE_BATCH");__wrap_func_8001E298(&sprite,&ot);assert(renders==1 && last_sprite==&sprite && last_ot==&ot && sprite==9);
 __wrap_PcPort_FieldTestInputMerge(&h,&p,&r);assert(inputs==1);assert(__wrap_PcPort_FeiHd2dTestCaptureWanted(999));assert(wanted==1);
 assert(__wrap_PcPort_FeiHd2dCapture(NULL,NULL,NULL)==17 && captures==1);
 setenv("HD2D_POSE_BATCH","1",1);__wrap_func_8001E298(NULL,&ot);assert(renders==2 && last_sprite==NULL);
 live=1;__wrap_func_8001E298(g_PsxRam,&ot);assert(renders==3 && !cached && !decodes);
 assert(!PcPort_Hd2dBatchExpected(&m,&a,&id,&f,&n));assert(!__wrap_PcPort_FeiHd2dTestCaptureWanted(999));assert(wanted==1);
 end_cache(700);assert(broken && cache_done && !start); /* missing must FAIL, no fabricated pose */
 broken=0;cached=BATCH_TARGET_COUNT;for(int i=0;i<BATCH_TARGET_COUNT;i++)poses[i].valid=1;end_cache(701);assert(!broken && start==1320);
 assert(!__wrap_PcPort_FeiHd2dTestCaptureWanted(1319));assert(__wrap_PcPort_FeiHd2dTestCaptureWanted(1326));
 active=0;assert(PcPort_Hd2dBatchExpected(&m,&a,&id,&f,&n));assert(m==batch_map && a==batch_actor && f==batch_targets[0].frame && n==batch_targets[0].parts);
 puts("PASS batch off/invalid forwarding, missing-cache failure,600settle and dynamic metadata gates");
}
