static void check_brigandier_parts_after_sell_weapons(void)
{
    static MenuManager manager;
    static MenuUnk1 decor;
    static MenuSelectionMenu selection;
    PcRoboRootLinks root = {
        .menu = &menu, .manager = &manager, .decor = &decor, .selection = &selection,
        .callerReturnPc = STOCK_TRANSITION, .choice = 2, .previousStripRows = 2,
        .strip = &strip, .counter = counter
    };
    menu.menu2Choice = 0;
    strip.unk1400 = 2;
    strip.unk1404 = 6;

    /* File 06, backing out of Brigandier's empty Sell Weapons shelf. The
     * two title packets retained the same second-row cursor. The badges
     * retained different glyph tails. All six tags survived entry to Buy. */
    const uint8_t *secondCursor = (const uint8_t *)&strip.polys0[2];
    const unsigned firstBadgeTail[] = {0x440, 0x378};
    const unsigned secondBadgeTail[] = {0x918, 0x8f0};
    for (unsigned bank = 0; bank < 2; ++bank) {
        const unsigned delta = bank * sizeof(POLY_FT4);
        const uint8_t *title = counter + 0xd0 + delta;
        const uint8_t *firstBadge = counter + 0x620 + delta;
        const uint8_t *secondBadge = counter + 0xd00 + delta;
        assert(known_prior_link(&root, title, dma(secondCursor)));
        assert(known_prior_link(&root, title, dma(&strip.polys0[3])));
        assert(known_prior_link(&root, firstBadge, dma(counter + firstBadgeTail[bank])));
        assert(known_prior_link(&root, secondBadge, dma(counter + secondBadgeTail[bank])));

        assert(!known_prior_link(&root, title, dma(&strip.polys0[6 + bank])));
        assert(!known_prior_link(&root, title + 1, dma(secondCursor)));
        assert(!known_prior_link(&root, title, dma(secondCursor) + 1));
        assert(!known_prior_link(&root, firstBadge, dma(counter + secondBadgeTail[bank])));
        assert(!known_prior_link(&root, secondBadge, dma(counter + firstBadgeTail[bank])));
        assert(!known_prior_link(&root, firstBadge, dma(counter + 0x300 + delta)));
    }

    const uint8_t *title = counter + 0xd0;
    root.callerReturnPc = STOCK_LIST;
    assert(!known_prior_link(&root, title, dma(secondCursor)));
    root.callerReturnPc = STOCK_TRANSITION;
    root.previousStripRows = 0;
    assert(!known_prior_link(&root, title, dma(secondCursor)));
    root.previousStripRows = 2;
    selectedGear = 0;
    assert(!known_prior_link(&root, title, dma(secondCursor)));
    selectedGear = 4;
    D_80059171 = 5;
    assert(!known_prior_link(&root, title, dma(secondCursor)));
    D_80059171 = 2;
    g_GameSceneMapNum = 70;
    assert(!known_prior_link(&root, title, dma(secondCursor)));
    g_GameSceneMapNum = 71;
}

    check_brigandier_parts_after_sell_weapons();
    puts("PASS Bal Brigandier Buy Parts after leaving Sell Weapons in both packet banks");
