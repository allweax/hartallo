#ifndef _HARTALLO_CODEC_264_INTERPOL_H_
#define _HARTALLO_CODEC_264_INTERPOL_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"
#include "hartallo/h264/hl_codec_264_macros.h"

HL_BEGIN_DECLS

struct hl_codec_264_interpol_indices_s;
struct hl_codec_264_s;
struct hl_codec_264_mv_xs;
struct hl_codec_264_pict_s;
struct hl_codec_264_mb_s;

HL_ERROR_T hl_codec_264_interpol_indices_create(struct hl_codec_264_interpol_indices_s** pp_obj, uint32_t u_width, uint32_t u_height, hl_bool_t b_mbaff);
void hl_codec_264_interpol_load_samples4x4_u8_cpp(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/);
extern void (*hl_codec_264_interpol_load_samples4x4_u8)(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/);
void hl_codec_264_interpol_load_samples4x4_chroma_u8_cpp(const hl_pixel_t* cSCb_u8, HL_ALIGNED(16) uint8_t retCb_u8[4/*A=0,B=1,C=2,D=3*/][16], const hl_pixel_t* cSCr_u8, HL_ALIGNED(16) uint8_t retCr_u8[4/*A=0,B=1,C=2,D=3*/][16], int32_t i_x0, int32_t i_y0, int32_t i_height, int32_t i_width);
extern void (*hl_codec_264_interpol_load_samples4x4_chroma_u8)(const hl_pixel_t* cSCb_u8, HL_ALIGNED(16) uint8_t retCb_u8[4/*A=0,B=1,C=2,D=3*/][16], const hl_pixel_t* cSCr_u8, HL_ALIGNED(16) uint8_t retCr_u8[4/*A=0,B=1,C=2,D=3*/][16], int32_t i_x0, int32_t i_y0, int32_t i_height, int32_t i_width);
void hl_codec_264_interpol_load_samples4_u32(const uint32_t* pc_indices, const hl_pixel_t* cSL_u8, uint32_t* ret_u32/*[4]*/);
extern void (*hl_codec_264_interpol_chroma_cat1_u8)(struct hl_codec_264_s* p_codec, const hl_pixel_t* cSCb_u8, const hl_pixel_t* cSCr_u8, int32_t xFracC, int32_t yFracC, int32_t _xIntC, int32_t _yIntC, int32_t partWidthC, int32_t partHeightC, int32_t i_pic_height, int32_t i_pic_width, HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16], HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);
HL_ERROR_T hl_codec_264_interpol_init_functions();

typedef struct hl_codec_264_interpol_indices_s {
    HL_DECLARE_OBJECT;

    uint32_t u_width;
    uint32_t u_height;
    uint32_t u_stride;
    hl_bool_t b_mbaff;
    uint32_t* pc_indices; // padded, must no be freed()
    uint32_t* p_indices_unpadded;
}
hl_codec_264_interpol_indices_t;

