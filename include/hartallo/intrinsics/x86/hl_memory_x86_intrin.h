#ifndef _HARTALLO_MEMORY_X86_INTRIN_H_
#define _HARTALLO_MEMORY_X86_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

#include "hartallo/hl_memory.h" // FIXME: remove

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

HL_ALWAYS_INLINE static void hl_memory_copy4x4_intrin_sse2(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride)
{
    _mm_store_si128((__m128i*)p_dst, _mm_load_si128((__m128i*)pc_src));
    _mm_store_si128((__m128i*)&p_dst[dst_stride], _mm_load_si128((__m128i*)&pc_src[src_stride]));
    _mm_store_si128((__m128i*)&p_dst[dst_stride << 1], _mm_load_si128((__m128i*)&pc_src[src_stride << 1]));
    _mm_store_si128((__m128i*)&p_dst[(dst_stride << 1) + dst_stride], _mm_load_si128((__m128i*)&pc_src[(src_stride << 1) + src_stride]));
}

HL_ALWAYS_INLINE static void hl_memory_copy4x4_unaligned_intrin_sse2(int32_t *p_dst, hl_size_t dst_stride, const int32_t *pc_src, hl_size_t src_stride)
{
    _mm_storeu_si128((__m128i*)p_dst, _mm_loadu_si128 ((__m128i*)pc_src));
    _mm_storeu_si128((__m128i*)&p_dst[dst_stride], _mm_loadu_si128 ((__m128i*)&pc_src[src_stride]));
    _mm_storeu_si128((__m128i*)&p_dst[dst_stride << 1], _mm_loadu_si128 ((__m128i*)&pc_src[src_stride << 1]));
    _mm_storeu_si128((__m128i*)&p_dst[(dst_stride << 1) + dst_stride], _mm_loadu_si128 ((__m128i*)&pc_src[(src_stride << 1) + src_stride]));
}

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u32_to_u8_intrin_sse41(uint8_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const uint32_t *pc_src, hl_size_t src_stride)
{
    __m128i xmm0, xmm1, xmm2, xmm3;

	// https://connect.microsoft.com/VisualStudio/feedback/details/1054251/mm-extract-epi32-introduces-zero-extension-instruction-in-64-bit-mode
	// _mm_extract_epi32 doesn't work on 64bit CPU

    _mm_store_si128(&xmm0, _mm_load_si128((__m128i*)pc_src));
    pc_src+=src_stride;
    _mm_store_si128(&xmm1, _mm_load_si128((__m128i*)pc_src));
    pc_src+=src_stride;
    _mm_store_si128(&xmm2, _mm_load_si128((__m128i*)pc_src));
    pc_src+=src_stride;
    _mm_store_si128(&xmm3, _mm_load_si128((__m128i*)pc_src));

    _mm_store_si128(&xmm0, _mm_packus_epi32(xmm0, xmm1));
    _mm_store_si128(&xmm2, _mm_packus_epi32(xmm2, xmm3));
    _mm_store_si128(&xmm0, _mm_packus_epi16(xmm0, xmm2));

#if HL_CPU_TYPE_X64
#	if defined(_MSC_VER) && _MSC_VER < 2000/* MSVC 2015 */ // Crash: https://connect.microsoft.com/VisualStudio/feedback/details/879286/incorrect-code-generated-in-x64-build-results-in-access-violation
		p_dst[0] = xmm0.m128i_u8[0],  p_dst[1] = xmm0.m128i_u8[1],  p_dst[2] = xmm0.m128i_u8[2],  p_dst[3] = xmm0.m128i_u8[3], p_dst+=dst_stride;
		p_dst[0] = xmm0.m128i_u8[4],  p_dst[1] = xmm0.m128i_u8[5],  p_dst[2] = xmm0.m128i_u8[6],  p_dst[3] = xmm0.m128i_u8[7], p_dst+=dst_stride;
		p_dst[0] = xmm0.m128i_u8[8],  p_dst[1] = xmm0.m128i_u8[9],  p_dst[2] = xmm0.m128i_u8[10], p_dst[3] = xmm0.m128i_u8[11], p_dst+=dst_stride;
		p_dst[0] = xmm0.m128i_u8[12], p_dst[1] = xmm0.m128i_u8[13], p_dst[2] = xmm0.m128i_u8[14], p_dst[3] = xmm0.m128i_u8[15];
#else
	{
		int64_t _v = _mm_extract_epi64(xmm0, 0);
		*((uint32_t*)p_dst) = (_v & 0xFFFFFFFF); p_dst+=dst_stride;
		*((uint32_t*)p_dst) = (_v >> 32); p_dst+=dst_stride;
		_v = _mm_extract_epi64(xmm0, 1);
		*((uint32_t*)p_dst) = (_v & 0xFFFFFFFF); p_dst+=dst_stride;
		*((uint32_t*)p_dst) = (_v >> 32);
	}
#	endif
#else
	 *((uint32_t*)p_dst) = _mm_extract_epi32(xmm0, 0);
    p_dst+=dst_stride;
    *((uint32_t*)p_dst) = _mm_extract_epi32(xmm0, 1);
    p_dst+=dst_stride;
    *((uint32_t*)p_dst) = _mm_extract_epi32(xmm0, 2);
    p_dst+=dst_stride;
    *((uint32_t*)p_dst) = _mm_extract_epi32(xmm0, 3);
#endif
}

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u32_to_u8_stride4x4_intrin_sse41(uint8_t *p_dst, HL_ALIGNED(16) const uint32_t *pc_src)
{
    __m128i xmm0, xmm1, xmm2, xmm3;

    _mm_store_si128(&xmm0, _mm_load_si128((__m128i*)&pc_src[0]));
    _mm_store_si128(&xmm1, _mm_load_si128((__m128i*)&pc_src[4]));
    _mm_store_si128(&xmm2, _mm_load_si128((__m128i*)&pc_src[8]));
    _mm_store_si128(&xmm3, _mm_load_si128((__m128i*)&pc_src[12]));

    _mm_store_si128(&xmm0, _mm_packus_epi32(xmm0, xmm1));
    _mm_store_si128(&xmm2, _mm_packus_epi32(xmm2, xmm3));
    _mm_store_si128(&xmm0, _mm_packus_epi16(xmm0, xmm2));

    if (((((uintptr_t)(p_dst)) & 15) == 0)) { // (p_dst % 16) == 0 means SSE alignment is OK
        _mm_store_si128((__m128i*)p_dst, xmm0);
    }
    else {
        _mm_storeu_si128((__m128i*)p_dst, xmm0);
    }
}

