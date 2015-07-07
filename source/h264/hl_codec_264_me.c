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

extern HL_ERROR_T hl_codec_264_interpol_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvLX,
    const hl_pixel_t* cSL,
    HL_OUT_ALIGNED(16) void* predPartLXL16x16, int32_t predPartLXLSampleSize);


static HL_INLINE int32_t _hl_codec_264_me_get_distorsion(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, // Reference frame (previsously decoded and reconstructed frame) - at MB(0,0)
    HL_IN const hl_pixel_t* pc_SL_enc_mb, // Current frame to encode - at MB(0,0)
    HL_IN int32_t luma4x4BlkIdxStart, // First 4x4 luma blocks to process (In raster scan)
    HL_IN int32_t luma4x4BlkIdxCountWidth, // Number of 4x4 luma blocks to process (col)
    HL_IN int32_t luma4x4BlkIdxCountHeight // Number of 4x4 luma blocks to process (col))
);

// Compute residual bits count ("rbc") and distorsion ("dist")
static HL_INLINE HL_ERROR_T _hl_codec_264_me_get_rbc_and_dist(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, HL_IN uint32_t u_SL_ref_mb_stride, HL_IN uint32_t u_SL_ref_mb_x0, HL_IN uint32_t u_SL_ref_mb_y0,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride, HL_IN uint32_t u_SL_enc_mb_x0, HL_IN uint32_t u_SL_enc_mb_y0,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_OUT int32_t *p_rbc, // residual bits count
    HL_OUT int32_t *p_dist // distorsion
);

static HL_ERROR_T _UMHexagonS(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref, // Reference frame (previsously decoded and reconstructed frame) - at Picture(0,0)
    HL_IN const hl_pixel_t* pc_SL_enc,  // Current frame to encode - at Picture(0,0)
    HL_IN uint32_t u_SL_mb_x0, HL_IN uint32_t u_SL_mb_y0, HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN uint32_t me_range,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN int32_t refIdxLX,
    HL_IN int32_t predMode,
    HL_IN HL_CODEC_264_SUBMB_TYPE_T currSubMbType,
    HL_IN HL_CODEC_264_LIST_IDX_T listSuffix,
    HL_OUT int32_t *pi_Single_ctr, // Best "Single_ctr" - JVT-O079 2.3 Elimination of single coefficients in inter macroblocks
    HL_OUT hl_bool_t *pb_probably_pskip, // Whether the best pos could be PSkip
    HL_OUT hl_codec_264_mv_xt* mvpLX, // Predicted motion vector (used to compute MVD)
    HL_OUT int32_t *pi_best_dist, // Best distortion (OUT)
    HL_OUT double *pd_best_cost, // Best cost (OUT)
    HL_OUT int32_t best_pos[2] // Best pos (x,y)
);

HL_ERROR_T hl_codec_264_me_mb_find_best_cost(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref,
    HL_IN const hl_pixel_t* pc_SL_enc,
    HL_IN HL_CODEC_264_SUBMB_TYPE_T currSubMbType,
    HL_IN HL_CODEC_264_LIST_IDX_T listSuffix,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN int32_t refIdxLX,
    HL_IN int32_t predMode,
    HL_IN uint32_t u_SL_mb_x0, HL_IN uint32_t u_SL_mb_y0, HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN uint32_t me_range,
    HL_OUT int32_t *pi_Single_ctr, // Best "Single_ctr" - JVT-O079 2.3 Elimination of single coefficients in inter macroblocks
    HL_OUT hl_bool_t *pb_probably_pskip, // Whether the best pos could be PSkip
    HL_OUT hl_codec_264_mv_xt* mvpLX, // Predicted motion vector (used to compute MVD)
    HL_OUT int32_t *pi_best_dist, // Best distortion (OUT)
    HL_OUT double *pd_best_cost, // Best cost (OUT)
    HL_OUT int32_t best_pos[2] // Best pos (x,y)
)
{
    HL_ERROR_T err;

    *pb_probably_pskip = HL_FALSE;
    *pi_Single_ctr = 9;

    err = _UMHexagonS(
              p_mb,
              p_codec,
              pc_SL_ref, // Reference frame (previsously decoded and reconstructed frame) - at Picture(0,0)
              pc_SL_enc,  // Current frame to encode - at Picture(0,0)
              u_SL_mb_x0, u_SL_mb_y0, u_SL_4x4_width_count, u_SL_4x4_height_count,
              me_range,
              mbPartIdx,
              subMbPartIdx,
              refIdxLX,
              predMode,
              currSubMbType,
              listSuffix,
              pi_Single_ctr,
              pb_probably_pskip,
              mvpLX,
              pi_best_dist,
              pd_best_cost,
              best_pos
          );

    return err;
}

// Compute distorsion (SAD only for now)
static HL_INLINE int32_t _hl_codec_264_me_get_distorsion(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, // Reference frame (previsously decoded and reconstructed frame) - at MB(0,0)
    HL_IN const hl_pixel_t* pc_SL_enc_mb, // Current frame to encode - at MB(0,0)
    HL_IN int32_t luma4x4BlkIdxStart, // First 4x4 luma blocks to process (In raster scan)
    HL_IN int32_t luma4x4BlkIdxCountWidth, // Number of 4x4 luma blocks to process (col)
    HL_IN int32_t luma4x4BlkIdxCountHeight // Number of 4x4 luma blocks to process (col))
)
{
    int32_t luma4x4BlkIdx, i_dist = 0, xO, yO, luma4x4BlkIdxEnd;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_pixel_t *_pc_SL_ref_mb, *_pc_SL_enc_mb;

    pc_slice_header = p_codec->layers.pc_active->pc_slice_hdr;
    luma4x4BlkIdxEnd = luma4x4BlkIdxStart + (luma4x4BlkIdxCountWidth * luma4x4BlkIdxCountHeight);

    for (luma4x4BlkIdx = luma4x4BlkIdxStart; luma4x4BlkIdx < luma4x4BlkIdxEnd; ++luma4x4BlkIdx) {
        xO = Raster4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
        yO = Raster4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
        _pc_SL_ref_mb = (pc_SL_ref_mb + (_pc_SL_enc_mb - pc_SL_enc_mb));
        _pc_SL_enc_mb = (pc_SL_enc_mb + xO + (yO * pc_slice_header->PicWidthInSamplesL));
        i_dist += p_codec->encoder.fn_distortion_compute_4x4_u8(
                      _pc_SL_ref_mb, pc_slice_header->PicWidthInSamplesL,
                      _pc_SL_enc_mb, pc_slice_header->PicWidthInSamplesL
                  );
    }

    return i_dist;
}

