/* 1-HIT KILLS: opt-in dev-menu cheat ("1-HIT KILLS" on CHEATS, console
 * `onehit [on|off]`, XENO_ONE_HIT_KILLS=1 for headless runs).  OFF by default;
 * while off the hook below returns before reading or writing any guest word,
 * so battles are retail-identical (pc_port/tests/run_one_hit_kills_test.sh).
 *
 * Where: battle.bin func_80085618 is the one routine that applies HP damage,
 * on foot and in Gear battles alike.  For each battler slot s (0..2 party,
 * 3..10 enemies) of action row r it reads the queued amount
 * u16 D_800C3FE8[r*72 + s*2] and, for the HP kinds 0/5/7/8 at
 * D_800C4000[r*72 + s], subtracts it from the slot's HP: s16 D_800CCD34 +
 * s*0x170 on foot, u32 D_800CCDEC + s*0x170 when the Gear byte D_800C3EB8 +
 * s*28 is set.  At zero it sets the down bits and, for an enemy, calls
 * func_800883AC (drops the enemy from its group's alive count and mask).
 * The routine runs as guest MIPS in the battle runtime, so the hook
 * sits at the runtime's host/guest boundary (battle_mips_runtime.c
 * runtime_bridge, like god_mode.c): at the routine's entry, before its first
 * instruction, it raises an enemy's queued amount to that enemy's current HP.
 * Retail then does the subtraction, the death flags, func_800883AC and
 * everything after -- nothing here writes HP, flags or rewards.
 *
 * Only raised, never invented: a row the game computed as 0 (miss, immune,
 * scripted invulnerability), a foot heal/absorb (bit 15 = negative), a slot
 * that is inactive or already down, or a non-HP kind is left exactly as the
 * game computed it.  Party slots are never touched (that is GOD MODE's job).
 *
 * Bosses: an enemy whose battle AI looks at or changes HP -- phase changes at
 * an HP threshold, "can't die" scripts that restore HP, scripted battle ends
 * -- keeps retail damage for the whole fight (see ai_touches_hp).  The AI is
 * a fixed 4-byte instruction stream per section (func_80079934 advances by 4;
 * sections end at 0xFD/0xFF, ops < 0x80 go to func_8007EF6C, >= 0x80 are
 * conditions over the AI variables only), and a condition can see HP only
 * after one of these ops copied it into a variable:
 *   op 41 (func_8007B3E4) set own u16 stat p1       -- flagged when p1 is 0/1
 *   op 44 (func_8007B578) read target u16 stat p2   -- flagged when p2 is 0/1
 *   op 45 (func_8007B608) write target u16 stat p2  -- flagged when p2 is 0/1
 *   op 46 (func_8007B6C0) read Gear HP (p2 == 1)    -- flagged when p2 is 1
 *   op 47 (func_8007B7B0) write Gear HP / max HP    -- always flagged
 *   ops 92..97 (func_8007D8C0..DFD4) pick a target by u16 stat p1 -- p1 0/1
 *   order type 11 (func_8007916C, stat add) built with op 1/2 on stat 0/1
 * (u16 stat 0 = max HP, 1 = HP: func_8007A280's field table.)  The
 * target-selection ops 71/72/90/91 (lowest-HP party member / ally) only pick
 * a target and are not flagged.  Gear amounts are 16-bit in retail, so a Gear
 * above 65535 HP takes 65535 per hit (the cap is logged). */
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "one_hit_kills.h"

