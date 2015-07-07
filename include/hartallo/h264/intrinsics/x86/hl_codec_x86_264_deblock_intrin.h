#ifndef _HARTALLO_CODEC_X86_264_DEBLOCK_INTRIN_H_
#define _HARTALLO_CODEC_X86_264_DEBLOCK_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_memory.h"
#include "hartallo/intrinsics/x86/hl_math_x86_intrin.h"
#include "hartallo/intrinsics/x86/hl_utils_x86_intrin.h"

/* Headers for chroma interpolation */
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_sps.h"

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

#if defined(_MSC_VER) || defined(__GNUC__)
#define _sse_setzero_ps _mm_setzero_ps
#else
static HL_ALWAYS_INLINE __m128 _sse_setzero_ps(void)
{
    __m128 xmm;
    return _mm_xor_ps(xmm, xmm);
}
#endif

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_intrin_sse2(
    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    _mm_store_si128((__m128i *)p[3], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)p[2], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)p[1], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)p[0], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[0], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[1], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[2], _mm_loadu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[3], _mm_loadu_si128((__m128i*)pc_luma_samples));
}

// TODO: add ASM version
static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_intrin_sse3(
    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    _mm_store_si128((__m128i *)p[3], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)p[2], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)p[1], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)p[0], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[0], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[1], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[2], _mm_lddqu_si128((__m128i*)pc_luma_samples));
    pc_luma_samples += u_luma_stride;
    _mm_store_si128((__m128i *)q[3], _mm_lddqu_si128((__m128i*)pc_luma_samples));
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_intrin_sse2(
    const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    _mm_storel_epi64((__m128i *)&p[3][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[2][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[1][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[0][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[0][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[1][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[2][0], _mm_loadu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[3][0], _mm_loadu_si128((__m128i*)pc_cb_samples));

    _mm_storel_epi64((__m128i *)&p[3][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[2][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[1][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[0][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[0][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[1][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[2][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[3][8], _mm_loadu_si128((__m128i*)pc_cr_samples));
}

// TODO: add ASM version
static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_intrin_sse3(
    const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    _mm_storel_epi64((__m128i *)&p[3][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[2][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[1][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[0][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[0][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[1][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[2][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));
    pc_cb_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[3][0], _mm_lddqu_si128((__m128i*)pc_cb_samples));

    _mm_storel_epi64((__m128i *)&p[3][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[2][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[1][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&p[0][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[0][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[1][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[2][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
    pc_cr_samples += u_chroma_stride;
    _mm_storel_epi64((__m128i *)&q[3][8], _mm_lddqu_si128((__m128i*)pc_cr_samples));
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_intrin_sse2(
    uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    HL_ALIGNED(16) const uint8_t pf[3/*pf0,pf1,pf2*/][16], HL_ALIGNED(16) const uint8_t qf[3/*qf0,qf1,qf2*/][16])
{
    pc_luma_samples += u_luma_stride; // skip pf/qf[3] and take p/qf[0,2,2]
    _mm_storeu_si128((__m128i *)pc_luma_samples, _mm_load_si128((__m128i*)pf[2]));
    pc_luma_samples += u_luma_stride;
    _mm_storeu_si128((__m128i *)pc_luma_samples, _mm_load_si128((__m128i*)pf[1]));
    pc_luma_samples += u_luma_stride;
    _mm_storeu_si128((__m128i *)pc_luma_samples, _mm_load_si128((__m128i*)pf[0]));
    pc_luma_samples += u_luma_stride;
    _mm_storeu_si128((__m128i *)pc_luma_samples, _mm_load_si128((__m128i*)qf[0]));
    pc_luma_samples += u_luma_stride;
    _mm_storeu_si128((__m128i *)pc_luma_samples, _mm_load_si128((__m128i*)qf[1]));
    pc_luma_samples += u_luma_stride;
    _mm_storeu_si128((__m128i *)pc_luma_samples, _mm_load_si128((__m128i*)qf[2]));
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_intrin_sse2(
    uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    HL_ALIGNED(16) const uint8_t pf[3/*pf0,pf1,pf2*/][16], HL_ALIGNED(16) const uint8_t qf[3/*qf0,qf1,qf2*/][16])
{
    __m128 xmm0_samples;

    pc_cb_samples += (u_chroma_stride); // skip p/q[3]
    pc_cr_samples += (u_chroma_stride); // skip p/q[3]

    _mm_store_ps((float*)&xmm0_samples, _mm_load_ps((float*)pf[2]));
    _mm_storel_pi((__m64*)pc_cb_samples, xmm0_samples);
    _mm_storeh_pi((__m64*)pc_cr_samples, xmm0_samples);
    pc_cb_samples += u_chroma_stride;
    pc_cr_samples += u_chroma_stride;
    _mm_store_ps((float*)&xmm0_samples, _mm_load_ps((float*)pf[1]));
    _mm_storel_pi((__m64*)pc_cb_samples, xmm0_samples);
    _mm_storeh_pi((__m64*)pc_cr_samples, xmm0_samples);
    pc_cb_samples += u_chroma_stride;
    pc_cr_samples += u_chroma_stride;
    _mm_store_ps((float*)&xmm0_samples, _mm_load_ps((float*)pf[0]));
    _mm_storel_pi((__m64*)pc_cb_samples, xmm0_samples);
    _mm_storeh_pi((__m64*)pc_cr_samples, xmm0_samples);
    pc_cb_samples += u_chroma_stride;
    pc_cr_samples += u_chroma_stride;

    _mm_store_ps((float*)&xmm0_samples, _mm_load_ps((float*)qf[0]));
    _mm_storel_pi((__m64*)pc_cb_samples, xmm0_samples);
    _mm_storeh_pi((__m64*)pc_cr_samples, xmm0_samples);
    pc_cb_samples += u_chroma_stride;
    pc_cr_samples += u_chroma_stride;
    _mm_store_ps((float*)&xmm0_samples, _mm_load_ps((float*)qf[1]));
    _mm_storel_pi((__m64*)pc_cb_samples, xmm0_samples);
    _mm_storeh_pi((__m64*)pc_cr_samples, xmm0_samples);
    pc_cb_samples += u_chroma_stride;
    pc_cr_samples += u_chroma_stride;
    _mm_store_ps((float*)&xmm0_samples, _mm_load_ps((float*)qf[2]));
    _mm_storel_pi((__m64*)pc_cb_samples, xmm0_samples);
    _mm_storeh_pi((__m64*)pc_cr_samples, xmm0_samples);
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_intrin_ssse3(
    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
    int16_t bS[2],
    int16_t alpha, int16_t beta,
    HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4];
    __m128i xmm_cmp, xmm_p0, xmm_p1, xmm_q0, xmm_q1, xmm_bS, xmm_alpha, xmm_beta;
    __m128i xmmi_zero;
    __m128 xmmf_zero;

    _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
    _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

    _mm_store_si128((__m128i*)&xmm_bS, _mm_set_epi16(bS[1], bS[1], bS[1], bS[1], bS[0], bS[0], bS[0], bS[0]));
    _mm_store_si128((__m128i*)&xmm_alpha, _mm_set1_epi16(alpha));
    _mm_store_si128((__m128i*)&xmm_beta, _mm_set1_epi16(beta));

#if 0
    _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
#else
    _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
    _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));

    _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
    _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
    _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
#endif


    // bS[0] != 0
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_cmpgt_epi16(xmm_bS, xmmi_zero));
    // && |p0 - q0| < alpha
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p0, xmm_q0)), xmm_alpha)));
    // && |p1 - p0| < beta
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p1, xmm_p0)), xmm_beta)));
    // && |q1 - q0| < beta
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_q1, xmm_q0)), xmm_beta)));
    // (8-468): filterSamplesFlag = bS[0] != 0 && |p0 - q0| < alpha && |p1 - p0| < beta && |q1 - q0| < beta
    _mm_store_si128((__m128i*)filterSamplesFlag, _mm_and_si128(_mm_load_si128((__m128i*)__x86_globals_array8_ones), xmm_cmp));
}


static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_intrin_sse4(
    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
    int16_t bS[2],
    int16_t alpha, int16_t beta,
    HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4];
    __m128i xmm_bS, xmm_ones_bits;

    _mm_store_si128((__m128i*)&xmm_bS, _mm_set_epi16(bS[1], bS[1], bS[1], bS[1], bS[0], bS[0], bS[0], bS[0]));
    _mm_store_si128((__m128i*)&xmm_ones_bits, _mm_load_si128((__m128i*)__x86_globals_array4_one_bits));

    if (_mm_testz_si128(xmm_ones_bits, xmm_bS)) {
        _mm_store_si128((__m128i*)filterSamplesFlag, _mm_setzero_si128());
    }
    else {
        __m128i xmm_cmp, xmm_p0, xmm_q0, xmm_alpha;
        __m128i xmmi_zero;
        __m128 xmmf_zero;

        _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
        _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

        _mm_store_si128((__m128i*)&xmm_alpha, _mm_set1_epi16(alpha));
#if 0
        _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
        _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
#else
        _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
        _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
        _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
        _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
#endif

        // bS[0] != 0
        _mm_store_si128((__m128i*)&xmm_cmp, _mm_cmpgt_epi16(xmm_bS, _mm_setzero_si128())); // at least #1 non-zero bS -> see first "_mm_testz_si128"
        // && |p0 - q0| < alpha
        _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p0, xmm_q0)), xmm_alpha)));
        if (_mm_testz_si128(xmm_ones_bits, xmm_cmp)) {
            _mm_store_si128((__m128i*)filterSamplesFlag, xmmi_zero);
        }
        else {
            __m128i xmm_beta, xmm_p1;

            _mm_store_si128((__m128i*)&xmm_beta, _mm_set1_epi16(beta));
#if 0
            _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
#else
            _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
            _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
#endif

            // && |p1 - p0| < beta
            _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p1, xmm_p0)), xmm_beta)));
            if (_mm_testz_si128(xmm_ones_bits, xmm_cmp)) {
                _mm_store_si128((__m128i*)filterSamplesFlag, xmmi_zero);
            }
            else {
                __m128i xmm_q1;
#if 0
                _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
#else
                _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
                _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));
#endif
                // && |q1 - q0| < beta
                _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_q1, xmm_q0)), xmm_beta)));
                if (_mm_testz_si128(xmm_ones_bits, xmm_cmp)) {
                    _mm_store_si128((__m128i*)filterSamplesFlag, xmmi_zero);
                }
                else {
                    // (8-468): filterSamplesFlag = bS[0] != 0 && |p0 - q0| < alpha && |p1 - p0| < beta && |q1 - q0| < beta
                    _mm_store_si128((__m128i*)filterSamplesFlag, _mm_and_si128(_mm_load_si128((__m128i*)__x86_globals_array8_ones), xmm_cmp));
                }
            }
        }
    }
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_intrin_ssse3(
    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
    int16_t bS[4],
    int16_t alpha, int16_t beta,
    HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4];
    __m128i xmm_cmp, xmm_p0, xmm_p1, xmm_q0, xmm_q1, xmm_bS, xmm_alpha, xmm_beta;
    __m128i xmmi_zero;
    __m128 xmmf_zero;

    _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
    _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

    _mm_store_si128((__m128i*)&xmm_bS, _mm_set_epi16(bS[3], bS[3], bS[2], bS[2], bS[1], bS[1], bS[0], bS[0]));
    _mm_store_si128((__m128i*)&xmm_alpha, _mm_set1_epi16(alpha));
    _mm_store_si128((__m128i*)&xmm_beta, _mm_set1_epi16(beta));

