#ifndef _HARTALLO_CODEC_264_ENCODE_H_
#define _HARTALLO_CODEC_264_ENCODE_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

typedef struct hl_codec_264_encode_slice_data_s {
    HL_DECLARE_OBJECT;

    int32_t i_qp;

    int32_t i_mb_start;
    int32_t i_mb_end;

    uint32_t u_mb_skip_run; // actual "mb_skip_run"

    uint8_t* p_rbsp_bytes;
    hl_size_t u_rbsp_bytes_size;
    hl_size_t u_rbsp_bytes_num;
    struct hl_codec_264_bits_s* pobj_bits;

    struct hl_codec_264_slice_s* pc_slice;

    struct hl_memory_blocks_s* pc_mem_blocks;

    struct {
        double d_lambda_mode; // Lagrange multiplier used to compute the cost in the RDO module (cost = D + (lambda * R)).
        double d_lambda_motion;
        struct hl_codec_264_bits_s* pobj_bits;
        uint8_t bits_buff[HL_CODEC_264_RDO_BUFFER_MAX_SIZE];
        int32_t Single_ctr; // JVT-O079 - 2.3 Elimination of single coefficients in inter macroblocks
        double Bsize[8/*Mode=1...7*/]; // JVT-O079 2.1.3.1.2.2 Early termination
        double Costs[8/*Mode=1...7*/]; // JVT-O079: Used to compute "Uplayer Prediction cost"
        int32_t BestMV00/*mbPartIdx=0, subMbPartIdx=0*/[8/*Mode=1...7*/][2/*x=0,y=0*/]; // JVT-O079: Used to compute "Uplayer Prediction cost"

        // FIXME: not used yet
        struct {
			hl_rect_xt wnd_search;

            //== Inputs: Next values must be "set" before calling the Motion Estimation (Module) ==//
            const hl_pixel_t* pc_SL_ref; // Reference frame (previsously decoded and reconstructed frame) - at Picture(0,0)
            const hl_pixel_t* pc_SL_enc;  // Current frame to encode - at Picture(0,0)
            uint32_t me_range;
            int32_t mbPartIdx;
            int32_t subMbPartIdx;
            int32_t refIdxLX;
            enum HL_CODEC_264_SUBMB_TYPE_E currSubMbType;
            enum HL_CODEC_264_LIST_IDX_E listSuffix;

            int32_t xL_Idx[4/*mbPartIdx*/][4/*subMbPartIdx*/];
            int32_t yL_Idx[4/*mbPartIdx*/][4/*subMbPartIdx*/];
            int32_t xP[4/*mbPartIdx*/][4/*subMbPartIdx*/];
            int32_t yP[4/*mbPartIdx*/][4/*subMbPartIdx*/];
            int32_t xS[4/*mbPartIdx*/][4/*subMbPartIdx*/];
            int32_t yS[4/*mbPartIdx*/][4/*subMbPartIdx*/];

            //== Outputs: Next values must be "set" by the Motion Estimation (ME) module after each call ==//
            int32_t i_Single_ctr[4/*mbPartIdx*/][4/*subMbPartIdx*/]; // Best "Single_ctr" - JVT-O079 2.3 Elimination of single coefficients in inter macroblocks
            hl_bool_t b_probably_pskip; // Whether the best pos could be PSkip
            struct hl_codec_264_mv_xs mvpLX[4/*mbPartIdx*/][4/*subMbPartIdx*/]; // Predicted motion vector (used to compute MVD)
            int32_t i_best_dist[4/*mbPartIdx*/][4/*subMbPartIdx*/]; // Best distortion
			int32_t i_best_CodedBlockPatternLuma4x4[4/*mbPartIdx*/][4/*subMbPartIdx*/];
            double d_best_cost[4/*mbPartIdx*/][4/*subMbPartIdx*/]; // Best cost
            struct hl_codec_264_mv_xs mvBest[4/*mbPartIdx*/][4/*subMbPartIdx*/]; // Best pos (x,y)
        } me;
    } rdo;
}
hl_codec_264_encode_slice_data_t;

HL_ERROR_T hl_codec_264_encode_frame(
    struct hl_codec_264_s* p_self,
    const struct hl_frame_video_s* pc_frame,
    hl_bool_t b_encoding_changed, hl_bool_t b_sps_changed, hl_bool_t b_pps_changed);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_ENCODE_H_ */
