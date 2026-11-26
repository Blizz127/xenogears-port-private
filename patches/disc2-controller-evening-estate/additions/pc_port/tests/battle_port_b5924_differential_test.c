/* The stub below maps every 0x20 subdirectory to the valid Disc 1 base. */
int PcPort_BattleArchiveDisc(void) { return 1; }
int ArchiveDecodeAlignedSize(unsigned int index);
int ArchiveDecodeAlignedSize(unsigned int index)
    uint32_t sz = 0x40u + ((index & 7u) * 0x20u);
    record(ADDR_ADAS, index, sz, 0, 0);
    rt.file1.archive20_base = 3087u;
    uint32_t initial_file1_base = rt.file1.archive20_base;
    rt.file1.archive20_base = initial_file1_base;
    if (leaf->address == 0x8008AB70u && rt.file1.archive20_base) {
        fail(name, "archive20-base", "selecting (0x20,2) kept the file 1 base");