// Compute residual bits count (in bits)
// "rbc" = residual bits count
static HL_INLINE HL_ERROR_T _hl_codec_264_me_get_rbc(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, // Reference frame (previsously decoded and reconstructed frame) - at MB(0,0)
    HL_IN const hl_pixel_t* pc_SL_enc_mb, // Current frame to encode - at MB(0,0)
    HL_IN int32_t luma4x4BlkIdxStart, // First 4x4 luma blocks to process (In raster scan)
    HL_IN int32_t luma4x4BlkIdxCountWidth, // Number of 4x4 luma blocks to process (col)
    HL_IN int32_t luma4x4BlkIdxCountHeight, // Number of 4x4 luma blocks to process (col))
    HL_OUT int32_t *p_bits
)
{
    int32_t luma4x4BlkIdx, xO, yO, luma4x4BlkIdxEnd;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_pixel_t *_pc_SL_ref_mb, *_pc_SL_enc_mb;
    HL_ALIGNED(16) hl_int32_4x4_t* SL_res; // residual
    HL_ALIGNED(16) hl_int32_16_t* LumaLevel; // luma level
    hl_codec_264_residual_write_block_f f_write_block;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};
    hl_codec_264_encode_slice_data_t* pc_esd;
    HL_ERROR_T err;
    hl_bool_t b_all_zeros;

    *p_bits = 0;
    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    luma4x4BlkIdxEnd = luma4x4BlkIdxStart + (luma4x4BlkIdxCountWidth * luma4x4BlkIdxCountHeight);
    residual_inv_type.b_rdo = HL_TRUE;
    residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;

    if (!pc_slice_header->pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_res);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &LumaLevel);

    for (luma4x4BlkIdx = luma4x4BlkIdxStart; luma4x4BlkIdx < luma4x4BlkIdxEnd; ++luma4x4BlkIdx) {
        xO = Raster4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
        yO = Raster4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
        _pc_SL_ref_mb = (pc_SL_ref_mb + (_pc_SL_enc_mb - pc_SL_enc_mb));
        _pc_SL_enc_mb = (pc_SL_enc_mb + xO + (yO * pc_slice_header->PicWidthInSamplesL));

        // Reset bits
        err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
        if (err) {
            goto bail;
        }
        // Compute residual, Luma
        // TODO: Use threadholds
        err = hl_codec_264_rdo_mb_compute_inter_luma4x4(
                  p_mb, p_codec,
                  _pc_SL_enc_mb, pc_slice_header->PicWidthInSamplesL,
                  _pc_SL_ref_mb, pc_slice_header->PicWidthInSamplesL,
                  (*SL_res), (*LumaLevel), &b_all_zeros);
        if (err) {
            goto bail;
        }
        // Compute bitrate
        if (!b_all_zeros) {
            err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*LumaLevel), 0, 15, 16);
            if (err) {
                goto bail;
            }
            (*p_bits) += (int32_t)((pc_esd->rdo.pobj_bits->pc_current - pc_esd->rdo.pobj_bits->pc_start) << 3) + (7 - pc_esd->rdo.pobj_bits->i_bits_count);
        }
    }

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_res);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, LumaLevel);
    return err;
}

// Compute residual bits count ("rbc") and distorsion ("dist")
static HL_INLINE HL_ERROR_T _hl_codec_264_me_get_rbc_and_dist(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, HL_IN uint32_t u_SL_ref_mb_stride, HL_IN uint32_t u_SL_ref_mb_x0, HL_IN uint32_t u_SL_ref_mb_y0,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride, HL_IN uint32_t u_SL_enc_mb_x0, HL_IN uint32_t u_SL_enc_mb_y0,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_OUT int32_t *p_rbc, // residual bits count
    HL_OUT int32_t *p_dist // distorsion
)
{
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_pixel_t *_pc_SL_ref_mb, *_pc_SL_enc_mb;
    HL_ALIGNED(16) hl_int32_4x4_t* SL_res; // residual
    HL_ALIGNED(16) hl_int32_4x4_t* SL_rec; // reconstructed
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4;
    HL_ALIGNED(16) hl_uint8_4x4_t* tmp4x4_u8;
    HL_ALIGNED(16) hl_int32_16_t* LumaLevel; // luma level
    uint32_t u_SL_4x4_width_index, u_SL_4x4_height_index;
    hl_codec_264_residual_write_block_f f_write_block;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};
    HL_ERROR_T err;
    hl_bool_t b_all_zeros;

    *p_rbc = 0;
    *p_dist = 0;
    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    residual_inv_type.b_rdo = HL_TRUE;
    residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;

    if (!pc_slice_header->pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_res);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_rec);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4_u8);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &LumaLevel);

    for (u_SL_4x4_width_index = 0; u_SL_4x4_width_index < u_SL_4x4_width_count; ++u_SL_4x4_width_index) {
        for (u_SL_4x4_height_index = 0; u_SL_4x4_height_index < u_SL_4x4_height_count; ++u_SL_4x4_height_index) {
            _pc_SL_enc_mb = (pc_SL_enc_mb + (u_SL_enc_mb_x0 + (u_SL_4x4_width_index << 2)) + ((u_SL_enc_mb_y0 + (u_SL_4x4_height_index << 2)) * u_SL_enc_mb_stride));
            _pc_SL_ref_mb = (pc_SL_ref_mb + (u_SL_ref_mb_x0 + (u_SL_4x4_width_index << 2)) + ((u_SL_ref_mb_y0 + (u_SL_4x4_height_index << 2)) * u_SL_ref_mb_stride));

            // Reset bits
            err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
            if (err) {
                goto bail;
            }
            // Compute residual, Luma
            // TODO: Use threadholds
            err = hl_codec_264_rdo_mb_compute_inter_luma4x4(
                      p_mb, p_codec,
                      _pc_SL_enc_mb, u_SL_enc_mb_stride,
                      _pc_SL_ref_mb, u_SL_ref_mb_stride,
                      (*SL_res), (*LumaLevel), &b_all_zeros);
            if (err) {
                goto bail;
            }
            // Compute bitrate
            if (!b_all_zeros) {
                static const hl_bool_t __isLumaTrue = HL_TRUE;
                static const hl_bool_t __isIntra16x16False = HL_FALSE;

                residual_inv_type.i_luma4x4BlkIdx = LumaBlockIndices4x4_YX[(u_SL_enc_mb_y0 + (u_SL_4x4_height_index << 2))/*yP*/][(u_SL_enc_mb_x0 + (u_SL_4x4_width_index << 2))/*xP*/];
                err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*LumaLevel), 0, 15, 16);
                if (err) {
                    goto bail;
                }
                (*p_rbc) += (int32_t)((pc_esd->rdo.pobj_bits->pc_current - pc_esd->rdo.pobj_bits->pc_start) << 3) + (7 - pc_esd->rdo.pobj_bits->i_bits_count);

                // Residual exist means decode samples and reconstruct
                // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                InverseScan4x4((*LumaLevel), (*tmp4x4));
                // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*tmp4x4), (*SL_res), __isLumaTrue, __isIntra16x16False, -1);
#if 0
                // Add predicted samples to residual and clip the result
                hl_math_addclip_4x4_u8xu32(_pc_SL_ref_mb, u_SL_ref_mb_stride, (const int32_t*)(*SL_res), 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4), 4);
                // convert to u8
                hl_memory_copy4x4_u32_to_u8_stride4x4((uint8_t*)(*tmp4x4_u8), (const uint32_t *)(*tmp4x4));
