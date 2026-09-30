/* Test-only HOOK 4 lane for run_hd2d_buffers_test.sh (never linked into the
 * port build). Defines PcPort_Hd2dFrameBuffers, so the PsyCross producer
 * runs, and the HOOK 2 sprite hooks, which only record the player's parts
 * (and return 0: retail draws them). Links with
 * -Wl,--wrap=ParsePrimitivesLinkedList (PsyCross's OT walker, called by
 * DrawOTag after its scene begin and display placement; DrawOTag itself is
 * already wrapped by xg_plat): the wrapper walks each list before PsyCross
 * does and numbers its entries independently of the producer, so each
 * recorded part gets the depth HOOK 4 must report for it (its OT entry's
 * ordinal in the scene, hd2d_sprite.h).
 *
 * XENO_HD2D_BUFFERS_TEST_DIR=dir: once XENO_HD2D_BUFFERS_TEST_SETTLE (90)
 * presents have shown the player's parts, dumps XENO_HD2D_BUFFERS_TEST_DUMPS
 * (6) presents, every XENO_HD2D_BUFFERS_TEST_EVERY (15) presents, as
 *   frame-N.depth (u16 LE, GL rows bottom-up), frame-N.coverage (u8),
 *   frame-N.color (RGBA8, the default framebuffer: the frame before HOOK 1),
 *   frame-N.txt (sizes, flags, depth_max, and per part: depth, quad, the
 *   display-space opaque pixels from the VRAM mirror),
 * then prints "[hd2d-buffers-test] dumps complete". Readbacks restore every
 * GL binding and pack state they touch.
 * -DHD2D_BUFFERS_TEST_MUTANT: the depth readback is mapped through a
 * reversed OT order (d -> depth_max + 1 - d) before it is written, a
 * producer that numbered the OT backwards; the analysis must FAIL. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <GL/gl.h>
#include "psyq/libgpu.h"
#include "hd2d_sprite.h"

#ifndef GL_READ_FRAMEBUFFER_BINDING
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#endif
#ifndef GL_PACK_ROW_LENGTH
#define GL_PACK_ROW_LENGTH 0x0D02
#endif

extern int g_PlayerActorIndex;
extern int g_GPUDisabledState;
extern unsigned short vram[];
extern DRAWENV activeDrawEnv;
extern DISPENV activeDispEnv;
extern int g_windowWidth, g_windowHeight;
void __real_ParsePrimitivesLinkedList(u_long *p, int singlePrimitive);

typedef struct {
    void *ot, *poly;
    short x[4], y[4];
    unsigned char u[4], v[4];
    unsigned short tpage, clut;
    int part, build, live;
} Part;
typedef struct {
    Part p;
    unsigned depth;          /* expected: the OT entry ordinal of p.ot */
    int ofsX, ofsY, dispW, dispH, dfe;
} ScenePart;

static Part s_pending[128];
static int s_npending, s_build;
static ScenePart s_scene[64];
static int s_nscene;
static unsigned s_count;       /* OT entries walked this scene */
static unsigned s_presents, s_feiPresents, s_dumps, s_nextDump;
static unsigned long s_hook4, s_hook4Valid, s_walks;
static const char *s_dir;
static unsigned s_settle = 90, s_every = 15, s_maxDumps = 6;
static int s_init;

static void Init(void)
{
    const char *e;
    if (s_init) return;
    s_init = 1;
    s_dir = getenv("XENO_HD2D_BUFFERS_TEST_DIR");
    if ((e = getenv("XENO_HD2D_BUFFERS_TEST_SETTLE")) && *e) s_settle = (unsigned)atoi(e);
    if ((e = getenv("XENO_HD2D_BUFFERS_TEST_EVERY")) && *e) s_every = (unsigned)atoi(e);
    if ((e = getenv("XENO_HD2D_BUFFERS_TEST_DUMPS")) && *e) s_maxDumps = (unsigned)atoi(e);
}

