#ifndef _HARTALLO_MATH_X86_INTRIN_H_
#define _HARTALLO_MATH_X86_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_math.h"

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

#define HL_MATH_CLIP3_INTRIN_SSE41(min4, max4, val4) \
	_mm_min_epi32(_mm_max_epi32((val4), (min4)), (max4))

#define HL_MATH_TRANSPOSE4X1_EPI32_SSE2(row0, row1, row2, row3) { \
    __m128 __tmp3, __tmp2, __tmp1, __tmp0;                          \
    __tmp0 = _mm_shuffle_ps(_mm_cvtepi32_ps((row0)), _mm_cvtepi32_ps((row1)), 0x44);          \
    __tmp2 = _mm_shuffle_ps(_mm_cvtepi32_ps((row0)), _mm_cvtepi32_ps((row1)), 0xEE);          \
    __tmp1 = _mm_shuffle_ps(_mm_cvtepi32_ps((row2)), _mm_cvtepi32_ps((row3)), 0x44);          \
    __tmp3 = _mm_shuffle_ps(_mm_cvtepi32_ps((row2)), _mm_cvtepi32_ps((row3)), 0xEE);          \
																								  \
    _mm_store_si128((__m128i*)&(row0), _mm_cvtps_epi32(_mm_shuffle_ps(__tmp0, __tmp1, 0x88)));							  \
    _mm_store_si128((__m128i*)&(row1), _mm_cvtps_epi32(_mm_shuffle_ps(__tmp0, __tmp1, 0xDD)));							  \
    _mm_store_si128((__m128i*)&(row2), _mm_cvtps_epi32(_mm_shuffle_ps(__tmp2, __tmp3, 0x88)));							  \
    _mm_store_si128((__m128i*)&(row3), _mm_cvtps_epi32(_mm_shuffle_ps(__tmp2, __tmp3, 0xDD)));							  \
}

#define HL_MATH_TRANSPOSE4X4_EPI32_SSE2(in4x4, out4x4) \
{ \
	__m128i _row0 = _mm_load_si128((__m128i*)(in4x4)[0]); /* a1 a2 a3 a4 */\
	__m128i _row1 = _mm_load_si128((__m128i*)(in4x4)[1]); /* b1 b2 b3 b4 */\
	__m128i _row2 = _mm_load_si128((__m128i*)(in4x4)[2]); /* c1 c2 c3 c4 */\
	__m128i _row3 = _mm_load_si128((__m128i*)(in4x4)[3]); /* d1 d2 d3 d4 */\
 \
	HL_MATH_TRANSPOSE4X1_EPI32_SSE2(_row0, _row1, _row2, _row3); \
 \
	_mm_store_si128((__m128i*)out4x4[0], _row0); \
	_mm_store_si128((__m128i*)out4x4[1], _row1); \
	_mm_store_si128((__m128i*)out4x4[2], _row2); \
	_mm_store_si128((__m128i*)out4x4[3], _row3); \
}

// "_mm_mullo_epi32(a, b)" implementation using SSE2 only
#if 1
#define hl_mm_mullo_epi32_sse2(a, b) \
	_mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps((a)), _mm_cvtepi32_ps((b))))
