 * terminator pair 0x0F,0x00), reset the battle table and leading sound
 * controller fields, and set the two mode bytes. */
#ifdef XENO_PC_PORT
extern u16 D_8005A3A0[16];
#endif
#ifdef XENO_PC_PORT
    /* Retail 8001BAC4..BADC clears adjacent guest ranges 8005A3A0..A3BF
     * and 8005A3C0..A3C7. Their native symbols have independent storage. */
    g_SoundVolumeController.commonAttr.mvol.right = 0;
    g_SoundVolumeController.commonAttr.mvol.left = 0;
    g_SoundVolumeController.commonAttr.mask = 0;
    for (k = 15; k >= 0; --k) {
        D_8005A3A0[k] = 0;
    }
#else
#endif
