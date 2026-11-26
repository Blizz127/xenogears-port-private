#include "robo_menu_stock_camera_goal.h"
int PcPort_RoboMenuSupportsSellCategory(PcRoboMenuImage *program)
    extern int g_GameSceneMapNum;
    if (!live_shop(program, 0x4788) || g_Menu->menu1Choice != 1 || g_Menu->menu2Choice >= 2)
        return 0;
    if (D_80059171 == 2) return 1;
    if (D_80059171 != 5 || (g_GameSceneMapNum & 0x3fff) != 292 || g_Menu->menu2Choice != 1)
        return 0;
    return PcPort_RoboMenuStockGearCurrent(program, 0, 1, 1);
