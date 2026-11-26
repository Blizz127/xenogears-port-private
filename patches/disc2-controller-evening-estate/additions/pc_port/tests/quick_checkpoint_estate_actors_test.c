/* Relocate Fei's set-speed routine and use an operand different from Disc 1's. */
enum { FEI_ROUTINE_12 = 0x321, FEI_HAND_BACK_SPEED = 0x187,
       FEI_COLD_SPEED = 256, SPRITE_SPEED_MASK = 0x7ff80 };
static unsigned fei_speed_calls, submitted_citan_speed;
void func_80021BCC(void *sprite, u32 speed)
{
    assert(sprite == scene.sprites[CITAN] || sprite == scene.sprites[FEI]);
    u32 bits = get32(sprite, 0xac);
    put32(sprite, 0xac, (bits & ~SPRITE_SPEED_MASK) | ((speed & 0xfffu) << 7));
    speed_calls++;
    if (sprite == scene.sprites[FEI])
        fei_speed_calls++;
    else
        submitted_citan_speed = speed;
}

    put16(field_script, 0x84 + FEI * 0x40, 0x2a2);
    put16(field_script, 0x84 + FEI * 0x40 + 12 * 2, FEI_ROUTINE_12);
    put16(field_script, 0x84 + FEI * 0x40 + 13 * 2, FEI_ROUTINE_12 + 0x12);
    put16(field_script, 0x84 + (FEI + 1) * 0x40, 0x380);
    bytecode[FEI_ROUTINE_12] = 0x21;
    put16(bytecode, FEI_ROUTINE_12 + 1, 0x8000u | FEI_HAND_BACK_SPEED);
    bytecode[0x1f3] = 0x08; /* Wait for Fei to start routine 12 at priority 3. */
    bytecode[0x1f4] = FEI;
    bytecode[0x1f5] = (3 << 5) | 12;
    scene.actors[FEI].moveSpeed = held ? FEI_HAND_BACK_SPEED : FEI_COLD_SPEED;
    put32(scene.sprites[FEI], 0xac, 0xc3a80043u | ((u32)scene.actors[FEI].moveSpeed << 7));
    speed_calls = fei_speed_calls = animation_calls = 0;
    unsigned n = speed_calls, m = animation_calls, d = direction_calls, f = frame_calls;
    for (int apply = 0; apply <= 1; apply++) {
        assert(!PcPort_EstateActorsRestore(wire, apply));
        assert(!memcmp(&scene, &before, sizeof scene));
        assert(speed_calls == n && animation_calls == m);
        assert(direction_calls == d && frame_calls == f);
        assert(field_words[0x40a / 2] == marker);
    }
    /* The new actor and sprite may already have a different local stride. */
    scene.actors[FEI].moveSpeed = 0x10d;
    put32(scene.sprites[FEI], 0xac, 0xb5a00025u | (0x10du << 7));
    /* Only the two representations of Fei's stride may change. The cold
     * entrance keeps its own speed and all unrelated sprite bits survive. */
    unsigned fei_speed = ip == HAND_BACK ? FEI_HAND_BACK_SPEED : before.actors[FEI].moveSpeed;
    ActorData expected_fei;
    memcpy(&expected_fei, &before.actors[FEI], sizeof expected_fei);
    expected_fei.moveSpeed = fei_speed;
    assert(!memcmp(&scene.actors[FEI], &expected_fei, sizeof expected_fei));
    u32 packed = get32(before.sprites[FEI], 0xac);
    if (ip == HAND_BACK)
        packed = (packed & ~SPRITE_SPEED_MASK) | (fei_speed << 7);
    u8 expected_sprite[sizeof before.sprites[FEI]];
    memcpy(expected_sprite, before.sprites[FEI], sizeof expected_sprite);
    put32(expected_sprite, 0xac, packed);
    assert(!memcmp(scene.sprites[FEI], expected_sprite, sizeof expected_sprite));
    assert(fei_speed_calls == (unsigned)(ip == HAND_BACK));
    assert(speed_calls > fei_speed_calls && submitted_citan_speed == want->moveSpeed);
static void check_handback_refusals(void)
{
    u8 wire[PC_PORT_QUICK_CAVE_BYTES] = {0};
    setup(HAND_BACK);
    assert(PcPort_EstateActorsCapture(wire));

    /* These edits are outside the three serialized VM owners' script bodies.
     * The old F11 fingerprint alone cannot validate the native speed write. */
    setup(COLD);
    bytecode[FEI_ROUTINE_12] = 0x22;
    refusal(wire);
    bytecode[FEI_ROUTINE_12] = 0x21;
    const u16 invalid_speeds[] = {FEI_HAND_BACK_SPEED, 0x8000, 0x9000};
    for (unsigned i = 0; i < sizeof invalid_speeds / sizeof invalid_speeds[0]; i++) {
        put16(bytecode, FEI_ROUTINE_12 + 1, invalid_speeds[i]);
        refusal(wire);
    }
    put16(bytecode, FEI_ROUTINE_12 + 1, 0x8000u | FEI_HAND_BACK_SPEED);
    /* A routine cannot read its immediate across the next script boundary. */
    put16(field_script, 0x84 + FEI * 0x40 + 13 * 2, FEI_ROUTINE_12 + 2);
    refusal(wire);

    setup(COLD);
    put16(field_script, 0x84 + (FEI + 1) * 0x40, FEI_ROUTINE_12);
    refusal(wire);
    setup(COLD);
    scene.fields[FEI].pSpriteData = 0;
    refusal(wire);

    /* The cold director never issued the speed command. Its record needs
     * neither that routine's operand nor a writable Fei sprite. */
    setup(COLD);
    assert(PcPort_EstateActorsCapture(wire));
    bytecode[FEI_ROUTINE_12] = 0;
    scene.fields[FEI].pSpriteData = 0;
    before = scene;
    assert(PcPort_EstateActorsRestore(wire, 0));
    assert(!memcmp(&before, &scene, sizeof scene));
    assert(!speed_calls && !animation_calls && !direction_calls && !frame_calls);
    assert(PcPort_EstateActorsRestore(wire, 1));
    assert(!memcmp(&before.actors[FEI], &scene.actors[FEI], sizeof(ActorData)));
    assert(!memcmp(before.sprites[FEI], scene.sprites[FEI], sizeof before.sprites[FEI]));
    assert(!fei_speed_calls);
}

    check_handback_refusals();
