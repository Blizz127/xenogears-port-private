static const ScriptsFile *s_tauraLaboratoryScript;
static int s_restoreTauraLaboratory;
        s_tauraLaboratoryScript = NULL;
        if (s_waitExcavationLift || s_restoreForestParty || s_restoreTauraParty || s_restoreTauraLaboratory) {
            s_restoreTauraLaboratory = 0;
static int taura_meeting_parked(void);
                (taura_meeting_parked() && PcPort_TauraPartyCapture(checkpoint->party_followers)) ||
    if ((s_waitExcavationLift || s_restoreForestParty || s_restoreTauraParty || s_restoreTauraLaboratory ||
               !s_waitExcavationLift && !s_restoreForestParty && !s_restoreTauraParty && !s_restoreTauraLaboratory) {
#include "quick_checkpoint_taura_laboratory.inc"
    s_tauraLaboratoryScript = NULL;
    s_restoreTauraLaboratory = 0;
    restore_taura_laboratory();
    if ((s_restoreTauraParty || s_restoreTauraLaboratory) && (!checkpoint_is_safe() || g_FieldCameraMode != 0))
    } else if (s_restoreTauraLaboratory) {
        if (!s_loaded.has_party_followers || !PcPort_TauraPartyRestore(s_loaded.party_followers))
            PcPort_TauraPartyRegroup();
    s_restoreTauraLaboratory = 0;
