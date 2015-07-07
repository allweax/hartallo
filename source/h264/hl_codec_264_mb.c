#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_rc.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_rdo.h"
#include "hartallo/h264/hl_codec_264_residual.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_encode.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_frame.h"
#include "hartallo/hl_md5.h"
#include "hartallo/hl_string.h"
#include "hartallo/hl_debug.h"

#include <limits.h> /* INT_MAX */

static HL_ERROR_T _hl_codec_264_mb_write_no_pcm(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec, int32_t* pi_bits_hdr_count, int32_t* pi_bits_data_count);

HL_ERROR_T hl_codec_264_mb_create(uint32_t u_addr, hl_codec_264_mb_t** pp_mb)
{
    extern const hl_object_def_t *hl_codec_264_mb_def_t;
    if (!pp_mb) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_mb = hl_object_create(hl_codec_264_mb_def_t);
    if (!*pp_mb) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_mb)->u_addr = u_addr;
    (*pp_mb)->i_activity = INT_MAX;
    return HL_ERROR_SUCCESS;
}

// Set "mb_type", flags, "MbPartPredMode" and SubParts size.
// Must never be called for "SKIP" mode: "hl_codec_264_utils_init_mb_current()" for more info.
HL_ERROR_T hl_codec_264_mb_set_mb_type(hl_codec_264_mb_t* p_mb, uint32_t mb_type, const hl_codec_264_t* pc_codec)
{
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_codec->layers.pc_active->pc_slice_hdr;
    p_mb->noSubMbPartSizeLessThan8x8Flag = 1;
    // SVC "Mb_Inferred"
    if (mb_type == HL_CODEC_264_SVC_MB_TYPE_INFERRED) {
        // G.7.4.6 Macroblock layer in scalable extension semantics
        p_mb->mb_type = mb_type;
        p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_BL;
        p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INFERRED;
        if (IsSliceHeaderEI(pc_slice_header)) {
            // Table G-5 – Inferred macroblock type I_BL for EI slices.
            p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTRA_BL;
        }
        else {
            p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_INTER_BL;
        }
        p_mb->NumMbPart = 1;
        p_mb->MbPartWidth = p_mb->MbPartHeight = 16;
        return HL_ERROR_SUCCESS;
    }
    // INTER B
    if (IsSliceHeaderB(pc_slice_header)) {
        // B
        if (mb_type < 23) {
            p_mb->mb_type = mb_type;
            p_mb->e_type = HL_CODEC_264_MB_TYPE_START_SLICE_B + (mb_type + 1);
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTER_B;
            p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODES_B[p_mb->mb_type][0];
            p_mb->MbPartPredMode[1] = HL_CODEC_264_MB_MODES_B[p_mb->mb_type][1];
            p_mb->NumMbPart = HL_CODEC_264_MB_PART_NUM_B[mb_type];
            p_mb->MbPartWidth = HL_CODEC_264_MB_PART_SIZE_B[mb_type][0];
            p_mb->MbPartHeight = HL_CODEC_264_MB_PART_SIZE_B[mb_type][1];

            return HL_ERROR_SUCCESS;
        }

        // INTRA
        if (mb_type > 25 + 23) {
            HL_DEBUG_ERROR("Invalid 'mb_type' (%d)", mb_type);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        p_mb->mb_type = mb_type - 23;
        p_mb->e_type = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][0];
        p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA;
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][1];
        p_mb->Intra16x16PredMode = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][2];
        return HL_ERROR_SUCCESS;
    }

    // INTRA
    if (IsSliceHeaderI(pc_slice_header)) {
        if (mb_type > 25) {
            HL_DEBUG_ERROR("Invalid 'mb_type' (%d)", mb_type);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        p_mb->mb_type = mb_type;
        p_mb->e_type = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][0];
        p_mb->flags_type = (p_mb->mb_type == I_PCM) ? HL_CODEC_264_MB_TYPE_FLAGS_PCM : ((p_mb->e_type >= HL_CODEC_264_MB_TYPE_I_16X16_0_0_0 && p_mb->e_type <= HL_CODEC_264_MB_TYPE_I_16X16_3_2_1)
                           ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16 :  HL_CODEC_264_MB_TYPE_FLAGS_INTRA);
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][1];
        p_mb->Intra16x16PredMode = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][2];
        return HL_ERROR_SUCCESS;
    }

    // INTER P / SP
    if (mb_type < 5) { // INTER P?
        p_mb->mb_type = mb_type;
        p_mb->e_type = HL_CODEC_264_MB_TYPE_START_SLICE_P_AND_SP + (mb_type + 1);
        p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTER_P;
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODES_P[p_mb->mb_type][0];
        p_mb->MbPartPredMode[1] = HL_CODEC_264_MB_MODES_P[p_mb->mb_type][1];
        p_mb->NumMbPart = HL_CODEC_264_MB_PART_NUM_P[mb_type];
        p_mb->MbPartWidth = HL_CODEC_264_MB_PART_SIZE_P[mb_type][0];
        p_mb->MbPartHeight = HL_CODEC_264_MB_PART_SIZE_P[mb_type][1];
    }
    else { // INTRA?
        if (mb_type > 25 + 5) {
            HL_DEBUG_ERROR("Invalid 'mb_type' (%d)", mb_type);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        p_mb->mb_type = mb_type - 5;
        p_mb->e_type = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][0];
        p_mb->flags_type = (p_mb->mb_type == I_PCM) ? HL_CODEC_264_MB_TYPE_FLAGS_PCM : ((p_mb->e_type >= HL_CODEC_264_MB_TYPE_I_16X16_0_0_0 && p_mb->e_type <= HL_CODEC_264_MB_TYPE_I_16X16_3_2_1)
                           ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16 :  HL_CODEC_264_MB_TYPE_FLAGS_INTRA);
        p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][1];
        p_mb->Intra16x16PredMode = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][2];
    }

    return HL_ERROR_SUCCESS;
}