#else
				hl_math_addclip_4x4_u8xi32(_pc_SL_ref_mb, u_SL_ref_mb_stride, (const int32_t*)(*SL_res), 4, (uint8_t*)(*tmp4x4_u8), 4);
#endif
                // Compute distorsion
                (*p_dist) += p_codec->encoder.fn_distortion_compute_4x4_u8(
                                 _pc_SL_enc_mb, u_SL_enc_mb_stride,
                                 (const uint8_t*)(*tmp4x4_u8), 4
                             );
            }
            else {
                // Residual doesn't exist

                // Compute distorsion
                (*p_dist) += p_codec->encoder.fn_distortion_compute_4x4_u8(
                                 _pc_SL_enc_mb, u_SL_enc_mb_stride,
                                 _pc_SL_ref_mb, u_SL_ref_mb_stride
                             );
            }
        } // end-of-for (u_SL_4x4_height_index...
    } // end-of-for (u_SL_4x4_width_index...

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_res);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_rec);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4_u8);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, LumaLevel);

    return err;
}


static HL_ERROR_T _UMHexagonS_GetCostAndDist(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN hl_codec_264_mv_xt* p_mvLX,
    HL_IN const hl_pixel_t* pc_SL_ref,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN HL_ALIGNED(16) uint8_t tmpPredPartLXL_u8[16][16],
    HL_OUT int32_t *p_rbc, // residual bits count
    HL_OUT int32_t *p_dist // distorsion
)
{
    int32_t partHeight, partWidth;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    partHeight = p_mb->partHeight[mbPartIdx][subMbPartIdx];
    partWidth = p_mb->partWidth[mbPartIdx][subMbPartIdx];

    err = hl_codec_264_interpol_luma(
              p_codec,
              p_mb,
              mbPartIdx,
              subMbPartIdx,
              p_mvLX,
              pc_SL_ref,
              tmpPredPartLXL_u8, sizeof(tmpPredPartLXL_u8[0][0]));
    if (err) {
        goto bail;
    }

    // compute residual bits count ("rbc") and distorsion ("dist")
    err = _hl_codec_264_me_get_rbc_and_dist(
              p_mb,
              p_codec,
              (const uint8_t*)tmpPredPartLXL_u8, 16, 0, 0,
              pc_SL_enc_mb, u_SL_enc_mb_stride, 0, 0,
              u_SL_4x4_width_count,
              u_SL_4x4_height_count,
              p_rbc,
              p_dist
          );
    if (err) {
        return err;
    }
bail:
    return err;
}

static HL_ALWAYS_INLINE HL_ERROR_T _GetDist_Motion(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, HL_IN uint32_t u_SL_ref_mb_stride, HL_IN uint32_t u_SL_ref_mb_x0, HL_IN uint32_t u_SL_ref_mb_y0,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride, HL_IN uint32_t u_SL_enc_mb_x0, HL_IN uint32_t u_SL_enc_mb_y0,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_OUT int32_t *p_dist // distorsion
)
{
    const hl_pixel_t *_pc_SL_ref_mb, *_pc_SL_enc_mb;
    uint32_t u_SL_4x4_width_index, u_SL_4x4_height_index, uYL0;

    *p_dist = 0;
#if 1
    for (u_SL_4x4_height_index = 0; u_SL_4x4_height_index < u_SL_4x4_height_count; ++u_SL_4x4_height_index) {
        uYL0 = (u_SL_ref_mb_y0 + (u_SL_4x4_height_index << 2));
        _pc_SL_enc_mb = (pc_SL_enc_mb + (uYL0 * u_SL_enc_mb_stride)) + u_SL_enc_mb_x0;
        _pc_SL_ref_mb = (pc_SL_ref_mb + (uYL0 * u_SL_ref_mb_stride)) + u_SL_ref_mb_x0;
        for (u_SL_4x4_width_index = 0; u_SL_4x4_width_index < u_SL_4x4_width_count; ++u_SL_4x4_width_index) {
            *(p_dist) += hl_math_sad4x4_u8(
                             _pc_SL_enc_mb, u_SL_enc_mb_stride,
                             _pc_SL_ref_mb, u_SL_ref_mb_stride);
            _pc_SL_enc_mb += 4;
            _pc_SL_ref_mb += 4;
        } // end-of-for (u_SL_4x4_width_index...
    } // end-of-for (u_SL_4x4_height_index...

#else
    for (u_SL_4x4_width_index = 0; u_SL_4x4_width_index < u_SL_4x4_width_count; ++u_SL_4x4_width_index) {
        for (u_SL_4x4_height_index = 0; u_SL_4x4_height_index < u_SL_4x4_height_count; ++u_SL_4x4_height_index) {
            // FIXME: could be less CPU intensive
            uXL0 = (u_SL_enc_mb_x0 + (u_SL_4x4_width_index << 2));
            uYL0 = (u_SL_ref_mb_y0 + (u_SL_4x4_height_index << 2));
            _pc_SL_enc_mb = (pc_SL_enc_mb + uXL0 + (uYL0 * u_SL_enc_mb_stride));
            _pc_SL_ref_mb = (pc_SL_ref_mb + uXL0 + (uYL0 * u_SL_ref_mb_stride));

            *(p_dist) += hl_math_sad4x4_u8(
                             _pc_SL_enc_mb, u_SL_enc_mb_stride,
                             _pc_SL_ref_mb, u_SL_ref_mb_stride);
        } // end-of-for (u_SL_4x4_height_index...
    } // end-of-for (u_SL_4x4_width_index...
#endif

    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _GetDist2_Motion(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN hl_codec_264_mv_xt* p_mvLX,
    HL_IN const hl_pixel_t* pc_SL_ref,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN HL_ALIGNED(16) uint8_t tmpPredPartLXL_u8[16][16],
    HL_OUT int32_t *p_dist // distorsion
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    err = hl_codec_264_interpol_luma(
              p_codec,
              p_mb,
              mbPartIdx,
              subMbPartIdx,
              p_mvLX,
              pc_SL_ref,
              tmpPredPartLXL_u8, sizeof(tmpPredPartLXL_u8[0][0]));
    if (err) {
        goto bail;
    }

    // compute residual bits count ("rbc") and distorsion ("dist")
    err = _GetDist_Motion(
              p_mb,
              p_codec,
              (const uint8_t*)tmpPredPartLXL_u8, 16, 0, 0,
              pc_SL_enc_mb, u_SL_enc_mb_stride, 0, 0,
              u_SL_4x4_width_count,
              u_SL_4x4_height_count,
              p_dist
          );
    if (err) {
        return err;
    }
bail:
    return err;
}

// FIXME: remove
// Compute residual bits count ("rbc") and distorsion ("dist")
static HL_INLINE HL_ERROR_T _GetRbcAndDist_Mode(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref_mb, HL_IN uint32_t u_SL_ref_mb_stride, HL_IN uint32_t u_SL_ref_mb_x0, HL_IN uint32_t u_SL_ref_mb_y0,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride, HL_IN uint32_t u_SL_enc_mb_x0, HL_IN uint32_t u_SL_enc_mb_y0,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_OUT int32_t *pi_Single_ctr,
    HL_OUT int32_t *p_rbc, // residual bits count
    HL_OUT int32_t *p_dist // distorsion
)
{
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};
    hl_codec_264_residual_write_block_f f_write_block;
    const hl_pixel_t *_pc_SL_ref_mb, *_pc_SL_enc_mb;
    HL_ALIGNED(16) hl_int32_4x4_t* SL_res; // residual
    HL_ALIGNED(16) hl_int32_4x4_t* SL_rec; // reconstructed
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4;
    HL_ALIGNED(16) hl_uint8_4x4_t* tmp4x4_u8;
    HL_ALIGNED(16) hl_int32_16_t* LumaLevel; // luma level
    uint32_t u_SL_4x4_width_index, u_SL_4x4_height_index, uXL0, uYL0;
    HL_ERROR_T err;
    hl_bool_t b_all_zeros;

    *p_rbc = 0;
    *p_dist = 0;
    *pi_Single_ctr = 0;
    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    residual_inv_type.b_rdo = HL_TRUE;
    residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;

    if (!pc_slice_header->pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_res);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &SL_rec);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4_u8);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &LumaLevel);

    // Reset bits
    err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
    if (err) {
        goto bail;
    }

    for (u_SL_4x4_width_index = 0; u_SL_4x4_width_index < u_SL_4x4_width_count; ++u_SL_4x4_width_index) {
        for (u_SL_4x4_height_index = 0; u_SL_4x4_height_index < u_SL_4x4_height_count; ++u_SL_4x4_height_index) {
            uXL0 = (u_SL_enc_mb_x0 + (u_SL_4x4_width_index << 2));
            uYL0 = (u_SL_enc_mb_y0 + (u_SL_4x4_height_index << 2));
            _pc_SL_enc_mb = pc_SL_enc_mb + uXL0 + (uYL0 * u_SL_enc_mb_stride);
            _pc_SL_ref_mb = pc_SL_ref_mb + uXL0 + (uYL0 * u_SL_ref_mb_stride);

            // Compute residual, Luma
            // TODO: Use threadholds
            err = hl_codec_264_rdo_mb_compute_inter_luma4x4(
                      p_mb, p_codec,
                      _pc_SL_enc_mb, u_SL_enc_mb_stride,
                      _pc_SL_ref_mb, u_SL_ref_mb_stride,
                      (*SL_res), (*LumaLevel), &b_all_zeros);
            if (err) {
                goto bail;
            }
            // Compute bitrate
            if (!b_all_zeros) {
                static const hl_bool_t __isLumaTrue = HL_TRUE;
                static const hl_bool_t __isIntra16x16False = HL_FALSE;

                residual_inv_type.i_luma4x4BlkIdx = LumaBlockIndices4x4_YX[uYL0][uXL0];
                err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*LumaLevel), 0, 15, 16);
                if (err) {
                    goto bail;
                }

                // Residual exist means decode samples and reconstruct
                // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                InverseScan4x4((*LumaLevel), (*tmp4x4));
                // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*tmp4x4), (*SL_res), __isLumaTrue, __isIntra16x16False, -1);
#if 0
                // Add predicted samples to residual and clip the result
                hl_math_addclip_4x4_u8xu32(_pc_SL_ref_mb, u_SL_ref_mb_stride, (const int32_t*)(*SL_res), 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4), 4);
                // convert to u8
                hl_memory_copy4x4_u32_to_u8_stride4x4((uint8_t*)(*tmp4x4_u8), (const uint32_t *)(*tmp4x4));
