#ifndef HD2D_POSE_BATCH_CLOCK_H
#define HD2D_POSE_BATCH_CLOCK_H
/* Absolute presentation schedule, no acceptance-dependent window selection. */
static unsigned Hd2dBatchStart(unsigned settled){return ((settled+23u)/24u)*24u;}
static int Hd2dBatchIndex(unsigned present,unsigned start,unsigned targets,unsigned triples){
 if(!triples || !targets || present<start)return -1;
 unsigned n=(present-start)/(24u*triples);return n<targets?(int)n:-1;
}
#endif
