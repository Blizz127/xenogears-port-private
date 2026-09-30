/* gpu_polygon_size_rule.h -- the PS1 GPU's polygon size limit.
 *
 * The GPU takes vertex coordinates as signed 11-bit values (-1024..1023) and
 * draws nothing for a triangle whose vertices lie 1024 or more pixels apart
 * horizontally, or 512 or more vertically.  A quad is rasterised as the two
 * triangles (v0, v1, v2) and (v1, v2, v3), each tested on its own.  Games rely
 * on this: the Xenogears world map submits terrain and road triangles whose
 * vertices the GTE left unflagged but that span the screen when a camera turn
 * brings them close to the eye, and the PS1 never shows them.
 *
 * PsyCross drew every triangle, so those came out as long spikes across the
 * screen.  The rule is kept here, in plain C, so it can be tested on its own;
 * pc_port/patches/psycross_gpu_polygon_size_limit.patch applies it. */
#ifndef XENO_GPU_POLYGON_SIZE_RULE_H
#define XENO_GPU_POLYGON_SIZE_RULE_H

/* Sign-extend the low 11 bits, as the GPU reads a vertex coordinate. */
static inline int xeno_gpu_coord11(int v)
{
    v &= 0x7FF;
    return (v & 0x400) ? v - 0x800 : v;
}

/* Nonzero when the GPU would not draw triangle (x0,y0) (x1,y1) (x2,y2). */
static inline int xeno_gpu_triangle_too_large(int x0, int y0, int x1, int y1,
                                              int x2, int y2)
{
    int ax = xeno_gpu_coord11(x0), bx = xeno_gpu_coord11(x1), cx = xeno_gpu_coord11(x2);
    int ay = xeno_gpu_coord11(y0), by = xeno_gpu_coord11(y1), cy = xeno_gpu_coord11(y2);
    int min_x = ax < bx ? ax : bx, max_x = ax > bx ? ax : bx;
    int min_y = ay < by ? ay : by, max_y = ay > by ? ay : by;

    if (cx < min_x) min_x = cx;
    if (cx > max_x) max_x = cx;
    if (cy < min_y) min_y = cy;
    if (cy > max_y) max_y = cy;
    return (max_x - min_x) >= 1024 || (max_y - min_y) >= 512;
}

#endif
