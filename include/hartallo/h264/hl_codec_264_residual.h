#ifndef _HARTALLO_CODEC_264_RESIDUAL_H_
#define _HARTALLO_CODEC_264_RESIDUAL_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;

typedef HL_ERROR_T (*hl_codec_264_residual_read_block_f)(
    HL_IN struct hl_codec_264_residual_inv_xs* p_inv,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_OUT int32_t *coeffLevels, // table with at least "maxNumCoeff" values
    HL_IN int32_t startIdx,
    HL_IN int32_t endIdx,
    HL_IN uint32_t maxNumCoeff);
typedef HL_ERROR_T (*hl_codec_264_residual_write_block_f)(
    struct hl_codec_264_residual_inv_xs* p_inv,
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    struct hl_codec_264_bits_s* p_bits,
    int32_t coeffLevel[16],
    int32_t startIdx,
    int32_t endIdx,
    int32_t maxNumCoef);

HL_ERROR_T hl_codec_264_residual_read(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t startIdx, int32_t endIdx);
HL_ERROR_T hl_codec_264_residual_write_block_cavlc(
    struct hl_codec_264_residual_inv_xs* p_inv,
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    struct hl_codec_264_bits_s* p_bits,
    int32_t coeffLevel[16],
    int32_t startIdx,
    int32_t endIdx,
    int32_t maxNumCoef);
HL_ERROR_T hl_codec_264_residual_write(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t startIdx,
    int32_t endIdx);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_RESIDUAL_H_ */