#else
				hl_math_addclip_4x4_u8xi32(_pc_SL_ref_mb, u_SL_ref_mb_stride, (const int32_t*)(*SL_res), 4, (uint8_t*)(*tmp4x4_u8), 4);
#endif
                // Compute distorsion
                (*p_dist) += hl_math_ssd4x4_u8(
                                 _pc_SL_enc_mb, u_SL_enc_mb_stride,
                                 (const uint8_t*)(*tmp4x4_u8), 4
                             );
                // Update "Single_ctr"
                (*pi_Single_ctr) += pc_esd->rdo.Single_ctr;
            }
            else {
                // Residual doesn't exist

                // Compute distorsion
                (*p_dist) += hl_math_ssd4x4_u8(
                                 _pc_SL_enc_mb, u_SL_enc_mb_stride,
                                 _pc_SL_ref_mb, u_SL_ref_mb_stride
                             );
            }
        } // end-of-for (u_SL_4x4_height_index...
    } // end-of-for (u_SL_4x4_width_index...

    (*p_rbc) = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->rdo.pobj_bits);
    //(*p_rbc) = hl_math_nnz16((*LumaLevel));

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_res);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, SL_rec);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4_u8);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, LumaLevel);

    return err;
}


// FIXME: remove
static HL_ERROR_T _GetRbcAndDist2_Mode(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN hl_codec_264_mv_xt* p_mvLX,
    HL_IN const hl_pixel_t* pc_SL_ref,
    HL_IN const hl_pixel_t* pc_SL_enc_mb, HL_IN uint32_t u_SL_enc_mb_stride,
    HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN HL_ALIGNED(16) uint8_t tmpPredPartLXL_u8[16][16],
    HL_OUT int32_t *pi_Single_ctr,
    HL_OUT int32_t *p_rbc, // residual bits count
    HL_OUT int32_t *p_dist // distorsion
)
{
    HL_ERROR_T err = hl_codec_264_interpol_luma(
                         p_codec,
                         p_mb,
                         mbPartIdx,
                         subMbPartIdx,
                         p_mvLX,
                         pc_SL_ref,
                         tmpPredPartLXL_u8, sizeof(tmpPredPartLXL_u8[0][0]));
    if (err) {
        goto bail;
    }

    // compute residual bits count ("rbc") and distorsion ("dist")
    err = _GetRbcAndDist_Mode(
              p_mb,
              p_codec,
              (const uint8_t*)tmpPredPartLXL_u8, 16, 0, 0,
              pc_SL_enc_mb, u_SL_enc_mb_stride, 0, 0,
              u_SL_4x4_width_count,
              u_SL_4x4_height_count,
              pi_Single_ctr,
              p_rbc,
              p_dist
          );
    if (err) {
        return err;
    }
bail:
    return err;
}