// Perform Tap6Filter without rounding
HL_ALWAYS_INLINE static int32_t hl_codec_interpol_264_tap6filter_vert(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL
)
{
    return Tap6Filter(cSL[pc_indices[0]], cSL[pc_indices[i_indices_stride]], cSL[pc_indices[i_indices_stride<<1]], cSL[pc_indices[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices[i_indices_stride<<2]], cSL[pc_indices[(i_indices_stride<<2)+i_indices_stride]]);
}

// Full Tap6Filter
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Clip(((Tap6Filter + 16) >> 5))
HL_ALWAYS_INLINE static void hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    int32_t ret[4][4] // results
)
{
    int32_t i = 0;
    const uint32_t* pc_indices_start;
    for (i = 0; i < 4; ++i) {
        pc_indices_start = (pc_indices + i);
        ret[0][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        ret[0][i] = HL_MATH_CLIP2(255, ((ret[0][i] + 16) >> 5));// (8-246)
        pc_indices_start+=i_indices_stride;
        ret[1][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        ret[1][i] = HL_MATH_CLIP2(255, ((ret[1][i] + 16) >> 5));
        pc_indices_start+=i_indices_stride;
        ret[2][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        ret[2][i] = HL_MATH_CLIP2(255, ((ret[2][i] + 16) >> 5));
        pc_indices_start+=i_indices_stride;
        ret[3][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        ret[3][i] = HL_MATH_CLIP2(255, ((ret[3][i] + 16) >> 5));
    }
}

// Partial Tap6Filter, without clipping
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Tap6Filter
HL_ALWAYS_INLINE static void hl_codec_interpol_264_tap6filter_vert4x4_cpp(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    int32_t ret[4][4] // results
)
{
    int32_t i = 0;
    const uint32_t* pc_indices_start;
    for (i = 0; i < 4; ++i) {
        pc_indices_start = (pc_indices + i);
        ret[0][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        pc_indices_start+=i_indices_stride;
        ret[1][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        pc_indices_start+=i_indices_stride;
        ret[2][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
        pc_indices_start+=i_indices_stride;
        ret[3][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<1]], cSL[pc_indices_start[(i_indices_stride<<1)+i_indices_stride]], cSL[pc_indices_start[i_indices_stride<<2]], cSL[pc_indices_start[(i_indices_stride<<2)+i_indices_stride]]);
    }
}

// Partial Tap6Filter
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Clip(((Tap6Filter + 16) >> 5))
HL_ALWAYS_INLINE static int32_t hl_codec_interpol_264_tap6filter_horiz(
    const uint32_t* pc_indices,
    const hl_pixel_t* cSL
)
{
    return Tap6Filter(cSL[pc_indices[0]], cSL[pc_indices[1]], cSL[pc_indices[2]], cSL[pc_indices[3]], cSL[pc_indices[4]], cSL[pc_indices[5]]);
}

HL_ALWAYS_INLINE static void hl_codec_interpol_264_tap6filter_horiz4x4_cpp(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    int32_t ret[4][4] // results
)
{
    int32_t i = 0;
    const uint32_t* pc_indices_start;
    for (i = 0; i < 4; ++i) {
        pc_indices_start = (pc_indices + i);
        ret[0][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        pc_indices_start+=i_indices_stride;
        ret[1][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        pc_indices_start+=i_indices_stride;
        ret[2][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        pc_indices_start+=i_indices_stride;
        ret[3][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
    }
}

// Full Tap6Filter
// Tap6Filter = (E - 5F + 20G + 20H - 5I + J) = (E - 5(F + I) + 20(G + H) + J)
// RET = Clip(((Tap6Filter + 16) >> 5))
HL_ALWAYS_INLINE static void hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    int32_t ret[4][4] // results
)
{
    int32_t i = 0;
    const uint32_t* pc_indices_start;
    for (i = 0; i < 4; ++i) {
        pc_indices_start = (pc_indices + i);
        ret[0][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        ret[0][i] = HL_MATH_CLIP2(255, ((ret[0][i] + 16) >> 5));// (8-246)
        pc_indices_start+=i_indices_stride;
        ret[1][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        ret[1][i] = HL_MATH_CLIP2(255, ((ret[1][i] + 16) >> 5));// (8-246)
        pc_indices_start+=i_indices_stride;
        ret[2][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        ret[2][i] = HL_MATH_CLIP2(255, ((ret[2][i] + 16) >> 5));// (8-246)
        pc_indices_start+=i_indices_stride;
        ret[3][i] = Tap6Filter(cSL[pc_indices_start[0]], cSL[pc_indices_start[1]], cSL[pc_indices_start[2]], cSL[pc_indices_start[3]], cSL[pc_indices_start[4]], cSL[pc_indices_start[5]]);
        ret[3][i] = HL_MATH_CLIP2(255, ((ret[3][i] + 16) >> 5));// (8-246)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma00_horiz4_cpp(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL
)
{
    predPartLXL[0] = cSL[pc_indices_horiz[0]];
    predPartLXL[1] = cSL[pc_indices_horiz[1]];
    predPartLXL[2] = cSL[pc_indices_horiz[2]];
    predPartLXL[3] = cSL[pc_indices_horiz[3]];
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma00_horiz4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
#define MOVE_SAMPLE(i,j) ((uint8_t*)predPartLXL16x1)[(i)] = cSL_u8[pc_indices_horiz[(j)]]
    MOVE_SAMPLE(0,0);
    MOVE_SAMPLE(1,1);
    MOVE_SAMPLE(2,2);
    MOVE_SAMPLE(3,3);
    pc_indices_horiz+=i_indices_stride;
    MOVE_SAMPLE(4,0);
    MOVE_SAMPLE(5,1);
    MOVE_SAMPLE(6,2);
    MOVE_SAMPLE(7,3);
    pc_indices_horiz+=i_indices_stride;
    MOVE_SAMPLE(8,0);
    MOVE_SAMPLE(9,1);
    MOVE_SAMPLE(10,2);
    MOVE_SAMPLE(11,3);
    pc_indices_horiz+=i_indices_stride;
    MOVE_SAMPLE(12,0);
    MOVE_SAMPLE(13,1);
    MOVE_SAMPLE(14,2);
    MOVE_SAMPLE(15,3);
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma01_vert4_cpp(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t G,h1,h,i;
    for (i = 0; i < 4; ++i) {
        G = cSL[pc_indices_vert[(i_indices_stride << 1)]];
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
        h = HL_MATH_CLIP2(MaxPixelValueY[i], ((h1 + 16) >> 5));// (8-246)
        predPartLXL[i] = (G + h + 1) >> 1;// (8-254)
        ++pc_indices_vert;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma01_vert4x4_u8_cpp(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t G, h[4][4], i, j;
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL, h);
    for (j = 0; j < 4; ++j) {
        for (i = 0; i < 4; ++i) {
            G = cSL[(pc_indices_vert + i)[(i_indices_stride << 1)]];
            predPartLXL16x1[i] = (G + h[j][i] + 1) >> 1;// (8-254)
        }
        pc_indices_vert += i_indices_stride;
        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma02_vert4_cpp(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t h1, i;
    for (i = 0; i < 4; ++i) {
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
        predPartLXL[i] = HL_MATH_CLIP2(MaxPixelValueY[i], ((h1 + 16) >> 5));// (8-246)
        ++pc_indices_vert;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma02_vert4x4_u8_cpp(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t h[4][4];
    // h = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL, h);
    predPartLXL16x1[0] = h[0][0];
    predPartLXL16x1[1] = h[0][1];
    predPartLXL16x1[2] = h[0][2];
    predPartLXL16x1[3] = h[0][3];
    predPartLXL16x1[4] = h[1][0];
    predPartLXL16x1[5] = h[1][1];
    predPartLXL16x1[6] = h[1][2];
    predPartLXL16x1[7] = h[1][3];
    predPartLXL16x1[8] = h[2][0];
    predPartLXL16x1[9] = h[2][1];
    predPartLXL16x1[10] = h[2][2];
    predPartLXL16x1[11] = h[2][3];
    predPartLXL16x1[12] = h[3][0];
    predPartLXL16x1[13] = h[3][1];
    predPartLXL16x1[14] = h[3][2];
    predPartLXL16x1[15] = h[3][3];
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma03_vert4_cpp(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t h1,h,i;
    for (i = 0; i < 4; ++i) {
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
        h = HL_MATH_CLIP2(MaxPixelValueY[i], ((h1 + 16) >> 5));// (8-246)
        predPartLXL[i] = (cSL[pc_indices_vert[(i_indices_stride * 3)]] + h + 1) >> 1;// (8-255)
        ++pc_indices_vert;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma03_vert4x4_u8_cpp(
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t h[4][4], j, i_indices_stride_mul3 = (i_indices_stride * 3);
    // h = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL, h);
    for (j = 0; j < 4; ++j) {
        predPartLXL16x1[0] = (cSL[(pc_indices_vert + 0)[i_indices_stride_mul3]] + h[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (cSL[(pc_indices_vert + 1)[i_indices_stride_mul3]] + h[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (cSL[(pc_indices_vert + 2)[i_indices_stride_mul3]] + h[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (cSL[(pc_indices_vert + 3)[i_indices_stride_mul3]] + h[j][3] + 1) >> 1;
        pc_indices_vert += i_indices_stride;
        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma10_horiz4_cpp(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t i,G,b1,b;
    for (i = 0; i < 4; ++i) {
        G = cSL[pc_indices_horiz[2]];
        b1 = hl_codec_interpol_264_tap6filter_horiz(pc_indices_horiz, cSL);
        b = HL_MATH_CLIP2(MaxPixelValueY[i], ((b1 + 16) >> 5));// (8-245)
        predPartLXL[i] = (G + b + 1 ) >> 1;// (8-252)
        ++pc_indices_horiz;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma10_horiz4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t j,b[4][4];

    // b = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL, b);
    // RET = ((G + b + 1 ) >> 1)
    for (j = 0; j < 4; ++j) {
        predPartLXL16x1[0] = (cSL[(pc_indices_horiz + 0)[2]] + b[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (cSL[(pc_indices_horiz + 1)[2]] + b[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (cSL[(pc_indices_horiz + 2)[2]] + b[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (cSL[(pc_indices_horiz + 3)[2]] + b[j][3] + 1) >> 1;
        pc_indices_horiz += i_indices_stride;
        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma11_diag4_cpp(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t i,h1,h,b1,b;
    for (i = 0; i < 4; ++i) {
        // vert
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert, i_indices_stride, cSL);
        h = HL_MATH_CLIP2(MaxPixelValueY[i], ((h1 + 16) >> 5));// (8-246)

        // horiz
        b1 = hl_codec_interpol_264_tap6filter_horiz(pc_indices_horiz, cSL);
        b = HL_MATH_CLIP2(MaxPixelValueY[i], ((b1 + 16) >> 5));// (8-245)

        predPartLXL[i] = (b + h + 1) >> 1;// (8-260)

        ++pc_indices_vert;
        ++pc_indices_horiz;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma11_diag4x4_u8_cpp(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t j,h[4][4],b[4][4];

    // h = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL, h);
    // b = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL, b);
    for (j = 0; j < 4; ++j) {
        predPartLXL16x1[0] = (b[j][0] + h[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (b[j][1] + h[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (b[j][2] + h[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (b[j][3] + h[j][3] + 1) >> 1;
        predPartLXL16x1 += 4;
    }
}


HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma12_vert4_cpp(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t h1,h,cc,dd,m1,ee,ff,j1,j,i;
    for (i = 0; i < 4; ++i) {
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[0] + i, i_indices_stride, cSL);
        h = HL_MATH_CLIP2(MaxPixelValueY[i], ((h1 + 16) >> 5));// (8-246)

        cc = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[1] + i, i_indices_stride, cSL);
        dd = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[2] + i, i_indices_stride, cSL);
        m1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[3] + i, i_indices_stride, cSL);
        ee = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[4] + i, i_indices_stride, cSL);
        ff = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[5] + i, i_indices_stride, cSL);
        j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);
        j = HL_MATH_CLIP2(MaxPixelValueY[i], ((j1 + 512) >> 10));

        predPartLXL[i] = (h + j + 1) >> 1;// (8-257)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma12_vert4x4_u8_cpp(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t h1[4][4],h[4][4],cc[4][4],dd[4][4],m1[4][4],ee[4][4],ff[4][4],j1[4][4],j[4][4],i;
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[0], i_indices_stride, cSL, h1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[1], i_indices_stride, cSL, cc);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[2], i_indices_stride, cSL, dd);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[3], i_indices_stride, cSL, m1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[4], i_indices_stride, cSL, ee);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[5], i_indices_stride, cSL, ff);
    for (i = 0; i < 4; ++i) {
        j1[i][0] = Tap6Filter(cc[i][0],dd[i][0],h1[i][0],m1[i][0],ee[i][0],ff[i][0]);
        j[i][0] = HL_MATH_CLIP2(255,((j1[i][0] + 512) >> 10));
        j1[i][1] = Tap6Filter(cc[i][1],dd[i][1],h1[i][1],m1[i][1],ee[i][1],ff[i][1]);
        j[i][1] = HL_MATH_CLIP2(255,((j1[i][1] + 512) >> 10));
        j1[i][2] = Tap6Filter(cc[i][2],dd[i][2],h1[i][2],m1[i][2],ee[i][2],ff[i][2]);
        j[i][2] = HL_MATH_CLIP2(255,((j1[i][2] + 512) >> 10));
        j1[i][3] = Tap6Filter(cc[i][3],dd[i][3],h1[i][3],m1[i][3],ee[i][3],ff[i][3]);
        j[i][3] = HL_MATH_CLIP2(255,((j1[i][3] + 512) >> 10));
        h[i][0] = HL_MATH_CLIP2(255, ((h1[i][0] + 16) >> 5));
        h[i][1] = HL_MATH_CLIP2(255, ((h1[i][1] + 16) >> 5));
        h[i][2] = HL_MATH_CLIP2(255, ((h1[i][2] + 16) >> 5));
        h[i][3] = HL_MATH_CLIP2(255, ((h1[i][3] + 16) >> 5));
        predPartLXL16x1[0] = (h[i][0] + j[i][0] + 1) >> 1;
        predPartLXL16x1[1] = (h[i][1] + j[i][1] + 1) >> 1;
        predPartLXL16x1[2] = (h[i][2] + j[i][2] + 1) >> 1;
        predPartLXL16x1[3] = (h[i][3] + j[i][3] + 1) >> 1;

        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma13_diag4_cpp(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t h,s,i;

    for (i = 0; i < 4; ++i) {
        h = hl_codec_interpol_264_tap6filter_vert(&pc_indices_vert[i], i_indices_stride, cSL);
        h = HL_MATH_CLIP2(MaxPixelValueY[i], ((h + 16) >> 5));// (8-246)

        s = hl_codec_interpol_264_tap6filter_horiz(&pc_indices_horiz[i], cSL);
        s = HL_MATH_CLIP2(MaxPixelValueY[i], ((s + 16) >> 5));

        predPartLXL[i] = (h + s + 1) >> 1;// (8-262)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma13_diag4x4_u8_cpp(
    const uint32_t* pc_indices_vert,
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t h[4][4],s[4][4],j;

    // h = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL_u8, h);
    // s = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, s);
    for (j = 0; j < 4; ++j) {
        predPartLXL16x1[0] = (s[j][0] + h[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (s[j][1] + h[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (s[j][2] + h[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (s[j][3] + h[j][3] + 1) >> 1;
        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma20_horiz4_cpp(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t b1,i;
    for (i = 0; i< 4; ++i) {
        b1 = hl_codec_interpol_264_tap6filter_horiz(pc_indices_horiz, cSL);
        predPartLXL[i] = HL_MATH_CLIP2(MaxPixelValueY[i], ((b1 + 16) >> 5));// (8-245)
        ++pc_indices_horiz;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma20_horiz4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t b1[4][4];
    // h = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, b1);
    // FIXME copy4x4()
    predPartLXL16x1[0] = b1[0][0];
    predPartLXL16x1[1] = b1[0][1];
    predPartLXL16x1[2] = b1[0][2];
    predPartLXL16x1[3] = b1[0][3];
    predPartLXL16x1[4] = b1[1][0];
    predPartLXL16x1[5] = b1[1][1];
    predPartLXL16x1[6] = b1[1][2];
    predPartLXL16x1[7] = b1[1][3];
    predPartLXL16x1[8] = b1[2][0];
    predPartLXL16x1[9] = b1[2][1];
    predPartLXL16x1[10] = b1[2][2];
    predPartLXL16x1[11] = b1[2][3];
    predPartLXL16x1[12] = b1[3][0];
    predPartLXL16x1[13] = b1[3][1];
    predPartLXL16x1[14] = b1[3][2];
    predPartLXL16x1[15] = b1[3][3];
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma21_diag4_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t i,h1,b,cc,dd,m1,ee,ff,j;
    for (i = 0; i< 4; ++i) {
        b = hl_codec_interpol_264_tap6filter_horiz(&pc_indices_horiz[i], cSL);
        b = HL_MATH_CLIP2(MaxPixelValueY[i], ((b + 16) >> 5)); // (8-245)
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[0] + i, i_indices_stride, cSL);

        cc = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[1] + i, i_indices_stride, cSL);
        dd = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[2] + i, i_indices_stride, cSL);
        m1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[3] + i, i_indices_stride, cSL);
        ee = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[4] + i, i_indices_stride, cSL);
        ff = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[5] + i, i_indices_stride, cSL);
        j = Tap6Filter(cc, dd, h1, m1, ee, ff);
        j = HL_MATH_CLIP2(MaxPixelValueY[i], ((j + 512) >> 10));

        predPartLXL[i] = (b + j + 1) >> 1;// (8-256)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma21_diag4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t i, b[4][4], h1[4][4], cc[4][4], dd[4][4], m1[4][4], ee[4][4], ff[4][4], j[4][4];
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, b);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[0], i_indices_stride, cSL_u8, h1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[1], i_indices_stride, cSL_u8, cc);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[2], i_indices_stride, cSL_u8, dd);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[3], i_indices_stride, cSL_u8, m1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[4], i_indices_stride, cSL_u8, ee);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[5], i_indices_stride, cSL_u8, ff);

    for (i = 0; i < 4; ++i) {
        j[i][0] = Tap6Filter(cc[i][0], dd[i][0], h1[i][0], m1[i][0], ee[i][0], ff[i][0]);
        j[i][0] = HL_MATH_CLIP2(255,((j[i][0] + 512) >> 10));
        j[i][1] = Tap6Filter(cc[i][1], dd[i][1], h1[i][1], m1[i][1], ee[i][1], ff[i][1]);
        j[i][1] = HL_MATH_CLIP2(255,((j[i][1] + 512) >> 10));
        j[i][2] = Tap6Filter(cc[i][2], dd[i][2], h1[i][2], m1[i][2], ee[i][2], ff[i][2]);
        j[i][2] = HL_MATH_CLIP2(255,((j[i][2] + 512) >> 10));
        j[i][3] = Tap6Filter(cc[i][3], dd[i][3], h1[i][3], m1[i][3], ee[i][3], ff[i][3]);
        j[i][3] = HL_MATH_CLIP2(255,((j[i][3] + 512) >> 10));

        predPartLXL16x1[0] = (b[i][0] + j[i][0] + 1) >> 1;
        predPartLXL16x1[1] = (b[i][1] + j[i][1] + 1) >> 1;
        predPartLXL16x1[2] = (b[i][2] + j[i][2] + 1) >> 1;
        predPartLXL16x1[3] = (b[i][3] + j[i][3] + 1) >> 1;

        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma22_vert4_cpp(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t h1,cc,dd,m1,ee,ff,j1,i;
    for (i = 0; i< 4; ++i) {
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[0] + i, i_indices_stride, cSL);
        cc = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[1] + i, i_indices_stride, cSL);
        dd = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[2] + i, i_indices_stride, cSL);
        m1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[3] + i, i_indices_stride, cSL);
        ee = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[4] + i, i_indices_stride, cSL);
        ff = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[5] + i, i_indices_stride, cSL);
        j1 = Tap6Filter(cc,dd,h1,m1,ee,ff);

        predPartLXL[i] = HL_MATH_CLIP2(MaxPixelValueY[i], ((j1 + 512) >> 10));
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma22_vert4x4_u8_cpp(
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t h1[4][4], cc[4][4], dd[4][4], m1[4][4], ee[4][4], ff[4][4], j1, i;

    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[0], i_indices_stride, cSL_u8, h1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[1], i_indices_stride, cSL_u8, cc);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[2], i_indices_stride, cSL_u8, dd);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[3], i_indices_stride, cSL_u8, m1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[4], i_indices_stride, cSL_u8, ee);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[5], i_indices_stride, cSL_u8, ff);

    for (i = 0; i < 4; ++i) {
        j1 = Tap6Filter(cc[i][0], dd[i][0], h1[i][0], m1[i][0], ee[i][0], ff[i][0]);
        predPartLXL16x1[0] = HL_MATH_CLIP2(255, ((j1 + 512) >> 10));
        j1 = Tap6Filter(cc[i][1], dd[i][1], h1[i][1], m1[i][1], ee[i][1], ff[i][1]);
        predPartLXL16x1[1] = HL_MATH_CLIP2(255, ((j1 + 512) >> 10));
        j1 = Tap6Filter(cc[i][2], dd[i][2], h1[i][2], m1[i][2], ee[i][2], ff[i][2]);
        predPartLXL16x1[2] = HL_MATH_CLIP2(255, ((j1 + 512) >> 10));
        j1 = Tap6Filter(cc[i][3], dd[i][3], h1[i][3], m1[i][3], ee[i][3], ff[i][3]);
        predPartLXL16x1[3] = HL_MATH_CLIP2(255, ((j1 + 512) >> 10));

        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma23_diag4_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t s,h1,cc,dd,m1,ee,ff,j,i;
    for (i = 0; i< 4; ++i) {
        s = hl_codec_interpol_264_tap6filter_horiz(&pc_indices_horiz[i], cSL);
        s = HL_MATH_CLIP2(MaxPixelValueY[i], ((s + 16) >> 5));

        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[0] + i, i_indices_stride, cSL);
        cc = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[1] + i, i_indices_stride, cSL);
        dd = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[2] + i, i_indices_stride, cSL);
        m1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[3] + i, i_indices_stride, cSL);
        ee = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[4] + i, i_indices_stride, cSL);
        ff = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[5] + i, i_indices_stride, cSL);
        j = Tap6Filter(cc,dd,h1,m1,ee,ff);
        j = HL_MATH_CLIP2(MaxPixelValueY[i], ((j + 512) >> 10));

        predPartLXL[i] = (j + s + 1) >> 1;// (8-259)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma23_diag4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert[6],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t s[4][4], h1[4][4], cc[4][4], dd[4][4], m1[4][4], ee[4][4], ff[4][4], j, i;

    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, s);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[0], i_indices_stride, cSL_u8, h1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[1], i_indices_stride, cSL_u8, cc);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[2], i_indices_stride, cSL_u8, dd);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[3], i_indices_stride, cSL_u8, m1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[4], i_indices_stride, cSL_u8, ee);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[5], i_indices_stride, cSL_u8, ff);

    for (i = 0; i < 4; ++i) {
        j = Tap6Filter(cc[i][0], dd[i][0], h1[i][0], m1[i][0], ee[i][0], ff[i][0]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[0] = (j + s[i][0] + 1) >> 1;
        j = Tap6Filter(cc[i][1], dd[i][1], h1[i][1], m1[i][1], ee[i][1], ff[i][1]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[1] = (j + s[i][1] + 1) >> 1;
        j = Tap6Filter(cc[i][2], dd[i][2], h1[i][2], m1[i][2], ee[i][2], ff[i][2]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[2] = (j + s[i][2] + 1) >> 1;
        j = Tap6Filter(cc[i][3], dd[i][3], h1[i][3], m1[i][3], ee[i][3], ff[i][3]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[3] = (j + s[i][3] + 1) >> 1;

        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma30_horiz4_cpp(
    const uint32_t* pc_indices_horiz,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t b,i;
    for (i = 0; i< 4; ++i) {
        b = hl_codec_interpol_264_tap6filter_horiz(&pc_indices_horiz[i], cSL);
        b = HL_MATH_CLIP2(MaxPixelValueY[i], ((b + 16) >> 5));// (8-245)

        predPartLXL[i] = (cSL[pc_indices_horiz[3 + i]] + b + 1) >> 1;// (8-253)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma30_horiz4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t b[4][4],j;
    // b = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, b);

    for (j = 0; j< 4; ++j) {
        predPartLXL16x1[0] = (cSL_u8[pc_indices_horiz[3]] + b[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (cSL_u8[pc_indices_horiz[4]] + b[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (cSL_u8[pc_indices_horiz[5]] + b[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (cSL_u8[pc_indices_horiz[6]] + b[j][3] + 1) >> 1;
        predPartLXL16x1 += 4;
        pc_indices_horiz += i_indices_stride;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma31_diag4_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t b,m,i;
    for (i = 0; i < 4; ++i) {
        b = hl_codec_interpol_264_tap6filter_horiz(&pc_indices_horiz[i], cSL);
        b = HL_MATH_CLIP2(MaxPixelValueY[i], ((b + 16) >> 5));// (8-245)

        m = hl_codec_interpol_264_tap6filter_vert(&pc_indices_vert[i], i_indices_stride, cSL);
        m = HL_MATH_CLIP2(MaxPixelValueY[i], ((m + 16) >> 5));

        predPartLXL[i] = (b + m + 1) >> 1;// (8-261)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma31_diag4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t b[4][4], m[4][4], j;

    // b = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, b);
    // m = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL_u8, m);
    // (b + m + 1) >> 1
    for (j = 0; j < 4; ++j) {
        predPartLXL16x1[0] = (m[j][0] + b[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (m[j][1] + b[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (m[j][2] + b[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (m[j][3] + b[j][3] + 1) >> 1;
        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma32_vert4_cpp(
    const uint32_t* pc_indices_vert[7],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t h1,cc,dd,m1,ee,ff,j,i;
    for (i = 0; i < 4; ++i) {
        h1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[0] + i, i_indices_stride, cSL);
        cc = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[1] + i, i_indices_stride, cSL);
        dd = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[2] + i, i_indices_stride, cSL);
        m1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[3] + i, i_indices_stride, cSL);
        ee = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[4] + i, i_indices_stride, cSL);
        ff = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[5] + i, i_indices_stride, cSL);
        j = Tap6Filter(cc,dd,h1,m1,ee,ff);
        j = HL_MATH_CLIP2(MaxPixelValueY[i], ((j + 512) >> 10));

        m1 = hl_codec_interpol_264_tap6filter_vert(pc_indices_vert[6] + i, i_indices_stride, cSL);
        m1 = HL_MATH_CLIP2(MaxPixelValueY[i], ((m1 + 16) >> 5));

        predPartLXL[i] = (j + m1 + 1) >> 1;// (8-258)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma32_vert4x4_u8_cpp(
    const uint32_t* pc_indices_vert[7],
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t s[4][4], h1[4][4], cc[4][4], dd[4][4], m1[4][4], ee[4][4], ff[4][4], j, i;

    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert[6], i_indices_stride, cSL_u8, s);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[0], i_indices_stride, cSL_u8, h1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[1], i_indices_stride, cSL_u8, cc);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[2], i_indices_stride, cSL_u8, dd);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[3], i_indices_stride, cSL_u8, m1);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[4], i_indices_stride, cSL_u8, ee);
    hl_codec_interpol_264_tap6filter_vert4x4_cpp(pc_indices_vert[5], i_indices_stride, cSL_u8, ff);

    for (i = 0; i < 4; ++i) {
        j = Tap6Filter(cc[i][0], dd[i][0], h1[i][0], m1[i][0], ee[i][0], ff[i][0]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[0] = (j + s[i][0] + 1) >> 1;
        j = Tap6Filter(cc[i][1], dd[i][1], h1[i][1], m1[i][1], ee[i][1], ff[i][1]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[1] = (j + s[i][1] + 1) >> 1;
        j = Tap6Filter(cc[i][2], dd[i][2], h1[i][2], m1[i][2], ee[i][2], ff[i][2]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[2] = (j + s[i][2] + 1) >> 1;
        j = Tap6Filter(cc[i][3], dd[i][3], h1[i][3], m1[i][3], ee[i][3], ff[i][3]);
        j = HL_MATH_CLIP2(255, ((j + 512) >> 10));
        predPartLXL16x1[3] = (j + s[i][3] + 1) >> 1;

        predPartLXL16x1 += 4;
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma33_diag4_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL,
    HL_ALIGNED(16) int32_t* predPartLXL,
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // (1 << (BitDepth)) - 1
)
{
    int32_t s,m,i;
    for (i = 0; i <4; ++i) {
        s = hl_codec_interpol_264_tap6filter_horiz(&pc_indices_horiz[i], cSL);
        s = HL_MATH_CLIP2(MaxPixelValueY[i], ((s + 16) >> 5));

        m = hl_codec_interpol_264_tap6filter_vert(&pc_indices_vert[i], i_indices_stride, cSL);
        m = HL_MATH_CLIP2(MaxPixelValueY[i], ((m + 16) >> 5));

        predPartLXL[i] = (m + s + 1) >> 1;// (8-263)
    }
}

HL_ALWAYS_INLINE static void hl_codec_264_interpol_luma33_diag4x4_u8_cpp(
    const uint32_t* pc_indices_horiz,
    const uint32_t* pc_indices_vert,
    int32_t i_indices_stride,
    const hl_pixel_t* cSL_u8,
    HL_ALIGNED(16) uint8_t* predPartLXL16x1
)
{
    int32_t s[4][4],m[4][4],j;
    // s = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_horiz4x4_cpp(pc_indices_horiz, i_indices_stride, cSL_u8, s);
    // m = Clip((Tap6Filter + 16) >>5)
    hl_codec_interpol_264_tap6filter_full_vert4x4_cpp(pc_indices_vert, i_indices_stride, cSL_u8, m);

    for (j = 0; j <4; ++j) {
        predPartLXL16x1[0] = (m[j][0] + s[j][0] + 1) >> 1;
        predPartLXL16x1[1] = (m[j][1] + s[j][1] + 1) >> 1;
        predPartLXL16x1[2] = (m[j][2] + s[j][2] + 1) >> 1;
        predPartLXL16x1[3] = (m[j][3] + s[j][3] + 1) >> 1;
        predPartLXL16x1+=4;
    }
}


extern void (*hl_codec_264_interpol_luma00_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL);
extern void (*hl_codec_264_interpol_luma01_vert4)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma02_vert4)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL,HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma03_vert4)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma10_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma11_diag4)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma12_vert4)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma13_diag4)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma20_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma21_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma22_vert4)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma23_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma30_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma31_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma32_vert4)(const uint32_t* pc_indices_vert[7], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
extern void (*hl_codec_264_interpol_luma33_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);

extern void (*hl_codec_264_interpol_luma00_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma01_vert4x4_u8)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma02_vert4x4_u8)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma03_vert4x4_u8)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma10_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
extern void (*hl_codec_264_interpol_luma11_diag4x4_u8)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma12_vert4x4_u8)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma13_diag4x4_u8)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma20_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma21_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma22_vert4x4_u8)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma23_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma30_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma31_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma32_vert4x4_u8)(const uint32_t* pc_indices_vert[7], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);
extern void (*hl_codec_264_interpol_luma33_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1);

extern HL_ERROR_T (*hl_codec_264_interpol_chroma)(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const struct hl_codec_264_mv_xs* mvCLX,
    const struct hl_codec_264_mv_xs* mvLX, // used when "ChromaArrayType"=3
    const struct hl_codec_264_pict_s* refPicLXCb,
    const struct hl_codec_264_pict_s* refPicLXCr,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_INTERPOL_H_ */
