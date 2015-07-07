#include "hartallo/h264/intrinsics/x86/hl_codec_x86_264_interpol_intrin.h"

/* == MORE CODE IN HEADER FILE == */


#if HL_HAVE_X86_INTRIN

// TODO: add ASM version
void hl_codec_x86_264_interpol_chroma_cat1_u8_intrin_sse2(
    hl_codec_264_t* p_codec,
    const hl_pixel_t* cSCb_u8, const hl_pixel_t* cSCr_u8,
    int32_t xFracC, int32_t yFracC,
    int32_t _xIntC, int32_t _yIntC, int32_t partWidthC, int32_t partHeightC, int32_t i_pic_height, int32_t i_pic_width,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16])
{
    int32_t yC, xC, yIntC, xIntC;

    HL_ALIGNED(16) uint8_t (*retCb_u8)[4/*A=0,B=1,C=2,D=3*/][16];
    HL_ALIGNED(16) uint8_t (*retCr_u8)[4/*A=0,B=1,C=2,D=3*/][16];

    __m128i xmm_retCb_u8[4/*A=0,B=1,C=2,D=3*/], xmm_retCr_u8[4/*A=0,B=1,C=2,D=3*/];
    __m128i xmm_xFracC_per_yFracC, xmm_8minus_xFracC, xmm_8minus_yFracC, xmm_8minus_xFracC_per_8minus_yFracC, xmm_8minus_xFracC_per_yFracC, xmm_8minus_yFracC_per_xFracC;
    __m128i xmm_tmp;

#define IDX_A 0
#define IDX_B 1
#define IDX_C 2
#define IDX_D 3

    static hl_bool_t __sse_initialized = HL_FALSE;
    static __m128i __sse_xFracC_per_yFracC[8][8];
    static __m128i __sse_8minus_xFracC[8][8];
    static __m128i __sse_8minus_yFracC[8][8];
    static __m128i __sse_8minus_xFracC_per_8minus_yFracC[8][8];
    static __m128i __sse_8minus_xFracC_per_yFracC[8][8];
    static __m128i __sse_8minus_yFracC_per_xFracC[8][8];
    static __m128i __sse_32;
    static __m128i __sse_0;

    (p_codec);

    if (!__sse_initialized) {
        int32_t xFracC, yFracC;
        for (yFracC = 0; yFracC < 8; ++yFracC) {
            for (xFracC = 0; xFracC < 8; ++xFracC) {
                _mm_store_si128(&__sse_xFracC_per_yFracC[yFracC][xFracC], _mm_set1_epi16(xFracC * yFracC));
                _mm_store_si128(&__sse_8minus_xFracC[yFracC][xFracC], _mm_set1_epi16(8 - xFracC));
                _mm_store_si128(&__sse_8minus_yFracC[yFracC][xFracC], _mm_set1_epi16(8 - yFracC));
                _mm_store_si128(&__sse_8minus_xFracC_per_8minus_yFracC[yFracC][xFracC], _mm_set1_epi16((8 - xFracC) * (8 - yFracC)));
                _mm_store_si128(&__sse_8minus_xFracC_per_yFracC[yFracC][xFracC], _mm_set1_epi16((8 - xFracC) * yFracC));
                _mm_store_si128(&__sse_8minus_yFracC_per_xFracC[yFracC][xFracC], _mm_set1_epi16((8 - yFracC) * xFracC));
            }
        }
        _mm_store_si128(&__sse_32, _mm_set1_epi16(32));
        _mm_store_si128(&__sse_0, _mm_setzero_si128());

        __sse_initialized = HL_TRUE;
    }

    retCb_u8 = (uint8_t (*)[4][16])&xmm_retCb_u8;
    retCr_u8 = (uint8_t (*)[4][16])&xmm_retCr_u8;

    _mm_store_si128(&xmm_xFracC_per_yFracC, (__sse_xFracC_per_yFracC[yFracC][xFracC]));
    _mm_store_si128(&xmm_8minus_xFracC, (__sse_8minus_xFracC[yFracC][xFracC]));
    _mm_store_si128(&xmm_8minus_yFracC, (__sse_8minus_yFracC[yFracC][xFracC]));
    _mm_store_si128(&xmm_8minus_xFracC_per_8minus_yFracC, (__sse_8minus_xFracC_per_8minus_yFracC[yFracC][xFracC]));
    _mm_store_si128(&xmm_8minus_xFracC_per_yFracC, (__sse_8minus_xFracC_per_yFracC[yFracC][xFracC]));
    _mm_store_si128(&xmm_8minus_yFracC_per_xFracC, (__sse_8minus_yFracC_per_xFracC[yFracC][xFracC]));

    for (yC = 0; yC < partHeightC; yC+=4) {
        yIntC = _yIntC + yC; // (8-230)
        for (xC = 0; xC < partWidthC; xC+=4) {
            xIntC = _xIntC + xC; // (8-229)
            hl_codec_264_interpol_load_samples4x4_chroma_u8(cSCb_u8, (*retCb_u8), cSCr_u8, (*retCr_u8), xIntC, yIntC, i_pic_height, i_pic_width);

            /* 8.4.2.2.2 Chroma sample interpolation process */
            // == CB == //
            _mm_store_si128((__m128i *)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_8minus_yFracC, _mm_unpacklo_epi8(xmm_retCb_u8[IDX_A], __sse_0)), _mm_mullo_epi16(xmm_8minus_yFracC_per_xFracC, _mm_unpacklo_epi8(xmm_retCb_u8[IDX_B], __sse_0))),
                            _mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_yFracC, _mm_unpacklo_epi8(xmm_retCb_u8[IDX_C], __sse_0)), _mm_mullo_epi16(xmm_xFracC_per_yFracC, _mm_unpacklo_epi8(xmm_retCb_u8[IDX_D], __sse_0)))), __sse_32), 6));
            _mm_store_si128((__m128i *)&predPartLXCb[yC][xC], _mm_unpacklo_epi16(xmm_tmp, __sse_0));
            _mm_store_si128((__m128i *)&predPartLXCb[yC + 1][xC], _mm_unpackhi_epi16(xmm_tmp, __sse_0));
            _mm_store_si128((__m128i *)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_8minus_yFracC, _mm_unpackhi_epi8(xmm_retCb_u8[IDX_A], __sse_0)), _mm_mullo_epi16(xmm_8minus_yFracC_per_xFracC, _mm_unpackhi_epi8(xmm_retCb_u8[IDX_B], __sse_0))),
                            _mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_yFracC, _mm_unpackhi_epi8(xmm_retCb_u8[IDX_C], __sse_0)), _mm_mullo_epi16(xmm_xFracC_per_yFracC, _mm_unpackhi_epi8(xmm_retCb_u8[IDX_D], __sse_0)))), __sse_32), 6));
            _mm_store_si128((__m128i *)&predPartLXCb[yC + 2][xC], _mm_unpacklo_epi16(xmm_tmp, __sse_0));
            _mm_store_si128((__m128i *)&predPartLXCb[yC + 3][xC], _mm_unpackhi_epi16(xmm_tmp, __sse_0));
            // == CR == //
            _mm_store_si128((__m128i *)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_8minus_yFracC, _mm_unpacklo_epi8(xmm_retCr_u8[IDX_A], __sse_0)), _mm_mullo_epi16(xmm_8minus_yFracC_per_xFracC, _mm_unpacklo_epi8(xmm_retCr_u8[IDX_B], __sse_0))),
                            _mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_yFracC, _mm_unpacklo_epi8(xmm_retCr_u8[IDX_C], __sse_0)), _mm_mullo_epi16(xmm_xFracC_per_yFracC, _mm_unpacklo_epi8(xmm_retCr_u8[IDX_D], __sse_0)))), __sse_32), 6));
            _mm_store_si128((__m128i *)&predPartLXCr[yC][xC], _mm_unpacklo_epi16(xmm_tmp, __sse_0));
            _mm_store_si128((__m128i *)&predPartLXCr[yC + 1][xC], _mm_unpackhi_epi16(xmm_tmp, __sse_0));
            _mm_store_si128((__m128i *)&xmm_tmp, _mm_srai_epi16(_mm_add_epi16(_mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_8minus_yFracC, _mm_unpackhi_epi8(xmm_retCr_u8[IDX_A], __sse_0)), _mm_mullo_epi16(xmm_8minus_yFracC_per_xFracC, _mm_unpackhi_epi8(xmm_retCr_u8[IDX_B], __sse_0))),
                            _mm_add_epi16(_mm_mullo_epi16(xmm_8minus_xFracC_per_yFracC, _mm_unpackhi_epi8(xmm_retCr_u8[IDX_C], __sse_0)), _mm_mullo_epi16(xmm_xFracC_per_yFracC, _mm_unpackhi_epi8(xmm_retCr_u8[IDX_D], __sse_0)))), __sse_32), 6));
            _mm_store_si128((__m128i *)&predPartLXCr[yC + 2][xC], _mm_unpacklo_epi16(xmm_tmp, __sse_0));
            _mm_store_si128((__m128i *)&predPartLXCr[yC + 3][xC], _mm_unpackhi_epi16(xmm_tmp, __sse_0));
        }
    }
}

#endif /* HL_HAVE_X86_INTRIN */