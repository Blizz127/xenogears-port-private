/* The loaded controller is identical on both US discs, but the directory
 * bases differ. Disc 2's (0x20,2) even shares Disc 1's (0x20,0) base. */
extern int PcPort_BattleArchiveDisc(void) __attribute__((weak));

static int file1_mounted_archive_matches(uint32_t archive)
{
    if (!PcPort_BattleArchiveDisc || (archive != 3087u && archive != 3082u))
        return 0;
    return PcPort_BattleArchiveDisc() == (archive == 3087u ? 1 : 2);
}

        if (runtime->file1.archive20_base && cpu->gpr[4] == 1 &&
            cpu->gpr[5] == FILE1_BASE && cpu->gpr[6] == 0 &&
            cpu->gpr[7] == 0x80 && size == FILE1_BYTES &&
            runtime_read(runtime, 0x8004fe14u, 4, &archive) == 0 &&
            archive == runtime->file1.archive20_base && file1_mounted_archive_matches(archive))
            *candidate = (int)archive;
    uint32_t archive = 0;
    if (target == 0x80028470u || target == 0x8008ab4cu ||
        target == 0x8008ab70u || target == 0x8008ab94u) {
        /* Native main44 wrappers call ArchiveSetIndex directly. Only AB4C
         * selects (0x20,0); AB70 and AB94 select subdirectories 2 and 3. */
        runtime->file1.archive20_base = 0;
        if ((target == 0x8008ab4cu ||
             (target == 0x80028470u && cpu->gpr[4] == 0x20 && cpu->gpr[5] == 0)) &&
            runtime_read(runtime, 0x8004fe14u, 4, &archive) == 0 &&
            (target != 0x80028470u || result == archive) &&
            file1_mounted_archive_matches(archive))
            runtime->file1.archive20_base = archive;
    } else if (target == 0x800295d8u && candidate && (int32_t)result == 0 &&
               (uint32_t)candidate == runtime->file1.archive20_base &&
               runtime_read(runtime, 0x8004fe14u, 4, &archive) == 0 &&
               archive == (uint32_t)candidate && file1_mounted_archive_matches(archive)) {