// Set SubMbPredType, NumSubMbPart, SubMbPartWidth, SubMbPartHeight, noSubMbPartSizeLessThan8x8Flag, partWidth, partHeight...
// Require valid "p_mb->flags_type" (call "hl_codec_264_mb_set_mb_type()")
HL_ERROR_T hl_codec_264_mb_set_sub_mb_type(hl_codec_264_mb_t* p_mb, const uint32_t (* sub_mb_types)[4], const hl_codec_264_t* pc_codec)
{
    int32_t i_mb_part_idx; // 8x8 block
    uint32_t u_sub_mb_type;

    if (!HL_CODEC_264_MB_TYPE_IS_INTER(p_mb)) {
        HL_DEBUG_ERROR("Unexpected code called");
        return HL_ERROR_UNEXPECTED_CALL;
    }

    //
    //	SubMbPredType, NumSubMbPart, SubMbPartWidth, SubMbPartHeight and noSubMbPartSizeLessThan8x8Flag
    //
    if (HL_CODEC_264_MB_TYPE_IS_INTER_B(p_mb)) {
        // INTER B
        for (i_mb_part_idx = 0; i_mb_part_idx < p_mb->NumMbPart; ++i_mb_part_idx) {
            u_sub_mb_type = (*sub_mb_types)[i_mb_part_idx];
            if (u_sub_mb_type > 12) {
                HL_DEBUG_ERROR("Invalid 'sub_mb_type' (%d)", u_sub_mb_type);
                return HL_ERROR_INVALID_BITSTREAM;
            }
            p_mb->SubMbPredType[i_mb_part_idx] = HL_CODEC_264_SUBMB_PRED_TYPE_B[u_sub_mb_type];
            p_mb->SubMbPredMode[i_mb_part_idx] = HL_CODEC_264_SUBMB_PRED_MODE_B[u_sub_mb_type];
            p_mb->NumSubMbPart[i_mb_part_idx] = HL_CODEC_264_SUBMB_PART_NUM_B[u_sub_mb_type];
            p_mb->SubMbPartWidth[i_mb_part_idx] = HL_CODEC_264_SUBMB_PART_SIZE_B[u_sub_mb_type][0];
            p_mb->SubMbPartHeight[i_mb_part_idx] = HL_CODEC_264_SUBMB_PART_SIZE_B[u_sub_mb_type][1];
            if (p_mb->SubMbPredType[i_mb_part_idx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8) {
                if (p_mb->NumSubMbPart[i_mb_part_idx] > 1) {
                    p_mb->noSubMbPartSizeLessThan8x8Flag = 0;
                }
                else if (!pc_codec->sps.pc_active->direct_8x8_inference_flag) {
                    p_mb->noSubMbPartSizeLessThan8x8Flag = 0;
                }
            }
        }
    }
    else {
        // INTER P
        for (i_mb_part_idx = 0; i_mb_part_idx < p_mb->NumMbPart; ++i_mb_part_idx) {
            u_sub_mb_type = (*sub_mb_types)[i_mb_part_idx];
            if (u_sub_mb_type > 3) {
                HL_DEBUG_ERROR("Invalid 'sub_mb_type' (%d)", u_sub_mb_type);
                return HL_ERROR_INVALID_BITSTREAM;
            }
            p_mb->SubMbPredType[i_mb_part_idx] = HL_CODEC_264_SUBMB_PRED_TYPE_P[u_sub_mb_type];
            p_mb->SubMbPredMode[i_mb_part_idx] = HL_CODEC_264_SUBMB_PRED_MODE_P[u_sub_mb_type];
            p_mb->NumSubMbPart[i_mb_part_idx] = HL_CODEC_264_SUBMB_PART_NUM_P[u_sub_mb_type];
            p_mb->SubMbPartWidth[i_mb_part_idx] = HL_CODEC_264_SUBMB_PART_SIZE_P[u_sub_mb_type][0];
            p_mb->SubMbPartHeight[i_mb_part_idx] = HL_CODEC_264_SUBMB_PART_SIZE_P[u_sub_mb_type][1];
        }
    }

    //
    //	partWidth, partHeight, partWidthC, partHeightC
    //
    {
        // FIXME: ease for PCM, INTRA, and PSKIP
        int32_t i_mb_sub_part_idx;
        for (i_mb_part_idx = 0; i_mb_part_idx < p_mb->NumMbPart; ++i_mb_part_idx) {
            for (i_mb_sub_part_idx = 0; i_mb_sub_part_idx < p_mb->NumSubMbPart[i_mb_part_idx]; ++i_mb_sub_part_idx) {
                p_mb->partWidth[i_mb_part_idx][i_mb_sub_part_idx] = p_mb->SubMbPartWidth[i_mb_part_idx];
                p_mb->partHeight[i_mb_part_idx][i_mb_sub_part_idx] = p_mb->SubMbPartHeight[i_mb_part_idx];
                p_mb->partWidthC[i_mb_part_idx][i_mb_sub_part_idx] = p_mb->partWidth[i_mb_part_idx][i_mb_sub_part_idx] >> pc_codec->sps.pc_active->SubWidthC_TrailingZeros;
                p_mb->partHeightC[i_mb_part_idx][i_mb_sub_part_idx] = p_mb->partHeight[i_mb_part_idx][i_mb_sub_part_idx] >> pc_codec->sps.pc_active->SubHeightC_TrailingZeros;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// "sub_mb_type" elements not present (NumMbPart != 4)
HL_ERROR_T hl_codec_264_mb_set_sub_mb_type_when_not_present(hl_codec_264_mb_t* p_mb, const hl_codec_264_t* pc_codec)
{
    int32_t i_mb_part_idx; // 8x8 block
    uint32_t max_mb_type;
    const int32_t (*PART_SIZE_X)[2];

    if (HL_CODEC_264_MB_TYPE_IS_INTER_B(p_mb)) {
        PART_SIZE_X = HL_CODEC_264_MB_PART_SIZE_B;
        max_mb_type = 23;
    }
    else {
        PART_SIZE_X = HL_CODEC_264_MB_PART_SIZE_P;
        max_mb_type = 5;
    }

    if (p_mb->mb_type >= max_mb_type) {
        HL_DEBUG_ERROR("%d not valid as 'mb_type'", p_mb->mb_type);
        return HL_ERROR_INVALID_BITSTREAM;
    }

    //
    //	partWidth, partHeight, partWidthC, partHeightC
    //

    for (i_mb_part_idx = 0; i_mb_part_idx < p_mb->NumMbPart; ++i_mb_part_idx) {
        // i_mb_sub_part_idx = 0, 1 beacuse we cannot have 3 and we know NumMbPart != 4.
        p_mb->NumSubMbPart[i_mb_part_idx] = 1;
        p_mb->SubMbPartWidth[i_mb_part_idx] = p_mb->partWidth[i_mb_part_idx][0] = p_mb->partWidth[i_mb_part_idx][1] = PART_SIZE_X[p_mb->mb_type][0];
        p_mb->SubMbPartHeight[i_mb_part_idx] = p_mb->partHeight[i_mb_part_idx][0] = p_mb->partHeight[i_mb_part_idx][1] = PART_SIZE_X[p_mb->mb_type][1];
        p_mb->partWidthC[i_mb_part_idx][0] = p_mb->partWidthC[i_mb_part_idx][1] = p_mb->partWidth[i_mb_part_idx][0] >> pc_codec->sps.pc_active->SubWidthC_TrailingZeros;
        p_mb->partHeightC[i_mb_part_idx][0] = p_mb->partHeightC[i_mb_part_idx][1] = p_mb->partHeight[i_mb_part_idx][0] >> pc_codec->sps.pc_active->SubHeightC_TrailingZeros;
        // p_mb->SubMbPartWidth[i_mb_part_idx] = p_mb->SubMbPartHeight[i_mb_part_idx] = 16;
    }


    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_mb_encode_pcm(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec)
{
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_pixel_t *p_SL, *p_SU, *p_SV;
    uint32_t x, y;
    HL_ERROR_T err;

    if (!p_mb || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // Set current macroblock type, partition sizes/types...
    err = hl_codec_264_mb_set_mb_type(p_mb, I_PCM, p_codec);
    if (err) {
        return err;
    }

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    p_SL = ((const hl_pixel_t*)p_codec->encoder.pc_frame->data_ptr[0]) + p_mb->xL + (p_mb->yL * pc_slice_header->PicWidthInSamplesL);
    p_SU = ((const hl_pixel_t*)p_codec->encoder.pc_frame->data_ptr[1]) + p_mb->xC + (p_mb->yC * pc_slice_header->PicWidthInSamplesC);
    p_SV = ((const hl_pixel_t*)p_codec->encoder.pc_frame->data_ptr[2]) + p_mb->xC + (p_mb->yC * pc_slice_header->PicWidthInSamplesC);

    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, p_mb->mb_type);
    hl_codec_264_bits_align(pc_esd->pobj_bits);

    for (y = 0; y < 16; ++ y) {
        for (x = 0; x < 16; x+=4) {
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SL[x], pc_sps->BitDepthY);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SL[x + 1], pc_sps->BitDepthY);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SL[x + 2], pc_sps->BitDepthY);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SL[x + 3], pc_sps->BitDepthY);
        }
        p_SL += pc_slice_header->PicWidthInSamplesL;
    }
    for (y = 0; y < pc_sps->MbHeightC; ++ y) {
        for (x = 0; x < pc_sps->MbWidthC; x+=4) {
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SU[x], pc_sps->BitDepthC);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SU[x + 1], pc_sps->BitDepthC);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SU[x + 2], pc_sps->BitDepthC);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SU[x + 3], pc_sps->BitDepthC);
        }
        p_SU += pc_slice_header->PicWidthInSamplesC;
    }
    for (y = 0; y < pc_sps->MbHeightC; ++ y) {
        for (x = 0; x < pc_sps->MbWidthC; x+=4) {
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SV[x], pc_sps->BitDepthC);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SV[x + 1], pc_sps->BitDepthC);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SV[x + 2], pc_sps->BitDepthC);
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_SV[x + 3], pc_sps->BitDepthC);
        }
        p_SV += pc_slice_header->PicWidthInSamplesC;
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_mb_encode_intra(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec, int32_t *pi_mad, int32_t* pi_bits_hdr_count, int32_t* pi_bits_data_count)
{
    HL_ERROR_T err;

    // Guess best Intra mode
    err = (p_codec->layers.currDQId > 0)
          ? hl_codec_264_rdo_mb_guess_best_intra_pred_svc(p_mb, p_codec) // Scalable Video Coding (SVC)
          : hl_codec_264_rdo_mb_guess_best_intra_pred_avc(p_mb, p_codec, pi_mad); // Advandced Video Coding (AVC)
    if (err) {
        return err;
    }

    // Write bits to the stream
    err = _hl_codec_264_mb_write_no_pcm(p_mb, p_codec, pi_bits_hdr_count, pi_bits_data_count);
    if (err) {
        return err;
    }

    // Patch base layer MB for SVC enhanced layers
    if (p_codec->encoder.b_svc_enabled && p_codec->layers.currDQId == 0) {
        // Patching macroblock type
        if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_4X4;
            p_mb->mb_type = I_4x4;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4;
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_8X8;
            p_mb->mb_type = I_8x8;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8;
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_16X16;
            p_mb->mb_type = I_16x16;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16;
        }
    }

    return err;
}

