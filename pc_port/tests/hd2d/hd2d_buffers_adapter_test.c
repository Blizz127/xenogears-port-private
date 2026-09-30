#include "hd2d_buffers_adapter.h"
#include "hd2d_sprite.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int enabled;
int PcPort_Hd2dEnabled(void) { return enabled; }
static PcPortHd2dBuffers b={11,22,640,480,3,4095};
static XgHd2dFrame f;
static void offer(void) { PcPort_Hd2dFrameBuffers(&b); }
static int take(int path) { return PcPort_Hd2dBuffersConsume(path,640,480,&f); }
int main(void) {
    PcPort_Hd2dFrameBuffers((const PcPortHd2dBuffers *)(uintptr_t)1);
    assert(!PcPort_Hd2dBuffersConsume(0,640,480,(XgHd2dFrame *)(uintptr_t)1));
    enabled=1;PcPort_Hd2dBuffersReset();
    f.field_active=1;f.width=320;f.height=240;f.x=10;f.y=20;f.particle_count=7;
    offer();assert(take(0));assert(f.linear_depth_verified && f.protection_verified);
    assert(f.linear_depth_texture==11 && f.protection_texture==22 && f.order_scale==65535.0f/4095);
    assert(f.auxiliary_width==640 && f.auxiliary_height==480 && f.x==10 && f.particle_count==7);
    assert(f.depth_mode==XG_HD2D_DEPTH_OT_ORDER && f.protection_mode==XG_HD2D_PROTECTION_COVERAGE_BITS);
    assert(!take(0) && !f.linear_depth_verified && !f.protection_verified);
    b.flags=1;offer();assert(take(0) && f.linear_depth_verified && !f.protection_verified);
    b.flags=2;offer();assert(take(0) && !f.linear_depth_verified && f.protection_verified);
    b.flags=3;
    for(unsigned i=0;i<2;i++) { b.depth_max=i?65536:0;offer();assert(take(0));assert(!f.linear_depth_verified && f.protection_verified); }
    b.depth_max=65535;offer();assert(take(0) && f.order_scale==1);
    offer();assert(!take(1));assert(!take(0));
    offer();PcPort_Hd2dFrameBuffers(NULL);assert(!take(0));
    offer();assert(!PcPort_Hd2dBuffersConsume(0,640,480,NULL));assert(!take(0));
    offer();f.field_active=0;assert(!take(0));f.field_active=1;assert(!take(0));
    b.width=639;offer();assert(!take(0));b.width=640;
    f.x=400;offer();assert(!take(0));f.x=10;
    offer();enabled=0;PcPort_Hd2dBuffersReset();enabled=1;PcPort_Hd2dBuffersReset();assert(!take(0));
    b.flags=0;offer();assert(!take(0));
    b.flags=3;b.depth_tex=0;offer();assert(take(0) && !f.linear_depth_verified);b.depth_tex=11;
    b.coverage_tex=0;offer();assert(take(0) && !f.protection_verified);
    puts("hd2d buffers adapter: CPU-only independent planes, consume-once, OFF and expiry PASS");
}
