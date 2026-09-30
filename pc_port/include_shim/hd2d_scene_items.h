#ifndef PC_PORT_HD2D_SCENE_ITEMS_H
#define PC_PORT_HD2D_SCENE_ITEMS_H
#include "../include/xg_plat/hd2d.h"
/* CPU-only in/out: caller supplies verified current viewport/field metadata.
 * Adds original area art-direction lights and ordinal dust; no retail physics
 * or inferred feet. Call after Hook4 consume. No contact shadows without
 * verified projection/order observations. OFF never inspects output. */
/* Stateless source: explicit reset is provided for bridge lifecycle symmetry. */
void PcPort_Hd2dSceneItemsReset(void);
int PcPort_Hd2dSceneInputs(int path,int drawable_width,int drawable_height,XgHd2dFrame *frame);
#endif
