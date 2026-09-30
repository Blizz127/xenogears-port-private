#include <assert.h>
#include "common.h"
#include "psyq/libgpu.h"
#include "hd2d_scene_contacts.h"
static int enabled, map=14;
int PcPort_Hd2dEnabled(void){return enabled;}
int PcPort_Hd2dOpeningActive(void){return 1;}
int PcPort_Hd2dCurrentMap(void){return map;}
static POLY_FT4 packets[6];
static PcPortHd2dSpriteInfo info={0xdb7d0105,190,1,(void*)10};
static void offer(int n){PcPort_Hd2dSceneContactBegin(&info);for(int i=0;i<n;++i){PcPortHd2dRetailQuad q={0};q.poly=packets+i;PcPort_Hd2dSceneContactDraw(&info,i,&q);}PcPort_Hd2dSceneContactEnd(&info);}
static void observe(int n){for(int i=0;i<n;++i){float b[4]={.3f,.2f+i*.05f,.5f,.3f+i*.05f};PcPort_Hd2dSceneContactGpuPart(packets+i,b,20);}}
int main(void){PcPortHd2dSceneContact out[8];
 PcPort_Hd2dSceneContactBegin((void*)1);PcPort_Hd2dSceneContactDraw((void*)1,0,(void*)1);PcPort_Hd2dSceneContactGpuPart((void*)1,(void*)1,20);assert(!PcPort_Hd2dSceneContactsTake(14,100,(void*)1));
 enabled=1;offer(6);PcPort_Hd2dSceneContactsGpuBegin();observe(6);
 assert(PcPort_Hd2dSceneContactsTake(14,100,out)==1);assert(out[0].x==.4f && out[0].y==.2f && out[0].raw_slot==20);
 assert(!PcPort_Hd2dSceneContactsTake(14,100,out));
 offer(5);PcPort_Hd2dSceneContactsGpuBegin();observe(5);assert(!PcPort_Hd2dSceneContactsTake(14,100,out));
 PcPort_Hd2dSceneContactsReset();offer(6);PcPort_Hd2dSceneContactsGpuBegin();observe(5);assert(!PcPort_Hd2dSceneContactsTake(14,100,out));
 offer(6);packets[2].u0++;PcPort_Hd2dSceneContactsGpuBegin();observe(6);assert(!PcPort_Hd2dSceneContactsTake(14,100,out));
 offer(6);map=1;PcPort_Hd2dSceneContactsGpuBegin();observe(6);assert(!PcPort_Hd2dSceneContactsTake(1,100,out));map=14;
 offer(6);offer(6);PcPort_Hd2dSceneContactsGpuBegin();observe(6);assert(PcPort_Hd2dSceneContactsTake(14,100,out)==1);
 // Repeated discarded/rejected GPU observations cannot exhaust reused packets.
 for(int i=0;i<24;++i){offer(6);PcPort_Hd2dSceneContactsGpuBegin();}
 observe(6);assert(PcPort_Hd2dSceneContactsTake(14,100,out)==1);
 for(int i=0;i<24;++i){offer(5);PcPort_Hd2dSceneContactsGpuBegin();}
 offer(6);observe(6);assert(PcPort_Hd2dSceneContactsTake(14,100,out)==1);
 PcPort_Hd2dSceneContactsReset();offer(6);PcPort_Hd2dSceneContactsGpuBegin();observe(6);assert(!PcPort_Hd2dSceneContactsTake(14,0,out));
 info.frame=42;offer(6);observe(6);assert(!PcPort_Hd2dSceneContactsTake(14,100,out));
}
