#include "hartallo/h264/hl_codec_264_me_ds.h"
#include "hartallo/h264/hl_codec_264_me.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_rdo.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_cavlc.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_residual.h"
#include "hartallo/h264/hl_codec_264_pred_intra.h"
#include "hartallo/h264/hl_codec_264_pred_inter.h"
#include "hartallo/h264/hl_codec_264_transf.h"
#include "hartallo/h264/hl_codec_264_quant.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_encode.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_frame.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

#include <cfloat> /* DBL_MAX */

#define DSP_FLAGS	0xFFFFFF

#define DSP_POS_INT_A 0
#define DSP_POS_INT_B 1
#define DSP_POS_INT_C 2
#define DSP_POS_INT_D 3
#define DSP_POS_INT_E 4
#define DSP_POS_INT_F 5
#define DSP_POS_INT_G 6
#define DSP_POS_INT_H 7
#define DSP_POS_INT_I 8

#define DSP_FLAG_INT_A 1 // (1 << 0)
#define DSP_FLAG_INT_B 2 // (1 << 1)
#define DSP_FLAG_INT_C 4 // (1 << 2)
#define DSP_FLAG_INT_D 8 // (1 << 3)
#define DSP_FLAG_INT_E 16 // (1 << 4)
#define DSP_FLAG_INT_F 32 // (1 << 5)
#define DSP_FLAG_INT_G 64 // (1 << 6)
#define DSP_FLAG_INT_H 128 // (1 << 7)
#define DSP_FLAG_INT_I 256 // (1 << 8)

#define CHECK_ERR_RETURN(_err_) if ((_err_)) return (_err_);
#define CHECK_ERR_BAIL(_err_) if ((_err_)) goto bail;

#define SET_NEW_COST(partIdx, subPartIdx, d_cost_new, i_Single_ctr_new, i_dist_new, i_CodedBlockPatternLuma4x4_new, pc_mv) \
        pc_esd->rdo.me.i_Single_ctr[(partIdx)][(subPartIdx)] = (i_Single_ctr_new); \
        pc_esd->rdo.me.d_best_cost[(partIdx)][(subPartIdx)] = (d_cost_new); \
        pc_esd->rdo.me.i_best_dist[(partIdx)][(subPartIdx)] = (i_dist_new); \
        pc_esd->rdo.me.mvBest[(partIdx)][(subPartIdx)].x = (pc_mv)->x; \
        pc_esd->rdo.me.mvBest[(partIdx)][(subPartIdx)].y = (pc_mv)->y; \
		pc_esd->rdo.me.i_best_CodedBlockPatternLuma4x4[(partIdx)][(subPartIdx)] = (i_CodedBlockPatternLuma4x4_new);

#define SET_NEW_COST_IF_BEST(partIdx, subPartIdx, d_cost_new, i_Single_ctr_new, i_dist_new, i_CodedBlockPatternLuma4x4_new, pc_mv) \
	if ((d_cost_new) < pc_esd->rdo.me.d_best_cost[(partIdx)][(subPartIdx)]) { \
        SET_NEW_COST((partIdx), (subPartIdx), (d_cost_new), (i_Single_ctr_new), (i_dist_new), (i_CodedBlockPatternLuma4x4_new), (pc_mv)); \
    }

#define SET_NEW_SEARCH_WINDOW(CX, CY) \
	pc_esd->rdo.me.wnd_search.left = (CX) - pc_esd->rdo.me.me_range; \
	pc_esd->rdo.me.wnd_search.right = (CX) + pc_esd->rdo.me.me_range; \
	pc_esd->rdo.me.wnd_search.top = (CY) - pc_esd->rdo.me.me_range; \
	pc_esd->rdo.me.wnd_search.bottom = (CY) + pc_esd->rdo.me.me_range;

extern HL_ERROR_T hl_codec_264_interpol_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvLX,
    const hl_pixel_t* cSL,
    HL_OUT_ALIGNED(16) void* predPartLXL16x16, int32_t predPartLXLSampleSize);
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_me_ds_mb_compute_cost_motion(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_codec_264_me_part_xt *pc_part,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN const hl_codec_264_mv_xt* pc_mvLX,
    HL_OUT int32_t *pi_dist,
	HL_OUT int32_t *pi_rbc_mv
);
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_me_ds_mb_compute_cost_mode(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_codec_264_me_part_xt *pc_part,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN const hl_codec_264_mv_xt* pc_mvLX,
    HL_OUT int32_t *pi_Single_ctr,
    HL_OUT int32_t *pi_rbc, // residual bits count
    HL_OUT int32_t *pi_dist, // distorsion
	HL_OUT int32_t *pi_CodedBlockPatternLuma4x4
);