#if 0
    _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
#else
    _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
    _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));

    _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
    _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
    _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
#endif


    // bS[0] != 0
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_cmpgt_epi16(xmm_bS, _mm_setzero_si128()));
    // && |p0 - q0| < alpha
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p0, xmm_q0)), xmm_alpha)));
    // && |p1 - p0| < beta
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p1, xmm_p0)), xmm_beta)));
    // && |q1 - q0| < beta
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_q1, xmm_q0)), xmm_beta)));
    // (8-468): filterSamplesFlag = bS[0] != 0 && |p0 - q0| < alpha && |p1 - p0| < beta && |q1 - q0| < beta
    _mm_store_si128((__m128i*)filterSamplesFlag, _mm_and_si128(_mm_load_si128((__m128i*)__x86_globals_array8_ones), xmm_cmp));
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_intrin_sse4(
    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
    int16_t bS[4],
    int16_t alpha, int16_t beta,
    HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4];
    __m128i xmm_bS, xmm_ones_bits;

    _mm_store_si128((__m128i*)&xmm_bS, _mm_set_epi16(bS[3], bS[3], bS[2], bS[2], bS[1], bS[1], bS[0], bS[0]));
    _mm_store_si128((__m128i*)&xmm_ones_bits, _mm_load_si128((__m128i*)__x86_globals_array4_one_bits));

    if (_mm_testz_si128(xmm_ones_bits, xmm_bS)) {
        _mm_store_si128((__m128i*)filterSamplesFlag, _mm_setzero_si128());
    }
    else {
        __m128i xmm_cmp, xmm_p0, xmm_q0, xmm_alpha;
        __m128i xmmi_zero;
        __m128 xmmf_zero;

        _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
        _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

        _mm_store_si128((__m128i*)&xmm_alpha, _mm_set1_epi16(alpha));
#if 0
        _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
        _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
#else
        _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
        _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
        _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
        _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
#endif

        // bS[0] != 0
        _mm_store_si128((__m128i*)&xmm_cmp, _mm_cmpgt_epi16(xmm_bS, _mm_setzero_si128())); // at least #1 non-zero bS -> see first "_mm_testz_si128"
        // && |p0 - q0| < alpha
        _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p0, xmm_q0)), xmm_alpha)));
        if (_mm_testz_si128(xmm_ones_bits, xmm_cmp)) {
            _mm_store_si128((__m128i*)filterSamplesFlag, xmmi_zero);
        }
        else {
            __m128i xmm_beta, xmm_p1;

            _mm_store_si128((__m128i*)&xmm_beta, _mm_set1_epi16(beta));
#if 0
            _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
#else
            _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
            _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
#endif

            // && |p1 - p0| < beta
            _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p1, xmm_p0)), xmm_beta)));
            if (_mm_testz_si128(xmm_ones_bits, xmm_cmp)) {
                _mm_store_si128((__m128i*)filterSamplesFlag, xmmi_zero);
            }
            else {
                __m128i xmm_q1;
#if 0
                _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
#else
                _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
                _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));
