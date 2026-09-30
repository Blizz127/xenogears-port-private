/* Original screen-space art direction, deliberately distinct from world light
 * reconstruction. No GL, allocation, retail-data access or animation state. */
#include "hd2d_scene_items.h"
#include "hd2d_integration.h"
#include "hd2d_scene_contacts.h"
#pragma weak PcPort_Hd2dSceneContactsTake
#include <math.h>
void PcPort_Hd2dSceneItemsReset(void) { /* No retained scene state. */ }
int PcPort_Hd2dSceneInputs(int path,int width,int height,XgHd2dFrame *f)
{
    int map;
    if(!PcPort_Hd2dEnabled()) return 0;
    if(!f) return 0;
    f->light_count=f->shadow_count=f->particle_count=0;
    if(path!=0 || !PcPort_Hd2dOpeningActive() || width<=0 || height<=0 ||
       !f->field_active || f->width<=0 || f->height<=0 || f->x<0 || f->y<0 ||
       f->width>width || f->height>height || f->x>width-f->width || f->y>height-f->height) return 0;
    map=PcPort_Hd2dCurrentMap();
    if(f->field_id!=map || (map!=1 && map!=14)) return 0;
    /* Authored upper-left warm key and restrained fill for the two approved
     * opening areas. Positions are art direction, not observed game lamps. */
    f->light_count=2;
    f->lights[0]=(XgHd2dItem){.18f,.86f,.95f,1.0f};
    f->lights[1]=(XgHd2dItem){.78f,.68f,.55f,.35f};
    /* Fixed original ambient dust motif. z is ordinal distance, never metres.
     * Require current certified OT depth/coverage; shader performs occlusion. */
    if(f->depth_mode==XG_HD2D_DEPTH_OT_ORDER && f->linear_depth_verified &&
       f->protection_verified && f->protection_mode==XG_HD2D_PROTECTION_COVERAGE_BITS &&
       isfinite(f->order_scale) && f->order_scale>0) {
        static const XgHd2dItem dust[6]={
            {.17f,.32f,.25f,.0025f},{.31f,.66f,.4f,.002f},
            {.48f,.44f,.6f,.0025f},{.63f,.78f,.3f,.002f},
            {.76f,.28f,.55f,.0025f},{.86f,.57f,.7f,.002f}};
        unsigned i;
        f->particle_mode=XG_HD2D_PARTICLE_OT_ORDER;f->particle_count=6;
        for(i=0;i<6;++i) f->particles[i]=dust[i];
        if(PcPort_Hd2dSceneContactsTake) {
            float maximum=65535.0f/f->order_scale;
            if(maximum>=1 && maximum<=65535) {
                PcPortHd2dSceneContact contacts[HD2D_CONTACT_MAX];
                unsigned depth_max=(unsigned)lroundf(maximum);
                f->shadow_count=PcPort_Hd2dSceneContactsTake(map,depth_max,contacts);
                f->shadow_mode=XG_HD2D_SHADOW_OT_CONTACT;
                for(i=0;i<f->shadow_count;++i) {
                    f->shadows[i]=(XgHd2dItem){contacts[i].x,contacts[i].y,contacts[i].radius_x,contacts[i].radius_y};
                    f->shadow_order[i]=1.0f-(float)contacts[i].raw_slot/depth_max;
                }
            }
        }
    }
    return 1;
}
