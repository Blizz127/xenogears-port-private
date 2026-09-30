#include "hd2d_scene_items.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
static int enabled,active=1,map=14;
int PcPort_Hd2dEnabled(void){return enabled;}
int PcPort_Hd2dOpeningActive(void){assert(enabled);return active;}
int PcPort_Hd2dCurrentMap(void){assert(enabled);return map;}
int main(void){
 XgHd2dFrame f={0};
 assert(!PcPort_Hd2dSceneInputs(0,640,480,(void*)(uintptr_t)1));
 enabled=1;f.field_active=1;f.field_id=14;f.width=320;f.height=240;f.x=10;f.y=20;
 assert(PcPort_Hd2dSceneInputs(0,640,480,&f));assert(f.light_count==2 && !f.shadow_count && !f.particle_count);
 f.depth_mode=XG_HD2D_DEPTH_OT_ORDER;f.protection_mode=XG_HD2D_PROTECTION_COVERAGE_BITS;
 f.linear_depth_verified=f.protection_verified=1;f.order_scale=16;
 assert(PcPort_Hd2dSceneInputs(0,640,480,&f));assert(f.particle_count==6 && f.particle_mode==XG_HD2D_PARTICLE_OT_ORDER);
 for(unsigned i=0;i<f.light_count;++i)assert(isfinite(f.lights[i].x) && isfinite(f.lights[i].y) && f.lights[i].z>0 && f.lights[i].w>0);
 XgHd2dFrame before=f;PcPort_Hd2dSceneItemsReset();assert(PcPort_Hd2dSceneInputs(0,640,480,&f));assert(!memcmp(&before,&f,sizeof f));
 for(unsigned i=0;i<f.particle_count;++i)assert(f.particles[i].z>=0 && f.particles[i].z<=1 && f.particles[i].w>0);
 assert(f.x==10 && f.y==20 && f.width==320 && f.order_scale==16);
 assert(!PcPort_Hd2dSceneInputs(1,640,480,&f));assert(!f.light_count && !f.particle_count);
 active=0;assert(!PcPort_Hd2dSceneInputs(0,640,480,&f));active=1;
 map=99;f.field_id=99;assert(!PcPort_Hd2dSceneInputs(0,640,480,&f));
 map=1;f.field_id=1;assert(PcPort_Hd2dSceneInputs(0,640,480,&f));
 f.linear_depth_verified=0;assert(PcPort_Hd2dSceneInputs(0,640,480,&f));assert(!f.particle_count);
 f.field_id=14;assert(!PcPort_Hd2dSceneInputs(0,640,480,&f));assert(!f.light_count && !f.particle_count);f.field_id=1;
 f.x=500;assert(!PcPort_Hd2dSceneInputs(0,640,480,&f));
}
