#ifndef _HARTALLO_CODEC_X86_264_INTERPOL_INTRIN_H_
#define _HARTALLO_CODEC_X86_264_INTERPOL_INTRIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_memory.h"
#include "hartallo/intrinsics/x86/hl_math_x86_intrin.h"
#include "hartallo/intrinsics/x86/hl_utils_x86_intrin.h"

/* Headers for chroma interpolation */
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_sps.h"

//FIXME: all these functions must be ported to work SSE/SSE2

HL_BEGIN_DECLS

#if HL_HAVE_X86_INTRIN

extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_0[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_1[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_2[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_cvt_8to32_3[4];

extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_1_1[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_1[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_2_2[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_1_2[4];

extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];

extern void (*hl_codec_264_interpol_load_samples4x4_u8)(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/);
extern void (*hl_codec_264_interpol_load_samples4x4_chroma_u8)(const hl_pixel_t* cSCb_u8, HL_ALIGNED(16) uint8_t retCb_u8[4/*A=0,B=1,C=2,D=3*/][16], const hl_pixel_t* cSCr_u8, HL_ALIGNED(16) uint8_t retCr_u8[4/*A=0,B=1,C=2,D=3*/][16], int32_t i_x0, int32_t i_y0, int32_t i_height, int32_t i_width);

#define hl_codec_x86_264_interpol_set_samples(pc_indices, i_gap, cSL) \
	_mm_set_epi32((cSL)[((i_gap) << 1) + (i_gap)], (cSL)[(i_gap) << 1], (cSL)[(i_gap)], (cSL)[0])

// Load samples at index: pc_indices[0], pc_indices[gap], pc_indices[gap * 2] and pc_indices[gap * 3] into "ret"
HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_load_samples_intrin_sse2(
    const uint32_t* pc_indices,
    int32_t i_gap, // 1 = continuous
    const hl_pixel_t* cSL,
    __m128i* ret)
{
    /*if (i_gap == 1 && (pc_indices[3] - pc_indices[0]) == 3) {
    	_mm_store_si128(ret,  _mm_loadu_si128((__m128i*)&cSL[*pc_indices]));
    }
    else {*/
    _mm_store_si128(ret, _mm_set_epi32(cSL[*(pc_indices + ((i_gap << 1) + i_gap))], cSL[*(pc_indices + (i_gap << 1))], cSL[*(pc_indices + i_gap)], cSL[*pc_indices]));
    //}
}

// TODO: Add ASM version
HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_load_samples4x4_u8_intrin_sse2(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/)
{
    if ((pc_indices[3] - pc_indices[0]) == 3) {
        _mm_store_si128((__m128i*)ret_u8, _mm_unpacklo_epi32(
                            _mm_unpacklo_epi32(_mm_cvtsi32_si128(*((uint32_t*)&cSL_u8[pc_indices[0]])), _mm_cvtsi32_si128(*((uint32_t*)&cSL_u8[pc_indices[i_indices_stride * 2]]))),
                            _mm_unpacklo_epi32(_mm_cvtsi32_si128(*((uint32_t*)&cSL_u8[pc_indices[i_indices_stride]])), _mm_cvtsi32_si128(*((uint32_t*)&cSL_u8[pc_indices[i_indices_stride * 3]]))))
                       );
    }
    else {
        int32_t u0, u1, u2, u3;
        u0 = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        pc_indices += i_indices_stride;
        u1 = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        pc_indices += i_indices_stride;
        u2 = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        pc_indices += i_indices_stride;
        u3 = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        _mm_store_si128((__m128i*)ret_u8, _mm_unpacklo_epi32(
                            _mm_unpacklo_epi32(_mm_cvtsi32_si128(u0), _mm_cvtsi32_si128(u2)),
                            _mm_unpacklo_epi32(_mm_cvtsi32_si128(u1), _mm_cvtsi32_si128(u3)))
                       );
    }
}

// FIXME: not optimized
HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_load_samples4x4_u8_intrin_sse41(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/)
{
    __m128i ret = _mm_setzero_si128();
    if ((pc_indices[3] - pc_indices[0]) == 3) {
        _mm_store_si128(&ret, _mm_insert_epi32(ret, *((uint32_t*)&cSL_u8[pc_indices[0]]), 0));
        _mm_store_si128(&ret, _mm_insert_epi32(ret, *((uint32_t*)&cSL_u8[pc_indices[i_indices_stride]]), 1));
        _mm_store_si128(&ret, _mm_insert_epi32(ret, *((uint32_t*)&cSL_u8[pc_indices[i_indices_stride * 2]]), 2));
        _mm_store_si128(&ret, _mm_insert_epi32(ret, *((uint32_t*)&cSL_u8[pc_indices[i_indices_stride * 3]]), 3));
    }
    else {
        _mm_store_si128(&ret, _mm_insert_epi32(ret, cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24), 0));
        pc_indices+=i_indices_stride;
        _mm_store_si128(&ret, _mm_insert_epi32(ret, cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24), 1));
        pc_indices+=i_indices_stride;
        _mm_store_si128(&ret, _mm_insert_epi32(ret, cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24), 2));
        pc_indices+=i_indices_stride;
        _mm_store_si128(&ret, _mm_insert_epi32(ret, cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24), 3));
    }
    _mm_store_si128((__m128i*)ret_u8, ret);
}


#if 1

// Load samples at index: pc_indices[0], pc_indices[1], pc_indices[2] and pc_indices[3] into "ret"
// "ret" contains 4 packed 32-bits values
// IMPORTANT: this function requires sizeof(hl_pixel_t) = 4
// TODO: rename and add "u32" inthe name
HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2( // FIXME: _mm_shuffle_epi8-> SSE3
    const uint32_t* pc_indices, // ebp + 8
    const hl_pixel_t* cSL, // ebp + 12
    __m128i* ret // ebp + 16
)
{
    const int32_t diff = (pc_indices[3] - pc_indices[0]);
    const hl_pixel_t* p_start = &cSL[*pc_indices];
    if (hl_memory_is_address_aligned(p_start)) {
        _mm_store_si128(ret,  _mm_load_si128((__m128i*)p_start));
    }
    else {
        _mm_store_si128(ret,  _mm_loadu_si128((__m128i*)p_start));
    }
    switch(diff) {
    case 0:
        _mm_store_si128(ret, _mm_shuffle_epi8(_mm_load_si128(ret), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_0_0)));
        break;
    case 1: {
        if((pc_indices[1] - pc_indices[0]) == 0) {
            _mm_store_si128(ret, _mm_shuffle_epi8(_mm_load_si128(ret), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_0_1)));
        }
        else {
            _mm_store_si128(ret, _mm_shuffle_epi8(_mm_load_si128(ret), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_1_1_1)));
        }
    }
    break;
    case 2:
        if((pc_indices[1] - pc_indices[0]) == 0) {
            _mm_store_si128(ret, _mm_shuffle_epi8(_mm_load_si128(ret), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_1_2)));
        }
        else {
            _mm_store_si128(ret, _mm_shuffle_epi8(_mm_load_si128(ret), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_1_2_2)));
        }
        break;
    }
}
#else
#define hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2( \
    /*const uint32_t**/ pc_indices, \
    /*const hl_pixel_t**/ cSL, \
    /*__m128i**/ ret) \
{ \
	extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_0[4]; \
	extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_1_1[4]; \
	extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_0_1[4]; \
	extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_1_2_2[4]; \
	extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_shuffle_mask_0_0_1_2[4]; \
	 \
	const int32_t diff = ((pc_indices)[3] - (pc_indices)[0]); \
	const int32_t* p_start = &(cSL)[*(pc_indices)]; \
	if (hl_memory_is_address_aligned(p_start)) { \
		_mm_store_si128((ret),  _mm_load_si128((__m128i*)p_start)); \
	} \
	else { \
		_mm_store_si128((ret),  _mm_loadu_si128((__m128i*)p_start)); \
	} \
	switch(diff) { \
		case 0: \
			_mm_store_si128((ret), _mm_shuffle_epi8(_mm_load_si128(ret), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_0_0))); \
			break; \
		case 1: \
			{ \
				if(((pc_indices)[1] - (pc_indices)[0]) == 0){ \
					_mm_store_si128((ret), _mm_shuffle_epi8(_mm_load_si128((ret)), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_0_1))); \
				} \
				else { \
					_mm_store_si128((ret), _mm_shuffle_epi8(_mm_load_si128((ret)), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_1_1_1))); \
				} \
			} \
			break; \
		case 2: \
			if(((pc_indices)[1] - (pc_indices)[0]) == 0){ \
				_mm_store_si128((ret), _mm_shuffle_epi8(_mm_load_si128((ret)), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_0_1_2))); \
			} \
			else { \
				_mm_store_si128((ret), _mm_shuffle_epi8(_mm_load_si128((ret)), _mm_load_si128((__m128i*)__x86_globals_array4_shuffle_mask_0_1_2_2))); \
			} \
			break; \
	} \
}
#endif

