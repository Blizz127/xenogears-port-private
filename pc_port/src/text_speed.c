/* text_speed.c -- opt-in Enhancement: dialogue TEXT SPEED and AUTO-ADVANCE.
 *
 * Both settings default OFF; with both off every hook below returns the
 * retail answer (1 tick, no auto-confirm) without touching game state, so the
 * field dialogue runs exactly as retail.  Rows: Options > Enhancements, via
 * PcPort_DevMenuRegisterEnhancementValue; values persist in config.ini
 * [enhancements] through that API.
 *
 * Seams (port-only bodies; the matching build uses INCLUDE_ASM for both):
 *   func_80034888 (src/slus_006.64/system/system.c, retail 0x80034888) --
 *     per-frame tick of a message window.  Retail runs one reveal tick per
 *     frame: count down the per-glyph delay (+0x86, reload +0x88) or run the
 *     glyph interpreter func_80033DF0 (0x80033DF0) once and upload the row.
 *     PcPort_TextRevealTicks() returns how many such ticks this frame.
 *   func_8008004C (src/field/dialogue/text_box_render.c, retail 0x8008004C) --
 *     field text-box pump.  Its ordered pass takes the confirm branch when
 *     D_800C2694 & 0x20 (Circle released) for the order-0 box.
 *     PcPort_TextAutoAdvanceFrame() returns a box mask that takes the same
 *     branch.
 *
 * TEXT SPEED (0 retail, 1 2x, 2 4x, 3 instant) only for field dialogue
 * windows (g_FieldTextBoxes[i] + 0x18):
 *   - 2x/4x run 2/4 retail ticks per frame, instant runs ticks until the
 *     page stops.  Ticks stop at the first explicit delay (0F 00 / 0F 02 /
 *     0F 0D set +0x84), at any wait (0x8: page end 02, keep-page 03, message
 *     end 00), at the page-clear flag (0x40) and when the string ends (0x4).
 *     Explicit delays therefore last exactly their retail frame count at
 *     every setting, including instant, and waits are never skipped: the
 *     completion signal a script polls (func_80033CD0 = flags 0x8 and +0x6B,
 *     box visibility) is produced by the unchanged retail code, only sooner.
 *   - Speed-override (0F 01) and per-glyph-delay (0F 0E) codes are reveal
 *     pacing, so they scale with the rest: they change what one retail tick
 *     reveals, and the setting changes how many ticks run.
 *   - The message routine has no per-character sound (func_80033DF0,
 *     func_80034888, func_8008004C and func_8007DCF8 call no sound function),
 *     so there is no blip to stack; this file plays none either.
 *
 * AUTO-ADVANCE (off/on, delay 1.0/1.5/2.5 s): a box is auto-confirmed after
 * it has been, continuously for the delay, in the retail "continue arrow"
 * state -- the exact test func_8008004C uses to show the arrow:
 *     func_80033CD0(window) != 0 (wait flag 0x8 armed, +0x6B = 1/2/3)
 *     and the choice cursor (+0x37C) hidden,
 * restricted further to the only box retail lets confirm (order 0, open
 * timer 0, visible, not a +0x412 box), and, for the message-end wait
 * (+0x6B == 1), only while the box owner's script is parked on opcode 0x9C
 * (func_8009BB0C, "wait for this actor's window to close"), and only while
 * the field's own input masks let confirm (0x20) through -- a page whose
 * confirm is masked does not wait for a player.
 *   Why choices cannot be auto-confirmed: a choice is opcode 0xA9
 *   (func_8009BC98).  It waits for the message-end wait, then shows the
 *   cursor.  While the owner sits on 0xA9 the box is not eligible (opcode is
 *   not 0x9C), and once the cursor is shown it is not eligible (+0x37C == 0);
 *   so the message-end page of a choice is never confirmed by this file.
 *   Mid-message waits (02/03) never carry a cursor -- 0xA9 only acts on the
 *   message-end wait.  Menus that are not field text boxes (shops, name
 *   entry, save screens, battle menus) are never seen by this code.
 *   The delay is counted in serviced vblanks (60 per second at speed 1, so it
 *   follows fast-forward), at most 6 per field frame so a stall, pause or
 *   menu does not count, and restarts whenever the page changes (window
 *   script pointer +0x1C or wait kind +0x6B) or the state is left.  A manual
 *   press keeps working: the retail condition is ORed, never replaced.
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"

#include "port_dev_menu.h"

extern int func_80033CD0(void* window);
extern int PcPort_GetServicedVblankCount(void) __attribute__((weak));
extern u16 D_800ADB00; /* field button mask applied after the pad read */

