#include "hd2d_pose_replay_memory.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 unsigned char live[16],original[16],pose[16];
 for(int i=0;i<16;i++){live[i]=i;pose[i]=i+32;}memcpy(original,live,16);
 Hd2dPoseRange r[2]={{live,pose,12,{0}},{live+4,pose+4,12,{0}}};
 assert(Hd2dPoseApply(r,2));assert(!memcmp(live,pose,16));memset(live,77,16);
 Hd2dPoseRestore(r,2);assert(!memcmp(live,original,16));
 r[1].live=NULL;assert(!Hd2dPoseApply(r,2));assert(!memcmp(live,original,16));
 r[1].live=live+4;r[1].size=HD2D_POSE_BYTES+1;assert(!Hd2dPoseApply(r,2));assert(!memcmp(live,original,16));
 assert(!Hd2dPoseApply(NULL,1));assert(!Hd2dPoseApply(r,HD2D_POSE_RANGES+1));
 puts("PASS replay transaction restores aliases/renderer writes; invalid plans leave live memory untouched");
}