// "ret" contains 16 packed 8-bits values
HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    __m128i* ret // 16x8b values
)
{
    __m128i tmp0_32, tmp1_32, tmp2_32, tmp3_32;
    __m128i tmp0_16, tmp1_16;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &tmp0_32);
    pc_indices+=i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &tmp1_32);
    pc_indices+=i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &tmp2_32);
    pc_indices+=i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &tmp3_32);

    // Packs the 8 signed 32-bit integers from "tmpX" and "tmpY" into signed 16-bit integers and saturates.
    _mm_store_si128(&tmp0_16, _mm_packs_epi32(tmp0_32, tmp1_32));
    _mm_store_si128(&tmp1_16, _mm_packs_epi32(tmp2_32, tmp3_32));
    // Packs the 16 signed 16-bit integers from "tmp0_16" and "tmp1_16" into 8-bit unsigned integers and saturates.
    _mm_store_si128(ret, _mm_packus_epi16(tmp0_16, tmp1_16));
}

// Partial Operation
HL_ALWAYS_INLINE static void hl_codec_x86_264_tap6filter_vert4_intrin_sse41(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    __m128i* ret // 4x32b values
)
{
    // P = origin
    // [E] positions = (P + 0), (P + stride), (P + (stride * 2), (P + (stride * 3)), (P + (stride *4)), (P + (stride * 5))
    // P = P + 1
    // [F], [G], [H], [I] and [J]  positions are computed as E
    // This means (P + 0), (P + 1), (P + 2)....(P + 5) will contain [E] then [F]....[G] every time we increment by "stride".

    __m128i E, F, G, H, I, J;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &E);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &F);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &G);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &H);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &I);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &J);

    hl_math_tap6filter4x1_u32_intrin_sse41(&E, &F, &G, &H, &I, &J, ret);
}

// Partial Operation
HL_ALWAYS_INLINE static void hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    __m128i* ret_lo, // 8x16b values
    __m128i* ret_hi // 8x16b values
)
{
    __m128i E, F, G, H, I, J;

    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&E);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&F);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&G);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&H);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&I);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&J);

    hl_math_tap6filter4x4_u8_partial_intrin_sse3(&E, &F, &G, &H, &I, &J, ret_lo, ret_hi);
}

// Full Operation
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Clip(((Tap6Filter + 16) >> 5))
// Clip operation is done by the math function when the "lo" and "hi" 16bits are packed.
HL_ALWAYS_INLINE static void hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    __m128i* ret // 16x8b values
)
{
    __m128i E, F, G, H, I, J;

    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&E);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&F);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&G);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&H);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&I);
    pc_indices += i_indices_stride;
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&J);

    hl_math_tap6filter4x4_u8_full_intrin_sse3(&E, &F, &G, &H, &I, &J, ret);
}