HL_ERROR_T hl_codec_264_mb_encode_inter(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec, int32_t *pi_mad, int32_t* pi_bits_hdr_count, int32_t* pi_bits_data_count)
{
    HL_ERROR_T err;

    // Guess best Inter mode
    err = (p_codec->layers.currDQId > 0)
          ? hl_codec_264_rdo_mb_guess_best_inter_pred_svc(p_mb, p_codec) // Scalable Video Coding (SVC)
          : hl_codec_264_rdo_mb_guess_best_inter_pred_avc(p_mb, p_codec, pi_mad); // Advandced Video Coding (AVC)
    if (err) {
        return err;
    }

    // Write bits to the stream
    err = _hl_codec_264_mb_write_no_pcm(p_mb, p_codec, pi_bits_hdr_count, pi_bits_data_count);
    if (err) {
        return err;
    }

    return err;
}

// Set default Quant values for the MB
HL_ERROR_T hl_codec_264_mb_set_default_quant_values(hl_codec_264_mb_t* p_mb, const hl_codec_264_t* pc_codec)
{
    int32_t QPyprev;
    int32_t HL_ALIGN(HL_ALIGN_V) qPI[4];
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_mb_t* p_mb_prev = p_mb->u_addr > 0 ? pc_layer->pp_list_macroblocks[p_mb->u_addr - 1] : HL_NULL;
    const hl_codec_264_nal_pps_t* pc_pps = pc_layer->pc_slice_hdr->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    int32_t MinusQpBdOffsetC = -pc_sps->QpBdOffsetC;

    if(p_mb_prev && p_mb_prev->l_slice_id == p_mb->l_slice_id) {
        QPyprev = p_mb_prev->QPy;
    }
    else {
        QPyprev = pc_layer->pc_slice_curr->p_header->SliceQPY;
    }

    p_mb->QSy = pc_layer->pc_slice_curr->p_header->QSY;
    // compute QPy as per (7-36) and TransformBypassModeFlag
    p_mb->QPy = ((QPyprev + p_mb->mb_qp_delta + 52 + (pc_sps->QpBdOffsetY << 1)) % (52 + pc_sps->QpBdOffsetY)) - pc_sps->QpBdOffsetY;
    p_mb->QPyprime = p_mb->QPy + pc_sps->QpBdOffsetY; //(7-37)
    p_mb->TransformBypassModeFlag = (pc_sps->qpprime_y_zero_transform_bypass_flag == 1 && p_mb->QPyprime == 0) ? 1 : 0;
    // 8.5.8 Derivation process for chroma quantisation parameters
    p_mb->qPOffset[0] = pc_pps->chroma_qp_index_offset; // (8-313)
    p_mb->qPOffset[1] = pc_pps->second_chroma_qp_index_offset; // (8-314)


#if /* FIXME: HL_HAVE_SIMD */ 0
    _mm_store_si128((__m128i*)qPI,
                    HL_MATH_CLIP3_SIMD(_mm_set1_epi32(MinusQpBdOffsetC), _mm_set1_epi32(51),
                                       _mm_add_epi32(_mm_set1_epi32(p_mb->QPy), _mm_set_epi32(p_mb->qPOffset[1], p_mb->qPOffset[0], p_mb->qPOffset[1], p_mb->qPOffset[0]))));
#else
    qPI[0] = HL_MATH_CLIP3(MinusQpBdOffsetC, 51, p_mb->QPy + p_mb->qPOffset[0]); // (8-315)
    qPI[1] = HL_MATH_CLIP3(MinusQpBdOffsetC, 51, p_mb->QPy + p_mb->qPOffset[1]); // (8-315)
    qPI[2] = HL_MATH_CLIP3(MinusQpBdOffsetC, 51, p_mb->QSy + p_mb->qPOffset[0]); // (8-315)
    qPI[3] = HL_MATH_CLIP3(MinusQpBdOffsetC, 51, p_mb->QSy + p_mb->qPOffset[1]); // (8-315)
#endif

    p_mb->QPc[0] = qPI2QPC[qPI[0]];
    p_mb->QPc[1] = qPI2QPC[qPI[1]];
    p_mb->QSc[0] = qPI2QPC[qPI[2]];
    p_mb->QSc[1] = qPI2QPC[qPI[3]];

    p_mb->QPprimeC[0] = p_mb->QPc[0] + pc_sps->QpBdOffsetC;
    p_mb->QPprimeC[1] = p_mb->QPc[1] + pc_sps->QpBdOffsetC;

    return HL_ERROR_SUCCESS;
}

