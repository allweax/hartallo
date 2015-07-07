#include "hartallo/h264/hl_codec_264_rdo.h"
#include "hartallo/h264/hl_codec_264_me_ds.h" // FIXME: remove
#include "hartallo/h264/hl_codec_264.h"
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
#include "hartallo/h264/hl_codec_264_decode_svc.h"
#include "hartallo/h264/hl_codec_264_transf.h"
#include "hartallo/h264/hl_codec_264_quant.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_me.h"
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
extern HL_ERROR_T (*hl_codec_264_interpol_chroma)(
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    const struct hl_codec_264_mv_xs* mvCLX,
    const struct hl_codec_264_mv_xs* mvLX,
    const struct hl_codec_264_pict_s* refPicLXCb,
    const struct hl_codec_264_pict_s* refPicLXCr,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);

static HL_ERROR_T _hl_codec_264_rdo_mb_guess_best_intra16x16_pred(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    double d_lambda,
    HL_OUT int32_t* CodedBlockPatternLuma4x4,
    HL_OUT int32_t *best_dist,
    HL_OUT double *best_cost
);
static HL_ERROR_T _hl_codec_264_rdo_mb_guess_best_intra4x4_pred(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    double d_lambda,
    HL_OUT int32_t* CodedBlockPatternLuma4x4,
    HL_OUT int32_t *best_dist,
    HL_OUT double *best_cost
);
static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_intra16x16_luma(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec
);
static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_inter(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    int32_t Single_ctr_luma
);
static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_chroma(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    int32_t predCb[16][16],
    int32_t predCr[16][16]
);
static HL_ERROR_T _hl_codec_264_rdo_mb_guess_cbp(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec
);
static HL_ERROR_T _hl_codec_264_rdo_mb_is_zeros_inter16x16_chroma(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    int32_t refIdxL0,
    const hl_codec_264_mv_xt* p_mvLX,
    hl_bool_t *pb_zeros);
static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_luma_pskip(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    const hl_codec_264_mv_xt* pc_mvPSkip
);

static double last_best_intra_cost; // FIXME

HL_ERROR_T hl_codec_264_rdo_mb_guess_best_intra_pred_avc(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec, int32_t* pi_mad)
{
    HL_ERROR_T err;
    hl_codec_264_layer_t* pc_layer;
    double best_cost16x16, best_cost4x4, d_lambda;
    int32_t Intra16x16CodedBlockPatternLuma4x4, Intra4x4CodedBlockPatternLuma4x4, best_dist16x16, best_dist4x4;
    hl_codec_264_encode_slice_data_t* pc_esd;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];

    if (p_mb->u_addr == 58) {
        // FIXME:
        int a = 0;
    }

    // FIXME: do not use lambda as param
    // Update lambda to allow comparison with P_MBs
    //d_lambda = IsSliceHeaderP(pc_esd->pc_slice->p_header) ? p_codec->encoder.rdo.d_lambda_motion : p_codec->encoder.rdo.d_lambda_mode;
    d_lambda = p_codec->encoder.rdo.d_lambda_mode;

    // Map memory blocks
    // hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Intra16x16_TotalCoeffsLuma);

    // Best cost for Intra16x16 (WITHOUT luma reconstruction)
    // This function sets: "Intra16x16PredMode", "Intra16x16DCLevel", "Intra16x16ACLevel",
    err = _hl_codec_264_rdo_mb_guess_best_intra16x16_pred(
              p_mb,
              p_codec,
              d_lambda,
              &Intra16x16CodedBlockPatternLuma4x4,
              &best_dist16x16,
              &best_cost16x16);
    if (err) {
        goto bail;
    }

    // FIXME:
    //best_cost16x16 = DBL_MAX;

    // FIXME
    if (Intra16x16CodedBlockPatternLuma4x4 == 0) {
        // FIXME:
        int a = 0;
    }

    if (best_cost16x16 == 0) {
        best_dist4x4 = INT_MAX;
        best_cost4x4 = DBL_MAX;
    }
    else {
        // Best cost for Intra4x4 (WITH luma reconstruction)
        // This function sets: "CodedBlockPatternLuma4x4", "Intra4x4PredMode", "LumaLevel"
        err = _hl_codec_264_rdo_mb_guess_best_intra4x4_pred(
                  p_mb,
                  p_codec,
                  d_lambda,
                  &Intra4x4CodedBlockPatternLuma4x4,
                  &best_dist4x4,
                  &best_cost4x4);
        if (err) {
            goto bail;
        }
    }

    // FIXME:
    //best_cost4x4 = DBL_MAX;


// FIXME
#if 0
    p_mb->intra_chroma_pred_mode = Intra_Chroma_DC;
#else
    // The chroma4x4 chroma prediction is simular to intra16x16, so guess it from there
    switch (p_mb->Intra16x16PredMode) {
    case Intra_16x16_Vertical:
        p_mb->intra_chroma_pred_mode = Intra_Chroma_Vertical;
        break;
    case Intra_16x16_Plane:
        p_mb->intra_chroma_pred_mode = Intra_Chroma_Plane;
        break;
    case Intra_16x16_Horizontal:
        p_mb->intra_chroma_pred_mode = Intra_Chroma_Horizontal;
        break;
    case Intra_16x16_DC:
        p_mb->intra_chroma_pred_mode = Intra_Chroma_DC;
        break;
    }
#endif

    // Choose best modes and perform default initialization
    if (best_cost4x4 < best_cost16x16) {
        /* Intra4x4 */
        p_mb->mb_type = I_NxN;
        p_mb->e_type = HL_CODEC_264_MB_TYPE_I_NXN;
        p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4;
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_4X4;
        p_mb->CodedBlockPatternLuma4x4 = Intra4x4CodedBlockPatternLuma4x4;

        // Predict 4x4 Intra modes(Outputs: "prev_intra4x4_pred_mode_flag" and "rem_intra4x4_pred_mode")
        hl_codec_264_pred_intra_perform_prediction_modes_4x4(p_codec, p_mb);

        {
            // Update 4x4 cost
            int32_t i_rbc;
            // prev_intra4x4_pred_mode_flag - u(1)
            i_rbc = 16; // TODO: CABAC
            // rem_intra4x4_pred_mode - u(3)
            i_rbc += hl_math_nz16_cpp(p_mb->prev_intra4x4_pred_mode_flag) * 3; // TODO: CABAC

            best_cost4x4 += (d_lambda * i_rbc);
        }

        // Set MAD used for RC (Rate Control)
        if (pi_mad) {
            *pi_mad = best_dist4x4;
        }
    }
    if (best_cost16x16 <= best_cost4x4) { // "<'='", the '=' is a MUST
        /* Intra16x16 */
        p_mb->mb_type = I_16x16_0_0_0; // Must be update once CBP is known
        p_mb->e_type = HL_CODEC_264_MB_TYPE_I_16X16_0_0_0;
        p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16;
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_16X16;
        p_mb->CodedBlockPatternLuma4x4 = Intra16x16CodedBlockPatternLuma4x4;

        // Set MAD used for RC (Rate Control)
        if (pi_mad) {
            *pi_mad = best_dist16x16;
        }
    }

    last_best_intra_cost = best_cost16x16 < best_cost4x4 ? best_cost16x16 : best_cost4x4;

    // Reconstruct Chroma
    {
        HL_ALIGNED(16) hl_int32_33_t* pCb33;
        HL_ALIGNED(16) hl_int32_33_t* pCr33;
        HL_ALIGNED(16) hl_int32_16x16_t* predCb;
        HL_ALIGNED(16) hl_int32_16x16_t* predCr;
        const hl_pixel_t *p_cSU, *p_cSV;

        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &pCb33);
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &pCr33);
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predCb);
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predCr);

        p_cSU = (pc_layer->pc_fs_curr->p_pict->pc_data_u);
        p_cSV = (pc_layer->pc_fs_curr->p_pict->pc_data_v);

        hl_codec_264_pred_intra_get_neighbouring_samples_C(p_codec, p_mb, p_cSU, p_cSV, *pCb33, *pCr33);
        hl_codec_264_pred_intra_perform_prediction_chroma(p_codec, p_mb, *predCb, *predCr, *pCb33, *pCr33, p_mb->intra_chroma_pred_mode);

        err = _hl_codec_264_rdo_mb_reconstruct_chroma(p_mb, p_codec, (*predCb), (*predCr));

        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, pCb33);
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, pCr33);
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predCb);
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predCr);

        if (err) {
            goto bail;
        }
    }

    // Reconstruct Luma16x16 (Luma4x4 already reconstructed when guessing modes)
    if (HL_CODEC_264_MB_TYPE_IS_INTRA_16x16(p_mb)) {
        err = _hl_codec_264_rdo_mb_reconstruct_intra16x16_luma(p_mb, p_codec);
        if (err) {
            goto bail;
        }
    }

    // Guess CBP
    err = _hl_codec_264_rdo_mb_guess_cbp(p_mb, p_codec);
    if (err) {
        goto bail;
    }

    // Update "mb_type" using Table 7-11 for Intra16x16 now that CBP is knwon
    if (HL_CODEC_264_MB_TYPE_IS_INTRA_16x16(p_mb)) {
        p_mb->mb_type += ((p_mb->CodedBlockPatternChroma << 2) + p_mb->Intra16x16PredMode + (p_mb->CodedBlockPatternLuma ? 12 : 0));
    }

    if (IsSliceHeaderP(pc_esd->pc_slice->p_header)) {
        p_mb->mb_type += 5;
    }

#if 0
    err = hl_codec_264_mb_print_samples(p_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_esd->pc_slice->p_header->PicWidthInSamplesL, 0/*Y*/);
    if (err) {
        return err;
    }
#endif

bail:
    // Unmap memory blocks
    // hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Intra16x16_TotalCoeffsLuma);

    return err;
}

HL_ERROR_T hl_codec_264_rdo_mb_guess_best_intra_pred_svc(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec)
{
#if 1
    // Code from "_hl_codec_264_decode_svc_resample_intra()"

    HL_ERROR_T err;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_pixel_t *p_eSL, *p_cSL, *_p_eSL;
    HL_ALIGNED(16) hl_int32_16x16_t* mbPredL;
    HL_ALIGNED(16) hl_int32_16x16_t* mbPredCb;
    HL_ALIGNED(16) hl_int32_16x16_t* mbPredCr;
    HL_ALIGNED(16) hl_int32_16x4x4_t* Residual16x4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4L;
    HL_ALIGNED(16) hl_int32_16x4x4_t* ACCoeff16x4x4L;
    hl_bool_t isAllZeros;
    int32_t xO, yO, luma4x4BlkIdx;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;
    static const hl_bool_t __isIntra16x16False = HL_FALSE;
    static const hl_bool_t __isIntraBlockTrue = HL_TRUE;
    static const int32_t chromaFlagZero = 0;
    static const int32_t chromaFlagOne = 1;
    static const int32_t iCbCrMinus1 = -1;
    static const int32_t iCbCrZero = 0;
    static const int32_t iCbCrOne = 1;

    extern HL_ERROR_T _hl_codec_264_decode_svc_resample_intra_colour_comps(
        hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t chromaFlag, int32_t iCbCr,
        int32_t mbW, int32_t mbH, HL_OUT int32_t mbPred[16][16]
    );

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    p_eSL = (p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);
    p_cSL = (pc_layer->pc_fs_curr->p_pict->pc_data_y);

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &mbPredL);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &mbPredCb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &mbPredCr);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Residual16x4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff16x4x4L);

    // TODO: For now (beta version) we always reuse prediction from base layer. This is very bad.
    // We must perform another RDO cycle and compare with the one from base layer
    p_mb->ext.svc.base_mode_flag = 1;

    /* Set default values */
    p_mb->mb_type = HL_CODEC_264_SVC_MB_TYPE_INFERRED;
    p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_BL;
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INFERRED;
    p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_BL;
    p_mb->NumMbPart = 1;
    p_mb->MbPartWidth = p_mb->MbPartHeight = 16;
    p_mb->CodedBlockPatternLuma4x4 = 0;

    /* G.8.1.5.1 Macroblock initialisation process */
    err = hl_codec_264_utils_derivation_process_initialisation_svc(p_codec, p_mb); // FIXME: needed?
    if (err) {
        goto bail;
    }

    // FIXME
    if (p_mb->u_addr == 333) {
        int a = 0;
    }

    // G.8.6.2.1 Resampling process for intra samples of a macroblock colour component
    err = _hl_codec_264_decode_svc_resample_intra_colour_comps(p_codec, p_mb, chromaFlagZero, iCbCrMinus1, 16, 16, (*mbPredL));
    if (err) {
        goto bail;
    }
    if (pc_sps->ChromaArrayType != 0) {
        // G.8.6.2.1 Resampling process for intra samples of a macroblock colour component
        err = _hl_codec_264_decode_svc_resample_intra_colour_comps(p_codec, p_mb, chromaFlagOne, iCbCrZero, pc_sps->MbWidthC, pc_sps->MbHeightC, (*mbPredCb));
        if (err) {
            goto bail;
        }
        err = _hl_codec_264_decode_svc_resample_intra_colour_comps(p_codec, p_mb, chromaFlagOne, iCbCrOne, pc_sps->MbWidthC, pc_sps->MbHeightC, (*mbPredCr));
        if (err) {
            goto bail;
        }
    }

    // FIXME: split I_4x4, I16x16

    // Build Luma coeffs and reconstruct for prediction
    for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
        xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
        yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
        // Move samples pointer to the right 4x4 block
        _p_eSL = p_eSL + (yO * pc_slice_header->PicWidthInSamplesL) + xO;
        // Get residual
        hl_math_sub4x4_u8x32(
            _p_eSL, pc_slice_header->PicWidthInSamplesL,
            (const int32_t*)&(*mbPredL)[yO][xO], 16,
            (int32_t*)(*Residual16x4x4L)[luma4x4BlkIdx], 4);
        isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*Residual16x4x4L)[luma4x4BlkIdx]);
        if (isAllZeros) {
            hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0); // FIXME: not needed: remove
        }
        else {
            // Forward transform Cf
            hl_codec_264_transf_frw_residual4x4((*Residual16x4x4L)[luma4x4BlkIdx], (*tmp4x4L));
            // Scaling and quantization
            hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockTrue, (*tmp4x4L), (*ACCoeff16x4x4L)[luma4x4BlkIdx]);
            isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*ACCoeff16x4x4L)[luma4x4BlkIdx]);
            if (isAllZeros) {
                hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0); // FIXME: not needed: remove
            }
            else {
                // ZigZag4x4()
                Scan4x4_L((*ACCoeff16x4x4L)[luma4x4BlkIdx], p_mb->LumaLevel[luma4x4BlkIdx], __isIntra16x16ACFalse);
                p_mb->CodedBlockPatternLuma4x4 |= (1 << luma4x4BlkIdx);
            }
        }

        // Reconstruct Luma
        if ((p_mb->CodedBlockPatternLuma4x4 & (1 << luma4x4BlkIdx))) {
            // 8.5.12 Scaling and transformation process for residual 4x4 blocks
            hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*ACCoeff16x4x4L)[luma4x4BlkIdx], (*Residual16x4x4L)[luma4x4BlkIdx], __isLumaTrue, __isIntra16x16False, -1);
            // Add predicted samples to residual and clip the result
            hl_math_addclip_4x4((const int32_t*)&(*mbPredL)[yO][xO], 16, (const int32_t*)(*Residual16x4x4L)[luma4x4BlkIdx], 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4L), 4);
            // 8.5.14 Picture construction process prior to deblocking filter process
            hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*tmp4x4L), 4, luma4x4BlkIdx);
        }
        else {
            // No residual means "PRED" samples = "reconstructed" samples
            // 8.5.14 Picture construction process prior to deblocking filter process
            hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)&(*mbPredL)[yO][xO], 16, luma4x4BlkIdx);
        }
    }

    // Buil chroma coeffs and reconstruct
    err = _hl_codec_264_rdo_mb_reconstruct_chroma(p_mb, p_codec, (*mbPredCb), (*mbPredCr));
    if (err) {
        goto bail;
    }

    // Guess CBP
    err = _hl_codec_264_rdo_mb_guess_cbp(p_mb, p_codec);
    if (err) {
        goto bail;
    }