// Partial Operation
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Tap6Filter, without clip and shift
HL_ALWAYS_INLINE static void hl_codec_x86_264_tap6filter_vert4x4_partial_intrin_sse3(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    __m128i* ret_lo, // 8x16b values
    __m128i* ret_hi	 // 8x16b values
)
{
    __m128i E, F, G, H, I, J;
    hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(pc_indices, i_indices_stride, cSL, &E);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(pc_indices, i_indices_stride, cSL, &F);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(pc_indices, i_indices_stride, cSL, &G);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(pc_indices, i_indices_stride, cSL, &H);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(pc_indices, i_indices_stride, cSL, &I);
    pc_indices += i_indices_stride;
    hl_codec_x86_264_interpol_load_samples_continuous4x4_u32_intrin_sse2(pc_indices, i_indices_stride, cSL, &J);

    hl_math_tap6filter4x4_u8_partial_intrin_sse3(&E, &F, &G, &H, &I, &J, ret_lo, ret_hi);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(
    const uint32_t* pc_indices,
    const hl_pixel_t* cSL,
    __m128i* ret
)
{
    __m128i E, F, G, H, I, J;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &E);
    pc_indices += 1;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &F);
    pc_indices += 1;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &G);
    pc_indices += 1;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &H);
    pc_indices += 1;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &I);
    pc_indices += 1;
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices, cSL, &J);

    hl_math_tap6filter4x1_u32_intrin_sse41(&E, &F, &G, &H, &I, &J, ret);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    __m128i* ret
)
{
    __m128i E, F, G, H, I, J;

    hl_codec_264_interpol_load_samples4x4_u8(pc_indices, i_indices_stride, cSL_u8, (uint8_t*)&E);
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices[1], i_indices_stride, cSL_u8, (uint8_t*)&F);
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices[2], i_indices_stride, cSL_u8, (uint8_t*)&G);
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices[3], i_indices_stride, cSL_u8, (uint8_t*)&H);
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices[4], i_indices_stride, cSL_u8, (uint8_t*)&I);
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices[5], i_indices_stride, cSL_u8, (uint8_t*)&J);

    hl_math_tap6filter4x4_u8_full_intrin_sse3(&E, &F, &G, &H, &I, &J, ret);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma00_horiz4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL
)
{
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(pc_indices_horiz, cSL, (__m128i*)predPartLXL);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma00_horiz4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    hl_codec_264_interpol_load_samples4x4_u8(pc_indices_horiz, i_indices_stride, cSL_u8, predPartLXL16x1);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma01_vert4_intrin_sse41(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    __m128i G, h1, h;

    // load (unaligned) G
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(&pc_indices_vert[(i_indices_stride << 1)], cSL, &G);
    // h1 = Tap6Filter(indices)
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);
    // h1 = (h1 + 16)
    _mm_store_si128(&h1, _mm_add_epi32(h1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    // h1 = h1 >> 5;
    _mm_store_si128(&h1, _mm_srai_epi32(h1, 5));
    // h = Clip1Y(h1, BitDepthY)
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &h1, &h);
    // h += G
    _mm_store_si128(&h, _mm_add_epi32(h, G));
    // h = h + 1;
    _mm_store_si128(&h, _mm_add_epi32(h, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    // ret = h >>= 1
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(h, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma01_vert4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i G, h; // 16x8b

    // Load G's samples
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_vert[(i_indices_stride << 1)], i_indices_stride, cSL_u8, (uint8_t*)&G);
    // h = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h);
    // h = ((h + G + 1) >> 1)
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8 (h, G));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma02_vert4_intrin_sse41(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    __m128i h1;

    // h1 = Tap6Filter(indices)
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);
    // h1 = (h1 + 16)
    _mm_store_si128(&h1, _mm_add_epi32(h1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    // h1 = h1 >> 5;
    _mm_store_si128(&h1, _mm_srai_epi32(h1, 5));
    // ret = Clip1Y(h1, BitDepthY)
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &h1, (hl_int128_t*)predPartLXL);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma02_vert4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    // h1 = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, (__m128i*)predPartLXL16x1);
}


HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma03_vert4_intrin_sse41(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i h1,h,G;
    // h1 = Tap6Filter(indices)
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);
    // h1 = (h1 + 16)
    _mm_store_si128(&h1, _mm_add_epi32(h1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    // h1 = h1 >> 5;
    _mm_store_si128(&h1, _mm_srai_epi32(h1, 5));
    // h = Clip1Y(h1, BitDepthY)
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &h1, &h);
    // predPartLXL[i] = (cSL[pc_indices_vert[(i_indices_stride * 3)]] + h + 1) >> 1;// (8-255)
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(&pc_indices_vert[(i_indices_stride * 3)], cSL, &G);
    _mm_store_si128(&h, _mm_add_epi32(h, G));
    _mm_store_si128(&h, _mm_add_epi32(h, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(h, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma03_vert4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i h, G;
    // h = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h);
    // Load G
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_vert[(i_indices_stride * 3)], i_indices_stride, cSL_u8, (uint8_t*)&G);
    // h = ((h + G + 1) >> 1)
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(h, G));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma10_horiz4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    __m128i G,b1,b;
    // load (unaligned) G
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(&pc_indices_horiz[2], cSL, &G);
    // b1 = Tap6FilterHoriz(indices)
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b1);
    // b1 = (b1 + 16)
    _mm_store_si128(&b1, _mm_add_epi32(b1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    // b1 = b1 >> 5
    _mm_store_si128(&b1, _mm_srai_epi32(b1, 5));
    // b = Clip1Y(b1, BitDepthY)
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b1, &b);
    // b = b + 1
    _mm_store_si128(&b, _mm_add_epi32(b, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    // b = b + G
    _mm_store_si128(&b, _mm_add_epi32(b, G));
    // predPartLXL = b >> 1
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(b, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma10_horiz4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i G, b;
    // load (unaligned) G
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_horiz[2], i_indices_stride, cSL_u8, (uint8_t*)&G);
    // b = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b);
    // b = ((b + G + 1) >> 1)
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(b, G));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma11_diag4_intrin_sse41(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i h1,h,b1,b;

    // h1 = _hl_codec_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &h1);
    // h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);// (8-246)
    _mm_store_si128(&h1, _mm_add_epi32(h1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&h1, _mm_srai_epi32(h1, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &h1, &h);
    // b1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b1);
    // b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);// (8-245)
    _mm_store_si128(&b1, _mm_add_epi32(b1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&b1, _mm_srai_epi32(b1, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b1, &b);
    // predPartLXL[i] = (b + h + 1) >> 1;// (8-260
    _mm_store_si128(&h, _mm_add_epi32(h, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&h, _mm_add_epi32(h, b));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(h, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma11_diag4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i h, b;

    // h = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h);
    // b = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b);
    // b = (b + h + 1) >> 1
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(b, h));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma12_vert4_intrin_sse41(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    // FIXME: blocks_map and do it for all others and test speed
    __m128i h1,h,cc,dd,m1,ee,ff,j1,j;
    // h1 = Tap6FilterVert(indices[0])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);
    // h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);// (8-246)
    _mm_store_si128(&h, _mm_add_epi32(h1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&h, _mm_srai_epi32(h, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &h, &h);
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);
    // j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);
    hl_math_tap6filter4x1_u32_intrin_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j1);
    // j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j, _mm_add_epi32(j1, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)));
    _mm_store_si128(&j, _mm_srai_epi32(j, 10));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &j, &j);
    // predPartLXL[i] = (h + j + 1) >> 1;// (8-257)
    _mm_store_si128(&j, _mm_add_epi32(j, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&j, _mm_add_epi32(j, h));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(j, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma12_vert4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array8_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];

    __m128i h1_lo, h1_hi, h, cc_lo, cc_hi, dd_lo, dd_hi, m1_lo, m1_hi, ee_lo, ee_hi, ff_lo, ff_hi, j1_lo, j1_hi, j_lo, j_hi;

    // h1 = Tap6FilterVert(indices[0])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi); // (h1_lo, h1_hi) -> 8x16b
    // h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&h,
                    _mm_packus_epi16(
                        _mm_srai_epi16(_mm_add_epi16(h1_lo, _mm_load_si128((__m128i*)__x86_globals_array8_sixteens)), 5),
                        _mm_srai_epi16(_mm_add_epi16(h1_hi, _mm_load_si128((__m128i*)__x86_globals_array8_sixteens)), 5))
                   ); // h -> 16x8b
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi); // (cc_lo, cc_hi) -> 8x16b
    // dd = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi); // (dd_lo, dd_hi) -> 8x16b
    // m1 = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi); // (m1_lo, m1_hi) -> 8x16b
    // ee = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi); // (ee_lo, ee_hi) -> 8x16b
    // ff = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi); // (ff_lo, ff_hi) -> 8x16b
    // j1 = Tap6Filter(cc_lo, dd_lo, h1_lo, m1_lo, ee_lo, ff_lo)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j_lo = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_lo, _mm_packs_epi32(j1_lo, j1_hi)); // j_lo -> 8x16b
    // j1 = Tap6Filter(cc_hi, dd_hi, h1_hi, m1_hi, ee_hi, ff_hi)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j_hi = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_hi, _mm_packs_epi32(j1_lo, j1_hi)); // j_hi -> 8x16b
    // h = (h + j + 1) >> 1; with j = _mm_packus_epi16(j_lo, j_hi) -> 16x8b
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(h, _mm_packus_epi16(j_lo, j_hi)));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma13_diag4_intrin_sse41(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i h,s;
    // h1 = _hl_codec_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &h);
    // h = HL_MATH_CLIP1Y(((h1 + 16) >> 5), BitDepthY);// (8-246)
    _mm_store_si128(&h, _mm_add_epi32(h, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&h, _mm_srai_epi32(h, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &h, &h);

    // s1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &s);
    // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&s, _mm_add_epi32(s, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&s, _mm_srai_epi32(s, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &s, &s);

    // predPartLXL[i] = (h + s + 1) >> 1;// (8-262)
    _mm_store_si128(&s, _mm_add_epi32(s, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&s, _mm_add_epi32(s, h));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(s, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma13_diag4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i h, s;

    // h = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &h);
    // s = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &s);
    // s = (s + h + 1) >> 1
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(s, h));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma20_horiz4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i b;
    // b = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b);
    // predPartLXL[i] = HL_MATH_CLIP1Y(((b + 16) >> 5), BitDepthY);// (8-245)
    _mm_store_si128(&b, _mm_add_epi32(b, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&b, _mm_srai_epi32(b, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b, (hl_int128_t*)predPartLXL);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma20_horiz4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    // b = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, (__m128i*)predPartLXL16x1);
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma21_diag4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i h1,b,cc,dd,m1,ee,ff,j;

    // b1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b);
    // h1 = _hl_codec_264_tap6filter_vert(pc_indices_vert[0], i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);
    // b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);// (8-245)
    _mm_store_si128(&b, _mm_add_epi32(b, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&b, _mm_srai_epi32(b, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b, &b);

    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);
    // j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);
    hl_math_tap6filter4x1_u32_intrin_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j);
    // j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j, _mm_add_epi32(j, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)));
    _mm_store_si128(&j, _mm_srai_epi32(j, 10));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &j, &j);

    // predPartLXL[i] = (b + j + 1) >> 1;// (8-256)
    _mm_store_si128(&j, _mm_add_epi32(j, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&j, _mm_add_epi32(j, b));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(j, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma21_diag4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];

    __m128i h1_lo, h1_hi, cc_lo, cc_hi, dd_lo, dd_hi, m1_lo, m1_hi, ee_lo, ee_hi, ff_lo, ff_hi, j1_lo, j1_hi, j_lo, j_hi, b;

    // b = Tap6FilterHoriz()
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b); // (b) -> 8x16b

    // h1 = Tap6FilterVert(indices[0])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi); // (h1_lo, h1_hi) -> 8x16b
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi); // (cc_lo, cc_hi) -> 8x16b
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi); // (dd_lo, dd_hi) -> 8x16b
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi); // (m1_lo, m1_hi) -> 8x16b
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi); // (ee_lo, ee_hi) -> 8x16b
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi); // (ff_lo, ff_hi) -> 8x16b
    // j_lo = Tap6Filter(cc_lo, dd_lo, h1_lo, m1_lo, ee_lo, ff_lo)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j1_lo = HL_MATH_CLIP1Y(((j1_lo + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_lo, _mm_packs_epi32(j1_lo, j1_hi)); // j_lo -> 8x16b
    // j_hi = Tap6Filter(cc_hi, dd_hi, h1_hi, m1_hi, ee_hi, ff_hi)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j_hi = HL_MATH_CLIP1Y(((j1_hi + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_hi, _mm_packs_epi32(j1_lo, j1_hi)); // j_hi -> 8x16b
    // b = (b + j + 1) >> 1; with j = _mm_packus_epi16(j_lo, j_hi) -> 16x8b
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(b, _mm_packus_epi16(j_lo, j_hi)));
}


HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma22_vert4_intrin_sse41(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    __m128i h1,cc,dd,m1,ee,ff,j1;

    // h1 = _hl_codec_264_tap6filter_vert(pc_indices_vert[0], i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);
    // j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);
    hl_math_tap6filter4x1_u32_intrin_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j1);
    // predPartLXL[i] = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1, _mm_add_epi32(j1, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)));
    _mm_store_si128(&j1, _mm_srai_epi32(j1, 10));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &j1, (hl_int128_t*)predPartLXL);
}


HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma22_vert4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    __m128i h1_lo, h1_hi, cc_lo, cc_hi, dd_lo, dd_hi, m1_lo, m1_hi, ee_lo, ee_hi, ff_lo, ff_hi, j1_lo, j1_hi, j_lo, j_hi;

    // h1 = Tap6FilterVert(indices[0])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi); // (h1_lo, h1_hi) -> 8x16b
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi); // (cc_lo, cc_hi) -> 8x16b
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi); // (dd_lo, dd_hi) -> 8x16b
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi); // (m1_lo, m1_hi) -> 8x16b
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi); // (ee_lo, ee_hi) -> 8x16b
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi); // (ff_lo, ff_hi) -> 8x16b
    // j_lo = Tap6Filter(cc_lo, dd_lo, h1_lo, m1_lo, ee_lo, ff_lo)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j1_lo = HL_MATH_CLIP1Y(((j1_lo + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_lo, _mm_packs_epi32(j1_lo, j1_hi)); // j_lo -> 8x16b
    // j_hi = Tap6Filter(cc_hi, dd_hi, h1_hi, m1_hi, ee_hi, ff_hi)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j_hi = HL_MATH_CLIP1Y(((j1_hi + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_hi, _mm_packs_epi32(j1_lo, j1_hi)); // j_hi -> 8x16b
    //  j = packs(j_lo, j_hi) -> 16x8b
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_packus_epi16(j_lo, j_hi));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma23_diag4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i s,h1,cc,dd,m1,ee,ff,j;

    // s1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &s);
    // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&s, _mm_add_epi32(s, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&s, _mm_srai_epi32(s, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &s, &s);

    // h1 = _hl_codec_264_tap6filter_vert(pc_indices_vert[0], i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);
    // j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);
    hl_math_tap6filter4x1_u32_intrin_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j);
    // j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j, _mm_add_epi32(j, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)));
    _mm_store_si128(&j, _mm_srai_epi32(j, 10));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &j, &j);
    // predPartLXL[i] = (j + s + 1) >> 1;// (8-259)
    _mm_store_si128(&j, _mm_add_epi32(j, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&j, _mm_add_epi32(j, s));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(j, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma23_diag4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    __m128i s, h1_lo, h1_hi, cc_lo, cc_hi, dd_lo, dd_hi, m1_lo, m1_hi, ee_lo, ee_hi, ff_lo, ff_hi, j1_lo, j1_hi, j_lo, j_hi;

    // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &s);
    // h1 = Tap6FilterVert(indices[0])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi); // (h1_lo, h1_hi) -> 8x16b
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi); // (cc_lo, cc_hi) -> 8x16b
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi); // (dd_lo, dd_hi) -> 8x16b
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi); // (m1_lo, m1_hi) -> 8x16b
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi); // (ee_lo, ee_hi) -> 8x16b
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi); // (ff_lo, ff_hi) -> 8x16b
    // j_lo = Tap6Filter(cc_lo, dd_lo, h1_lo, m1_lo, ee_lo, ff_lo)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j1_lo = HL_MATH_CLIP1Y(((j1_lo + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_lo, _mm_packs_epi32(j1_lo, j1_hi)); // j_lo -> 8x16b
    // j_hi = Tap6Filter(cc_hi, dd_hi, h1_hi, m1_hi, ee_hi, ff_hi)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j_hi = HL_MATH_CLIP1Y(((j1_hi + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_hi, _mm_packs_epi32(j1_lo, j1_hi)); // j_hi -> 8x16b
    // s = (j + s + 1) >> 1; with j = _mm_packus_epi16(j_lo, j_hi) -> 16x8b
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(s, _mm_packus_epi16(j_lo, j_hi)));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma30_horiz4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i b,G;

    // b1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b);
    // b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);// (8-245)
    _mm_store_si128(&b, _mm_add_epi32(b, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&b, _mm_srai_epi32(b, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b, &b);
    // predPartLXL[i] = (cSL[pc_indices_horiz[3]] + b + 1) >> 1;// (8-253)
    hl_codec_x86_264_interpol_load_samples_continuous_intrin_sse2(&pc_indices_horiz[3], cSL, &G);
    _mm_store_si128(&G, _mm_add_epi32(G, b));
    _mm_store_si128(&G, _mm_add_epi32(G, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(G, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma30_horiz4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i b, G;

    // b1 = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b);
    // load samples
    hl_codec_264_interpol_load_samples4x4_u8(&pc_indices_horiz[3], i_indices_stride, cSL_u8, (uint8_t*)&G);
    // b = (G + b + 1) >> 1;
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(b, G));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma31_diag4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i b, m;

    // b1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b);
    // b = HL_MATH_CLIP1Y(((b1 + 16) >> 5), BitDepthY);// (8-245)
    _mm_store_si128(&b, _mm_add_epi32(b, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&b, _mm_srai_epi32(b, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b, &b);
    // m1 = _hl_codec_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &m);
    // m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&m, _mm_add_epi32(m, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&m, _mm_srai_epi32(m, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &m, &m);
    // predPartLXL[i] = (b + m + 1) >> 1;// (8-261)
    _mm_store_si128(&m, _mm_add_epi32(m, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&m, _mm_add_epi32(m, b));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(m, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma31_diag4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i b, m;

    // b = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &b);
    // m = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &m);
    // b = (b + m + 1) >> 1
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(b, m));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma32_vert4_intrin_sse41(
    const uint32_t* pc_indices_vert[7],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i h1,cc,dd,m1,ee,ff,j;

    // h1 = _hl_codec_264_tap6filter_vert(pc_indices_vert[0], i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[0], i_indices_stride, cSL, &h1);
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[1], i_indices_stride, cSL, &cc);
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[2], i_indices_stride, cSL, &dd);
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[3], i_indices_stride, cSL, &m1);
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[4], i_indices_stride, cSL, &ee);
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[5], i_indices_stride, cSL, &ff);
    // j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);
    hl_math_tap6filter4x1_u32_intrin_sse41(&cc,&dd,&h1,&m1,&ee,&ff,&j);
    // j = HL_MATH_CLIP1Y(((j1 + 512) >> 10), BitDepthY);
    _mm_store_si128(&j, _mm_add_epi32(j, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)));
    _mm_store_si128(&j, _mm_srai_epi32(j, 10));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &j, &j);

    // m1 = _hl_codec_264_tap6filter_vert(pc_indices_vert[6], i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert[6], i_indices_stride, cSL, &m1);
    // m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&m1, _mm_add_epi32(m1, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&m1, _mm_srai_epi32(m1, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &m1, &m1);

    // predPartLXL[i] = (j + m + 1) >> 1;// (8-258)
    _mm_store_si128(&m1, _mm_add_epi32(m1, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&m1, _mm_add_epi32(m1, j));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(m1, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma32_vert4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_vert[7],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
    __m128i s, h1_lo, h1_hi, cc_lo, cc_hi, dd_lo, dd_hi, m1_lo, m1_hi, ee_lo, ee_hi, ff_lo, ff_hi, j1_lo, j1_hi, j_lo, j_hi;

    // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert[6], i_indices_stride, cSL_u8, &s);
    // h1 = Tap6FilterVert(indices[0])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[0], i_indices_stride, cSL_u8, &h1_lo, &h1_hi); // (h1_lo, h1_hi) -> 8x16b
    // cc = Tap6FilterVert(indices[1])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[1], i_indices_stride, cSL_u8, &cc_lo, &cc_hi); // (cc_lo, cc_hi) -> 8x16b
    // dd = Tap6FilterVert(indices[2])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[2], i_indices_stride, cSL_u8, &dd_lo, &dd_hi); // (dd_lo, dd_hi) -> 8x16b
    // m1 = Tap6FilterVert(indices[3])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[3], i_indices_stride, cSL_u8, &m1_lo, &m1_hi); // (m1_lo, m1_hi) -> 8x16b
    // ee = Tap6FilterVert(indices[4])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[4], i_indices_stride, cSL_u8, &ee_lo, &ee_hi); // (ee_lo, ee_hi) -> 8x16b
    // ff = Tap6FilterVert(indices[5])
    hl_codec_x86_264_tap6filter_vert4x4_u8_partial_intrin_sse3(pc_indices_vert[5], i_indices_stride, cSL_u8, &ff_lo, &ff_hi); // (ff_lo, ff_hi) -> 8x16b
    // j_lo = Tap6Filter(cc_lo, dd_lo, h1_lo, m1_lo, ee_lo, ff_lo)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_lo, &dd_lo, &h1_lo, &m1_lo, &ee_lo, &ff_lo, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j1_lo = HL_MATH_CLIP1Y(((j1_lo + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_lo, _mm_packs_epi32(j1_lo, j1_hi)); // j_lo -> 8x16b
    // j_hi = Tap6Filter(cc_hi, dd_hi, h1_hi, m1_hi, ee_hi, ff_hi)
    hl_math_tap6filter4x2_u16_partial_intrin_sse3(&cc_hi, &dd_hi, &h1_hi, &m1_hi, &ee_hi, &ff_hi, &j1_lo, &j1_hi); // (j1_lo, j1_hi) -> 4x32b
    // j_hi = HL_MATH_CLIP1Y(((j1_hi + 512) >> 10), BitDepthY);
    _mm_store_si128(&j1_lo, _mm_srai_epi32(_mm_add_epi32(j1_lo, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j1_hi, _mm_srai_epi32(_mm_add_epi32(j1_hi, _mm_load_si128((__m128i*)__x86_globals_array4_five_hundred_and_twelves)), 10));
    _mm_store_si128(&j_hi, _mm_packs_epi32(j1_lo, j1_hi)); // j_hi -> 8x16b
    // s = (j + s + 1) >> 1; with j = _mm_packus_epi16(j_lo, j_hi) -> 16x8b
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(s, _mm_packus_epi16(j_lo, j_hi)));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma33_diag4_intrin_sse41(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
    __m128i s,m;

    // s1 = _hl_codec_264_tap6filter_horiz(pc_indices_horiz, cSL);
    hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &s);
    // s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&s, _mm_add_epi32(s, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&s, _mm_srai_epi32(s, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &s, &s);

    // m1 = _hl_codec_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
    hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &m);
    // m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);
    _mm_store_si128(&m, _mm_add_epi32(m, _mm_load_si128((__m128i*)__x86_globals_array4_sixteens)));
    _mm_store_si128(&m, _mm_srai_epi32(m, 5));
    hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &m, &m);

    // predPartLXL[i] = (m + s + 1) >> 1;// (8-263)
    _mm_store_si128(&m, _mm_add_epi32(m, _mm_load_si128((__m128i*)__x86_globals_array4_ones)));
    _mm_store_si128(&m, _mm_add_epi32(m, s));
    _mm_store_si128((__m128i*)predPartLXL, _mm_srai_epi32(m, 1));
}

HL_ALWAYS_INLINE static void hl_codec_x86_264_interpol_luma33_diag4x4_u8_intrin_sse3(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    __m128i s, m;

    // s = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_horiz4x4_u8_full_intrin_sse3(pc_indices_horiz, i_indices_stride, cSL_u8, &s);
    // m = Clip1Y(((Tap6Filter(indices) + 16) >> 5))
    hl_codec_x86_264_tap6filter_vert4x4_u8_full_intrin_sse3(pc_indices_vert, i_indices_stride, cSL_u8, &m);
    // m = (m + s + 1) >> 1
    _mm_store_si128((__m128i*)predPartLXL16x1, _mm_avg_epu8(m, s));
}

void hl_codec_x86_264_interpol_chroma_cat1_u8_intrin_sse2(
    struct hl_codec_264_s* p_codec,
    const hl_pixel_t* cSCb_u8, const hl_pixel_t* cSCr_u8,
    int32_t xFracC, int32_t yFracC,
    int32_t _xIntC, int32_t _yIntC, int32_t partWidthC, int32_t partHeightC, int32_t i_pic_height, int32_t i_pic_width,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);

#if 0
#error "Your're using the worst code I've ever seen"
HL_ALWAYS_INLINE static HL_ERROR_T hl_codec_x86_264_interpol_chroma_intrin_sse41(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvCLX,
    const hl_codec_264_mv_xt* mvLX, // used when "ChromaArrayType"=3
    const hl_codec_264_pict_t* refPicLXCb,
    const hl_codec_264_pict_t* refPicLXCr,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16])
{
    static hl_bool_t g_initialized = HL_FALSE;
    static __m128i __xFracC_per_yFracC[8][8], __8minus_xFracC[8][8], __8minus_yFracC[8][8], __8minus_xFracC_per_8minus_yFracC[8][8], __8minus_xFracC_per_yFracC[8][8], __8minus_yFracC_per_xFracC[8][8];
    static __m128i _0000, _0011, _0101, _1111, _2222, _32;

    int32_t xC, yC, xFracC, yFracC, idx0, idx1, idx2, idx3;
    int32_t refPicHeightEffectiveC, refPicWidthEffectiveC;
    int32_t  partHeightC = p_mb->partHeightC[mbPartIdx][subMbPartIdx];
    int32_t  partWidthC = p_mb->partWidthC[mbPartIdx][subMbPartIdx];

    const hl_pixel_t* cSb = refPicLXCb->pc_data_u;
    const hl_pixel_t* cSr = refPicLXCb->pc_data_v;
    int32_t chroma_width = refPicLXCb->uWidthC;//FIXME

    __m128i xIntC, yIntC, _xIntC, _yIntC, _chroma_width, _chroma_width_m2, _00ww;
    __m128i tmp8, maxX, maxY;
    __m128i idxA, idxB, idxC, idxD;

    _mm_store_si128(&_chroma_width, _MM_SET1_EPI32_SSE41(chroma_width));
    _mm_store_si128(&_chroma_width_m2, _mm_slli_epi32(_chroma_width, 1));
    _mm_store_si128(&_00ww, _MM_SET_EPI32_SSE41(chroma_width, chroma_width, 0, 0));

    refPicWidthEffectiveC = p_codec->layers.pc_active->pc_slice_hdr->PicWidthInSamplesC;
    if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag == 0 || p_mb->mb_field_decoding_flag == 0) {
        refPicHeightEffectiveC = p_codec->layers.pc_active->pc_slice_hdr->PicHeightInSamplesC;
    }
    else {
        refPicHeightEffectiveC = p_codec->layers.pc_active->pc_slice_hdr->PicHeightInSamplesC >> 1;
    }

    _mm_store_si128(&maxX, _MM_SET1_EPI32_SSE41(refPicWidthEffectiveC - 1));
    _mm_store_si128(&maxY, _MM_SET1_EPI32_SSE41((refPicHeightEffectiveC - 1) * chroma_width));

    if(!g_initialized) {
        _mm_store_si128(&_0011, _MM_SET_EPI32_SSE41(1, 1, 0, 0));
        _mm_store_si128(&_0101, _MM_SET_EPI32_SSE41(1, 0, 1, 0));
        _mm_store_si128(&_0000, _MM_SET1_EPI32_SSE41(0));
        _mm_store_si128(&_1111, _MM_SET1_EPI32_SSE41(1));
        _mm_store_si128(&_2222, _MM_SET1_EPI32_SSE41(2));
        _mm_store_si128(&_32, _MM_SET1_EPI16_SSSE3(32));
        for(yFracC = 0; yFracC < 8; ++yFracC) {
            for(xFracC = 0; xFracC < 8; ++xFracC) {
                _mm_store_si128(&__xFracC_per_yFracC[yFracC][xFracC], _MM_SET1_EPI16_SSSE3(xFracC * yFracC));
                _mm_store_si128(&__8minus_xFracC[yFracC][xFracC], _MM_SET1_EPI16_SSSE3(8 - xFracC));
                _mm_store_si128(&__8minus_yFracC[yFracC][xFracC], _MM_SET1_EPI16_SSSE3(8 - yFracC));
                _mm_store_si128(&__8minus_xFracC_per_8minus_yFracC[yFracC][xFracC], _MM_SET1_EPI16_SSSE3((8 - xFracC) * (8 - yFracC)));
                _mm_store_si128(&__8minus_xFracC_per_yFracC[yFracC][xFracC], _MM_SET1_EPI16_SSSE3((8 - xFracC) * yFracC));
                _mm_store_si128(&__8minus_yFracC_per_xFracC[yFracC][xFracC], _MM_SET1_EPI16_SSSE3((8 - yFracC) * xFracC));
            }
        }
        g_initialized = HL_TRUE;
    }

    if (p_codec->sps.pc_active->ChromaArrayType == 1) {
        __m128i _xFracC_per_yFracC, _8minus_xFracC, _8minus_yFracC, _8minus_xFracC_per_8minus_yFracC, _8minus_xFracC_per_yFracC, _8minus_yFracC_per_xFracC;
        _xIntC = _MM_SET1_EPI32_SSE41((p_mb->xL_Idx >> p_codec->sps.pc_active->SubWidthC_TrailingZeros) + (mvCLX->x >> 3));
        _yIntC = _MM_SET1_EPI32_SSE41(((p_mb->yL_Idx >> p_codec->sps.pc_active->SubHeightC_TrailingZeros) + (mvCLX->y >> 3)) * chroma_width);
        xFracC = mvCLX->x & 7; // (8-231)
        yFracC = mvCLX->y & 7; // (8-232)

        _mm_store_si128(&_xFracC_per_yFracC, (__xFracC_per_yFracC[yFracC][xFracC]));
        _mm_store_si128(&_8minus_xFracC, (__8minus_xFracC[yFracC][xFracC]));
        _mm_store_si128(&_8minus_yFracC, (__8minus_yFracC[yFracC][xFracC]));
        _mm_store_si128(&_8minus_xFracC_per_8minus_yFracC, (__8minus_xFracC_per_8minus_yFracC[yFracC][xFracC]));
        _mm_store_si128(&_8minus_xFracC_per_yFracC, (__8minus_xFracC_per_yFracC[yFracC][xFracC]));
        _mm_store_si128(&_8minus_yFracC_per_xFracC, (__8minus_yFracC_per_xFracC[yFracC][xFracC]));

        for (yC = 0; yC < partHeightC; yC+=2) {
            _mm_store_si128(&yIntC, _mm_add_epi32(_yIntC, _00ww)); // (8-230)
            _mm_store_si128(&xIntC, _mm_add_epi32(_xIntC, _0101)); // (8-229)
            _mm_store_si128(&p_codec->simd.mo_comp.yAC, HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxY, yIntC)); // (8-268)
            _mm_store_si128(&p_codec->simd.mo_comp.yCC, HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxY, _mm_add_epi32(yIntC, _chroma_width))); // (8-270)
            for (xC = 0; xC < partWidthC; xC+=2) {
                _mm_store_si128(&p_codec->simd.mo_comp.xAC, HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxX, xIntC)); // (8-264)
                _mm_store_si128(&p_codec->simd.mo_comp.xBC, HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxX, _mm_add_epi32(xIntC, _1111))); // (8-265)

                // yDC = yCC and yBC = yAC and xCC = xAC and xDC = xBC
                _mm_store_si128(&idxA, _mm_add_epi32(p_codec->simd.mo_comp.xAC, p_codec->simd.mo_comp.yAC));
                _mm_store_si128(&idxB, _mm_add_epi32(p_codec->simd.mo_comp.xBC, p_codec->simd.mo_comp.yAC));
                _mm_store_si128(&idxC, _mm_add_epi32(p_codec->simd.mo_comp.xAC, p_codec->simd.mo_comp.yCC));
                _mm_store_si128(&idxD, _mm_add_epi32(p_codec->simd.mo_comp.xBC, p_codec->simd.mo_comp.yCC));

                // 8.4.2.2.2 Chroma sample interpolation process
                idx0 = _mm_extract_epi32(idxA, 0);
                idx1 = _mm_extract_epi32(idxA, 1);
                idx2 = _mm_extract_epi32(idxA, 2);
                idx3 = _mm_extract_epi32(idxA, 3);
                _mm_store_si128(&p_codec->simd.mo_comp.A, _MM_SET_EPI16_SSE41(cSb[idx3], cSb[idx2], cSb[idx1], cSb[idx0], cSr[idx3], cSr[idx2], cSr[idx1], cSr[idx0]));
                idx0 = _mm_extract_epi32(idxB, 0);
                idx1 = _mm_extract_epi32(idxB, 1);
                idx2 = _mm_extract_epi32(idxB, 2);
                idx3 = _mm_extract_epi32(idxB, 3);
                _mm_store_si128(&p_codec->simd.mo_comp.B, _MM_SET_EPI16_SSE41(cSb[idx3], cSb[idx2], cSb[idx1], cSb[idx0], cSr[idx3], cSr[idx2], cSr[idx1], cSr[idx0]));
                idx0 = _mm_extract_epi32(idxC, 0);
                idx1 = _mm_extract_epi32(idxC, 1);
                idx2 = _mm_extract_epi32(idxC, 2);
                idx3 = _mm_extract_epi32(idxC, 3);
                _mm_store_si128(&p_codec->simd.mo_comp.C, _MM_SET_EPI16_SSE41(cSb[idx3], cSb[idx2], cSb[idx1], cSb[idx0], cSr[idx3], cSr[idx2], cSr[idx1], cSr[idx0]));
                idx0 = _mm_extract_epi32(idxD, 0);
                idx1 = _mm_extract_epi32(idxD, 1);
                idx2 = _mm_extract_epi32(idxD, 2);
                idx3 = _mm_extract_epi32(idxD, 3);
                _mm_store_si128(&p_codec->simd.mo_comp.D, _MM_SET_EPI16_SSE41(cSb[idx3], cSb[idx2], cSb[idx1], cSb[idx0], cSr[idx3], cSr[idx2], cSr[idx1], cSr[idx0]));

                _mm_store_si128(&tmp8, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(_8minus_xFracC_per_8minus_yFracC, p_codec->simd.mo_comp.A), _mm_mullo_epi16(_8minus_yFracC_per_xFracC, p_codec->simd.mo_comp.B)),
                                                      _mm_add_epi16(_mm_mullo_epi16(_8minus_xFracC_per_yFracC, p_codec->simd.mo_comp.C), _mm_mullo_epi16(_xFracC_per_yFracC, p_codec->simd.mo_comp.D))), _32), 6));

                predPartLXCb[yC][xC] = _mm_extract_epi16(tmp8, 4);
                predPartLXCb[yC][xC + 1] = _mm_extract_epi16(tmp8, 5);
                predPartLXCb[yC + 1][xC] = _mm_extract_epi16(tmp8, 6);
                predPartLXCb[yC + 1][xC + 1] = _mm_extract_epi16(tmp8, 7);
                predPartLXCr[yC][xC] = _mm_extract_epi16(tmp8, 0);
                predPartLXCr[yC][xC + 1] = _mm_extract_epi16(tmp8, 1);
                predPartLXCr[yC + 1][xC] = _mm_extract_epi16(tmp8, 2);
                predPartLXCr[yC + 1][xC + 1] = _mm_extract_epi16(tmp8, 3);

                if ((xC + 2) < partWidthC) {
                    _mm_store_si128(&xIntC, _mm_add_epi32(xIntC, _2222));
                }
            }
            if ((yC + 2) < partHeightC) {
                _mm_store_si128(&_yIntC, _mm_add_epi32(_yIntC, _chroma_width_m2));
            }
        }
    }
    else if(p_codec->sps.pc_active->ChromaArrayType == 2) {
        __m128i _xFracC_per_yFracC, _8minus_xFracC, _8minus_yFracC, _8minus_xFracC_per_8minus_yFracC, _8minus_xFracC_per_yFracC, _8minus_yFracC_per_xFracC;
        xFracC = mvCLX->x & 7; // (8-235)
        yFracC = (mvCLX->y & 3) << 1; // (8-236)
        _xIntC = _MM_SET1_EPI32_SSE41((p_mb->xL_Idx >> p_codec->sps.pc_active->SubWidthC_TrailingZeros) + (mvCLX->x >> 3));
        _yIntC = _MM_SET1_EPI32_SSE41(((p_mb->yL_Idx >> p_codec->sps.pc_active->SubHeightC_TrailingZeros) + (mvCLX->y >> 2)) * chroma_width);

        _xFracC_per_yFracC = (__xFracC_per_yFracC[yFracC][xFracC]);
        _8minus_xFracC = (__8minus_xFracC[yFracC][xFracC]);
        _8minus_yFracC = (__8minus_yFracC[yFracC][xFracC]);
        _8minus_xFracC_per_8minus_yFracC = (__8minus_xFracC_per_8minus_yFracC[yFracC][xFracC]);
        _8minus_xFracC_per_yFracC = (__8minus_xFracC_per_yFracC[yFracC][xFracC]);
        _8minus_yFracC_per_xFracC = (__8minus_yFracC_per_xFracC[yFracC][xFracC]);

        for(yC = 0; yC < partHeightC; yC+=2) {
            _mm_store_si128(&yIntC, _mm_add_epi32(_yIntC, _0011)); // (8-230)
            _mm_store_si128(&xIntC, _mm_add_epi32(_xIntC, _0101)); // (8-229)
            p_codec->simd.mo_comp.yAC = HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxY, yIntC); // (8-268)
            p_codec->simd.mo_comp.yCC = HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxY, _mm_add_epi32(yIntC, _chroma_width)); // (8-270)
            for(xC = 0; xC < partWidthC; xC+=2) {
                _mm_store_si128(&p_codec->simd.mo_comp.xAC, HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxX, xIntC)); // (8-264)
                _mm_store_si128(&p_codec->simd.mo_comp.xBC, HL_MATH_CLIP3_INTRIN_SSE41(_0000, maxX, _mm_add_epi32(xIntC, _1111))); // (8-265)

                // yDC = yCC and yBC = yAC and xCC = xAC and xDC = xBC
                _mm_store_si128(&idxA, _mm_add_epi32(p_codec->simd.mo_comp.xAC, p_codec->simd.mo_comp.yAC));
                _mm_store_si128(&idxB, _mm_add_epi32(p_codec->simd.mo_comp.xBC, p_codec->simd.mo_comp.yAC));
                _mm_store_si128(&idxC, _mm_add_epi32(p_codec->simd.mo_comp.xAC, p_codec->simd.mo_comp.yCC));
                _mm_store_si128(&idxD, _mm_add_epi32(p_codec->simd.mo_comp.xBC, p_codec->simd.mo_comp.yCC));

                // 8.4.2.2.2 Chroma sample interpolation process

                _mm_store_si128(&p_codec->simd.mo_comp.A, _mm_set_epi16(cSb[_mm_extract_epi32(idxA, 3)], cSb[_mm_extract_epi32(idxA, 2)], cSb[_mm_extract_epi32(idxA, 1)], cSb[_mm_extract_epi32(idxA, 0)],
                                cSr[idxA.m128i_i32[3]], cSr[_mm_extract_epi32(idxA, 2)], cSr[_mm_extract_epi32(idxA, 1)], cSr[_mm_extract_epi32(idxA, 0)]));
                _mm_store_si128(&p_codec->simd.mo_comp.B, _mm_set_epi16(cSb[_mm_extract_epi32(idxB, 3)], cSb[_mm_extract_epi32(idxB, 2)], cSb[_mm_extract_epi32(idxB, 1)], cSb[_mm_extract_epi32(idxB, 0)],
                                cSr[_mm_extract_epi32(idxB, 3)], cSr[_mm_extract_epi32(idxB, 2)], cSr[_mm_extract_epi32(idxB, 1)], cSr[_mm_extract_epi32(idxB, 0)]));
                _mm_store_si128(&p_codec->simd.mo_comp.C, _mm_set_epi16(cSb[_mm_extract_epi32(idxC, 3)], cSb[_mm_extract_epi32(idxC, 2)], cSb[_mm_extract_epi32(idxC, 1)], cSb[_mm_extract_epi32(idxC, 0)],
                                cSr[_mm_extract_epi32(idxC, 3)], cSr[_mm_extract_epi32(idxC, 2)], cSr[_mm_extract_epi32(idxC, 1)], cSr[_mm_extract_epi32(idxC, 0)]));
                _mm_store_si128(&p_codec->simd.mo_comp.D, _mm_set_epi16(cSb[_mm_extract_epi32(idxD, 3)], cSb[_mm_extract_epi32(idxD, 2)], cSb[_mm_extract_epi32(idxD, 1)], cSb[_mm_extract_epi32(idxD, 0)],
                                cSr[_mm_extract_epi32(idxD, 3)], cSr[_mm_extract_epi32(idxD, 2)], cSr[_mm_extract_epi32(idxD, 1)], cSr[_mm_extract_epi32(idxD, 0)]));
                tmp8 = _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(_8minus_xFracC_per_8minus_yFracC, p_codec->simd.mo_comp.A), _mm_mullo_epi16(_8minus_yFracC_per_xFracC, p_codec->simd.mo_comp.B)),
                                                    _mm_add_epi16(_mm_mullo_epi16(_8minus_xFracC_per_yFracC, p_codec->simd.mo_comp.C), _mm_mullo_epi16(_xFracC_per_yFracC, p_codec->simd.mo_comp.D))), _32), 6);

                predPartLXCb[yC][xC] = HL_INT128_I16(&tmp8, 4);
                predPartLXCb[yC][xC + 1] = HL_INT128_I16(&tmp8, 5);
                predPartLXCb[yC + 1][xC] = HL_INT128_I16(&tmp8, 6);
                predPartLXCb[yC + 1][xC + 1] = HL_INT128_I16(&tmp8, 7);
                predPartLXCr[yC][xC] = HL_INT128_I16(&tmp8, 0);
                predPartLXCr[yC][xC + 1] = HL_INT128_I16(&tmp8, 1);
                predPartLXCr[yC + 1][xC] = HL_INT128_I16(&tmp8, 2);
                predPartLXCr[yC + 1][xC + 1] = HL_INT128_I16(&tmp8, 3);

                if ((xC + 2) < partWidthC) {
                    _mm_store_si128(&xIntC, _mm_add_epi32(xIntC, _2222));
                }
            }

            if ((yC + 2) < partHeightC) {
                _mm_store_si128(&_yIntC, _mm_add_epi32(_yIntC, _chroma_width_m2));
            }
        }
    }
    else if(p_codec->sps.pc_active->ChromaArrayType == 3) {
#if 1
        extern HL_ERROR_T hl_codec_264_interpol_luma(
            hl_codec_264_t* p_codec,
            hl_codec_264_mb_t* p_mb,
            int32_t mbPartIdx,
            int32_t subMbPartIdx,
            const hl_codec_264_mv_xt* mvLX,
            const hl_codec_264_pict_t* refPicLXL,
            HL_OUT_ALIGNED(16) int32_t predPartLXL[16][16], int32_t predPartLXLSampleSize);
        hl_codec_264_interpol_luma(p_codec, p_mb,
                                   mbPartIdx,subMbPartIdx,
                                   mvLX,
                                   refPicLXCb, predPartLXCb);
        hl_codec_264_interpol_luma(p_codec, p_mb,
                                   mbPartIdx,subMbPartIdx,
                                   mvLX,
                                   refPicLXCr, predPartLXCr);
#else
        xFracC = (mvCLX->x & 3);// (8-239)
        yFracC = (mvCLX->y & 3);// (8-240)
        for(yC=0; yC<p_mb->partHeightC[mbPartIdx][subMbPartIdx]; ++yC) {
            for(xC=0; xC<p_mb->partWidthC[mbPartIdx][subMbPartIdx]; ++xC) {
                xIntC = p_mb->xL_Idx + (mvLX->x >> 2) + xC;// (8-237)
                yIntC = p_mb->yL_Idx + (mvLX->y >> 2) + yC;// (8-238)

                // 8.4.2.2.1 Luma sample interpolation process
                predPartLXCb[yC][xC] = hl264PredInter_LumaSampleInterpolate(p_codec, p_mb, xIntC, yIntC, xFracC, yFracC, mbPartIdx, subMbPartIdx, refPicLXCb);
                predPartLXCr[yC][xC] = hl264PredInter_LumaSampleInterpolate(p_codec, p_mb, xIntC, yIntC, xFracC, yFracC, mbPartIdx, subMbPartIdx, refPicLXCr);
            }
        }
#endif
    }

    return HL_ERROR_SUCCESS;
}
#endif

#endif /* HL_HAVE_X86_INTRIN */

HL_END_DECLS

#endif /* _HARTALLO_CODEC_X86_264_INTERPOL_INTRIN_H_ */
