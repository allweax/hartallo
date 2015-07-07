#ifndef _HARTALLO_CODEC_264_PRED_INTER_H_
#define _HARTALLO_CODEC_264_PRED_INTER_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_mb_s;
struct hl_codec_264_pict_s;

HL_ERROR_T hl_codec_264_pred_inter_decode(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    hl_bool_t b_compute_mvs_and_refs);
#define hl_codec_264_pred_inter_decode_with_mvs_and_refs_computation(p_codec, p_mb) hl_codec_264_pred_inter_decode((p_codec), (p_mb), HL_TRUE)
#define hl_codec_264_pred_inter_decode_without_mvs_and_refs_computation(p_codec, p_mb) hl_codec_264_pred_inter_decode((p_codec), (p_mb), HL_FALSE)

HL_ERROR_T hl_codec_264_pred_inter_predict(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const struct hl_codec_264_mv_xs* mvLX,
    const struct hl_codec_264_mv_xs* mvCLX,
    const struct hl_codec_264_pict_s* refPicLXL,
    const struct hl_codec_264_pict_s* refPicLXCb,
    const struct hl_codec_264_pict_s* refPicLXCr,
    HL_OUT int32_t predPartLXL[16][16],
    HL_OUT int32_t predPartLXCb[16][16],
    HL_OUT int32_t predPartLXCr[16][16]);

HL_ERROR_T hl_codec_264_pred_inter_select_refpic(
    const struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    struct hl_codec_264_dpb_fs_s** RefPicListX,
    int32_t refIdxLX,
    const struct hl_codec_264_pict_s** refPicLXL,
    const struct hl_codec_264_pict_s** refPicLXCb,
    const struct hl_codec_264_pict_s** refPicLXCr);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_PRED_INTER_H_ */