#define TEXT_WINDOW(i) ((u8*)&g_FieldTextBoxes[i] + 0x18)
#define OP_WAIT_WINDOW_CLOSE 0x9C /* func_8009BB0C */
#define INSTANT_TICKS (1 << 20)    /* bound only; a page stops long before */
#define MAX_VBLANKS_PER_FRAME 6

enum { SPEED_RETAIL, SPEED_2X, SPEED_4X, SPEED_INSTANT, SPEED_COUNT };
enum { DELAY_1_0, DELAY_1_5, DELAY_2_5, DELAY_COUNT };
static const char *const k_speedNames[SPEED_COUNT] = { "RETAIL", "2X", "4X", "INSTANT" };
static const int k_delayVblanks[DELAY_COUNT] = { 60, 90, 150 };
static const char *const k_delayNames[DELAY_COUNT] = { "1.0 S", "1.5 S", "2.5 S" };

static int s_speed = SPEED_RETAIL;
static int s_auto;
static int s_delay = DELAY_1_5;

typedef struct {
    int active;
    u32 page;      /* window +0x1C when the wait was first seen */
    u8 kind;       /* +0x6B */
    int elapsed;   /* vblanks in the state */
    int last;      /* vblank count at the previous frame */
} AutoTrack;
static AutoTrack s_track[4];

static int log_on(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("XENO_TEXT_DIAG");
        on = e != NULL && *e != '\0' && *e != '0';
    }
    return on;
}

int PcPort_TextSpeedGet(void) { return s_speed; }
void PcPort_TextSpeedSet(int v)
{
    s_speed = v < 0 ? 0 : v >= SPEED_COUNT ? SPEED_COUNT - 1 : v;
    fprintf(stderr, "[text-speed] text speed %s\n", k_speedNames[s_speed]);
}
int PcPort_TextAutoAdvanceGet(void) { return s_auto; }
void PcPort_TextAutoAdvanceSet(int v)
{
    s_auto = v != 0;
    fprintf(stderr, "[text-speed] auto-advance %s\n", s_auto ? "ON" : "OFF");
}
int PcPort_TextAutoDelayGet(void) { return s_delay; }
void PcPort_TextAutoDelaySet(int v)
{
    s_delay = v < 0 ? 0 : v >= DELAY_COUNT ? DELAY_COUNT - 1 : v;
    fprintf(stderr, "[text-speed] auto-advance delay %s\n", k_delayNames[s_delay]);
}

/* Row labels: the value is an index into the list in parentheses.  The
 * config keys follow from them (port_dev_menu.h): enhancements.
 * text_speed__retail_2x_4x_instant_, auto_advance__off_on_,
 * auto_advance_delay__1_0_1_5_2_5_s_. */
#define LABEL_SPEED "TEXT SPEED (RETAIL/2X/4X/INSTANT)"
#define LABEL_AUTO "AUTO-ADVANCE (OFF/ON)"
#define LABEL_DELAY "AUTO-ADVANCE DELAY (1.0/1.5/2.5 S)"