// 6.4.10.7 Derivation process for neighbouring partitions
// FIXME: could be easier (eclipse)
void hl_codec_264_mb_get_neighbouring_partitions(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    int32_t mbPartIdx,
    HL_CODEC_264_SUBMB_TYPE_T currSubMbType,
    int32_t subMbPartIdx,
    hl_bool_t luma,
    int32_t* mbAddrA, int32_t* mbPartIdxA, int32_t* subMbPartIdxA,
    int32_t* mbAddrB, int32_t* mbPartIdxB, int32_t* subMbPartIdxB,
    int32_t* mbAddrC, int32_t* mbPartIdxC, int32_t* subMbPartIdxC,
    int32_t* mbAddrD, int32_t* mbPartIdxD, int32_t* subMbPartIdxD
)
{
    enum N_e { N_A, N_B, N_C, N_D };
    enum N_e N;
    int32_t *mbAddrN, *mbPartIdxN, *subMbPartIdxN;
    int32_t x,y,xS,yS,xD,yD,xN,yN,xW,yW,predPartWidth;
    const hl_codec_264_mb_t* mbN;

    // 6.4.2.1 - upper-left sample of the macroblock partition
    x = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);
    y = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);

    //xS and yS
    switch(p_mb->e_type) {
    case HL_CODEC_264_MB_TYPE_P_8X8:
    case HL_CODEC_264_MB_TYPE_P_8X8REF0:
    case HL_CODEC_264_MB_TYPE_B_8X8:
        // 6.4.2.2 - upper-left sample of the sub-macroblock partition
        hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);
        break;
    default:
        xS = yS = 0;
        break;
    }

    // predPartWidth
    switch(p_mb->e_type) {
    case HL_CODEC_264_MB_TYPE_P_SKIP:
    case HL_CODEC_264_MB_TYPE_B_SKIP:
    case HL_CODEC_264_MB_TYPE_B_DIRECT_16X16: {
        predPartWidth = 16;
        break;
    }
    case HL_CODEC_264_MB_TYPE_B_8X8: {
        if(currSubMbType==HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8) {
            predPartWidth=16;
        }
        else {
            // FIXME: predPartWidth = SubMbPartWidth( sub_mb_type[ mbPartIdx ] ).
            HL_DEBUG_ERROR("Not checked code");
            predPartWidth=p_mb->SubMbPartWidth[mbPartIdx];
        }
        break;
    }
    case HL_CODEC_264_MB_TYPE_P_8X8:
    case HL_CODEC_264_MB_TYPE_P_8X8REF0: {
        predPartWidth=p_mb->SubMbPartWidth[mbPartIdx];
        break;
    }
    default: {
        predPartWidth=p_mb->MbPartWidth;
        break;
    }
    }

    for(N=N_A; N<=N_D; ++N) {
        xD = N==N_C ? predPartWidth : xD_yD[N][0];
        yD = xD_yD[N][1];
        switch(N) {
        case N_A:
            mbAddrN=mbAddrA,mbPartIdxN=mbPartIdxA,subMbPartIdxN=subMbPartIdxA;
            break;
        case N_B:
            mbAddrN=mbAddrB,mbPartIdxN=mbPartIdxB,subMbPartIdxN=subMbPartIdxB;
            break;
        case N_C:
            mbAddrN=mbAddrC,mbPartIdxN=mbPartIdxC,subMbPartIdxN=subMbPartIdxC;
            break;
        case N_D:
            mbAddrN=mbAddrD,mbPartIdxN=mbPartIdxD,subMbPartIdxN=subMbPartIdxD;
            break;
        }
        xN = x + xS + xD;// (6-29)
        yN = y + yS + yD;// (6-30)
        // 6.4.11 Derivation process for neighbouring locations
        hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, xN, yN, mbAddrN, &xW, &yW, luma);
        if (*mbAddrN != HL_CODEC_264_MB_ADDR_NOT_AVAIL) {
            int32_t svc_part_not_avail = 0;
            mbN = p_codec->layers.pc_active->pp_list_macroblocks[*mbAddrN];
            // 6.4.12.4 Derivation process for macroblock and sub-macroblock partition indices
            hl_codec_264_mb_get_sub_partition_indices(mbN, xW, yW, mbPartIdxN, subMbPartIdxN);
            // FIXME: not sure. could partitions be out of order?
            // When the partition given by mbPartIdxN and subMbPartIdxN is not yet decoded, the macroblock
            // partition mbPartIdxN and the sub-macroblock partition subMbPartIdxN are marked as not
            // available
            /* G.8.4.1.1 SVC derivation process for luma motion vector components and reference indices of a macroblock or sub-macroblock partition
            	g) In subclause 6.4.10.7, a macroblock partition or sub-macroblock partition given by mbAddrN, mbPartIdxN,
            	and subMbPartIdxN is treated as not yet decoded when mbAddrN is equal to CurrMbAddr and
            	(4 * mbPartIdxN + subMbPartIdxN) is greater than (4 * mbPartIdx + subMbPartIdx).
            */
            if (p_codec->layers.pc_active->DQId > 0) {
                svc_part_not_avail = ((((*mbPartIdxN) << 2) + (*subMbPartIdxN)) > ((mbPartIdx << 2) + subMbPartIdx));
            }
            if ((*mbAddrN==p_mb->u_addr) && (svc_part_not_avail || *mbPartIdxN > mbPartIdx || (*mbPartIdxN == mbPartIdx && *subMbPartIdxN > subMbPartIdx))) {
                *mbAddrN = HL_CODEC_264_MB_ADDR_NOT_AVAIL;
                *mbPartIdxN = HL_CODEC_264_MB_PART_IDX_NOT_AVAIL;
                *subMbPartIdxN = HL_CODEC_264_SUBMB_PART_IDX_NOT_AVAIL;
            }
        }
        else {
            *mbPartIdxN = HL_CODEC_264_MB_PART_IDX_NOT_AVAIL;
            *subMbPartIdxN = HL_CODEC_264_SUBMB_PART_IDX_NOT_AVAIL;
        }
    }
}