bail:
// Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, mbPredL);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, mbPredCb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, mbPredCr);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Residual16x4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff16x4x4L);
    return err;
#else
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer, *pc_layer_ref;
    int32_t xP, yP, xO, yO, refMbAddr, refMbPartIdx, refSubMbPartIdx, luma4x4BlkIdx;
    const hl_codec_264_mb_t* pc_mb_ref;
    const hl_pixel_t *p_eSL, *p_cSL, *_p_eSL;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_bool_t isAllZeros;
    HL_ALIGNED(16) hl_int32_16x4x4_t* Residual16x4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4L;
    HL_ALIGNED(16) hl_int32_16x4x4_t* Pred16x4x4L;
    HL_ALIGNED(16) hl_int32_16x4x4_t* ACCoeff16x4x4L;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;
    static const hl_bool_t __isIntra16x16False = HL_FALSE;
    static const hl_bool_t __isIntraBlockTrue = HL_TRUE;

    pc_layer = p_codec->layers.pc_active;
    pc_layer_ref = pc_layer->pc_ref;
    pc_slice_header = pc_layer->pc_slice_hdr;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    p_eSL = (p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);
    p_cSL = (pc_layer->pc_fs_curr->p_pict->pc_data_y);

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Residual16x4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Pred16x4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff16x4x4L);

    // TODO: For now (beta version) we always reuse prediction from base layer. This is very bad.
    // We must perform another RDO cycle and compare with the one from base layer
    p_mb->ext.svc.base_mode_flag = 1;

    /* Set default values */
    p_mb->mb_type = HL_CODEC_264_SVC_MB_TYPE_INFERRED;
    p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_BL;
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INFERRED;
    if (IsSliceHeaderEI(pc_layer->pc_slice_hdr)) {
        // Table G-5 – Inferred macroblock type I_BL for EI slices.
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_BL;
    }
    else {
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTER_BL;
    }
    p_mb->NumMbPart = 1;
    p_mb->MbPartWidth = p_mb->MbPartHeight = 16;

    /* G.8.1.5.1 Macroblock initialisation process */
    err = hl_codec_264_utils_derivation_process_initialisation_svc(p_codec, p_mb);
    if (err) {
        goto bail;
    }

    // "G.6.2" should be called for each (4x4) block but in our case it's not needed as we're sure all partitions and sub-partitions are INTRA coded
    // TODO: call function for each 4x4 when support for INTRA coding in P and B slices is added (see "G.8.6.1.1" to see understand how (4x4) blocks are called)
    yP = 1; // (y << 2) + 1;
    xP = 1; // (x << 2) + 1;
    // G.6.2 Derivation process for reference layer partitions
    hl_codec_264_utils_derivation_process_for_ref_layer_partitions_svc(p_codec, p_mb, xP, yP, &refMbAddr, &refMbPartIdx, &refSubMbPartIdx);
    if (refMbAddr < 0 || refMbAddr >= (int32_t)pc_layer_ref->u_list_macroblocks_count || !(pc_mb_ref = pc_layer_ref->pp_list_macroblocks[refMbAddr])) {
        HL_DEBUG_ERROR("%d not valid as 'refMbAddr' value", refMbAddr);
        return HL_ERROR_INVALID_BITSTREAM;
    }
    // p_mb->flags_type = pc_mb_ref->flags_type;
    // p_mb->MbPartPredMode[0] = pc_mb_ref->MbPartPredMode[0];
    p_mb->CodedBlockPatternLuma4x4 = 0;

    /* Build Luma 4x4 coefficients */
    if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mb_ref, 0)) {
        HL_ALIGNED(16) hl_int32_13_t* p13;

        // Map memory blocks
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &p13);

        for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
            // Get neighbouring 4x4 samples for prediction
            /*err = */hl_codec_264_pred_intra_get_neighbouring_samples_4x4L(p_codec, p_mb, luma4x4BlkIdx, &xO, &yO, p_cSL, (*p13));
            if (err) {
                goto Intra_4x4_done;
            }
            // Move samples pointer to the right 4x4 block
            _p_eSL = p_eSL + (yO * pc_slice_header->PicWidthInSamplesL) + xO;
            // Get PRED samples
            hl_codec_264_pred_intra_perform_prediction_4x4L(p_codec, p_mb, (*Pred16x4x4L)[luma4x4BlkIdx], (*p13), pc_mb_ref->Intra4x4PredMode[luma4x4BlkIdx]);
            // Get residual
            hl_math_sub4x4_u8x32(
                _p_eSL, pc_slice_header->PicWidthInSamplesL,
                (const int32_t*)(*Pred16x4x4L)[luma4x4BlkIdx], 4,
                (int32_t*)(*Residual16x4x4L)[luma4x4BlkIdx], 4);
            isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*Residual16x4x4L)[luma4x4BlkIdx]);
            if (isAllZeros) {
                hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0);
            }
            else {
                // Forward transform Cf
                hl_codec_264_transf_frw_residual4x4((*Residual16x4x4L)[luma4x4BlkIdx], (*tmp4x4L));
                // Scaling and quantization
                hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockTrue, (*tmp4x4L), (*ACCoeff16x4x4L)[luma4x4BlkIdx]);
                isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*ACCoeff16x4x4L)[luma4x4BlkIdx]);
                if (isAllZeros) {
                    hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0);
                }
                else {
                    // ZigZag4x4()
                    Scan4x4_L((*ACCoeff16x4x4L)[luma4x4BlkIdx], p_mb->LumaLevel[luma4x4BlkIdx], __isIntra16x16ACFalse);
                    p_mb->CodedBlockPatternLuma4x4 |= (1 << luma4x4BlkIdx);
                }
            }
        }

Intra_4x4_done:
        // Unmap memory blocks
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, p13);
    } // end-of-4x4
    /* Build Luma 4x4 coefficients */
    else if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mb_ref, 0)) {
        HL_DEBUG_ERROR("Not implemented yet");
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    } // end-of-16x16
    else {
        HL_DEBUG_ERROR("Not implemented yet");
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    }

    /* Reconstruct Luma4x4 */
    if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mb_ref, 0)) {
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
            if ((p_mb->CodedBlockPatternLuma4x4 & (1 << luma4x4BlkIdx))) {
                // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*ACCoeff16x4x4L)[luma4x4BlkIdx], (*Residual16x4x4L)[luma4x4BlkIdx], __isLumaTrue, __isIntra16x16False, -1);
                if (pc_mb_ref->TransformBypassModeFlag == 1 && (pc_mb_ref->Intra4x4PredMode[luma4x4BlkIdx] == Intra_4x4_Vertical || pc_mb_ref->Intra4x4PredMode[luma4x4BlkIdx] == Intra_4x4_Horizontal)) {
                    // 8.5.15 Intra residual transform-bypass decoding process
                    hl_codec_264_transf_bypass_intra_residual(4, 4, (int32_t)pc_mb_ref->Intra4x4PredMode[luma4x4BlkIdx], (*Residual16x4x4L)[luma4x4BlkIdx]);
                }
                // Add predicted samples to residual and clip the result
                hl_math_addclip_4x4((const int32_t*)(*Pred16x4x4L)[luma4x4BlkIdx], 4, (const int32_t*)(*Residual16x4x4L)[luma4x4BlkIdx], 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4L), 4);
                // 8.5.14 Picture construction process prior to deblocking filter process
                hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*tmp4x4L), 4, luma4x4BlkIdx);
            }
            else {
                // No residual means "PRED" samples = "reconstructed" samples
                // 8.5.14 Picture construction process prior to deblocking filter process
                hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*Pred16x4x4L)[luma4x4BlkIdx], 4, luma4x4BlkIdx);
            }
        }
    }// end-of-4x4
    /* Build Luma 4x4 coefficients */
    else if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mb_ref, 0)) {
        HL_DEBUG_ERROR("Not implemented yet");
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    } // end-of-16x16
    else {
        HL_DEBUG_ERROR("Not implemented yet");
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    }

    // Reconstruct Chroma
    {
        HL_ALIGNED(16) hl_int32_33_t* pCb33;
        HL_ALIGNED(16) hl_int32_33_t* pCr33;
        HL_ALIGNED(16) hl_int32_16x16_t* predCb;
        HL_ALIGNED(16) hl_int32_16x16_t* predCr;
        const hl_pixel_t *p_cSU, *p_cSV;

        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &pCb33);
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &pCr33);
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predCb);
        hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predCr);

        p_cSU = (pc_layer->pc_fs_curr->p_pict->pc_data_u);
        p_cSV = (pc_layer->pc_fs_curr->p_pict->pc_data_v);

        hl_codec_264_pred_intra_get_neighbouring_samples_C(p_codec, p_mb, p_cSU, p_cSV, *pCb33, *pCr33);
        hl_codec_264_pred_intra_perform_prediction_chroma(p_codec, p_mb, *predCb, *predCr, *pCb33, *pCr33, pc_mb_ref->intra_chroma_pred_mode);

        err = _hl_codec_264_rdo_mb_reconstruct_chroma(p_mb, p_codec, (*predCb), (*predCr));

        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, pCb33);
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, pCr33);
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predCb);
        hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predCr);

        if (err) {
            goto bail;
        }
    }

    // Guess CBP
    err = _hl_codec_264_rdo_mb_guess_cbp(p_mb, p_codec);
    if (err) {
        goto bail;
    }
    /*p_mb->CodedBlockPatternLuma  = 0;
    p_mb->CodedBlockPatternChroma = 0;
    p_mb->coded_block_pattern = 0;*/

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Residual16x4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Pred16x4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff16x4x4L);

    return err;
#endif
}