__attribute__((constructor)) static void text_speed_register(void)
{
    PcPort_DevMenuRegisterEnhancementValue(LABEL_SPEED, PcPort_TextSpeedGet, PcPort_TextSpeedSet,
                                           0, SPEED_COUNT - 1, 1, NULL);
    PcPort_DevMenuRegisterEnhancementValue(LABEL_AUTO, PcPort_TextAutoAdvanceGet,
                                           PcPort_TextAutoAdvanceSet, 0, 1, 1, NULL);
    PcPort_DevMenuRegisterEnhancementValue(LABEL_DELAY, PcPort_TextAutoDelayGet,
                                           PcPort_TextAutoDelaySet, 0, DELAY_COUNT - 1, 1, NULL);
}

static int field_dialogue_window(const void *window)
{
    int i;
    for (i = 0; i < 4; i++)
        if (window == TEXT_WINDOW(i)) return 1;
    return 0;
}

s32 PcPort_TextRevealTicks(void *window)
{
    if (s_speed == SPEED_RETAIL || !field_dialogue_window(window)) return 1;
    if (s_speed == SPEED_2X) return 2;
    if (s_speed == SPEED_4X) return 4;
    return INSTANT_TICKS;
}

/* The box's own "page complete, waiting for a plain confirm" state; see the
 * header.  Reads only. */
int PcPort_TextPlainConfirmWait(int i)
{
    FieldTextBox *box = &g_FieldTextBoxes[i];
    u8 *window = TEXT_WINDOW(i);
    int kind;

    if (box->visibility != 0 || box->order != 0 || box->unk412 != 0 || box->windowOpenTimer != 0)
        return 0;
    if (box->cursor.visibility == 0) return 0; /* choice cursor shown */
    /* A page only waits for a player's confirm if confirm can reach it: the
     * pad read masks with controllerBtnMask, then with D_800ADB00
     * (FieldPollControllers). */
    if (!(g_FieldControl.controllerBtnMask & 0x20) || !(D_800ADB00 & 0x20)) return 0;
    kind = func_80033CD0(window);
    if (kind == 2 || kind == 3) return kind;   /* mid-message page waits */
    if (kind != 1) return 0;
    {   /* message end: only while the owner waits for the window to close */
        ActorData *owner;
        u8 *script = (u8 *)g_FieldScriptVMCurScriptData;
        if (box->ownerActorID < 0 || box->ownerActorID >= g_FieldNumActors || g_FieldActors == NULL ||
            script == NULL)
            return 0;
        owner = (ActorData *)(uintptr_t)g_FieldActors[box->ownerActorID].pActorData;
        if (owner == NULL || script[owner->scriptInstructionPointer] != OP_WAIT_WINDOW_CLOSE) return 0;
    }
    return 1;
}

s32 PcPort_TextAutoAdvanceFrame(void)
{
    int i, now, mask = 0;

    if (!s_auto) {
        for (i = 0; i < 4; i++) s_track[i].active = 0;
        return 0;
    }
    now = PcPort_GetServicedVblankCount ? PcPort_GetServicedVblankCount() : 0;
    for (i = 0; i < 4; i++) {
        AutoTrack *t = &s_track[i];
        u8 *window = TEXT_WINDOW(i);
        int kind = PcPort_TextPlainConfirmWait(i);
        u32 page = *(u32 *)(window + 0x1C);
        int d;

        if (!kind) {
            t->active = 0;
            continue;
        }
        if (!t->active || t->page != page || t->kind != window[0x6B]) {
            t->active = 1;
            t->page = page;
            t->kind = window[0x6B];
            t->elapsed = 0;
            t->last = now;
            continue;
        }
        /* without a vblank source count two per field frame (30 fps) */
        d = PcPort_GetServicedVblankCount ? now - t->last : 2;
        t->last = now;
        if (d < 0) d = 0;
        if (d > MAX_VBLANKS_PER_FRAME) d = MAX_VBLANKS_PER_FRAME;
        t->elapsed += d;
        if (t->elapsed >= k_delayVblanks[s_delay]) {
            mask |= 1 << i;
            t->active = 0;
            if (log_on())
                fprintf(stderr, "[text-speed] auto-advance box=%d wait=%d after %d vblanks\n",
                        i, kind, t->elapsed);
        }
    }
    return mask;
}