static HL_ERROR_T _hl_codec_264_mb_write_no_pcm(hl_codec_264_mb_t* p_mb, hl_codec_264_t* p_codec, int32_t* pi_bits_hdr_count, int32_t* pi_bits_data_count)
{
    int32_t mbPartIdx, i_bits_hdr_count, i_bits_data_count;
    uint32_t noSubMbPartSizeLessThan8x8Flag, u_prev_mb_skipped;
    static int _transform_size_8x8_flag = 0; // TODO: FRExt not supported yet
    uint32_t u_scan_idx_start = 0, u_scan_idx_end = 15;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_layer_t *pc_layer;
    hl_codec_264_encode_slice_data_t* pc_esd;

    HL_ERROR_T err = HL_ERROR_SUCCESS;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    i_bits_hdr_count = 0;
    i_bits_data_count = 0;
    u_prev_mb_skipped = 0;

    // Scalable Video Coding (SVC)
    if (pc_slice_header->SVCExtFlag) {
        u_scan_idx_start = pc_slice_header->ext.svc.scan_idx_start;
        u_scan_idx_end = pc_slice_header->ext.svc.scan_idx_end;
    }

    if (pi_bits_data_count) {
        *pi_bits_data_count = 0;
    }
    if (pi_bits_hdr_count) {
        *pi_bits_hdr_count = 0;
    }

    if (pi_bits_hdr_count) {
        i_bits_hdr_count = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits);
    }

    /* P_SKIP */
    if (!IsSliceHeaderI(pc_slice_header) && !IsSliceHeaderSI(pc_slice_header)) {
        if (pc_pps->entropy_coding_mode_flag) {
            // FIXME:CABAC is supported by Hartallo 1.0 -> get the code from there.
            HL_DEBUG_ERROR("Not implemented");
            err = HL_ERROR_NOT_IMPLEMENTED;
            goto bail;
        }
        else {
            if (HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb)) {
                ++pc_esd->u_mb_skip_run;
                if (p_mb->u_addr == (pc_esd->i_mb_end - 1)) {
                    // mb_skip_run ue(v)
                    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_esd->u_mb_skip_run);
                }
                if (pi_bits_hdr_count) {
                    *pi_bits_hdr_count = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits) - i_bits_hdr_count;
                }
                return HL_ERROR_SUCCESS;
            }
            else {
                if (pc_esd->u_mb_skip_run > 0) {
                    // mb_skip_run ue(v)
                    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_esd->u_mb_skip_run);
                    pc_esd->u_mb_skip_run = 0;
                }
                else {
                    // mb_skip_run ue(v)
                    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_esd->u_mb_skip_run);
                }
            }
        }
    } // end-of-P_SKIP

    if (pc_slice_header->MbaffFrameFlag && ((p_mb->u_addr & 1) == 0 || ((p_mb->u_addr & 1) == 1 && u_prev_mb_skipped))) {
        // mb_field_decoding_flag u(1) | ae(v)
        hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->mb_field_decoding_flag); //TODO: CABAC
    }

    // 7.3.5 Macroblock layer syntax
    //== begin of macroblock_layer( )
    if (pc_slice_header->SVCExtFlag) {
        int32_t base_layer_available = !pc_slice_header->ext.svc.NoInterLayerPredFlag;
        if (base_layer_available) {
            if (p_mb->ext.svc.InCropWindow) {
                if (pc_slice_header->ext.svc.adaptive_base_mode_flag) {
                    // base_mode_flag u(1) | ae(v)
                    hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->ext.svc.base_mode_flag); // TODO: CABAC
                }
            }
        }
    }

    if (!p_mb->ext.svc.base_mode_flag) {
        if (pc_pps->entropy_coding_mode_flag) {
            switch(pc_slice_header->SliceTypeModulo5) {
            case HL_CODEC_264_SLICE_TYPE_I: {
                // mb_type: ue(v) | ae(v)
                HL_DEBUG_ERROR("CABAC not supported yet");
                break;
            }
            case HL_CODEC_264_SLICE_TYPE_P:
            case HL_CODEC_264_SLICE_TYPE_SP: {
                // mb_type: ue(v) | ae(v)
                HL_DEBUG_ERROR("CABAC not supported yet");
                break;
            }
            default: {
                HL_DEBUG_ERROR("Unexpected code called");
                err = HL_ERROR_INVALID_BITSTREAM;
                goto bail;
            }
            }
        }
        else {
            // mb_type: ue(v) | ae(v)
            hl_codec_264_bits_write_ue(pc_esd->pobj_bits, p_mb->mb_type); // TODO: CABAC
        }
    }


    if (!p_mb->ext.svc.base_mode_flag) {
        noSubMbPartSizeLessThan8x8Flag = 1;
        if (!HL_CODEC_264_MB_TYPE_IS_I_NxN(p_mb) && !HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0) && p_mb->NumMbPart == 4) {
            // 7.3.5.2 Sub-macroblock prediction syntax
            // G.7.3.6.2 Sub-macroblock prediction in scalable extension syntax
            // sub_mb_pred( mb_type )
            // sub_mb_pred_in_scalable_extension( mb_type )
            int32_t subMbPartIdx,max_ref_idx0,max_ref_idx1;

            max_ref_idx0 = pc_slice_header->num_ref_idx_l0_active_minus1;
            max_ref_idx1 = pc_slice_header->num_ref_idx_l1_active_minus1;

            // Read "sub_mb_type" values
            for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                hl_codec_264_bits_write_ue(pc_esd->pobj_bits, p_mb->sub_mb_type[mbPartIdx]); // TODO: CABAC
            }

            if (pc_slice_header->SVCExtFlag) {
                if (p_mb->ext.svc.InCropWindow && pc_slice_header->ext.svc.adaptive_motion_prediction_flag) {
                    for (mbPartIdx = 0; mbPartIdx < 4; ++mbPartIdx) {
                        if (p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_DIRECT && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L1) {
                            // motion_prediction_flag_l0[ mbPartIdx ] 2 u(1) | ae(v)
                            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx]); // TODO: CABAC
                        }
                    }
                    for (mbPartIdx = 0; mbPartIdx < 4; ++mbPartIdx) {
                        if (p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_DIRECT && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L0) {
                            // motion_prediction_flag_l1[ mbPartIdx ] 2 u(1) | ae(v)
                            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx]); // TODO: CABAC
                        }
                    }
                }
            }

            for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                if ((pc_slice_header->num_ref_idx_l0_active_minus1 > 0 ||
                        p_mb->mb_field_decoding_flag != pc_slice_header->field_pic_flag) &&
                        p_mb->e_type != HL_CODEC_264_MB_TYPE_P_8X8REF0 &&
                        p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 &&
                        p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L1 &&
                        (!pc_slice_header->SVCExtFlag || !p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx])) {
                    // ref_idx_l0 te(v) | ae(v)
                    hl_codec_264_bits_write_te(pc_esd->pobj_bits, p_mb->ref_idx_l0[mbPartIdx], max_ref_idx0); // TODO: CABAC
                }
            }
            for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                if ((pc_slice_header->num_ref_idx_l1_active_minus1 > 0 ||
                        p_mb->mb_field_decoding_flag != pc_slice_header->field_pic_flag) &&
                        p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 &&
                        p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L0 &&
                        (!pc_slice_header->SVCExtFlag || !p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx])) {
                    // ref_idx_l0 te(v) | ae(v)
                    hl_codec_264_bits_write_te(pc_esd->pobj_bits, p_mb->ref_idx_l1[mbPartIdx], max_ref_idx1); // TODO: CABAC
                }
            }

            for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                if (p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L1) {
                    for (subMbPartIdx=0; subMbPartIdx<p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                        // mvd_l0 se(v)
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l0[mbPartIdx][subMbPartIdx].x); // TODO: CABAC
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l0[mbPartIdx][subMbPartIdx].y); // TODO: CABAC
                    }
                }
            }
            for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                if (p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L0) {
                    for (subMbPartIdx=0; subMbPartIdx<p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                        // mvd_l1 se(v)
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l1[mbPartIdx][subMbPartIdx].x); // TODO: CABAC
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l1[mbPartIdx][subMbPartIdx].y); // TODO: CABAC
                    }
                }
            }
        }
        else {
            if(_transform_size_8x8_flag && p_mb->e_type == HL_CODEC_264_MB_TYPE_I_NXN) {
                hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->transform_size_8x8_flag); // TODO: CABAC
            }

            // 7.3.5.1 Macroblock prediction syntax
            // G.7.3.6.1 Macroblock prediction in scalable extension syntax
            // begin-of-mb_pred(mb_type)
            // mb_pred_in_scalable_extension( mb_type )
            if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0) || HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0) || HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0)) {
                    int32_t luma4x4BlkIdx;
                    for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
                        hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]); // TODO: CABAC
                        if (!p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
                            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx], 3); // TODO: CABAC
                        }
                    }
                }
                else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0)) {
                    int32_t luma8x8BlkIdx;
                    for (luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; ++luma8x8BlkIdx) {
                        hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]); // TODO: CABAC
                        if (!p_mb->prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]) {
                            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_mb->rem_intra8x8_pred_mode[luma8x8BlkIdx], 3); // TODO: CABAC
                        }
                    }

                }
                // NOTE: ChromaArrayType must be equal to 1 (YCbCr 420) for Baseline
                if (pc_sps->ChromaArrayType) {
                    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, p_mb->intra_chroma_pred_mode); // TODO: CABAC
                }
            }
            else if (!HL_CODEC_264_MB_MODE_IS_DIRECT(p_mb, 0)) {
                int32_t max_ref_idx0, max_ref_idx1;

                if (pc_slice_header->SVCExtFlag && p_mb->ext.svc.InCropWindow && pc_slice_header->ext.svc.adaptive_motion_prediction_flag) {
                    for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                        if (!HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, mbPartIdx)) {
                            // motion_prediction_flag_l0[ mbPartIdx ] 2 u(1) | ae(v)
                            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx]); // TODO: CABAC
                        }
                    }
                    for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                        if (!HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx)) {
                            // motion_prediction_flag_l1[ mbPartIdx ] 2 u(1) | ae(v)
                            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx]); // TODO: CABAC
                        }
                    }
                }

                max_ref_idx0 = pc_slice_header->num_ref_idx_l0_active_minus1;
                max_ref_idx1 = pc_slice_header->num_ref_idx_l1_active_minus1;
                for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                    if ((pc_slice_header->num_ref_idx_l0_active_minus1 > 0 ||
                            p_mb->mb_field_decoding_flag != pc_slice_header->field_pic_flag) &&
                            !HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, mbPartIdx) &&
                            (!pc_slice_header->SVCExtFlag || !p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx])) {
                        // ref_idx_l0 te(v) | ae(v)
                        hl_codec_264_bits_write_te(pc_esd->pobj_bits, p_mb->ref_idx_l0[mbPartIdx], max_ref_idx0); // TODO: CABAC
                    }
                }
                for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart ; ++mbPartIdx) {
                    if ((pc_slice_header->num_ref_idx_l1_active_minus1 > 0 ||
                            p_mb->mb_field_decoding_flag != pc_slice_header->field_pic_flag) &&
                            !HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx) &&
                            (!pc_slice_header->SVCExtFlag || !p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx])) {
                        // ref_idx_l1 te(v) | ae(v)
                        hl_codec_264_bits_write_te(pc_esd->pobj_bits, p_mb->ref_idx_l1[mbPartIdx], max_ref_idx1); // TODO: CABAC
                    }
                }
                for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                    if (!HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, mbPartIdx)) {
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l0[mbPartIdx][0].x); // TODO: CABAC
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l0[mbPartIdx][0].y); // TODO: CABAC
                    }
                }
                for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart ; ++mbPartIdx) {
                    if (!HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx)) {
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l1[mbPartIdx][0].x); // TODO: CABAC
                        hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mvd_l1[mbPartIdx][0].y); // TODO: CABAC
                    }
                }
            }
            // endo-of-mb_pred(mb_type)
        }
    } // end-of-!base_mode_flag

    if (pc_slice_header->SVCExtFlag) {
        if (pc_slice_header->ext.svc.adaptive_residual_prediction_flag && !IsSliceHeaderEI(pc_slice_header) &&
                (p_mb->ext.svc.base_mode_flag ||
                 (!HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0) &&
                  !HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0) &&
                  !HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0) &&
                  p_mb->ext.svc.InCropWindow))) {
            // residual_prediction_flag 2 u(1) | ae(v)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->ext.svc.residual_prediction_flag); // TODO: CABAC
        }
    }

    if (u_scan_idx_end >= u_scan_idx_start) {
        if (!HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
            // coded_block_pattern me(v)|ae(v)
            hl_codec_264_bits_write_me(pc_esd->pobj_bits, p_mb->coded_block_pattern, pc_sps->ChromaArrayType, (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0))); // TODO: CABAC

            if (p_mb->CodedBlockPatternLuma > 0 &&
                    pc_pps->transform_8x8_mode_flag &&
                    (p_mb->ext.svc.base_mode_flag ||
                     (p_mb->e_type != HL_CODEC_264_MB_TYPE_I_NXN &&
                      noSubMbPartSizeLessThan8x8Flag &&
                      (p_mb->e_type != HL_CODEC_264_MB_TYPE_B_DIRECT_16X16 ||
                       pc_sps->direct_8x8_inference_flag)))) {
                hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_mb->transform_size_8x8_flag); // TODO: CABAC
            }
        }

        if (p_mb->CodedBlockPatternLuma > 0 || p_mb->CodedBlockPatternChroma > 0 || HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
            // mb_qp_delta se(v)|ae(v)
            hl_codec_264_bits_write_se(pc_esd->pobj_bits, p_mb->mb_qp_delta);
        }

        if (pi_bits_hdr_count) {
            *pi_bits_hdr_count = (int32_t)(hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits) - i_bits_hdr_count);
        }

        if (p_mb->CodedBlockPatternLuma > 0 || p_mb->CodedBlockPatternChroma > 0 || HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
            int32_t i_bits_count_data;
            if (pi_bits_data_count) {
                i_bits_count_data = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits);
            }
            // residual( 0, 15 )
            // residual( scan_idx_start, scan_idx_end )
            err = hl_codec_264_residual_write(p_codec, p_mb, u_scan_idx_start, u_scan_idx_end);
            if (err) {
                goto bail;
            }
            if (pi_bits_data_count) {
                *pi_bits_data_count = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits) - i_bits_count_data;
            }
        }
    } // end-of (scab_idx_end >= scan_idx_start)
    else {
        if (pi_bits_hdr_count) {
            *pi_bits_hdr_count = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits) - i_bits_hdr_count;
        }
    }

    // == end-of- macroblock_layer( )

