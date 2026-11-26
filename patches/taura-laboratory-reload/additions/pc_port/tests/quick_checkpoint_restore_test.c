static int s_restoreTauraLaboratory;
int PcPort_TauraPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES])
{
    (void)in;
    abort(); /* Ordinary contact cases never enter Taura's laboratory. */
}

int PcPort_TauraPartyRegroup(void) { abort(); }