#else
#define hl_mm_mullo_epi32_sse2(a, b) \
	_mm_unpacklo_epi32(_mm_shuffle_epi32(_mm_mul_epu32((a), (b)), _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(_mm_mul_epu32(_mm_srli_si128(a,4), _mm_srli_si128((b), 4)), _MM_SHUFFLE (0,0,2,0)))
#endif

#define hl_mm_clip3_epi16_sse2(min, max, val) \
	_mm_min_epi16(_mm_max_epi16((val), (min)), (max))

// r(int32_t) = ((a0 * b0) + (a1 * b1) + (a2 * b2) + (a3 * b3))
#define HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41(a, b, r) \
	*(r) = _mm_cvtss_si32(_mm_dp_ps(_mm_cvtepi32_ps(_mm_load_si128((__m128i*)(a))), _mm_cvtepi32_ps(_mm_load_si128((__m128i*)(b))), 0xF1))
#define HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3(a, b, r) \
	{ \
		__m128i __tmp__ = hl_mm_mullo_epi32_sse2(_mm_load_si128((__m128i*)(a)), _mm_load_si128((__m128i*)(b))); \
		_mm_store_si128(&__tmp__,  _mm_hadd_epi32(__tmp__, __tmp__)); \
		_mm_store_si128(&__tmp__, _mm_hadd_epi32(__tmp__, __tmp__)); \
		*r = ((int32_t*)&__tmp__)[0]; \
	}
// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_math_dot_product4x1_intrin_sse41(HL_ALIGNED(16) const int32_t a[4], HL_ALIGNED(16) const int32_t b[4], int32_t* r)
{
    HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41(a, b, r);
}
// TODO: add ASM version
HL_ALWAYS_INLINE static void hl_math_dot_product4x1_intrin_ssse3(HL_ALIGNED(16) const int32_t a[4], HL_ALIGNED(16) const int32_t b[4], int32_t* r)
{
    HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3(a, b, r);
}

#define HL_MATH_MUL4X4_INTRIN_SSSE3(block1, block2, out) \
{ \
	HL_ALIGN(HL_ALIGN_V) int32_t block2T[4][4]; \
	HL_MATH_TRANSPOSE4X4_EPI32_SSE2((block2), block2T); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[0], block2T[0], &(out)[0][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[0], block2T[1], &(out)[0][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[0], block2T[2], &(out)[0][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[0], block2T[3], &(out)[0][3]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[1], block2T[0], &(out)[1][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[1], block2T[1], &(out)[1][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[1], block2T[2], &(out)[1][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[1], block2T[3], &(out)[1][3]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[2], block2T[0], &(out)[2][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[2], block2T[1], &(out)[2][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[2], block2T[2], &(out)[2][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[2], block2T[3], &(out)[2][3]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[3], block2T[0], &(out)[3][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[3], block2T[1], &(out)[3][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[3], block2T[2], &(out)[3][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSSE3((block1)[3], block2T[3], &(out)[3][3]); \
}

#define HL_MATH_MUL4X4_INTRIN_SSE41(block1, block2, out) \
{ \
	HL_ALIGN(HL_ALIGN_V) int32_t block2T[4][4]; \
	HL_MATH_TRANSPOSE4X4_EPI32_SSE2((block2), block2T); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[0], block2T[0], &(out)[0][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[0], block2T[1], &(out)[0][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[0], block2T[2], &(out)[0][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[0], block2T[3], &(out)[0][3]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[1], block2T[0], &(out)[1][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[1], block2T[1], &(out)[1][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[1], block2T[2], &(out)[1][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[1], block2T[3], &(out)[1][3]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[2], block2T[0], &(out)[2][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[2], block2T[1], &(out)[2][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[2], block2T[2], &(out)[2][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[2], block2T[3], &(out)[2][3]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[3], block2T[0], &(out)[3][0]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[3], block2T[1], &(out)[3][1]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[3], block2T[2], &(out)[3][2]); \
	HL_MATH_DOT_PRODUCT4X1_INTRIN_SSE41((block1)[3], block2T[3], &(out)[3][3]); \
}

HL_ALWAYS_INLINE static void hl_math_mul4x4_intrin_ssse3_or_41(HL_ALIGNED(16) const int32_t a[4][4], HL_ALIGNED(16) const int32_t b[4][4], HL_ALIGNED(16) int32_t r[4][4])
{
    HL_ALIGN(HL_ALIGN_V) int32_t bT[4][4];
    HL_MATH_TRANSPOSE4X4_EPI32_SSE2((b), bT);
    hl_math_dot_product4x1((a)[0], bT[0], &(r)[0][0]);
    hl_math_dot_product4x1((a)[0], bT[1], &(r)[0][1]);
    hl_math_dot_product4x1((a)[0], bT[2], &(r)[0][2]);
    hl_math_dot_product4x1((a)[0], bT[3], &(r)[0][3]);
    hl_math_dot_product4x1((a)[1], bT[0], &(r)[1][0]);
    hl_math_dot_product4x1((a)[1], bT[1], &(r)[1][1]);
    hl_math_dot_product4x1((a)[1], bT[2], &(r)[1][2]);
    hl_math_dot_product4x1((a)[1], bT[3], &(r)[1][3]);
    hl_math_dot_product4x1((a)[2], bT[0], &(r)[2][0]);
    hl_math_dot_product4x1((a)[2], bT[1], &(r)[2][1]);
    hl_math_dot_product4x1((a)[2], bT[2], &(r)[2][2]);
    hl_math_dot_product4x1((a)[2], bT[3], &(r)[2][3]);
    hl_math_dot_product4x1((a)[3], bT[0], &(r)[3][0]);
    hl_math_dot_product4x1((a)[3], bT[1], &(r)[3][1]);
    hl_math_dot_product4x1((a)[3], bT[2], &(r)[3][2]);
    hl_math_dot_product4x1((a)[3], bT[3], &(r)[3][3]);
}

HL_ALWAYS_INLINE static hl_bool_t hl_math_allzero16_intrin_sse41(HL_ALIGNED(16) const int32_t (*block)[16])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_one_bits[4];
    static __m128i __x86_data_math_m128i_one_bits;
    static hl_bool_t __blockOnesSet = HL_FALSE;
    if (!__blockOnesSet) {
        __x86_data_math_m128i_one_bits = _mm_load_si128((__m128i*)__x86_globals_array4_one_bits);
        __blockOnesSet = HL_TRUE;
    }
    return _mm_testz_si128(_mm_load_si128((__m128i*)&(*block)[0]), __x86_data_math_m128i_one_bits) &&
           _mm_testz_si128(_mm_load_si128((__m128i*)&(*block)[4]), __x86_data_math_m128i_one_bits) &&
           _mm_testz_si128(_mm_load_si128((__m128i*)&(*block)[8]), __x86_data_math_m128i_one_bits) &&
           _mm_testz_si128(_mm_load_si128((__m128i*)&(*block)[12]), __x86_data_math_m128i_one_bits);
}

HL_ALWAYS_INLINE static hl_bool_t hl_math_allzero16_intrin_sse2(HL_ALIGNED(16) const int32_t (*block)[16])
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_zeros[4];
    static __m128i __x86_data_math_m128i_zeros;
    static hl_bool_t __blockZerosSet = HL_FALSE;
    if (!__blockZerosSet) {
#if 0
        __x86_data_math_m128i_zeros = _mm_setzero_si128();
#else
        __x86_data_math_m128i_zeros = _mm_load_si128((__m128i*)__x86_globals_array4_zeros);
#endif
        __blockZerosSet = HL_TRUE;
    }
    return (_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i*)&(*block)[0]), __x86_data_math_m128i_zeros)) == 0xFFFF) &&
           (_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i*)&(*block)[4]), __x86_data_math_m128i_zeros)) == 0xFFFF) &&
           (_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i*)&(*block)[8]), __x86_data_math_m128i_zeros)) == 0xFFFF) &&
           (_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i*)&(*block)[12]), __x86_data_math_m128i_zeros)) == 0xFFFF);
}

// Multiplies the 16 signed or unsigned 8-bit integers from "a" by the 16 signed or unsigned 8-bit integers from "b".
HL_ALWAYS_INLINE static __m128i hl_math_mullo_epi8_intrin_sse3(
    __m128i a, // 16x8b
    __m128i b // 16x8b
)
{
    // _mm_mullo_epi8 not supported by any SSE version
    __m128i _zero, a_lo, a_hi, b_lo, b_hi;
    _mm_store_si128(&_zero, _mm_setzero_si128());
    _mm_store_si128(&a_lo, _mm_unpacklo_epi8((a), _zero));
    _mm_store_si128(&a_hi, _mm_unpackhi_epi8((a), _zero));
    _mm_store_si128(&b_lo, _mm_unpacklo_epi8((b), _zero));
    _mm_store_si128(&b_hi, _mm_unpackhi_epi8((b), _zero));
    // Packs the 16 signed 16-bit integers from "tmp0" and "tmp1" into 8-bit unsigned integers and saturates.
    return _mm_packus_epi16(_mm_mullo_epi16(a_lo, b_lo), _mm_mullo_epi16(a_hi, b_hi));
}

// Partial
// Used in MPEG codecs (e.g. H.264)
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// Each parameter contains 4x32b packed values
HL_ALWAYS_INLINE static void hl_math_tap6filter4x1_u32_intrin_sse41(
    const hl_int128_t* e,
    const hl_int128_t* f,
    const hl_int128_t* g,
    const hl_int128_t* h,
    const hl_int128_t* i,
    const hl_int128_t* j,
    hl_int128_t* ret)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_fives[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_twenties[4];
    _mm_store_si128(ret, _mm_add_epi32(
                        _mm_add_epi32(_mm_sub_epi32(_mm_load_si128(e), _mm_mullo_epi32(_mm_load_si128((__m128i*)__x86_globals_array4_fives), _mm_add_epi32(_mm_load_si128(f), _mm_load_si128(i)))),
                                      _mm_mullo_epi32(_mm_load_si128((__m128i*)__x86_globals_array4_twenties), _mm_add_epi32(_mm_load_si128(g), _mm_load_si128(h)))), _mm_load_si128(j)));
}

// Full (FIXME: No ASM version)
HL_ALWAYS_INLINE static void hl_math_tap6filter4x1_u32_intrin_sse2(
    const hl_int128_t* e,
    const hl_int128_t* f,
    const hl_int128_t* g,
    const hl_int128_t* h,
    const hl_int128_t* i,
    const hl_int128_t* j,
    hl_int128_t* ret)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_fives[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_twenties[4];
    _mm_store_si128(ret, _mm_add_epi32(
                        _mm_add_epi32(_mm_sub_epi32(_mm_load_si128(e), hl_mm_mullo_epi32_sse2(_mm_load_si128((__m128i*)__x86_globals_array4_fives), _mm_add_epi32(_mm_load_si128(f), _mm_load_si128(i)))),
                                      hl_mm_mullo_epi32_sse2(_mm_load_si128((__m128i*)__x86_globals_array4_twenties), _mm_add_epi32(_mm_load_si128(g), _mm_load_si128(h)))), _mm_load_si128(j)));
}

// FIXME: SSE2 (don't forget to move ASM version also)
// Full operation
// Used in MPEG codecs (e.g. H.264)
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Clip(((Tap6Filter + 16) >> 5))
// Each parameter contains 16x8b packed values
HL_ALWAYS_INLINE static void hl_math_tap6filter4x4_u8_full_intrin_sse3(
    const hl_int128_t* e, // 16x8b
    const hl_int128_t* f, // 16x8b
    const hl_int128_t* g, // 16x8b
    const hl_int128_t* h, // 16x8b
    const hl_int128_t* i, // 16x8b
    const hl_int128_t* j, // 16x8b
    hl_int128_t* ret	  // 16x8b
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fives[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_twenties[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_sixteens[4];

    __m128i _e, _f, _g, _h, _i, _j, _zero, ret_lo, ret_hi;

    _mm_store_si128(&_zero, _mm_setzero_si128());

    _mm_store_si128(&_e, _mm_unpacklo_epi8(_mm_load_si128(e), _zero));
    _mm_store_si128(&_f, _mm_unpacklo_epi8(_mm_load_si128(f), _zero));
    _mm_store_si128(&_g, _mm_unpacklo_epi8(_mm_load_si128(g), _zero));
    _mm_store_si128(&_h, _mm_unpacklo_epi8(_mm_load_si128(h), _zero));
    _mm_store_si128(&_i, _mm_unpacklo_epi8(_mm_load_si128(i), _zero));
    _mm_store_si128(&_j, _mm_unpacklo_epi8(_mm_load_si128(j), _zero));

    // RET_LO = TAP6FILTER(E,F,G,H,I,J)
    _mm_store_si128(&ret_lo, _mm_add_epi16(
                        _mm_add_epi16(_mm_sub_epi16(_e, _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_fives), _mm_add_epi16 (_f, _i))),
                                      _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_twenties), _mm_add_epi16(_g, _h))), _j));

    _mm_store_si128(&_e, _mm_unpackhi_epi8(_mm_load_si128(e), _zero));
    _mm_store_si128(&_f, _mm_unpackhi_epi8(_mm_load_si128(f), _zero));
    _mm_store_si128(&_g, _mm_unpackhi_epi8(_mm_load_si128(g), _zero));
    _mm_store_si128(&_h, _mm_unpackhi_epi8(_mm_load_si128(h), _zero));
    _mm_store_si128(&_i, _mm_unpackhi_epi8(_mm_load_si128(i), _zero));
    _mm_store_si128(&_j, _mm_unpackhi_epi8(_mm_load_si128(j), _zero));

    // RET_HI = TAP6FILTER(E,F,G,H,I,J)
    _mm_store_si128(&ret_hi, _mm_add_epi16(
                        _mm_add_epi16(_mm_sub_epi16(_e, _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_fives), _mm_add_epi16 (_f, _i))),
                                      _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_twenties), _mm_add_epi16(_g, _h))), _j));

    // RET_LO/HI = ((RET + 16) >> 5)
    _mm_store_si128(&ret_lo, _mm_srai_epi16(_mm_add_epi16(ret_lo, _mm_load_si128((__m128i*)__x86_globals_array8_sixteens)), 5));
    _mm_store_si128(&ret_hi, _mm_srai_epi16(_mm_add_epi16(ret_hi, _mm_load_si128((__m128i*)__x86_globals_array8_sixteens)), 5));

    // Packs the 16 signed 16-bit integers from "ret_lo" and "ret_hi" into 8-bit unsigned integers and saturates.
    // Saturate = Clip(0, 255, val)
    _mm_store_si128(ret, _mm_packus_epi16(ret_lo, ret_hi));
}

