#include "field/camera.h"
enum { FEI = 5, BART = 6, ROCK = 7, SLOTS = 8, FOREST_ACTORS = 17, TAURA_ACTORS = 24, LAB_ACTORS = 34, TRAIL = 32, STRIDE = 0x48 };
    FieldActor slot[LAB_ACTORS];
    ActorData actor[LAB_ACTORS];
    _Alignas(4) u8 sprite[LAB_ACTORS][0x180];
CameraInterpolation g_CamInterpolation;
    if (g_GameSceneMapNum == 616)
        assert(sprite == scene.sprite[2] || sprite == scene.sprite[14] || sprite == scene.sprite[16]);
    else if (g_GameSceneMapNum == 102)
void func_800821F4(void *sprite, s16 animation, void *field)
{
    assert(g_GameSceneMapNum == 616 && field == &scene.slot[2] && sprite == scene.sprite[2]);
    assert(animation == scene.actor[2].curAnimationId);
    scene.actor[2].scriptFlags.flags &= ~0x800u;
}

    FieldActor slots[LAB_ACTORS];
    ActorData actors[LAB_ACTORS];
    u8 sprites[LAB_ACTORS][0x180];
static void lab_setup(void)
{
    taura_setup();
    g_GameSceneMapNum = 616;
    g_FieldNumActors = D_800ADBFC = LAB_ACTORS;
    test_script_words[2] = 615;
    test_script_words[0x1c0 / 2] = 0x1400;
    D_800AFB54 = 2;
    D_800AFB44[0] = 136;
    D_800AFB44[1] = 2;
    D_800B2360 = 9;
    D_800B2364 = 18;
    D_800B2368 = 0;
    g_CamInterpolation.curAngleY = 512;
    pose(1, 0, (VECTOR){-25755648,1966080,-11599872,0}, 61, 0);
    scene.actor[1].rotation.vy = (s16)0xfffe;
    pose(2, 1, (VECTOR){-25886720,0,11010048,0}, 32, 0);
    scene.actor[2].scriptFlags.flags = 0x010204b0;
    pose(14, 255, (VECTOR){-6094848,1966080,-11468800,0}, 68, 0);
    pose(16, 255, (VECTOR){-15858992,2031616,-11398704,0}, 68, 0);
    for (int i = 0; i < TRAIL; ++i) {
        u8 *row = D_800B14F0 + i * STRIDE;
        set_word(row + 0x16, 61 + i);
        set_word(row + 0x18, 0);
    }
}

static void taura_laboratory(void)
{
    enum { NPC_START = 12 + PC_PORT_QUICK_BART_BYTES };
    lab_setup();
    ActorData *elly = &scene.actor[2];
    elly->position = (VECTOR){-22806528,2031616,-11534336,0};
    elly->rotation.vy = 2048;
    elly->curAnimationId = 1;
    elly->flags |= 0x1000;
    elly->scriptFlags.flags |= 0x04000800;
    scene.actor[14].rotation = (SVECTOR){-43,-43,4053,0};
    scene.actor[16].rotation = (SVECTOR){(s16)34751,(s16)34751,1983,0};
    scene.actor[14].flags |= 0x200;
    scene.actor[16].flags &= ~0x200u;
    set_word(scene.sprite[14] + 0x80, 4565);
    set_word(scene.sprite[16] + 0x80, 2495);
    struct FieldImage earned;
    observe(&earned);
    u8 wire[PC_PORT_QUICK_PARTY_BYTES], again[sizeof wire];
    assert(PcPort_TauraPartyCapture(wire));
    assert(!memcmp(wire, "F616", 4) && wire[NPC_START - 2] == 1 && wire[NPC_START - 1] == 1);

    lab_setup();
    memset(D_800B14F0, 0, sizeof D_800B14F0);
    for (int i = 0; i < TRAIL; ++i)
        set_wide(D_800B14F0 + i * STRIDE, 0x4000);
    D_800B2360 = D_800B2364 = 0;
    scene.actor[16].flags |= 0x200;
    scene.actor[14].flags &= ~0x200u;
    for (int i = 0; i < 2; ++i) {
        int slot = i == 0 ? 14 : 16;
        scene.actor[slot].rotation = (SVECTOR){0};
        set_word(scene.sprite[slot] + 0x32, (u16)-1);
        set_word(scene.sprite[slot] + 0x80, 7);
    }
    assert(PcPort_TauraPartyRestore(wire));
    assert_pose(2, &earned);
    assert_restored_trail(&earned);
    assert(elly->curAnimationId == 1 && (elly->scriptFlags.flags & 0x04000800) == 0x04000800);
    assert(elly->flags & 0x1000);
    assert(PcPort_TauraPartyCapture(again) && !memcmp(again, wire, sizeof wire));
    test_script_words[0x1c0 / 2] = 0x14c0; /* Earlier scenario flags can coexist. */
    assert(PcPort_TauraPartyCapture(again) && PcPort_TauraPartyRestore(again));
    assert(test_script_words[0x1c0 / 2] == 0x14c0);
    assert(!PcPort_TauraWorkshopPartyCapture(again));
    assert_rejected(PcPort_TauraWorkshopPartyRestore, wire);
    assert_rejected(restore_workshop_party, wire);

    const size_t invalid[] = {0, 4, 6, 8 + 172 - 1, NPC_START - 4, NPC_START - 2,
                              NPC_START - 1, NPC_START + 1, NPC_START + 5,
                              NPC_START + 7, NPC_START + 9, NPC_START + 10,
                              NPC_START + 24, sizeof wire - 1};
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        memcpy(again, wire, sizeof again);
        again[invalid[i]] = 0x67;
        assert_rejected(PcPort_TauraPartyRestore, again);
    }
    scene.actor[2].walkmeshId = 1;
    assert(!PcPort_TauraPartyCapture(again));
    scene.actor[2].walkmeshId = 0;
    set_word(D_800B14F0 + STRIDE + 0x18, 1);
    assert(!PcPort_TauraPartyCapture(again));
    set_word(D_800B14F0 + STRIDE + 0x18, 0);
    test_script_words[1] = 1;
    assert_rejected(PcPort_TauraPartyRestore, wire);
    test_script_words[1] = 0;
    test_script_words[0x1c0 / 2] = 0x400;
    assert_rejected(PcPort_TauraPartyRestore, wire);
    test_script_words[0x1c0 / 2] = 0x1c00;
    assert_rejected(PcPort_TauraPartyRestore, wire);
    test_script_words[0x1c0 / 2] = 0x1400;
    scene.slot[16].pSpriteData = 0;
    assert_rejected(PcPort_TauraPartyRestore, wire);

    lab_setup();
    placement_calls = trail_initializations = trail_records = 0;
    assert(PcPort_TauraPartyRegroup());
    assert(!memcmp(&elly->position, &scene.actor[1].position, 12));
    assert((u16)elly->rotation.vy == 0x8400 && elly->rotation.vz == 1024);
    assert(elly->scriptFlags.flags & 0x04000000);
    assert(D_800B2360 == 0 && trail_records == TRAIL);
    lab_setup();
    scene.actor[1].position.vx += 5 * 65536;
    const s16 previous = elly->rotation.vy;
    placement_calls = trail_initializations = trail_records = 0;
    assert(PcPort_TauraPartyRegroup());
    assert(elly->position.vx == scene.actor[1].position.vx && elly->rotation.vy == previous);
}

    taura_laboratory();
    puts("PASS 616 postmeeting Elly, trail, NPC facings, legacy regroup and atomic rejection");
