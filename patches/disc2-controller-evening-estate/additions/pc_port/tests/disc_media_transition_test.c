#include <sys/wait.h>
uint8_t D_80018004[0x7a];
uint32_t g_ArchiveHeader, g_ArchiveDebugTable;
static uint8_t table[0x8000], D_801EA8F4[8];
static uint32_t g_ArchiveTable;
           (reads == 2 && lba == 0x28 && bytes == sizeof D_80018004));
            assert(PcPort_BattleArchiveDisc() == 0);
                assert(PcPort_BattleArchiveDisc() == 0);
            /* The new drive is closed, but the game's directory still belongs
             * to its previous disc until the retail validator reloads it. */
            if (disc2_path) assert(PcPort_BattleArchiveDisc() == 0);
    assert(fread(sector, 1, sizeof D_80018004, f) == sizeof D_80018004);
    assert(memcmp(D_80018004, sector, sizeof D_80018004) == 0 && fclose(f) == 0);
static int alter_directory_read;
static _Thread_local uint64_t positional_bytes;
    ssize_t got = __real_pread(fd, buf, bytes, offset);
    if (got > 0) positional_bytes += (uint64_t)got;
    if (alter_directory_read && offset == 0x28 * 2352 + 24 && got > 0) {
        ((uint8_t *)buf)[0] ^= 1;
        alter_directory_read = 0;
    }
    return got;
}

static void check_disc2_directory_identity(void)
{
    /* Change a directory entry while preserving the disc-number bytes. */
    alter_directory_read = 1;
    assert(PcPort_BattleArchiveDisc() == 0 && !alter_directory_read);
    D_80018004[0] ^= 1;
    assert(PcPort_BattleArchiveDisc() == 0);
    /* Even when the mounted read and the installed directory agree, their
     * altered contents must fail the retail directory fingerprint. */
    alter_directory_read = 1;
    assert(PcPort_BattleArchiveDisc() == 0);
    alter_directory_read = 0;
    D_80018004[0] ^= 1;
    assert(PcPort_BattleArchiveDisc() == 2);
        assert(PcPort_BattleArchiveDisc() == 1);
    mounted_disc = D_80018004[0x78];
    assert(PcPort_BattleArchiveDisc() == (int)mounted_disc);
}

static void check_distinct_boot_descriptor(const char *verified_path)
{
    /* Isolate this SDK instance before initializing the ordinary drive. The
     * parent has already accepted the genuine startup path. */
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        FILE *original = fopen(verified_path, "rb");
        FILE *other = tmpfile();
        assert(original && other && fseek(original, 0, SEEK_END) == 0);
        long length = ftell(original);
        assert(length > 0 && ftruncate(fileno(other), length) == 0);
        const int markers[] = {0x17, 0x28};
        unsigned char sector[2048];
        for (size_t i = 0; i < sizeof markers / sizeof markers[0]; ++i) {
            long offset = (long)markers[i] * 2352 + 24;
            assert(fseek(original, offset, SEEK_SET) == 0);
            assert(fread(sector, 1, sizeof sector, original) == sizeof sector);
            assert(pwrite(fileno(other), sector, sizeof sector, offset) == (ssize_t)sizeof sector);
        }
        memcpy(D_80018004, sector, sizeof D_80018004);
        assert(fclose(original) == 0);
        char path[64];
        assert(snprintf(path, sizeof path, "/dev/fd/%d", fileno(other)) > 0);
        PsyX_CDFS_Init(path, 0, 2352);
        assert(CdInit() == 1);
        assert(PsyX_CdReadManagedSectors(0x17, 1, sector) == 1 && !memcmp(sector, "DS01_XENOGEARS", 14));
        assert(PsyX_CdReadManagedSectors(0x28, 1, sector) == 1);
        assert(memcmp(D_80018004, sector, sizeof D_80018004) == 0);
        g_ArchiveHeader = (u32)(uintptr_t)D_80018004;
        PcPort_VerifyBattleBootDisc();
        assert(PcPort_BattleArchiveDisc() == 0);
        assert(!PsyX_CdSetLidOpen(1));
        assert(fclose(other) == 0);
        _exit(0);
    }
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(PcPort_BattleArchiveDisc() == 0);
    /* A non-directory keeps the startup stamp out of the user's cache. */
    assert(setenv("XDG_CACHE_HOME", invalid_path, 1) == 0);
    assert(PcPort_VerifyDiscImage(argv[1]) == 0);
    check_distinct_boot_descriptor(argv[1]);
    unsigned long scratch[2048 / sizeof(unsigned long)];
    assert(PcPort_BattleArchiveDisc() == 0);
    assert(PsyX_CdReadManagedSectors(0x28, 1, scratch) == 1);
    assert((uintptr_t)table <= UINT32_MAX && (uintptr_t)D_80018004 <= UINT32_MAX);
    memcpy(D_80018004, scratch, sizeof D_80018004);
    memcpy(table, scratch, sizeof D_80018004);
    g_ArchiveHeader = (u32)(uintptr_t)D_80018004;
    assert(PcPort_BattleArchiveDisc() == 0);
#ifdef XENO_TEST_DISC_INDEX_READ_ERROR
    uint64_t before = positional_bytes;
#endif
    PcPort_VerifyBattleBootDisc();
#ifdef XENO_TEST_DISC_INDEX_READ_ERROR
    assert(positional_bytes - before == 718738272u);
    before = positional_bytes;
#endif
    /* Repeating boot setup must not rehash. The battle query only reads
     * the 18 sectors spanning the marker and installed directory. */
    PcPort_VerifyBattleBootDisc();
    assert(PcPort_BattleArchiveDisc() == 1);
#ifdef XENO_TEST_DISC_INDEX_READ_ERROR
    assert(positional_bytes - before == 18u * 2048u);
#endif
    g_ArchiveHeader = (u32)(uintptr_t)table;
    assert(PcPort_BattleArchiveDisc() == 0);
    g_ArchiveHeader = (u32)(uintptr_t)D_80018004;
    g_ArchiveDebugTable = (u32)(uintptr_t)table;
    assert(PcPort_BattleArchiveDisc() == 0);
    g_ArchiveDebugTable = 0;
    assert(PcPort_BattleArchiveDisc() == 1);
    assert(!PsyX_CdSetLidOpen(1));
    assert(PcPort_BattleArchiveDisc() == 1);
    memset(D_80018004, 0xa5, sizeof D_80018004);
    for (size_t i = 0; i < sizeof D_80018004; ++i) assert(D_80018004[i] == 0xa5);
    assert(D_80018004[0x78] == 1 && D_80018004[0x79] == 0);
    assert(PcPort_BattleArchiveDisc() == 1);
            assert(D_80018004[0x78] == requested && D_80018004[0x79] == 0);
            mounted_disc = D_80018004[0x78];
            assert(PcPort_BattleArchiveDisc() == requested);
#ifdef XENO_TEST_DISC_INDEX_READ_ERROR
            if (step == 0) check_disc2_directory_identity();
#endif
            int directory = D_80018004[8] | (D_80018004[9] << 8);
        puts("PASS battle archive identity on startup, ordinary and title exchanges, stale directories, open lid and rollback");
