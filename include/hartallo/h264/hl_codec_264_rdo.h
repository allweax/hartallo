#ifndef _HARTALLO_CODEC_264_RDO_H_
#define _HARTALLO_CODEC_264_RDO_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

HL_ERROR_T hl_codec_264_rdo_mb_guess_best_intra_pred_avc(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec, int32_t* pi_mad);
HL_ERROR_T hl_codec_264_rdo_mb_guess_best_intra_pred_svc(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rdo_mb_guess_best_inter_pred_avc(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec, int32_t *pi_mad);
HL_ERROR_T hl_codec_264_rdo_mb_guess_best_inter_pred_svc(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec);

HL_ERROR_T hl_codec_264_rdo_mb_compute_inter_luma4x4(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_pixel_t* p_SL_in, int32_t i_SL_in_stride,
    HL_IN const hl_pixel_t* p_SL_pred, int32_t i_SL_pred_stride,
    HL_OUT int32_t SL_res[4][4], // Residual (output)
    HL_OUT int32_t LumaLevel[16], // Luma levels (output)
    HL_OUT hl_bool_t *pb_zeros // Whether all luma blocks are zeros (output)
);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_RDO_H_ */
