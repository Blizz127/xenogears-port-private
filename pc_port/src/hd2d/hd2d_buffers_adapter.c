/* HOOK4 metadata only: borrowed textures; no GL calls or allocations. */
#include "../../include_shim/hd2d_buffers_adapter.h"
#include "../../include_shim/hd2d_sprite.h"
#include "../../include_shim/hd2d_integration.h"
#include <string.h>
static PcPortHd2dBuffers pending;
static int available;
void PcPort_Hd2dBuffersReset(void)
{
    available=0;
    memset(&pending,0,sizeof pending);
}
void PcPort_Hd2dFrameBuffers(const PcPortHd2dBuffers *buffers)
{
    if (!PcPort_Hd2dEnabled()) return; /* before pointer/state inspection */
    PcPort_Hd2dBuffersReset();
    if (!buffers) return;
    pending=*buffers;
    available=1;
}
int PcPort_Hd2dBuffersConsume(int path,int drawable_width,int drawable_height,
                            XgHd2dFrame *frame)
{
    PcPortHd2dBuffers b;
    int had;
    if (!PcPort_Hd2dEnabled()) return 0; /* even an invalid frame is harmless */
    b=pending; had=available;
    PcPort_Hd2dBuffersReset(); /* expire even NULL, path1, or out-of-scope */
    if (!frame) return 0;
    frame->linear_depth_texture=frame->protection_texture=0;
    frame->linear_depth_verified=frame->protection_verified=0;
    frame->auxiliary_width=frame->auxiliary_height=0;
    frame->order_scale=0;
    frame->depth_mode=XG_HD2D_DEPTH_OT_ORDER;
    frame->protection_mode=XG_HD2D_PROTECTION_COVERAGE_BITS;
    if (!had || path!=0 || !frame->field_active || drawable_width<=0 || drawable_height<=0 ||
        b.width!=drawable_width || b.height!=drawable_height || frame->width<=0 || frame->height<=0 ||
        frame->x<0 || frame->y<0 || frame->width>drawable_width || frame->height>drawable_height ||
        frame->x>drawable_width-frame->width || frame->y>drawable_height-frame->height) return 0;
    frame->auxiliary_width=b.width; frame->auxiliary_height=b.height;
    if ((b.flags & PC_PORT_HD2D_BUFFERS_DEPTH) && b.depth_tex && b.depth_max>=1 && b.depth_max<=65535) {
        frame->linear_depth_texture=b.depth_tex;
        frame->linear_depth_verified=1;
        frame->order_scale=65535.0f/(float)b.depth_max;
    }
    if ((b.flags & PC_PORT_HD2D_BUFFERS_COVERAGE) && b.coverage_tex) {
        frame->protection_texture=b.coverage_tex;
        frame->protection_verified=1;
    }
    return frame->linear_depth_verified || frame->protection_verified;
}