HL_ERROR_T hl_codec_264_me_ds_mb_find_best_cost(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_codec_264_me_part_xt *pc_part
)
{
#define MAX_INDEXES	32
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t mbPartIdx, subMbPartIdx, Idx;
    hl_codec_264_layer_t* pc_layer;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_mv_xt mvLX, mvLX_int_best, *pc_mvpLX;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const int32_t (*DiamondSearchIndexes)[MAX_INDEXES][2];
    const int32_t (*DiamondSearchIndexesCount);
    int32_t DiamondSearchCenter[2/*x,y*/], DiamondSearchBestIdx;
    int32_t i_Single_ctr, i_rbc, i_dist, i_rbc_mv, i_CodedBlockPatternLuma4x4;
    double d_cost, d_cost_int_best;
    hl_codec_264_mv_xt (*MvL0)[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    int32_t i_shift, i_dsp_flags;

    static const int32_t DiamondSearchIndexesInt[MAX_INDEXES][2] = {
#if 1 /* 9 values */
        { 0, 2 },
        { -1, 1 }, { 1, 1 },
        { -2, 0 }, { 0, 0 }, { 2, 0},
        { -1, -1 }, { 1, -1 },
        { 0, -2 },
#elif 0 /* 13 values */
        { 0, 2 },
        { -1, 1 }, { 1, 1 },
        { -2, 0 }, { 0, 0 }, { 2, 0},
        { -1, -1 }, { 1, -1 },
        { 0, -2 },

        { 0, 4 },
        { -3, 1 }, { 3, 1 },
        { -3, -2}, { 3, -2 },
        // { 3, 2 }, { 2, 3 }, { 3, 3 }, { 2, 2 }, { -3, 2 }, { 2, -3 }, { 3, -3 }, { -2, 2 }
#else /* 16 values */
        {4, 2}, {-4, 2}, {4, -2}, {-4, -2}, // (+-4,+-2)
        {4, 1}, {-4, 1}, {4, -1}, {-4, -1}, // (+-4,+-1)
        {4, 0}, {-4, 0}, // (+-4, 0)
        {2, 3}, {-2, 3}, {2, -3}, {-2, -3}, // (+-2,+-3)
        {0, 4}, {0, -4}, // (0, +-4)
#endif
    };
    static const int32_t DiamondSearchIndexesIntCount = 9;

    static const int32_t DiamondSearchIndexesHalf[MAX_INDEXES][2] = {
         { 0, 1 }, { -1, 0 }, { 0, -1 }, { 1, 0 }, { 0, 0 },
        // ... completed with 4 fake points
    };
    static const int32_t DiamondSearchIndexesHalfCount = 5;

    static const int32_t DiamondSearchIndexesQuater[MAX_INDEXES][2] = {
		{ -1, 1 }, { 0, 1 }, { 1, 1 },
		{ -1, 0 }, { 0, 0 }, { 1, 0 },
		{ -1, -1 }, { 0, -1 }, { 1, -1 },
        // ... completed with 4 fake points
    };
    static const int32_t DiamondSearchIndexesQuaterCount = 9;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_layer->pc_slice_hdr;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    MvL0 = pc_layer->SVCExtFlag ? &p_mb->mvL0 : &p_mb->MvL0; // TODO: "L1"

    p_mb->NumMbPart = pc_part->NumMbPart;
    p_mb->MbPartWidth = pc_part->MbPartWidth;
    p_mb->MbPartHeight = pc_part->MbPartHeight;

    // Build all parts. Required because part "N" could be used for MV prediction while we're at part N-1.
    for (mbPartIdx = 0; mbPartIdx < pc_part->NumMbPart; ++mbPartIdx) {
        p_mb->predFlagL0[mbPartIdx] = 1; // TODO: "L1"
        p_mb->MbPartPredMode[mbPartIdx] = HL_CODEC_264_MB_MODE_PRED_L0; // TODO: "B, Direct, BiPred"
        p_mb->SubMbPredType[mbPartIdx] = pc_part->SubMbPredType[mbPartIdx];
        p_mb->SubMbPredMode[mbPartIdx] = pc_part->SubMbPredMode[mbPartIdx];
        p_mb->NumSubMbPart[mbPartIdx] = pc_part->NumSubMbPart[mbPartIdx];

        p_mb->SubMbPartWidth[mbPartIdx] = pc_part->SubMbPartWidth[mbPartIdx];
        p_mb->SubMbPartHeight[mbPartIdx] = pc_part->SubMbPartHeight[mbPartIdx];

        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
            // compute "partWidth", "partHeight", "partWidthC" and "partHeightC" used by interpolation functions
            p_mb->partWidth[mbPartIdx][subMbPartIdx] = p_mb->SubMbPartWidth[mbPartIdx];
            p_mb->partHeight[mbPartIdx][subMbPartIdx] = p_mb->SubMbPartHeight[mbPartIdx];
            p_mb->partWidthC[mbPartIdx][subMbPartIdx] = p_mb->partWidth[mbPartIdx][subMbPartIdx] >> pc_sps->SubWidthC_TrailingZeros;
            p_mb->partHeightC[mbPartIdx][subMbPartIdx] = p_mb->partHeight[mbPartIdx][subMbPartIdx] >> pc_sps->SubHeightC_TrailingZeros;

            // compute "xL_Idx" and "yL_Idx", used by interpolation functions
            if (mbPartIdx == 0 && subMbPartIdx == 0) {
                pc_esd->rdo.me.xL_Idx[0][0] = p_mb->xL;
                pc_esd->rdo.me.yL_Idx[0][0] = p_mb->yL;
                pc_esd->rdo.me.xP[0][0] = pc_esd->rdo.me.yP[0][0] = pc_esd->rdo.me.xS[0][0] = pc_esd->rdo.me.yS[0][0] = 0;
            }
            else {
                // 6.4.2.1 - upper-left sample of the macroblock partition
                pc_esd->rdo.me.xP[mbPartIdx][subMbPartIdx] = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);// (6-11)
                pc_esd->rdo.me.yP[mbPartIdx][subMbPartIdx] = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);// (6-12)
                // 6.4.2.2 - upper-left sample of the sub-macroblock partition
                hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &pc_esd->rdo.me.xS[mbPartIdx][subMbPartIdx], &pc_esd->rdo.me.yS[mbPartIdx][subMbPartIdx]);

                pc_esd->rdo.me.xL_Idx[mbPartIdx][subMbPartIdx] = p_mb->xL + (pc_esd->rdo.me.xP[mbPartIdx][subMbPartIdx] + pc_esd->rdo.me.xS[mbPartIdx][subMbPartIdx]);
                pc_esd->rdo.me.yL_Idx[mbPartIdx][subMbPartIdx] = p_mb->yL + (pc_esd->rdo.me.yP[mbPartIdx][subMbPartIdx] + pc_esd->rdo.me.yS[mbPartIdx][subMbPartIdx]);
            }

            pc_esd->rdo.me.i_Single_ctr[mbPartIdx][subMbPartIdx] = 9;
            pc_esd->rdo.me.i_best_dist[mbPartIdx][subMbPartIdx] = INT_MAX;
            pc_esd->rdo.me.d_best_cost[mbPartIdx][subMbPartIdx] = DBL_MAX;
        } // end-of-for(subMbPartIdx)
    } //end-of-for(mbPartIdx)
    pc_esd->rdo.me.b_probably_pskip = HL_FALSE;

    // FIXME
	if (p_mb->u_addr == 19 && pc_part->Mode == HL_CODEC_264_MODE_16X8) {
        int a = 0;
    }

    // Compute PSkip cost
    if (pc_part->Mode == HL_CODEC_264_MODE_16X16 && pc_esd->rdo.me.refIdxLX == 0) {
        int32_t refIdxL0;
        // 8.4.1.1 Derivation process for luma motion vectors for skipped macroblocks in P and SP slices
        hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(
            p_codec,
            p_mb,
            &mvLX,
            &refIdxL0);
        if (refIdxL0 == 0) {
            // 8.4.1.3 Derivation process for luma motion vector prediction
            pc_mvpLX = &pc_esd->rdo.me.mvpLX[0][1];
            err = hl_codec_264_utils_derivation_process_for_luma_movect_prediction(
                      p_codec,
                      p_mb,
                      0,
                      0,
                      pc_esd->rdo.me.refIdxLX,
                      pc_esd->rdo.me.currSubMbType,
                      pc_mvpLX,
                      pc_esd->rdo.me.listSuffix);
            CHECK_ERR_BAIL(err);
            if (pc_mvpLX->x == mvLX.x && pc_mvpLX->y == mvLX.y) {
                err = hl_codec_264_me_ds_mb_compute_cost_mode(p_mb, p_codec, pc_part, 0, 0, pc_mvpLX, &i_Single_ctr, &i_rbc, &i_dist, &i_CodedBlockPatternLuma4x4);
                CHECK_ERR_BAIL(err);
                if (i_rbc == 0 || i_Single_ctr < 6) { // No residual?
                    d_cost = 0;//i_dist + (0/*i_rbc_mv */* p_codec->encoder.rdo.d_lambda_mode);
                    pc_esd->rdo.me.b_probably_pskip = HL_TRUE;
					SET_NEW_COST(0, 0, d_cost, i_Single_ctr, i_dist, i_CodedBlockPatternLuma4x4, pc_mvpLX);
                    //goto bail;//FIXME
                }
            }
        }
    }

    for (mbPartIdx = 0; mbPartIdx < p_mb->NumMbPart; ++mbPartIdx) {
        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
			d_cost_int_best = DBL_MAX;
            DiamondSearchIndexes = &DiamondSearchIndexesInt;
            DiamondSearchIndexesCount = &DiamondSearchIndexesIntCount;
            i_shift = 2; // Integer Pel
			i_dsp_flags = DSP_FLAGS;
            // 8.4.1.3 Derivation process for luma motion vector prediction
            pc_mvpLX = &pc_esd->rdo.me.mvpLX[mbPartIdx][subMbPartIdx];
            err = hl_codec_264_utils_derivation_process_for_luma_movect_prediction(
                      p_codec,
                      p_mb,
                      mbPartIdx,
                      subMbPartIdx,
                      pc_esd->rdo.me.refIdxLX,
                      pc_esd->rdo.me.currSubMbType,
                      pc_mvpLX,
                      pc_esd->rdo.me.listSuffix);
            CHECK_ERR_BAIL(err);
            
			// Cost at MVP
			err = hl_codec_264_me_ds_mb_compute_cost_mode(p_mb, p_codec, pc_part, mbPartIdx, subMbPartIdx, pc_mvpLX, &i_Single_ctr, &i_rbc, &i_dist, &i_CodedBlockPatternLuma4x4);
			CHECK_ERR_BAIL(err);
			i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se(0/*MVD.x*/) + hl_codec_264_bits_count_bits_se(0/*MVD.y*/)); // TODO: ae(v) for CABAC
			d_cost = i_dist + ((i_rbc + i_rbc_mv) * p_codec->encoder.rdo.d_lambda_mode);
			SET_NEW_COST_IF_BEST(mbPartIdx, subMbPartIdx, d_cost, i_Single_ctr, i_dist, i_CodedBlockPatternLuma4x4, pc_mvpLX);
			

            // Cost at (0, 0)
            if (pc_mvpLX->x != 0 || pc_mvpLX->y != 0) {
                mvLX.x = mvLX.y = 0;
                err = hl_codec_264_me_ds_mb_compute_cost_mode(p_mb, p_codec, pc_part, mbPartIdx, subMbPartIdx, &mvLX, &i_Single_ctr, &i_rbc, &i_dist, &i_CodedBlockPatternLuma4x4);
                CHECK_ERR_BAIL(err);
                i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - pc_mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - pc_mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
                d_cost = i_dist + ((i_rbc + i_rbc_mv) * p_codec->encoder.rdo.d_lambda_mode);
				SET_NEW_COST_IF_BEST(mbPartIdx, subMbPartIdx, d_cost, i_Single_ctr, i_dist, i_CodedBlockPatternLuma4x4, &mvLX);
            }

