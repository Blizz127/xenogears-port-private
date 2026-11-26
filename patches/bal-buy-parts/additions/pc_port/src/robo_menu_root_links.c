static int brigandier_parts_after_category(const PcRoboRootLinks *r)
           !r->returningToParts && (r->previousStripRows == 2 || r->previousStripRows == 4) &&
           PcPort_RoboMenuStockGearCurrent(r->program, 4, 2, 0);
    if (brigandier_parts_after_category(r)) return 1;
        /* Both title banks can retain the last cursor of the previous strip.
         * Sell Weapons drew two rows and Tune Up drew four. */
        if (brigandier_parts_after_category(r) &&
