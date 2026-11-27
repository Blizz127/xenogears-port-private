#include "psx_memory.h"
/* The normal links place the battle table and controller in separate objects.
 * Guard both independently so a controller underwrite cannot masquerade as a
 * correct table reset. The non-PIE fixture also keeps game pointers representable. */
_Alignas(16) uint8_t name_tail_table[32 + 32 + 32];
_Alignas(16) uint8_t name_tail_sound[32 + 0x48 + 32];
        ".globl D_8005A3A0\n.set D_8005A3A0, name_tail_table+32\n"
        ".globl g_SoundVolumeController\n.set g_SoundVolumeController, name_tail_sound+32\n");
_Alignas(16) uint8_t g_PsxRam[PSX_RAM_SIZE];
    if (template_seed & 1u) {
        memcpy(PSX_ADDR(GLYPH_ADDRESS), native_glyphs, sizeof(native_glyphs));
        glyph_pointer = GLYPH_ADDRESS;
    }
    memset(name_tail_table, 0x63, sizeof(name_tail_table));
        require(name_tail_sound[i] == (i >= 32 && i < 40 ? 0 : 0x5c),
    for (unsigned i = 0; i < sizeof(name_tail_table); ++i)
        require(name_tail_table[i] == (i >= 32 && i < 64 ? 0 : 0x63),
                "native battle table reset extent", i);
    /* Guest layout has the 32-byte battle table directly before the controller. */
    for (unsigned i = 0; i < 32 + sizeof(name_tail_sound); ++i)
