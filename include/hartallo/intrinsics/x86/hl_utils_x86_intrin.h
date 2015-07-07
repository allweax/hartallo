#ifndef _HARTALLO_UTILS_X86_INTRIN_H_
#define _HARTALLO_UTILS_X86_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0[4];

#if !defined(_MM_SET1_EPI32_SSE41)
#	define _MM_SET1_EPI32_SSE41(v) _mm_shuffle_epi8(_mm_insert_epi32(_mm_setzero_si128(), (v), 0), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_0_0))
//#	define _MM_SET1_EPI32_SSE41(v) _mm_set1_epi32((v))
#endif /* _MM_SET1_EPI32_SSE41 */

#if !defined(_MM_SET1_EPI16_SSSE3)
#	define _MM_SET1_EPI16_SSSE3(v) _mm_shuffle_epi8(_mm_insert_epi16(_mm_setzero_si128(), (v), 0), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_0_0_0_0_0_0))
//#	define _MM_SET1_EPI16_SSSE3(v) _mm_set1_epi16((v))
#endif /* _MM_SET1_EPI16_SSSE3 */

#if !defined(_MM_SET_EPI32_SSE41)
#	define _MM_SET_EPI32_SSE41(d, c, b, a) _mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_insert_epi32(_mm_setzero_si128(), (a), 0), (b), 1), (c), 2), (d), 3)
// #	define _MM_SET_EPI32_SSE41(d, c, b, a) _mm_set_epi32((d), (c), (b), (a))
#endif /* _MM_SET_EPI32_SSE41 */

#if !defined(_MM_SET_EPI16_SSE41)
#	define _MM_SET_EPI16_SSE41(h, g, f, e, d, c, b, a) _MM_SET_EPI32_SSE41(((h) << 16)|(g), ((f) << 16)|(e), ((d) << 16)|(c), ((b) << 16)|(a))
// #	define _MM_SET_EPI16_SSE41(h, g, f, e, d, c, b, a) _mm_set_epi16((h), (g), (f), (e), (d), (c), (b), (a))
#endif

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_UTILS_X86_INTRIN_H_ */