/* ---- HOOK 2: record the player's parts (retail draws them) ------------- */
int PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo *info)
{
    if (info->actor >= 0 && info->actor == g_PlayerActorIndex) s_build++;
    return 1;
}

int PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo *info, int part, const PcPortHd2dRetailQuad *q)
{
    Part *p;
    int i;
    if (info->actor < 0 || info->actor != g_PlayerActorIndex || !q->ot || !q->poly) return 0;
    /* drop parts of builds more than two back (their OT was walked or dropped) */
    for (i = 0; i < s_npending; ) {
        if (!s_pending[i].live || s_pending[i].build < s_build - 2) s_pending[i] = s_pending[--s_npending];
        else i++;
    }
    if (s_npending >= (int)(sizeof s_pending / sizeof s_pending[0])) return 0;
    p = &s_pending[s_npending++];
    memset(p, 0, sizeof *p);
    p->ot = q->ot;
    p->poly = q->poly;
    memcpy(p->x, q->x, sizeof p->x);
    memcpy(p->y, q->y, sizeof p->y);
    memcpy(p->u, q->u, sizeof p->u);
    memcpy(p->v, q->v, sizeof p->v);
    p->tpage = q->tpage;
    p->clut = q->clut;
    p->part = part;
    p->build = s_build;
    p->live = 1;
    return 0;
}

void PcPort_Hd2dSpriteEnd(const PcPortHd2dSpriteInfo *info) { (void)info; }

/* ---- independent OT numbering ------------------------------------------- */
/* Number the OT entries of this walk (as the producer must: every
 * zero-length packet from the head to the terminator). */
static void Walk(u_long *head)
{
    uint32_t link = (uint32_t)(uintptr_t)head & 0xFFFFFF;
    int steps = 0, i;
    const int dfe = activeDrawEnv.dfe;
    s_walks++;
    while (steps++ < 1000000) {
        const uint32_t *pkt = (const uint32_t *)(uintptr_t)link;
        if (!pkt) break;
        if ((pkt[0] >> 24) == 0) {
            s_count++;
            for (i = 0; i < s_npending; i++)
                if (s_pending[i].live && (uintptr_t)s_pending[i].ot == (uintptr_t)link && s_nscene < 64) {
                    ScenePart *sp = &s_scene[s_nscene++];
                    sp->p = s_pending[i];
                    sp->depth = s_count > 65535 ? 65535 : s_count;
                    /* PsyCross DrawEnvOffset: display-relative draw offset */
                    sp->dfe = dfe;
                    sp->ofsX = dfe ? activeDrawEnv.ofs[0] - activeDispEnv.disp.x : 0;
                    sp->ofsY = dfe ? activeDrawEnv.ofs[1] - activeDispEnv.disp.y : 0;
                    sp->dispW = activeDispEnv.disp.w;
                    sp->dispH = activeDispEnv.disp.h;
                    s_pending[i].live = 0;
                }
        }
        if ((pkt[0] & 0xFFFFFF) == 0xFFFFFF) break;
        link = pkt[0] & 0xFFFFFF;
    }
}

void __wrap_ParsePrimitivesLinkedList(u_long *p, int singlePrimitive)
{
    if (p && !singlePrimitive) Walk(p);
    __real_ParsePrimitivesLinkedList(p, singlePrimitive);
}

