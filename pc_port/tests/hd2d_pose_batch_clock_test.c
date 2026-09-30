#include "hd2d_pose_batch_clock.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(Hd2dBatchStart(601)==624);assert(Hd2dBatchStart(624)==624);
 assert(Hd2dBatchIndex(623,624,2,1)==-1);
 for(unsigned n=624;n<648;n++)assert(Hd2dBatchIndex(n,624,2,1)==0);
 for(unsigned n=648;n<672;n++)assert(Hd2dBatchIndex(n,624,2,1)==1);
 assert(Hd2dBatchIndex(672,624,2,1)==-1);
 assert(Hd2dBatchIndex(624,624,0,1)==-1);assert(Hd2dBatchIndex(624,624,2,0)==-1);
 for(unsigned n=0;n<137;n++){unsigned p=624+n*72;assert(Hd2dBatchIndex(p+6,624,137,3)==(int)n);assert(Hd2dBatchIndex(p+70,624,137,3)==(int)n);}
 puts("PASS deterministic complete triple windows, missing/invalid/outside boundaries");
}