// FIXME: SSE2 (don't forget to move ASM version also)
// Partial operation (version 1)
// Used in MPEG codecs (e.g. H.264)
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Tap6Filter without clip and shift
// Each input parameter contains 16x8b packed values
// Ret parameter contains 2 8x16b packed values
HL_ALWAYS_INLINE static void hl_math_tap6filter4x4_u8_partial_intrin_sse3(
    const hl_int128_t* e, // 16x8b
    const hl_int128_t* f, // 16x8b
    const hl_int128_t* g, // 16x8b
    const hl_int128_t* h, // 16x8b
    const hl_int128_t* i, // 16x8b
    const hl_int128_t* j, // 16x8b
    hl_int128_t* ret_lo,  // 8x16b
    hl_int128_t* ret_hi	  // 8x16b
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_fives[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_twenties[4];

    __m128i _e, _f, _g, _h, _i, _j, _zero;

    _mm_store_si128(&_zero, _mm_setzero_si128());

    _mm_store_si128(&_e, _mm_unpacklo_epi8(_mm_load_si128(e), _zero));
    _mm_store_si128(&_f, _mm_unpacklo_epi8(_mm_load_si128(f), _zero));
    _mm_store_si128(&_g, _mm_unpacklo_epi8(_mm_load_si128(g), _zero));
    _mm_store_si128(&_h, _mm_unpacklo_epi8(_mm_load_si128(h), _zero));
    _mm_store_si128(&_i, _mm_unpacklo_epi8(_mm_load_si128(i), _zero));
    _mm_store_si128(&_j, _mm_unpacklo_epi8(_mm_load_si128(j), _zero));

    // RET = TAP6FILTER(E,F,G,H,I,J)
    _mm_store_si128(ret_lo, _mm_add_epi16(
                        _mm_add_epi16 (_mm_sub_epi16(_e, _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_fives), _mm_add_epi16 (_f, _i))),
                                       _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_twenties), _mm_add_epi16 (_g, _h))), _j));

    _mm_store_si128(&_e, _mm_unpackhi_epi8(_mm_load_si128(e), _zero));
    _mm_store_si128(&_f, _mm_unpackhi_epi8(_mm_load_si128(f), _zero));
    _mm_store_si128(&_g, _mm_unpackhi_epi8(_mm_load_si128(g), _zero));
    _mm_store_si128(&_h, _mm_unpackhi_epi8(_mm_load_si128(h), _zero));
    _mm_store_si128(&_i, _mm_unpackhi_epi8(_mm_load_si128(i), _zero));
    _mm_store_si128(&_j, _mm_unpackhi_epi8(_mm_load_si128(j), _zero));
    _mm_store_si128(ret_hi, _mm_add_epi16(
                        _mm_add_epi16 (_mm_sub_epi16(_e, _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_fives), _mm_add_epi16 (_f, _i))),
                                       _mm_mullo_epi16(_mm_load_si128((__m128i*)__x86_globals_array8_twenties), _mm_add_epi16 (_g, _h))), _j));
}