HL_ERROR_T hl_codec_264_rdo_mb_guess_best_inter_pred_avc(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec, int32_t *pi_mad)
{
    typedef struct hl_codec_264_rdo_part_def_xs {
        HL_CODEC_264_MB_TYPE_T mbType;
        const hl_codec_264_me_part_xt* parts;
        int32_t parts_count;
    }
    hl_codec_264_rdo_part_def_xt;

    HL_ERROR_T err;
    int32_t i, j, mbPartIdx, subMbPartIdx, refIdxL0;
    hl_codec_264_layer_t* pc_layer;
    uint32_t max_ref_frame, u, me_range;
    hl_bool_t b_best_found, b_probably_pskip, b_pskip;
    double cost_sum, best_cost;
    int32_t dist_sum, best_dist, Single_ctr_sum, best_Single_ctr, homogeneousities[4/*mbPartIdx*/];
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_dpb_fs_t* pc_fs_ref; // Frame used for reference
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_mv_xt mv_pskip;
    const hl_codec_264_rdo_part_def_xt* pc_part_def, *BestPartDef;
    const hl_codec_264_me_part_xt *pc_part, *BestPart;
    HL_CODEC_264_SUBMB_TYPE_T currSubMbType;
    hl_codec_264_mv_xt (*MvL0)[4]/*mbPartIdx*/[4/*subMbPartIdx*/], (*MvCL0)[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    int32_t (*PredFlag0)[4];
    int32_t (*RefIdxL0)[4];
    int32_t BestRefIdxL0[4/*mbPartIdx*/][4/*subMbPartIdx*/];
    int32_t BestMvL0[4/*mbPartIdx*/][4/*subMbPartIdx*/][2/*x,y*/];
    int32_t BestMVP[4/*mbPartIdx*/][4/*subMbPartIdx*/][2/*x,y*/], MVP[4/*mbPartIdx*/][4/*subMbPartIdx*/][2/*x,y*/];
    int32_t ModeFlags;

    static const hl_codec_264_me_part_xt __rdo_me_part16x16 [] = {
        /* 16x16 */
        {
            1, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_NA }, // SubMbType[4/*mbPartIdx*/]
            { 1 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 16 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 16 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_NA }, // SubMbPredMode[4/*mbPartIdx*/]
            { 16 }, // MbPartWidth( mb_type )
            { 16 }, // MbPartHeight( mb_type )
            { 3 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_16X16 }, // Mode
        },
    };
    static const hl_codec_264_me_part_xt __rdo_me_part16x8 [] = {
        /* 16x8 */
        {
            2, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_NA, HL_CODEC_264_SUBMB_TYPE_NA }, // SubMbType[4/*mbPartIdx*/]
            { 1, 1, 0, 0 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 16, 16, 0, 0 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 8, 8, 0, 0 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0 }, // SubMbPredMode[4/*mbPartIdx*/]
            { 16 }, // MbPartWidth( mb_type )
            { 8 }, // MbPartHeight( mb_type )
            { 5 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_16X8 }, // Mode
        },
    };
    static const hl_codec_264_me_part_xt __rdo_me_part8x16 [] = {
        /* 8x16 */
        {
            2, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_NA, HL_CODEC_264_SUBMB_TYPE_NA }, // SubMbType[4/*mbPartIdx*/]
            { 1, 1, 0, 0 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 8, 8, 0, 0 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 16, 16, 0, 0 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0 }, // SubMbPredMode[4/*mbPartIdx*/]
            { 8 }, // MbPartWidth( mb_type )
            { 16 }, // MbPartHeight( mb_type )
            { 5 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_8X16 }, // Mode
        },
    };
    static const hl_codec_264_me_part_xt __rdo_me_part8x8 [] = {
        // 8x8, 8x8, 8x8, 8x8
        {
            4, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_P_L0_8X8, HL_CODEC_264_SUBMB_TYPE_P_L0_8X8, HL_CODEC_264_SUBMB_TYPE_P_L0_8X8, HL_CODEC_264_SUBMB_TYPE_P_L0_8X8 }, // SubMbType[4/*mbPartIdx*/]
            { 1, 1, 1, 1 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 8, 8, 8, 8 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 8, 8, 8, 8 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0 }, // SubMbPredMode[4/*mbPartIdx*/]
            { 8 },// MbPartWidth( mb_type )
            { 8 },// MbPartHeight( mb_type )
            { 11 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_8X8_SUB8X8 }, // Mode
        },
        // 8x4, 8x4, 8x4, 8x4
        {
            4, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_P_L0_8X4, HL_CODEC_264_SUBMB_TYPE_P_L0_8X4, HL_CODEC_264_SUBMB_TYPE_P_L0_8X4, HL_CODEC_264_SUBMB_TYPE_P_L0_8X4 }, // SubMbType[4/*mbPartIdx*/]
            { 2, 2, 2, 2 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 8, 8, 8, 8 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 4, 4, 4, 4 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0 }, // SubMbPredMode[4/*mbPartIdx*/]
            { 8 },// MbPartWidth( mb_type )
            { 8 },// MbPartHeight( mb_type )
            { 19 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_8X8_SUB8X4 }, // Mode
        },
        // 4x8, 4x8, 4x8, 4x8
        {
            4, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_P_L0_4X8, HL_CODEC_264_SUBMB_TYPE_P_L0_4X8, HL_CODEC_264_SUBMB_TYPE_P_L0_4X8, HL_CODEC_264_SUBMB_TYPE_P_L0_4X8 }, // SubMbType[4/*mbPartIdx*/]
            { 2, 2, 2, 2 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 4, 4, 4, 4 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 8, 8, 8, 8 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0 }, // SubMbPredMode[4/*mbPartIdx*/]
            { 8 },// MbPartWidth( mb_type )
            { 8 },// MbPartHeight( mb_type )
            { 19 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_8X8_SUB4X8 }, // Mode
        },
        // 4x4, 4x4, 4x4, 4x4
        {
            4, // NumMbPart
            { HL_CODEC_264_SUBMB_TYPE_P_L0_4X4, HL_CODEC_264_SUBMB_TYPE_P_L0_4X4, HL_CODEC_264_SUBMB_TYPE_P_L0_4X4, HL_CODEC_264_SUBMB_TYPE_P_L0_4X4 }, // SubMbType[4/*mbPartIdx*/]
            { 4, 4, 4, 4 }, // NumSubMbPart[4/*mbPartIdx*/]
            { 4, 4, 4, 4 }, // SubMbPartWidth[4/*mbPartIdx*/]
            { 4, 4, 4, 4 },	// SubMbPartHeight[4/*mbPartIdx*/]
            { HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0, HL_CODEC_264_SUBMB_MODE_PRED_L0 }, // SubMbPredMode[4/*mbPartIdx*/]
            { 8 },// MbPartWidth( mb_type )
            { 8 },// MbPartHeight( mb_type )
            { 27 }, // NumHeaderBitsCAVLC
            { HL_CODEC_264_MODE_8X8_SUB4X4 }, // Mode
        },
    };

    static const hl_codec_264_rdo_part_def_xt __rdo_me_parts[] = {
        {
            HL_CODEC_264_MB_TYPE_P_L0_16X16, // mbType
            __rdo_me_part16x16, // parts
            sizeof(__rdo_me_part16x16)/sizeof(__rdo_me_part16x16[0]) // parts_count
        },
        {
            HL_CODEC_264_MB_TYPE_P_L0_L0_16X8, // mbType
            __rdo_me_part16x8, // parts
            sizeof(__rdo_me_part16x8)/sizeof(__rdo_me_part16x8[0])
        },
        {
            HL_CODEC_264_MB_TYPE_P_L0_L0_8X16, // mbType
            __rdo_me_part8x16, // parts
            sizeof(__rdo_me_part8x16)/sizeof(__rdo_me_part8x16[0])
        },
        {
            HL_CODEC_264_MB_TYPE_P_8X8REF0, // mbType
            __rdo_me_part8x8, // parts
            sizeof(__rdo_me_part8x8)/sizeof(__rdo_me_part8x8[0]) // parts_count
        },
    };
    static const int32_t __rdo_me_parts_count = (sizeof(__rdo_me_parts) / sizeof(__rdo_me_parts[0]));

    if (p_codec->layers.currDQId > 0) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_layer->pc_slice_hdr;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;
    max_ref_frame = (pc_slice_header->num_ref_idx_l0_active_minus1 + 1);
    pc_esd->rdo.me.pc_SL_enc = p_codec->encoder.pc_frame->data_ptr[0];
    me_range = HL_MATH_CLIP3(HL_CODEC_264_ME_RANGE_MIN, HL_CODEC_264_ME_RANGE_MAX, p_codec->pc_base->me_range);
    currSubMbType = HL_CODEC_264_SUBMB_TYPE_NA; // Unsless unless for "B_8x8"
    MvL0 = pc_layer->SVCExtFlag ? &p_mb->mvL0 : &p_mb->MvL0; // TODO: "L1"
    MvCL0 = pc_layer->SVCExtFlag ? &p_mb->mvCL0 : &p_mb->MvCL0; // TODO: "L1"
    RefIdxL0 = pc_layer->SVCExtFlag ? &p_mb->refIdxL0 : &p_mb->RefIdxL0;  // TODO: "L1"
    PredFlag0 = pc_layer->SVCExtFlag ? &p_mb->predFlagL0 : &p_mb->PredFlagL0;  // TODO: "L1"
    best_cost = DBL_MAX;
    best_dist = 0;
    BestPart = HL_NULL;
    b_best_found = HL_FALSE;
    best_Single_ctr = 9;

    // Macroblock is P by default
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTER_P; // TODO: "B"

    if (p_mb->u_addr == 14) {
        int a = 0; // FIXME
    }

    for (u = 0; u < max_ref_frame; ++u) {
        pc_fs_ref = pc_layer->pobj_poc->RefPicList0[u]; // TODO: B frames
        if (!pc_fs_ref || !HL_CODEC_264_REF_TYPE_IS_USED(pc_fs_ref->RefType)) {
            continue;
        }
        pc_esd->rdo.me.refIdxLX = (int32_t)u;
        // Set reference samples
        pc_esd->rdo.me.pc_SL_ref = pc_fs_ref->p_pict->pc_data_y;
        // Enable all modes
        ModeFlags = 0xFFFF;
        for (i = 0; i < __rdo_me_parts_count && !b_best_found; ++i) {
            pc_part_def = &__rdo_me_parts[i];
            p_mb->e_type = pc_part_def->mbType;

            for (j = 0; j < pc_part_def->parts_count; ++j) {
                pc_part = &pc_part_def->parts[j];

                // Check if current mode is enabled
                if (!((1 << pc_part->Mode) & ModeFlags)) {
                    continue;
                }

                // JVT-O079: - 2.1.3.4.3.1 Homogeneous block detection
                if (p_codec->pc_base->me_early_term_flag && pc_part->Mode == HL_CODEC_264_MODE_16X16) { // 16x16(first mode)?
                    static const int32_t PART8X8_X[4/*mbPartIdx*/] = { 0, 8, 0, 8 };
                    static const int32_t PART8X8_Y[4/*mbPartIdx*/] = { 0, 0, 8, 8 };

                    const hl_pixel_t* pc_SL_enc_mb;
                    int32_t x_non_hz_vt_edge_start = (p_mb->xL == 0) ? 1 : ((p_mb->xL == (pc_slice_header->PicWidthInSamplesL - 16)) ? (pc_slice_header->PicWidthInSamplesL - 17) : p_mb->xL);
                    int32_t y_non_hz_vt_edge_start = (p_mb->yL == 0) ? 1 : ((p_mb->yL == (pc_slice_header->PicHeightInSamplesL - 16)) ? (pc_slice_header->PicHeightInSamplesL - 17) : p_mb->yL);
                    if (p_mb->u_addr == 3) {
                        int a = 0; // FIXME
                    }
                    for (mbPartIdx = 0; mbPartIdx < 4; ++mbPartIdx) {
                        pc_SL_enc_mb = pc_esd->rdo.me.pc_SL_enc
                                       + (PART8X8_X[mbPartIdx] + x_non_hz_vt_edge_start)
                                       + ((PART8X8_Y[mbPartIdx] + y_non_hz_vt_edge_start) * pc_slice_header->PicWidthInSamplesL);
                        homogeneousities[mbPartIdx] = hl_math_homogeneousity8x8_u8(pc_SL_enc_mb, pc_slice_header->PicWidthInSamplesL);
                    }

                    if (homogeneousities[0] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8
                            && homogeneousities[1] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8
                            && homogeneousities[2] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8
                            && homogeneousities[3] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8) {
                        ModeFlags = (1 << HL_CODEC_264_MODE_16X16);
                    }
                    else if ((homogeneousities[0] + homogeneousities[1] + homogeneousities[2] + homogeneousities[3]) < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH16X16) {
                        if (homogeneousities[0] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8 && homogeneousities[1] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8) {
                            ModeFlags = (1 << HL_CODEC_264_MODE_16X16) | (1 << HL_CODEC_264_MODE_16X8);
                        }
                        else {
                            ModeFlags = (1 << HL_CODEC_264_MODE_16X16) | (1 << HL_CODEC_264_MODE_8X16);
                        }
                    }
                    else if (homogeneousities[0] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X4
                             && homogeneousities[1] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X4
                             && homogeneousities[2] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X4
                             && homogeneousities[3] < HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X4) {
                        ModeFlags = (1 << HL_CODEC_264_MODE_16X16)
                                    | (1 << HL_CODEC_264_MODE_16X8)
                                    | (1 << HL_CODEC_264_MODE_8X16)
                                    | (1 << HL_CODEC_264_MODE_8X8_SUB8X8)
                                    | (1 << HL_CODEC_264_MODE_8X8_SUB8X4)
                                    | (1 << HL_CODEC_264_MODE_8X8_SUB4X8);
                    }
                }

                pc_esd->rdo.me.currSubMbType = currSubMbType;
                pc_esd->rdo.me.me_range = me_range;
                err = hl_codec_264_me_ds_mb_find_best_cost(p_mb, p_codec, pc_part);
                if (err) {
                    return err;
                }
                cost_sum = 0;
                dist_sum = 0;
                Single_ctr_sum = 0;
                b_probably_pskip = pc_esd->rdo.me.b_probably_pskip;
                for (mbPartIdx = 0; mbPartIdx < p_mb->NumMbPart; ++mbPartIdx) {
                    for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                        // Accumulate the cost
                        cost_sum += pc_esd->rdo.me.d_best_cost[mbPartIdx][subMbPartIdx];
                        // Accumulate the dist
                        dist_sum += pc_esd->rdo.me.i_best_dist[mbPartIdx][subMbPartIdx];
                        // Accumulate "Single_ctr"
                        Single_ctr_sum += pc_esd->rdo.me.i_Single_ctr[mbPartIdx][subMbPartIdx];
                        // Set the best MV for the next predictions
                        (*MvL0)[mbPartIdx][subMbPartIdx].x = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].x; // FIXME: remove
                        (*MvL0)[mbPartIdx][subMbPartIdx].y = pc_esd->rdo.me.mvBest[mbPartIdx][subMbPartIdx].y; // FIXME: remove
                        MVP[mbPartIdx][subMbPartIdx][0] = pc_esd->rdo.me.mvpLX[mbPartIdx][subMbPartIdx].x;
                        MVP[mbPartIdx][subMbPartIdx][1] = pc_esd->rdo.me.mvpLX[mbPartIdx][subMbPartIdx].y;
                    }
                }

#if 0
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
                }
                for (mbPartIdx = 0; mbPartIdx < pc_part->NumMbPart; ++mbPartIdx) {
                    for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                        // compute "partWidth", "partHeight", "partWidthC" and "partHeightC" used by interpolation functions
                        p_mb->partWidth[mbPartIdx][subMbPartIdx] = p_mb->SubMbPartWidth[mbPartIdx];
                        p_mb->partHeight[mbPartIdx][subMbPartIdx] = p_mb->SubMbPartHeight[mbPartIdx];
                        p_mb->partWidthC[mbPartIdx][subMbPartIdx] = p_mb->partWidth[mbPartIdx][subMbPartIdx] >> pc_sps->SubWidthC_TrailingZeros;
                        p_mb->partHeightC[mbPartIdx][subMbPartIdx] = p_mb->partHeight[mbPartIdx][subMbPartIdx] >> pc_sps->SubHeightC_TrailingZeros;

                        // compute "xL_Idx" and "yL_Idx", used by interpolation functions
                        if (mbPartIdx == 0 && subMbPartIdx == 0) {
                            p_mb->xL_Idx = p_mb->xL;
                            p_mb->yL_Idx = p_mb->yL;
                            xP = yP = xS = yS = 0;
                        }
                        else {
                            // 6.4.2.1 - upper-left sample of the macroblock partition
                            xP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);// (6-11)
                            yP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);// (6-12)
                            // 6.4.2.2 - upper-left sample of the sub-macroblock partition
                            hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);

                            p_mb->xL_Idx = p_mb->xL + (xP + xS);
                            p_mb->yL_Idx = p_mb->yL + (yP + yS);
                        }
                        // Get best cost for this part/sub-part definition
                        err = hl_codec_264_me_mb_find_best_cost(
                                  p_mb,
                                  p_codec,
                                  pc_SL_ref,
                                  pc_SL_enc,
                                  currSubMbType,
                                  HL_CODEC_264_LIST_IDX_0, // TODO: B frames
                                  mbPartIdx,
                                  subMbPartIdx,
                                  (int32_t)u, /* refIdxLX */
                                  pc_part->Mode,
                                  (xP + xS),
                                  (yP + yS),
                                  (p_mb->SubMbPartWidth[mbPartIdx] >> 2),
                                  (p_mb->SubMbPartHeight[mbPartIdx] >> 2),
                                  me_range,
                                  &Single_ctr,
                                  &b_probably_pskip,
                                  &mvpLX,
                                  &dist,
                                  &cost,
                                  best_pos);
                        if (err) {
                            return err;
                        }
                        // Accumulate the cost
                        cost_sum += cost;
                        // Accumulate the dist
                        dist_sum += dist;
                        // Accumulate "Single_ctr"
                        Single_ctr_sum += Single_ctr;
                        // Set the best MV for the next predictions
                        (*MvL0)[mbPartIdx][subMbPartIdx].x = best_pos[0] /*<< (2 / i_best_pel)*/;
                        (*MvL0)[mbPartIdx][subMbPartIdx].y = best_pos[1] /*<< (2 / i_best_pel)*/;
                        MVP[mbPartIdx][subMbPartIdx][0] = mvpLX.x;
                        MVP[mbPartIdx][subMbPartIdx][1] = mvpLX.y;
                    } // end-of-for(subMbPartIdx)
                } //end-of-for(mbPartIdx)
#endif
                // JVT-O079 - 2.3 Elimination of single coefficients in inter macroblocks
                if (!b_probably_pskip && cost_sum && Single_ctr_sum < 6) {
                    // cost_sum = dist_sum; // Do not remove rate because distorsion computed with the single coeffs
                    // Now that residual is discarded, could it be a PSkip MB?
                    if (HL_CODEC_264_MB_TYPE_IS_P_L0_16X16(p_mb)) {
                        hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(
                            p_codec,
                            p_mb,
                            &mv_pskip,
                            &refIdxL0);
                        b_probably_pskip = refIdxL0 == 0 && (mv_pskip.x == MVP[0][0][0] && mv_pskip.y == MVP[0][0][1])
                                           && ((*MvL0)[0][0].x == MVP[0][0][0] && (*MvL0)[0][0].y == MVP[0][0][1]); // (MVD.x,MVD.y)=(0,0)
                    }
                    // HL_DEBUG_TALKATIVE("P(%u) - Single_ctr_sum=%d, b_probably_pskip=%s", p_mb->u_addr, Single_ctr_sum, b_probably_pskip?"TRUE":"FALSE");
                }