/* ---- opaque texels of a part (VRAM mirror, as fei_hd2d.c reads them) ----- */
static int Texel(const Part *p, int u, int v)
{
    int xb = (p->tpage & 0xF) * 64, yb = ((p->tpage >> 4) & 1) * 256, mode = (p->tpage >> 7) & 3;
    int cx = (p->clut & 0x3F) * 16, cy = (p->clut >> 6) & 0x1FF;
    const unsigned short *row = &vram[((yb + (v & 255)) & 511) * 1024];
    u &= 255;
    if (mode == 0) return vram[cy * 1024 + ((cx + ((row[(xb + (u >> 2)) & 1023] >> ((u & 3) * 4)) & 0xF)) & 1023)];
    if (mode == 1) return vram[cy * 1024 + ((cx + ((row[(xb + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 0xFF)) & 1023)];
    return row[(xb + u) & 1023];
}

/* Display-space pixels whose texel is opaque, one "y:x0-x1/x2-x3" entry per
 * row that has any, rows separated by ';'. Axis-aligned quads only. */
static void WriteMask(FILE *f, const ScenePart *sp)
{
    const Part *p = &sp->p;
    int x0, x1, y0, y1, px, py, rows = 0;
    if (!(p->y[0] == p->y[1] && p->y[2] == p->y[3] && p->x[0] == p->x[2] && p->x[1] == p->x[3]) ||
        p->x[1] == p->x[0] || p->y[2] == p->y[0]) {
        fprintf(f, " mask=none\n");
        return;
    }
    x0 = p->x[0] < p->x[1] ? p->x[0] : p->x[1];
    x1 = p->x[0] < p->x[1] ? p->x[1] : p->x[0];
    y0 = p->y[0] < p->y[2] ? p->y[0] : p->y[2];
    y1 = p->y[0] < p->y[2] ? p->y[2] : p->y[0];
    fprintf(f, " mask=");
    for (py = y0; py < y1; py++) {
        int run = -1, spans = 0;
        const float t = (py + 0.5f - p->y[0]) / (float)(p->y[2] - p->y[0]);
        const int v = (int)(p->v[0] + t * (p->v[2] - p->v[0]));
        for (px = x0; px <= x1; px++) {
            int opaque = 0;
            if (px < x1) {
                const float s = (px + 0.5f - p->x[0]) / (float)(p->x[1] - p->x[0]);
                const int u = (int)(p->u[0] + s * (p->u[1] - p->u[0]));
                opaque = Texel(p, u, v) != 0;
            }
            if (opaque && run < 0) run = px;
            if (!opaque && run >= 0) {
                if (!spans) fprintf(f, "%s%d:", rows++ ? ";" : "", py + sp->ofsY);
                fprintf(f, "%s%d-%d", spans++ ? "/" : "", run + sp->ofsX, px - 1 + sp->ofsX);
                run = -1;
            }
        }
    }
    fprintf(f, "\n");
}

/* ---- readback ------------------------------------------------------------ */
static int ReadTexture(unsigned tex, GLenum type, void *out)
{
    GLint prevTex = 0, packAlign = 4, packRow = 0;
    if (!tex) return 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &packRow);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, type, out);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex);
    glPixelStorei(GL_PACK_ALIGNMENT, packAlign);
    glPixelStorei(GL_PACK_ROW_LENGTH, packRow);
    return glGetError() == GL_NO_ERROR;
}

static int ReadColor(int w, int h, unsigned char *out)
{
    GLint readFb = -1, packAlign = 4, packRow = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFb);
    if (readFb != 0) return 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &packRow);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out);
    glPixelStorei(GL_PACK_ALIGNMENT, packAlign);
    glPixelStorei(GL_PACK_ROW_LENGTH, packRow);
    return glGetError() == GL_NO_ERROR;
}

static void WriteFile(const char *name, const void *data, size_t n)
{
    char path[1024];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", s_dir, name);
    f = fopen(path, "wb");
    if (!f) return;
    fwrite(data, 1, n, f);
    fclose(f);
}

