#ifndef _HARTALLO_CODEC_264_ME_H_
#define _HARTALLO_CODEC_264_ME_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

HL_ERROR_T hl_codec_264_me_mb_find_best_cost(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref,
    HL_IN const hl_pixel_t* pc_SL_enc,
    HL_IN enum HL_CODEC_264_SUBMB_TYPE_E currSubMbType,
    HL_IN enum HL_CODEC_264_LIST_IDX_Z listSuffix,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN int32_t refIdxLX,
    HL_IN int32_t predMode,
    HL_IN uint32_t u_SL_mb_x0, HL_IN uint32_t u_SL_mb_y0, HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN uint32_t me_range,
    HL_OUT int32_t *pi_Single_ctr, // Best "Single_ctr" - JVT-O079 2.3 Elimination of single coefficients in inter macroblocks
    HL_OUT hl_bool_t *pb_probably_pskip, // Whether the best pos could be PSkip
    HL_OUT struct hl_codec_264_mv_xs* mvpLX, // Predicted motion vector (used to compute MVD)
    HL_OUT int32_t *pi_best_dist, // Best distortion (OUT)
    HL_OUT double *pd_best_cost, // Best cost (OUT)
    HL_OUT int32_t best_pos[2] // Best pos (x,y)
);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_ME_H_ */
