/* Same original field script as the awakening, but entrance 0's later story
 * leaves three residents hidden and Fei and Elly together after the meeting. */
int g_FieldNumActors;
static unsigned laboratory_captures, laboratory_restores, laboratory_regroups;

int PcPort_TauraPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES])
{
    require(g_FieldActors == tank_fields && g_GameSceneMapNum == 616 &&
            tank_local(2) == 0 && (tank_local(0x1c0) & 0x1c00) == 0x1400,
            "laboratory capture dispatched outside the completed meeting");
    ++laboratory_captures;
    memcpy(out, "F616", 4);
    memcpy(out + 4, &tank_actors[2].rotation.vy, sizeof(s16));
    return 1;
}

int PcPort_TauraPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES])
{
    require(g_FieldActors == tank_fields && g_actor->position.vz == -177 * 65536 &&
            !memcmp(in, "F616", 4), "laboratory F12 dispatched before Fei or with another party tag");
    memcpy(&tank_actors[2].rotation.vy, in + 4, sizeof(s16));
    ++laboratory_restores;
    return 1;
}

int PcPort_TauraPartyRegroup(void)
{
    require(g_FieldActors == tank_fields && g_actor->position.vz == -177 * 65536,
            "laboratory legacy regroup preceded Fei's saved placement");
    tank_actors[2].position = g_actor->position;
    ++laboratory_regroups;
    return 1;
}

static void laboratory_opcode(void)
{
    require(D_800AFD1C >= 0 && D_800AFD1C < TANK_ACTORS &&
            g_FieldScriptVMCurActor == &tank_actors[D_800AFD1C] &&
            D_800B06B8 == &tank_fields[D_800AFD1C], "laboratory opcode used the wrong actor");
    const u8 *code = g_FieldScriptVMCurScriptData;
    unsigned ip = g_FieldScriptVMCurActor->scriptInstructionPointer;
    switch (code[ip]) {
    case 0xfe: {
        int id = D_800AFD1C;
        require(code[ip + 1] == 0x1c && ((id == 12 && ip == 0x1d7) || (id == 13 && ip == 0x207)),
                "laboratory actor entered an unsupported extended instruction");
        require(tank_local(0x408 + 4 * (id - 12)) == -80, "laboratory bob height was not restored first");
        tank_actors[id].position.vy = tank_sprites[id].position.y = -80 * 65536;
        tank_fields[id].transformMatrix.t[1] = -80;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
        break;
    }
    case 0x67:
        require(D_800AFD1C == 19 && ip == 0x610 && code[ip + 1] == 16,
                "laboratory heading update selected a different actor");
        tank_actors[16].rotation.vy = (s16)0x8a00;
        break;
    case 0x28: /* Enable actor from the field's own script operand. */
        require(D_800AFD1C == 33 && code[ip + 1] == 19, "enabled the wrong laboratory controller");
        tank_actors[19].scriptFlags.flags &= ~1u;
        break;
    case 0x29: {
        unsigned id = code[ip + 1];
        require(D_800AFD1C == 19 && (id == 12 || id == 13 || id == 17),
                "deactivated an unrelated laboratory resident");
        tank_fields[id].status |= ACTOR_STATUS_INVISIBLE;
        tank_actors[id].scriptFlags.flags |= 1;
        break;
    }
    case 0x19:
        require(D_800AFD1C == 14 && ip == 0x22e, "unexpected laboratory actor placement");
        tank_actors[14].position = (VECTOR){-93 * 65536, 30 * 65536, -175 * 65536, 0};
        break;
    case 0x69:
        require(D_800AFD1C == 14 && ip == 0x234, "unexpected laboratory actor heading");
        tank_actors[14].rotation.vy = (s16)0x8800;
        break;
    default:
        fail("unexpected laboratory opcode, dialogue or choreography");
    }
}

static void laboratory_constructor(const PcPortQuickCheckpoint *checkpoint)
{
    tank_constructor(checkpoint);
    g_FieldNumActors = TANK_ACTORS;
    tank_actors[17].scriptInstructionPointer = 0x3bf;
    tank_actors[19].scriptFlags.flags = 0x201b1;
    tank_actors[19].scriptInstructionPointer = 0x4ec; /* Immediately after the constructor VM. */
    tank_actors[1].scriptFlags.flags = 0x24430;
    tank_actors[2].scriptFlags.flags = 0x10204b0;
    tank_actors[2].characterId = 1;
    target_z = checkpoint->position[2];
    target_calls = matrix_calls = 0;
}

