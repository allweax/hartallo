#include "hartallo/h264/hl_codec_264_pred_inter.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_transf.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_interpol.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_asynctask.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

static HL_SHOULD_INLINE HL_ERROR_T _hl_codec_264_pred_inter_transf_decode_luma4x4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_IN_ALIGNED(16) const int32_t predL[16][16]);

static HL_SHOULD_INLINE HL_ERROR_T _hl_codec_264_pred_inter_transf_decode_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_IN_ALIGNED(16) const int32_t predCb[16][16],
    HL_IN_ALIGNED(16) const int32_t predCr[16][16]);

HL_ERROR_T hl_codec_264_interpol_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvLX,
    const hl_pixel_t* cSL,
    HL_OUT_ALIGNED(16) void* predPartLXL16x16,
    int32_t predPartLXLSampleSize);

HL_ERROR_T hl_codec_264_interpol_chroma_cpp(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvCLX,
    const hl_codec_264_mv_xt* mvLX, // used when "ChromaArrayType"=3
    const hl_codec_264_pict_t* refPicLXCb,
    const hl_codec_264_pict_t* refPicLXCr,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);

static int32_t _hl_codec_264_pred_inter_chroma_interpolate(
    const hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t xIntC, int32_t yIntC,
    int32_t xFracC, int32_t yFracC,
    hl_bool_t Cb,
    const hl_codec_264_pict_t* refPicLXC);

