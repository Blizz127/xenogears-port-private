/* PS1 GPU polygon size limit (gpu_polygon_size_rule.h, applied to PsyCross
 * by psycross_gpu_polygon_size_limit.patch).  Cases follow the hardware
 * rule: vertices are signed 11-bit, and a triangle spanning >= 1024 pixels
 * horizontally or >= 512 vertically is not drawn; a quad is the triangles
 * v0-v1-v2 and v1-v2-v3. */
#include <stdio.h>
#include "gpu_polygon_size_rule.h"

static int fails;
#define CHECK(expr, want) do { int got_ = (expr); if (got_ != (want)) { \
    printf("FAIL %s: got %d want %d\n", #expr, got_, (want)); fails++; } } while (0)

int main(void)
{
    /* horizontal: 1023 apart is drawn, 1024 is not */
    CHECK(xeno_gpu_triangle_too_large(0, 0, 1023, 0, 0, 10), 0);
    CHECK(xeno_gpu_triangle_too_large(0, 0, 1023, 0, -1, 10), 1);
    CHECK(xeno_gpu_triangle_too_large(-512, 0, 511, 0, 0, 1), 0);
    CHECK(xeno_gpu_triangle_too_large(-512, 0, 512, 0, 0, 1), 1);
    /* vertical: 511 apart is drawn, 512 is not */
    CHECK(xeno_gpu_triangle_too_large(0, 0, 5, 511, 9, 3), 0);
    CHECK(xeno_gpu_triangle_too_large(0, -256, 5, 256, 9, 3), 1);
    /* GTE-saturated extremes (SX -0x400 .. 0x3FF) */
    CHECK(xeno_gpu_triangle_too_large(-1024, 100, 1023, 100, 0, 120), 1);
    CHECK(xeno_gpu_triangle_too_large(-1024, 100, -1, 100, -2, 120), 0);
    CHECK(xeno_gpu_triangle_too_large(-1024, 100, -1, 100, 0, 120), 1);
    /* 11-bit wrap: 1100 is read as -948, so (1100, 0)-(0, 0) spans 948 */
    CHECK(xeno_gpu_coord11(1100), -948);
    CHECK(xeno_gpu_coord11(-1025), 1023);
    CHECK(xeno_gpu_triangle_too_large(1100, 0, 0, 0, 0, 5), 0);
    /* a quad whose second half (v1 v2 v3) alone is too wide */
    {
        int x0 = 0, y0 = 0, x1 = 300, y1 = 0, x2 = 0, y2 = 40, x3 = -800, y3 = 40;
        CHECK(xeno_gpu_triangle_too_large(x0, y0, x1, y1, x2, y2), 0);
        CHECK(xeno_gpu_triangle_too_large(x1, y1, x2, y2, x3, y3), 1);
    }
    if (fails) { printf("GPU POLYGON SIZE LIMIT TEST FAIL (%d)\n", fails); return 1; }
    puts("GPU POLYGON SIZE LIMIT TEST PASS");
    return 0;
}
