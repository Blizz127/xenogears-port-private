#include "hd2d_footprint_cell.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 unsigned angles[]={0,511,512,1535,1536,2559,2560,3583,3584,4095,4096};
 int expected[]={6,6,0,0,4,4,2,2,6,6,6};
 for(unsigned j=0;j<sizeof(angles)/sizeof(*angles);++j){
  assert(Hd2dFootprintLegacyCell(angles[j],190,0,1)==expected[j]);
  assert(Hd2dFootprintLegacyCell(angles[j],190,1,1)==expected[j]+1);
  assert(Hd2dFootprintLegacyCell(angles[j],168,2,1)==expected[j]);
 }
 assert(Hd2dFootprintLegacyCell(1024,190,0,1)==0);
 assert(Hd2dFootprintLegacyCell(0,168,0,0)==-1);
 assert(Hd2dFootprintLegacyCell(0,-1,0,1)==-1);
 assert(Hd2dFootprintLegacyCell(0,32768,0,1)==-1);
 assert(Hd2dFootprintLegacyCell(0,168,-1,1)==-1);
 assert(Hd2dFootprintLegacyCell(0,168,3,1)==-1);
 puts("PASS diagnostic angle boundaries, walking parity, front190 and invalid metadata");
}
