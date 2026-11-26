/* The post-dinner director parks after asking Fei to run his departure
 * script. A cold entrance parks on a different branch. EST1 preserves the
 * director, but entering the map constructs Fei with the entrance speed.
 * Recover just the speed operand from the mounted field, without repeating
 * the scripted turn and walk. This also works for existing EST1 saves. */
static int estate_player_speed(const EstateActors *saved, const EstateRoutines *ranges, u16 *speed)
{
    enum { HAND_BACK = 0x20c, CALL_FEI = 0x1f3, DEPARTURE = 12 };
    *speed = 0;
    if (saved->vm[0].ip != HAND_BACK)
        return 1;

    const u8 *code = g_FieldScriptVMCurScriptData;
    if (CALL_FEI < ranges[0].update || HAND_BACK >= ranges[0].talk ||
        code[CALL_FEI] != 0x08 || code[CALL_FEI + 1] != ESTATE_FEI ||
        (code[CALL_FEI + 2] & 0x1f) != DEPARTURE || !g_FieldActors[ESTATE_FEI].pSpriteData)
        return 0;

    const u8 *row = (const u8 *)g_FieldCurScriptFile + 0x84 + ESTATE_FEI * SCRIPT_OFFSET_TABLE_SIZE;
    u16 begin = estate_half(row + 2 * DEPARTURE);
    u16 end = estate_half(row + 2 * (DEPARTURE + 1));
    u16 actor_end = estate_half(row + SCRIPT_OFFSET_TABLE_SIZE);
    if (begin < ranges[0].end || begin + 3 > end || end > actor_end ||
        actor_end > ranges[1].begin || code[begin] != 0x21)
        return 0;
    u16 operand = estate_half(code + begin + 1);
    if (operand <= 0x8000 || operand > 0x8fff)
        return 0;
    *speed = operand & 0xfff;
    return 1;
}

    u16 player_speed;
    if (!estate_player_speed(&saved, ranges, &player_speed))
        return 0;
        if (player_speed) {
            actor_data(ESTATE_FEI)->moveSpeed = player_speed;
            func_80021BCC((void *)(uintptr_t)g_FieldActors[ESTATE_FEI].pSpriteData, player_speed);
        }