// JVT-F017-  https://hl78965.googlecode.com/svn/trunk/clean/hartallo/documentation/JVT-F017.pdf
static HL_ERROR_T _UMHexagonS(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* pc_SL_ref, // Reference frame (previsously decoded and reconstructed frame) - at Picture(0,0)
    HL_IN const hl_pixel_t* pc_SL_enc,  // Current frame to encode - at Picture(0,0)
    HL_IN uint32_t u_SL_mb_x0, HL_IN uint32_t u_SL_mb_y0, HL_IN uint32_t u_SL_4x4_width_count, HL_IN uint32_t u_SL_4x4_height_count,
    HL_IN uint32_t me_range,
    HL_IN int32_t mbPartIdx,
    HL_IN int32_t subMbPartIdx,
    HL_IN int32_t refIdxLX,
    HL_IN int32_t predMode,
    HL_IN HL_CODEC_264_SUBMB_TYPE_T currSubMbType,
    HL_IN HL_CODEC_264_LIST_IDX_T listSuffix,
    HL_OUT int32_t *pi_Single_ctr, // Best "Single_ctr" - JVT-O079 2.3 Elimination of single coefficients in inter macroblocks
    HL_OUT hl_bool_t *pb_probably_pskip, // Whether the best pos could be PSkip
    HL_OUT hl_codec_264_mv_xt* mvpLX, // Predicted motion vector (used to compute MVD)
    HL_OUT int32_t *pi_best_dist, // Best distortion (OUT)
    HL_OUT double *pd_best_cost, // Best cost (OUT)
    HL_OUT int32_t best_pos[2] // Best pos (x,y)
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_mv_xt mvLX, mvBestLX, mvPSkip;
    HL_ALIGN(HL_ALIGN_V) uint8_t tmpPredPartLXL_u8[16][16]; // FIXME: map() - used to avoid creating new versions in the called functions
    int32_t i_int_x_start, i_int_y_start, i_rbc, i_dist, i_rbc_mv, i_best_dist, i_k, i_i, i_me_range_div4, i_cx, i_cy, i_int_cx, i_int_cy, i_guard, Single_ctr, i_best_Single_ctr;
    uint32_t u_SL_enc_stride, u_SL_ref_stride, u_blk_width, u_blk_height;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_layer_t* pc_layer;
    const hl_pixel_t *pc_SL_ref_mb, *pc_SL_enc_mb;
    double d_best_cost, d_cost, d_costUp, JTh1, JTh2;
    hl_bool_t b_check_pskip;
    hl_rect_xt wnd_search;
    int32_t predModeUp;
    static const int32_t __IPelLShift = 2; // *4
    static const int32_t __IPelRShift = 2; // /4
    static const int32_t __IPelAnd = 3; // %4
    static const int32_t __GuardMaxVal = 15; // FIXME: should be 7 (see standard)

    // Omega16-HP => see JVT-O079 section 2.1.3.1 Step_3-2
    static const int32_t SixteenPointsHexagonPattern[16][2/*x,y*/] = {
        {4, 2}, {-4, 2}, {4, -2}, {-4, -2}, // (+-4,+-2)
        {4, 1}, {-4, 1}, {4, -1}, {-4, -1}, // (+-4,+-1)
        {4, 0}, {-4, 0}, // (+-4, 0)
        {2, 3}, {-2, 3}, {2, -3}, {-2, -3}, // (+-2,+-3)
        {0, 4}, {0, -4}, // (0, +-4)
    };
    static const int32_t SixteenPointsHexagonPatternCount = sizeof(SixteenPointsHexagonPattern)/sizeof(SixteenPointsHexagonPattern[0]);

    // Omega3 => see JVT-O079 section 2.1.3.1 Step_4-1
    static const int32_t HexagonSearchPattern[6][2/*x,y*/] = {
        {2, 0}, {-2, 0}, // (cmx +-2, cmy)
        {1, 2}, {-1, 2}, {1, -2}, {-1, -2} // (cmx +-1, cmy+-2)
    };
    static const int32_t HexagonSearchPatternCount = sizeof(HexagonSearchPattern)/sizeof(HexagonSearchPattern[0]);

    // Omega4 => see JVT-O079 section 2.1.3.1 Step_4-2
    static const int32_t DiamondSearchIntPattern[4][2/*x,y*/] = {
        {1, 0}, {-1, 0}, // (cmx +-1, cmy)
        {0, 1}, {0, -1}, // (cmx, cmy +-1)
    };
    static const int32_t DiamondSearchIntPatternCount = sizeof(DiamondSearchIntPattern)/sizeof(DiamondSearchIntPattern[0]);

#if 0 // FIXME: this one is correct
    // Omega5 => see JVT-O079 section 2.1.3.2 Step_2
    static const int32_t DiamondSearchFractPattern[4][2/*x,y*/] = {
        {1, 0}, {-1, 0}, // (cmx +-1, cmy)
        {0, 1}, {0, -1}, // (cmx, cmy +-1)
    };
    static const int32_t DiamondSearchFractPatternCount = sizeof(DiamondSearchFractPattern)/sizeof(DiamondSearchFractPattern[0]);
#else
    // Omega5 => see JVT-O079 section 2.1.3.2 Step_2
    static const int32_t DiamondSearchFractPattern[7][2/*x,y*/] = {
        {1, 0}, {-1, 0}, {1, -1}, {1, 1}, {-1, -1},
        {0, 1}, {0, -1},
    };
    static const int32_t DiamondSearchFractPatternCount = sizeof(DiamondSearchFractPattern)/sizeof(DiamondSearchFractPattern[0]);
#endif

#define JVT_O079_CHECK_EARLY_TEMINATION(__cost, __th1, __th2) \
	if ((__cost) < (__th1))  goto Step_42; \
	else if ((__cost) < (__th2)) goto Step_41; \
 
#define CONTINUE_IF_ICX_INVALID() if ((p_mb->xL + i_cx) < 0 || ((p_mb->xL + i_cx) + u_blk_width) >= (int32_t)pc_slice_header->PicWidthInSamplesL) continue;
#define CONTINUE_IF_ICY_INVALID() if ((p_mb->yL + i_cy) < 0 || ((p_mb->yL + i_cy) + u_blk_height) >= (int32_t)pc_slice_header->PicHeightInSamplesL) continue;
#define CHECK_ERR(_err_) if ((_err_)) goto bail;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;

    u_blk_width = (u_SL_4x4_width_count << 2);
    u_blk_height = (u_SL_4x4_height_count << 2);

    i_int_x_start = p_mb->xL + u_SL_mb_x0;
    i_int_y_start = p_mb->yL + u_SL_mb_y0;

    u_SL_enc_stride = pc_slice_header->PicWidthInSamplesL;
    u_SL_ref_stride = pc_slice_header->PicWidthInSamplesL;

    pc_SL_enc_mb = (pc_SL_enc + i_int_x_start + (i_int_y_start * u_SL_enc_stride));
    (pc_SL_ref_mb);

    i_me_range_div4 = (me_range >> 2);

    *pb_probably_pskip = HL_FALSE;
    *pi_Single_ctr = 9;

    // Prepare parts for Half and Quater
    //p_mb->partWidth[mbPartIdx][subMbPartIdx] = u_blk_width;
    //p_mb->partHeight[mbPartIdx][subMbPartIdx] = u_blk_height;

    d_best_cost = DBL_MAX;
    i_best_Single_ctr = 9;

    JTh1 = 0;
    JTh2 = 0;

    if (p_mb->u_addr == 2) {
        int a = 0; // FIXME
    }

    // 8.4.1.3 Derivation process for luma motion vector prediction
    err = hl_codec_264_utils_derivation_process_for_luma_movect_prediction(
              p_codec,
              p_mb,
              mbPartIdx,
              subMbPartIdx,
              refIdxLX,
              currSubMbType,
              mvpLX,
              listSuffix);
    CHECK_ERR(err);

    b_check_pskip = (predMode == HL_CODEC_264_MODE_16X16);
    if (b_check_pskip) {
        int32_t refIdxL0;
        hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(p_codec, p_mb, &mvPSkip, &refIdxL0);
        b_check_pskip = (refIdxL0 == 0);
    }

    // Get cost for PSkip
    if (b_check_pskip) {
        err = _GetRbcAndDist2_Mode(
                  p_mb,
                  p_codec,
                  mbPartIdx,
                  subMbPartIdx,
                  &mvPSkip,
                  pc_SL_ref,
                  pc_SL_enc_mb, u_SL_enc_stride,
                  u_SL_4x4_width_count, u_SL_4x4_height_count,
                  tmpPredPartLXL_u8,
                  &Single_ctr,
                  &i_rbc, &i_dist);
        if (i_rbc == 0) {
            *pb_probably_pskip = HL_TRUE;
            *pi_Single_ctr = 0;
            best_pos[0] = mvPSkip.x, best_pos[1] = mvPSkip.y;
            *pd_best_cost = (double)i_dist;
            *pi_best_dist = i_dist;
            goto bail;
        }
        else {
            // "tmpPredPartLXL_u8" already computed in "_GetRbcAndDist2_Mode()"
            err = _GetDist_Motion(
                      p_mb,
                      p_codec,
                      (const uint8_t*)tmpPredPartLXL_u8, 16, 0, 0,
                      pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                      u_SL_4x4_width_count,
                      u_SL_4x4_height_count,
                      &i_dist
                  );
            i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvPSkip.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvPSkip.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
            d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
            if (d_cost < d_best_cost) {
                mvBestLX.x = mvPSkip.x, mvBestLX.y = mvPSkip.y;
                d_best_cost = d_cost;
                i_best_dist = i_dist;
            }
        }
    }

    switch (predMode) {
    case HL_CODEC_264_MODE_16X8:
    case HL_CODEC_264_MODE_8X16:
        predModeUp = HL_CODEC_264_MODE_16X16;
        break;
    case HL_CODEC_264_MODE_8X8_SUB8X8:
        predModeUp = HL_CODEC_264_MODE_16X8;
        break;
    case HL_CODEC_264_MODE_8X8_SUB8X4:
    case HL_CODEC_264_MODE_8X8_SUB4X8:
        predModeUp = HL_CODEC_264_MODE_8X8_SUB8X8;
        break;
    case HL_CODEC_264_MODE_8X8_SUB4X4:
        predModeUp = HL_CODEC_264_MODE_8X8_SUB8X4;
        break;
    }

    //**************************************************
    //	2.1 Starting search point prediction
    //**************************************************
    {
        const int32_t MVs[][2/*x,y*/] = {
            {mvpLX->x, mvpLX->y},
            {mvpLX->x + 1, mvpLX->y}, {mvpLX->x - 1, mvpLX->y}, {mvpLX->x, mvpLX->y + 1}, {mvpLX->x, mvpLX->y - 1},
            {0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}
        };
        // FIXME
        for (i_i = 0;  i_i < (sizeof(MVs)/(2 * sizeof(MVs[0][0]))); ++i_i) {
            mvLX.x = MVs[i_i][0], mvLX.y = MVs[i_i][1];
            err = _GetDist2_Motion(
                      p_mb,
                      p_codec,
                      mbPartIdx,
                      subMbPartIdx,
                      &mvLX,
                      pc_SL_ref,
                      pc_SL_enc_mb, u_SL_enc_stride,
                      u_SL_4x4_width_count, u_SL_4x4_height_count,
                      tmpPredPartLXL_u8,
                      &i_dist);
            CHECK_ERR(err);
            i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
            d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
            if (d_cost < d_best_cost) {
                mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
                d_best_cost = d_cost;
                i_best_dist = i_dist;
            }
        }
    }

    // Cost at Up
    if (predMode != 1) {
        mvLX.x = pc_esd->rdo.BestMV00[predModeUp][0], mvLX.y = pc_esd->rdo.BestMV00[predModeUp][1];
        err = _GetDist2_Motion(
                  p_mb,
                  p_codec,
                  mbPartIdx,
                  subMbPartIdx,
                  &mvLX,
                  pc_SL_ref,
                  pc_SL_enc_mb, u_SL_enc_stride,
                  u_SL_4x4_width_count, u_SL_4x4_height_count,
                  tmpPredPartLXL_u8,
                  &i_dist);
        CHECK_ERR(err);
        i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
        d_costUp = d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
        if (d_cost < d_best_cost) {
            mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
            d_best_cost = d_cost;
            i_best_dist = i_dist;
        }
    }


    // JVT-O079 2.1.3.1.2.2 Early termination
    if (p_codec->pc_base->me_early_term_flag) {
        double JPred, alpha, beta1, beta2;
        if (refIdxLX == 0) {
            if (predMode == 1) {
                // Median Prediction
                JPred = d_cost;
            }
            else {
                // Uplayer Prediction
                JPred = pc_esd->rdo.Costs[predModeUp] / 2;
                //JPred = d_costUp / 2;
            }
        }
        else {
            // FIXME: Computing "Neighboring Ref-frame Prediction" not supported yet
            JPred = d_cost;
        }
        alpha = pc_esd->rdo.Bsize[predMode] / (JPred * JPred);
        beta1 = alpha - HL_CODEC_264_EARLY_TERMINATION_APLHA1[predMode];
        beta2 = alpha - HL_CODEC_264_EARLY_TERMINATION_APLHA2[predMode];
        JTh1 = JPred * (1 + beta1);
        JTh2 = JPred * (1 + beta2);
    }

    JVT_O079_CHECK_EARLY_TEMINATION(d_best_cost, JTh1, JTh2);

    // define integer-pel center position at frame-unit
    i_int_cx = i_int_x_start + (mvBestLX.x >> __IPelRShift);
    i_int_cy = i_int_y_start + (mvBestLX.y >> __IPelRShift);

    //******************************************
    // 2.2 Unsymmetrical-cross search
    //******************************************

    // set search window
    wnd_search.left = (i_int_cx - me_range);
    wnd_search.right = (i_int_cx + me_range);
    wnd_search.top = (i_int_cy - (me_range << 1));  // Half range(W/2) for vertical
    wnd_search.bottom = (i_int_cy + (me_range << 1)); // Half range(W/2) for vertical
    // clip search window window
    if (wnd_search.right > (int32_t)pc_slice_header->PicWidthInSamplesL) {
        wnd_search.left -= (wnd_search.right - pc_slice_header->PicWidthInSamplesL);
    }
    if (wnd_search.bottom > (int32_t)pc_slice_header->PicHeightInSamplesL) {
        wnd_search.top -= (wnd_search.bottom - pc_slice_header->PicHeightInSamplesL);
    }
    wnd_search.left = HL_MATH_CLIP3(0, (int32_t)pc_slice_header->PicWidthInSamplesL, wnd_search.left);
    wnd_search.right = HL_MATH_CLIP3(wnd_search.left, (int32_t)(pc_slice_header->PicWidthInSamplesL - u_blk_width), wnd_search.right);
    wnd_search.top = HL_MATH_CLIP3(0, (int32_t)pc_slice_header->PicHeightInSamplesL, wnd_search.top);
    wnd_search.bottom = HL_MATH_CLIP3(wnd_search.top, (int32_t)(pc_slice_header->PicHeightInSamplesL - u_blk_height), wnd_search.bottom);

    // horizontal
    i_cy = i_int_cy;
    i_cx = wnd_search.left;
    pc_SL_ref_mb = (pc_SL_ref + i_cx + (i_cy * u_SL_ref_stride));
    mvLX.y = ((i_cy - i_int_y_start) << __IPelLShift);
    i_i = (int32_t)hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/); // TODO: ae(v) for CABAC
    for (; i_cx <= wnd_search.right; i_cx += 2) {
        err = _GetDist_Motion(
                  p_mb,
                  p_codec,
                  pc_SL_ref_mb, u_SL_ref_stride, 0, 0,
                  pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                  u_SL_4x4_width_count,
                  u_SL_4x4_height_count,
                  &i_dist
              );
        if (err) {
            goto bail;
        }
        mvLX.x = ((i_cx - i_int_x_start) << __IPelLShift);
        i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + i_i); // TODO: ae(v) for CABAC
        d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
        if (d_cost < d_best_cost) {
            mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
            d_best_cost = d_cost;
            i_best_dist = i_dist;
            JVT_O079_CHECK_EARLY_TEMINATION(d_cost, JTh1, JTh2);
        }
        pc_SL_ref_mb += 2;
    }
    // vertical
    i_cy = wnd_search.top;
    i_cx = i_int_cx;
    pc_SL_ref_mb = (pc_SL_ref + i_cx + (i_cy * u_SL_ref_stride));
    mvLX.x = ((i_cx - i_int_x_start) << __IPelLShift);
    i_i = (int32_t)hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/); // TODO: ae(v) for CABAC
    for (; i_cy <= wnd_search.bottom; i_cy += 2) {
        err = _GetDist_Motion(
                  p_mb,
                  p_codec,
                  pc_SL_ref_mb, u_SL_ref_stride, 0, 0,
                  pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                  u_SL_4x4_width_count,
                  u_SL_4x4_height_count,
                  &i_dist
              );
        if (err) {
            goto bail;
        }
        mvLX.y = ((i_cy - i_int_y_start) << __IPelLShift);
        i_rbc_mv = (int32_t)(i_i + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
        d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
        if (d_cost < d_best_cost) {
            mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
            d_best_cost = d_cost;
            i_best_dist = i_dist;
            JVT_O079_CHECK_EARLY_TEMINATION(d_cost, JTh1, JTh2);
        }
        pc_SL_ref_mb += (u_SL_ref_stride << 1);
    }

    //*******************************************
    // 2.3 Uneven Multi-Hexagon-Grid Search
    //*******************************************

    // save best position "Multi-Hexagon-grid" as "full search" will override it.
    // define integer-pel center position at frame-unit
    i_int_cx = i_int_x_start + (mvBestLX.x >> __IPelRShift);
    i_int_cy = i_int_y_start + (mvBestLX.y >> __IPelRShift);

    // first a full search with search range equals 2 is carried out around the search center
    // set search window
    // FIXME: using pattern - current code not correct
    wnd_search.left = (i_int_cx - 2);
    wnd_search.right = (i_int_cx + 2);
    wnd_search.top = (i_int_cy - 2);
    wnd_search.bottom = (i_int_cy + 2);
    // clip search window window
    wnd_search.left = HL_MATH_CLIP3(0, (int32_t)pc_slice_header->PicWidthInSamplesL, wnd_search.left);
    wnd_search.right = HL_MATH_CLIP3(wnd_search.left, (int32_t)(pc_slice_header->PicWidthInSamplesL - u_blk_width), wnd_search.right);
    wnd_search.top = HL_MATH_CLIP3(0, (int32_t)pc_slice_header->PicHeightInSamplesL, wnd_search.top);
    wnd_search.bottom = HL_MATH_CLIP3(wnd_search.top, (int32_t)(pc_slice_header->PicHeightInSamplesL - u_blk_height), wnd_search.bottom);
    for (i_cx = wnd_search.left; i_cx <= wnd_search.right; ++i_cx) {
        for (i_cy = wnd_search.top; i_cy <= wnd_search.bottom; ++i_cy) {
            pc_SL_ref_mb = (pc_SL_ref + i_cx + (i_cy * pc_slice_header->PicWidthInSamplesL));
            err = _GetDist_Motion(
                      p_mb,
                      p_codec,
                      pc_SL_ref_mb, u_SL_ref_stride, 0, 0,
                      pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                      u_SL_4x4_width_count,
                      u_SL_4x4_height_count,
                      &i_dist
                  );
            if (err) {
                goto bail;
            }
            mvLX.x = ((i_cx - i_int_x_start) << __IPelLShift);
            mvLX.y = ((i_cy - i_int_y_start) << __IPelLShift);
            i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
            d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
            if (d_cost < d_best_cost) {
                mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
                d_best_cost = d_cost;
                i_best_dist = i_dist;
                JVT_O079_CHECK_EARLY_TEMINATION(d_cost, JTh1, JTh2);
            }
        }
    }

    // a Multi-Hexagon-grid search strategy is taken
    // scale from 1 to W/4
    for (i_k = 1; i_k <= i_me_range_div4; ++i_k) {
        for (i_i = 0; i_i < SixteenPointsHexagonPatternCount; ++i_i) {
            i_cx = i_int_cx + (i_k * SixteenPointsHexagonPattern[i_i][0]);
            CONTINUE_IF_ICX_INVALID();
            i_cy = i_int_cy + (i_k * SixteenPointsHexagonPattern[i_i][1]);
            CONTINUE_IF_ICY_INVALID();
            pc_SL_ref_mb = (pc_SL_ref + i_cx + (i_cy * pc_slice_header->PicWidthInSamplesL));
            err = _GetDist_Motion(
                      p_mb,
                      p_codec,
                      pc_SL_ref_mb, u_SL_ref_stride, 0, 0,
                      pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                      u_SL_4x4_width_count,
                      u_SL_4x4_height_count,
                      &i_dist
                  );
            if (err) {
                goto bail;
            }
            mvLX.x = ((i_cx - i_int_x_start) << __IPelLShift);
            mvLX.y = ((i_cy - i_int_y_start) << __IPelLShift);
            i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
            d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
            if (d_cost < d_best_cost) {
                mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
                d_best_cost = d_cost;
                i_best_dist = i_dist;
                JVT_O079_CHECK_EARLY_TEMINATION(d_cost, JTh1, JTh2);
            }
        }
    }

    //*************************************************
    // 2.4 Extended Hexagon-based Search (EHS)
    //*************************************************

    // Step_4-1
Step_41:
    i_k = 0;
    i_int_cx = i_int_x_start + (mvBestLX.x >> __IPelRShift);
    i_int_cy = i_int_y_start + (mvBestLX.y >> __IPelRShift);

    for (i_i = 0; i_i < HexagonSearchPatternCount; ++i_i) {
        i_cx = i_int_cx + HexagonSearchPattern[i_i][0];
        CONTINUE_IF_ICX_INVALID();
        i_cy = i_int_cy + HexagonSearchPattern[i_i][1];
        CONTINUE_IF_ICY_INVALID();
        pc_SL_ref_mb = (pc_SL_ref + i_cx + (i_cy * pc_slice_header->PicWidthInSamplesL));
        err = _GetDist_Motion(
                  p_mb,
                  p_codec,
                  pc_SL_ref_mb, u_SL_ref_stride, 0, 0,
                  pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                  u_SL_4x4_width_count,
                  u_SL_4x4_height_count,
                  &i_dist
              );
        if (err) {
            goto bail;
        }
        mvLX.x = ((i_cx - i_int_x_start) << __IPelLShift);
        mvLX.y = ((i_cy - i_int_y_start) << __IPelLShift);
        i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
        d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
        if (d_cost < d_best_cost) {
            mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
            d_best_cost = d_cost;
            i_best_dist = i_dist;
            ++i_k;
        }
    }
    if (i_k) {
        goto Step_41;
    }

    // Step_4-2
Step_42:
    i_k = 0;
    i_int_cx = i_int_x_start + (mvBestLX.x >> __IPelRShift);
    i_int_cy = i_int_y_start + (mvBestLX.y >> __IPelRShift);
    for (i_i = 0; i_i < DiamondSearchIntPatternCount; ++i_i) {
        i_cx = i_int_cx + DiamondSearchIntPattern[i_i][0];
        CONTINUE_IF_ICX_INVALID();
        i_cy = i_int_cy + DiamondSearchIntPattern[i_i][1];
        CONTINUE_IF_ICY_INVALID();
        pc_SL_ref_mb = (pc_SL_ref + i_cx + (i_cy * pc_slice_header->PicWidthInSamplesL));
        err = _GetDist_Motion(
                  p_mb,
                  p_codec,
                  pc_SL_ref_mb, u_SL_ref_stride, 0, 0,
                  pc_SL_enc_mb, u_SL_enc_stride, 0, 0,
                  u_SL_4x4_width_count,
                  u_SL_4x4_height_count,
                  &i_dist
              );
        if (err) {
            goto bail;
        }
        mvLX.x = ((i_cx - i_int_x_start) << __IPelLShift);
        mvLX.y = ((i_cy - i_int_y_start) << __IPelLShift);
        i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
        d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
        if (d_cost < d_best_cost) {
            mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
            d_best_cost = d_cost;
            i_best_dist = i_dist;
            ++i_k;
        }
    }
    if (i_k) {
        goto Step_42;
    }

    //**************************************************************************
    // Center biased Fractional Pel Search algorithm for fractional pel search
    //**************************************************************************
#if 0
    // CBFPS_Step_1. Initial search point prediction
    mvLX.x = ((mvpLX->x << 2) - mvBestLX.x) & 3; // FIXME: "<<2" for pred?
    mvLX.y = ((mvpLX->y << 2) - mvBestLX.y) & 3; // FIXME: "<<2" for pred?
    err = _UMHexagonS_GetCostAndDist(
              p_mb,
              p_codec,
              mbPartIdx,
              subMbPartIdx,
              &mvLX,
              pc_SL_ref,
              pc_SL_enc_mb, u_SL_enc_stride,
              u_SL_4x4_width_count, u_SL_4x4_height_count,
              tmpPredPartLXL, tmpPredPartLXL_u8,
              &i_rbc,
              &i_dist);
    if (err) {
        goto bail;
    }
    i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
    d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * (i_rbc_mv + i_rbc));
    if (d_cost < d_cost00) {
        i_cx = mvLX.x, i_cy = mvLX.y;
    }
    else {
        i_cx = 0, i_cy = 0;
    }