// FIXME: SSE2 (don't forget to move ASM version also)
// Partial operation (version 2)
// Used in MPEG codecs (e.g. H.264)
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Tap6Filter without clip and shift
// Each input parameter contains 8x16b packed values
// Ret parameter contains 2 8x16b packed values
HL_ALWAYS_INLINE static void hl_math_tap6filter4x2_u16_partial_intrin_sse3(
    const hl_int128_t* e, // 8x16b
    const hl_int128_t* f, // 8x16b
    const hl_int128_t* g, // 8x16b
    const hl_int128_t* h, // 8x16b
    const hl_int128_t* i, // 8x16b
    const hl_int128_t* j, // 8x16b
    hl_int128_t* ret_lo,  // 4x32b
    hl_int128_t* ret_hi  // 4x32b
)
{
    __m128i _e, _f, _g, _h, _i, _j, _zero;

    _mm_store_si128(&_zero, _mm_setzero_si128());

    _mm_store_si128(&_e, _mm_unpacklo_epi16(_mm_load_si128(e), _zero));
    _mm_store_si128(&_f, _mm_unpacklo_epi16(_mm_load_si128(f), _zero));
    _mm_store_si128(&_g, _mm_unpacklo_epi16(_mm_load_si128(g), _zero));
    _mm_store_si128(&_h, _mm_unpacklo_epi16(_mm_load_si128(h), _zero));
    _mm_store_si128(&_i, _mm_unpacklo_epi16(_mm_load_si128(i), _zero));
    _mm_store_si128(&_j, _mm_unpacklo_epi16(_mm_load_si128(j), _zero));
    hl_math_tap6filter4x1_u32(&_e, &_f, &_g, &_h, &_i, &_j, ret_lo);

    _mm_store_si128(&_e, _mm_unpackhi_epi16(_mm_load_si128(e), _zero));
    _mm_store_si128(&_f, _mm_unpackhi_epi16(_mm_load_si128(f), _zero));
    _mm_store_si128(&_g, _mm_unpackhi_epi16(_mm_load_si128(g), _zero));
    _mm_store_si128(&_h, _mm_unpackhi_epi16(_mm_load_si128(h), _zero));
    _mm_store_si128(&_i, _mm_unpackhi_epi16(_mm_load_si128(i), _zero));
    _mm_store_si128(&_j, _mm_unpackhi_epi16(_mm_load_si128(j), _zero));
    hl_math_tap6filter4x1_u32(&_e, &_f, &_g, &_h, &_i, &_j, ret_hi);
}

