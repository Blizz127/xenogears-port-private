    if (g_GameSceneMapNum == 616 && tank_local(2) == 0) {
        require(D_800AFD1C == 16 && g_FieldScriptVMCurActor == &tank_actors[16] &&
                D_800B06B8 == &tank_fields[16] && x == -242 && z == -174,
                "laboratory Taura restored in the wrong actor or floor position");
        tank_actors[16].position.vy = tank_sprites[16].position.y = 31 * 65536;
        return;
    }
static void test_taura_laboratory(void);

    test_taura_laboratory();
