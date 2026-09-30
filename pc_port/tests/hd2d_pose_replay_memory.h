#ifndef HD2D_POSE_REPLAY_MEMORY_H
#define HD2D_POSE_REPLAY_MEMORY_H
#include <stddef.h>
#include <string.h>
#define HD2D_POSE_RANGES 5
#define HD2D_POSE_BYTES 1600
/* Snapshot ALL originals before writing: aliases restore original bytes too. */
typedef struct {void *live;const void *pose;size_t size;unsigned char saved[HD2D_POSE_BYTES];} Hd2dPoseRange;
static int Hd2dPoseApply(Hd2dPoseRange *r,size_t n) {
 if(!r || n>HD2D_POSE_RANGES)return 0;
 for(size_t i=0;i<n;i++)if(!r[i].live || !r[i].pose || !r[i].size || r[i].size>HD2D_POSE_BYTES)return 0;
 for(size_t i=0;i<n;i++)memcpy(r[i].saved,r[i].live,r[i].size);
 for(size_t i=0;i<n;i++)memmove(r[i].live,r[i].pose,r[i].size);
 return 1;
}
static void Hd2dPoseRestore(Hd2dPoseRange *r,size_t n) {
 for(size_t i=n;i>0;i--)memcpy(r[i-1].live,r[i-1].saved,r[i-1].size);
}
#endif