// Used in MPEG codecs (e.g. H.264)
// Clip3
// /!\ IMPORTANT: requires min <= max otherwise result will be different than HL_MATH_CLIP3(x,y,z). Should never happen.
HL_ALWAYS_INLINE static void hl_math_clip3_4x1_intrin_sse41(
    const hl_int128_t* min,
    const hl_int128_t* max,
    const hl_int128_t* val,
    __m128i* ret)
{
    _mm_store_si128(ret, _mm_min_epi32(_mm_max_epi32(_mm_load_si128(val), _mm_load_si128(min)), _mm_load_si128(max)));
}

// Used in MPEG codecs (e.g. H.264)
// Clip2(max, val) = Clip3(0, max, val)
HL_ALWAYS_INLINE static void hl_math_clip2_4x1_intrin_sse41(
    const hl_int128_t* max,
    const hl_int128_t* val,
    hl_int128_t* ret)
{
    //hl_math_clip3_4x1_intrin_sse41((__m128i*)__x86_globals_array4_zeros, max, val, ret);
    _mm_store_si128(ret, _mm_min_epi32(_mm_max_epi32(_mm_load_si128(val), _mm_setzero_si128()), _mm_load_si128(max)));
}

// Used in MPEG codecs (e.g. H.264)
// Clip1Y(x, BitDepth) / Clip1C(x, BitDepth) = Clip3(0, (1 << BitDepth) - 1, x) = Clip2((1 << (BitDepth)) - 1, x) = Clip2(MaxPixelVal, x)
// /!\ IMPORTANT: This function will compute max pixel value ((1 << (BitDepth)) - 1) which take CPU cyles.
//		It's highly recommended to do it yourself and call "hl_math_clip3_4x1_intrin_sse41()" or "hl_math_clip2_4x1_intrin_sse41()" instead of this function.
// /!\ IMPORTANT: BitDepth only first BitDepth value will be used (means 4 values are expected to be equal).
//		An Int128 is used because "PSLLD" instriction (_mm_slli_epi32 in intrinsic) requires an immediate value or SSE/MMX register: http://www.rz.uni-karlsruhe.de/rz/docs/VTune/reference/vc256.htm
//		The first 3 dwords in the Int128 must be zeros. For example BitDepth 8 = [ 8, 0, 0, 0 ]
HL_ALWAYS_INLINE static void hl_math_clip1_4x1_intrin_sse41(
    const hl_int128_t* val,
    const hl_int128_t* BitDepth, // BitDepthY or BitDepthC
    hl_int128_t* ret)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_zeros[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

    __m128i max = _mm_slli_epi32(_mm_load_si128((__m128i*)__x86_globals_array4_ones), *((int32_t*)BitDepth)); // max = (1 << BitDepth)
    _mm_store_si128(&max, _mm_sub_epi32(max, _mm_load_si128((__m128i*)__x86_globals_array4_ones))); // max = ((1 << BitDepth) - 1)
    hl_math_clip3_4x1_intrin_sse41((__m128i*)__x86_globals_array4_zeros, &max, val, ret);
}