#define OHK_DAMAGE_FN     0x80085618u  /* battle.bin damage application */
#define OHK_ACTIVE_TABLE  0x800D2DCCu  /* u8 per slot: battler present */
#define OHK_AMOUNT_TABLE  0x800C3FE8u  /* u16 [11] per 72-byte action row */
#define OHK_KIND_TABLE    0x800C4000u  /* u8 [11] per action row */
#define OHK_GEAR_TABLE    0x800C3EB8u  /* u8 per slot, stride 28: Gear battler */
#define OHK_SLOT_STRIDE   0x170u
#define OHK_FOOT_HP       0x800CCD34u  /* s16 */
#define OHK_GEAR_HP       0x800CCDECu  /* u32 */
#define OHK_STATUS        0x800CCD64u  /* u16, bit 15 = down */
#define OHK_AI_HEADER     0x800D3400u  /* enemy (slot-3)*64: four section pointers */
#define OHK_FIRST_ENEMY   3u
#define OHK_SLOTS         11u
#define OHK_FOOT_MAX      0x7FFFu      /* foot amounts are signed 16-bit */
#define OHK_GEAR_MAX      0xFFFFu      /* Gear amounts are unsigned 16-bit */
#define OHK_SCAN_LIMIT    2048u        /* instructions per AI section */

static atomic_int enabled;

static void one_hit_init(void)
{
    static int inited;
    const char* env;

    if (inited) return;
    inited = 1;
    env = getenv("XENO_ONE_HIT_KILLS");
    if (env != NULL && env[0] != '\0' && env[0] != '0') {
        atomic_store(&enabled, 1);
        fprintf(stderr, "[xeno-port][one-hit] ON from XENO_ONE_HIT_KILLS\n");
    }
}

int PcPort_OneHitKillsEnabled(void) { one_hit_init(); return atomic_load(&enabled); }

void PcPort_OneHitKillsToggle(void)
{
    int value = !PcPort_OneHitKillsEnabled();
    atomic_store(&enabled, value);
    fprintf(stderr, "[xeno-port][one-hit] %s (party HP damage to enemies raised "
            "to the enemy's HP; HP-scripted enemies keep retail damage)\n",
            value ? "ON" : "OFF");
}

static int rd(PcPortMipsCpu* cpu, uint32_t address, unsigned width, uint32_t* value)
{
    return cpu->bus.read(cpu->bus.opaque, address, width, value);
}

/* 1: the section touches HP, 0: it does not, -1: unreadable. */
static int section_touches_hp(PcPortMipsCpu* cpu, uint32_t pc, const char** why)
{
    int order_type11 = 0, order_hp_field = 0;
    unsigned n;

    for (n = 0; n < OHK_SCAN_LIMIT; n++, pc += 4u) {
        uint32_t b[4];
        unsigned i;
        for (i = 0; i < 4; i++)
            if (rd(cpu, pc + i, 1, &b[i]) != 0) return -1;
        switch (b[0]) {
        case 0xFD: case 0xFF:
            if (order_type11 && order_hp_field) { *why = "order11-hp-stat"; return 1; }
            return 0;
        case 1:  /* order byte p1 = p2 */
            if (b[1] == 0 && b[2] == 11) order_type11 = 1;
            if (b[1] == 1 && b[2] <= 1) order_hp_field = 1;
            break;
        case 2:  /* order byte p1 = u8 variable: type or stat unknown */
            if (b[1] == 0) order_type11 = 1;
            if (b[1] == 1) order_hp_field = 1;
            break;
        case 41: if (b[1] <= 1) { *why = "op41-set-own-hp"; return 1; } break;
        case 44: if (b[2] <= 1) { *why = "op44-read-hp"; return 1; } break;
        case 45: if (b[2] <= 1) { *why = "op45-write-hp"; return 1; } break;
        case 46: if (b[2] == 1) { *why = "op46-read-gear-hp"; return 1; } break;
        case 47: *why = "op47-write-gear-hp"; return 1;
        case 92: case 93: case 94: case 95: case 96: case 97:
            if (b[1] <= 1) { *why = "op92-97-hp-select"; return 1; }
            break;
        default: break;
        }
    }
    return -1;  /* no terminator: not an AI stream we understand */
}

typedef struct {
    uint32_t ptr[4];
    int valid;
    int hp_scripted;
    const char* why;
} AiVerdict;
static AiVerdict s_ai[OHK_SLOTS - OHK_FIRST_ENEMY];