#if 0 // FIXME
                // Fast motion estimation based on FIXME
                if (p_codec->pc_base->me_early_term_flag && pc_part->Mode == HL_CODEC_264_MODE_16X16) {
                    p_mb->mvL0[0][0].x = (*MvL0)[0][0].x;
                    p_mb->mvL0[0][0].y = (*MvL0)[0][0].y;
                    err = _hl_codec_264_rdo_mb_reconstruct_inter(p_mb, p_codec, best_Single_ctr);
                    err = _hl_codec_264_rdo_mb_guess_cbp(p_mb, p_codec);
                    if (err) {
                        return err;
                    }
                    // FIXME (a woudji description): Lee and Jeon’s Early-SKIP algorithm [2] aborts the RDcost
                    // evaluation for zero macroblocks (CBP = 0) with motion vector equal to the PMV (predicted motion vector) in the previous frame.
                    if (p_mb->coded_block_pattern == 0 && u/*refIdxLX*/ == 0 && (*MvL0)[0][0].x == MVP[0][0][0] && (*MvL0)[0][0].y == MVP[0][0][1]) {
                        ModeFlags = 0;
                    }
                    else {
                        switch (p_mb->coded_block_pattern) {
                        case 0:
                        case 16:
                        case 32:
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            ModeFlags = 0;
                            break;
                        case 15:
                        case 31:
                        case 47:
                            ModeFlags = (1 << HL_CODEC_264_MODE_8X8_SUB8X8) | (1 << HL_CODEC_264_MODE_8X8_SUB8X4) | (1 << HL_CODEC_264_MODE_8X8_SUB4X8);
                            break;
                        default:
                            break;
                        }
                    }
                }
#endif

                // Add headers cost
                cost_sum += (p_codec->encoder.rdo.d_lambda_mode * pc_part->NumHeaderBitsCAVLC); // TODO: CABAC
                // Save Cost (will be used to compute Up layer prediction cost)
                pc_esd->rdo.Costs[pc_part->Mode] = cost_sum;
                pc_esd->rdo.BestMV00[pc_part->Mode][0] = (*MvL0)[0][0].x;
                pc_esd->rdo.BestMV00[pc_part->Mode][1] = (*MvL0)[0][0].y;
                if (cost_sum < best_cost) {
                    best_cost = cost_sum;
                    best_dist = dist_sum;
                    best_Single_ctr = Single_ctr_sum;
                    BestPart = pc_part;
                    BestPartDef = pc_part_def;
                    for (mbPartIdx = 0; mbPartIdx < pc_part->NumMbPart; ++mbPartIdx) {
                        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                            BestRefIdxL0[mbPartIdx][subMbPartIdx] = u;
                            BestMvL0[mbPartIdx][subMbPartIdx][0] = (*MvL0)[mbPartIdx][subMbPartIdx].x;
                            BestMvL0[mbPartIdx][subMbPartIdx][1] = (*MvL0)[mbPartIdx][subMbPartIdx].y;
                            BestMVP[mbPartIdx][subMbPartIdx][0] = MVP[mbPartIdx][subMbPartIdx][0];
                            BestMVP[mbPartIdx][subMbPartIdx][1] = MVP[mbPartIdx][subMbPartIdx][1];
                        }
                    }
                }
                else if (p_codec->pc_base->me_early_term_flag) {
                    //if (best_dist < dist_sum) {
                    //ModeFlags = 0; // FIXME
                    //}
                    // FIXME
                    //b_best_found |= HL_TRUE;
                    //break;
                }
            } // end-of-for(j)

            if ((b_pskip = b_probably_pskip)) {
                static int32_t refIdxL0_PSkip = 0; // "b_probably_pskip==true" means "refIdxL0==0" already checked by ME module
                // Check that chroma residual are zeros
                mv_pskip.x = BestMvL0[0][0][0];
                mv_pskip.y = BestMvL0[0][0][1];
                err = _hl_codec_264_rdo_mb_is_zeros_inter16x16_chroma(
                          p_mb,
                          p_codec,
                          refIdxL0_PSkip,
                          &mv_pskip,
                          &b_pskip);
                if (err) {
                    return err;
                }
            }

            b_best_found |= (best_cost == 0) || b_pskip; // FIXME: use Threshold
            if (p_codec->pc_base->me_early_term_flag) {
                // FIXME
                //b_best_found |= b_probably_pskip; // Early term -> 16x16 without luma residual ends the prediction
            }
        } // end-of-for(i)
    }// end-of-for(u)

    if (!BestPart) {
        HL_DEBUG_ERROR("No prediction done");
        return HL_ERROR_INVALID_STATE;
    }

    if (p_mb->u_addr == 77) {
        int a = 0; // FIXME
    }

#if 1
    // FIXME: doesn't work with SVC
    // FIXME: reuse homogeneity
    if (!b_pskip) {
        err = hl_codec_264_rdo_mb_guess_best_intra_pred_avc(p_mb, p_codec, pi_mad);
        if (last_best_intra_cost <= best_cost) {
            //HL_DEBUG_INFO("INTRA in....P SLICE");
            return HL_ERROR_SUCCESS;
        }
    }
#endif

    // Init current MB with the best part
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTER;
    p_mb->refIdxL0[0] = p_mb->refIdxL0[1] = p_mb->refIdxL0[2] = p_mb->refIdxL0[3] = p_mb->RefIdxL0[0] = p_mb->RefIdxL0[1] = p_mb->RefIdxL0[2] = p_mb->RefIdxL0[3] = 0; // TODO: "L1"
    p_mb->predFlagL0[0] = p_mb->predFlagL0[1] = p_mb->predFlagL0[2] = p_mb->predFlagL0[3] = p_mb->PredFlagL0[0] = p_mb->PredFlagL0[1] = p_mb->PredFlagL0[2] = p_mb->PredFlagL0[3] = 0; // TODO: "L1"
    p_mb->e_type = BestPartDef->mbType;
    p_mb->mb_type = (p_mb->e_type - HL_CODEC_264_MB_TYPE_START_SLICE_P_AND_SP - 1);
    p_mb->NumMbPart = BestPart->NumMbPart;
    p_mb->MbPartWidth = BestPart->MbPartWidth;
    p_mb->MbPartHeight = BestPart->MbPartHeight;
    for (mbPartIdx = 0; mbPartIdx < p_mb->NumMbPart; ++mbPartIdx) {
        p_mb->RefIdxL0[mbPartIdx] = p_mb->refIdxL0[mbPartIdx] = BestRefIdxL0[mbPartIdx][0];
        p_mb->PredFlagL0[mbPartIdx] = p_mb->predFlagL0[mbPartIdx] = 1;

        p_mb->MbPartPredMode[mbPartIdx] = HL_CODEC_264_MB_MODE_PRED_L0; // TODO: "B, Direct, BiPred"
        p_mb->SubMbPredType[mbPartIdx] = BestPart->SubMbPredType[mbPartIdx];
        p_mb->SubMbPredMode[mbPartIdx] = BestPart->SubMbPredMode[mbPartIdx];
        p_mb->NumSubMbPart[mbPartIdx] = BestPart->NumSubMbPart[mbPartIdx];

        p_mb->SubMbPartWidth[mbPartIdx] = BestPart->SubMbPartWidth[mbPartIdx];
        p_mb->SubMbPartHeight[mbPartIdx] = BestPart->SubMbPartHeight[mbPartIdx];

        p_mb->sub_mb_type[mbPartIdx] = (p_mb->SubMbPredType[mbPartIdx] - HL_CODEC_264_SUBMB_TYPE_START_MB_P - 1); // TODO: "B"

        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
            p_mb->mvL0[mbPartIdx][subMbPartIdx].x = BestMvL0[mbPartIdx][subMbPartIdx][0]; // TODO: "L1"
            p_mb->mvL0[mbPartIdx][subMbPartIdx].y = BestMvL0[mbPartIdx][subMbPartIdx][1]; // TODO: "L1"
            // 8.4.1.4 Derivation process for chroma motion vectors
            err = hl_codec_264_utils_derivation_process_for_chroma_movects(
                      p_codec,
                      p_mb,
                      &p_mb->mvL0[mbPartIdx][subMbPartIdx],
                      &p_mb->mvCL0[mbPartIdx][subMbPartIdx],
                      HL_CODEC_264_LIST_IDX_0); // TODO: "L1"
            if (err) {
                return err;
            }

            p_mb->partWidth[mbPartIdx][subMbPartIdx] = p_mb->SubMbPartWidth[mbPartIdx];
            p_mb->partHeight[mbPartIdx][subMbPartIdx] = p_mb->SubMbPartHeight[mbPartIdx];
            p_mb->partWidthC[mbPartIdx][subMbPartIdx] = p_mb->partWidth[mbPartIdx][subMbPartIdx] >> pc_sps->SubWidthC_TrailingZeros;
            p_mb->partHeightC[mbPartIdx][subMbPartIdx] = p_mb->partHeight[mbPartIdx][subMbPartIdx] >> pc_sps->SubHeightC_TrailingZeros;

            p_mb->mvd_l0[mbPartIdx][subMbPartIdx].x = p_mb->mvL0[mbPartIdx][subMbPartIdx].x - BestMVP[mbPartIdx][subMbPartIdx][0];// (8-174)
            p_mb->mvd_l0[mbPartIdx][subMbPartIdx].y = p_mb->mvL0[mbPartIdx][subMbPartIdx].y - BestMVP[mbPartIdx][subMbPartIdx][1];// (8-175)

            p_mb->MvL0[mbPartIdx][subMbPartIdx] = p_mb->mvL0[mbPartIdx][subMbPartIdx];
            p_mb->MvCL0[mbPartIdx][subMbPartIdx] = p_mb->mvCL0[mbPartIdx][subMbPartIdx];
        }
    }

    if (b_pskip) {
        // reconstruct luma
        // chroma already reconstructed when we called "_hl_codec_264_rdo_mb_is_zeros_inter16x16_chroma()" to check that all chroma residual values are zeros
        err = _hl_codec_264_rdo_mb_reconstruct_luma_pskip(p_mb, p_codec, &p_mb->mvL0[0][0]);
        if (err) {
            return err;
        }
        p_mb->e_type = HL_CODEC_264_MB_TYPE_P_SKIP;
        p_mb->flags_type |= HL_CODEC_264_MB_TYPE_FLAGS_SKIP;
        p_mb->mb_type = (p_mb->e_type - HL_CODEC_264_MB_TYPE_START_SLICE_P_AND_SP - 1);

        p_mb->coded_block_pattern = 0;
        p_mb->CodedBlockPatternChroma = 0;
        p_mb->CodedBlockPatternLuma4x4 = 0;
        p_mb->CodedBlockPatternLuma = 0;
    }
    else {
        // Reconstruct luma and chroma
        err = _hl_codec_264_rdo_mb_reconstruct_inter(p_mb, p_codec, best_Single_ctr);
        if (err) {
            return err;
        }

        // Guess CBP
        err = _hl_codec_264_rdo_mb_guess_cbp(p_mb, p_codec);
        if (err) {
            return err;
        }
    } // end-of-else(!p_skip)

#if 1
    // Check whether it's PSkip (ones we filed to predict)
    if (!HL_CODEC_264_MB_TYPE_IS_SKIP(p_mb) && p_mb->coded_block_pattern == 0 && HL_CODEC_264_MB_TYPE_IS_P_L0_16X16(p_mb) && p_mb->mvd_l0[0][0].x == 0 && p_mb->mvd_l0[0][0].y == 0) {
        hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(
            p_codec,
            p_mb,
            &mv_pskip,
            &refIdxL0);
        if (mv_pskip.x == BestMVP[0][0][0] && mv_pskip.y == BestMVP[0][0][1]) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_P_SKIP;
            p_mb->mb_type = (p_mb->e_type - HL_CODEC_264_MB_TYPE_START_SLICE_P_AND_SP - 1);
        }
    }
#endif

    // Set MAD used for RC (Rate Control)
    if (pi_mad) {
        *pi_mad = best_dist;
    }

    return err;
}