static void expect_laboratory_constructor(const char *why)
{
    require(tank_actors[19].scriptInstructionPointer == 0x4ec &&
            tank_actors[17].scriptInstructionPointer == 0x3bf &&
            (tank_actors[19].scriptFlags.flags & 1) &&
            !(tank_fields[12].status & ACTOR_STATUS_INVISIBLE) &&
            !(tank_fields[13].status & ACTOR_STATUS_INVISIBLE) &&
            !(tank_fields[17].status & ACTOR_STATUS_INVISIBLE), why);
}

static void test_taura_laboratory(void)
{
    PcPortQuickCheckpoint legacy = {.on_disc2 = 1, .map = 616, .entrance = 0,
        .position = {-393 * 65536, 30 * 65536, -177 * 65536}};
    camera_game_word(&legacy, 0x1930, 249);
    camera_game_word(&legacy, 0x1934, 615);
    camera_game_word(&legacy, 0x1af0, 0x1400);
    memcpy(legacy.game_state + 0x1d34, (u8[]){0, 1, 255}, 3);
    reset_case();
    mounted_disc = 2;
    for (unsigned i = 0; i < 256; ++i)
        g_FieldScriptVMHandlers[i] = laboratory_opcode;
    laboratory_constructor(&legacy);
    PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES);
    expect_laboratory_constructor("ordinary field visit applied a saved meeting");

    for (unsigned i = 0; i < 7; ++i) {
        PcPortQuickCheckpoint other = legacy;
        switch (i) {
        case 0: other.entrance = 1; camera_game_word(&other, 0x1932, 1); break;
        case 1: camera_game_word(&other, 0x1930, 250); break;
        case 2: camera_game_word(&other, 0x1af0, 0x400); break;
        case 3: other.game_state[0x1d35] = 255; break;
        case 4:
            other.field_actor_kind = PC_PORT_QUICK_ACTORS_FOLLOW_CAMERA;
            other.has_party_followers = 1;
            memcpy(other.party_followers, "W615", 4);
            break;
        case 5: camera_game_word(&other, 0x1af0, 0x1c00); break; /* Later emissary event. */
        case 6: break; /* Mismatched script size, otherwise the correct slot. */
        }
        load_fixture(&other);
        laboratory_constructor(&other);
        PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES - (i == 6));
        expect_laboratory_constructor("wrong entrance, story, party, tag or script restored the meeting");
    }

    load_fixture(&legacy);
    laboratory_constructor(&legacy);
    tank_actors[19].scriptInstructionPointer = 0x4ed; /* Controller's later disabled poll. */
    PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES);
    require((tank_actors[19].scriptFlags.flags & 1) &&
            !(tank_fields[17].status & ACTOR_STATUS_INVISIBLE),
            "later controller phase incorrectly staged the laboratory handback");
    laboratory_constructor(&legacy);
    PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES);
    require(tank_actors[19].scripts[0].currentIP == 0x634 &&
            tank_actors[19].scriptInstructionPointer == 0x634 &&
            !(tank_actors[19].scriptFlags.flags & 1) &&
            (tank_fields[12].status & ACTOR_STATUS_INVISIBLE) &&
            (tank_fields[13].status & ACTOR_STATUS_INVISIBLE) &&
            (tank_fields[17].status & ACTOR_STATUS_INVISIBLE) &&
            tank_actors[17].scriptInstructionPointer == 0x3c0 &&
            tank_actors[17].scripts[0].currentIP == 0x3c0 &&
            tank_actors[14].position.vx == -93 * 65536 && (u16)tank_actors[14].rotation.vy == 0x8800 &&
            tank_actors[12].position.vy == -80 * 65536 && tank_actors[13].position.vy == -80 * 65536 &&
            tank_actors[16].position.vx == -15858992 && tank_actors[16].position.vz == -11398704 &&
            (u16)tank_actors[16].rotation.vy == 0x8a00,
            "postmeeting cast, Citan, fractional Taura or parked controller lost");
    D_800B21D0 = 1;
    PcPort_QuickCheckpointRestorePlayer();
    require(!g_actor->position.vz && !laboratory_regroups, "legacy regroup bypassed scene lock");
    D_800B21D0 = 0;
    g_FieldCameraMode = 2;
    PcPort_QuickCheckpointRestorePlayer();
    require(!g_actor->position.vz && !laboratory_regroups, "legacy regroup bypassed camera handback");
    g_FieldCameraMode = 0;
    PcPort_QuickCheckpointRestorePlayer();
    require(g_actor->position.vz == target_z && laboratory_regroups == 1 && !laboratory_restores &&
            !memcmp(&g_actor->position, &tank_actors[2].position, sizeof(VECTOR)),
            "legacy laboratory did not regroup once after Fei's saved placement");

    /* Actual F11/F12 and field-constructor hooks, including the typed codec's
     * dependence on a complete camera and the already-earned room handback. */
    taura_test_field();
    laboratory_constructor(&legacy);
    g_actor->position = (VECTOR){legacy.position[0], legacy.position[1], legacy.position[2], 0};
    tank_actors[19].scriptFlags.flags &= ~1u;
    tank_actors[19].scripts[0].currentIP = tank_actors[19].scriptInstructionPointer = 0x634;
    tank_actors[19].scripts[0].flags_0x12 = 7;
    tank_actors[2].rotation.vy = 0x440;
    PcPortQuickCheckpoint typed;
    taura_test_save(&typed);
    require(laboratory_captures == 1 && typed.field_actor_kind == PC_PORT_QUICK_ACTORS_FOLLOW_CAMERA &&
            typed.has_party_followers && !memcmp(typed.party_followers, "F616", 4),
            "ordinary F11 lost the postmeeting party companion");
    load_fixture_slot();
    laboratory_constructor(&typed);
    PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES);
    g_FieldCameraMode = 2;
    PcPort_QuickCheckpointRestorePlayer();
    PcPort_QuickCheckpointFieldFrameSubmitted();
    require(PcPort_QuickCheckpointHoldFieldPresentation() && !laboratory_restores,
            "typed laboratory F12 presented the entrance while the camera still owned it");
    g_FieldCameraMode = 0;
    PcPort_QuickCheckpointRestorePlayer();
    PcPort_QuickCheckpointFieldFrameSubmitted();
    require(!PcPort_QuickCheckpointHoldFieldPresentation(), "restored laboratory frame stayed hidden");
    require(laboratory_restores == 1 && laboratory_regroups == 1 && tank_actors[2].rotation.vy == 0x440,
            "typed laboratory F12 substituted a legacy regroup or lost follower facing");
    ++g_actor->position.vz;
    PcPort_QuickCheckpointRestorePlayer();
    require(g_actor->position.vz == target_z + 1 && laboratory_restores == 1,
            "continued Fei motion was overwritten by a repeated restore");
    /* The physical f4dd F11 carried FCV1 but neither PWF1 nor PTY1. */
    typed.has_party_followers = 0;
    load_fixture(&typed);
    laboratory_constructor(&typed);
    PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES);
    PcPort_QuickCheckpointRestorePlayer();
    require(laboratory_regroups == 2 && laboratory_restores == 1 &&
            tank_actors[19].scriptInstructionPointer == 0x634 &&
            tank_actors[16].position.vz == -11398704,
            "camera-only legacy laboratory F12 did not recover the authored handback");
    /* Bits 6 and 7 can come from unrelated Disc 1 progress. */
    script_word(0x1c0, 0x14c0);
    taura_test_save(&typed);
    require(typed.has_party_followers && !memcmp(typed.party_followers, "F616", 4),
            "unrelated scenario flags blocked a new laboratory F11");
    load_fixture_slot();
    laboratory_constructor(&typed);
    PcPort_QuickCheckpointFieldScriptsReady(TANK_SCRIPT_BYTES);
    PcPort_QuickCheckpointRestorePlayer();
    require(laboratory_restores == 2 && tank_local(0x1c0) == 0x14c0 &&
            tank_actors[19].scripts[0].currentIP == 0x634,
            "laboratory F12 lost unrelated scenario flags or the saved party");
    for (unsigned i = 0; i < 256; ++i)
        g_FieldScriptVMHandlers[i] = tank_opcode;
    puts("QUICK GATE Disc 2 laboratory e0 pre/postmeeting, cast, actor owner and typed/legacy F12: PASS");
}
