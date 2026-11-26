extern uint8_t D_80059171;
    /* Sell Weapons turns the retained actor to face forward. The hangar
     * uses Weltall; Bal can also switch to Brigandier before this draw. */
    const int hangarWeltall = s->field == 292 && D_80059171 == 5 && s->banks.shape == &weltall;
        !(away == 0 && (s->field == 71 || hangarWeltall) && g_Menu && s->frame.menu == g_Menu &&