HL_ERROR_T hl_codec_264_rdo_mb_guess_best_inter_pred_svc(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec)
{
    HL_ERROR_T err;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_pixel_t *p_eSL, *p_cSL, *_p_eSL;
    int32_t x, y, xS, yS, xO, yO, xP, yP, luma4x4BlkIdx, isP8x8OrP8x8Ref0OrB8x8, mbPartIdx, subMbPartIdx, partWidth, partHeight, partWidthC, partHeightC;
    const hl_codec_264_pict_t *refPicLXL, *refPicLXCb, *refPicLXCr;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartL;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartCb;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartCr;
    HL_ALIGNED(16) hl_int32_16x16_t* predMbL;
    HL_ALIGNED(16) hl_int32_16x16_t* predMbCb;
    HL_ALIGNED(16) hl_int32_16x16_t* predMbCr;
    HL_ALIGNED(16) hl_int32_16x4x4_t* Residual16x4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4L;
    HL_ALIGNED(16) hl_int32_16x4x4_t* ACCoeff16x4x4L;
    hl_bool_t isAllZeros;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;
    static const hl_bool_t __isIntra16x16False = HL_FALSE;
    static const hl_bool_t __isIntraBlockTrue = HL_TRUE;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartL);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartCb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartCr);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predMbL);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predMbCb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predMbCr);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Residual16x4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff16x4x4L);

    p_eSL = (p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);
    p_cSL = (pc_layer->pc_fs_curr->p_pict->pc_data_y);

    if (p_mb->u_addr == 333) {
        int a = 0;
    }

    // Prediction generation is from "_hl_codec_264_decode_svc_inter()"

    // TODO: For now (beta version) we always reuse prediction from base layer. This is very bad.
    // We must perform another RDO cycle and compare with the one from base layer
    p_mb->ext.svc.base_mode_flag = 1;

    /* Set default values */
    p_mb->mb_type = HL_CODEC_264_SVC_MB_TYPE_INFERRED;
    p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_BL;
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INFERRED;
    p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTER_BL;
    p_mb->NumMbPart = 1;
    p_mb->MbPartWidth = p_mb->MbPartHeight = 16;
    p_mb->CodedBlockPatternLuma4x4 = 0;

    /* G.8.1.5.1 Macroblock initialisation process */
    err = hl_codec_264_utils_derivation_process_initialisation_svc(p_codec, p_mb);
    if (err) {
        goto bail;
    }

    // G.8.4.1 SVC derivation process for motion vector components and reference indices
    err = hl_codec_264_utils_derivation_process_for_mv_comps_and_ref_indices_svc(p_codec, p_mb);
    if (err) {
        return err;
    }

    isP8x8OrP8x8Ref0OrB8x8 = (HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb));

    for (mbPartIdx = 0; mbPartIdx < p_mb->NumMbPart; ++mbPartIdx) {
        // "6.4.2.1" -> InverseRasterScan_Pow2Full
        xP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);
        yP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);

        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
            if (isP8x8OrP8x8Ref0OrB8x8) {
                partWidth = p_mb->SubMbPartWidth[mbPartIdx]; // (G-103)
                partHeight = p_mb->SubMbPartHeight[mbPartIdx]; // (G-104)
            }
            else {
                partWidth = p_mb->MbPartWidth; // (G-101)
                partHeight = p_mb->MbPartHeight; // (G-102)
            }

            if (pc_sps->ChromaArrayType) {
                partWidthC = partWidth / pc_sps->SubWidthC; // (G-105)
                partHeightC = partHeight / pc_sps->SubHeightC; // (G-106): FIXME: standard says "partHeight / SubWidthC" but looks like an error

                if (p_mb->predFlagL0[mbPartIdx]) {
                    // 8.4.1.4 Derivation process for chroma motion vectors
                    err = hl_codec_264_utils_derivation_process_for_chroma_movects(p_codec, p_mb, &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx], HL_CODEC_264_LIST_IDX_0);
                    if (err) {
                        goto bail;
                    }
                }
                else if (p_mb->predFlagL1[mbPartIdx]) {
                    // 8.4.1.4 Derivation process for chroma motion vectors
                    err = hl_codec_264_utils_derivation_process_for_chroma_movects(p_codec, p_mb, &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx], HL_CODEC_264_LIST_IDX_1);
                    if (err) {
                        goto bail;
                    }
                }
            }

            // 6.4.2.2 - upper-left sample of the sub-macroblock partition
            hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);

            // Compute "xL_Idx" and "yL_Idx", required by "hl_codec_264_pred_inter_predict()"
            p_mb->xL_Idx = p_mb->xL + (xP + xS);
            p_mb->yL_Idx = p_mb->yL + (yP + yS);

            {
                /* 8.4.2 Decoding process for Inter prediction samples */

                // Get reference pictures for the partition (ignore sub-parts)
                if (subMbPartIdx == 0 /*&& (p_mb->predFlagL0[mbPartIdx] || p_mb->predFlagL1[mbPartIdx])*/) {
                    // 8.4.2.1 Reference picture selection process
                    err = hl_codec_264_pred_inter_select_refpic(p_codec, p_mb,
                            p_mb->predFlagL1[mbPartIdx] == 1 ? pc_layer->pobj_poc->RefPicList1 : pc_layer->pobj_poc->RefPicList0,
                            p_mb->predFlagL1[mbPartIdx] == 1 ? p_mb->refIdxL1[mbPartIdx] : p_mb->refIdxL0[mbPartIdx],
                            &refPicLXL, &refPicLXCb, &refPicLXCr);
                    if (err) {
                        goto bail;
                    }
                }

                if(p_mb->predFlagL0[mbPartIdx]) {
                    // 8.4.2.2 Fractional sample interpolation process
                    err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                          &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx],
                                                          refPicLXL, refPicLXCb, refPicLXCr,
                                                          (*predPartL), (*predPartCb), (*predPartCr));
                    if (err) {
                        goto bail;
                    }
                }
                else if(p_mb->predFlagL1[mbPartIdx]) {
                    // 8.4.2.2 Fractional sample interpolation process
                    err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                          &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx],
                                                          refPicLXL, refPicLXCb, refPicLXCr,
                                                          (*predPartL), (*predPartCb), (*predPartCr));
                    if (err) {
                        goto bail;
                    }
                }

            } // end-of-8.4.2

            for (y = 0; y < partHeight; ++y) {
                for (x = 0; x < partWidth; ++x) {
                    (*predMbL)[yP + yS + y][xP + xS + x] = (*predPartL)[y][x]; // (G-107)
                }
            }

            if (pc_sps->ChromaArrayType) {
                for (y = 0; y < partHeightC; ++y) {
                    for (x = 0; x < partWidthC; ++x) {
                        (*predMbCb)[( yP + yS ) / pc_sps->SubHeightC + y][( xP + xS ) / pc_sps->SubWidthC + x] = (*predPartCb)[y][x]; // (G-108)
                        (*predMbCr)[( yP + yS ) / pc_sps->SubHeightC + y][( xP + xS ) / pc_sps->SubWidthC + x] = (*predPartCr)[y][x]; // (G-109)
                    }
                }
            }
        } // end-of-for(subMbPartIdx)
    } // end-of-for (mbPartIdx)

    // FIXME: split I_4x4, I16x16

    // Build Luma coeffs and reconstruct for prediction
    for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
        xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
        yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
        // Move samples pointer to the right 4x4 block
        _p_eSL = p_eSL + (yO * pc_slice_header->PicWidthInSamplesL) + xO;
        // Get residual
        hl_math_sub4x4_u8x32(
            _p_eSL, pc_slice_header->PicWidthInSamplesL,
            (const int32_t*)&(*predMbL)[yO][xO], 16,
            (int32_t*)(*Residual16x4x4L)[luma4x4BlkIdx], 4);
        isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*Residual16x4x4L)[luma4x4BlkIdx]);
        if (isAllZeros) {
            hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0);
        }
        else {
            // Forward transform Cf
            hl_codec_264_transf_frw_residual4x4((*Residual16x4x4L)[luma4x4BlkIdx], (*tmp4x4L));
            // Scaling and quantization
            hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockTrue, (*tmp4x4L), (*ACCoeff16x4x4L)[luma4x4BlkIdx]);
            isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*ACCoeff16x4x4L)[luma4x4BlkIdx]);
            if (isAllZeros) {
                hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0);
            }
            else {
                // ZigZag4x4()
                Scan4x4_L((*ACCoeff16x4x4L)[luma4x4BlkIdx], p_mb->LumaLevel[luma4x4BlkIdx], __isIntra16x16ACFalse);
                p_mb->CodedBlockPatternLuma4x4 |= (1 << luma4x4BlkIdx);
            }
        }

        // Reconstruct Luma
        if ((p_mb->CodedBlockPatternLuma4x4 & (1 << luma4x4BlkIdx))) {
            // 8.5.12 Scaling and transformation process for residual 4x4 blocks
            hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*ACCoeff16x4x4L)[luma4x4BlkIdx], (*Residual16x4x4L)[luma4x4BlkIdx], __isLumaTrue, __isIntra16x16False, -1);
            // Add predicted samples to residual and clip the result
            hl_math_addclip_4x4((const int32_t*)&(*predMbL)[yO][xO], 16, (const int32_t*)(*Residual16x4x4L)[luma4x4BlkIdx], 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4L), 4);
            // 8.5.14 Picture construction process prior to deblocking filter process
            hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*tmp4x4L), 4, luma4x4BlkIdx);
        }
        else {
            // No residual means "PRED" samples = "reconstructed" samples
            // 8.5.14 Picture construction process prior to deblocking filter process
            hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)&(*predMbL)[yO][xO], 16, luma4x4BlkIdx);
        }
    }

    // Buil chroma coeffs and reconstruct
    err = _hl_codec_264_rdo_mb_reconstruct_chroma(p_mb, p_codec, (*predMbCb), (*predMbCr));
    if (err) {
        goto bail;
    }

    // Guess CBP
    err = _hl_codec_264_rdo_mb_guess_cbp(p_mb, p_codec);
    if (err) {
        goto bail;
    }

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartL);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartCb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartCr);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predMbL);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predMbCb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predMbCr);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Residual16x4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff16x4x4L);

    return err;
}


// This function will NOT reconstruct 16x16 samples
// This function will sets: "Intra16x16PredMode", "Intra16x16DCLevel", "Intra16x16ACLevel"
static HL_ERROR_T _hl_codec_264_rdo_mb_guess_best_intra16x16_pred(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    double d_lambda,
    HL_OUT int32_t* CodedBlockPatternLuma4x4,
    HL_OUT int32_t *best_dist,
    HL_OUT double *best_cost
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_residual_write_block_f f_write_block;
    HL_ALIGNED(16) hl_int32_33_t* p33;
    HL_ALIGNED(16) hl_int32_4x4_t* residual4x4L;
    HL_ALIGNED(16) hl_int32_16x16_t* pred16x16L;
    HL_ALIGNED(16) hl_int32_16_t* Intra16x16DCLevel;
    HL_ALIGNED(16) hl_int32_16x16_t* Intra16x16ACLevel;
    HL_ALIGNED(16) hl_int32_4x4_t* ACCoeff4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* DCCoeff4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4L;
    HL_ALIGNED(16) hl_uint8_4x4_t* tmp4x4L_u8;
    HL_CODEC_264_I16x16_MODE_T mode;
    double d_dist, d_rate, d_cost;
    int32_t luma4x4BlkIdx, xO, yO, BestCodedBlockPatternLuma4x4, Single_ctr;
    hl_bool_t isAllZeros;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};
    const hl_pixel_t *p_cSL, *p_eSL, *_p_eSL;
    static const hl_bool_t __isIntraBlockTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16True = HL_TRUE;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    if (!pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    if (p_mb->u_addr == 2) {
        int kaka = 0;// FIXME
    }

    // Set default values
    *best_cost = DBL_MAX;
    *CodedBlockPatternLuma4x4 = 0;
    residual_inv_type.b_rdo = HL_TRUE;
    p_cSL = pc_layer->pc_fs_curr->p_pict->pc_data_y;
    p_eSL = (p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);

    // Hack the macroblock to make sure all functions will return right values when IS_INTRA(), IS_16x16()... are called
    p_mb->e_type = HL_CODEC_264_MB_TYPE_I_16X16_0_0_0;
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16;
    p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_16X16;
    p_mb->Intra16x16PredMode = Intra_16x16_DC;

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &p33);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L_u8);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &residual4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &pred16x16L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Intra16x16DCLevel);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &Intra16x16ACLevel);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &DCCoeff4x4L);

    // Get neighbouring_samples
    err = hl_codec_264_pred_intra_get_neighbouring_samples_16x16L(p_codec, p_mb, p_cSL, *p33);
    if (err) {
        goto bail;
    }

    for (mode = 0; mode < HL_CODEC_264_I16x16_MODE_MAX_COUNT; ++mode) {
        switch (mode) {
        case Intra_16x16_Vertical: {
            if((*p33)[17] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                continue;
            }
            break;
        }
        case Intra_16x16_Horizontal: {
            if((*p33)[1] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                continue;
            }
            break;
        }
        case Intra_16x16_Plane: {
            if((*p33)[0] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                continue;
            }
            break;
        }
        } // end-of-switch

        d_dist = 0;
        BestCodedBlockPatternLuma4x4 = 0;
        Single_ctr = 0;

        // Reset RDO bits
        err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
        if (err) {
            goto bail;
        }

        // Get PRED samples
        hl_codec_264_pred_intra_perform_prediction_16x16L(p_codec, p_mb, *pred16x16L, *p33, mode);

        /* Intra16x16 AC coeffs */
        residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL;

        // For each (4x4 block) in scan block order
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
            xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
            yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

            _p_eSL = p_eSL + xO + (yO * pc_slice_header->PicWidthInSamplesL);

            // Compute residual
            hl_math_sub4x4_u8x32(
                _p_eSL, pc_slice_header->PicWidthInSamplesL,
                (const int32_t*)&(*pred16x16L)[yO][xO], 16,
                (int32_t*)(*residual4x4L), 4);
            // FIXME: do nothing when all residual values are zeros

            //== Intra16x16ACLevel ==//
            // Forward transform Cf
            hl_codec_264_transf_frw_residual4x4((*residual4x4L), (*tmp4x4L));
            // Scaling and quantization
            hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockTrue, (*tmp4x4L), (*ACCoeff4x4L));
            // ZigZag4x4()
            Scan4x4_L((*ACCoeff4x4L), (*Intra16x16ACLevel)[luma4x4BlkIdx], __isIntra16x16ACTrue);
            (*Intra16x16ACLevel)[luma4x4BlkIdx][15] = 0;

            // Write AC coeffs of the current 4x4 luma block to the residual
            isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*ACCoeff4x4L)[0][0]/*&(*Intra16x16ACLevel)[luma4x4BlkIdx]*/); // Not "Intra16x16ACLevel" but "ACCoeff4x4L" with DC corff.
            if (!isAllZeros) {
                residual_inv_type.i_luma4x4BlkIdx = luma4x4BlkIdx;
                err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*Intra16x16ACLevel)[luma4x4BlkIdx], 0, 15, 16);
                if (err) {
                    goto bail;
                }
                BestCodedBlockPatternLuma4x4 |= (1 << luma4x4BlkIdx);
                Single_ctr += pc_esd->rdo.Single_ctr;
            }
            // DC coeffs
            (*DCCoeff4x4L)[yO >> 2][xO >> 2] = (*tmp4x4L)[0][0];
        } // end-of-for(luma4x4BlkIdx)

        // JVT - O079 - 2.3 Elimination of single coefficients in inter macroblocks
        if (BestCodedBlockPatternLuma4x4 && Single_ctr < 6) {
            // HL_DEBUG_TALKATIVE("I16x16(%u) - Single_ctr=%d", p_mb->u_addr, Single_ctr);
            BestCodedBlockPatternLuma4x4 = 0;
        }

        /* Reconstruct and compute distorsion */

        if (BestCodedBlockPatternLuma4x4) {
            hl_int32_4x4_t* c;
            hl_int32_4x4_t* dcY; // must be zeros
            hl_int32_16x16_t* rMb; // must be zeros
            hl_int32_16x16_t* u;
            hl_int32_4x4_t* r;

            int32_t luma4x4BlkIdx, xO, yO/*, i*/;
            hl_int32_16_t* lumaList;

            /* Intra16x16 4x4 DC Hadamard transform + quantize DC coeffs */
            hl_codec_264_transf_frw_hadamard4x4_dc_luma((*DCCoeff4x4L), (*tmp4x4L)); // Hadamard transform
            hl_codec_264_quant_frw4x4_scale_dc_luma(p_mb->QPy, __isIntraBlockTrue, (*tmp4x4L), (*DCCoeff4x4L)); // Post-scaling and quantization
            Scan4x4_L((*DCCoeff4x4L), (*Intra16x16DCLevel), __isIntra16x16ACFalse);

            // Write "Intra16x16DCLevel" coeffs. "Intra16x16ACLevel" already done.
            residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL;
            err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*Intra16x16DCLevel), 0, 15, 16);
            if (err) {
                goto bail;
            }

            // map() blocks
            hl_memory_blocks_map(pc_esd->pc_mem_blocks, &c);
            hl_memory_blocks_map_4x4zeros(pc_esd->pc_mem_blocks, &dcY); // must be zeros
            hl_memory_blocks_map_16x16zeros(pc_esd->pc_mem_blocks, &rMb); // must be zeros
            hl_memory_blocks_map(pc_esd->pc_mem_blocks, &u);
            hl_memory_blocks_map(pc_esd->pc_mem_blocks, &r);
            hl_memory_blocks_map(pc_esd->pc_mem_blocks, &lumaList);

            // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
            InverseScan4x4((*Intra16x16DCLevel), (*c));
            // 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
            hl_codec_264_transf_scale_luma_dc_coeff_intra16x16(p_codec, p_mb, p_mb->QPyprime, p_codec->sps.pc_active->BitDepthY, (*c), (*dcY));

            for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
                static const int32_t __count = 15 * sizeof(int32_t);
                (*lumaList)[0] = (*dcY)[dcYij[luma4x4BlkIdx][0]][dcYij[luma4x4BlkIdx][1]];
                memcpy(&(*lumaList)[1], &(*Intra16x16ACLevel)[luma4x4BlkIdx][0], __count); // FIXME: use hl_memory_copy()
                // 6.4.3 Inverse 4x4 luma block scanning process
                xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
                yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

                if (!hl_math_allzero16(lumaList)) { // Intra16x16ACLevel 4x4 CBP never saved
                    // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                    InverseScan4x4((*lumaList), (*c));
                    // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                    hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (*c), (*r), __isLumaTrue, __isIntra16x16True, -1);
                    // Copy 4x4 residual block
                    hl_memory_copy4x4(&(*rMb)[yO][xO], 16, (const int32_t *)(*r), 4);
                }//if (!AllZero)
            }//for (each blk4x4)

            // Add predicted data to the residual and clip the result
            hl_math_addclip_16x16((int32_t*)(*pred16x16L), 16, (const int32_t*)(*rMb), 16, p_codec->PixelMaxValueY, (int32_t*)(*u), 16);

            // Compute distortion
            for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
                xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
                yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
                _p_eSL = p_eSL + xO + (yO * pc_slice_header->PicWidthInSamplesL);
                hl_memory_copy4x4_u32_to_u8((uint8_t*)(*tmp4x4L_u8), 4, (const int32_t*)&(*u)[yO][xO], 16);
                d_dist += hl_math_sad4x4_u8(
                              _p_eSL, pc_slice_header->PicWidthInSamplesL,
                              (const uint8_t*)(*tmp4x4L_u8), 4);
            }

            // unmap() memory blocks
            hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, c);
            hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, dcY);
            hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, rMb);
            hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, u);
            hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, r);
            hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, lumaList);
        }
        else {
            hl_memory_set((*Intra16x16DCLevel), 16, 0);
            for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
                xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
                yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
                _p_eSL = p_eSL + xO + (yO * pc_slice_header->PicWidthInSamplesL);
                hl_memory_copy4x4_u32_to_u8((uint8_t*)(*tmp4x4L_u8), 4, (const int32_t*)&(*pred16x16L)[yO][xO], 16);
                d_dist += hl_math_sad4x4_u8(
                              _p_eSL, pc_slice_header->PicWidthInSamplesL,
                              (const uint8_t*)(*tmp4x4L_u8), 4);
            }
        }

        // compute cost
        d_rate = (double)hl_codec_264_bits_get_stream_index(pc_esd->rdo.pobj_bits);
        d_cost = (d_dist + (d_lambda * d_rate) /*+ activity*/);
        if (d_cost < *best_cost) {
            *best_cost = d_cost;
            *best_dist = (int32_t)d_dist;
            *CodedBlockPatternLuma4x4 = BestCodedBlockPatternLuma4x4;
            p_mb->Intra16x16PredMode = mode;
            hl_memory_copy16x16(((int32_t*)p_mb->Intra16x16ACLevel), ((const int32_t*)(*Intra16x16ACLevel)));
            hl_memory_copy16(((int32_t*)p_mb->Intra16x16DCLevel), ((const int32_t*)(*Intra16x16DCLevel))); // FIXME: use array[max_mode][16] and set best_mode_idx=?
        }
    } // end-of for(mode)

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, p33);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L_u8);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, residual4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, pred16x16L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Intra16x16DCLevel);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, Intra16x16ACLevel);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, DCCoeff4x4L);

    return err;
}

