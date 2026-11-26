/* A remembered startup stamp is a boot optimization. A battle load needs a
 * full check in this process before it can use that image as evidence. */
static int battleDisc HOST_ONLY_BSS;
static int battleBootEligible HOST_ONLY_BSS;
extern int PsyX_CdAuthorizeLid(int (*verify)(int)) __attribute__((weak));
extern unsigned char D_80018004[] __attribute__((weak));
extern uint32_t g_ArchiveHeader __attribute__((weak));
extern uint32_t g_ArchiveDebugTable __attribute__((weak));
extern int PsyX_CdReadManagedSectors(uint32_t lba, uint32_t count, void *out) __attribute__((weak));

void PcPort_NotifyVerifiedDiscInsertion(int disc)
{
    /* A failed insert, an opened lid or a close on the held image must not
     * change which file was accepted. The getter checks the live drive. */
    battleBootEligible = 0;
    battleDisc = (disc == 1 || disc == 2) ? disc : 0;
}

/* The SDK offers its actual file descriptor to the callback while holding its
 * cursor and media locks. Always decline enrollment: a read-only identity
 * query must not make the tray managed or change its existing verifier. */
static int inspect_battle_boot_fd(int fd)
{
    battleBootEligible = 0;
    if (PcPort_VerifyMountedDisc1(fd))
        battleDisc = 1;
    return 0;
}

void PcPort_VerifyBattleBootDisc(void)
{
    if (battleBootEligible && PsyX_CdAuthorizeLid)
        (void)PsyX_CdAuthorizeLid(inspect_battle_boot_fd);
}

static int battle_directory_matches(const unsigned char *directory, int disc)
{
    static const char *const expected[] = {
        "610b2c7c2322e8dadeb5f87ace961e4e7dc55378e479a5b1239343225717fc67",
        "fd8aff8403d852ce4384fcec547a5f23179e3d2d157157f572943f4ee0349788"
    };
    unsigned char digest[32];
#ifdef _WIN32
    XenoWinHash ctx = {0};
    int ok = xeno_win_hash_init(&ctx, BCRYPT_SHA256_ALGORITHM) &&
             BCRYPT_SUCCESS(BCryptHashData(ctx.hash, (PUCHAR)directory, 122, 0)) &&
             BCRYPT_SUCCESS(BCryptFinishHash(ctx.hash, digest, sizeof digest, 0));
    xeno_win_hash_close(&ctx);
#else
    unsigned int length = 0;
    int ok = EVP_Digest(directory, 122, digest, &length, EVP_sha256(), NULL) == 1 &&
             length == sizeof digest;
#endif
    if (!ok) return 0;
    const char *reference = expected[disc - 1];
    for (size_t i = 0; i < sizeof digest; ++i) {
        char pair[3];
        snprintf(pair, sizeof pair, "%02x", (unsigned)digest[i]);
        if (memcmp(pair, reference + 2 * i, 2)) return 0;
    }
    return 1;
}

int PcPort_BattleArchiveDisc(void)
{
    /* ArchiveInit stores the actual non-PIE host array address. A synthetic
     * guest pointer, a debug directory, and a standalone filename lookup
     * cannot stand in for the game's directory installed from this drive. */
    if (!D_80018004 || !&g_ArchiveHeader || !&g_ArchiveDebugTable || g_ArchiveDebugTable ||
        g_ArchiveHeader != (uintptr_t)D_80018004 || !PsyX_CdReadManagedSectors)
        return 0;
    int disc = battleDisc;
    if (!disc) return 0;

    /* Read both sectors in one positional operation on the actual mounted
     * image. The SDK holds its cursor-use lock until the read is complete.
     * No disc command, cursor movement or implicit path fallback. */
    unsigned char sectors[18][2048];
    if (PsyX_CdReadManagedSectors(23, 18, sectors) != 1) return 0;
    const unsigned char *marker = sectors[0];
    const unsigned char *directory = sectors[17];
    unsigned entry = (unsigned)directory[0x40] | ((unsigned)directory[0x41] << 8);
    if (marker[3] != '0' + disc || memcmp(marker + 4, "_XEN", 4) ||
        directory[0x78] != disc || directory[0x79] != 0 ||
        entry != (disc == 1 ? 3088u : 3083u) ||
        memcmp(D_80018004, directory, 122) || !battle_directory_matches(directory, disc))
        return 0;
    return disc;
}

    battleDisc = 0;
    battleBootEligible = 0;
        battleBootEligible = 1;
    battleBootEligible = 1;
