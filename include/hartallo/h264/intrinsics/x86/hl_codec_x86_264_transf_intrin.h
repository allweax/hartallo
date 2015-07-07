#ifndef _HARTALLO_CODEC_X86_264_TRANSF_INTRIN_H_
#define _HARTALLO_CODEC_X86_264_TRANSF_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_memory.h"
#include "hartallo/intrinsics/x86/hl_math_x86_intrin.h"
#include "hartallo/intrinsics/x86/hl_utils_x86_intrin.h"

#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_mb.h"

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

HL_ALWAYS_INLINE static void hl_codec_x86_264_transf_inverse_residual4x4_intrin_sse2(int32_t bitDepth, HL_ALIGNED(16) int32_t d[4][4], HL_ALIGNED(16) int32_t r[4][4])
{
    __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_thirty_twos[4];

    (bitDepth);

    _mm_store_si128(&xmm0, _mm_load_si128((__m128i*)d[0]));
    _mm_store_si128(&xmm1, _mm_load_si128((__m128i*)d[1]));
    _mm_store_si128(&xmm2, _mm_load_si128((__m128i*)d[2]));
    _mm_store_si128(&xmm3, _mm_load_si128((__m128i*)d[3]));

    // e
    HL_MATH_TRANSPOSE4X1_EPI32_SSE2(xmm0, xmm1, xmm2, xmm3);
    // f
    _mm_store_si128(&xmm4, _mm_add_epi32(xmm0, xmm2));
    _mm_store_si128(&xmm5, _mm_sub_epi32(xmm0, xmm2));
    _mm_store_si128(&xmm6, _mm_sub_epi32(_mm_srai_epi32(xmm1, 1), xmm3));
    _mm_store_si128(&xmm7, _mm_add_epi32(xmm1, _mm_srai_epi32(xmm3, 1)));
    _mm_store_si128(&xmm0, _mm_add_epi32(xmm4, xmm7));
    _mm_store_si128(&xmm1, _mm_add_epi32(xmm5, xmm6));
    _mm_store_si128(&xmm2, _mm_sub_epi32(xmm5, xmm6));
    _mm_store_si128(&xmm3, _mm_sub_epi32(xmm4, xmm7));
    HL_MATH_TRANSPOSE4X1_EPI32_SSE2(xmm0, xmm1, xmm2, xmm3);
    // g
    _mm_store_si128((__m128i*)&xmm4, _mm_add_epi32(xmm0, xmm2));
    _mm_store_si128((__m128i*)&xmm5, _mm_sub_epi32(xmm0, xmm2));
    _mm_store_si128((__m128i*)&xmm6, _mm_sub_epi32(_mm_srai_epi32(xmm1, 1), xmm3));
    _mm_store_si128((__m128i*)&xmm7, _mm_add_epi32(xmm1, _mm_srai_epi32(xmm3, 1)));

    // h
    _mm_store_si128((__m128i*)&xmm0, _mm_add_epi32(xmm4, xmm7));
    _mm_store_si128((__m128i*)&xmm1, _mm_add_epi32(xmm5, xmm6));
    _mm_store_si128((__m128i*)&xmm2, _mm_sub_epi32(xmm5, xmm6));
    _mm_store_si128((__m128i*)&xmm3, _mm_sub_epi32(xmm4, xmm7));

    // r
    _mm_store_si128(&xmm4, _mm_load_si128((__m128i*)__x86_globals_array4_thirty_twos));
    _mm_store_si128((__m128i*)r[0], _mm_srai_epi32(_mm_add_epi32(xmm0, xmm4), 6));
    _mm_store_si128((__m128i*)r[1], _mm_srai_epi32(_mm_add_epi32(xmm1, xmm4), 6));
    _mm_store_si128((__m128i*)r[2], _mm_srai_epi32(_mm_add_epi32(xmm2, xmm4), 6));
    _mm_store_si128((__m128i*)r[3], _mm_srai_epi32(_mm_add_epi32(xmm3, xmm4), 6));
}

