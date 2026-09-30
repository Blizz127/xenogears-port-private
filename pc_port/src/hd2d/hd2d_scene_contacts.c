#include <assert.h>
#include "common.h"
#include "psyq/libgpu.h"
#include "hd2d_scene_contacts.h"
#include "hd2d_integration.h"
#include <math.h>
#include <string.h>
static struct Group {int used,sealed,bad,map;PcPortHd2dSpriteInfo info;unsigned count,seen,slot;const void *packet[6];POLY_FT4 snapshot[6];float box[4];} groups[HD2D_CONTACT_MAX];
static int building=-1;
static int SameInfo(const PcPortHd2dSpriteInfo *a,const PcPortHd2dSpriteInfo *b){
 return a && b && a->identity==b->identity && a->frame==b->frame && a->actor==b->actor && a->sprite==b->sprite;
}
void PcPort_Hd2dSceneContactsReset(void){memset(groups,0,sizeof groups);building=-1;}
void PcPort_Hd2dSceneContactBegin(const PcPortHd2dSpriteInfo *info){
 unsigned i;if(!PcPort_Hd2dEnabled())return;building=-1;
 if(!PcPort_Hd2dOpeningActive() || !info || info->identity!=0xdb7d0105 || info->actor!=1 || info->frame!=190)return;
 for(i=0;i<HD2D_CONTACT_MAX;++i)if(groups[i].bad)memset(groups+i,0,sizeof groups[i]);
 for(i=0;i<HD2D_CONTACT_MAX;++i)if(!groups[i].used){struct Group *g=groups+i;memset(g,0,sizeof *g);g->used=1;g->map=PcPort_Hd2dCurrentMap();g->info=*info;building=(int)i;break;}
}
void PcPort_Hd2dSceneContactDraw(const PcPortHd2dSpriteInfo *info,int part,const PcPortHd2dRetailQuad *q){
 unsigned i,j;struct Group *g;if(!PcPort_Hd2dEnabled())return;
 if(building<0)return;
 g=groups+building;
 if(!SameInfo(info,&g->info) || !q || !q->poly || part!=(int)g->count || part>=6){g->bad=1;return;}
 for(i=0;i<HD2D_CONTACT_MAX;++i)for(j=0;j<groups[i].count;++j)if(groups[i].packet[j]==q->poly)groups[i].bad=1;
 g->packet[part]=q->poly;memcpy(g->snapshot+part,q->poly,sizeof(POLY_FT4));g->count++;
}
void PcPort_Hd2dSceneContactEnd(const PcPortHd2dSpriteInfo *info){
 struct Group *g;if(!PcPort_Hd2dEnabled())return;if(building<0)return;
 g=groups+building;
 g->sealed=SameInfo(info,&g->info) && g->count==6 && !g->bad;
 if(!g->sealed)g->bad=1;
 building=-1;
}
void PcPort_Hd2dSceneContactsGpuBegin(void){
 unsigned i;if(!PcPort_Hd2dEnabled())return;
 for(i=0;i<HD2D_CONTACT_MAX;++i){groups[i].seen=0;groups[i].slot=0;}
}
void PcPort_Hd2dSceneContactGpuPart(const void *packet,const float *b,unsigned slot){
 unsigned i,j;if(!PcPort_Hd2dEnabled())return;
 if(!packet || !b || slot<1 || slot>65535)return;
 for(i=0;i<HD2D_CONTACT_MAX;++i){struct Group *g=groups+i;
  if(!g->used || !g->sealed || g->bad)continue;
  for(j=0;j<g->count;++j)if(g->packet[j]==packet){POLY_FT4 a=g->snapshot[j],v;memcpy(&v,packet,sizeof v);
   ((P_TAG*)&a)->addr=((P_TAG*)&v)->addr=0; /* only mutable OT link address ignored */
   if(memcmp(&a,&v,sizeof a) || !g->sealed || g->bad || g->map!=PcPort_Hd2dCurrentMap() ||
      (g->seen&(1u<<j)) || (g->slot && g->slot!=slot)) {g->bad=1;return;}
   for(unsigned k=0;k<4;++k)if(!isfinite(b[k]) || b[k]<0 || b[k]>1){g->bad=1;return;}
   if(b[0]>=b[2] || b[1]>=b[3]){g->bad=1;return;}
   if(!g->seen)memcpy(g->box,b,sizeof g->box);else {g->box[0]=fminf(g->box[0],b[0]);g->box[1]=fminf(g->box[1],b[1]);g->box[2]=fmaxf(g->box[2],b[2]);g->box[3]=fmaxf(g->box[3],b[3]);}
   g->seen|=1u<<j;g->slot=slot;return;
  }
 }
}
unsigned PcPort_Hd2dSceneContactsTake(int map,unsigned max,PcPortHd2dSceneContact *out){
 unsigned i,n=0;if(!PcPort_Hd2dEnabled())return 0;
 if(!out || max<1 || max>65535){PcPort_Hd2dSceneContactsReset();return 0;}
 for(i=0;i<HD2D_CONTACT_MAX;++i){struct Group *g=groups+i;
  if(g->used && g->sealed && !g->bad && g->seen==63 && g->map==map && g->slot<=max){out[n++]=(PcPortHd2dSceneContact){(g->box[0]+g->box[2])*.5f,g->box[1],(g->box[2]-g->box[0])*.35f,(g->box[3]-g->box[1])*.035f,g->slot};}
  if(g->seen || g->bad || g->map!=map)memset(g,0,sizeof *g);
 }
 return n;
}