#endif

                // && |q1 - q0| < beta
                _mm_store_si128((__m128i*)&xmm_cmp, _mm_and_si128(xmm_cmp, _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_q1, xmm_q0)), xmm_beta)));
                if (_mm_testz_si128(xmm_ones_bits, xmm_cmp)) {
                    _mm_store_si128((__m128i*)filterSamplesFlag, xmmi_zero);
                }
                else {
                    // (8-468): filterSamplesFlag = bS[0] != 0 && |p0 - q0| < alpha && |p1 - p0| < beta && |q1 - q0| < beta
                    _mm_store_si128((__m128i*)filterSamplesFlag, _mm_and_si128(_mm_load_si128((__m128i*)__x86_globals_array8_ones), xmm_cmp));
                }
            }
        }
    }
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_intrin_ssse3(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2,
    int16_t bS[2],
    int16_t indexA,
    int16_t beta,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fours[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_move_mask_hi64[4];
    extern const int32_t HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[52][5];

    __m128i xmm_tc0, xmm_tc, xmm_ap, xmm_aq, xmm_delta, xmm_p0, xmm_p1, xmm_p2, xmm_q0, xmm_q1, xmm_q2, xmm_min, xmm_4s, xmm_tmp, xmm_cmp, xmm_ones, xmm_beta;
    __m128i xmmi_zero;
    __m128 xmmf_zero;

    _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
    _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

    _mm_store_si128((__m128i*)&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array8_ones));
    _mm_store_si128((__m128i*)&xmm_4s, _mm_load_si128((__m128i*)__x86_globals_array8_fours));

    _mm_store_si128((__m128i*)&xmm_beta, _mm_set1_epi16(beta));

#if 0
    _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
    _mm_store_si128((__m128i*)&xmm_q2, _mm_set_epi16(q2[7], q2[6], q2[5], q2[4], q2[3], q2[2], q2[1], q2[0]));

    _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
    _mm_store_si128((__m128i*)&xmm_p2, _mm_set_epi16(p2[7], p2[6], p2[5], p2[4], p2[3], p2[2], p2[1], p2[0]));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
#else
    _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
    _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));
    _mm_store_ps((float *)&xmm_q2, _mm_loadl_pi(xmmf_zero, (__m64 *)q2));
    _mm_store_si128((__m128i*)&xmm_q2, _mm_unpacklo_epi8(xmm_q2, xmmi_zero));

    _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
    _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
    _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
    _mm_store_ps((float *)&xmm_p2, _mm_loadl_pi(xmmf_zero, (__m64 *)p2));
    _mm_store_si128((__m128i*)&xmm_p2, _mm_unpacklo_epi8(xmm_p2, xmmi_zero));
#endif

    _mm_store_si128((__m128i*)&xmm_tc0, _mm_set_epi16(
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]]));

    // (8-471), ap = |p2 - p0|
    _mm_store_si128((__m128i*)&xmm_ap, _mm_abs_epi16(_mm_sub_epi16(xmm_p2, xmm_p0)));

    // (8-472), aq = |q2 - q0|
    _mm_store_si128((__m128i*)&xmm_aq, _mm_abs_epi16(_mm_sub_epi16(xmm_q2, xmm_q0)));

    // (8-473), tc = tc0 + ((ap < beta) ? 1 : 0) + ((aq < beta) ? 1 : 0)
    _mm_store_si128((__m128i*)&xmm_tc,
                    _mm_add_epi16(xmm_tc0, _mm_add_epi16(_mm_and_si128(_mm_cmplt_epi16(xmm_ap, xmm_beta), xmm_ones), _mm_and_si128(_mm_cmplt_epi16(xmm_aq, xmm_beta), xmm_ones))));

    // (8-475), delta = clip3(-tc, tc, ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3))
    _mm_store_si128((__m128i*)&xmm_min, _mm_sub_epi16(_mm_setzero_si128(), xmm_tc));
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_slli_epi16(_mm_sub_epi16(xmm_q0, xmm_p0), 2), _mm_add_epi16(_mm_sub_epi16(xmm_p1, xmm_q1), xmm_4s)), 3));
    _mm_store_si128((__m128i*)&xmm_delta, hl_mm_clip3_epi16_sse2(xmm_min, xmm_tc, xmm_tmp));

    // (8-476), pf0 = clip3(0, 255, (p0 + delta))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(xmm_p0, xmm_delta));
    _mm_storel_epi64((__m128i*)pf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-477), qf0 = clip3(0, 255, (q0 - delta))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_sub_epi16(xmm_q0, xmm_delta));
    _mm_storel_epi64((__m128i*)qf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-478), if (ap < beta) pf1 = p1 + clip3(-tc0, tc0, (p2[0] + ((p0[0] + q0[0] + 1) >> 1) - (p1[0] << 1)) >> 1); else pf1 = p1;
    _mm_store_si128((__m128i*)&xmm_min, _mm_sub_epi16(_mm_setzero_si128(), xmm_tc0));
    _mm_store_si128((__m128i*)&xmm_tmp, hl_mm_clip3_epi16_sse2(xmm_min, xmm_tc0, _mm_srai_epi16(_mm_add_epi16(xmm_p2, _mm_sub_epi16(_mm_avg_epu16(xmm_p0, xmm_q0), _mm_slli_epi16(xmm_p1, 1))), 1)));
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_cmplt_epi16(xmm_ap, xmm_beta));
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(xmm_p1, _mm_and_si128(xmm_cmp, xmm_tmp)));
    _mm_storel_epi64((__m128i*)pf1, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-480), if (aq < beta) qf1 = q1 + clip3(-tc0, tc0, (q2[0] + ((p0[0] + q0[0] + 1) >> 1) - (q1[0] << 1)) >> 1); else qf1 = q1;
    _mm_store_si128((__m128i*)&xmm_tmp, hl_mm_clip3_epi16_sse2(xmm_min, xmm_tc0, _mm_srai_epi16(_mm_add_epi16(xmm_q2, _mm_sub_epi16(_mm_avg_epu16(xmm_p0, xmm_q0), _mm_slli_epi16(xmm_q1, 1))), 1)));
    _mm_store_si128((__m128i*)&xmm_cmp, _mm_cmplt_epi16(xmm_aq, xmm_beta));
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(xmm_q1, _mm_and_si128(xmm_cmp, xmm_tmp)));
    _mm_storel_epi64((__m128i*)qf1, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-482), pf2 = p2
    *((uint64_t*)pf2) = *((uint64_t*)p2);

    // (8-483), qf2 = q2
    *((uint64_t*)qf2) = *((uint64_t*)q2);
}