HL_ALWAYS_INLINE static void hl_memory_copy16x16_u8_to_u32_stride16x16_intrin_sse2(HL_ALIGNED(16) uint32_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src)
{
    int32_t i;
    __m128i xmm_zero, xmm_src, xmm_lo16, xmm_hi16;
    _mm_store_si128(&xmm_zero, _mm_setzero_si128());

    for (i = 0; i < 16; ++i) {
        _mm_store_si128(&xmm_src, _mm_load_si128((__m128i*)pc_src));
        _mm_store_si128(&xmm_lo16, _mm_unpacklo_epi8(xmm_src, xmm_zero));
        _mm_store_si128(&xmm_hi16, _mm_unpackhi_epi8(xmm_src, xmm_zero));
        _mm_store_si128((__m128i*)p_dst, _mm_unpacklo_epi16(xmm_lo16, xmm_zero));
        _mm_store_si128((__m128i*)&p_dst[4], _mm_unpackhi_epi16(xmm_lo16, xmm_zero));
        _mm_store_si128((__m128i*)&p_dst[8], _mm_unpacklo_epi16(xmm_hi16, xmm_zero));
        _mm_store_si128((__m128i*)&p_dst[12], _mm_unpackhi_epi16(xmm_hi16, xmm_zero));

        p_dst += 16;
        pc_src += 16;
    }
}

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u8_to_u32_stride16x16_intrin_sse2(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src)
{
    __m128i xmm_zero;

    _mm_store_si128(&xmm_zero, _mm_setzero_si128());
    _mm_store_si128((__m128i*)&p_dst[0], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)&pc_src[0])), xmm_zero), xmm_zero));
    _mm_store_si128((__m128i*)&p_dst[16], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)&pc_src[16])), xmm_zero), xmm_zero));
    _mm_store_si128((__m128i*)&p_dst[32], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)&pc_src[32])), xmm_zero), xmm_zero));
    _mm_store_si128((__m128i*)&p_dst[48], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*((int32_t*)&pc_src[48])), xmm_zero), xmm_zero));
}

