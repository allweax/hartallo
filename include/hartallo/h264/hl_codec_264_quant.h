#ifndef _HARTALLO_CODEC_264_QUANT_H_
#define _HARTALLO_CODEC_264_QUANT_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_mb_s;

// FIXME: QP is in range -> use two tables: QP_POSITIVE[] and QP_NEGATIVE[]
static HL_ALWAYS_INLINE double hl_codec_264_quant_qp2qstep(int32_t QP)
{
    register int32_t i;
    double Qstep;
    int32_t QP_div6;
    static const double QP2QSTEP[6] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

    Qstep = QP2QSTEP[QP % 6];
    QP_div6 = QP/6;
    for (i=0; i<QP_div6; i++) {
        Qstep *= 2;
    }
    return Qstep;
}

void hl_codec_264_quant_scale_residual4x4_cpp(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s* pc_mb,
    int32_t bitDepth,
    int32_t qP,
    const int32_t c[4][4],
    hl_bool_t luma,
    hl_bool_t Intra16x16,
    int32_t CbCrIdx,
    /* out */int32_t d[4][4]);
extern void (*hl_codec_264_quant_scale_residual4x4)(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, int32_t bitDepth, int32_t qP, const int32_t c[4][4], hl_bool_t luma, hl_bool_t Intra16x16, int32_t CbCrIdx, /* out */int32_t d[4][4]);

extern void (*hl_codec_264_quant_frw4x4_scale_ac)(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);
extern void (*hl_codec_264_quant_frw4x4_scale_dc_luma)(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);
extern void (*hl_codec_264_quant_frw2x2_scale_dc_chroma)(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[2][2], HL_ALIGNED(16) int32_t out2x2[2][2]);


HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_QUANT_H_ */
