/* Test-only observation of the real native scene provider, no input changes. */
#include "hd2d_scene_items.h"
#include "hd2d_integration.h"
#include <stdio.h>
#include <stdlib.h>
extern unsigned g_PcPortPresentedFrames;
extern int __real_PcPort_Hd2dSceneInputs(int,int,int,XgHd2dFrame*);
int __wrap_PcPort_Hd2dSceneInputs(int path,int width,int height,XgHd2dFrame *frame)
{
    int result=__real_PcPort_Hd2dSceneInputs(path,width,height,frame);
    const char *target=getenv("XENO_HD2D_SCENE_PROBE");
    if(target && frame) {
        static FILE *out;
        if(!out) out=fopen(target,"w");
        if(out) {
            XgHd2dPlan plan=XgHd2d_Plan(PcPort_Hd2dConfiguration(),frame);
            fprintf(out,"scene present=%u map=%d result=%d depth=%d protection=%d lights=%u shadows=%u particles=%u shadow_mode=%d active=%x\n",
                g_PcPortPresentedFrames+1,frame->field_id,result,frame->linear_depth_verified,
                frame->protection_verified,frame->light_count,frame->shadow_count,
                frame->particle_count,frame->shadow_mode,plan.active);
            fflush(out);
        }
    }
    return result;
}