// TODO: SSE2 instead?
static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_intrin_ssse3(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2,
    int16_t bS[4],
    int16_t indexA,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_ones[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fours[4];
    extern const int32_t HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[52][5];

    __m128i xmm_tc0, xmm_tc, xmm_delta, xmm_min, xmm_tmp, xmm_ones, xmm_4s, xmm_p0, xmm_p1, xmm_p2, xmm_q0, xmm_q1, xmm_q2;
    __m128i xmmi_zero;
    __m128 xmmf_zero;

    _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
    _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

    _mm_store_si128((__m128i*)&xmm_ones, _mm_load_si128((__m128i*)__x86_globals_array8_ones));
    _mm_store_si128((__m128i*)&xmm_4s, _mm_load_si128((__m128i*)__x86_globals_array8_fours));

#if 0
    _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
    _mm_store_si128((__m128i*)&xmm_q2, _mm_set_epi16(q2[7], q2[6], q2[5], q2[4], q2[3], q2[2], q2[1], q2[0]));

    _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
    _mm_store_si128((__m128i*)&xmm_p2, _mm_set_epi16(p2[7], p2[6], p2[5], p2[4], p2[3], p2[2], p2[1], p2[0]));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
#else
    _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
    _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));
    _mm_store_ps((float *)&xmm_q2, _mm_loadl_pi(xmmf_zero, (__m64 *)q2));
    _mm_store_si128((__m128i*)&xmm_q2, _mm_unpacklo_epi8(xmm_q2, xmmi_zero));

    _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
    _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
    _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
    _mm_store_ps((float *)&xmm_p2, _mm_loadl_pi(xmmf_zero, (__m64 *)p2));
    _mm_store_si128((__m128i*)&xmm_p2, _mm_unpacklo_epi8(xmm_p2, xmmi_zero));
#endif

    _mm_store_si128((__m128i*)&xmm_tc0, _mm_set_epi16(
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[3]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[3]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[2]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[2]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]],
                        HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]]));

    // (8-473), tc = tc0 + 1
    _mm_store_si128((__m128i*)&xmm_tc, _mm_add_epi16(xmm_tc0, xmm_ones));

    // (8-475), delta = clip3(-tc[0], tc[0], ((((q0[0] - p0[0]) << 2) + (p1[0] - q1[0]) + 4) >> 3));
    _mm_store_si128((__m128i*)&xmm_min, _mm_sub_epi16(_mm_setzero_si128(), xmm_tc));
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_slli_epi16(_mm_sub_epi16(xmm_q0, xmm_p0), 2), _mm_add_epi16(_mm_sub_epi16(xmm_p1, xmm_q1), xmm_4s)), 3));
    _mm_store_si128((__m128i*)&xmm_delta, hl_mm_clip3_epi16_sse2(xmm_min, xmm_tc, xmm_tmp));

    // (8-476), pf0 = clip3(0, 255, (p0 + delta))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(xmm_p0, xmm_delta));
    _mm_storel_epi64((__m128i*)pf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-477), qf0 = clip3(0, 255, (q0 - delta))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_sub_epi16(xmm_q0, xmm_delta));
    _mm_storel_epi64((__m128i*)qf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-479), pf1 = p1
    *((uint64_t*)pf1) = *((uint64_t*)p1);

    // (8-481), qf1 = q1
    *((uint64_t*)qf1) = *((uint64_t*)q1);

    // (8-482), pf2 = p2
    *((uint64_t*)pf2) = *((uint64_t*)p2);

    // (8-483), qf2 = q2
    *((uint64_t*)qf2) = *((uint64_t*)q2);
}



static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_intrin_ssse3(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3,
    int16_t alpha, int16_t beta,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_twos[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_threes[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fours[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4];

    __m128i xmm_tmp, xmm_cmp_if, xmm_cmp_else, xmm_ret_if, xmm_ret_else, xmm_2, xmm_3, xmm_4, xmm_ap, xmm_aq, xmm_p0, xmm_p1, xmm_p2, xmm_p3, xmm_q0, xmm_q1, xmm_q2, xmm_q3, xmm_beta, xmm_alpha;
    __m128i xmmi_zero;
    __m128 xmmf_zero;

    _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
    _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

#if 0
    _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));
    _mm_store_si128((__m128i*)&xmm_q2, _mm_set_epi16(q2[7], q2[6], q2[5], q2[4], q2[3], q2[2], q2[1], q2[0]));
    _mm_store_si128((__m128i*)&xmm_q3, _mm_set_epi16(q3[7], q3[6], q3[5], q3[4], q3[3], q3[2], q3[1], q3[0]));

    _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
    _mm_store_si128((__m128i*)&xmm_p2, _mm_set_epi16(p2[7], p2[6], p2[5], p2[4], p2[3], p2[2], p2[1], p2[0]));
    _mm_store_si128((__m128i*)&xmm_p3, _mm_set_epi16(p3[7], p3[6], p3[5], p3[4], p3[3], p3[2], p3[1], p3[0]));
#else
    _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
    _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));
    _mm_store_ps((float *)&xmm_q2, _mm_loadl_pi(xmmf_zero, (__m64 *)q2));
    _mm_store_si128((__m128i*)&xmm_q2, _mm_unpacklo_epi8(xmm_q2, xmmi_zero));
    _mm_store_ps((float *)&xmm_q3, _mm_loadl_pi(xmmf_zero, (__m64 *)q3));
    _mm_store_si128((__m128i*)&xmm_q3, _mm_unpacklo_epi8(xmm_q3, xmmi_zero));

    _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
    _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
    _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
    _mm_store_ps((float *)&xmm_p2, _mm_loadl_pi(xmmf_zero, (__m64 *)p2));
    _mm_store_si128((__m128i*)&xmm_p2, _mm_unpacklo_epi8(xmm_p2, xmmi_zero));
    _mm_store_ps((float *)&xmm_p3, _mm_loadl_pi(xmmf_zero, (__m64 *)p3));
    _mm_store_si128((__m128i*)&xmm_p3, _mm_unpacklo_epi8(xmm_p3, xmmi_zero));