// RET = Clip2(MAX, (M1 + M2))
// FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
// Strides must be multiple of "4"
HL_ALWAYS_INLINE static void hl_math_addclip_4x4_intrin_sse41(
    HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */
    HL_ALIGNED(16) int32_t* ret, int32_t ret_stride
)
{
    __m128i max4 = _mm_load_si128((__m128i*)max);

    _mm_store_si128((__m128i*)ret, _mm_add_epi32(_mm_load_si128((__m128i*)m1), _mm_load_si128((__m128i*)m2)));
    hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)ret, (hl_int128_t*)ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _mm_store_si128((__m128i*)ret, _mm_add_epi32(_mm_load_si128((__m128i*)m1), _mm_load_si128((__m128i*)m2)));
    hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)ret, (hl_int128_t*)ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _mm_store_si128((__m128i*)ret, _mm_add_epi32(_mm_load_si128((__m128i*)m1), _mm_load_si128((__m128i*)m2)));
    hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)ret, (hl_int128_t*)ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _mm_store_si128((__m128i*)ret, _mm_add_epi32(_mm_load_si128((__m128i*)m1), _mm_load_si128((__m128i*)m2)));
    hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)ret, (hl_int128_t*)ret);
}

// RET = Clip2(MAX, (M1 + M2))
// FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
// Strides must be multiple of "4" and more than 8
HL_ALWAYS_INLINE static void hl_math_addclip_8x8_intrin_sse41(
    HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */
    HL_ALIGNED(16) int32_t* ret, int32_t ret_stride
)
{
    int32_t i;
    __m128i max4 = _mm_load_si128((__m128i*)max);
    for (i = 0; i < 8; ++i, ret += ret_stride, m1 += m1_stride, m2 += m2_stride) {
        _mm_store_si128((__m128i*)ret, _mm_add_epi32(_mm_load_si128((__m128i*)m1), _mm_load_si128((__m128i*)m2)));
        hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)ret, (hl_int128_t*)ret);
        _mm_store_si128((__m128i*)&ret[4], _mm_add_epi32(_mm_load_si128((__m128i*)&m1[4]), _mm_load_si128((__m128i*)&m2[4])));
        hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)&ret[4], (hl_int128_t*)&ret[4]);
    }
}