#else
    i_cx = mvBestLX.x;
    i_cy = mvBestLX.y;
#endif

    i_guard = 0;
CBFPS_Step_2:
    i_k = 0;
    for (i_i = 0; i_i < DiamondSearchFractPatternCount; ++i_i) {
        mvLX.x = i_cx + DiamondSearchFractPattern[i_i][0];
        mvLX.y = i_cy + DiamondSearchFractPattern[i_i][1];
        // FIXME: use SATD for Fracts
        err = _GetDist2_Motion(
                  p_mb,
                  p_codec,
                  mbPartIdx,
                  subMbPartIdx,
                  &mvLX,
                  pc_SL_ref,
                  pc_SL_enc_mb, u_SL_enc_stride,
                  u_SL_4x4_width_count, u_SL_4x4_height_count,
                  tmpPredPartLXL_u8,
                  &i_dist);
        if (err) {
            goto bail;
        }
        i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
        d_cost = i_dist + (p_codec->encoder.rdo.d_lambda_motion * i_rbc_mv);
        if (d_cost < d_best_cost) {
            mvBestLX.x = mvLX.x, mvBestLX.y = mvLX.y;
            d_best_cost = d_cost;
            i_best_dist = i_dist;
            ++i_k;
        }
    }
    if (i_k) {
        i_cx = mvBestLX.x;
        i_cy = mvBestLX.y;
        if (i_guard++ < __GuardMaxVal) {
            goto CBFPS_Step_2;
        }
    }

    /* End of Motion Estimation */

    // compute Mode cost
    err = _GetRbcAndDist2_Mode(
              p_mb,
              p_codec,
              mbPartIdx,
              subMbPartIdx,
              &mvBestLX,
              pc_SL_ref,
              pc_SL_enc_mb, u_SL_enc_stride,
              u_SL_4x4_width_count, u_SL_4x4_height_count,
              tmpPredPartLXL_u8,
              &Single_ctr,
              &i_rbc,
              &i_dist);
    if (err) {
        return err;
    }
    i_rbc_mv = (int32_t)(hl_codec_264_bits_count_bits_se((mvBestLX.x - mvpLX->x)/*MVD.x*/) + hl_codec_264_bits_count_bits_se((mvBestLX.y - mvpLX->y)/*MVD.y*/)); // TODO: ae(v) for CABAC
    d_best_cost = i_dist + (p_codec->encoder.rdo.d_lambda_mode * (i_rbc_mv + i_rbc));
    i_best_dist = i_dist;
    i_best_Single_ctr = Single_ctr;

//done:
    // set ret values
    best_pos[0] = mvBestLX.x, best_pos[1] = mvBestLX.y;
    *pd_best_cost = d_best_cost;
    *pi_best_dist = i_best_dist;
    *pi_Single_ctr = i_best_Single_ctr;

bail:
    return err;
}