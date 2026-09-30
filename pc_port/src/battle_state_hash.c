/* TEST TOOLING: per-frame battle state hash (XENO_BATTLE_STATE_HASH_LOG=<path>).
 * Off (no file, no work) unless the variable is set.  Used by
 * pc_port/tests/run_one_hit_kills_test.sh to show that a battle with the
 * 1-HIT KILLS cheat off is identical, frame by frame, to a build without the
 * cheat, and to read the rewards the game itself granted.
 *
 * Called from the battle runtime bridge (battle_mips_runtime.c) before every
 * guest instruction fetch; it only reads guest memory.  Lines:
 *   B <battle> start
 *   F <battle> <frame> <crc32> hp=<slot hp list>        at each battle DrawOTag
 *   D <battle> <frame> <start-of-0x2300-byte g_GameState, hex>  when it changed
 *   A <battle> <frame> row=<r> amounts=<11 x u16>             damage routine entry
 * The frame CRC covers the eleven 0x170-byte battler records (D_800CCCE8),
 * the 32 action rows (D_800C3FE8), the enemy AI headers/variables
 * (D_800D3400), the enemy group counters (D_800D301C) and g_GameState. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "battle_mips_adapter.h"

#define BSH_BATTLE_ENTRY 0x80070F40u
#define BSH_DRAW_OTAG    0x80044BD0u
#define BSH_DAMAGE_FN    0x80085618u
#define BSH_GAMESTATE_SIZE 0x2300u

extern unsigned char g_GameState[];

void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu* cpu, uint32_t target);

static uint32_t crc_table[256];

static uint32_t crc_bytes(uint32_t crc, const unsigned char* p, size_t n)
{
    size_t i;
    if (!crc_table[1]) {
        uint32_t k, c;
        int j;
        for (k = 0; k < 256; k++) {
            for (c = k, j = 0; j < 8; j++) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            crc_table[k] = c;
        }
    }
    crc = ~crc;
    for (i = 0; i < n; i++) crc = crc_table[(crc ^ p[i]) & 255u] ^ (crc >> 8);
    return ~crc;
}

static uint32_t crc_guest(PcPortMipsCpu* cpu, uint32_t crc, uint32_t address, uint32_t size)
{
    unsigned char buf[256];
    uint32_t done = 0;
    while (done < size) {
        uint32_t n = size - done < sizeof buf ? size - done : (uint32_t)sizeof buf, i, v;
        for (i = 0; i < n; i++)
            buf[i] = cpu->bus.read(cpu->bus.opaque, address + done + i, 1, &v) == 0 ? (unsigned char)v : 0xEEu;
        crc = crc_bytes(crc, buf, n);
        done += n;
    }
    return crc;
}

void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu* cpu, uint32_t target)
{
    static int inited;
    static FILE* f;
    static unsigned battle, frame;
    static uint32_t last_gs;
    unsigned slot;

    if (!inited) {
        const char* path = getenv("XENO_BATTLE_STATE_HASH_LOG");
        inited = 1;
        if (path && path[0]) f = fopen(path, "w");
    }
    if (!f) return;
    if (target == BSH_BATTLE_ENTRY) {
        battle++;
        frame = 0;
        last_gs = 0;
        fprintf(f, "B %u start\n", battle);
        fflush(f);
        return;
    }
    if (target == BSH_DAMAGE_FN) {
        uint32_t row = (cpu->gpr[4] & 255u) * 72u, v;
        fprintf(f, "A %u %u row=%u amounts=", battle, frame, cpu->gpr[4] & 255u);
        for (slot = 0; slot < 11; slot++) {
            if (cpu->bus.read(cpu->bus.opaque, 0x800C3FE8u + row + slot * 2u, 2, &v)) v = 0xFFFFFFFFu;
            fprintf(f, "%s%u", slot ? "," : "", v);
        }
        fputc('\n', f);
        fflush(f);
        return;
    }
    if (target != BSH_DRAW_OTAG) return;
    {
        uint32_t crc = 0, gs = crc_bytes(0, g_GameState, BSH_GAMESTATE_SIZE), v;
        crc = crc_guest(cpu, crc, 0x800CCCE8u, 11u * 0x170u);
        crc = crc_guest(cpu, crc, 0x800C3FE8u, 32u * 72u);
        crc = crc_guest(cpu, crc, 0x800D3400u, 8u * 64u);
        crc = crc_guest(cpu, crc, 0x800D301Cu, 0x40u);
        crc = crc_bytes(crc, (const unsigned char*)&gs, sizeof gs);
        fprintf(f, "F %u %u %08x hp=", battle, frame, crc);
        for (slot = 0; slot < 11; slot++) {
            uint32_t gear = 0;
            cpu->bus.read(cpu->bus.opaque, 0x800C3EB8u + slot * 28u, 1, &gear);
            if (cpu->bus.read(cpu->bus.opaque, (gear ? 0x800CCDECu : 0x800CCD34u) + slot * 0x170u,
                              gear ? 4 : 2, &v)) v = 0;
            fprintf(f, "%s%u", slot ? "," : "", v);
        }
        fputc('\n', f);
        if (gs != last_gs) {
            unsigned i;
            last_gs = gs;
            fprintf(f, "D %u %u ", battle, frame);
            for (i = 0; i < BSH_GAMESTATE_SIZE; i++) fprintf(f, "%02x", g_GameState[i]);
            fputc('\n', f);
        }
        fflush(f);
        frame++;
    }
}
