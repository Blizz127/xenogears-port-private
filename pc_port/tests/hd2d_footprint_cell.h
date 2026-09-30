#ifndef HD2D_FOOTPRINT_CELL_H
#define HD2D_FOOTPRINT_CELL_H
/* Legacy diagnostic label only; never an authored replacement-art cell. */
static int Hd2dFootprintLegacyCell(unsigned angle,int frame,int animation,int valid) {
    int cell;
    if(!valid || frame<0 || frame>32767 || animation<0 || animation>2)return -1;
    angle &= 4095;
    cell=angle<512 || angle>=3584 ? 6 : angle<1536 ? 0 : angle<2560 ? 4 : 2;
    return cell+((animation!=0 && ((frame/2)&1)) ? 1 : 0);
}
#endif
