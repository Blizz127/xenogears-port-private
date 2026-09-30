/* text_diag.c -- TEST TOOLING: field dialogue-state log and message-relative
 * confirm presses, for the TEXT SPEED / AUTO-ADVANCE tests
 * (pc_port/tests/run_textspeed_scene_test.sh).
 *
 * Inert unless XENO_TEXT_DIAG is set (non-empty, not "0").  Called once per
 * field frame at the end of func_8008004C (PcPort_TextDiagFrame, weak seam in
 * src/field/dialogue/text_box_render.c), after every box has ticked.  It is
 * NOT part of the enhancement: the off-identity test links it into both the
 * build without the enhancement and the build with it.
 *
 * Log (stderr, "[text-diag] "):
 *   f=N ...      one line per field frame while any box is visible: per
 *                visible box its order, open timer, window flags, the wait
 *                kind the scripts see (func_80033CD0), explicit delay
 *                (+0x84), glyph-delay counter (+0x86), glyph x/row, script
 *                offset into its string, row widths, choice cursor, arrow,
 *                plus the confirm edge D_800C2694.
 *   page ...     when a box's wait is armed: kind, script offset, row widths
 *                and a CRC of the text rows in VRAM (what the player sees).
 *   close ...    when a box closes: CRC of g_GameState.
 *   press ...    when this file presses confirm.
 * Presses (through the field test-input hold, i.e. a real pad release two
 * frames later, seen by the whole field like a player's press):
 *   XENO_TEXT_DIAG_PRESS=K   confirm after the continue arrow has shown K
 *                            frames (0/unset: never)
 *   XENO_TEXT_DIAG_CHOICE=K  confirm after a choice cursor has shown K frames
 *                            (0/unset: never, the choice waits)
 * Only reads game state; the presses go through PcPort_FieldTestInputHold.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "field/text_box.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern int func_80033CD0(void* window);
extern void PcPort_FieldTestInputHold(unsigned value, unsigned frames);
extern u16 D_800C2694;

static int s_on = -1;
static int s_pressAfter, s_choiceAfter;
static unsigned s_frame;
static int s_prevKind[4], s_prevVisible[4];
static int s_arrowFrames[4], s_cursorFrames[4], s_pressed[4];
static u32 s_prevPage[4];
static u16 s_vram[0x4000];

static u32 crc32(const void* data, size_t n, u32 crc)
{
    const u8* p = data;
    size_t i;
    int k;
    crc = ~crc;
    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static int env_int(const char* name)
{
    const char* e = getenv(name);
    return e ? atoi(e) : 0;
}

static s32 script_offset(int i)
{
    u8* box = (u8*)&g_FieldTextBoxes[i];
    return (s32)(*(u32*)(box + 0x18 + 0x1C) - *(u32*)(box + 0xA8));
}

static int rows_text(u8* window, char* out, size_t n)
{
    u8* rows = (u8*)(uintptr_t)*(u32*)(window + 0x28);
    int r, count = *(s16*)(window + 0x0C), used = 0;
    out[0] = '\0';
    if (rows == NULL || count <= 0 || count > 8) return 0;
    for (r = 0; r < count && used < (int)n - 8; r++)
        used += snprintf(out + used, n - used, "%s%d", r ? "," : "", *(s16*)(rows + r * 0x60 + 0x58));
    return count;
}

static u32 rows_vram_crc(u8* window)
{
    u8* rows = (u8*)(uintptr_t)*(u32*)(window + 0x28);
    int r, count = *(s16*)(window + 0x0C);
    u32 crc = 0;
    if (rows == NULL || count <= 0 || count > 8) return 0;
    for (r = 0; r < count; r++) {
        RECT rect = *(RECT*)(rows + r * 0x60 + 0x50);
        size_t words = (size_t)rect.w * (size_t)rect.h;
        if (rect.w <= 0 || rect.h <= 0 || words > sizeof s_vram / sizeof s_vram[0]) continue;
        StoreImage(&rect, (u_long*)s_vram);
        DrawSync(0);
        crc = crc32(&rect, sizeof rect, crc);
        crc = crc32(s_vram, words * 2, crc);
    }
    return crc;
}

void PcPort_TextDiagFrame(void)
{
    char line[1024];
    int i, used, any = 0;

    if (s_on < 0) {
        const char* e = getenv("XENO_TEXT_DIAG");
        s_on = e != NULL && *e != '\0' && *e != '0';
        s_pressAfter = env_int("XENO_TEXT_DIAG_PRESS");
        s_choiceAfter = env_int("XENO_TEXT_DIAG_CHOICE");
        if (s_on)
            fprintf(stderr, "[text-diag] on press=%d choice=%d\n", s_pressAfter, s_choiceAfter);
    }
    if (!s_on) return;
    s_frame++;
    used = snprintf(line, sizeof line, "[text-diag] f=%u c=%x", s_frame, (unsigned)(D_800C2694 & 0x20));
    for (i = 0; i < 4; i++) {
        FieldTextBox* box = &g_FieldTextBoxes[i];
        u8* window = (u8*)box + 0x18;
        int visible = box->visibility == 0;
        int kind, arrow, cursor;
        char rows[96];

        if (!visible) {
            if (s_prevVisible[i])
                fprintf(stderr, "[text-diag] close f=%u b=%d gs=%08x\n", s_frame, i,
                        (unsigned)crc32(&g_GameState, sizeof g_GameState, 0));
            s_prevVisible[i] = 0;
            s_prevKind[i] = 0;
            s_arrowFrames[i] = s_cursorFrames[i] = s_pressed[i] = 0;
            continue;
        }
        if (!s_prevVisible[i])
            fprintf(stderr, "[text-diag] open f=%u b=%d owner=%d\n", s_frame, i, box->ownerActorID);
        s_prevVisible[i] = 1;
        any = 1;
        kind = func_80033CD0(window);
        cursor = box->cursor.visibility == 0;
        arrow = kind != 0 && !cursor && box->windowOpenTimer == 0;
        rows_text(window, rows, sizeof rows);
        if (used < (int)sizeof line - 200)
            used += snprintf(line + used, sizeof line - used,
                             " | b%d o=%d ot=%d fl=%04x k=%d dl=%d gd=%d x=%d r=%d sb=%d off=%d w=%s cur=%d/%d st=%d",
                             i, (s16)box->order, box->windowOpenTimer, *(u16*)(window + 0x10), kind,
                             *(s16*)(window + 0x84), *(s16*)(window + 0x86), *(s16*)(window + 0x00),
                             *(s16*)(window + 0x02), *(s16*)(window + 0x16), (int)script_offset(i), rows,
                             cursor, box->cursor.curPosition, box->status);
        if (kind != 0 && (s_prevKind[i] == 0 || s_prevPage[i] != *(u32*)(window + 0x1C)))
            fprintf(stderr, "[text-diag] page f=%u b=%d k=%d off=%d w=%s vram=%08x\n", s_frame, i, kind,
                    (int)script_offset(i), rows, (unsigned)rows_vram_crc(window));
        if (kind != 0 && s_prevPage[i] != *(u32*)(window + 0x1C)) s_pressed[i] = 0;
        s_prevKind[i] = kind;
        s_prevPage[i] = *(u32*)(window + 0x1C);

        /* message-relative presses (order-0 box only, like retail's confirm) */
        s_arrowFrames[i] = arrow ? s_arrowFrames[i] + 1 : 0;
        s_cursorFrames[i] = cursor && box->windowOpenTimer == 0 ? s_cursorFrames[i] + 1 : 0;
        if (box->order == 0 && !s_pressed[i] &&
            ((s_pressAfter > 0 && s_arrowFrames[i] >= s_pressAfter) ||
             (s_choiceAfter > 0 && s_cursorFrames[i] >= s_choiceAfter))) {
            PcPort_FieldTestInputHold(0x20, 1);
            s_pressed[i] = 1;
            fprintf(stderr, "[text-diag] press f=%u b=%d k=%d %s\n", s_frame, i, kind,
                    cursor ? "choice" : "page");
        }
        if (!arrow && !cursor) s_pressed[i] = 0;
    }
    if (any) fprintf(stderr, "%s\n", line);
}