// This function will reconstruct 4x4 samples
// This function will sets: "CodedBlockPatternLuma4x4", "Intra4x4PredMode", "LumaLevel"
static HL_ERROR_T _hl_codec_264_rdo_mb_guess_best_intra4x4_pred(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    double d_lambda,
    HL_OUT int32_t* CodedBlockPatternLuma4x4,
    HL_OUT int32_t *best_dist,
    HL_OUT double *best_cost
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_residual_write_block_f f_write_block;
    HL_ALIGNED(16) hl_int32_13_t* p13;
    HL_ALIGNED(16) hl_int32_4x4_t* pred4x4L;
    HL_ALIGNED(16) hl_int32_16x4x4_t* BestPred16x4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* residual4x4L;
    HL_ALIGNED(16) hl_int32_16x16_t* LumaLevel;
    HL_ALIGNED(16) hl_int32_4x4_t* ACCoeff4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* DCCoeff4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4L;
    HL_ALIGNED(16) hl_uint8_4x4_t* tmp4x4L_u8;
    HL_CODEC_264_I4x4_MODE_T mode;
    double d_dist, d_min_dist4x4, d_rate, d_min_cost4x4, d_cost;
    int32_t luma4x4BlkIdx, xO, yO, Single_ctr, BestSingle_ctr;
    hl_bool_t isAllZeros, b_best_all_zeros;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};
    const hl_pixel_t *p_eSL, *p_cSL, *_p_eSL;
    static const hl_bool_t __isIntraBlockTrue = HL_TRUE;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16False = HL_FALSE;
    static const hl_bool_t __isIntra16x16ACTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    if (!pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // Set default values
    *best_cost = 0;
    *best_dist = 0;
    *CodedBlockPatternLuma4x4 = 0;
    Single_ctr = 0;
    residual_inv_type.b_rdo = HL_TRUE;
    residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;
    p_eSL = (p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);
    p_cSL = (pc_layer->pc_fs_curr->p_pict->pc_data_y);

    // Hack the macroblock to make sure all functions will return right values when IS_INTRA(), IS_I4x4()... are called
    p_mb->e_type = HL_CODEC_264_MB_TYPE_I_NXN;
    p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4;
    p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_4X4;

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &p13);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &pred4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &BestPred16x4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L_u8);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &residual4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &LumaLevel);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &DCCoeff4x4L);



    // For each (4x4 block) in scan block order
    for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
        d_min_cost4x4 = DBL_MAX;
        p_mb->Intra4x4PredMode[luma4x4BlkIdx] = Intra_4x4_DC;
        residual_inv_type.i_luma4x4BlkIdx = luma4x4BlkIdx;

        // Get neighbouring 4x4 samples for prediction
        err = hl_codec_264_pred_intra_get_neighbouring_samples_4x4L(p_codec, p_mb, luma4x4BlkIdx, &xO, &yO, p_cSL, (*p13));
        if (err) {
            goto bail;
        }

        // Move samples pointer to the right 4x4 block
        _p_eSL = p_eSL + (yO * pc_slice_header->PicWidthInSamplesL) + xO;

        for (mode = 0; mode < HL_CODEC_264_I4x4_MODE_MAX_COUNT; ++mode) {
            switch(mode) {
            case Intra_4x4_Vertical:
            case Intra_4x4_Diagonal_Down_Left:
            case Intra_4x4_Vertical_Left: {
                if ((*p13)[5] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                    continue;
                }
                break;
            }
            case Intra_4x4_Horizontal:
            case Intra_4x4_Horizontal_Up: {
                if ((*p13)[1] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                    continue;
                }
                break;
            }
            case Intra_4x4_Diagonal_Down_Right:
            case Intra_4x4_Vertical_Right:
            case Intra_4x4_Horizontal_Down: {
                if ((*p13)[0] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                    continue;
                }
                break;
            }
            }// end-of-switch

            // FIXME:
            if (p_mb->u_addr == 0 && luma4x4BlkIdx == 11 /*&& mode == Intra_4x4_DC*/) {
                int kaka = 0;
            }

            // Get PRED samples
            hl_codec_264_pred_intra_perform_prediction_4x4L(p_codec, p_mb, (*pred4x4L), (*p13), mode);

            // Get residual
            hl_math_sub4x4_u8x32(
                _p_eSL, pc_slice_header->PicWidthInSamplesL,
                (const int32_t*)(*pred4x4L), 4,
                (int32_t*)(*residual4x4L), 4);
            isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*residual4x4L)); // is residual zero?

            if (isAllZeros) {
                // no residual values (exact match) -> break
                d_min_cost4x4 = 0;
                d_min_dist4x4 = 0;
                p_mb->Intra4x4PredMode[luma4x4BlkIdx] = mode;
                b_best_all_zeros = HL_TRUE;
                hl_memory_copy4x4((int32_t*)(*BestPred16x4x4L)[luma4x4BlkIdx], 4, (const int32_t*)(*pred4x4L), 4);
                hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0); // FIXME: LumaLevel (residual) operates on 8x8 blocks
                break;
            }
            else {
                // Reset RDO bits
                err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
                if (err) {
                    goto bail;
                }
                // Compute SSD (Sum of Squared Differences)
                // d_dist = hl_math_sad4x4_u8x32(
                //             _p_eSL, pc_slice_header->PicWidthInSamplesL,
                //             (const int32_t*)(*pred4x4L), 4);

                //== LumaLevel ==//
                // Forward transform Cf
                hl_codec_264_transf_frw_residual4x4((*residual4x4L), (*tmp4x4L));
                // Scaling and quantization
                hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockTrue, (*tmp4x4L), (*ACCoeff4x4L));
                // ZigZag4x4()
                Scan4x4_L((*ACCoeff4x4L), (*LumaLevel)[luma4x4BlkIdx], __isIntra16x16ACFalse);

                // Write Luma coeffs of the current 4x4 luma block to the residual
                isAllZeros = hl_math_allzero16(&(*LumaLevel)[luma4x4BlkIdx]);
                if (!isAllZeros) {
                    err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, (*LumaLevel)[luma4x4BlkIdx], 0, 15, 16);
                    if (err) {
                        goto bail;
                    }

                    // Residual exist means decode samples and reconstruct
                    // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                    InverseScan4x4((*LumaLevel)[luma4x4BlkIdx], (*tmp4x4L));
                    // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                    hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*tmp4x4L), (*residual4x4L), __isLumaTrue, __isIntra16x16False, -1);
                    // Add predicted samples to residual and clip the result
                    hl_math_addclip_4x4((const int32_t*)(*pred4x4L), 4, (const int32_t*)(*residual4x4L), 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4L), 4);

                    hl_memory_copy4x4_u32_to_u8_stride4x4((uint8_t*)(*tmp4x4L_u8), (const uint32_t *)(*tmp4x4L));
                    d_dist = hl_math_sad4x4_u8(
                                 _p_eSL, pc_slice_header->PicWidthInSamplesL,
                                 (const uint8_t*)(*tmp4x4L_u8), 4);
                }
                else {
                    hl_memory_copy4x4_u32_to_u8_stride4x4((uint8_t*)(*tmp4x4L_u8), (const uint32_t *)(*pred4x4L));
                    d_dist = hl_math_sad4x4_u8(
                                 _p_eSL, pc_slice_header->PicWidthInSamplesL,
                                 (const uint8_t*)(*tmp4x4L_u8), 4);
                }
                // compute cost
                d_rate = (double)hl_codec_264_bits_get_stream_index(pc_esd->rdo.pobj_bits);
                d_cost = (d_dist + (d_lambda * d_rate) /*+ activity*/);
            }
            if (d_cost < d_min_cost4x4) {
                d_min_cost4x4 = d_cost;
                d_min_dist4x4 = d_dist;
                p_mb->Intra4x4PredMode[luma4x4BlkIdx] = mode;
                b_best_all_zeros = isAllZeros;
                BestSingle_ctr = isAllZeros ? 0 : pc_esd->rdo.Single_ctr;
                hl_memory_copy4x4((int32_t*)(*BestPred16x4x4L)[luma4x4BlkIdx], 4, (const int32_t*)(*pred4x4L), 4);
                // if (!isAllZeros) { // FIXME: LumaLevel (residual) operates on 8x8 blocks
                hl_memory_copy16(p_mb->LumaLevel[luma4x4BlkIdx], (*LumaLevel)[luma4x4BlkIdx]);
                // }
            }
        } // end-of-for(mode)

        *best_cost = *best_cost + d_min_cost4x4;
        *best_dist = (int32_t)(*best_dist + d_min_dist4x4);
        if (!b_best_all_zeros) {
            (*CodedBlockPatternLuma4x4) |= (1 << luma4x4BlkIdx);
            Single_ctr += BestSingle_ctr;
        }

        /* Reconstruct to allow doing next 4x4 prediction using reconstructed samples */
        {
            if (((*CodedBlockPatternLuma4x4) & (1 << luma4x4BlkIdx))) {
                // Residual exist means decode samples and reconstruct
                // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                InverseScan4x4(p_mb->LumaLevel[luma4x4BlkIdx], (*tmp4x4L));
                // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*tmp4x4L), (*residual4x4L), __isLumaTrue, __isIntra16x16False, -1);

                if (p_mb->TransformBypassModeFlag == 1 && (p_mb->Intra4x4PredMode[luma4x4BlkIdx] == Intra_4x4_Vertical || p_mb->Intra4x4PredMode[luma4x4BlkIdx] == Intra_4x4_Horizontal)) {
                    // 8.5.15 Intra residual transform-bypass decoding process
                    hl_codec_264_transf_bypass_intra_residual(4, 4, (int32_t)p_mb->Intra4x4PredMode[luma4x4BlkIdx], (*residual4x4L));
                }
                // Add predicted samples to residual and clip the result
                hl_math_addclip_4x4((const int32_t*)(*BestPred16x4x4L)[luma4x4BlkIdx], 4, (const int32_t*)(*residual4x4L), 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4L), 4);
                // 8.5.14 Picture construction process prior to deblocking filter process
                hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*tmp4x4L), 4, luma4x4BlkIdx);
            }
            else {
                // No residual means "PRED" samples = "reconstructed" samples
                // 8.5.14 Picture construction process prior to deblocking filter process
                hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*BestPred16x4x4L)[luma4x4BlkIdx], 4, luma4x4BlkIdx);
            }
        } // end-of-reconstruct

    } // end-of-for(luma4x4BlkIdx)

    // JVT - O079 - 2.3 Elimination of single coefficients in inter macroblocks
#if 0 // Code not correct because "4x4" at "N" was predicted using samples at "N-1" (which could be reconstructed using the useless single coeff)
    if ((Single_ctr < 6 && *CodedBlockPatternLuma4x4)) {
        // HL_DEBUG_TALKATIVE("I4x4(%u) - Single_ctr=%d", p_mb->u_addr, Single_ctr);
        // Reconstruct blocks with single coeff without residual
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
            if (((*CodedBlockPatternLuma4x4) & (1 << luma4x4BlkIdx))) {
                hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*BestPred16x4x4L)[luma4x4BlkIdx], 4, luma4x4BlkIdx);
            }
        }
        *CodedBlockPatternLuma4x4 = 0;
        // *best_cost = *best_dist; // Do not remove rate because distorsion computed with the single coeffs
    }
