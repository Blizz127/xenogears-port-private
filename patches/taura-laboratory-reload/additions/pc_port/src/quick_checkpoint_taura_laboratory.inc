/* The second Taura meeting has a persistent completion bit, but its cast and
 * the final phase of controller 19 live only in the loaded field. The ordinary
 * constructor correctly skips the meeting once bit 12 is set. */
enum { TAURA_MEETING = 19, TAURA_MEETING_HOLD = 0x634 };

static int taura_laboratory_state(const uint8_t *state)
{
    return tank_word(state + 0x1930) == 249 && tank_word(state + 0x1932) == 0 &&
           tank_word(state + 0x1934) == 615 &&
           (tank_word(state + 0x1930 + 0x1c0) & 0x1c00) == 0x1400 &&
           !memcmp(state + 0x1d34, (u8[]){0, 1, 0xff}, 3);
}

static int taura_meeting_parked(void)
{
    if (!s_tauraLaboratoryScript || s_tauraLaboratoryScript != g_FieldCurScriptFile ||
        D_800ADBFC != TANK_ACTORS || !g_FieldActors || !g_FieldActors[TAURA_MEETING].pActorData)
        return 0;
    const ActorData *actor = tank_actor(TAURA_MEETING);
    return !(actor->scriptFlags.flags & 1u) &&
           actor->scripts[0].currentIP == TAURA_MEETING_HOLD &&
           actor->scripts[0].flags_0x12 == 7 && actor->scriptInstructionPointer == TAURA_MEETING_HOLD;
}

static void restore_taura_laboratory(void)
{
    /* s_tankScript already authenticates the US Disc 2 script, its declared
     * 5,836 bytes, actor table, code address and hash. Entrance 1's awakening
     * uses a different cast and a different controller. */
    if (!s_tankScript || g_FieldScriptMemoryU16[1] != 0)
        return;
    s_tauraLaboratoryScript = g_FieldCurScriptFile;
    if (!s_restoreReady || !s_loaded.on_disc2 || s_loaded.map != TANK_FIELD ||
        s_loaded.entrance != 0 || !taura_laboratory_state(s_loaded.game_state) ||
        s_loaded.field_actor_file_rejected ||
        (s_loaded.field_actor_kind != PC_PORT_QUICK_ACTORS_NONE &&
         s_loaded.field_actor_kind != PC_PORT_QUICK_ACTORS_FOLLOW_CAMERA) ||
        (s_loaded.has_party_followers && memcmp(s_loaded.party_followers, "F616", 4)) ||
        !g_FieldActors || g_PlayerActorIndex != TANK_PLAYER || g_FieldNumActors < TANK_ACTORS ||
        g_FieldScriptMemoryU16[0] != 249 || g_FieldScriptMemoryU16[2] != 615 ||
        (g_FieldScriptMemoryU16[0x1c0 / 2] & 0x1c00) != 0x1400)
        return;

    const int cast[] = {1, 2, 12, 13, 14, 16, 17, TAURA_MEETING, 33};
    for (unsigned i = 0; i < sizeof cast / sizeof cast[0]; ++i) {
        if (!g_FieldActors[cast[i]].pActorData || !g_FieldActors[cast[i]].pSpriteData)
            return;
    }
    ActorData *controller = tank_actor(TAURA_MEETING);
    if (!(controller->scriptFlags.flags & 1u) || controller->scriptInstructionPointer != 0x4ec ||
        tank_actor(1)->characterId != 0 || tank_actor(2)->characterId != 1)
        return;

    /* The two tank residents had run their first bobbing update before the
     * meeting hid them. FE1C materializes that height in each actor's context. */
    for (int index = 12; index <= 13; ++index) {
        unsigned height_word = (0x408 + (index - 12) * 4) / 2;
        g_FieldScriptMemoryU16[height_word] = (u16)-80;
        g_FieldScriptMemoryU16[height_word + 1] = 64;
        u16 next = checkpoint_instruction(index, index == 12 ? 0x1d7 : 0x207);
        ActorData *resident = tank_actor(index);
        resident->prevPosition.y = -80;
        resident->scripts[0].currentIP = resident->scriptInstructionPointer = next;
        g_FieldActors[index].childMatrix.t[1] = g_FieldActors[index].transformMatrix.t[1];
    }
    checkpoint_instruction(33, 0xab7); /* Enable actor 19, then park it before VM dispatch. */
    checkpoint_instruction(TAURA_MEETING, 0x4f2); /* Disable resident 17. */
    checkpoint_instruction(TAURA_MEETING, 0x4f4); /* Disable resident 13. */
    checkpoint_instruction(TAURA_MEETING, 0x4f6); /* Disable resident 12. */
    tank_actor(17)->scripts[0].currentIP = tank_actor(17)->scriptInstructionPointer = 0x3c0;
    checkpoint_instruction(14, 0x22e); /* Citan's authored placement and facing. */
    checkpoint_instruction(14, 0x234);

    /* Taura's ten-update walk toward Fei ends between integer coordinates.
     * Recover floor contact via the original placement routine, then restore
     * the sub-unit endpoint from the original postmeeting handback. */
    checkpoint_place_actor(16, -242, -174);
    ActorData *taura = tank_actor(16);
    SpriteData *sprite = (SpriteData *)(uintptr_t)g_FieldActors[16].pSpriteData;
    taura->position.vx = sprite->position.x = -15858992;
    taura->position.vz = sprite->position.z = -11398704;
    sprite->direction = 2072;
    checkpoint_instruction(TAURA_MEETING, 0x610);
    for (int index = 14; index <= 16; index += 2) {
        ActorData *npc = tank_actor(index);
        npc->prevPosition = (SVEC){npc->position.vx >> 16, npc->position.vy >> 16, npc->position.vz >> 16};
        /* Normal postmeeting interaction and contact, before either NPC turns
         * to speak to the player. Their positions remain fixed in this phase. */
        npc->flags = (npc->flags & ~0x200u) | 0x10000u;
        npc->scriptFlags.flags &= ~0x04000000u;
    }

    controller->scripts[0].currentIP = TAURA_MEETING_HOLD;
    controller->scripts[0].flags_0x12 = 7;
    controller->scriptInstructionPointer = TAURA_MEETING_HOLD;
    s_restoreTauraLaboratory = 1;
}
