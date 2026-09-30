/* Reuse the unchanged strict packet/GPU/alpha observer. Only select its exact
 * expected metadata dynamically from the actively replayed observed pose. */
#define __wrap_PcPort_Hd2dSpriteBegin BatchStrictBegin
#include "hd2d_footprint_probe.c"
#undef __wrap_PcPort_Hd2dSpriteBegin
extern int PcPort_Hd2dBatchExpected(int*,int*,unsigned*,int*,int*);
int __wrap_PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo *i){
 if(!PcPort_Hd2dBatchExpected(&expected_map,&expected_actor,&expected_fnv,&expected_frame,&expected_parts)){
  PcPort_Hd2dSetEnabled(0);return __real_PcPort_Hd2dSpriteBegin(i);
 }
 configured=1;return BatchStrictBegin(i);
}