HL_ALWAYS_INLINE static void hl_memory_copy16x16_u8_to_u32_stride16x4_intrin_sse2(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/)
{
    int32_t i;
    __m128i xmm_src, xmm_zero;
    _mm_store_si128(&xmm_zero, _mm_setzero_si128());
    for (i = 0; i < 16; ++i) {
        if (i && ((/*i % 4*/ i & 3) == 0)) { // new line?
            p_dst += 48; /*(16*3)*/;
        }
        _mm_store_si128(&xmm_src, _mm_load_si128((__m128i*)pc_src));
        _mm_store_si128((__m128i*)&p_dst[0], _mm_unpacklo_epi16(_mm_unpacklo_epi8(xmm_src, xmm_zero), xmm_zero));
        _mm_store_si128((__m128i*)&p_dst[16], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_srli_si128(xmm_src, 4), xmm_zero), xmm_zero));
        _mm_store_si128((__m128i*)&p_dst[32], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_srli_si128(xmm_src, 8), xmm_zero), xmm_zero));
        _mm_store_si128((__m128i*)&p_dst[48], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_srli_si128(xmm_src, 12), xmm_zero), xmm_zero));
        pc_src += 16;
        p_dst += 4;
    }
}

HL_ALWAYS_INLINE static void hl_memory_copy16x16_u8_stride16x4_intrin_sse2(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/)
{
    int32_t i;
    __m128i xmm_src;

    for (i = 0; i < 16; ++i) {
        if (i && ((/*i % 4*/ i & 3) == 0)) { // new line?
            p_dst += 48; /*(16*3)*/;
        }
        _mm_store_si128(&xmm_src, _mm_load_si128((__m128i*)pc_src));
        *((int32_t*)&p_dst[0]) = _mm_cvtsi128_si32(xmm_src);
        *((int32_t*)&p_dst[16]) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_src, 4));
        *((int32_t*)&p_dst[32]) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_src, 8));
        *((int32_t*)&p_dst[48]) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_src, 12));
        pc_src += 16;
        p_dst += 4;
    }
}

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u8_to_u32_stride16x4_intrin_sse2(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, const uint8_t *pc_src/*stride=4*/)
{
    __m128i xmm_src, xmm_zero;

    _mm_store_si128(&xmm_src, _mm_load_si128((__m128i*)pc_src));
    _mm_store_si128(&xmm_zero, _mm_setzero_si128());
    _mm_store_si128((__m128i*)&p_dst[0], _mm_unpacklo_epi16(_mm_unpacklo_epi8(xmm_src, xmm_zero), xmm_zero));
    _mm_store_si128((__m128i*)&p_dst[16], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_srli_si128(xmm_src, 4), xmm_zero), xmm_zero));
    _mm_store_si128((__m128i*)&p_dst[32], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_srli_si128(xmm_src, 8), xmm_zero), xmm_zero));
    _mm_store_si128((__m128i*)&p_dst[48], _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_srli_si128(xmm_src, 12), xmm_zero), xmm_zero));
}

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u8_stride16x4_intrin_sse2(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/)
{
    __m128i xmm_src;
    _mm_store_si128(&xmm_src, _mm_load_si128((__m128i*)pc_src));

    *((int32_t*)&p_dst[0]) = _mm_cvtsi128_si32(xmm_src);
    *((int32_t*)&p_dst[16]) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_src, 4));
    *((int32_t*)&p_dst[32]) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_src, 8));
    *((int32_t*)&p_dst[48]) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_src, 12));
}

HL_ALWAYS_INLINE static void hl_memory_setzero4x4_intrin_sse2(int32_t* p_mem)
{
    _mm_prefetch((const char*)p_mem, _MM_HINT_T0);
    _mm_store_si128((__m128i*)&p_mem[0], _mm_setzero_si128());
    _mm_store_si128((__m128i*)&p_mem[4], _mm_setzero_si128());
    _mm_store_si128((__m128i*)&p_mem[8], _mm_setzero_si128());
    _mm_store_si128((__m128i*)&p_mem[12], _mm_setzero_si128());
}

HL_ALWAYS_INLINE static void hl_memory_setzero16x16_intrin_sse2(int32_t* p_mem)
{
    int32_t i;
    __m128i zero;

    _mm_store_si128(&zero, _mm_setzero_si128());
    _mm_prefetch((const char*)&zero, _MM_HINT_T0);

    for (i = 0; i < 256; i+=64) {
        _mm_store_si128((__m128i*)&p_mem[i], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 4], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 8], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 12], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 16], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 20], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 24], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 28], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 32], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 36], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 40], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 44], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 48], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 52], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 56], zero);
        _mm_store_si128((__m128i*)&p_mem[i + 60], zero);
    }
}

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_MEMORY_X86_INTRIN_H_ */

