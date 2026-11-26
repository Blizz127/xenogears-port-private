/* Isolate the camera write between two preparations of the retained actor.
 * The complete node bank and camera come from the original owned shelf.
 * The oracle runs the original BA2C once to produce the next bank. */
#include <assert.h>
#include <stdio.h>
#include "robo_menu_mesh.c"

SystemMenu *g_Menu;
int g_GameSceneMapNum;
uint8_t D_80059171;
static SystemMenu menu, otherMenu;
static PcRoboOpeningActor carried;
static uint8_t liveNodes[sizeof(carried.afterNodes)], camera[0x34];
static uint8_t ram[0x200000];

static void store16(void *p, int16_t v) { memcpy(p, &v, sizeof(v)); }

static const uint8_t *capture_ptr(const void *slot, size_t bytes)
{
    uint32_t address = word(slot);
    assert(bytes <= sizeof(ram) && address >= 0x80000000u &&
           address - 0x80000000u <= sizeof(ram) - bytes);
    return ram + (address - 0x80000000u);
}

static void install(const char *path)
{
    FILE *f = fopen(path, "rb");
    assert(f && fread(ram, 1, sizeof(ram), f) == sizeof(ram));
    assert(fgetc(f) == EOF && !ferror(f) && !fclose(f));
    const uint8_t *srcMenu = capture_ptr(ram + 0x625a0, 0x1e94);
    const uint8_t *actor = capture_ptr(ram + 0x1e8670 + 4, SHELL_BYTES);
    const uint8_t *nodes = capture_ptr(actor + 4, node_bytes(&weltall));
    assert((word(ram + 0x4f34c) & 0x3fff) == 292 && ram[0x59171] == 5 &&
           ram[0x1d9084] == 0 && srcMenu[0x336] == 1 && srcMenu[0x338] == 1);
    menu.menu1Choice = srcMenu[0x336];
    menu.menu2Choice = srcMenu[0x338];
    menu.unk337 = srcMenu[0x337];
    menu.unk339 = srcMenu[0x339];
    menu.unk33A = srcMenu[0x33a];
    g_Menu = &menu;
    carried.field = g_GameSceneMapNum = 292;
    D_80059171 = 5;
    carried.frame.menu = &menu;
    carried.banks.shape = &weltall;
    carried.nodes = liveNodes;
    carried.turnCamera = camera;
    memcpy(liveNodes, nodes, node_bytes(&weltall));
    memcpy(carried.afterNodes, nodes, node_bytes(&weltall));
    memcpy(camera, ram + 0x1d9050, sizeof(camera));
}

static void verify_pair(int priorYaw, int nextYaw)
{
    assert(priorYaw != nextYaw);
    store16(carried.afterNodes + NODE_YAW, priorYaw);
    store16(liveNodes + NODE_YAW, nextYaw);
    assert(carried_nodes(&carried, liveNodes));

    store16(liveNodes + NODE_YAW, nextYaw + 1);
    assert(!carried_nodes(&carried, liveNodes));
    store16(liveNodes + NODE_YAW, nextYaw);
    store16(carried.afterNodes + NODE_YAW, priorYaw - 1);
    assert(!carried_nodes(&carried, liveNodes));
    store16(carried.afterNodes + NODE_YAW, priorYaw);

    liveNodes[NODE_YAW - 1] ^= 1;
    assert(!carried_nodes(&carried, liveNodes));
    liveNodes[NODE_YAW - 1] ^= 1;
    liveNodes[node_bytes(&weltall) - 1] ^= 1;
    assert(!carried_nodes(&carried, liveNodes));
    liveNodes[node_bytes(&weltall) - 1] ^= 1;

    camera[0x32] ^= 1;
    assert(!carried_nodes(&carried, liveNodes));
    camera[0x32] ^= 1;
    const uint8_t *registered = carried.turnCamera;
    carried.turnCamera = NULL;
    assert(!carried_nodes(&carried, liveNodes));
    carried.turnCamera = registered;

    otherMenu = menu;
    g_Menu = &otherMenu;
    assert(!carried_nodes(&carried, liveNodes));
    g_Menu = &menu;
    carried.field = 291;
    assert(!carried_nodes(&carried, liveNodes));
    carried.field = 292;
    D_80059171 = 7;
    assert(!carried_nodes(&carried, liveNodes));
    D_80059171 = 5;
    carried.banks.shape = &brigandier;
    assert(!carried_nodes(&carried, liveNodes));
    carried.banks.shape = &weltall;
    menu.menu2Choice = 0;
    assert(!carried_nodes(&carried, liveNodes));
    menu.menu2Choice = 1;
    menu.menu1Choice = 2;
    assert(!carried_nodes(&carried, liveNodes));
    menu.menu1Choice = 1;
    assert(carried_nodes(&carried, liveNodes));

    /* Preserve the prior Bal forward shelf admission on the identical yaw. */
    carried.field = g_GameSceneMapNum = 71;
    D_80059171 = 2;
    assert(carried_nodes(&carried, liveNodes));
    carried.field = g_GameSceneMapNum = 292;
    D_80059171 = 5;
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    install(argv[1]);
    static uint8_t originalPrior[NODES * NODE_BYTES];
    memcpy(originalPrior,liveNodes,sizeof(originalPrior));
    int priorYaw = halfword(originalPrior + NODE_YAW);
    uint32_t priorSum = word(camera + 0x2c);
    install(argv[2]);
    assert(!memcmp(originalPrior,liveNodes,NODE_YAW));
    assert(!memcmp(originalPrior + NODE_YAW + 2,liveNodes + NODE_YAW + 2,
                   sizeof(originalPrior) - NODE_YAW - 2));
    memcpy(carried.afterNodes, originalPrior, sizeof(originalPrior));
    int nextYaw = halfword(liveNodes + NODE_YAW);
    uint32_t start = word(camera + 8), goal = word(camera + 0x14);
    assert((start == (uint32_t)-1024 && !goal && priorYaw < nextYaw) ||
           (!start && goal == (uint32_t)-1024 && priorYaw > nextYaw));
    uint32_t step = word(camera + 0x20), sum = word(camera + 0x2c);
    assert(step && sum - step == priorSum);
    assert(turn_bearing(camera, sum) == nextYaw && turn_bearing(camera, priorSum) == priorYaw);
    carried.field = g_GameSceneMapNum = 71;
    D_80059171 = 2;
    assert(carried_nodes(&carried, liveNodes));
    fprintf(stderr, "Bal comparison accepts %d to %d\n", priorYaw, nextYaw);
    carried.field = g_GameSceneMapNum = 292;
    D_80059171 = 5;
    verify_pair(priorYaw, nextYaw);
    printf("PASS original Sell Weapons slot-1 carried yaw %d to %d, step %u, accumulation %u\n",
           priorYaw, nextYaw, step, sum);
    return 0;
}