static void Dump(const PcPortHd2dBuffers *b)
{
    const size_t n = (size_t)b->width * (size_t)b->height;
    unsigned short *depth = malloc(n * 2);
    unsigned char *cov = malloc(n), *color = malloc(n * 4);
    char name[256], path[1024];
    FILE *f;
    int i;
    while (glGetError() != GL_NO_ERROR) {}
    if (!depth || !cov || !color || !ReadTexture(b->depth_tex, GL_UNSIGNED_SHORT, depth) ||
        !ReadTexture(b->coverage_tex, GL_UNSIGNED_BYTE, cov) || !ReadColor(b->width, b->height, color)) {
        fprintf(stderr, "[hd2d-buffers-test] readback failed at present %u\n", s_presents);
        goto out;
    }
#ifdef HD2D_BUFFERS_TEST_MUTANT
    for (size_t k = 0; k < n; k++)
        if (depth[k]) depth[k] = (unsigned short)(b->depth_max + 1 - depth[k]);
#endif
    snprintf(name, sizeof name, "frame-%06u.depth", s_presents); WriteFile(name, depth, n * 2);
    snprintf(name, sizeof name, "frame-%06u.coverage", s_presents); WriteFile(name, cov, n);
    snprintf(name, sizeof name, "frame-%06u.color", s_presents); WriteFile(name, color, n * 4);
    snprintf(path, sizeof path, "%s/frame-%06u.txt", s_dir, s_presents);
    if ((f = fopen(path, "w")) != NULL) {
        fprintf(f, "present=%u width=%d height=%d flags=%u depth_max=%u window=%dx%d walks=%lu clear=%d,%d,%d\n",
                s_presents, b->width, b->height, b->flags, b->depth_max, g_windowWidth, g_windowHeight, s_walks,
                activeDrawEnv.r0, activeDrawEnv.g0, activeDrawEnv.b0);
        for (i = 0; i < s_nscene; i++) {
            const ScenePart *sp = &s_scene[i];
            fprintf(f, "part=%d build=%d depth=%u ot=%lx poly=%lx disp=%dx%d ofs=%d,%d quad=%d,%d,%d,%d,%d,%d,%d,%d "
                    "uv=%d,%d,%d,%d,%d,%d,%d,%d",
                    sp->p.part, sp->p.build, sp->depth, (unsigned long)(uintptr_t)sp->p.ot,
                    (unsigned long)(uintptr_t)sp->p.poly, sp->dispW, sp->dispH, sp->ofsX, sp->ofsY,
                    sp->p.x[0], sp->p.y[0], sp->p.x[1], sp->p.y[1], sp->p.x[2], sp->p.y[2], sp->p.x[3], sp->p.y[3],
                    sp->p.u[0], sp->p.v[0], sp->p.u[1], sp->p.v[1], sp->p.u[2], sp->p.v[2], sp->p.u[3], sp->p.v[3]);
            WriteMask(f, sp);
        }
        fclose(f);
    }
    s_dumps++;
    fprintf(stderr, "[hd2d-buffers-test] dumped present %u (%d parts, depth_max %u)\n", s_presents, s_nscene, b->depth_max);
    if (s_dumps >= s_maxDumps) fprintf(stderr, "[hd2d-buffers-test] dumps complete\n");
out:
    free(depth);
    free(cov);
    free(color);
}

/* ---- HOOK 4 --------------------------------------------------------------- */
void PcPort_Hd2dFrameBuffers(const PcPortHd2dBuffers *b)
{
    Init();
    s_presents++;
    s_hook4++;
    if (b->flags == (PC_PORT_HD2D_BUFFERS_DEPTH | PC_PORT_HD2D_BUFFERS_COVERAGE)) {
        s_hook4Valid++;
        if (b->depth_max != (s_count > 65535 ? 65535 : s_count))
            fprintf(stderr, "[hd2d-buffers-test] present %u: depth_max %u but %u OT entries walked\n",
                    s_presents, b->depth_max, s_count);
        if (s_nscene > 0) {
            s_feiPresents++;
            if (s_feiPresents == s_settle) s_nextDump = s_presents;
            if (s_dir && s_nextDump && s_presents >= s_nextDump && s_dumps < s_maxDumps && b->width > 0 && b->height > 0) {
                Dump(b);
                s_nextDump = s_presents + s_every;
            }
        }
    }
    if (s_hook4 % 500 == 0)
        fprintf(stderr, "[hd2d-buffers-test] hook4=%lu valid=%lu walks=%lu fei_presents=%u\n",
                s_hook4, s_hook4Valid, s_walks, s_feiPresents);
    s_count = 0;
    s_nscene = 0;
}
