/* Record only a successful SDK insertion or initial lid authorization using
 * the actual mounted descriptor and the corresponding full-disc verifier. */
void PcPort_NotifyVerifiedDiscInsertion(int disc);
/* Authenticate the actual boot drive once its archive reads have drained. */
void PcPort_VerifyBattleBootDisc(void);
/* 1/2 for an authenticated US image whose mounted archive agrees with the
 * initialized game directory, 0 when that identity is unavailable. */
int PcPort_BattleArchiveDisc(void);