// RET = Clip2(MAX, (M1 + M2))
// FIXME: Use saturate add if max val is "0xFF" or "0xFFFF" -> "paddsb" -> "paddsw"
// Strides must be multiple of "4" and more than 16
HL_ALWAYS_INLINE static void hl_math_addclip_16x16_intrin_sse41(
    HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) const int32_t max[4], /* Should contains same value 4 times */
    HL_ALIGNED(16) int32_t* ret, int32_t ret_stride
)
{
    int32_t i;
    __m128i max4 = _mm_load_si128((__m128i*)max);
    for (i = 0; i < 16; ++i, ret += ret_stride, m1 += m1_stride, m2 += m2_stride) {
        _mm_store_si128((__m128i*)ret, _mm_add_epi32(_mm_load_si128((__m128i*)m1), _mm_load_si128((__m128i*)m2)));
        hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)ret, (hl_int128_t*)ret);
        _mm_store_si128((__m128i*)&ret[4], _mm_add_epi32(_mm_load_si128((__m128i*)&m1[4]), _mm_load_si128((__m128i*)&m2[4])));
        hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)&ret[4], (hl_int128_t*)&ret[4]);
        _mm_store_si128((__m128i*)&ret[8], _mm_add_epi32(_mm_load_si128((__m128i*)&m1[8]), _mm_load_si128((__m128i*)&m2[8])));
        hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)&ret[8], (hl_int128_t*)&ret[8]);
        _mm_store_si128((__m128i*)&ret[12], _mm_add_epi32(_mm_load_si128((__m128i*)&m1[12]), _mm_load_si128((__m128i*)&m2[12])));
        hl_math_clip2_4x1_intrin_sse41(&max4, (const hl_int128_t*)&ret[12], (hl_int128_t*)&ret[12]);
    }
}