#endif

    _mm_store_si128((__m128i*)&xmm_beta, _mm_set1_epi16(beta));
    _mm_store_si128((__m128i*)&xmm_alpha, _mm_set1_epi16(alpha));

    _mm_store_si128((__m128i*)&xmm_2,_mm_load_si128((__m128i*)__x86_globals_array8_twos));
    _mm_store_si128((__m128i*)&xmm_4,_mm_load_si128((__m128i*)__x86_globals_array8_fours));
    _mm_store_si128((__m128i*)&xmm_3,_mm_load_si128((__m128i*)__x86_globals_array8_threes));

    // (8-471), ap = |p2 - p0|
    _mm_store_si128((__m128i*)&xmm_ap, _mm_abs_epi16(_mm_sub_epi16(xmm_p2, xmm_p0)));

    // (8-472), aq = |q2 - q0|
    _mm_store_si128((__m128i*)&xmm_aq, _mm_abs_epi16(_mm_sub_epi16(xmm_q2, xmm_q0)));

    // TODO: compute a leg only if cmp_xxx has at least one '1' -> use SSE4.1 "_mm_testz_si128()"

    /*** PF ***/
    // cmp_if = (ap < beta && |p0[0] - q0[0]| < ((alpha >> 2) + 2))
    _mm_store_si128((__m128i*)&xmm_cmp_if, _mm_and_si128(_mm_cmplt_epi16(xmm_ap, xmm_beta), _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p0, xmm_q0)), _mm_add_epi16(_mm_srai_epi16(xmm_alpha, 2), xmm_2))));
    // cmp_else = !cmp_if
    _mm_store_si128((__m128i*)&xmm_cmp_else, _mm_andnot_si128(xmm_cmp_if, _mm_load_si128((__m128i*)__x86_globals_array4_one_bits)));

    // (8-485), cmp_if() -> pf0 = (p2 + (p1 << 1) + (p0 << 1) + (q0 << 1) + q1 + 4) >> 3
    _mm_store_si128((__m128i*)&xmm_ret_if, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(xmm_p2, _mm_slli_epi16(xmm_p1, 1)), _mm_slli_epi16(xmm_p0, 1)), _mm_slli_epi16(xmm_q0, 1)), xmm_q1), xmm_4), 3));
    // (8-488), cmp_else() -> pf0 = ((p1 << 1) + p0 + q1 + 2) >> 2
    _mm_store_si128((__m128i*)&xmm_ret_else, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(xmm_p1, 1), xmm_p0), xmm_q1), xmm_2), 2));
    // (8-485) + (8-488), pf0 = ((ret_if & cmp_if) + (ret_else & cmp_else))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(_mm_and_si128(xmm_ret_if, xmm_cmp_if), _mm_and_si128(xmm_ret_else, xmm_cmp_else)));
    _mm_storel_epi64((__m128i*)pf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-486), cmp_if() -> pf1 = (p2 + p1 + p0 + q0 + 2) >> 2
    _mm_store_si128((__m128i*)&xmm_ret_if, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(xmm_p2, xmm_p1), xmm_p0), xmm_q0), xmm_2), 2));
    // (8-489), cmp_else() -> pf1 = p1
    // (8-486) + (8-489), pf1 = ((ret_if & cmp_if) + (p1 & cmp_else))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(_mm_and_si128(xmm_ret_if, xmm_cmp_if), _mm_and_si128(xmm_p1, xmm_cmp_else)));
    _mm_storel_epi64((__m128i*)pf1, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-487), cmp_if() -> pf2 = ((p3 << 1) + 3*p2 + p1 + p0 + q0 + 4) >> 3;
    _mm_store_si128((__m128i*)&xmm_ret_if, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(xmm_p3, 1), _mm_mullo_epi16(xmm_p2, xmm_3)), xmm_p1), xmm_p0), xmm_q0), xmm_4), 3));
    // (8-490), cmp_else() -> pf2 = p2
    // (8-487) + (8-490), pf2 = ((ret_if & cmp_if) + (p2 & cmp_else))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(_mm_and_si128(xmm_ret_if, xmm_cmp_if), _mm_and_si128(xmm_p2, xmm_cmp_else)));
    _mm_storel_epi64((__m128i*)pf2, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    /***QF***/
    // cmp_if = (aq < beta && |p0[0] - q0[0]| < ((alpha >> 2) + 2))
    _mm_store_si128((__m128i*)&xmm_cmp_if, _mm_and_si128(_mm_cmplt_epi16(xmm_aq, xmm_beta), _mm_cmplt_epi16(_mm_abs_epi16(_mm_sub_epi16(xmm_p0, xmm_q0)), _mm_add_epi16(_mm_srai_epi16(xmm_alpha, 2), xmm_2))));
    // cmp_else = !cmp_ok
    _mm_store_si128((__m128i*)&xmm_cmp_else, _mm_andnot_si128(xmm_cmp_if, _mm_load_si128((__m128i*)__x86_globals_array4_one_bits)));

    // (8-492), cmp_if() -> qf0 = (p1 + (p0 << 1) + (q0 << 1) + (q1 << 1) + q2 + 4) >> 3
    _mm_store_si128((__m128i*)&xmm_ret_if, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(xmm_p1, _mm_slli_epi16(xmm_p0, 1)), _mm_slli_epi16(xmm_q0, 1)), _mm_slli_epi16(xmm_q1, 1)), xmm_q2), xmm_4), 3));
    // (8-495), cmp_else() -> qf0 = ((q1 << 1) + q0 + p1 + 2) >> 2
    _mm_store_si128((__m128i*)&xmm_ret_else, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(xmm_q1, 1), xmm_q0), xmm_p1), xmm_2), 2));
    // (8-485) + (8-488), qf0 = ((ret_if & cmp_if) + (ret_else & cmp_else))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(_mm_and_si128(xmm_ret_if, xmm_cmp_if), _mm_and_si128(xmm_ret_else, xmm_cmp_else)));
    _mm_storel_epi64((__m128i*)qf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-493), cmp_if() -> qf1 = (p0 + q0 + q1 + q2 + 2) >> 2
    _mm_store_si128((__m128i*)&xmm_ret_if, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(xmm_p0, xmm_q0), xmm_q1), xmm_q2), xmm_2), 2));
    // (8-496), cmp_else() -> qf1 = q1
    // (8-493) + (8-496), qf1 = ((ret_if & cmp_if) + (q1 & cmp_else))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(_mm_and_si128(xmm_ret_if, xmm_cmp_if), _mm_and_si128(xmm_q1, xmm_cmp_else)));
    _mm_storel_epi64((__m128i*)qf1, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-494), cmp_if() -> pf2 = ((q3 << 1) + 3*q2 + q1 + q0 + p0 + 4) >> 3;
    _mm_store_si128((__m128i*)&xmm_ret_if, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(xmm_q3, 1), _mm_mullo_epi16(xmm_q2, xmm_3)), xmm_q1), xmm_q0), xmm_p0), xmm_4), 3));
    // (8-497), cmp_else() -> qf2 = p2
    // (8-494) + (8-497), qf2 = ((ret_if & cmp_if) + (q2 & cmp_else))
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_add_epi16(_mm_and_si128(xmm_ret_if, xmm_cmp_if), _mm_and_si128(xmm_q2, xmm_cmp_else)));
    _mm_storel_epi64((__m128i*)qf2, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)
}