// 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
// /!\ Do not forget to change SSSE3 version if this one is changed
// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_codec_x86_264_transf_scale_luma_dc_coeff_intra16x16_intrin_sse41(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    int32_t qP,
    int32_t BitDepth,
    HL_IN_ALIGNED(16) const int32_t c[4][4],
    HL_OUT_ALIGNED(16) int32_t dcY[4][4])
{
    if (p_mb->TransformBypassModeFlag == 1) {
        _mm_store_si128((__m128i*)dcY[0], _mm_load_si128((__m128i*)c[0]));
        _mm_store_si128((__m128i*)dcY[1], _mm_load_si128((__m128i*)c[1]));
        _mm_store_si128((__m128i*)dcY[2], _mm_load_si128((__m128i*)c[2]));
        _mm_store_si128((__m128i*)dcY[3], _mm_load_si128((__m128i*)c[3]));
    }
    else {
#if 0
        static HL_ALIGN(HL_ALIGN_V) const int32_t m[4][4] = { {1, 1, 1, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -1, 1, -1} };
#endif
        static const int32_t kIsInterFlag0 = 0;
        static const int32_t kiYCbCr0 = 0; // luma=0, Cb=1, Cr=2
        __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

        // TODO: This is hadamard transform. Use same code in other places.
        // (8-324) f = MUL(m, c, m) = MUL(d, m) with d = MUL(m, c)

        _mm_store_si128((__m128i*)&xmm0, _mm_load_si128((__m128i*)c[0]));
        _mm_store_si128((__m128i*)&xmm1, _mm_load_si128((__m128i*)c[1]));
        _mm_store_si128((__m128i*)&xmm2, _mm_load_si128((__m128i*)c[2]));
        _mm_store_si128((__m128i*)&xmm3, _mm_load_si128((__m128i*)c[3]));

        // d = MUL(m, c)
        _mm_store_si128((__m128i*)&xmm4, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(xmm0, xmm1), xmm2), xmm3));
        _mm_store_si128((__m128i*)&xmm5, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(xmm0, xmm1), xmm2), xmm3));
        _mm_store_si128((__m128i*)&xmm6, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(xmm0, xmm1), xmm2), xmm3));
        _mm_store_si128((__m128i*)&xmm7, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(xmm0, xmm1), xmm2), xmm3));

        // f = MUL(d, m)
        HL_MATH_TRANSPOSE4X1_EPI32_SSE2(xmm4, xmm5, xmm6, xmm7);
        _mm_store_si128((__m128i*)&xmm0, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(xmm4, xmm5), xmm6), xmm7));
        _mm_store_si128((__m128i*)&xmm1, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(xmm4, xmm5), xmm6), xmm7));
        _mm_store_si128((__m128i*)&xmm2, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(xmm4, xmm5), xmm6), xmm7));
        _mm_store_si128((__m128i*)&xmm3, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(xmm4, xmm5), xmm6), xmm7));
        HL_MATH_TRANSPOSE4X1_EPI32_SSE2(xmm0, xmm1, xmm2, xmm3);

        if (p_mb->QPy >= 36) {
            const int32_t scale = p_codec->pps.pc_active->LevelScale4x4[kIsInterFlag0][kiYCbCr0][p_mb->QPy%6][0][0];
            const int32_t qP_shift = qP/6-6;
            const __m128i scale4 = _mm_set1_epi32(scale);
            _mm_store_si128((__m128i*)dcY[0], _mm_slli_epi32(_mm_mullo_epi32(xmm0, scale4), qP_shift));
            _mm_store_si128((__m128i*)dcY[1], _mm_slli_epi32(_mm_mullo_epi32(xmm1, scale4), qP_shift));
            _mm_store_si128((__m128i*)dcY[2], _mm_slli_epi32(_mm_mullo_epi32(xmm2, scale4), qP_shift));
            _mm_store_si128((__m128i*)dcY[3], _mm_slli_epi32(_mm_mullo_epi32(xmm3, scale4), qP_shift));
        }
        else {
            int32_t scale = p_codec->pps.pc_active->LevelScale4x4[kIsInterFlag0][kiYCbCr0][p_mb->QPy%6][0][0];
            const int32_t qP_plus = (1 << (5-qP/6));
            const int32_t qP_shift = (6-qP/6);
            const __m128i qP_plus4 = _mm_set1_epi32(qP_plus);
            const __m128i scale4 = _mm_set1_epi32(scale);
            _mm_store_si128((__m128i*)dcY[0], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(xmm0, scale4), qP_plus4), qP_shift));
            _mm_store_si128((__m128i*)dcY[1], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(xmm1, scale4), qP_plus4), qP_shift));
            _mm_store_si128((__m128i*)dcY[2], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(xmm2, scale4), qP_plus4), qP_shift));
            _mm_store_si128((__m128i*)dcY[3], _mm_srai_epi32(_mm_add_epi32(_mm_mullo_epi32(xmm3, scale4), qP_plus4), qP_shift));
        }
    }
}