#if 0
            DiamondSearchCenter[0] = 0;
            DiamondSearchCenter[1] = 0;
#else
            DiamondSearchCenter[0] = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].x >> 2;
            DiamondSearchCenter[1] = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].y >> 2;
#endif
			// Update search window
			SET_NEW_SEARCH_WINDOW(DiamondSearchCenter[0], DiamondSearchCenter[1]);

            while (1) {
                DiamondSearchBestIdx = -1;
                for (Idx = 0; Idx < *DiamondSearchIndexesCount; ++ Idx) {
					if (!(i_dsp_flags & (1 << Idx))) {
						continue;
					}
                    mvLX.x = (DiamondSearchCenter[0] + (*DiamondSearchIndexes)[Idx][0]);
                    mvLX.y = (DiamondSearchCenter[1] + (*DiamondSearchIndexes)[Idx][1]);
                    //mvLX.x = HL_MATH_CLIP3(-17, (int32_t)(pc_slice_header->PicWidthInSamplesL + 17), mvLX.x);
                    //mvLX.y = HL_MATH_CLIP3(-17, (int32_t)(pc_slice_header->PicHeightInSamplesL + 17), mvLX.y);
					if (mvLX.x < pc_esd->rdo.me.wnd_search.left || mvLX.x > pc_esd->rdo.me.wnd_search.right) {
						continue;
					}
					if (mvLX.y < pc_esd->rdo.me.wnd_search.top || mvLX.y > pc_esd->rdo.me.wnd_search.bottom) {
						continue;
					}
                    mvLX.x <<= i_shift;
                    mvLX.y <<= i_shift;

					if (i_shift == 2 && 0/*FIXME*/) { // Integer
						err = hl_codec_264_me_ds_mb_compute_cost_motion(p_mb, p_codec, pc_part, mbPartIdx, subMbPartIdx, &mvLX, &i_dist, &i_rbc_mv);
						CHECK_ERR_BAIL(err);
						d_cost = i_dist + (i_rbc_mv * p_codec->encoder.rdo.d_lambda_motion);
						if (d_cost < d_cost_int_best) {
							d_cost_int_best = d_cost;
							DiamondSearchBestIdx = Idx;
							mvLX_int_best.x = mvLX.x;
							mvLX_int_best.y = mvLX.y;
						}
					}
					else {
						err = hl_codec_264_me_ds_mb_compute_cost_mode(p_mb, p_codec, pc_part, mbPartIdx, subMbPartIdx, &mvLX, &i_Single_ctr, &i_rbc, &i_dist, &i_CodedBlockPatternLuma4x4);
						CHECK_ERR_BAIL(err);
						i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - pc_mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - pc_mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
						d_cost = i_dist + ((i_rbc + i_rbc_mv) * p_codec->encoder.rdo.d_lambda_mode);
						if (d_cost < pc_esd->rdo.me.d_best_cost[mbPartIdx][subMbPartIdx]) {
							DiamondSearchBestIdx = Idx;
							SET_NEW_COST(mbPartIdx, subMbPartIdx, d_cost, i_Single_ctr, i_dist, i_CodedBlockPatternLuma4x4, &mvLX);
						}
					}
				} // end-of-for(Idx....)

				i_dsp_flags = DSP_FLAGS;
                if (i_shift == 2 && (/*DiamondSearchBestIdx == 4 ||*/ DiamondSearchBestIdx == -1)) { // Center or not found
                    // Switch to Half-Pel (from Integer-Pel)
                    i_shift = 1;
					DiamondSearchIndexes = &DiamondSearchIndexesHalf;
                    DiamondSearchIndexesCount = &DiamondSearchIndexesHalfCount;
                    // Set center to Integer-Pel
					DiamondSearchCenter[0] = (/*mvLX_int_best*/pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].x >> 2);
                    DiamondSearchCenter[1] = (/*mvLX_int_best*/pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].y >> 2);
					// Update search window
					SET_NEW_SEARCH_WINDOW(DiamondSearchCenter[0], DiamondSearchCenter[1]);
                }
                else if ((i_shift == 1 || i_shift == 0) && (/*DiamondSearchBestIdx == 2 ||*/ DiamondSearchBestIdx == -1)) { // Center or not found
                    if (i_shift == 1) { // Half-Pel?
                        // Switch to Quater-Pel (from Half-Pel)
                        i_shift = 0;
						DiamondSearchIndexes = &DiamondSearchIndexesQuater;
						DiamondSearchIndexesCount = &DiamondSearchIndexesQuaterCount;
                        // Set center to Current-Pel ([Integer/Half]-Pel)
                        DiamondSearchCenter[0] = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].x;
                        DiamondSearchCenter[1] = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].y;
						// Update search window
						SET_NEW_SEARCH_WINDOW(DiamondSearchCenter[0], DiamondSearchCenter[1]);
                    }
                    else {
                        goto endof_while;
                    }
                }
                else {
                    DiamondSearchCenter[0] = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].x >> i_shift;
                    DiamondSearchCenter[1] = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].y >> i_shift;
					// SKIP POINTS
					if (i_shift == 2) {
						switch (DiamondSearchBestIdx) {
							case DSP_POS_INT_A: 
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_G | DSP_FLAG_INT_I | DSP_FLAG_INT_H);
								break;
							case DSP_POS_INT_B:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_F | DSP_FLAG_INT_I);
								break;
							case DSP_POS_INT_C:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_B | DSP_FLAG_INT_D | DSP_FLAG_INT_G | DSP_FLAG_INT_I | DSP_FLAG_INT_H); //!\ MD5 <>
								break;
							case DSP_POS_INT_D:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_C | DSP_FLAG_INT_F | DSP_FLAG_INT_H);
								break;
							case DSP_POS_INT_F:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_B | DSP_FLAG_INT_D | DSP_FLAG_INT_G);
								break;
							case DSP_POS_INT_G:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_A | DSP_FLAG_INT_B | DSP_FLAG_INT_H | DSP_FLAG_INT_F | DSP_FLAG_INT_C);
								break;
							case DSP_POS_INT_H:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_A | DSP_FLAG_INT_B | DSP_FLAG_INT_D | DSP_FLAG_INT_G | DSP_FLAG_INT_C);
								break;
							case DSP_POS_INT_I:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_C | DSP_FLAG_INT_A | DSP_FLAG_INT_B);
								break;
							case DSP_POS_INT_E:
							default:
								break;
						}
					}
					else if (i_shift == 1) {
						switch (DiamondSearchBestIdx) {
							case DSP_POS_INT_A:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_C);
								break;
							case DSP_POS_INT_B:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_D);
								break;
							case DSP_POS_INT_C:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_A);
								break;
							case DSP_POS_INT_D:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_B);
								break;
							case DSP_POS_INT_E:
							default:
								break;
						}
					}
					else if (i_shift == 0) {
						switch (DiamondSearchBestIdx) {
							case DSP_POS_INT_A: 
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_F | DSP_FLAG_INT_I | DSP_FLAG_INT_H);
								break;
							case DSP_POS_INT_B:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_D | DSP_FLAG_INT_G | DSP_FLAG_INT_H | DSP_FLAG_INT_I | DSP_FLAG_INT_F);
								break;
							case DSP_POS_INT_C:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_D | DSP_FLAG_INT_A | DSP_FLAG_INT_B | DSP_FLAG_INT_C | DSP_FLAG_INT_F);
								break;
							case DSP_POS_INT_D:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_B | DSP_FLAG_INT_C | DSP_FLAG_INT_F | DSP_FLAG_INT_I | DSP_FLAG_INT_H);
								break;
							case DSP_POS_INT_F:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_A | DSP_FLAG_INT_B | DSP_FLAG_INT_D | DSP_FLAG_INT_G | DSP_FLAG_INT_H);
								break;
							case DSP_POS_INT_G:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_B | DSP_FLAG_INT_C | DSP_FLAG_INT_F);
								break;
							case DSP_POS_INT_H:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_A | DSP_FLAG_INT_B | DSP_FLAG_INT_C | DSP_FLAG_INT_D | DSP_FLAG_INT_F);
								break;
							case DSP_POS_INT_I:
								i_dsp_flags &= ~(DSP_FLAG_INT_E | DSP_FLAG_INT_A | DSP_FLAG_INT_B | DSP_FLAG_INT_D);
								break;
							case DSP_POS_INT_E:
							default:
								break;
						}
					}
                }				
            } // end-of-while(1)