HL_ALWAYS_INLINE static void hl_math_addclip_4x4_u8xi32_intrin_sse41(
    HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride,
    HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,
    HL_ALIGNED(16) uint8_t* ret, int32_t ret_stride
)
{
	__m128i xmm_row0, xmm_row1, xmm_row2, xmm_row3, xmm_zero;
	_mm_store_si128(&xmm_zero, _mm_setzero_si128());
	
	_mm_store_si128(&xmm_row0, _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)m1)), xmm_zero), xmm_zero)); // i32 = ConvertToI32(m1_u8)
	_mm_store_si128(&xmm_row0, _mm_add_epi32(xmm_row0, _mm_load_si128((__m128i*)m2)));
	
	m1 += m1_stride, m2 += m2_stride;
	_mm_store_si128(&xmm_row1, _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)m1)), xmm_zero), xmm_zero)); // i32 = ConvertToI32(m1_u8)
	_mm_store_si128(&xmm_row1, _mm_add_epi32(xmm_row1, _mm_load_si128((__m128i*)m2)));
	
	m1 += m1_stride, m2 += m2_stride;
	_mm_store_si128(&xmm_row2, _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)m1)), xmm_zero), xmm_zero)); // i32 = ConvertToI32(m1_u8)
	_mm_store_si128(&xmm_row2, _mm_add_epi32(xmm_row2, _mm_load_si128((__m128i*)m2)));
	
	m1 += m1_stride, m2 += m2_stride;
	_mm_store_si128(&xmm_row3, _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)m1)), xmm_zero), xmm_zero)); // i32 = ConvertToI32(m1_u8)
	_mm_store_si128(&xmm_row3, _mm_add_epi32(xmm_row3, _mm_load_si128((__m128i*)m2)));
	
	// ret_u8 = ConvertToU8(Clip(0, u32, 255))
	_mm_store_si128(&xmm_row0, _mm_packus_epi32(xmm_row0, xmm_row1)); // Saturate_Int32_To_UnsignedInt16(v)
    _mm_store_si128(&xmm_row2, _mm_packus_epi32(xmm_row2, xmm_row3)); // Saturate_Int32_To_UnsignedInt16(v)
    _mm_store_si128(&xmm_row0, _mm_packus_epi16(xmm_row0, xmm_row2)); // Saturate_Int16_To_UnsignedInt8(v)

	if (ret_stride == 4) {
		_mm_store_si128((__m128i*)ret, xmm_row0);
	}
	else {
		*((int32_t*)ret) = _mm_cvtsi128_si32(xmm_row0); ret += ret_stride;
		*((int32_t*)ret) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_row0, 4)); ret += ret_stride;
		*((int32_t*)ret) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_row0, 8)); ret += ret_stride;
		*((int32_t*)ret) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_row0, 12));
	}
}

HL_ALWAYS_INLINE static int32_t hl_math_sad4x4_u8_intrin_sse2(const uint8_t* b1, int32_t b1_stride, const uint8_t* b2, int32_t b2_stride)
{
    const uint8_t* r0 = b1, *r1 = (r0 + b1_stride), *r2 = (r1 + b1_stride), *r3 = (r2 + b1_stride);
    __m128i xmm0, xmm1;
    _mm_store_si128(&xmm0, _mm_set_epi8(
                        r3[3], r3[2], r3[1], r3[0],
                        r2[3], r2[2], r2[1], r2[0],
                        r1[3], r1[2], r1[1], r1[0],
                        r0[3], r0[2], r0[1], r0[0]));
    r0 = b2, r1 = (r0 + b2_stride), r2 = (r1 + b2_stride), r3 = (r2 + b2_stride);
    _mm_store_si128(&xmm1, _mm_set_epi8(
                        r3[3], r3[2], r3[1], r3[0],
                        r2[3], r2[2], r2[1], r2[0],
                        r1[3], r1[2], r1[1], r1[0],
                        r0[3], r0[2], r0[1], r0[0]));

    _mm_store_si128(&xmm0, _mm_sad_epu8(xmm0, xmm1));
    return _mm_cvtsi128_si32(xmm0) + _mm_cvtsi128_si32(_mm_srli_si128(xmm0, 8));
}

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_MATH_X86_INTRIN_H_ */

