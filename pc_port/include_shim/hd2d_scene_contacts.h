#ifndef HD2D_SCENE_CONTACTS_H
#define HD2D_SCENE_CONTACTS_H
#include "hd2d_sprite.h"
#define HD2D_CONTACT_MAX 8
typedef struct {float x,y,radius_x,radius_y;unsigned raw_slot;} PcPortHd2dSceneContact;
void PcPort_Hd2dSceneContactsReset(void);
void PcPort_Hd2dSceneContactBegin(const PcPortHd2dSpriteInfo*);
void PcPort_Hd2dSceneContactDraw(const PcPortHd2dSpriteInfo*,int,const PcPortHd2dRetailQuad*);
void PcPort_Hd2dSceneContactEnd(const PcPortHd2dSpriteInfo*);
void PcPort_Hd2dSceneContactsGpuBegin(void);
void PcPort_Hd2dSceneContactGpuPart(const void*,const float *bounds,unsigned raw_slot);
/* Complete pre-occlusion quad-bottom geometry proxy, NOT anatomical evidence.
 * Only verified Fei idle cursor190/sixparts until further certified families.
 * Consume once; normalize raw_slot by actual Hook4 depth_max in integration. */
unsigned PcPort_Hd2dSceneContactsTake(int map,unsigned depth_max,PcPortHd2dSceneContact *out);
#endif
