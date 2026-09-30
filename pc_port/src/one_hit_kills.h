#ifndef XENO_PC_PORT_ONE_HIT_KILLS_H
#define XENO_PC_PORT_ONE_HIT_KILLS_H
/* 1-HIT KILLS: opt-in dev-menu cheat, OFF by default (one_hit_kills.c).
 * While on, HP damage the party lands on an enemy is raised to that enemy's
 * current HP inside the retail damage-application routine (battle.bin
 * func_80085618), so the kill, EXP, gold, drops and battle end all run
 * through the game's own code.  Enemies whose battle AI reads or writes HP
 * (HP-scripted phases, scripted invulnerability) keep retail damage. */
#include "battle_mips_adapter.h"
#ifdef __cplusplus
extern "C" {
#endif
int PcPort_OneHitKillsEnabled(void);
void PcPort_OneHitKillsToggle(void);
/* Battle runtime bridge hook (battle_mips_runtime.c), before every guest
 * instruction fetch; acts only at the damage routine's entry. */
void PcPort_OneHitKillsBeforeGuest(PcPortMipsCpu* cpu, uint32_t target);
#ifdef __cplusplus
}
#endif
#endif
