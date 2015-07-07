#ifndef _HARTALLO_CODEC_X86_264_QUANT_INTRIN_H_
#define _HARTALLO_CODEC_X86_264_QUANT_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_debug.h"

#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_layer.h"

#include "hartallo/intrinsics/x86/hl_utils_x86_intrin.h"
#include "hartallo/intrinsics/x86/hl_math_x86_intrin.h"

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

// TODO: no ASM version
// TODO: no SSE2/3 versions
HL_ALWAYS_INLINE static void hl_codec_x86_264_quant_scale_residual4x4_intin_sse41(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s* pc_mb,
    int32_t bitDepth,
    int32_t qP,
    const int32_t c[4][4],
    hl_bool_t luma,
    hl_bool_t Intra16x16,
    int32_t CbCrIdx,
    /* out */int32_t d[4][4])
{
    int32_t qP_div6 = qP/6;
    int32_t qP_mod6 = qP % 6;
    int32_t mbIsInterFlag = HL_CODEC_264_MB_TYPE_IS_INTER(pc_mb) ? 1 : 0;
    int32_t iYCbCr = pc_codec->sps.pc_active->separate_colour_plane_flag ? pc_codec->layers.pc_active->pc_slice_hdr->colour_plane_id : (luma ? 0 : CbCrIdx + 1); // Luma=0,Cb=1,Cr=2

    (bitDepth);

    if(qP >= 24) {
        const int32_t qP_shift = (qP_div6-4);
        _mm_store_si128((__m128i*)d[0], _mm_slli_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[0]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][0])), qP_shift));
        _mm_store_si128((__m128i*)d[1], _mm_slli_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[1]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][1])), qP_shift));
        _mm_store_si128((__m128i*)d[2], _mm_slli_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[2]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][2])), qP_shift));
        _mm_store_si128((__m128i*)d[3], _mm_slli_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[3]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][3])), qP_shift));
    }
    else {
        const int32_t qP_offset = (1 << (3 - qP_div6)); // 2^(3-qp/6)
        const int32_t qP_shift = (4 - qP_div6); //(4 - qP/6)
        const __m128i qP_offset128 = _MM_SET1_EPI32_SSE41(qP_offset);

        _mm_store_si128((__m128i*)d[0], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[0]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][0])), qP_offset128), qP_shift));
        _mm_store_si128((__m128i*)d[1], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[1]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][1])), qP_offset128), qP_shift));
        _mm_store_si128((__m128i*)d[2], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[2]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][2])), qP_offset128), qP_shift));
        _mm_store_si128((__m128i*)d[3], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_load_si128((__m128i*)c[3]), _mm_load_si128((__m128i*)pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][3])), qP_offset128), qP_shift));
    }

    if ((luma && Intra16x16) || !luma) {
        d[0][0] = c[0][0];
    }
}

// "_mm_abs_epi32" --> SSSE3
// "_mm_mullo_epi32" -->SSE4.1
HL_ALWAYS_INLINE static void hl_codec_264_quant_frw4x4_scale_ac_intin_sse41(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
	const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index = QP%6;
    int32_t i;

    __m128i xmm_quant_mf, xmm_f, xmm_in, xmm_z;
    __m128i xmm_zeros, xmm_ones, xmm_sign;
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    _mm_store_si128(&xmm_f, _mm_set1_epi32(f));
    _mm_store_si128(&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array4_ones));
    _mm_store_si128(&xmm_zeros, _mm_setzero_si128());

    for (i = 0; i < 4; ++i) {
        _mm_store_si128(&xmm_quant_mf, _mm_load_si128((__m128i*)HL_CODEC_264_QUANT_MF[mf_index][i]));
        _mm_store_si128(&xmm_in, _mm_load_si128((__m128i*)in4x4[i]));
        // SIGN(IN) -> ((IN)>=0 ? 1 : -1)
        _mm_store_si128(&xmm_sign, _mm_sub_epi32(_mm_and_si128(_mm_cmpgt_epi32(xmm_in, xmm_zeros), xmm_ones), _mm_and_si128(_mm_cmplt_epi32(xmm_in, xmm_zeros), xmm_ones)));
        // Z = (ABS(IN[i][j]) * HL_CODEC_264_QUANT_MF[mf_index][i][j] + f) >> qBits;
        _mm_store_si128(&xmm_z, _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_abs_epi32(xmm_in), xmm_quant_mf), xmm_f), qBits));
        // OUT = Z * SIGN(IN)
        _mm_store_si128((__m128i*)out4x4[i], _mm_mullo_epi32(xmm_sign, xmm_z));
    }
}

// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_codec_264_quant_frw4x4_scale_ac_intin_sse2(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index = QP%6;
    register int32_t i;

    __m128i xmm_quant_mf, xmm_f, xmm_in, xmm_z;
    __m128i xmm_zeros, xmm_ones, xmm_sign;
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    _mm_store_si128(&xmm_f, _mm_set1_epi32(f));
    _mm_store_si128(&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array4_ones));
    _mm_store_si128(&xmm_zeros, _mm_setzero_si128());

    for (i = 0; i < 4; ++i) {
        _mm_store_si128(&xmm_quant_mf, _mm_load_si128((__m128i*)HL_CODEC_264_QUANT_MF[mf_index][i]));
        _mm_store_si128(&xmm_in, _mm_load_si128((__m128i*)in4x4[i]));
        // SIGN(IN) -> ((IN)>=0 ? 1 : -1)
        _mm_store_si128(&xmm_sign, _mm_sub_epi32(_mm_and_si128(_mm_cmpgt_epi32(xmm_in, xmm_zeros), xmm_ones), _mm_and_si128(_mm_cmplt_epi32(xmm_in, xmm_zeros), xmm_ones)));
        // IN = ABS(IN) = (SIGN(IN) * IN)
        _mm_store_si128(&xmm_in, hl_mm_mullo_epi32_sse2(xmm_sign, xmm_in));
        // Z = (ABS(IN[i][j]) * HL_CODEC_264_QUANT_MF[mf_index][i][j] + f) >> qBits;
        _mm_store_si128(&xmm_z, _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(xmm_in, xmm_quant_mf), xmm_f), qBits));
        // OUT = Z * SIGN(IN)
        _mm_store_si128((__m128i*)out4x4[i], hl_mm_mullo_epi32_sse2(xmm_sign, xmm_z));
    }
}


// TODO: add ASM version
// "_mm_abs_epi32" --> SSSE3
// "_mm_mullo_epi32" -->SSE4.1
HL_ALWAYS_INLINE static void hl_codec_264_quant_frw4x4_scale_dc_luma_intin_sse41(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t qBits_plus1 = qBits+1;
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index = QP%6;
    const int32_t mf_00 = HL_CODEC_264_QUANT_MF[mf_index][0][0];
    const int32_t _2f = f<<1;
    int32_t i;

    __m128i xmm_mf00, xmm_2f, xmm_dcC, xmm_z;
    __m128i xmm_zeros, xmm_ones, xmm_sign;
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    _mm_store_si128(&xmm_2f, _mm_set1_epi32(f << 1));
    _mm_store_si128(&xmm_mf00, _mm_set1_epi32(mf_00));
    _mm_store_si128(&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array4_ones));
    _mm_store_si128(&xmm_zeros, _mm_setzero_si128());

    for (i = 0; i < 4; ++i) {
        _mm_store_si128(&xmm_dcC, _mm_load_si128((__m128i*)dcC[i]));
        // SIGN(dcC) -> ((dcC)>=0 ? 1 : -1)
        _mm_store_si128(&xmm_sign, _mm_sub_epi32(_mm_and_si128(_mm_cmpgt_epi32(xmm_dcC, xmm_zeros), xmm_ones), _mm_and_si128(_mm_cmplt_epi32(xmm_dcC, xmm_zeros), xmm_ones)));
        // Z = (HL_MATH_ABS_INT32(dcC[i][j]) * mf_00 + _2f) >>qBits_plus1;
        _mm_store_si128(&xmm_z, _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_abs_epi32(xmm_dcC), xmm_mf00), xmm_2f), qBits_plus1));
        // OUT = Z * SIGN(dcC)
        _mm_store_si128((__m128i*)out4x4[i], _mm_mullo_epi32(xmm_sign, xmm_z));
    }
}

// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_codec_264_quant_frw4x4_scale_dc_luma_intin_sse2(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t qBits_plus1 = qBits+1;
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index = QP%6;
    const int32_t mf_00 = HL_CODEC_264_QUANT_MF[mf_index][0][0];
    const int32_t _2f = f<<1;
    int32_t i;

    __m128i xmm_mf00, xmm_2f, xmm_dcC, xmm_z;
    __m128i xmm_zeros, xmm_ones, xmm_sign;
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    _mm_store_si128(&xmm_2f, _mm_set1_epi32(f << 1));
    _mm_store_si128(&xmm_mf00, _mm_set1_epi32(mf_00));
    _mm_store_si128(&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array4_ones));
    _mm_store_si128(&xmm_zeros, _mm_setzero_si128());

    for (i = 0; i < 4; ++i) {
        _mm_store_si128(&xmm_dcC, _mm_load_si128((__m128i*)dcC[i]));
        // SIGN(dcC) -> ((dcC)>=0 ? 1 : -1)
        _mm_store_si128(&xmm_sign, _mm_sub_epi32(_mm_and_si128(_mm_cmpgt_epi32(xmm_dcC, xmm_zeros), xmm_ones), _mm_and_si128(_mm_cmplt_epi32(xmm_dcC, xmm_zeros), xmm_ones)));
        // dcC = ABS(dcC) = (SIGN(dcC) * dcC)
        _mm_store_si128(&xmm_dcC, hl_mm_mullo_epi32_sse2(xmm_sign, xmm_dcC));
        // Z = (HL_MATH_ABS_INT32(dcC[i][j]) * mf_00 + _2f) >>qBits_plus1;
        _mm_store_si128(&xmm_z, _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(_mm_abs_epi32(xmm_dcC), xmm_mf00), xmm_2f), qBits_plus1));
        // OUT = Z * SIGN(dcC)
        _mm_store_si128((__m128i*)out4x4[i], hl_mm_mullo_epi32_sse2(xmm_sign, xmm_z));
    }
}

// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_codec_264_quant_frw2x2_scale_dc_chroma_intin_sse41(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[2][2], HL_ALIGNED(16) int32_t out2x2[2][2])
{
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t qBits_plus1=qBits+1;
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index=QP%6;
    const int32_t mf_00 = HL_CODEC_264_QUANT_MF[mf_index][0][0];
    const int32_t _2f=f<<1;

    __m128i xmm_mf00, xmm_2f, xmm_dcC, xmm_z;
    __m128i xmm_zeros, xmm_ones, xmm_sign;
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    _mm_store_si128(&xmm_2f, _mm_set1_epi32(f << 1));
    _mm_store_si128(&xmm_mf00, _mm_set1_epi32(mf_00));
    _mm_store_si128(&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array4_ones));
    _mm_store_si128(&xmm_zeros, _mm_setzero_si128());

    _mm_store_si128(&xmm_dcC, _mm_load_si128((__m128i*)dcC));
    // SIGN(dcC) -> ((dcC)>=0 ? 1 : -1)
    _mm_store_si128(&xmm_sign, _mm_sub_epi32(_mm_and_si128(_mm_cmpgt_epi32(xmm_dcC, xmm_zeros), xmm_ones), _mm_and_si128(_mm_cmplt_epi32(xmm_dcC, xmm_zeros), xmm_ones)));
    // Z = (HL_MATH_ABS_INT32(dcC[i][j]) * mf_00 + _2f) >>qBits_plus1;
    _mm_store_si128(&xmm_z, _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(_mm_abs_epi32(xmm_dcC), xmm_mf00), xmm_2f), qBits_plus1));
    // OUT = Z * SIGN(dcC)
    _mm_store_si128((__m128i*)out2x2, _mm_mullo_epi32(xmm_sign, xmm_z));
}

// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_codec_264_quant_frw2x2_scale_dc_chroma_intin_sse2(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[2][2], HL_ALIGNED(16) int32_t out2x2[2][2])
{
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t qBits_plus1=qBits+1;
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index=QP%6;
    const int32_t mf_00 = HL_CODEC_264_QUANT_MF[mf_index][0][0];
    const int32_t _2f=f<<1;

    __m128i xmm_mf00, xmm_2f, xmm_dcC, xmm_z;
    __m128i xmm_zeros, xmm_ones, xmm_sign;
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    _mm_store_si128(&xmm_2f, _mm_set1_epi32(f << 1));
    _mm_store_si128(&xmm_mf00, _mm_set1_epi32(mf_00));
    _mm_store_si128(&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array4_ones));
    _mm_store_si128(&xmm_zeros, _mm_setzero_si128());

    _mm_store_si128(&xmm_dcC, _mm_load_si128((__m128i*)dcC));
    // SIGN(dcC) -> ((dcC)>=0 ? 1 : -1)
    _mm_store_si128(&xmm_sign, _mm_sub_epi32(_mm_and_si128(_mm_cmpgt_epi32(xmm_dcC, xmm_zeros), xmm_ones), _mm_and_si128(_mm_cmplt_epi32(xmm_dcC, xmm_zeros), xmm_ones)));
    // dcC = ABS(dcC) = (SIGN(dcC) * dcC)
    _mm_store_si128(&xmm_dcC, hl_mm_mullo_epi32_sse2(xmm_sign, xmm_dcC));
    // Z = (HL_MATH_ABS_INT32(dcC[i][j]) * mf_00 + _2f) >>qBits_plus1;
    _mm_store_si128(&xmm_z, _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(_mm_abs_epi32(xmm_dcC), xmm_mf00), xmm_2f), qBits_plus1));
    // OUT = Z * SIGN(dcC)
    _mm_store_si128((__m128i*)out2x2, hl_mm_mullo_epi32_sse2(xmm_sign, xmm_z));
}

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_CODEC_X86_264_QUANT_INTRIN_H_ */
