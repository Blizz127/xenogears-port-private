/* Test-only read-only runtime facts. Link --wrap=PcPort_FeiHd2dBegin alongside
 * field_probe. Output path HD2D_ANIMATION_METADATA_LOG must be private/local.
 * No frame/pose writes, acceptance overrides, retail pixels or state dumps. */
#include "common.h"
#include "field/actor.h"
#include "fei_hd2d.h"
#include "quick_checkpoint.h"
#include "psx_memory.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int g_GameSceneMapNum,g_PlayerActorIndex,g_FieldNumActors;
extern unsigned g_PcPortPresentedFrames;
static FILE *output;
static unsigned seen[64],seen_count;
static const unsigned char *valid(uintptr_t addr,size_t bytes) {
 uintptr_t base=(uintptr_t)g_PsxRam;
 return addr>=base && bytes<=PSX_RAM_SIZE && addr-base<=PSX_RAM_SIZE-bytes ? (const unsigned char*)addr:NULL;
}
static unsigned short read16(const void *p){unsigned short v;memcpy(&v,p,2);return v;}
static unsigned read32(const void *p){unsigned v;memcpy(&v,p,4);return v;}
static unsigned hash(const unsigned char *p,size_t n){unsigned h=2166136261u;while(n--){h^=*p++;h*=16777619u;}return h;}
static void topology(const unsigned char *s,unsigned fnv) {
 const unsigned char *pkg=valid(read32(s+0x24),20),*table;
 unsigned count,header;
 for(unsigned i=0;i<seen_count;i++)if(seen[i]==fnv)return;
 if(!pkg || !(table=valid(read32(pkg),2)))return;
 header=read16(table);count=(header&511)+1;
 if(!valid((uintptr_t)table,count*2))return;
 if(seen_count>=64){fprintf(output,"metadata_error reason=identity_capacity\n");return;}
 seen[seen_count++]=fnv;
 const unsigned char *animations=valid(read32(pkg+0x10),2);
 fprintf(output,"package fnv=%08x decoder_index_limit=%u frame_header=%u descriptor_bytes=%u animations_flags=%d animations_low6=%d\n",fnv,count,header,header&0x8000?2:4,animations?(int)read16(animations):-1,animations?(int)(read16(animations)&63):-1);
 for(unsigned i=0;i<count;i++) {
  unsigned offset=read16(table+2*i),prefix=header&0x8000?4:6,stride=header&0x8000?2:4;
  const unsigned char *frame=valid((uintptr_t)table+offset,prefix);unsigned n;
  if(!frame){fprintf(output,"topology fnv=%08x index=%u valid=0\n",fnv,i);continue;}
  n=frame[0]&63;
  if(!valid((uintptr_t)frame,prefix+n*stride)){fprintf(output,"topology fnv=%08x index=%u valid=0\n",fnv,i);continue;}
  fprintf(output,"topology fnv=%08x index=%u offset=%u descriptor_parts=%u wide_offsets=%u descriptor_hash=%08x valid=1\n",fnv,i,offset,n,!!(frame[0]&128),hash(frame+prefix,n*stride));
 }
}
extern void __real_PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch*,void*,int);
void __wrap_PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch *b,void *sprite,int parts) {
 const unsigned char *s=valid((uintptr_t)sprite,0xb4);ActorData *a=NULL;
 if(s && PcPort_QuickCheckpointFieldIsActive() && g_FieldActors && g_PlayerActorIndex>=0 && g_PlayerActorIndex<g_FieldNumActors &&
    (uintptr_t)sprite==g_FieldActors[g_PlayerActorIndex].pSpriteData)
  a=(ActorData*)valid(g_FieldActors[g_PlayerActorIndex].pActorData,sizeof(ActorData));
 if(a && a->characterId==0) {
  if(!output){const char *p=getenv("HD2D_ANIMATION_METADATA_LOG");if(p && *p)output=fopen(p,"w");}
  if(output){
   unsigned fnv=PcPort_Hd2dSpriteIdentity(sprite);const unsigned char *anim=valid(read32(s+0x58),6);
   topology(s,fnv);
   fprintf(output,"sprite present=%u map=%d actor=%d fnv=%08x frame=%d parts=%d animation=%d angle=%u direction=%d flags3c=%08x flags40=%08x flagsac=%08x animation_flags=%d safe=%d\n",
     g_PcPortPresentedFrames+1,g_GameSceneMapNum&0x3fff,g_PlayerActorIndex,fnv,(short)read16(s+0x34),parts,a->curAnimationId,read16(s+0x80)&4095,(short)read16(s+0x32),read32(s+0x3c),read32(s+0x40),read32(s+0xac),anim?(int)read16(anim):-1,PcPort_QuickCheckpointFieldIsSafe());
   fflush(output);
  }
 }
 __real_PcPort_FeiHd2dBegin(b,sprite,parts);
}