endof_while:
            // Set "mvL0" for the current partition. Required because MVP for next predition depends on this value.
            (*MvL0)[mbPartIdx][subMbPartIdx].x = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].x;
            (*MvL0)[mbPartIdx][subMbPartIdx].y = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].y;
        } // end-of-for (subMbPartIdx...
    } // end-of-for (mbPartIdx...

bail:
    return err;
}

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_me_ds_mb_compute_cost_motion(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_codec_264_me_part_xt *pc_part,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN const hl_codec_264_mv_xt* pc_mvLX,
    HL_OUT int32_t *pi_dist,
	HL_OUT int32_t *pi_rbc_mv
)
{
	const hl_pixel_t *_pc_SL_enc_mb, *_pc_SL_ref_mb;
	hl_codec_264_encode_slice_data_t* pc_esd;
	uint32_t u_SL_stride, u_SL_4x4_width_index, u_SL_4x4_height_index, u_SL_4x4_width_count, u_SL_4x4_height_count;
	int32_t x, y;

#if _DEBUG
	if (pc_mvLX->x & 3 || pc_mvLX->y & 3) {
		HL_DEBUG_ERROR("Motion cost step0 expect Integer-Pel");
		return HL_ERROR_INVALID_PARAMETER;
	}
#endif

	pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];

	*pi_dist = 0;
	*pi_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((pc_mvLX->x - pc_esd->rdo.me.mvpLX[mbPartIdx][subMbPartIdx].x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((pc_mvLX->y - pc_esd->rdo.me.mvpLX[mbPartIdx][subMbPartIdx].y)/*MVD.y*/)); // TODO: ae(v) for CABAC

    u_SL_stride = pc_esd->pc_slice->p_header->PicWidthInSamplesL;

	u_SL_4x4_width_count = p_mb->partWidth[mbPartIdx][subMbPartIdx] >> 2;
    u_SL_4x4_height_count = p_mb->partHeight[mbPartIdx][subMbPartIdx] >> 2;
    for (u_SL_4x4_height_index = 0; u_SL_4x4_height_index < u_SL_4x4_height_count; ++u_SL_4x4_height_index) {
		x = pc_esd->rdo.me.xL_Idx[mbPartIdx][subMbPartIdx];
		y = (pc_esd->rdo.me.yL_Idx[mbPartIdx][subMbPartIdx] + (u_SL_4x4_height_index << 2));
        _pc_SL_enc_mb = pc_esd->rdo.me.pc_SL_enc + x + (y * u_SL_stride);
		_pc_SL_ref_mb = pc_esd->rdo.me.pc_SL_ref + (x + (pc_mvLX->x << 2)) + ((y + (pc_mvLX->y << 2)) * u_SL_stride);
		for (u_SL_4x4_width_index = 0; u_SL_4x4_width_index < u_SL_4x4_width_count; ++u_SL_4x4_width_index) {
			*(pi_dist) += hl_math_sad4x4_u8(
                             _pc_SL_enc_mb, u_SL_stride,
                             _pc_SL_ref_mb, u_SL_stride);
            _pc_SL_enc_mb += 4;
            _pc_SL_ref_mb += 4;
		} // end-of-for (u_SL_4x4_width_index...
	} // end-of-for (u_SL_4x4_height_index...
	return HL_ERROR_SUCCESS;
}

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_me_ds_mb_compute_cost_mode(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const hl_codec_264_me_part_xt *pc_part,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN const hl_codec_264_mv_xt* pc_mvLX,
    HL_OUT int32_t *pi_Single_ctr,
    HL_OUT int32_t *pi_rbc, // residual bits count
    HL_OUT int32_t *pi_dist, // distorsion
	HL_OUT int32_t *pi_CodedBlockPatternLuma4x4
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t luma4x4BlkIdx;
    hl_uint8_16x16_t* predPartLXL16x16_u8;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};
    hl_codec_264_residual_write_block_f f_write_block;
    const hl_pixel_t *_pc_SL_enc_mb;
    HL_ALIGNED(16) hl_int32_4x4_t* SL_res; // residual
    HL_ALIGNED(16) hl_int32_4x4_t* SL_rec; // reconstructed
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4;
    HL_ALIGNED(16) hl_uint8_4x4_t* tmp4x4_u8;
    HL_ALIGNED(16) hl_int32_16_t* LumaLevel; // luma level
    uint32_t u_SL_4x4_width_index, u_SL_4x4_height_index, u_SL_4x4_width_count, u_SL_4x4_height_count;
    hl_bool_t b_all_zeros;

    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    residual_inv_type.b_rdo = HL_TRUE;
    residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;

    *pi_Single_ctr = 0;
    *pi_rbc = 0;
    *pi_dist = 0;
	*pi_CodedBlockPatternLuma4x4 = 0;

    if (!pc_slice_header->pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // pc_SL_enc_mb = pc_esd->rdo.me.pc_SL_enc + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_res);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_rec);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4_u8);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &LumaLevel);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartLXL16x16_u8);

    // Reset bits
    err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
    CHECK_ERR_BAIL(err);
#if 0
    for (mbPartIdx = 0; mbPartIdx < p_mb->NumMbPart; ++mbPartIdx) {
        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
#else
    {
#endif
            // "xL_Idx" and "yL_Idx" are required for interpolation
            p_mb->xL_Idx = pc_esd->rdo.me.xL_Idx[mbPartIdx][subMbPartIdx];
            p_mb->yL_Idx = pc_esd->rdo.me.yL_Idx[mbPartIdx][subMbPartIdx];
            err = hl_codec_264_interpol_luma(
                p_codec,
                p_mb,
                mbPartIdx,
                subMbPartIdx,
                pc_mvLX,
                pc_esd->rdo.me.pc_SL_ref,
                &(*predPartLXL16x16_u8)[0][0], 1/*sizeof(uint8_t)*/);
            CHECK_ERR_BAIL(err);
            u_SL_4x4_width_count = p_mb->partWidth[mbPartIdx][subMbPartIdx] >> 2;
            u_SL_4x4_height_count = p_mb->partHeight[mbPartIdx][subMbPartIdx] >> 2;
            for (u_SL_4x4_height_index = 0; u_SL_4x4_height_index < u_SL_4x4_height_count; ++u_SL_4x4_height_index) {
                _pc_SL_enc_mb = pc_esd->rdo.me.pc_SL_enc + p_mb->xL_Idx + ((p_mb->yL_Idx + (u_SL_4x4_height_index << 2))* pc_slice_header->PicWidthInSamplesL);
                for (u_SL_4x4_width_index = 0; u_SL_4x4_width_index < u_SL_4x4_width_count; ++u_SL_4x4_width_index) {
                    luma4x4BlkIdx = LumaBlockIndices4x4_YX[pc_esd->rdo.me.yP[mbPartIdx][subMbPartIdx] + pc_esd->rdo.me.yS[mbPartIdx][subMbPartIdx] + (u_SL_4x4_height_index << 2)][pc_esd->rdo.me.xP[mbPartIdx][subMbPartIdx] + pc_esd->rdo.me.xS[mbPartIdx][subMbPartIdx] + (u_SL_4x4_width_index << 2)];
                    // Compute residual, Luma
                    // TODO: Use threadholds
                    err = hl_codec_264_rdo_mb_compute_inter_luma4x4(
                        p_mb, p_codec,
                        _pc_SL_enc_mb, pc_slice_header->PicWidthInSamplesL,
                        &(*predPartLXL16x16_u8)[(u_SL_4x4_height_index << 2)][(u_SL_4x4_width_index << 2)], 16,
                        (*SL_res), (*LumaLevel), &b_all_zeros);
                    CHECK_ERR_BAIL(err);
                    // Compute bitrate
                    if (!b_all_zeros) {
                        static const hl_bool_t __isLumaTrue = HL_TRUE;
                        static const hl_bool_t __isIntra16x16False = HL_FALSE;

                        residual_inv_type.i_luma4x4BlkIdx = luma4x4BlkIdx;
                        err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*LumaLevel), 0, 15, 16);
                        if (err) {
                            goto bail;
                        }

                        // Residual exist means decode samples and reconstruct
                        // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                        InverseScan4x4((*LumaLevel), (*tmp4x4));
                        // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                        hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*tmp4x4), (*SL_res), __isLumaTrue, __isIntra16x16False, -1);
						// Add predicted samples to residual and clip the result
						hl_math_addclip_4x4_u8xi32(
							&(*predPartLXL16x16_u8)[(u_SL_4x4_height_index << 2)][(u_SL_4x4_width_index << 2)], 16,
							(const int32_t*)(*SL_res), 4,
							(uint8_t*)(*tmp4x4_u8), 4);

                        // Compute distorsion
#if 0 // FIXME
                        (*pi_dist) += hl_math_ssd4x4_u8(
#else
                        (*pi_dist) += hl_math_sad4x4_u8(
#endif
                                         _pc_SL_enc_mb, pc_slice_header->PicWidthInSamplesL,
                                         (const uint8_t*)(*tmp4x4_u8), 4
                                     );
                        // Update "Single_ctr"
                        (*pi_Single_ctr) += pc_esd->rdo.Single_ctr;
						// Update "CodedBlockPatternLuma4x4"
						(*pi_CodedBlockPatternLuma4x4) |= (1 << luma4x4BlkIdx);
                    }
                    else {
                        // Residual doesn't exist

                        // Compute distorsion
#if 0 // FIXME
                        (*pi_dist) += hl_math_ssd4x4_u8(
#else
                        (*pi_dist) += hl_math_sad4x4_u8(
#endif
                                         _pc_SL_enc_mb, pc_slice_header->PicWidthInSamplesL,
                                         &(*predPartLXL16x16_u8)[(u_SL_4x4_height_index << 2)][(u_SL_4x4_width_index << 2)], 16
                                     );
                    }
                    _pc_SL_enc_mb += 4;
                } // end-of-for (u_SL_4x4_width_index...
            } // end-of-for (u_SL_4x4_height_index...
#if 0
        } // end-of-for (subMbPartIdx...)
    } // end-of-for (mbPartIdx...)
#else
        }
#endif
    (*pi_rbc) = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->rdo.pobj_bits);

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_res);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_rec);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4_u8);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, LumaLevel);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartLXL16x16_u8);
    return err;
}