// 8.4 Inter prediction process
HL_ERROR_T hl_codec_264_pred_inter_decode(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    hl_bool_t b_compute_mvs_and_refs)
{
    int32_t mbPartsCount = (p_mb->e_type == HL_CODEC_264_MB_TYPE_B_SKIP || p_mb->e_type == HL_CODEC_264_MB_TYPE_B_DIRECT_16X16) ? 4 : p_mb->NumMbPart;
    int32_t mbPartIdx, subMbPartIdx, subPartsCount, MvCnt = 0;
    hl_int32_16x16_t *predL, *predCb, *predCr, *predPartL, *predPartCb, *predPartCr;
    hl_memory_blocks_t* pc_mem_blocks;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    void (*_hl_memory_copy4x4)(int32_t *p_dst, hl_size_t dst_stride, const int32_t *pc_src, hl_size_t src_stride);

    static const int32_t luma4x4BlkIdxList[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    static const hl_size_t luma4x4BlkIdxListCount = sizeof(luma4x4BlkIdxList) / sizeof(luma4x4BlkIdxList[0]);

    int32_t xP, yP, xS, yS, x, y, _x, _y;

    const hl_codec_264_pict_t* refPicLXL;
    const hl_codec_264_pict_t* refPicLXCb;
    const hl_codec_264_pict_t* refPicLXCr;

    static hl_bool_t predLIs4x4SamplesOnlyFalse = HL_FALSE;

    if (p_mb->u_addr == 48) {
        int a = 0;
    }

    pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

    // map() memory
    hl_memory_blocks_map(pc_mem_blocks, &predL);
    hl_memory_blocks_map(pc_mem_blocks, &predCb);
    hl_memory_blocks_map(pc_mem_blocks, &predCr);
    hl_memory_blocks_map(pc_mem_blocks, &predPartL);
    hl_memory_blocks_map(pc_mem_blocks, &predPartCb);
    hl_memory_blocks_map(pc_mem_blocks, &predPartCr);

    if (mbPartsCount > 4) { // Not baseline
        HL_DEBUG_ERROR("No implemented yet");
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    }

    for (mbPartIdx = 0; mbPartIdx<mbPartsCount; ++mbPartIdx) {
        subPartsCount = p_mb->NumSubMbPart[mbPartIdx];//FIXME: is it right?
        subMbPartIdx = 0;

        do {
            // "b_compute_mvs_and_refs" = "false" -> multi-threaded(computation done by the caller), "true" -> single-threaded
            if (b_compute_mvs_and_refs) {
                // 8.4.1 Derivation process for motion vector components and reference indices
                err = hl_codec_264_utils_derivation_process_for_movect_comps_and_ref_indices(p_codec,p_mb, mbPartIdx, subMbPartIdx,
                        &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx],
                        &p_mb->refIdxL0[mbPartIdx], &p_mb->refIdxL1[mbPartIdx],
                        &p_mb->predFlagL0[mbPartIdx], &p_mb->predFlagL1[mbPartIdx],
                        &p_mb->subMvCnt);
                if (p_codec->pps.pc_active->weighted_pred_flag == 1) {
                    //if(p_codec->layers.pc_active->pc_slice_hdr->slice_type % 3 == 0 || p_codec->layers.pc_active->pc_slice_hdr->slice_type % 3 == 3)
                    // ....
                    HL_DEBUG_ERROR("Not implemented yet");
                    err = HL_ERROR_NOT_IMPLEMENTED;
                    goto bail;
                }

                p_mb->MvL0[mbPartIdx][subMbPartIdx] = p_mb->mvL0[mbPartIdx][subMbPartIdx]; //(8-162)
                p_mb->MvL1[mbPartIdx][subMbPartIdx] = p_mb->mvL1[mbPartIdx][subMbPartIdx]; //(8-163)
                p_mb->MvCL0[mbPartIdx][subMbPartIdx] = p_mb->mvCL0[mbPartIdx][subMbPartIdx];
                p_mb->MvCL1[mbPartIdx][subMbPartIdx] = p_mb->mvCL1[mbPartIdx][subMbPartIdx];
                p_mb->RefIdxL0[mbPartIdx] = p_mb->refIdxL0[mbPartIdx]; //(8-164)
                p_mb->RefIdxL1[mbPartIdx] = p_mb->refIdxL1[mbPartIdx]; //(8-165)
                p_mb->PredFlagL0[mbPartIdx] = p_mb->predFlagL0[mbPartIdx]; //(8-166)
                p_mb->PredFlagL1[mbPartIdx] = p_mb->predFlagL1[mbPartIdx]; //(8-167)
            } //end-of-if (b_compute_mvs_and_refs)

            MvCnt += p_mb->subMvCnt;

            if (mbPartIdx == 0 && subMbPartIdx == 0) {
                xP = yP = xS = yS = 0;
                p_mb->xL_Idx = p_mb->xL;
                p_mb->yL_Idx = p_mb->yL;
            }
            else {
                // 6.4.2.1 - upper-left sample of the macroblock partition
                xP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);// (6-11)
                yP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);// (6-12)
                // 6.4.2.2 - upper-left sample of the sub-macroblock partition
                hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);

                // compute "xL_Idx" and "yL_Idx", used by interpolation functions
                p_mb->xL_Idx = p_mb->xL + (xP + xS);
                p_mb->yL_Idx = p_mb->yL + (yP + yS);
            }

            // Get reference pictures for the partition (ignore sub-parts)
            if (subMbPartIdx == 0) {
                // 8.4.2.1 Reference picture selection process
                err = hl_codec_264_pred_inter_select_refpic(p_codec, p_mb,
                        p_mb->predFlagL1[mbPartIdx] == 1 ? p_codec->pc_poc->RefPicList1 : p_codec->pc_poc->RefPicList0,
                        p_mb->predFlagL1[mbPartIdx] == 1 ? p_mb->refIdxL1[mbPartIdx] : p_mb->refIdxL0[mbPartIdx],
                        &refPicLXL, &refPicLXCb, &refPicLXCr);
                if (err) {
                    HL_DEBUG_ERROR("hl_codec_264_pred_inter_select_refpic() failed");
                    break;
                }
            }

            // 8.4.2 Decoding process for Inter prediction samples
            if(p_mb->predFlagL0[mbPartIdx] == 1) {
                // 8.4.2.2 Fractional sample interpolation process
                err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                      &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx],
                                                      refPicLXL, refPicLXCb, refPicLXCr,
                                                      (*predPartL), (*predPartCb), (*predPartCr));
                if (err) {
                    goto bail;
                }
            }
            else if(p_mb->predFlagL1[mbPartIdx] == 1) {
                // 8.4.2.2 Fractional sample interpolation process
                err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                      &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx],
                                                      refPicLXL, refPicLXCb, refPicLXCr,
                                                      (*predPartL), (*predPartCb), (*predPartCr));
                if (err) {
                    goto bail;
                }
            }

            // Copy Luma Prediction samples
            _x = xP + xS;
            _y = yP + yS;
            _hl_memory_copy4x4 = (hl_memory_is_position_aligned(_x) && hl_memory_is_position_aligned(_y)) ? hl_memory_copy4x4 : hl_memory_copy4x4_unaligned;
            for (y = 0; y<p_mb->partHeight[mbPartIdx][subMbPartIdx]; y+=4) {
                // (8-168)
                for (x = 0; x < p_mb->partWidth[mbPartIdx][subMbPartIdx]; x+=4) {
                    _hl_memory_copy4x4(&(*predL)[_y + y][_x + x], 16, &(*predPartL)[y][x], 16);
                }
            }

            // Copy Chroma Prediction samples (not always aligned on 16 bytes)
            _x = (xP >> p_codec->sps.pc_active->SubWidthC_TrailingZeros) + (xS >> p_codec->sps.pc_active->SubWidthC_TrailingZeros);
            _y = (yP >> p_codec->sps.pc_active->SubHeightC_TrailingZeros) + (yS >> p_codec->sps.pc_active->SubHeightC_TrailingZeros);
            _hl_memory_copy4x4 = (hl_memory_is_position_aligned(_x) && hl_memory_is_position_aligned(_y)) ? hl_memory_copy4x4 : hl_memory_copy4x4_unaligned;
            for (y = 0; y<p_mb->partHeightC[mbPartIdx][subMbPartIdx]; y+=4) {
                // (8-169) , (8-169)
                for (x = 0; x < p_mb->partWidthC[mbPartIdx][subMbPartIdx]; x+=4) {
                    _hl_memory_copy4x4(&(*predCb)[_y + y][_x + x], 16, &(*predPartCb)[y][x], 16);
                    _hl_memory_copy4x4(&(*predCr)[_y + y][_x + x], 16, &(*predPartCr)[y][x], 16);
                }
            }
        }
        while (++subMbPartIdx < subPartsCount);
    }

    // 8.5.1 Specification of transform decoding process for 4x4 luma residual blocks
    err = hl_codec_264_transf_decode_luma4x4(p_codec, p_mb, luma4x4BlkIdxList, luma4x4BlkIdxListCount, (const int32_t*)(*predL), 16, predLIs4x4SamplesOnlyFalse);
    // 8.5.4 Specification of transform decoding process for chroma samples
    err = hl_codec_264_transf_decode_chroma(p_codec, p_mb, (*predCb), 0);
    err = hl_codec_264_transf_decode_chroma(p_codec, p_mb, (*predCr), 1);