/* Unreadable or unterminated AI counts as HP-scripted: retail damage is the
 * safe answer when the script cannot be read. */
static const AiVerdict* ai_touches_hp(PcPortMipsCpu* cpu, uint32_t slot)
{
    AiVerdict* v = &s_ai[slot - OHK_FIRST_ENEMY];
    uint32_t ptr[4];
    unsigned i;

    for (i = 0; i < 4; i++)
        if (rd(cpu, OHK_AI_HEADER + (slot - OHK_FIRST_ENEMY) * 64u + i * 4u, 4, &ptr[i]) != 0)
            ptr[i] = 0xFFFFFFFFu;
    if (v->valid && memcmp(v->ptr, ptr, sizeof ptr) == 0) return v;

    memcpy(v->ptr, ptr, sizeof ptr);
    v->valid = 1;
    v->hp_scripted = 0;
    v->why = "none";
    for (i = 0; i < 4 && !v->hp_scripted; i++) {
        const char* why = "unreadable";
        int r;
        if (ptr[i] == 0) continue;
        r = section_touches_hp(cpu, ptr[i], &why);
        if (r != 0) { v->hp_scripted = 1; v->why = why; }
    }
    fprintf(stderr, "[xeno-port][one-hit] slot=%u ai=%08x,%08x,%08x,%08x %s (%s)\n",
            slot, ptr[0], ptr[1], ptr[2], ptr[3],
            v->hp_scripted ? "HP-scripted: retail damage" : "eligible", v->why);
    return v;
}

void PcPort_OneHitKillsBeforeGuest(PcPortMipsCpu* cpu, uint32_t target)
{
    uint32_t row, slot;

    if (target != OHK_DAMAGE_FN || !PcPort_OneHitKillsEnabled()) return;

    row = (cpu->gpr[4] & 255u) * 72u;
    for (slot = OHK_FIRST_ENEMY; slot < OHK_SLOTS; ++slot) {
        uint32_t active, kind, status, gear, amount, hp, cap, address;
        const AiVerdict* ai;

        address = OHK_AMOUNT_TABLE + row + slot * 2u;
        if (rd(cpu, OHK_ACTIVE_TABLE + slot, 1, &active) || !active ||
            rd(cpu, OHK_KIND_TABLE + row + slot, 1, &kind) ||
            (kind != 0 && kind != 5 && kind != 7 && kind != 8) ||
            rd(cpu, OHK_STATUS + slot * OHK_SLOT_STRIDE, 2, &status) || (status & 0x8000u) ||
            rd(cpu, OHK_GEAR_TABLE + slot * 28u, 1, &gear) ||
            rd(cpu, address, 2, &amount)) {
            continue;
        }
        if (amount == 0 || (!gear && (amount & 0x8000u))) continue;
        if (gear) {
            if (rd(cpu, OHK_GEAR_HP + slot * OHK_SLOT_STRIDE, 4, &hp)) continue;
            cap = OHK_GEAR_MAX;
        } else {
            if (rd(cpu, OHK_FOOT_HP + slot * OHK_SLOT_STRIDE, 2, &hp) || (hp & 0x8000u)) continue;
            cap = OHK_FOOT_MAX;
        }
        if (hp == 0 || amount >= hp) continue;  /* retail's own hit already kills */

        ai = ai_touches_hp(cpu, slot);
        if (ai->hp_scripted) {
            fprintf(stderr, "[xeno-port][one-hit] kept retail damage %u slot=%u gear=%u "
                    "hp=%u (HP-scripted AI: %s)\n", amount, slot, gear != 0, hp, ai->why);
            continue;
        }
        if (cpu->bus.write(cpu->bus.opaque, address, 2, hp > cap ? cap : hp) == 0) {
            fprintf(stderr, "[xeno-port][one-hit] damage %u -> %u slot=%u gear=%u hp=%u%s\n",
                    amount, hp > cap ? cap : hp, slot, gear != 0, hp,
                    hp > cap ? " (16-bit Gear amount cap)" : "");
        }
    }
}
