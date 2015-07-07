#ifndef _HARTALLO_CODEC_264_TRANSF_H_
#define _HARTALLO_CODEC_264_TRANSF_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_mb_s;

HL_ERROR_T hl_codec_264_transf_decode_luma4x4(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    const int32_t* luma4x4BlkIdxList,
    hl_size_t luma4x4BlkIdxListCount,
    HL_IN_ALIGNED(16) const int32_t* predL,
    int32_t predLStride,
    hl_bool_t predLIs4x4SamplesOnly);

HL_ERROR_T hl_codec_264_transf_decode_intra16x16_luma(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    HL_IN_ALIGNED(16) const int32_t predL[16][16]);

HL_ERROR_T hl_codec_264_transf_decode_chroma(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    HL_IN_ALIGNED(16) const int32_t predC[16][16],
    int32_t iCbCr);

void hl_codec_264_transf_scale_residual4x4(
    const struct hl_codec_264_s* p_codec,
    const struct hl_codec_264_mb_s* p_mb,
    HL_IN const int32_t c[4][4],
    HL_OUT int32_t r[4][4],
    hl_bool_t luma,
    hl_bool_t Intra16x16,
    int32_t iCbCr
);
void hl_codec_264_transf_bypass_intra_residual(
    int32_t nW,
    int32_t nH,
    int32_t horPredFlag,
    HL_IN_OUT int32_t r[4][4]/*4x4 or more*/
);

extern void (*hl_codec_264_transf_inverse_residual4x4)(int32_t bitDepth, int32_t d[4][4], int32_t r[4][4]);

extern void (*hl_codec_264_transf_scale_luma_dc_coeff_intra16x16)(
    const struct hl_codec_264_s* p_codec,
    const struct hl_codec_264_mb_s* p_mb,
    int32_t qP,
    int32_t BitDepth,
    HL_IN_ALIGNED(16) const int32_t c[4][4],
    HL_OUT_ALIGNED(16) int32_t dcY[4][4]);
void hl_codec_264_transf_scale_chroma_dc_coeff(
    const struct hl_codec_264_s* p_codec,
    const struct hl_codec_264_mb_s* p_mb,
    HL_IN_ALIGNED(16) const int32_t c[4][4],
    int32_t bitDepth,
    int32_t qP,
    int32_t iCbCr,
    HL_OUT_ALIGNED(16) int32_t dcC[4][2]);
extern void (*hl_codec_264_transf_frw_residual4x4)(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);
extern void (*hl_codec_264_transf_frw_hadamard4x4_dc_luma)(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);
void hl_codec_264_transf_frw_hadamard2x2_dc_chroma(HL_ALIGNED(16) const int32_t in2x2[2][2], HL_ALIGNED(16) int32_t out2x2[2][2]);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_TRANSF_H_ */
