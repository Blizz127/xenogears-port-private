/* Compare the compiled Sell category, editor and item name with an earned
 * US Disc 1 Yggdrasil shelf. The runner authenticates the read-only inputs. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "robo_menu_frame_gpu.h"
#include "robo_menu_input.h"
#include "robo_menu_purchase.h"
#include "robo_menu_sell_editor.h"
#include "robo_menu_sell_rows.h"
#include "system/memory.h"

enum { RAM_BYTES = 0x200000, SHOP_BYTES = 0x4788 };
static SystemMenu menu;
static MenuUnk6 shared;
_Alignas(8) static unsigned char shop[SHOP_BYTES];
static unsigned char *weaponTable;
static int ownsMenu = 1, ownsShop = 1;

GameState g_GameState;
SystemMenu *g_Menu = &menu;
int g_GameSceneMapNum;
u8 D_80059171;
s16 g_SystemPalette1, g_SystemPalette2;
extern void *g_SystemDataFile, *g_SystemDataEntries, *g_SystemFontFile;
extern u32 D_8005934C, D_80059350, D_80059354, D_80059358, D_8005935C, D_80059364;

int PcPort_RoboMenuFrameOwnsBlock(const void *p, size_t n)
{
    return (p == &menu && ownsMenu && n <= sizeof(menu)) ||
           (p == shop && ownsShop && n <= sizeof(shop)) ||
           (p == &shared && n <= sizeof(shared)) ||
           (p == weaponTable && n <= 2003) ||
           (p == g_SystemDataFile && n <= 0x7178) ||
           (p == g_SystemFontFile && n <= 6594);
}

u_int HeapIsBlockPinned(HeapBlock *p)
{
    return p && ((void *)p == g_SystemDataFile || (void *)p == g_SystemFontFile);
}

static void read_exact(const char *path, void *dst, size_t size)
{
    FILE *f = fopen(path, "rb");
    assert(f && fread(dst, 1, size, f) == size && fgetc(f) == EOF && !ferror(f));
    assert(!fclose(f));
}

static uint32_t word(const void *p)
{
    uint32_t w;
    memcpy(&w, p, sizeof(w));
    return w;
}

static unsigned char *reference(const void *slot, size_t size)
{
    uint32_t w = word(slot);
    assert(w >= 0x80000000u && size <= RAM_BYTES && w - 0x80000000u <= RAM_BYTES - size);
    return PSX_ADDR(w);
}

static void install(PcRoboMenuImage *program)
{
    const unsigned char *originalMenu = reference(g_PsxRam + 0x625A0, 0x1E94);
    const unsigned char *originalShared = reference(originalMenu + 0x330, 0x1C);
    weaponTable = reference(originalShared + 8 + 4 * 4, 2003);
    uintptr_t compact = (uintptr_t)weaponTable;
    assert(compact <= UINT32_MAX);
    uint32_t address = (uint32_t)compact;
    memcpy(shared.unk8 + 4 * 4, &address, sizeof(address));
    menu.unk330 = &shared;
    menu.pShop = (MenuShop *)shop;
    menu.menu1Choice = originalMenu[0x336];
    menu.menu2Choice = originalMenu[0x338];
    D_80059171 = g_PsxRam[0x59171];
    g_GameSceneMapNum = word(g_PsxRam + 0x4F34C);
    uint8_t *gear = PcPort_RoboMenuMutable(program, 0x801D9084u, 1);
    assert(gear);
    *gear = g_PsxRam[0x1D9084];
    assert((g_GameSceneMapNum & 0x3fff) == 292 && D_80059171 == 5 &&
           *gear == 0 && menu.menu1Choice == 1 && menu.menu2Choice == 1);
    memcpy(&g_GameState, g_PsxRam + 0x6D634, sizeof(g_GameState));

    g_SystemDataFile = reference(g_PsxRam + 0x59368, 0x7178);
    g_SystemDataEntries = (unsigned char *)g_SystemDataFile + 4;
    g_SystemFontFile = reference(g_PsxRam + 0x5936C, 6594);
    D_8005934C = word(g_PsxRam + 0x5934C);
    D_80059350 = word(g_PsxRam + 0x59350);
    D_80059354 = word(g_PsxRam + 0x59354);
    D_80059358 = word(g_PsxRam + 0x59358);
    D_8005935C = (uint32_t)(uintptr_t)reference(g_PsxRam + 0x5935C, 1);
    D_80059364 = word(g_PsxRam + 0x59364);
    g_SystemPalette1 = 0x7440;
    g_SystemPalette2 = 0x7441;
}

static void check_scope(PcRoboMenuImage *program, PcRoboMenuImage *companion)
{
    assert(PcPort_RoboMenuSupportsSellCategory(program));
    D_80059171 = 7;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    D_80059171 = 5;
    g_GameSceneMapNum = 71;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    g_GameSceneMapNum = 292;
    uint8_t *gear = PcPort_RoboMenuMutable(program, 0x801D9084u, 1);
    *gear = 4;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    *gear = 0;
    menu.menu1Choice = 2;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    menu.menu1Choice = 1;
    menu.menu2Choice = 0;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    menu.menu2Choice = 2;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    menu.menu2Choice = 1;
    ownsShop = 0;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    ownsShop = 1;
    ownsMenu = 0;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    ownsMenu = 1;
    assert(!PcPort_RoboMenuSupportsSellCategory(NULL));
    assert(!PcPort_RoboMenuSupportsSellCategory(companion));
    menu.pShop = NULL;
    assert(!PcPort_RoboMenuSupportsSellCategory(program));
    menu.pShop = (MenuShop *)shop;

    g_GameSceneMapNum = 71;
    D_80059171 = 2;
    assert(PcPort_RoboMenuSupportsSellCategory(program));
    menu.menu2Choice = 0;
    assert(PcPort_RoboMenuSupportsSellCategory(program));
    D_80059171 = 5;
    g_GameSceneMapNum = 292;
    menu.menu2Choice = 1;
}

static void check_shelf(PcRoboMenuImage *program)
{
    PcRoboSellEditor *owner = NULL;
    PcRoboSellView v;
    GameState before = g_GameState;
    assert(PcPort_RoboMenuSupportsSellCategory(program));
    assert(!PcPort_RoboMenuBeginSellEditor(program, &owner, &v));
    assert(owner && v.action == PC_ROBO_SELL_REDRAW_LIST && v.origin == 0 && v.row == 0);
    assert(v.count == 1 && v.capacity == 100 && v.types[0] == 4 && v.ids[0] == 17 &&
           v.remaining[0] == 1 && v.selected[0] == 0 && v.unitPrice == 800);
    assert(v.originalGold == 1255 && v.proceeds == 0 && v.projectedGold == 1255);

    const uint8_t *text = PcPort_RoboMenuSellItemName(v.types[0], v.ids[0]);
    const char *expected = "Iron GWhip";
    assert(text);
    for (size_t i = 0; i <= strlen(expected); ++i) {
        unsigned c = text[i];
        int glyph = c == 0 ? 0 : c == 0x10 ? ' ' :
                    c >= 0x20 && c <= 0x39 ? 'A' + (int)c - 0x20 :
                    c >= 0x3d && c <= 0x56 ? 'a' + (int)c - 0x3d : -1;
        assert(glyph == expected[i]);
    }
    PcRoboSellEditor *foreign = owner;
    assert(PcPort_RoboMenuReleaseSellEditor(&foreign) && foreign == owner);
    assert(!PcPort_RoboMenuReleaseSellEditor(&owner) && !owner);
    assert(!memcmp(&g_GameState, &before, sizeof(before)));
}

int main(int argc, char **argv)
{
    assert(argc == 4);
    read_exact(argv[1], g_PsxRam, RAM_BYTES);
    static unsigned char file10[82080];
    read_exact(argv[2], file10, sizeof(file10));
    PcRoboMenuImage *program = PcPort_RoboMenuLoadBytes(PC_ROBO_MENU_PROGRAM, file10, sizeof(file10));
    static unsigned char file12[50868];
    read_exact(argv[3], file12, sizeof(file12));
    PcRoboMenuImage *companion = PcPort_RoboMenuLoadBytes(PC_ROBO_MENU_COMPANION, file12, sizeof(file12));
    assert(program && companion);
    install(program);
    check_scope(program, companion);
    check_shelf(program);
    PcPort_RoboMenuFree(companion);
    PcPort_RoboMenuFree(program);
    puts("PASS Yggdrasil Sell Weapons: Iron GWhip x1, 800 G, wallet 1255 G; scope, owner and Bal controls");
    return 0;
}