// 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
// /!\ Do not forget to change SSE41 version if this one is changed
// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_codec_x86_264_transf_scale_luma_dc_coeff_intra16x16_intrin_sse2(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    int32_t qP,
    int32_t BitDepth,
    HL_IN_ALIGNED(16) const int32_t c[4][4],
    HL_OUT_ALIGNED(16) int32_t dcY[4][4])
{
    if (p_mb->TransformBypassModeFlag == 1) {
        _mm_store_si128((__m128i*)dcY[0], _mm_load_si128((__m128i*)c[0]));
        _mm_store_si128((__m128i*)dcY[1], _mm_load_si128((__m128i*)c[1]));
        _mm_store_si128((__m128i*)dcY[2], _mm_load_si128((__m128i*)c[2]));
        _mm_store_si128((__m128i*)dcY[3], _mm_load_si128((__m128i*)c[3]));
    }
    else {
#if 0
        static HL_ALIGN(HL_ALIGN_V) const int32_t m[4][4] = { {1, 1, 1, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -1, 1, -1} };
#endif
        static const int32_t kIsInterFlag0 = 0;
        static const int32_t kiYCbCr0 = 0; // luma=0, Cb=1, Cr=2
        __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;

        // TODO: This is hadamard transform. Use same code in other places.
        // (8-324) f = MUL(m, c, m) = MUL(d, m) with d = MUL(m, c)

        _mm_store_si128((__m128i*)&xmm0, _mm_load_si128((__m128i*)c[0]));
        _mm_store_si128((__m128i*)&xmm1, _mm_load_si128((__m128i*)c[1]));
        _mm_store_si128((__m128i*)&xmm2, _mm_load_si128((__m128i*)c[2]));
        _mm_store_si128((__m128i*)&xmm3, _mm_load_si128((__m128i*)c[3]));

        // d = MUL(m, c)
        _mm_store_si128((__m128i*)&xmm4, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(xmm0, xmm1), xmm2), xmm3));
        _mm_store_si128((__m128i*)&xmm5, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(xmm0, xmm1), xmm2), xmm3));
        _mm_store_si128((__m128i*)&xmm6, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(xmm0, xmm1), xmm2), xmm3));
        _mm_store_si128((__m128i*)&xmm7, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(xmm0, xmm1), xmm2), xmm3));

        // f = MUL(d, m)
        HL_MATH_TRANSPOSE4X1_EPI32_SSE2(xmm4, xmm5, xmm6, xmm7);
        _mm_store_si128((__m128i*)&xmm0, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(xmm4, xmm5), xmm6), xmm7));
        _mm_store_si128((__m128i*)&xmm1, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(xmm4, xmm5), xmm6), xmm7));
        _mm_store_si128((__m128i*)&xmm2, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(xmm4, xmm5), xmm6), xmm7));
        _mm_store_si128((__m128i*)&xmm3, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(xmm4, xmm5), xmm6), xmm7));
        HL_MATH_TRANSPOSE4X1_EPI32_SSE2(xmm0, xmm1, xmm2, xmm3);

        if (p_mb->QPy >= 36) {
            const int32_t scale = p_codec->pps.pc_active->LevelScale4x4[kIsInterFlag0][kiYCbCr0][p_mb->QPy%6][0][0];
            const int32_t qP_shift = qP/6-6;
            const __m128i scale4 = _mm_set1_epi32(scale);
            _mm_store_si128((__m128i*)dcY[0], _mm_slli_epi32(hl_mm_mullo_epi32_sse2(xmm0, scale4), qP_shift));
            _mm_store_si128((__m128i*)dcY[1], _mm_slli_epi32(hl_mm_mullo_epi32_sse2(xmm1, scale4), qP_shift));
            _mm_store_si128((__m128i*)dcY[2], _mm_slli_epi32(hl_mm_mullo_epi32_sse2(xmm2, scale4), qP_shift));
            _mm_store_si128((__m128i*)dcY[3], _mm_slli_epi32(hl_mm_mullo_epi32_sse2(xmm3, scale4), qP_shift));
        }
        else {
            int32_t scale = p_codec->pps.pc_active->LevelScale4x4[kIsInterFlag0][kiYCbCr0][p_mb->QPy%6][0][0];
            const int32_t qP_plus = (1 << (5-qP/6));
            const int32_t qP_shift = (6-qP/6);
            const __m128i qP_plus4 = _mm_set1_epi32(qP_plus);
            const __m128i scale4 = _mm_set1_epi32(scale);
            _mm_store_si128((__m128i*)dcY[0], _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(xmm0, scale4), qP_plus4), qP_shift));
            _mm_store_si128((__m128i*)dcY[1], _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(xmm1, scale4), qP_plus4), qP_shift));
            _mm_store_si128((__m128i*)dcY[2], _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(xmm2, scale4), qP_plus4), qP_shift));
            _mm_store_si128((__m128i*)dcY[3], _mm_srai_epi32(_mm_add_epi32(hl_mm_mullo_epi32_sse2(xmm3, scale4), qP_plus4), qP_shift));
        }
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_transf_frw_residual4x4_intrin_sse2(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16)int32_t out4x4[4][4])
{
    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    // tmp4x4 = MULT(Cf, in4x4)
    _mm_store_si128(&r0, _mm_load_si128((const __m128i*)in4x4[0]));
    _mm_store_si128(&r1, _mm_load_si128((const __m128i*)in4x4[1]));
    _mm_store_si128(&r2, _mm_load_si128((const __m128i*)in4x4[2]));
    _mm_store_si128(&r3, _mm_load_si128((const __m128i*)in4x4[3]));

    _mm_store_si128(&r4, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(r0, r1), r2), r3));
    _mm_store_si128(&r5, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(_mm_slli_epi32(r0, 1), r1), r2), _mm_slli_epi32(r3, 1)));
    _mm_store_si128(&r6, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(r0, r1), r2), r3));
    _mm_store_si128(&r7, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(r0, _mm_slli_epi32(r1, 1)), _mm_slli_epi32(r2, 1)), r3));

    // out4x4 = MULT(tmp4x4, Cf_T)
    HL_MATH_TRANSPOSE4X1_EPI32_SSE2(r4, r5, r6, r7);
    _mm_store_si128(&r0, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(r4, r5), r6), r7));
    _mm_store_si128(&r1, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(_mm_slli_epi32(r4, 1), r5), r6), _mm_slli_epi32(r7, 1)));
    _mm_store_si128(&r2, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(r4, r5), r6), r7));
    _mm_store_si128(&r3, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(r4, _mm_slli_epi32(r5, 1)), _mm_slli_epi32(r6, 1)), r7));
    HL_MATH_TRANSPOSE4X1_EPI32_SSE2(r0, r1, r2, r3);

    _mm_store_si128((__m128i*)&out4x4[0], r0);
    _mm_store_si128((__m128i*)&out4x4[1], r1);
    _mm_store_si128((__m128i*)&out4x4[2], r2);
    _mm_store_si128((__m128i*)&out4x4[3], r3);
}