bail:
    // unmap() memory
    hl_memory_blocks_unmap(pc_mem_blocks, predL);
    hl_memory_blocks_unmap(pc_mem_blocks, predCb);
    hl_memory_blocks_unmap(pc_mem_blocks, predCr);
    hl_memory_blocks_unmap(pc_mem_blocks, predPartL);
    hl_memory_blocks_unmap(pc_mem_blocks, predPartCb);
    hl_memory_blocks_unmap(pc_mem_blocks, predPartCr);

    return err;
}

// 8.5.4 Specification of transform decoding process for chroma samples
static HL_SHOULD_INLINE HL_ERROR_T _hl_codec_264_pred_inter_transf_decode_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_IN_ALIGNED(16) const int32_t predCb[16][16],
    HL_IN_ALIGNED(16) const int32_t predCr[16][16])
{
    HL_ERROR_T err;

    // 8.5.4 Specification of transform decoding process for chroma samples
    err = hl_codec_264_transf_decode_chroma(p_codec, p_mb, predCb, 0);
    err = hl_codec_264_transf_decode_chroma(p_codec, p_mb, predCr, 1);
    return err;
}

// 8.4.2.1 Reference picture selection process
HL_ERROR_T hl_codec_264_pred_inter_select_refpic(
    const hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    struct hl_codec_264_dpb_fs_s** RefPicListX,
    int32_t refIdxLX,
    const hl_codec_264_pict_t** refPicLXL,
    const hl_codec_264_pict_t** refPicLXCb,
    const hl_codec_264_pict_t** refPicLXCr)
{
    const hl_codec_264_pict_t* refPic = HL_NULL;

    if (refIdxLX < 0) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    if (p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag == 0) {
        if(p_mb->mb_field_decoding_flag == 0) {
            refPic = RefPicListX[refIdxLX] ? RefPicListX[refIdxLX]->p_pict : HL_NULL;
        }
        else {
            HL_DEBUG_ERROR("Not implemented yet");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
    }
    else {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    if (!refPic || !refPic) {
        HL_DEBUG_ERROR("refPic at index (%d) is not valid", refIdxLX);
        return HL_ERROR_NOT_FOUND;
    }

    if (p_codec->sps.pc_active->separate_colour_plane_flag == 0) {
        *refPicLXL = refPic;
        *refPicLXCb = refPic;
        *refPicLXCr = refPic;
        //printf("refIdxLX adrr=%d\n", refIdxLX);
    }
    else {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return HL_ERROR_SUCCESS;
}

// 8.4.2.2 Fractional sample interpolation process
HL_ERROR_T hl_codec_264_pred_inter_predict(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvLX,
    const hl_codec_264_mv_xt* mvCLX,
    const hl_codec_264_pict_t* refPicLXL,
    const hl_codec_264_pict_t* refPicLXCb,
    const hl_codec_264_pict_t* refPicLXCr,
    HL_OUT int32_t predPartLXL[16][16],
    HL_OUT int32_t predPartLXCb[16][16],
    HL_OUT int32_t predPartLXCr[16][16])
{
    HL_ERROR_T err;

    // 8.4.2.2 Fractional sample interpolation process (Luma only)
    err = hl_codec_264_interpol_luma(p_codec, p_mb,
                                     mbPartIdx, subMbPartIdx,
                                     mvLX,
                                     refPicLXL->pc_data_y,
                                     predPartLXL, sizeof(predPartLXCr[0][0]));
    if (err) {
        return err;
    }
    // 8.4.2.2 Fractional sample interpolation process (Chroma only)
    err = hl_codec_264_interpol_chroma(p_codec, p_mb,
                                       mbPartIdx, subMbPartIdx,
                                       mvCLX, mvLX,
                                       refPicLXCb, refPicLXCr,
                                       predPartLXCb, predPartLXCr);

    return err;
}

// FIXME: remove
#include "hartallo/intrinsics/x86/hl_memory_x86_intrin.h"

// 8.4.2.2 Fractional sample interpolation process (Luma only)
HL_ERROR_T hl_codec_264_interpol_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvLX,
    const hl_pixel_t* cSL,
    HL_OUT_ALIGNED(16) void* predPartLXL16x16,
    int32_t predPartLXLSampleSize)

{
    int32_t xL,yL,xFracL,yFracL;
    hl_codec_264_interpol_indices_t* pc_interpol_indices = p_codec->pc_dpb->p_list_interpol_indices[(p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag == 0 || p_mb->mb_field_decoding_flag == 0) ? 0 : 1];
    hl_bool_t b_use_fast_8bits_version, b_out_is_8bits;
    int32_t _X, _Y, i_width, i_height,_xIntL, _yIntL, i_blk4x4x8bits_idx;
    int32_t i_indices_stride = pc_interpol_indices->u_stride;
    int32_t i_indices_stride_mul_p4 = (i_indices_stride << 2);
    int32_t partHeight = p_mb->partHeight[mbPartIdx][subMbPartIdx];
    int32_t partWidth = p_mb->partWidth[mbPartIdx][subMbPartIdx];
    hl_memory_blocks_t* pc_mem_blocks = HL_NULL;
    hl_uint8_16x16_t* predPartLXL4x4x16_u8 = HL_NULL;

    // "predPartLXLSampleSize" is used to control samples size for "predPartLXL16x16"
    if (predPartLXLSampleSize != sizeof(int32_t) && predPartLXLSampleSize != sizeof(uint8_t)) {
        HL_DEBUG_ERROR("predPartLXLSampleSize(%d) is invalid", predPartLXLSampleSize);
        return HL_ERROR_INVALID_PARAMETER;
    }

    i_blk4x4x8bits_idx = 0;
    i_width = pc_interpol_indices->u_width;
    i_height = pc_interpol_indices->u_height;

    b_out_is_8bits = predPartLXLSampleSize == sizeof(uint8_t);
    b_use_fast_8bits_version =
        (p_codec->layers.pc_active->p_list_slices[p_mb->u_slice_idx]->p_header->pc_pps->pc_sps->BitDepthY == 8
         && sizeof(hl_pixel_t) == sizeof(uint8_t));

    // map() memory
    if (b_use_fast_8bits_version) {
        pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);
        hl_memory_blocks_map(pc_mem_blocks, &predPartLXL4x4x16_u8);
    }

#define TEST_8BITS 0 // FIXME

    xFracL = mvLX->x & 3;// (8-227)
    yFracL = mvLX->y & 3;// (8-228)

    _yIntL = p_mb->yL_Idx + (mvLX->y >> 2); // (8-226)
    _xIntL = p_mb->xL_Idx + (mvLX->x >> 2); // (8-225)

    // Compute base origin and clip within the padded area.
    // -17 is high enough to be sure that for any N position:
    //	- If N is outside but close (e.g. -2) incrementing yL and xL will move the cursor into the image.
    //	- If N is outside but far (e.g. -200) incrementing yL and xL will not move the cursor into the image.
    //  - Regardless N, the number of steps to enter into the image are the same even if not clipped.
    _X = HL_MATH_CLIP3(-17, i_width + 17, _xIntL);
    _Y = HL_MATH_CLIP3(-17, i_height + 17, _yIntL);

    // (Start) 8.4.2.2.1 Luma sample interpolation process

    // Table 8-12 – Assignment of the luma prediction sample predPartLXL[ xL, yL ]
    switch (xFracL) {
    case 0: {
        switch(yFracL) {
        case 0: {
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + _X);
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma00_horiz4x4_u8(&pc_indices_horiz[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_horiz += i_indices_stride_mul_p4;
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma00_horiz4(&pc_indices_horiz[xL], cSL, &(*predPartLXL)[yL][xL]);
                    }
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 1: {
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma01_vert4x4_u8(&pc_indices_vert[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_vert += i_indices_stride_mul_p4;
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma01_vert4(&pc_indices_vert[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_vert += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 2: {
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma02_vert4x4_u8(&pc_indices_vert[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_vert += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma02_vert4(&pc_indices_vert[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_vert += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 3: {
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma03_vert4x4_u8(&pc_indices_vert[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_vert += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma03_vert4(&pc_indices_vert[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_vert += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        }
        break;
    }
    case 1: {
        switch(yFracL) {
        case 0: {
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma10_horiz4x4_u8(&pc_indices_horiz[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma10_horiz4(&pc_indices_horiz[xL], cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 1: {
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma11_diag4x4_u8(&pc_indices_vert[xL], &pc_indices_horiz[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_vert += (i_indices_stride_mul_p4);
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma11_diag4(&pc_indices_vert[xL], &pc_indices_horiz[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_vert += pc_interpol_indices->u_stride;
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 2: {
            uint32_t* pu_vert_start = pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X;
            const uint32_t* pc_indices_vert[6];
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma12_vert4x4_u8(pc_indices_vert, i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pu_vert_start += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma12_vert4(pc_indices_vert, i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pu_vert_start += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 3: {
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + ((_Y + 1) * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma13_diag4x4_u8(&pc_indices_vert[xL], &pc_indices_horiz[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_vert += (i_indices_stride_mul_p4);
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma13_diag4(&pc_indices_vert[xL], &pc_indices_horiz[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_vert += pc_interpol_indices->u_stride;
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        }
        break;
    }
    case 2: {
        switch(yFracL) {
        case 0: {
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma20_horiz4x4_u8(&pc_indices_horiz[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma20_horiz4(&pc_indices_horiz[xL], cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 1: {
            uint32_t* pu_vert_start = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            const uint32_t* pc_indices_vert[6];
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma21_diag4x4_u8(&pc_indices_horiz[xL], pc_indices_vert, i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pu_vert_start += i_indices_stride_mul_p4;
                    pc_indices_horiz += i_indices_stride_mul_p4;
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma21_diag4(&pc_indices_horiz[xL], pc_indices_vert, i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pu_vert_start += pc_interpol_indices->u_stride;
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 2: {
            uint32_t* pu_vert_start = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X);
            const uint32_t* pc_indices_vert[6];
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma22_vert4x4_u8(pc_indices_vert, i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pu_vert_start += i_indices_stride_mul_p4;
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma22_vert4(pc_indices_vert, i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pu_vert_start += pc_interpol_indices->u_stride;
                }
            }

            break;
        }
        case 3: {
            const uint32_t* pu_vert_start = pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X;
            const uint32_t* pc_indices_vert[6];
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + ((_Y + 1) * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pc_indices_vert[0] - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma23_diag4x4_u8(&pc_indices_horiz[xL], pc_indices_vert, i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pu_vert_start += i_indices_stride_mul_p4;
                    pc_indices_horiz += i_indices_stride_mul_p4;
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pc_indices_vert[0] - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        hl_codec_264_interpol_luma23_diag4(&pc_indices_horiz[xL], pc_indices_vert, i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pu_vert_start += pc_interpol_indices->u_stride;
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }

            break;
        }
        }
        break;
    }
    case 3: {
        switch(yFracL) {
        case 0: {
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + (_X - 2));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma30_horiz4x4_u8(&pc_indices_horiz[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma30_horiz4(&pc_indices_horiz[xL], cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 1: {
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + (_Y * i_indices_stride) + (_X - 2));
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + (_X + 1));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma31_diag4x4_u8(&pc_indices_horiz[xL], &pc_indices_vert[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                    pc_indices_vert += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma31_diag4(&pc_indices_horiz[xL], &pc_indices_vert[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                    pc_indices_vert += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 2: {
            uint32_t* pu_vert_start = pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + _X;
            const uint32_t* pc_indices_vert[7];
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        pc_indices_vert[6] = pu_vert_start + xL + 1;
                        hl_codec_264_interpol_luma32_vert4x4_u8(pc_indices_vert, i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pu_vert_start += i_indices_stride_mul_p4;
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        pc_indices_vert[0] = pu_vert_start + xL;
                        pc_indices_vert[1] = pu_vert_start + xL - 2;
                        pc_indices_vert[2] = pu_vert_start + xL - 1;
                        pc_indices_vert[3] = pu_vert_start + xL + 1;
                        pc_indices_vert[4] = pu_vert_start + xL + 2;
                        pc_indices_vert[5] = pu_vert_start + xL + 3;
                        pc_indices_vert[6] = pu_vert_start + xL + 1;
                        hl_codec_264_interpol_luma32_vert4(pc_indices_vert, i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pu_vert_start += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        case 3: {
            const uint32_t* pc_indices_horiz = (pc_interpol_indices->pc_indices + ((_Y + 1) * i_indices_stride) + (_X - 2));
            const uint32_t* pc_indices_vert = (pc_interpol_indices->pc_indices + ((_Y - 2) * i_indices_stride) + (_X + 1));
            if (b_use_fast_8bits_version) {
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma33_diag4x4_u8(&pc_indices_horiz[xL], &pc_indices_vert[xL], i_indices_stride, cSL, (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                    pc_indices_horiz += (i_indices_stride_mul_p4);
                    pc_indices_vert += (i_indices_stride_mul_p4);
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; ++yL) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_codec_264_interpol_luma33_diag4(&pc_indices_horiz[xL], &pc_indices_vert[xL], i_indices_stride, cSL, &(*predPartLXL)[yL][xL], p_codec->PixelMaxValueY);
                    }
                    pc_indices_horiz += pc_interpol_indices->u_stride;
                    pc_indices_vert += pc_interpol_indices->u_stride;
                }
            }
            break;
        }
        }
        break;
    }
    }
    // (End) 8.4.2.2.1 Luma sample interpolation process

    // Resize samples (8bits -> 32bits) and
    if (b_use_fast_8bits_version) {
        i_blk4x4x8bits_idx = 0;
        if (partHeight == 16 && partWidth == 16) {
            if (b_out_is_8bits) {
                hl_memory_copy16x16_u8_stride16x4((uint8_t*)predPartLXL16x16, &(*predPartLXL4x4x16_u8)[0][0]);
            }
            else {
                hl_memory_copy16x16_u8_to_u32_stride16x4((uint32_t*)predPartLXL16x16, &(*predPartLXL4x4x16_u8)[0][0]);
            }
        }
        else {
            if (b_out_is_8bits) {
                uint8_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_memory_copy4x4_u8_stride16x4(&(*predPartLXL)[yL][xL], (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                }
            }
            else {
                int32_t (*predPartLXL)[16][16] = predPartLXL16x16;
                for (yL=0; yL<partHeight; yL+=4) {
                    for (xL=0; xL<partWidth; xL+=4) {
                        hl_memory_copy4x4_u8_to_u32_stride16x4(&(*predPartLXL)[yL][xL], (*predPartLXL4x4x16_u8)[i_blk4x4x8bits_idx++]);
                    }
                }
            }
        }
    }

    // unmap() memory
    if (predPartLXL4x4x16_u8) {
        hl_memory_blocks_unmap(pc_mem_blocks, predPartLXL4x4x16_u8);
    }

    return HL_ERROR_SUCCESS;
}

// 8.4.2.2 Fractional sample interpolation process (Chroma only)
HL_ERROR_T hl_codec_264_interpol_chroma_cpp(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const hl_codec_264_mv_xt* mvCLX,
    const hl_codec_264_mv_xt* mvLX, // used when "ChromaArrayType"=3
    const hl_codec_264_pict_t* refPicLXCb,
    const hl_codec_264_pict_t* refPicLXCr,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16])

{
    int32_t refPicHeightEffectiveC, refPicWidthEffectiveC;
    const hl_codec_264_nal_slice_header_t* pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps;

    const hl_pixel_t* cSb = refPicLXCb->pc_data_u;
    const hl_pixel_t* cSr = refPicLXCb->pc_data_v;
    int32_t chroma_width = refPicLXCr->uWidthC;
    int32_t partWidthC = p_mb->partWidthC[mbPartIdx][subMbPartIdx];
    int32_t partHeightC = p_mb->partHeightC[mbPartIdx][subMbPartIdx];
    hl_bool_t b_use_fast_8bits_version;


    pc_slice_hdr = p_codec->layers.pc_active->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_sps = pc_slice_hdr->pc_pps->pc_sps;

    refPicWidthEffectiveC = pc_slice_hdr->PicWidthInSamplesC;
    if(pc_slice_hdr->MbaffFrameFlag == 0 || p_mb->mb_field_decoding_flag == 0) {
        refPicHeightEffectiveC = pc_slice_hdr->PicHeightInSamplesC;
    }
    else {
        refPicHeightEffectiveC = pc_slice_hdr->PicHeightInSamplesC >> 1;
    }

    b_use_fast_8bits_version = (pc_sps->BitDepthY == 8 && sizeof(hl_pixel_t) == 1);

    if (pc_sps->ChromaArrayType == 1) {
        int32_t _xIntC, _yIntC, xFracC, yFracC;

        _xIntC = (p_mb->xL_Idx >> pc_sps->SubWidthC_TrailingZeros) + (mvCLX->x >> 3);
        _yIntC = (p_mb->yL_Idx >> pc_sps->SubHeightC_TrailingZeros) + (mvCLX->y >> 3);
        xFracC = mvCLX->x & 7; // (8-231)
        yFracC = mvCLX->y & 7; // (8-232)

        if (b_use_fast_8bits_version) {
            hl_codec_264_interpol_chroma_cat1_u8(
                p_codec, cSb, cSr,
                xFracC, yFracC,
                _xIntC, _yIntC, partWidthC, partHeightC, refPicHeightEffectiveC, chroma_width,
                predPartLXCb, predPartLXCr);
        }
        else {
            int32_t _xFracC_per_yFracC, _8minus_xFracC, _8minus_yFracC, _8minus_xFracC_per_8minus_yFracC, _8minus_xFracC_per_yFracC, _8minus_yFracC_per_xFracC;
            int32_t maxX, maxY;
            int32_t xC, yC, xIntC, yIntC, xAC, xBC, xCC, xDC, yAC, yBC, yCC, yDC;

            maxX = refPicWidthEffectiveC - 1;
            maxY = refPicHeightEffectiveC - 1;

            _xFracC_per_yFracC = __xFracC_per_yFracC[yFracC][xFracC];
            _8minus_xFracC = __8minus_xFracC[yFracC][xFracC];
            _8minus_yFracC = __8minus_yFracC[yFracC][xFracC];
            _8minus_xFracC_per_8minus_yFracC = __8minus_xFracC_per_8minus_yFracC[yFracC][xFracC];
            _8minus_xFracC_per_yFracC = __8minus_xFracC_per_yFracC[yFracC][xFracC];
            _8minus_yFracC_per_xFracC = __8minus_yFracC_per_xFracC[yFracC][xFracC];

            for (yC = 0; yC < partHeightC; ++yC) {
                int32_t Cb, Cr; // FIXME: remove
                yIntC = _yIntC + yC; // (8-230)
                yAC = HL_MATH_CLIP3(0, maxY, yIntC) * chroma_width; // (8-268)
                yCC = HL_MATH_CLIP3(0, maxY, yIntC + 1) * chroma_width; // (8-270)
                yDC = yCC; // (8-271)
                yBC = yAC; // (8-269)
                for (xC = 0; xC < partWidthC; ++xC) {
                    xIntC = _xIntC + xC; // (8-229)

                    xAC = HL_MATH_CLIP3(0, maxX, xIntC); // (8-264)
                    xBC = HL_MATH_CLIP3(0, maxX, xIntC + 1); // (8-265)
                    xCC = xAC; // (8-266)
                    xDC = xBC; // (8-267)

                    // 8.4.2.2.2 Chroma sample interpolation process
                    Cb = (_8minus_xFracC_per_8minus_yFracC * cSb[xAC + yAC] + _8minus_yFracC_per_xFracC * cSb[xBC + yBC] +
                          _8minus_xFracC_per_yFracC * cSb[xCC + yCC] + _xFracC_per_yFracC * cSb[xDC + yDC] + 32) >> 6;
                    Cr = (_8minus_xFracC_per_8minus_yFracC * cSr[xAC + yAC] + _8minus_yFracC_per_xFracC * cSr[xBC + yBC] +
                          _8minus_xFracC_per_yFracC * cSr[xCC + yCC] + _xFracC_per_yFracC * cSr[xDC + yDC] + 32) >> 6;
                    if (predPartLXCb[yC][xC] != Cb ||  predPartLXCr[yC][xC] != Cr) {
                        int a = 0;
                        ++a;
                        // FIXME
                    }
                    predPartLXCb[yC][xC] = Cb;
                    predPartLXCr[yC][xC] = Cr;
                }
            }
        }
    }
    else if (pc_sps->ChromaArrayType == 2) {
        // TODO: add ASM and INTRIN versions once support for CAT2 is added and tested
        int32_t maxX, maxY;
        int32_t xAC, xBC, xCC, xDC, yAC, yBC, yCC, yDC;
        int32_t xIntC, yIntC, xC, yC, xFracC, yFracC;
        int32_t _xFracC_per_yFracC, _8minus_xFracC, _8minus_yFracC, _8minus_xFracC_per_8minus_yFracC, _8minus_xFracC_per_yFracC, _8minus_yFracC_per_xFracC;

        xFracC = mvCLX->x & 7; // (8-235)
        yFracC = (mvCLX->y & 3) << 1; // (8-236)

        maxX = refPicWidthEffectiveC - 1;
        maxY = refPicHeightEffectiveC - 1;

        _xFracC_per_yFracC = __xFracC_per_yFracC[yFracC][xFracC];
        _8minus_xFracC = __8minus_xFracC[yFracC][xFracC];
        _8minus_yFracC = __8minus_yFracC[yFracC][xFracC];
        _8minus_xFracC_per_8minus_yFracC = __8minus_xFracC_per_8minus_yFracC[yFracC][xFracC];
        _8minus_xFracC_per_yFracC = __8minus_xFracC_per_yFracC[yFracC][xFracC];
        _8minus_yFracC_per_xFracC = __8minus_yFracC_per_xFracC[yFracC][xFracC];

        for(yC=0; yC<partHeightC; ++yC) {
            yIntC = (p_mb->yL_Idx >> pc_sps->SubHeightC_TrailingZeros) + (mvCLX->y >> 2) + yC; // (8-234)
            yAC = HL_MATH_CLIP3(0, refPicHeightEffectiveC - 1, yIntC) * chroma_width; // (8-268)
            yBC = yAC; // (8-269)
            yCC = HL_MATH_CLIP3(0, refPicHeightEffectiveC - 1, yIntC + 1) * chroma_width; // (8-270)
            yDC = yCC; // (8-271)
            for(xC=0; xC<partWidthC; ++xC) {
                xIntC = (p_mb->xL_Idx >> pc_sps->SubWidthC_TrailingZeros) + (mvCLX->x >> 3) + xC; // (8-233)

                xAC = HL_MATH_CLIP3(0, refPicWidthEffectiveC - 1, xIntC); // (8-264)
                xBC = HL_MATH_CLIP3(0, refPicWidthEffectiveC - 1, xIntC + 1); // (8-265)
                xCC = xAC; // (8-266)
                xDC = xBC; // (8-267)

                // 8.4.2.2.2 Chroma sample interpolation process
                predPartLXCb[yC][xC] = (_8minus_xFracC_per_8minus_yFracC * cSb[xAC + yAC] + _8minus_yFracC_per_xFracC * cSb[xBC + yBC] +
                                        _8minus_xFracC_per_yFracC * cSb[xCC + yCC] + _xFracC_per_yFracC * cSb[xDC + yDC] + 32) >> 6;
                predPartLXCr[yC][xC] = (_8minus_xFracC_per_8minus_yFracC * cSr[xAC + yAC] + _8minus_yFracC_per_xFracC * cSr[xBC + yBC] +
                                        _8minus_xFracC_per_yFracC * cSr[xCC + yCC] + _xFracC_per_yFracC * cSr[xDC + yDC] + 32) >> 6;
            }
        }
    }
    else if (pc_sps->ChromaArrayType == 3) {
        hl_codec_264_interpol_luma(p_codec, p_mb,
                                   mbPartIdx,subMbPartIdx,
                                   mvLX,
                                   refPicLXCb->pc_data_u, predPartLXCb, sizeof(predPartLXCb[0][0]));
        hl_codec_264_interpol_luma(p_codec, p_mb,
                                   mbPartIdx,subMbPartIdx,
                                   mvLX,
                                   refPicLXCr->pc_data_v, predPartLXCr, sizeof(predPartLXCr[0][0]));
    }

    return HL_ERROR_SUCCESS;
}