#elif 0 // Quality is worst
    if ((Single_ctr < 6 && *CodedBlockPatternLuma4x4)) {
        *best_cost = 0;
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
            err = hl_codec_264_pred_intra_get_neighbouring_samples_4x4L(p_codec, p_mb, luma4x4BlkIdx, &xO, &yO, p_cSL, (*p13));
            hl_codec_264_pred_intra_perform_prediction_4x4L(p_codec, p_mb, (*pred4x4L), (*p13), p_mb->Intra4x4PredMode[luma4x4BlkIdx]);
            hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*pred4x4L), 4, luma4x4BlkIdx);
            _p_eSL = p_eSL + (yO * pc_slice_header->PicWidthInSamplesL) + xO;
            (*best_cost) += hl_math_sad4x4_u8(
                                _p_eSL, pc_slice_header->PicWidthInSamplesL,
                                (const uint8_t*)(*pred4x4L), 4);
        }
        *CodedBlockPatternLuma4x4 = 0;
    }
#endif

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, p13);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, pred4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, BestPred16x4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L_u8);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, residual4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, LumaLevel);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, DCCoeff4x4L);

    return err;
}

static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_intra16x16_luma(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_layer_t* pc_layer;
    //const hl_codec_264_nal_pps_t* pc_pps;
    //const hl_codec_264_nal_sps_t* pc_sps;
    //const hl_codec_264_nal_slice_header_t* pc_slice_header;
    HL_ALIGNED(16) hl_int32_33_t *p33;
    HL_ALIGNED(16) hl_int32_16x16_t* predL;


    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &p33);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predL);

    // Gets the 33 neighbouring Samples
    err = hl_codec_264_pred_intra_get_neighbouring_samples_16x16L(p_codec, p_mb, p_codec->layers.pc_active->pc_fs_curr->p_pict->pc_data_y, (*p33));
    if (err) {
        goto bail;
    }
    // Performs Intra16x16 prediction
    hl_codec_264_pred_intra_perform_prediction_16x16L(p_codec, p_mb, (*predL), (*p33), p_mb->Intra16x16PredMode);
    // 8.5.2 Specification of transform decoding process for luma samples of Intra_16x16 macroblock prediction mode
    hl_codec_264_transf_decode_intra16x16_luma(p_codec, p_mb, (*predL));

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, p33);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predL);

    return err;
}

// Check that chroma residual are zeros
// Also reconstructs chroma
static HL_ERROR_T _hl_codec_264_rdo_mb_is_zeros_inter16x16_chroma(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    int32_t refIdxL0,
    const hl_codec_264_mv_xt* p_mvLX,
    hl_bool_t *pb_zeros)
{
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_dpb_fs_t* pc_fs;
    const hl_codec_264_pict_t* refPicLXCb;
    const hl_codec_264_pict_t* refPicLXCr;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_mv_xt mvCX;
    static int32_t __mbPartIdx = 0;
    static int32_t __subMbPartIdx = 0;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartLXCb;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartLXCr;

    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartLXCb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartLXCr);

    pc_fs = p_codec->pc_poc->RefPicList0[refIdxL0];
    if (!pc_fs || !pc_fs->p_pict || !HL_CODEC_264_REF_TYPE_IS_USED(pc_fs->RefType)) {
        HL_DEBUG_ERROR("No valid frame store at position %d", refIdxL0);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    refPicLXCb = refPicLXCr = pc_fs->p_pict;

    // 8.4.1.4 Derivation process for chroma motion vectors
    err = hl_codec_264_utils_derivation_process_for_chroma_movects(
              p_codec,
              p_mb,
              p_mvLX,
              &mvCX,
              HL_CODEC_264_LIST_IDX_0); // TODO: "L1"
    if (err) {
        goto bail;
    }

    err = hl_codec_264_interpol_chroma(
              p_codec,
              p_mb,
              __mbPartIdx,
              __subMbPartIdx,
              &mvCX,
              p_mvLX,
              refPicLXCb,
              refPicLXCr,
              (*predPartLXCb),
              (*predPartLXCr));
    if (err) {
        goto bail;
    }

    /* Get chroma levels and reconstruct */
    err = _hl_codec_264_rdo_mb_reconstruct_chroma(p_mb, p_codec, (*predPartLXCb), (*predPartLXCr));
    if (err) {
        goto bail;
    }

    *pb_zeros = (!p_mb->CodedBlockPatternChromaAC4x4[0] &&
                 !p_mb->CodedBlockPatternChromaAC4x4[1] &&
                 !p_mb->CodedBlockPatternChromaDC4x4[0] &&
                 !p_mb->CodedBlockPatternChromaDC4x4[1]);

    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartLXCb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartLXCr);

bail:
    return err;
}

static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_luma_pskip(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    const hl_codec_264_mv_xt* pc_mvPSkip
)
{
    hl_codec_264_dpb_fs_t* pc_fs;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_codec_264_pict_t* refPicL;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t luma4x4BlkIdx, xO, yO;
    static int32_t __mbPartIdx = 0;
    static int32_t __subMbPartIdx = 0;
    static int32_t __refIdxL0 = 0;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartL;

    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartL);

    pc_fs = p_codec->pc_poc->RefPicList0[__refIdxL0];
    if (!pc_fs || !pc_fs->p_pict || !HL_CODEC_264_REF_TYPE_IS_USED(pc_fs->RefType)) {
        HL_DEBUG_ERROR("No valid frame store at position %d", __refIdxL0);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    refPicL = pc_fs->p_pict;

    err = hl_codec_264_interpol_luma(
              p_codec,
              p_mb,
              __mbPartIdx,
              __subMbPartIdx,
              pc_mvPSkip,
              refPicL->pc_data_y,
              (*predPartL), sizeof((*predPartL)[0][0]));
    if (err) {
        goto bail;
    }

    for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
        xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
        yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
        hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)&(*predPartL)[yO][xO], 16, luma4x4BlkIdx);
    }

    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartL);

bail:
    return err;
}


// Reconstruct both luma and chroma
// Generate Luma coeffs
static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_inter(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    int32_t Single_ctr_luma
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    int32_t /*chroma4x4BlkIdx, chroma4x4BlkCount, */xO, yO, luma4x4BlkIdx, xP, yP, xS, yS, _x, _y, x, y, mbPartIdx, subMbPartIdx;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartL;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartCb;
    HL_ALIGNED(16) hl_int32_16x16_t* predPartCr;
    HL_ALIGNED(16) hl_int32_16x16_t *predL;
    HL_ALIGNED(16) hl_int32_16x16_t *predCb;
    HL_ALIGNED(16) hl_int32_16x16_t *predCr;
    HL_ALIGNED(16) hl_int32_4x4_t *residual4x4;
    HL_ALIGNED(16) hl_int32_4x4_t *tmp4x4;
    HL_ALIGNED(16) hl_int32_4x4_t *ACCoeff4x4L;
    const hl_codec_264_pict_t* refPicLXL;
    const hl_codec_264_pict_t* refPicLXCb;
    const hl_codec_264_pict_t* refPicLXCr;
    void (*_hl_memory_copy4x4)(int32_t *p_dst, hl_size_t dst_stride, const int32_t *pc_src, hl_size_t src_stride);
    const hl_pixel_t *p_eSL, *_p_eSL;
    hl_bool_t b_all_zeros;
    static const hl_bool_t __isIntraBlockFalse = HL_FALSE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16False = HL_FALSE;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;
    p_eSL = (p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);

    // Reset MB values
    p_mb->CodedBlockPatternLuma4x4 = 0;

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartL);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartCb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predPartCr);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predL);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predCb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &predCr);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &residual4x4);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff4x4L);

    /* Build luma prediction samples (FIXME: should be done in ME module) */
    for (mbPartIdx = 0; mbPartIdx < p_mb->NumMbPart; ++mbPartIdx) {
        // 6.4.2.1 - upper-left sample of the macroblock partition
        xP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);// (6-11)
        yP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);// (6-12)
        for (subMbPartIdx = 0; subMbPartIdx < p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
            if (mbPartIdx == 0 && subMbPartIdx == 0) {
                xS = yS = 0;
                p_mb->xL_Idx = p_mb->xL;
                p_mb->yL_Idx = p_mb->yL;
            }
            else {
                // 6.4.2.2 - upper-left sample of the sub-macroblock partition
                hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);

                // compute "xL_Idx" and "yL_Idx", used by interpolation functions
                p_mb->xL_Idx = p_mb->xL + (xP + xS);
                p_mb->yL_Idx = p_mb->yL + (yP + yS);
            }

            // 8.4.2 Decoding process for Inter prediction samples
            if (p_mb->predFlagL0[mbPartIdx] == 1) {
                if (subMbPartIdx == 0) {
                    hl_codec_264_dpb_fs_t* pc_fs = p_codec->pc_poc->RefPicList0[p_mb->refIdxL0[mbPartIdx]];
                    if (!pc_fs || !pc_fs->p_pict || !HL_CODEC_264_REF_TYPE_IS_USED(pc_fs->RefType)) {
                        HL_DEBUG_ERROR("No valid frame store at position %d", p_mb->refIdxL0[mbPartIdx]);
                        err = HL_ERROR_INVALID_BITSTREAM;
                        goto bail;
                    }
                    refPicLXL = refPicLXCb = refPicLXCr = pc_fs->p_pict;
                }
                // 8.4.2.2 Fractional sample interpolation process
                err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                      &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx],
                                                      refPicLXL, refPicLXCb, refPicLXCr,
                                                      (*predPartL), (*predPartCb), (*predPartCr));
                if (err) {
                    goto bail;
                }
            }
            else if (p_mb->predFlagL1[mbPartIdx] == 1) {
                if (subMbPartIdx == 0) {
                    hl_codec_264_dpb_fs_t* pc_fs = p_codec->pc_poc->RefPicList1[p_mb->refIdxL1[mbPartIdx]];
                    if (!pc_fs || !pc_fs->p_pict || !HL_CODEC_264_REF_TYPE_IS_USED(pc_fs->RefType)) {
                        HL_DEBUG_ERROR("No valid frame store at position %d", p_mb->refIdxL1[mbPartIdx]);
                        err = HL_ERROR_INVALID_BITSTREAM;
                        goto bail;
                    }
                    refPicLXL = refPicLXCb = refPicLXCr = pc_fs->p_pict;
                }
                // 8.4.2.2 Fractional sample interpolation process
                err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                      &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx],
                                                      refPicLXL, refPicLXCb, refPicLXCr,
                                                      (*predPartL), (*predPartCb), (*predPartCr));
                if (err) {
                    goto bail;
                }
            }
            else {
                HL_DEBUG_ERROR("Invalid state");
                err = HL_ERROR_INVALID_STATE;
                goto bail;
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
        } // end-of-for(subMbPartIdx)
    } // end-of-for(mbPartIdx)

    if (Single_ctr_luma < 6) {
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
            xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
            yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];
            hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)&(*predL)[yO][xO], 16, luma4x4BlkIdx);
        }
    }
    else {
        /* Get luma levels and reconstruct */
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
            xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
            yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

            // Move samples pointer to the right 4x4 block
            _p_eSL = p_eSL + (yO * pc_slice_header->PicWidthInSamplesL) + xO;

            // Compute residual
            hl_math_sub4x4_u8x32(
                _p_eSL, pc_slice_header->PicWidthInSamplesL,
                (const int32_t*)&(*predL)[yO][xO], 16,
                (int32_t*)(*residual4x4), 4);
            b_all_zeros = hl_math_allzero16((const int32_t(*)[16])&(*residual4x4));

            if (!b_all_zeros) {
                //== LumaLevel ==//
                // Forward transform Cf
                hl_codec_264_transf_frw_residual4x4((*residual4x4), (*tmp4x4));
                // Scaling and quantization
                hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockFalse, (*tmp4x4), (*ACCoeff4x4L));
                b_all_zeros = hl_math_allzero16((const int32_t(*)[16])&(*ACCoeff4x4L));
                // ZigZag4x4()
                if (!b_all_zeros) {
                    Scan4x4_L((*ACCoeff4x4L), p_mb->LumaLevel[luma4x4BlkIdx], __isIntra16x16ACFalse);
                    p_mb->CodedBlockPatternLuma4x4 |= (1 << luma4x4BlkIdx);
                }
            }
            if(b_all_zeros) {
                hl_memory_set(p_mb->LumaLevel[luma4x4BlkIdx], 16, 0); // FIXME: LumaLevel (residual) operates on 8x8 blocks
            }

            /* Reconstruct to allow doing next 4x4 prediction using reconstructed samples */
            {
                if ((p_mb->CodedBlockPatternLuma4x4 & (1 << luma4x4BlkIdx))) {
                    // Residual exist means decode samples and reconstruct
                    // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                    InverseScan4x4(p_mb->LumaLevel[luma4x4BlkIdx], (*tmp4x4));
                    // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                    hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*tmp4x4), (*residual4x4), __isLumaTrue, __isIntra16x16False, -1);
                    // Add predicted samples to residual and clip the result
                    hl_math_addclip_4x4((const int32_t*)&(*predL)[yO][xO], 16, (const int32_t*)(*residual4x4), 4, p_codec->PixelMaxValueY, (int32_t*)(*tmp4x4), 4);
                    // 8.5.14 Picture construction process prior to deblocking filter process
                    hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*tmp4x4), 4, luma4x4BlkIdx);
                }
                else {
                    // No residual means "PRED" samples = "reconstructed" samples
                    // 8.5.14 Picture construction process prior to deblocking filter process
                    hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)&(*predL)[yO][xO], 16, luma4x4BlkIdx);
                }
            } // end-of-reconstruct
        } // end-of-for(luma4x4BlkIdx)
    }

    /* Get chroma levels and reconstruct */
    err = _hl_codec_264_rdo_mb_reconstruct_chroma(p_mb, p_codec, (*predCb), (*predCr));
    if (err) {
        goto bail;
    }

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartL);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartCb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predPartCr);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predL);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predCb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, predCr);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, residual4x4);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff4x4L);

    return err;
}