HL_ALWAYS_INLINE static void hl_codec_264_transf_frw_hadamard4x4_dc_luma_intrin_sse2(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    // tmp4x4 = MULT(Cf, in4x4)
    _mm_store_si128(&r0, _mm_load_si128((const __m128i*)in4x4[0]));
    _mm_store_si128(&r1, _mm_load_si128((const __m128i*)in4x4[1]));
    _mm_store_si128(&r2, _mm_load_si128((const __m128i*)in4x4[2]));
    _mm_store_si128(&r3, _mm_load_si128((const __m128i*)in4x4[3]));
    _mm_store_si128(&r4, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(r0, r1), r2), r3));
    _mm_store_si128(&r5, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(r0, r1), r2), r3));
    _mm_store_si128(&r6, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(r0, r1), r2), r3));
    _mm_store_si128(&r7, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(r0, r1), r2), r3));

    // out4x4 = MULT(tmp4x4, Cf_T)
    HL_MATH_TRANSPOSE4X1_EPI32_SSE2(r4, r5, r6, r7);
    _mm_store_si128(&r0, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(r4, r5), r6), r7));
    _mm_store_si128(&r1, _mm_sub_epi32(_mm_sub_epi32(_mm_add_epi32(r4, r5), r6), r7));
    _mm_store_si128(&r2, _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(r4, r5), r6), r7));
    _mm_store_si128(&r3, _mm_sub_epi32(_mm_add_epi32(_mm_sub_epi32(r4, r5), r6), r7));
    HL_MATH_TRANSPOSE4X1_EPI32_SSE2(r0, r1, r2, r3);
    // out4x4[i][j] >>= 1;
    _mm_store_si128((__m128i*)&out4x4[0], _mm_srai_epi32(r0, 1));
    _mm_store_si128((__m128i*)&out4x4[1], _mm_srai_epi32(r1, 1));
    _mm_store_si128((__m128i*)&out4x4[2], _mm_srai_epi32(r2, 1));
    _mm_store_si128((__m128i*)&out4x4[3], _mm_srai_epi32(r3, 1));
}

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_CODEC_X86_264_TRANSF_INTRIN_H_ */
