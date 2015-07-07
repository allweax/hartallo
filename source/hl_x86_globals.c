#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_debug.h"

#if HL_HAVE_X86_INTRIN || HL_HAVE_X86_ASM

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_sizeof_pixel = sizeof(hl_pixel_t);

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_minus_ones[4] = { -1, -1, -1, -1 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_zeros[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4] = { 0x00000001, 0x00000001, 0x00000001, 0x00000001 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_fives[4] = { 0x00000005, 0x00000005, 0x00000005, 0x00000005 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixes[4] = { 0x00000005, 0x00000005, 0x00000005, 0x00000005 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4] = { 0x00000010, 0x00000010, 0x00000010, 0x00000010 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_twenties[4] = { 0x00000014, 0x00000014, 0x00000014, 0x00000014 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_thirty_twos[4] = { 0x00000020, 0x00000020, 0x00000020, 0x00000020 };
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4] = { 0x00000200, 0x00000200, 0x00000200, 0x00000200 };

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array16_fives[4] = { 0x05050505, 0x05050505, 0x05050505, 0x05050505 }; // epi8
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array16_twenties[4] = { 0x14141414, 0x14141414, 0x14141414, 0x14141414 }; // epi8

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4] = { 0x00010001, 0x00010001, 0x00010001, 0x00010001 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_twos[4] = { 0x00020002, 0x00020002, 0x00020002, 0x00020002 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_threes[4] = { 0x00030003, 0x00030003, 0x00030003, 0x00030003 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fours[4] = { 0x00040004, 0x00040004, 0x00040004, 0x00040004 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fives[4] = { 0x00050005, 0x00050005, 0x00050005, 0x00050005 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_twenties[4] = { 0x00140014, 0x00140014, 0x00140014, 0x00140014 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_sixteens[4] = { 0x00100010, 0x00100010, 0x00100010, 0x00100010 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_two_hundred_fiftyfive[4] = { 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff }; // epi16(255, 255, 255...)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_five_hundred_and_twelves[4] = { 0x02000200, 0x02000200, 0x02000200, 0x02000200 }; // epi16

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0[4] = { 0x03020100, 0x03020100, 0x03020100, 0x03020100 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_1[4] = { 0x03020100, 0x03020100, 0x03020100, 0x07060504 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_1_1[4] = { 0x03020100, 0x03020100, 0x07060504, 0x07060504 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_1_2[4] = { 0x03020100, 0x03020100, 0x07060504, 0x0b0a0908 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_1_1[4] = { 0x03020100, 0x07060504, 0x07060504, 0x07060504 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_2_2[4] = { 0x03020100, 0x07060504, 0x0b0a0908, 0x0b0a0908 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_1_0[4] = { 0x03020100, 0x07060504, 0x07060504, 0x03020100 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_1_1_1_0[4] = { 0x07060504, 0x07060504, 0x07060504, 0x03020100 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_2_2_3_3[4] = { 0x0b0a0908, 0x0b0a0908, 0x0f0e0d0c, 0x0f0e0d0c }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_2_2_3_Z[4] = { 0x0b0a0908, 0x0b0a0908, 0x0f0e0d0c, 0x80808080 }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_3_2_2_3[4] = { 0x0f0e0d0c, 0x0b0a0908, 0x0b0a0908, 0x0f0e0d0c }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_Z_Z_Z_3[4] = { 0x80808080, 0x80808080, 0x80808080, 0x0f0e0d0c }; // epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_Z_Z_1_Z[4] = { 0x80808080, 0x80808080, 0x07060504, 0x80808080 }; // epi32

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0[4] = { 0x01000100, 0x01000100, 0x01000100, 0x01000100 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0_1_1_1_1[4] = { 0x01000100, 0x01000100, 0x03020302, 0x03020302 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_1_1_2_2_3_3[4] = { 0x01000100, 0x03020302, 0x05040504, 0x07060706 }; // epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_1_1_1_1_0_0_0_0[4] = { 0x03020302, 0x03020302, 0x01000100, 0x01000100 }; // epi16

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to16_lo[4] = { 0x80018000, 0x80038002, 0x80058004, 0x80078006 }; // Use _mm_unpacklo_epi8(xmm0, 0) // low(epi8) -> epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to16_hi[4] = { 0x80098008, 0x800b800a, 0x800d800c, 0x800f800e }; // Use _mm_unpackhi_epi8(xmm0, 0) // high(epi8) -> epi16
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_0[4] = { 0x80808000, 0x80808001, 0x80808002, 0x80808003 }; // extract(epi8,byte[0,1,2,3]) -> epi32(dword-0)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_1[4] = { 0x80808004, 0x80808005, 0x80808006, 0x80808007 }; // extract(epi8,byte[4,5,6,7]) -> epi32 (dword-1)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_2[4] = { 0x80808008, 0x80808009, 0x8080800a, 0x8080800b }; // extract(epi8,byte[8,9,10,11]) -> epi32 (dword-2)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_3[4] = { 0x8080800c, 0x8080800d, 0x8080800e, 0x8080800f }; // extract(epi8,byte[12,13,14,15]) -> epi32 (dword-3)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_32to8_0[4] = { 0x0c080400, 0x80808080, 0x80808080, 0x80808080 }; // extract(epi32, dword[0,1,2,3]) -> epi8(byte[0,1,2,3])
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_16to32_lo[4] = { 0x80800100, 0x80800302, 0x80800504, 0x80800706 }; // Use _mm_unpacklo_epi16(xmm0, 0) instead // low(epi16) -> epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_16to32_hi[4] = { 0x80800908, 0x80800b0a, 0x80800d0c, 0x80800f0e }; // Use _mm_unpackhi_epi16(xmm0, 0) instead // high(epi16) -> epi32
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_16to8_lo[4] = { 0x06040200, 0x0e0c0a08, 0x80808080, 0x80808080 }; // Use _mm_packus_epi16(xmm0, xmm0) instead // extract(epi16, word[4,5,6,7], word[0,1,2,3]) -> epi8(0, 0, byte[0,1,2,3], byte[0,1,2,3])

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_move_mask_lo64[4] = { 0x00000000, 0x00000000, 0x80808080, 0x80808080 }; // low64bits(a) -> low64bits(b)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_move_mask_hi64[4] = { 0x80808080, 0x80808080, 0x00000000, 0x00000000 }; // hi64bits(a) -> hi64bits(b)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_move_mask_lo32[4] = { 0x00000000, 0x00000000, 0x00000000, 0x80808080 }; // low32bits(a) -> low32bits(b)
HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_move_mask_hi32[4] = { 0x80808080, 0x00000000, 0x00000000, 0x00000000 }; // hi32bits(a) -> hi32bits(b)

HARTALLO_EXPORT HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sign_P_M_M_P[4] = { +1, -1, -1, +1 };

HL_ERROR_T hl_x86_globals_init()
{
    static hl_bool_t __g_initialized = HL_FALSE;

    HL_DEBUG_INFO("Initializing X86 global data");

    if (__g_initialized) {
        return HL_ERROR_SUCCESS;
    }

    /* Initialize Global data used for Chroma interpolation */


    return HL_ERROR_SUCCESS;
}

#endif /* HL_HAVE_X86_INTRIN || HL_HAVE_X86_ASM */