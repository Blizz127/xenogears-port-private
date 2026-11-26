    if (targetDisc == 1) {
        if (!first || !PsyX_CdInsertImage(first, verify)) return 0;
        PcPort_NotifyVerifiedDiscInsertion(1);
        return 1;
    }
    if (!PsyX_CdInsertImage(second, verify)) return 0;
    PcPort_NotifyVerifiedDiscInsertion(2);
    return 1;
        if (!PsyX_CdAuthorizeLid(PcPort_VerifyMountedDisc1)) return 0;
        PcPort_NotifyVerifiedDiscInsertion(1);
        return PsyX_CdSetLidOpen(1);
