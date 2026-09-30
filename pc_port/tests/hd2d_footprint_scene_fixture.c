/* Test-only Fei footprint isolation. Nonplayer sprite actors are omitted in
 * ALL retail/HD/hidden blocks; field geometry and the player's real renderer,
 * packets and OT remain unchanged. Never linked into production. This avoids
 * attributing independently moving NPC pixels to the retail-minus-hidden Fei
 * mask. Full-cast OFF identity is tested separately without this fixture. */
#include "common.h"
#include "field/actor.h"
#include "quick_checkpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int g_FieldNumActors,g_PlayerActorIndex,g_GameSceneMapNum;
extern unsigned g_PcPortPresentedFrames;
extern void __real_func_8001E298(void*,void*);
void __wrap_func_8001E298(void *sprite,void *ot)
{
    const char *on=getenv("XENO_HD2D_FOOTPRINT_ISOLATE_PLAYER");
    if (on && !strcmp(on,"1") && PcPort_QuickCheckpointFieldIsActive() &&
        g_FieldActors && g_FieldNumActors>0 && g_FieldNumActors<=1536 &&
        g_PlayerActorIndex>=0 && g_PlayerActorIndex<g_FieldNumActors) {
        /* A shared package/actor pointer must never hide the player. */
        if(sprite && (uintptr_t)sprite==g_FieldActors[g_PlayerActorIndex].pSpriteData) {
            __real_func_8001E298(sprite,ot);
            return;
        }
        for(int actor=0;actor<g_FieldNumActors;actor++)
            if(sprite && (uintptr_t)sprite==g_FieldActors[actor].pSpriteData) {
                if(actor!=g_PlayerActorIndex) {
                    fprintf(stderr,"[hd2d-footprint-scene] present=%u map=%d omitted_nonplayer_actor=%d\n",
                        g_PcPortPresentedFrames+1,g_GameSceneMapNum&0x3fff,actor);
                    return;
                }
                break;
            }
    }
    __real_func_8001E298(sprite,ot);
}