static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_intrin_sse2(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_twos[4];

    __m128i xmm_p0, xmm_p1, xmm_q0, xmm_q1, xmm_2, xmm_tmp;
    __m128i xmmi_zero;
    __m128 xmmf_zero;

    _mm_store_si128(&xmmi_zero, _mm_setzero_si128());
    _mm_store_ps((float *)&xmmf_zero, _sse_setzero_ps());

#if 0
    _mm_store_si128((__m128i*)&xmm_q0, _mm_set_epi16(q0[7], q0[6], q0[5], q0[4], q0[3], q0[2], q0[1], q0[0]));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_set_epi16(q1[7], q1[6], q1[5], q1[4], q1[3], q1[2], q1[1], q1[0]));

    _mm_store_si128((__m128i*)&xmm_p0, _mm_set_epi16(p0[7], p0[6], p0[5], p0[4], p0[3], p0[2], p0[1], p0[0]));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_set_epi16(p1[7], p1[6], p1[5], p1[4], p1[3], p1[2], p1[1], p1[0]));
#else
    _mm_store_ps((float *)&xmm_q0, _mm_loadl_pi(xmmf_zero, (__m64 *)q0));
    _mm_store_si128((__m128i*)&xmm_q0, _mm_unpacklo_epi8(xmm_q0, xmmi_zero));
    _mm_store_ps((float *)&xmm_q1, _mm_loadl_pi(xmmf_zero, (__m64 *)q1));
    _mm_store_si128((__m128i*)&xmm_q1, _mm_unpacklo_epi8(xmm_q1, xmmi_zero));

    _mm_store_ps((float *)&xmm_p0, _mm_loadl_pi(xmmf_zero, (__m64 *)p0));
    _mm_store_si128((__m128i*)&xmm_p0, _mm_unpacklo_epi8(xmm_p0, xmmi_zero));
    _mm_store_ps((float *)&xmm_p1, _mm_loadl_pi(xmmf_zero, (__m64 *)p1));
    _mm_store_si128((__m128i*)&xmm_p1, _mm_unpacklo_epi8(xmm_p1, xmmi_zero));
#endif

    _mm_store_si128((__m128i*)&xmm_2,_mm_load_si128((__m128i*)__x86_globals_array8_twos));

    // (8-488), pf0 = ((p1 << 1) + p0 + q1 + 2) >> 2;
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(xmm_p1, 1), xmm_p0), xmm_q1), xmm_2), 2));
    _mm_storel_epi64((__m128i*)pf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    // (8-489), pf1 = p1
    *((uint64_t*)pf1) = *((uint64_t*)p1);

    // (8-490), pf2 = p2
    *((uint64_t*)pf2) = *((uint64_t*)p2);

    //(8-495), qf0 = ((q1 << 1) + q0 + p1 + 2) >> 2;
    _mm_store_si128((__m128i*)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(xmm_q1, 1), xmm_q0), xmm_p1), xmm_2), 2));
    _mm_storel_epi64((__m128i*)qf0, _mm_packus_epi16(xmm_tmp, xmm_tmp)); // _mm_packus_epi16() = 16bits->8bits and clip3(0, 255, 8bitsVal)

    //(8-496), qf1 = q1
    *((uint64_t*)qf1) = *((uint64_t*)q1);

    //(8-497), qf2 = q2
    *((uint64_t*)qf2) = *((uint64_t*)q2);
}


