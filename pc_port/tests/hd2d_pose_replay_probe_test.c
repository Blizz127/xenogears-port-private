/* Stubbed forward/decline gates; no game state or GPU. Compile with a generated
 * private hd2d_pose_replay_target.h and the ordinary native source flags. */
#include "hd2d_pose_replay_probe.c"
#include <assert.h>
unsigned char g_PsxRam[PSX_RAM_SIZE];
FieldActor* volatile g_FieldActors;
int g_GameSceneMapNum=1,g_PlayerActorIndex=1,g_FieldNumActors=0;
unsigned g_PcPortPresentedFrames;
static int renders,inputs,wanted,captures,decodes,active;
static void *last_sprite,*last_ot;
int PcPort_QuickCheckpointFieldIsActive(void){return active;}
int PcPort_QuickCheckpointFieldIsSafe(void){return active;}
unsigned PcPort_Hd2dSpriteIdentity(const void*p){(void)p;return pose_fnv;}
void __real_func_8001E298(void*s,void*o){renders++;last_sprite=s;last_ot=o;}
void func_8001DAE8(void*s,u16 f,u32 p){(void)s;(void)f;(void)p;decodes++;}
int __real_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch*b,void*p,void*o){(void)b;(void)p;(void)o;captures++;return 17;}
void __real_PcPort_FieldTestInputMerge(u16*h,u16*p,u16*r){(void)h;(void)p;(void)r;inputs++;}
int __real_PcPort_FeiHd2dTestCaptureWanted(unsigned p){(void)p;wanted++;return 1;}
int main(void){
 u16 h=0,p=0,r=0;char sprite=9,ot=3;
 unsetenv("HD2D_POSE_REPLAY");__wrap_func_8001E298(&sprite,&ot);assert(renders==1 && last_sprite==&sprite && last_ot==&ot && sprite==9);
 __wrap_PcPort_FieldTestInputMerge(&h,&p,&r);assert(inputs==1);assert(__wrap_PcPort_FeiHd2dTestCaptureWanted(999));assert(wanted==1);
 assert(__wrap_PcPort_FeiHd2dCapture(NULL,NULL,NULL)==17 && captures==1);
 setenv("HD2D_POSE_REPLAY","1",1);__wrap_func_8001E298(NULL,&ot);assert(renders==2 && last_sprite==NULL);
 active=1;__wrap_func_8001E298(g_PsxRam,&ot);assert(renders==3 && last_sprite==g_PsxRam && !latched && !decodes);
 __wrap_PcPort_FieldTestInputMerge(&h,&p,&r);assert(inputs==1);assert(!__wrap_PcPort_FeiHd2dTestCaptureWanted(999));assert(wanted==1);
 puts("PASS off/invalid native hooks forward unchanged; unlatched capture blocked; decoder never forced");
}