static HL_ERROR_T _hl_codec_264_rdo_mb_reconstruct_chroma(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec,
    int32_t predCb[16][16],
    int32_t predCr[16][16]
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    hl_codec_264_residual_write_block_f f_write_block;
    int32_t chroma4x4BlkIdx, chroma4x4BlkCount, xO, yO, Single_ctr[2] = { 0, 0 }, TotalCoeffs[2] = { 0, 0 };
    hl_bool_t isAllZeros, isIntra;
    HL_ALIGNED(16) hl_int32_4x4_t* residual4x4C;
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4C;
    HL_ALIGNED(16) hl_int32_4x4_t* ACCoeff4x4;
    HL_ALIGNED(16) hl_int32_4x4_t* DCCoeff4x4Cb;
    HL_ALIGNED(16) hl_int32_4x4_t* DCCoeff4x4Cr;
    const hl_pixel_t *p_eSU, *p_eSV, *_p_eSU, *_p_eSV, *p_cSU, *p_cSV;
    static const hl_bool_t __isIntraBlockTrue = HL_TRUE;
    hl_codec_264_residual_inv_xt residual_inv_type = {0};

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;
    chroma4x4BlkCount = (1 << (pc_sps->ChromaArrayType + 1));
    p_eSU = (p_codec->encoder.pc_frame->data_ptr[1]) + p_mb->xC + (p_mb->yC * pc_slice_header->PicWidthInSamplesC);
    p_eSV = (p_codec->encoder.pc_frame->data_ptr[2]) + p_mb->xC + (p_mb->yC * pc_slice_header->PicWidthInSamplesC);
    p_cSU = (pc_layer->pc_fs_curr->p_pict->pc_data_u);
    p_cSV = (pc_layer->pc_fs_curr->p_pict->pc_data_v);
    isIntra = HL_CODEC_264_MB_TYPE_IS_INTRA(p_mb);
    residual_inv_type.b_rdo = HL_TRUE;
    residual_inv_type.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL;

    if (!pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // FIXME
    if (p_mb->u_addr == 5) {
        int a = 0;
    }

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &residual4x4C);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4C);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff4x4);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &DCCoeff4x4Cb);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &DCCoeff4x4Cr);

    p_mb->CodedBlockPatternChromaAC4x4[0] = 0;
    p_mb->CodedBlockPatternChromaAC4x4[1] = 0;
    p_mb->CodedBlockPatternChromaDC4x4[0] = 0;
    p_mb->CodedBlockPatternChromaDC4x4[1] = 0;

    // Reset RDO bits
    err = hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
    if (err) {
        goto bail;
    }

    /* Chroma Residual forward transform + quantize AC coeffs */
    for (chroma4x4BlkIdx = 0; chroma4x4BlkIdx < chroma4x4BlkCount; ++chroma4x4BlkIdx) {
        xO = InverseRasterScan16_4x4[chroma4x4BlkIdx][pc_sps->MbWidthC][0];
        yO = InverseRasterScan16_4x4[chroma4x4BlkIdx][pc_sps->MbHeightC][1];

        residual_inv_type.i_cbr4x4BlkIdx = chroma4x4BlkIdx;

        //== CB (Chroma component "0") ==//
        _p_eSU = p_eSU + (yO * pc_slice_header->PicWidthInSamplesC) + xO;
        hl_math_sub4x4_u8x32(
            _p_eSU, pc_slice_header->PicWidthInSamplesC,
            (const int32_t*)&predCb[yO][xO], 16,
            (int32_t*)(*residual4x4C), 4);
        isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*residual4x4C));
        if (!isAllZeros) {
            hl_codec_264_transf_frw_residual4x4((*residual4x4C), (*tmp4x4C));
            hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPc[0], __isIntraBlockTrue, (*tmp4x4C), (*ACCoeff4x4));
            Scan4x4_AC_C((*ACCoeff4x4), p_mb->ChromaACLevel[0][chroma4x4BlkIdx]);
            isAllZeros = hl_math_allzero16(&p_mb->ChromaACLevel[0][chroma4x4BlkIdx]);
            (*DCCoeff4x4Cb)[yO >> 2][xO >> 2] = (*tmp4x4C)[0][0];
            p_mb->CodedBlockPatternChromaAC4x4[0] |= isAllZeros ? 0 : (1 << chroma4x4BlkIdx);
            p_mb->CodedBlockPatternChromaDC4x4[0] |= (*tmp4x4C)[0][0] ? (1 << chroma4x4BlkIdx) : 0; // Will be updated later
        }
        else {
            (*DCCoeff4x4Cb)[yO >> 2][xO >> 2] = 0;
        }
        // FIXME: only if "Single_ctr" enabled
        if (Single_ctr[0] < 7 && (p_mb->CodedBlockPatternChromaAC4x4[0] & (1 << chroma4x4BlkIdx))) {
            residual_inv_type.i_iCbCr = 0;
            err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, p_mb->ChromaACLevel[0][chroma4x4BlkIdx], 0, 15, 16);
            if (err) {
                goto bail;
            }
            Single_ctr[0] += pc_esd->rdo.Single_ctr;
            TotalCoeffs[0] += p_mb->TotalCoeffsChromaACCbCr[0][chroma4x4BlkIdx];
        }

        //== CR (Chroma component "1") ==//
        _p_eSV = p_eSV + (yO * pc_slice_header->PicWidthInSamplesC) + xO;
        hl_math_sub4x4_u8x32(
            _p_eSV, pc_slice_header->PicWidthInSamplesC,
            (const int32_t*)&predCr[yO][xO], 16,
            (int32_t*)(*residual4x4C), 4);
        isAllZeros = hl_math_allzero16((const int32_t(*)[16])&(*residual4x4C));
        if (!isAllZeros) {
            hl_codec_264_transf_frw_residual4x4((*residual4x4C), (*tmp4x4C));
            hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPc[1], __isIntraBlockTrue, (*tmp4x4C), (*ACCoeff4x4));
            Scan4x4_AC_C((*ACCoeff4x4), p_mb->ChromaACLevel[1][chroma4x4BlkIdx]);
            isAllZeros = hl_math_allzero16(&p_mb->ChromaACLevel[1][chroma4x4BlkIdx]);
            (*DCCoeff4x4Cr)[yO >> 2][xO >> 2] = (*tmp4x4C)[0][0];
            p_mb->CodedBlockPatternChromaAC4x4[1] |= isAllZeros ? 0 : (1 << chroma4x4BlkIdx);
            p_mb->CodedBlockPatternChromaDC4x4[1] |= (*tmp4x4C)[0][0] ? (1 << chroma4x4BlkIdx) : 0; // Will be updated later
        }
        else {
            (*DCCoeff4x4Cr)[yO >> 2][xO >> 2] = 0;
        }
        // FIXME: only if "Single_ctr" enabled
        if (Single_ctr[1] < 7 && (p_mb->CodedBlockPatternChromaAC4x4[1] & (1 << chroma4x4BlkIdx))) {
            residual_inv_type.i_iCbCr = 1;
            err = f_write_block(&residual_inv_type, p_codec, p_mb, pc_esd->rdo.pobj_bits, p_mb->ChromaACLevel[1][chroma4x4BlkIdx], 0, 15, 16);
            if (err) {
                goto bail;
            }
            Single_ctr[1] += pc_esd->rdo.Single_ctr;
            TotalCoeffs[1] += p_mb->TotalCoeffsChromaACCbCr[1][chroma4x4BlkIdx];
        }
    }

    // FIXME: only if "Single_ctr" enabled
    if (Single_ctr[0] < 7 && TotalCoeffs[0] == 1) {
        p_mb->CodedBlockPatternChromaAC4x4[0] = 0;
        //p_mb->CodedBlockPatternChromaDC4x4[0] = 0; // FIXME: is it correct?
    }
    // FIXME: only if "Single_ctr" enabled
    if (Single_ctr[1] < 7 && TotalCoeffs[1] == 1) {
        p_mb->CodedBlockPatternChromaAC4x4[1] = 0;
        //p_mb->CodedBlockPatternChromaDC4x4[1] = 0;// FIXME: is it correct?
    }

    /* Chroma 2x2 DC Hadamard transform + quantize DC coeffs */
    if (p_mb->CodedBlockPatternChromaDC4x4[0] || p_mb->CodedBlockPatternChromaDC4x4[1]) {
        if (pc_sps->ChromaArrayType == 1) { // YUV420?
            HL_ALIGN(HL_ALIGN_V) tmp2x2[2][2];
            HL_ALIGN(HL_ALIGN_V) _tmp2x2[2][2];

            if (p_mb->CodedBlockPatternChromaDC4x4[0]) {
                HL_ALIGN(HL_ALIGN_V) DCCoeff2x2Cb[2][2] = { {(*DCCoeff4x4Cb)[0][0], (*DCCoeff4x4Cb)[0][1]}, {(*DCCoeff4x4Cb)[1][0], (*DCCoeff4x4Cb)[1][1]} };
                hl_codec_264_transf_frw_hadamard2x2_dc_chroma(DCCoeff2x2Cb, _tmp2x2);
                hl_codec_264_quant_frw2x2_scale_dc_chroma(p_mb->QPc[0], isIntra, _tmp2x2, tmp2x2);
                Scan2x2_DC_C(tmp2x2, p_mb->ChromaDCLevel[0]);
                p_mb->CodedBlockPatternChromaDC4x4[0] =
                    (tmp2x2[0][0] ? (1 << 0) : 0) | (tmp2x2[0][1] ? (1 << 1) : 0) | (tmp2x2[1][0] ? (1 << 2) : 0) | (tmp2x2[1][1] ? (1 << 3) : 0);
            }
            if (p_mb->CodedBlockPatternChromaDC4x4[1]) {
                HL_ALIGN(HL_ALIGN_V) DCCoeff2x2Cr[2][2] = { {(*DCCoeff4x4Cr)[0][0], (*DCCoeff4x4Cr)[0][1]}, {(*DCCoeff4x4Cr)[1][0], (*DCCoeff4x4Cr)[1][1]} };
                hl_codec_264_transf_frw_hadamard2x2_dc_chroma(DCCoeff2x2Cr, _tmp2x2);
                hl_codec_264_quant_frw2x2_scale_dc_chroma(p_mb->QPc[1], isIntra, _tmp2x2, tmp2x2);
                Scan2x2_DC_C(tmp2x2, p_mb->ChromaDCLevel[1]);
                p_mb->CodedBlockPatternChromaDC4x4[1] =
                    (tmp2x2[0][0] ? (1 << 0) : 0) | (tmp2x2[0][1] ? (1 << 1) : 0) | (tmp2x2[1][0] ? (1 << 2) : 0) | (tmp2x2[1][1] ? (1 << 3) : 0);
            }
        }
        else {
            HL_DEBUG_ERROR("Not implemented");
            err = HL_ERROR_NOT_IMPLEMENTED;
            goto bail;
        }
    }

    // Reconstruct
    // FIXME: analyse how it's "scaled and reconstructed" and do it yourself
    err = hl_codec_264_transf_decode_chroma(p_codec, p_mb, predCb, 0);
    if (err) {
        goto bail;
    }
    err = hl_codec_264_transf_decode_chroma(p_codec, p_mb, predCr, 1);
    if (err) {
        goto bail;
    }

bail:
    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, residual4x4C);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4C);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff4x4);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, DCCoeff4x4Cb);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, DCCoeff4x4Cr);

    return err;
}

static HL_ERROR_T _hl_codec_264_rdo_mb_guess_cbp(
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_t* p_codec
)
{
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->pc_slice_hdr;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    //== Set CBP  (Coded Block Pattern) for Luma and Chroma ==//
    if (HL_CODEC_264_MB_TYPE_IS_INTRA_16x16(p_mb)) {
        if (pc_sps->ChromaArrayType  != 3) {
            // p_mb->CodedBlockPatternLuma = (!p_mb->CodedBlockPatternChromaAC4x4[0] && !p_mb->CodedBlockPatternChromaAC4x4[1]) ? 0 : 15;
            p_mb->CodedBlockPatternLuma = p_mb->CodedBlockPatternLuma4x4 ? 15 : 0;
        }
        else {
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
    }
    else {
        if (pc_sps->ChromaArrayType  != 3) {
            int32_t i8x8, i4x4, i;
            p_mb->CodedBlockPatternLuma = 0;
            for (i8x8 = 0; i8x8 < 4; ++i8x8) {
                for (i4x4 = 0; i4x4 < 4; ++i4x4) {
                    i = (i8x8 << 2) + i4x4;
                    if (p_mb->CodedBlockPatternLuma4x4 & (1 << i)) {
                        p_mb->CodedBlockPatternLuma |= (1 << i8x8);
                        break;
                    }
                }
            }
        }
        else {
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
    }

    // Table 7-15 – Specification of CodedBlockPatternChroma values
    if (pc_sps->ChromaArrayType == 0 || pc_sps->ChromaArrayType == 3) {
        p_mb->CodedBlockPatternChroma = 0;
    }
    else {
        if (
            (p_mb->CodedBlockPatternChromaDC4x4[0] || p_mb->CodedBlockPatternChromaDC4x4[1])
            && (!p_mb->CodedBlockPatternChromaAC4x4[0] && !p_mb->CodedBlockPatternChromaAC4x4[1])
        ) {
            // One or more chroma DC transform coefficient levels shall be non-zero valued.
            // All chroma AC transform coefficient levels are equal to 0.
            p_mb->CodedBlockPatternChroma = 1;
        }
        else if (
            (p_mb->CodedBlockPatternChromaAC4x4[0] || p_mb->CodedBlockPatternChromaAC4x4[1])
        ) {
            // Zero or more chroma DC transform coefficient levels are non-zero valued.
            // One or more chroma AC transform coefficient levels shall be non-zero valued.
            p_mb->CodedBlockPatternChroma = 2;
        }
        else {
            p_mb->CodedBlockPatternChroma = 0;
        }
    }

    p_mb->coded_block_pattern = (p_mb->CodedBlockPatternChroma << 4 | p_mb->CodedBlockPatternLuma);

    if (p_mb->coded_block_pattern > 47) {
        p_mb->coded_block_pattern -= 16;
        p_mb->CodedBlockPatternChroma = p_mb->coded_block_pattern >> 4;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rdo_mb_compute_inter_luma4x4(
    HL_IN hl_codec_264_mb_t* p_mb,
    HL_IN hl_codec_264_t* p_codec,
    HL_IN const hl_pixel_t* p_SL_in, int32_t i_SL_in_stride,
    HL_IN const hl_pixel_t* p_SL_pred, int32_t i_SL_pred_stride,
    HL_OUT int32_t SL_res[4][4], // Residual (output)
    HL_OUT int32_t LumaLevel[16],
    HL_OUT hl_bool_t *pb_zeros // Whether all luma blocks are zeros
)
{
    HL_ALIGNED(16) hl_int32_4x4_t* tmp4x4L;
    HL_ALIGNED(16) hl_int32_4x4_t* ACCoeff4x4L;
    hl_codec_264_encode_slice_data_t* pc_esd;

    static const hl_bool_t __isIntraBlockFalse = HL_FALSE;
    static const hl_bool_t __isLumaTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16False = HL_FALSE;
    static const hl_bool_t __isIntra16x16ACTrue = HL_TRUE;
    static const hl_bool_t __isIntra16x16ACFalse = HL_FALSE;

    pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[p_mb->u_slice_idx];

    // Map memory blocks
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &tmp4x4L);
    hl_memory_blocks_map(pc_esd->pc_mem_blocks, &ACCoeff4x4L);

    //== Compute residual ==//
    hl_math_sub4x4_u8x8(p_SL_in, i_SL_in_stride, p_SL_pred, i_SL_pred_stride, (int32_t*)SL_res, 4);
    *pb_zeros = hl_math_allzero16((const int32_t (*)[16])SL_res);
    if (!*pb_zeros) {
        //== LumaLevel ==//
        // Forward transform Cf
        hl_codec_264_transf_frw_residual4x4(SL_res, (*tmp4x4L));
        // Scaling and quantization
        hl_codec_264_quant_frw4x4_scale_ac(p_mb->QPy, __isIntraBlockFalse, (*tmp4x4L), (*ACCoeff4x4L));
        // ZigZag4x4()
        Scan4x4_L((*ACCoeff4x4L), LumaLevel, __isIntra16x16ACFalse);
        // Is all Zeros
        *pb_zeros = hl_math_allzero16((const int32_t (*)[16])LumaLevel);
    }

    // Unmap memory blocks
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, tmp4x4L);
    hl_memory_blocks_unmap(pc_esd->pc_mem_blocks, ACCoeff4x4L);

    return HL_ERROR_SUCCESS;
}