#if 0
static HL_SHOULD_INLINE void hl_codec_x86_264_deblock_avc_baseline_load_pq_vert_luma_u8_intrin_sse2(
    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_move_mask_hi64[4];

    HL_ALIGN(HL_ALIGN_V) uint8_t pp [8][8+8] = { 0 };
    HL_ALIGN(HL_ALIGN_V) uint8_t qq [8][8+8] = { 0 };

    HL_ALIGN(8) uint8_t mmx_tmp[8*2] = { 0 };

    HL_ALIGN(HL_ALIGN_V) __m64 mmx_row[4/*4x8*/][2/*p=0,q=0*/];
    __m64 mmx_tmp0, mmx_tmp1, mmx_tmp_row[4];
    __m64 mmx_pq[4/*row*/][2/*p=0,q=0*/];

    __m128i sse_hi;

TODO:
    not implemented


    sse_hi = _mm_load_si128((__m128i*)__x86_globals_array4_move_mask_hi64);

    // #1 4x8
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[1]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[2]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[3]);
    pc_luma_samples += u_luma_stride;
    mmx_tmp0 = _mm_unpacklo_pi8(mmx_pq[0][0], mmx_pq[2][0]);
    mmx_tmp1 = _mm_unpacklo_pi8(mmx_pq[1][0], mmx_pq[3][0]);
    mmx_tmp_row[0] = _mm_unpacklo_pi8(mmx_tmp0, mmx_tmp1);
    mmx_tmp_row[1] = _mm_unpackhi_pi8(mmx_tmp0, mmx_tmp1);

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[1]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[2]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[3]);
    pc_luma_samples += u_luma_stride;
    mmx_tmp0 = _mm_unpacklo_pi8(mmx_pq[0][0], mmx_pq[2][0]);
    mmx_tmp1 = _mm_unpacklo_pi8(mmx_pq[1][0], mmx_pq[3][0]);
    mmx_tmp_row[2] = _mm_unpacklo_pi8(mmx_tmp0, mmx_tmp1);
    mmx_tmp_row[3] = _mm_unpackhi_pi8(mmx_tmp0, mmx_tmp1);

    mmx_row[0][0] = _mm_unpacklo_pi8(mmx_tmp_row[0] , mmx_tmp_row[2]);
    mmx_row[1][0] = _mm_unpackhi_pi8(mmx_tmp_row[0] , mmx_tmp_row[2]);
    mmx_row[2][0] = _mm_unpacklo_pi8(mmx_tmp_row[1] , mmx_tmp_row[3]);
    mmx_row[3][0] = _mm_unpackhi_pi8(mmx_tmp_row[1] , mmx_tmp_row[3]);

    // #2 4x8

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[1]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[2]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[3]);
    pc_luma_samples += u_luma_stride;
    mmx_tmp0 = _mm_unpacklo_pi8(mmx_pq[0][0], mmx_pq[2][0]);
    mmx_tmp1 = _mm_unpacklo_pi8(mmx_pq[1][0], mmx_pq[3][0]);
    mmx_tmp_row[0] = _mm_unpacklo_pi8(mmx_tmp0, mmx_tmp1);
    mmx_tmp_row[1] = _mm_unpackhi_pi8(mmx_tmp0, mmx_tmp1);

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[1]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[2]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), sse_hi, (char *)mmx_pq[3]);
    mmx_tmp0 = _mm_unpacklo_pi8(mmx_pq[0][0], mmx_pq[2][0]);
    mmx_tmp1 = _mm_unpacklo_pi8(mmx_pq[1][0], mmx_pq[3][0]);
    mmx_tmp_row[2] = _mm_unpacklo_pi8(mmx_tmp0, mmx_tmp1);
    mmx_tmp_row[3] = _mm_unpackhi_pi8(mmx_tmp0, mmx_tmp1);

    mmx_row[0][1] = _mm_unpacklo_pi8(mmx_tmp_row[0] , mmx_tmp_row[2]);
    mmx_row[1][1] = _mm_unpackhi_pi8(mmx_tmp_row[0] , mmx_tmp_row[2]);
    mmx_row[2][1] = _mm_unpacklo_pi8(mmx_tmp_row[1] , mmx_tmp_row[3]);
    mmx_row[3][1] = _mm_unpackhi_pi8(mmx_tmp_row[1] , mmx_tmp_row[3]);

    // EMMS
    _mm_empty();

    /*
    	HL_ALIGN(HL_ALIGN_V) uint8_t pp [8][8+8] = { 0 };
    	HL_ALIGN(HL_ALIGN_V) uint8_t qq [8][8+8] = { 0 };

    	HL_ALIGN(HL_ALIGN_V) uint8_t rr [8][16] = { 0 };
    	extern const int32_t __x86_globals_array4_move_mask_lo64[4];
    	extern const int32_t __x86_globals_array4_move_mask_hi64[4];
    	extern const int32_t __x86_globals_array4_move_mask_hi32[4];
    	__m128i xmm_hi;


    	xmm_hi = _mm_load_si128((__m128i*)__x86_globals_array4_move_mask_hi64);

    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[0]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[1]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[2]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[3]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[4]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[5]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[6]);
    	pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[7]);
    	pc_luma_samples += u_luma_stride;

    	// EPI8 matrix transpose
    	_mm_store_si128((__m128i*)rr[0], _mm_unpacklo_epi8(_mm_load_si128((__m128i*)&pp[0]), _mm_load_si128((__m128i*)&pp[2])));
    	_mm_store_si128((__m128i*)rr[1], _mm_unpacklo_epi8(_mm_load_si128((__m128i*)&pp[1]), _mm_load_si128((__m128i*)&pp[3])));

    	_mm_store_si128((__m128i*)rr[2], _mm_unpacklo_epi8(_mm_load_si128((__m128i*)&rr[0]), _mm_load_si128((__m128i*)&rr[1])));
    	_mm_store_si128((__m128i*)rr[3], _mm_unpackhi_epi8(_mm_load_si128((__m128i*)&rr[0]), _mm_load_si128((__m128i*)&rr[1])));
    */


    /*
    HL_ALIGN(HL_ALIGN_V) uint8_t pp [4][16] = { 0 };
    HL_ALIGN(HL_ALIGN_V) uint8_t qq [4][16] = { 0 };

    HL_ALIGN(HL_ALIGN_V) uint8_t rr [4][16] = { 0 };
    extern const int32_t __x86_globals_array4_move_mask_lo64[4];
    extern const int32_t __x86_globals_array4_move_mask_hi64[4];
    extern const int32_t __x86_globals_array4_move_mask_hi32[4];
    __m128i xmm_hi;


    xmm_hi = _mm_load_si128((__m128i*)__x86_globals_array4_move_mask_hi32);

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[0][0]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[0][0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[1][0]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[1][0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[2][0]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[2][0]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[3][0]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[3][0]);
    pc_luma_samples += u_luma_stride;

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[0][4]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[0][4]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[1][4]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[1][4]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[2][4]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[2][4]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[3][4]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[3][4]);
    pc_luma_samples += u_luma_stride;

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[0][8]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[0][8]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[1][8]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[1][8]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[2][8]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[2][8]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[3][8]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[3][8]);
    pc_luma_samples += u_luma_stride;

    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[0][12]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[0][12]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[1][12]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[1][12]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[2][12]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[2][12]);
    pc_luma_samples += u_luma_stride;
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[0]), xmm_hi, (char *)&pp[3][12]);
    _mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)&pc_luma_samples[4]), xmm_hi, (char *)&qq[3][12]);
    pc_luma_samples += u_luma_stride;

    // EPI8 matrix transpose
    _mm_store_si128((__m128i*)rr[0], _mm_unpacklo_epi8(_mm_load_si128((__m128i*)pp[0]), _mm_load_si128((__m128i*)pp[2])));
    _mm_store_si128((__m128i*)rr[1], _mm_unpackhi_epi8(_mm_load_si128((__m128i*)pp[1]), _mm_load_si128((__m128i*)pp[3])));
    _mm_store_si128((__m128i*)rr[2], _mm_unpacklo_epi8(_mm_load_si128((__m128i*)rr[0]), _mm_load_si128((__m128i*)rr[1])));
    _mm_store_si128((__m128i*)rr[3], _mm_unpackhi_epi8(_mm_load_si128((__m128i*)rr[0]), _mm_load_si128((__m128i*)pp[1])));
    */

    /*
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)pc_luma_samples), xmm1_hi, (char *)&qq[0][0]); pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)pc_luma_samples), xmm1_hi, (char *)&qq[0][8]); pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)pc_luma_samples), xmm1_hi, (char *)&qq[1][0]); pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)pc_luma_samples), xmm1_hi, (char *)&qq[1][8]); pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)pc_luma_samples), xmm1_hi, (char *)&qq[2][0]); pc_luma_samples += u_luma_stride;
    	_mm_maskmoveu_si128(_mm_loadu_si128((__m128i*)pc_luma_samples), xmm1_hi, (char *)&qq[2][8]); pc_luma_samples += u_luma_stride;
    */



    p[3][0] = pc_luma_samples[0], p[2][0] = pc_luma_samples[1], p[1][0] = pc_luma_samples[2], p[0][0] = pc_luma_samples[3];
    q[0][0] = pc_luma_samples[4], q[1][0] = pc_luma_samples[5], q[2][0] = pc_luma_samples[6], q[3][0] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][1] = pc_luma_samples[0], p[2][1] = pc_luma_samples[1], p[1][1] = pc_luma_samples[2], p[0][1] = pc_luma_samples[3];
    q[0][1] = pc_luma_samples[4], q[1][1] = pc_luma_samples[5], q[2][1] = pc_luma_samples[6], q[3][1] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][2] = pc_luma_samples[0], p[2][2] = pc_luma_samples[1], p[1][2] = pc_luma_samples[2], p[0][2] = pc_luma_samples[3];
    q[0][2] = pc_luma_samples[4], q[1][2] = pc_luma_samples[5], q[2][2] = pc_luma_samples[6], q[3][2] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][3] = pc_luma_samples[0], p[2][3] = pc_luma_samples[1], p[1][3] = pc_luma_samples[2], p[0][3] = pc_luma_samples[3];
    q[0][3] = pc_luma_samples[4], q[1][3] = pc_luma_samples[5], q[2][3] = pc_luma_samples[6], q[3][3] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][4] = pc_luma_samples[0], p[2][4] = pc_luma_samples[1], p[1][4] = pc_luma_samples[2], p[0][4] = pc_luma_samples[3];
    q[0][4] = pc_luma_samples[4], q[1][4] = pc_luma_samples[5], q[2][4] = pc_luma_samples[6], q[3][4] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][5] = pc_luma_samples[0], p[2][5] = pc_luma_samples[1], p[1][5] = pc_luma_samples[2], p[0][5] = pc_luma_samples[3];
    q[0][5] = pc_luma_samples[4], q[1][5] = pc_luma_samples[5], q[2][5] = pc_luma_samples[6], q[3][5] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][6] = pc_luma_samples[0], p[2][6] = pc_luma_samples[1], p[1][6] = pc_luma_samples[2], p[0][6] = pc_luma_samples[3];
    q[0][6] = pc_luma_samples[4], q[1][6] = pc_luma_samples[5], q[2][6] = pc_luma_samples[6], q[3][6] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][7] = pc_luma_samples[0], p[2][7] = pc_luma_samples[1], p[1][7] = pc_luma_samples[2], p[0][7] = pc_luma_samples[3];
    q[0][7] = pc_luma_samples[4], q[1][7] = pc_luma_samples[5], q[2][7] = pc_luma_samples[6], q[3][7] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][8] = pc_luma_samples[0], p[2][8] = pc_luma_samples[1], p[1][8] = pc_luma_samples[2], p[0][8] = pc_luma_samples[3];
    q[0][8] = pc_luma_samples[4], q[1][8] = pc_luma_samples[5], q[2][8] = pc_luma_samples[6], q[3][8] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][9] = pc_luma_samples[0], p[2][9] = pc_luma_samples[1], p[1][9] = pc_luma_samples[2], p[0][9] = pc_luma_samples[3];
    q[0][9] = pc_luma_samples[4], q[1][9] = pc_luma_samples[5], q[2][9] = pc_luma_samples[6], q[3][9] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][10] = pc_luma_samples[0], p[2][10] = pc_luma_samples[1], p[1][10] = pc_luma_samples[2], p[0][10] = pc_luma_samples[3];
    q[0][10] = pc_luma_samples[4], q[1][10] = pc_luma_samples[5], q[2][10] = pc_luma_samples[6], q[3][10] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][11] = pc_luma_samples[0], p[2][11] = pc_luma_samples[1], p[1][11] = pc_luma_samples[2], p[0][11] = pc_luma_samples[3];
    q[0][11] = pc_luma_samples[4], q[1][11] = pc_luma_samples[5], q[2][11] = pc_luma_samples[6], q[3][11] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][12] = pc_luma_samples[0], p[2][12] = pc_luma_samples[1], p[1][12] = pc_luma_samples[2], p[0][12] = pc_luma_samples[3];
    q[0][12] = pc_luma_samples[4], q[1][12] = pc_luma_samples[5], q[2][12] = pc_luma_samples[6], q[3][12] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][13] = pc_luma_samples[0], p[2][13] = pc_luma_samples[1], p[1][13] = pc_luma_samples[2], p[0][13] = pc_luma_samples[3];
    q[0][13] = pc_luma_samples[4], q[1][13] = pc_luma_samples[5], q[2][13] = pc_luma_samples[6], q[3][13] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][14] = pc_luma_samples[0], p[2][14] = pc_luma_samples[1], p[1][14] = pc_luma_samples[2], p[0][14] = pc_luma_samples[3];
    q[0][14] = pc_luma_samples[4], q[1][14] = pc_luma_samples[5], q[2][14] = pc_luma_samples[6], q[3][14] = pc_luma_samples[7];
    pc_luma_samples += u_luma_stride;
    p[3][15] = pc_luma_samples[0], p[2][15] = pc_luma_samples[1], p[1][15] = pc_luma_samples[2], p[0][15] = pc_luma_samples[3];
    q[0][15] = pc_luma_samples[4], q[1][15] = pc_luma_samples[5], q[2][15] = pc_luma_samples[6], q[3][15] = pc_luma_samples[7];
}

#endif /* LOAD_VERT */

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_CODEC_X86_264_DEBLOCK_INTRIN_H_ */