bail:
    return err;
}

HL_ERROR_T hl_codec_264_mb_print_samples(const hl_codec_264_mb_t* pc_mb, const hl_pixel_t *pc_samples, int32_t i_stride, int32_t i_line)
{
    int32_t mb_x = (i_line == 0 ? pc_mb->xL : pc_mb->xC), i;
    int32_t mb_y = (i_line == 0 ? pc_mb->yL : pc_mb->yC), j;
    int32_t mb_w = (i_line == 0 ? 16 : 8);
    int32_t mb_h = (i_line == 0 ? 16 : 8);
    const char* pc_md5;

    hl_codec_264_mb_get_md5(pc_mb, pc_samples, i_stride, i_line, &pc_md5);

    printf("\n==== MB #%d samples. MD5=%s ====\n", pc_mb->u_addr, pc_md5);

    pc_samples = pc_samples + (mb_y * i_stride) + mb_x;
    for (j = 0; j < mb_h; ++j) {
        for (i = 0; i < mb_w; ++i) {
            printf("%3d ", pc_samples[i]);
            if (((i + 1) & 3) == 0) {
                printf(" | ");
            }
        }
        if (((j + 1) & 3) == 0) {
            printf("\n");
            for (i = 0; i < mb_w + 3; ++i) {
                printf("----");
            }
        }
        printf("\n");
        pc_samples += i_stride;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_mb_get_md5(const hl_codec_264_mb_t* pc_mb, const hl_pixel_t *pc_samples, int32_t i_stride, int32_t i_line, const char** ppc_md5)
{
    int32_t mb_x = (i_line == 0 ? pc_mb->xL : pc_mb->xC);
    int32_t mb_y = (i_line == 0 ? pc_mb->yL : pc_mb->yC), j;
    int32_t mb_w = (i_line == 0 ? 16 : 8);
    int32_t mb_h = (i_line == 0 ? 16 : 8);
    static hl_md5string_t md5_result;
    static hl_md5context_t md5_ctx;
    static hl_md5digest_t md5_digest;

    pc_samples = pc_samples + (mb_y * i_stride) + mb_x;
    hl_md5init(&md5_ctx);
    for (j = 0; j < mb_h; ++j) {
        hl_md5update(&md5_ctx, (uint8_t const *)pc_samples, mb_w);
        pc_samples += i_stride;
    }
    hl_md5final(md5_digest, &md5_ctx);
    hl_str_from_hex(md5_digest, HL_MD5_DIGEST_SIZE, md5_result);

    *ppc_md5 = md5_result;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_mb_print_md5(const hl_codec_264_mb_t* pc_mb, const hl_pixel_t *pc_samples, int32_t i_stride, int32_t i_line)
{
    int32_t mb_x = (i_line == 0 ? pc_mb->xL : pc_mb->xC);
    int32_t mb_y = (i_line == 0 ? pc_mb->yL : pc_mb->yC), j;
    int32_t mb_w = (i_line == 0 ? 16 : 8);
    int32_t mb_h = (i_line == 0 ? 16 : 8);
    hl_md5string_t md5_result;
    hl_md5context_t md5_ctx;
    hl_md5digest_t md5_digest;

    pc_samples = pc_samples + (mb_y * i_stride) + mb_x;
    hl_md5init(&md5_ctx);
    for (j = 0; j < mb_h; ++j) {
        hl_md5update(&md5_ctx, (uint8_t const *)pc_samples, mb_w);
        pc_samples += i_stride;
    }
    hl_md5final(md5_digest, &md5_ctx);
    hl_str_from_hex(md5_digest, HL_MD5_DIGEST_SIZE, md5_result);

    HL_DEBUG_INFO("MB #%u MD5=%s", pc_mb->u_addr, md5_result);

    return HL_ERROR_SUCCESS;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_mb_t" ***/
static hl_object_t* hl_codec_264_mb_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_mb_t *p_mb = (hl_codec_264_mb_t*)self;
    if (p_mb) {

    }
    return self;
}
static hl_object_t* hl_codec_264_mb_dtor(hl_object_t * self)
{
    hl_codec_264_mb_t *p_mb = (hl_codec_264_mb_t*)self;
    if (p_mb) {

    }
    return self;
}
static int hl_codec_264_mb_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_mb_def_s = {
    sizeof(hl_codec_264_mb_t),
    hl_codec_264_mb_ctor,
    hl_codec_264_mb_dtor,
    hl_codec_264_mb_cmp,
    HL_TRUE, // aligned
};
const hl_object_def_t *hl_codec_264_mb_def_t = &hl_codec_264_mb_def_s;
