int PcPort_BattleArchiveDisc(void) { return 1; }
    assert(!runtime.file1.archive20_base);
    runtime.file1.archive20_base = ORIGINAL_OFFSET;
        assert(g_CurArchiveOffset == ORIGINAL_OFFSET && runtime.file1.archive20_base == ORIGINAL_OFFSET);
        assert(g_CurArchiveOffset == ORIGINAL_OFFSET && runtime.file1.archive20_base == ORIGINAL_OFFSET);
        assert(runtime.file1.archive20_base == ORIGINAL_OFFSET && runtime.results.generation == 1);
    assert(g_CurArchiveOffset == ORIGINAL_OFFSET && runtime.file1.archive20_base == ORIGINAL_OFFSET);
    assert(g_CurArchiveOffset == ORIGINAL_OFFSET && runtime.file1.archive20_base == ORIGINAL_OFFSET);
    assert(g_CurArchiveOffset == ORIGINAL_OFFSET && runtime.file1.archive20_base == ORIGINAL_OFFSET);
