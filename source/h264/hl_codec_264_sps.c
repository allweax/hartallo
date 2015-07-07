#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_rbsp.h"
#include "hartallo/h264/hl_codec_264_vui.h"
#include "hartallo/h264/hl_codec_264_hrd.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/hl_frame.h"
#include "hartallo/hl_debug.h"

HL_ERROR_T hl_codec_264_nal_sps_create(unsigned u_ref_idc, enum HL_CODEC_264_NAL_TYPE_E e_type, struct hl_codec_264_nal_sps_s** pp_sps)
{
    extern const hl_object_def_t *hl_codec_264_nal_sps_def_t;
    if (!pp_sps) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_sps = hl_object_create(hl_codec_264_nal_sps_def_t);
    if (!*pp_sps) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    hl_codec_264_nal_init((hl_codec_264_nal_t*)*pp_sps, e_type, u_ref_idc);
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _hl_codec_264_nal_sps_svc_create(hl_codec_264_nal_sps_svc_t** pp_sps_svc)
{
    extern const hl_object_def_t *hl_codec_264_nal_sps_svc_def_t;
    if (!pp_sps_svc) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_sps_svc = hl_object_create(hl_codec_264_nal_sps_svc_def_t);
    if (!*pp_sps_svc) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}



// must be called after successful parsing of the AVC part
static HL_ERROR_T hl_codec_264_nal_sps_derive(hl_codec_264_nal_sps_t* p_sps, struct hl_codec_264_s* p_codec)
{
    register int32_t i;

    if (!p_sps->seq_scaling_matrix_present_flag) {
        register uint32_t j;
        // Flat_4x4_16 and Flat_8x8_16
        // FIXME: SIMD
        for (i=0; i<6; ++i) {
            for (j=0; j<16; j+=4) {
                p_sps->ScalingList4x4[i][j] = 16;
                p_sps->ScalingList4x4[i][j + 1] = 16;
                p_sps->ScalingList4x4[i][j + 2] = 16;
                p_sps->ScalingList4x4[i][j + 3] = 16;
            }
            for (j=0; j<64; j+=4) {
                p_sps->ScalingList8x8[i][j] = 16;
                p_sps->ScalingList8x8[i][j + 1] = 16;
                p_sps->ScalingList8x8[i][j + 2] = 16;
                p_sps->ScalingList8x8[i][j + 3] = 16;
            }
        }
    }
    // When pic_order_cnt_type is equal to 1, the variable ExpectedDeltaPerPicOrderCntCycle is derived by (7-11)
    if (p_sps->pic_order_cnt_type == 1) {
        p_sps->ExpectedDeltaPerPicOrderCntCycle = 0;
        for (i = 0; i < (int32_t)p_sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
            p_sps->ExpectedDeltaPerPicOrderCntCycle += p_sps->offset_for_ref_frame[i];
        }
    }
    //Depending on the value of separate_colour_plane_flag, the value of the variable ChromaArrayType is assigned as follows.
    //– If separate_colour_plane_flag is equal to 0, ChromaArrayType is set equal to chroma_format_idc.
    //– Otherwise (separate_colour_plane_flag is equal to 1), ChromaArrayType is set equal to 0.
    if (p_sps->separate_colour_plane_flag == 0) {
        p_sps->ChromaArrayType = p_sps->chroma_format_idc;
    }
    else {
        p_sps->ChromaArrayType = 0;
    }
    // If chroma_format_idc is equal to 0 (monochrome) or separate_colour_plane_flag is equal to 1, MbWidthC and
    // MbHeightC are both equal to 0.
    if (p_sps->chroma_format_idc == 0 || p_sps->separate_colour_plane_flag == 1) {
        p_sps->MbHeightC = p_sps->MbWidthC = 0;
        p_sps->ChromaFormat = HL_CODEC_264_CHROMAFORMAT_MONOCHROME;
    }
    else {
        // Table 6-1
        if (p_sps->chroma_format_idc == 1 && p_sps->separate_colour_plane_flag == 0) {
            p_sps->SubWidthC = 2;
            p_sps->SubHeightC = 2;
            p_sps->SubWidthC_TrailingZeros = 1;
            p_sps->SubHeightC_TrailingZeros = 1;
            p_sps->ChromaFormat = HL_CODEC_264_CHROMAFORMAT_420;
        }
        else if (p_sps->chroma_format_idc == 2 && p_sps->separate_colour_plane_flag == 0) {
            p_sps->SubWidthC = 2;
            p_sps->SubHeightC = 1;
            p_sps->SubWidthC_TrailingZeros = 1;
            p_sps->SubHeightC_TrailingZeros = 0;
            p_sps->ChromaFormat = HL_CODEC_264_CHROMAFORMAT_422;
        }
        else if (p_sps->chroma_format_idc == 3 && p_sps->separate_colour_plane_flag == 0) {
            p_sps->SubWidthC = 1;
            p_sps->SubHeightC = 1;
            p_sps->SubWidthC_TrailingZeros = 0;
            p_sps->SubHeightC_TrailingZeros = 0;
            p_sps->ChromaFormat = HL_CODEC_264_CHROMAFORMAT_444;
        }
        p_sps->MbWidthC = 16 >> p_sps->SubWidthC_TrailingZeros;// (6-1)
        p_sps->MbHeightC = 16 >> p_sps->SubHeightC_TrailingZeros;// (6-2)
    }

    p_sps->uLog2MaxFrameNum = p_sps->log2_max_frame_num_minus4 + 4;
    p_sps->uMaxFrameNum = 1 << p_sps->uLog2MaxFrameNum;
    p_sps->uPicWidthInMbs = p_sps->uFrameWidthInMbs = p_sps->pic_width_in_mbs_minus1 + 1;
    if (p_sps->frame_mbs_only_flag) {
        p_sps->uFrameHeightInMbs = p_sps->pic_height_in_map_units_minus1 + 1;
    }
    else {
        p_sps->uFrameHeightInMbs = (p_sps->pic_height_in_map_units_minus1 + 1) << 1;
    }
    p_sps->uPicHeightInMapUnit = p_sps->pic_height_in_map_units_minus1 + 1;
    p_sps->uPicSizeInMapUnits = (p_sps->uPicWidthInMbs * p_sps->uPicHeightInMapUnit);

    // BitDepthY = 8 + bit_depth_luma_minus8 (7-2)
    p_sps->BitDepthY = 8 + p_sps->bit_depth_luma_minus8;
    // QpBdOffsetY = 6 * bit_depth_luma_minus8 (7-3)
    p_sps->QpBdOffsetY = 6 * p_sps->bit_depth_luma_minus8;//Should be zero for baseline
    // BitDepthC = 8 + bit_depth_chroma_minus8 (7-4)
    p_sps->BitDepthC = 8 + p_sps->bit_depth_chroma_minus8;
    // QpBdOffsetC = 6 * bit_depth_chroma_minus8 (7-5)
    p_sps->QpBdOffsetC = 6 * p_sps->bit_depth_chroma_minus8;

    // "PixelMaxValueY" and "PixelMaxValueC" are aligned on 16 bytes (for SSE) and this is why it's insided the codec instead of SPS (SPS struct not aligned).
    // PixelMaxValueY = ((1 << BitDepthY) - 1)
    p_codec->PixelMaxValueY[0] = p_codec->PixelMaxValueY[1] = p_codec->PixelMaxValueY[2] = p_codec->PixelMaxValueY[3] = ((1 << p_sps->BitDepthY) - 1);
    // PixelMaxValueC = ((1 << BitDepthC) - 1)
    p_codec->PixelMaxValueC[0] = p_codec->PixelMaxValueC[1] = p_codec->PixelMaxValueC[2] = p_codec->PixelMaxValueC[3] = ((1 << p_sps->BitDepthC) - 1);

    // Scalable Video Coding (SVC)
    if (p_sps->p_svc) {
        if (p_sps->p_svc->extended_spatial_scalability_idc != HL_CODEC_264_ESS_SEQ) {
            p_sps->p_svc->seq_ref_layer_chroma_phase_x_plus1_flag = p_sps->p_svc->chroma_phase_x_plus1_flag;
            p_sps->p_svc->seq_ref_layer_chroma_phase_y_plus1 = p_sps->p_svc->chroma_phase_y_plus1;
        }
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_nal_sps_decode(HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc, struct hl_codec_264_s* p_codec, hl_codec_264_nal_sps_t** pp_sps)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_nal_sps_t* p_sps;
    unsigned profile_idc;
    register hl_size_t i;

    if (!pp_sps || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    //profile_idc u(8)
    profile_idc = hl_codec_264_bits_read_u(p_codec->pobj_bits, 8);
    switch(profile_idc) {
    case HL_CODEC_264_PROFILE_BASELINE:
    case HL_CODEC_264_PROFILE_MAIN:
    case HL_CODEC_264_PROFILE_EXTENDED:
    case HL_CODEC_264_PROFILE_HIGH:
    case HL_CODEC_264_PROFILE_HIGH10:
    case HL_CODEC_264_PROFILE_HIGH422:
    case HL_CODEC_264_PROFILE_HIGH444:
    case HL_CODEC_264_PROFILE_CAVLC444:
    case HL_CODEC_264_PROFILE_BASELINE_SVC:
    case HL_CODEC_264_PROFILE_HIGH_SVC: {
        err = hl_codec_264_nal_sps_create(u_ref_idc, e_type, pp_sps);
        if (err) {
            goto bail;
        }
        p_sps = *pp_sps;
        p_sps->profile_idc = profile_idc;
        break;
    }
    default: {
        HL_DEBUG_ERROR("%d not supported as valid profile", profile_idc);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    }

    // constraint_set(x)_flag u(1) and (x) from 0 to 5
    p_sps->constraint_set0_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    p_sps->constraint_set1_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    p_sps->constraint_set2_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    p_sps->constraint_set3_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    p_sps->constraint_set4_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    p_sps->constraint_set5_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // reserved_zero_2bits u(2)
    p_sps->reserved_zero_2bits = hl_codec_264_bits_read_u(p_codec->pobj_bits, 2); // Must be zeros
    // level_idc u(8)
    p_sps->level_idc = hl_codec_264_bits_read_u(p_codec->pobj_bits, 8);
    if (p_sps->level_idc > HL_CODEC_264_LEVEL_51) {
        HL_DEBUG_ERROR("Invalid level_idc (%d)", p_sps->level_idc);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    // seq_parameter_set_id ue(v)
    p_sps->seq_parameter_set_id = hl_codec_264_bits_read_ue(p_codec->pobj_bits);

    if (p_sps->seq_parameter_set_id >= HL_CODEC_264_SPS_MAX_COUNT) {
        HL_DEBUG_ERROR("Invalid seq parameter set id (%d)", p_sps->seq_parameter_set_id);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }

    if (p_sps->profile_idc == 100 || p_sps->profile_idc == 110 ||
            p_sps->profile_idc == 122 || p_sps->profile_idc == 244 || p_sps->profile_idc == 44 ||
            p_sps->profile_idc == 83 || p_sps->profile_idc == 86 || p_sps->profile_idc == 118 ||
            p_sps->profile_idc == 128) {
        register hl_size_t u_seq_scaling_list_present_flag_count = 8;
        // chroma_format_idc ue(v)
        p_sps->chroma_format_idc = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        if (p_sps->chroma_format_idc == 3) {
            // separate_colour_plane_flag u(1)
            p_sps->separate_colour_plane_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            u_seq_scaling_list_present_flag_count = 12;
        }
        // bit_depth_luma_minus8 ue(v)
        p_sps->bit_depth_luma_minus8 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        // bit_depth_chroma_minus8 ue(v)
        p_sps->bit_depth_chroma_minus8 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        // qpprime_y_zero_transform_bypass_flag u(1)
        p_sps->qpprime_y_zero_transform_bypass_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        // seq_scaling_matrix_present_flag u(1)
        p_sps->seq_scaling_matrix_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        if (p_sps->seq_scaling_matrix_present_flag) {
            for (i = 0; i < u_seq_scaling_list_present_flag_count; i++) {
                // seq_scaling_list_present_flag[i] u(1)
                p_sps->seq_scaling_list_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                if(p_sps->seq_scaling_list_present_flag[i]) {
                    if (i < 6) { //4x4
                        /*err = */hl_codec_264_rbsp_avc_scaling_list_read(p_codec->pobj_bits, p_sps->ScalingList4x4[i], 16, p_sps->UseDefaultScalingMatrix4x4Flag[i]);
                    }
                    else { //8x8
                        /*err = */hl_codec_264_rbsp_avc_scaling_list_read(p_codec->pobj_bits, p_sps->ScalingList8x8[i-6], 64, p_sps->UseDefaultScalingMatrix8x8Flag[i-6]);
                    }
                }
                else { // Table 7-2 – Assignment of mnemonic names to scaling list indices and specification of fall-back rule
                    static const uint32_t u_size_of_int32x16 = sizeof(int32_t) << 4;
                    static const uint32_t u_size_of_int32x64 = sizeof(int32_t) << 6;
                    HL_DEBUG_ERROR("Not checked code");
                    err = HL_ERROR_NOT_IMPLEMENTED;
                    goto bail;
                    switch (i) {
                    case 0:
                        memcpy(p_sps->ScalingList4x4[i], Scaling_Default_4x4_Intra, u_size_of_int32x16);
                        break;
                    case 1:
                        memcpy(p_sps->ScalingList4x4[i], p_sps->ScalingList4x4[0], u_size_of_int32x16);
                        break;
                    case 2:
                        memcpy(p_sps->ScalingList4x4[i], p_sps->ScalingList4x4[1], u_size_of_int32x16);
                        break;
                    case 3:
                        memcpy(p_sps->ScalingList4x4[i], Scaling_Default_4x4_Inter, u_size_of_int32x16);
                        break;
                    case 4:
                        memcpy(p_sps->ScalingList4x4[i], p_sps->ScalingList4x4[3], u_size_of_int32x16);
                        break;
                    case 5:
                        memcpy(p_sps->ScalingList4x4[i], p_sps->ScalingList4x4[4], u_size_of_int32x16);
                        break;

                    case 6:
                        memcpy(p_sps->ScalingList8x8[i], Scaling_Default_8x8_Intra, u_size_of_int32x64);
                        break;
                    case 7:
                        memcpy(p_sps->ScalingList8x8[i], Scaling_Default_8x8_Inter, u_size_of_int32x64);
                        break;
                    case 8:
                        memcpy(p_sps->ScalingList8x8[i], p_sps->ScalingList8x8[6], u_size_of_int32x64);
                        break;
                    case 9:
                        memcpy(p_sps->ScalingList8x8[i], p_sps->ScalingList8x8[7], u_size_of_int32x64);
                        break;//FIXME: out of range
                    case 10:
                        memcpy(p_sps->ScalingList8x8[i], p_sps->ScalingList8x8[8], u_size_of_int32x64);
                        break;//FIXME: out of range
                    case 11:
                        memcpy(p_sps->ScalingList8x8[i], p_sps->ScalingList8x8[9], u_size_of_int32x64);
                        break;//FIXME: out of range
                    }
                }
            }
        }
    }
    else {
        // When chroma_format_idc is not present, it shall be inferred to be equal to 1 (4:2:0 chroma format)
        p_sps->chroma_format_idc = 1;
    }

    // log2_max_frame_num_minus4 ue(v)
    p_sps->log2_max_frame_num_minus4 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // pic_order_cnt_type ue(v)
    p_sps->pic_order_cnt_type = hl_codec_264_bits_read_ue(p_codec->pobj_bits);

    if (p_sps->pic_order_cnt_type == 0) {
        // log2_max_pic_order_cnt_lsb_minus4 ue(v)
        p_sps->log2_max_pic_order_cnt_lsb_minus4 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    }
    else if (p_sps->pic_order_cnt_type == 1) {
        // delta_pic_order_always_zero_flag u(1)
        p_sps->delta_pic_order_always_zero_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        // offset_for_non_ref_pic se(v)
        p_sps->offset_for_non_ref_pic = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        // offset_for_top_to_bottom_field se(v)
        p_sps->offset_for_top_to_bottom_field = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        // num_ref_frames_in_pic_order_cnt_cycle ue(v)
        p_sps->num_ref_frames_in_pic_order_cnt_cycle = hl_codec_264_bits_read_ue(p_codec->pobj_bits);

        for (i = 0; i < p_sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
            // offset_for_ref_frame[ i ] se(v)
            p_sps->offset_for_ref_frame[i] = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        }
    }

    // max_num_ref_frames ue(v)
    p_sps->max_num_ref_frames = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // gaps_in_frame_num_value_allowed_flag u(1)
    p_sps->gaps_in_frame_num_value_allowed_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // pic_width_in_mbs_minus1 ue(v)
    p_sps->pic_width_in_mbs_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // pic_height_in_map_units_minus1 ue(v)
    p_sps->pic_height_in_map_units_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // frame_mbs_only_flag u(1)
    p_sps->frame_mbs_only_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);

    if (!p_sps->frame_mbs_only_flag) {
        // mb_adaptive_frame_field_flag u(1)
        p_sps->mb_adaptive_frame_field_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    }

    // direct_8x8_inference_flag u(1)
    p_sps->direct_8x8_inference_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // frame_cropping_flag u(1)
    p_sps->frame_cropping_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);

    if (p_sps->frame_cropping_flag) {
        // frame_crop_left_offset ue(v)
        p_sps->frame_crop_left_offset = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        // frame_crop_right_offset ue(v)
        p_sps->frame_crop_right_offset = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        // frame_crop_top_offset ue(v)
        p_sps->frame_crop_top_offset = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        // frame_crop_bottom_offset ue(v)
        p_sps->frame_crop_bottom_offset = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    }

    // vui_parameters_present_flag u(1)
    p_sps->vui_parameters_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    if (p_sps->vui_parameters_present_flag) {
        // vui_parameters( )
        err = hl_codec_264_vui_create(&p_sps->p_vui);
        if (err) {
            goto bail;
        }
        err = hl_codec_264_rbsp_avc_vui_parameters_read(p_sps, p_codec->pobj_bits);
        if (err) {
            goto bail;
        }
    }

    // Guess useful values
    err = hl_codec_264_nal_sps_derive(p_sps, p_codec);
    if (err) {
        goto bail;
    }

    /*=== Scalable Video Coding (SVC) ===*/

    if (e_type == HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET) {
        // 7.3.2.1.3 Subset sequence parameter set RBSP syntax
        if (p_sps->profile_idc == HL_CODEC_264_PROFILE_BASELINE_SVC || p_sps->profile_idc == HL_CODEC_264_PROFILE_HIGH_SVC) {
            if (!p_sps->p_svc) {
                err = _hl_codec_264_nal_sps_svc_create(&p_sps->p_svc);
                if (err) {
                    goto bail;
                }
            }
            p_sps->b_svc = HL_TRUE;

            // seq_parameter_set_svc_extension( ) /* G.7.3.2.1.4 Sequence parameter set SVC extension syntax */
            // inter_layer_deblocking_filter_control_present_flag u(1)
            p_sps->p_svc->inter_layer_deblocking_filter_control_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            // extended_spatial_scalability_idc u(2)
            p_sps->p_svc->extended_spatial_scalability_idc = hl_codec_264_bits_read_u(p_codec->pobj_bits, 2);
            if (p_sps->ChromaArrayType == 1 || p_sps->ChromaArrayType == 2) {
                // chroma_phase_x_plus1_flag u(1)
                p_sps->p_svc->chroma_phase_x_plus1_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                if (p_sps->ChromaArrayType == 1) {
                    // chroma_phase_y_plus1 u(2)
                    p_sps->p_svc->chroma_phase_y_plus1 = hl_codec_264_bits_read_u(p_codec->pobj_bits, 2);
                }
            }
            if (p_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_SEQ) {
                if (p_sps->ChromaArrayType > 0) {
                    // seq_ref_layer_chroma_phase_x_plus1_flag u(1)
                    p_sps->p_svc->seq_ref_layer_chroma_phase_x_plus1_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    // seq_ref_layer_chroma_phase_y_plus1 u(2)
                    p_sps->p_svc->seq_ref_layer_chroma_phase_y_plus1 = hl_codec_264_bits_read_u(p_codec->pobj_bits, 2);
                }
                //seq_scaled_ref_layer_left_offset se(v)
                p_sps->p_svc->seq_scaled_ref_layer_left_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                // seq_scaled_ref_layer_top_offset se(v)
                p_sps->p_svc->seq_scaled_ref_layer_top_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                // seq_scaled_ref_layer_right_offset se(v)
                p_sps->p_svc->seq_scaled_ref_layer_right_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                // seq_scaled_ref_layer_bottom_offset se(v)
                p_sps->p_svc->seq_scaled_ref_layer_bottom_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
            }
            else {
                p_sps->p_svc->seq_ref_layer_chroma_phase_x_plus1_flag = p_sps->p_svc->chroma_phase_x_plus1_flag;
                p_sps->p_svc->seq_ref_layer_chroma_phase_y_plus1 = p_sps->p_svc->chroma_phase_y_plus1;
            }
            // seq_tcoeff_level_prediction_flag u(1)
            p_sps->p_svc->seq_tcoeff_level_prediction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            if (p_sps->p_svc->seq_tcoeff_level_prediction_flag) {
                // adaptive_tcoeff_level_prediction_flag u(1)
                p_sps->p_svc->adaptive_tcoeff_level_prediction_flag =  hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            }
            p_sps->p_svc->slice_header_restriction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            // end-of seq_parameter_set_svc_extension( )

            // svc_vui_parameters_present_flag u(1)
            p_sps->p_svc->svc_vui_parameters_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            if (p_sps->p_svc->svc_vui_parameters_present_flag) {
                // svc_vui_parameters_extension( ) /* G.14.1 SVC VUI parameters extension syntax */
                // vui_ext_num_entries_minus1 ue(v)
                uint32_t vui_ext_num_entries_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                if (vui_ext_num_entries_minus1 >= HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT) {
                    HL_DEBUG_ERROR("Invalid 'vui_ext_num_entries_minus1'");
                    err = HL_ERROR_INVALID_BITSTREAM;
                    goto bail;
                }
                err = hl_codec_264_vui_svc_create(&p_sps->p_svc->p_vui);
                if (err) {
                    goto bail;
                }
                p_sps->p_svc->p_vui->vui_ext_num_entries_minus1 = vui_ext_num_entries_minus1;
                for (i = 0; i <= vui_ext_num_entries_minus1; i++) {
                    // vui_ext_dependency_id [ i ] u(3)
                    p_sps->p_svc->p_vui->vui_ext_dependency_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                    // vui_ext_quality_id[ i ]  u(4)
                    p_sps->p_svc->p_vui->vui_ext_quality_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 4);
                    // vui_ext_temporal_id[ i ] u(3)
                    p_sps->p_svc->p_vui->vui_ext_temporal_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                    // vui_ext_timing_info_present_flag[ i ] u(1)
                    p_sps->p_svc->p_vui->vui_ext_timing_info_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    if (p_sps->p_svc->p_vui->vui_ext_timing_info_present_flag[i]) {
                        // vui_ext_num_units_in_tick[ i ] u(32)
                        p_sps->p_svc->p_vui->vui_ext_num_units_in_tick[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 32);
                        // vui_ext_time_scale[ i ] u(32)
                        p_sps->p_svc->p_vui->vui_ext_time_scale[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 32);
                        // vui_ext_fixed_frame_rate_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_fixed_frame_rate_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        // vui_ext_nal_hrd_parameters_present_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_nal_hrd_parameters_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        if (p_sps->p_svc->p_vui->vui_ext_nal_hrd_parameters_present_flag[i]) {
                            err = hl_codec_264_hrd_create(&p_sps->p_svc->p_vui->hrds_nal[i]);
                            if (err) {
                                goto bail;
                            }
                            err = hl_codec_264_rbsp_avc_hrd_parameters_read(p_sps->p_svc->p_vui->hrds_nal[i], p_codec->pobj_bits);
                            if (err) {
                                goto bail;
                            }
                        }
                        // vui_ext_vcl_hrd_parameters_present_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_vcl_hrd_parameters_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        if (p_sps->p_svc->p_vui->vui_ext_vcl_hrd_parameters_present_flag[i]) {
                            err = hl_codec_264_hrd_create(&p_sps->p_svc->p_vui->hrds_vcl[i]);
                            if (err) {
                                goto bail;
                            }
                            err = hl_codec_264_rbsp_avc_hrd_parameters_read(p_sps->p_svc->p_vui->hrds_vcl[i], p_codec->pobj_bits);
                            if (err) {
                                goto bail;
                            }
                        }
                        if (p_sps->p_svc->p_vui->vui_ext_nal_hrd_parameters_present_flag[i] || p_sps->p_svc->p_vui->vui_ext_vcl_hrd_parameters_present_flag[i]) {
                            // vui_ext_low_delay_hrd_flag[ i ] u(1)
                            p_sps->p_svc->p_vui->vui_ext_low_delay_hrd_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        }
                        // vui_ext_pic_struct_present_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_pic_struct_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    }
                }
            }
        }
        else if (p_sps->profile_idc == 118 || p_sps->profile_idc == 128) {
            HL_DEBUG_ERROR("H.264 MVC not supported yet");
            err = HL_ERROR_INVALID_BITSTREAM;
            goto bail;
        }

        // additional_extension2_flag u(1)
        if (/* additional_extension2_flag */hl_codec_264_bits_read_u1(p_codec->pobj_bits)) {
            HL_DEBUG_WARN("additional_extension2_flag is not equal to zero...ignoring bytes");
            if (0) { // while not needed as the decoder expect a SINGLE NAL as input
                while (hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits)) {
                    // additional_extension2_data_flag u(1)
                    hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                }
            }
        }
    } // end-of subsequent SPS parsing

    // rbsp_trailing_bits( )
    err = hl_codec_264_rbsp_avc_trailing_bits_read(p_codec->pobj_bits);
    if (err) {
        goto bail;
    }

bail:
    if (err && pp_sps) {
        HL_OBJECT_SAFE_FREE(*pp_sps);
    }
    return err;
}

HL_ERROR_T hl_codec_264_nal_sps_encode(struct hl_codec_264_nal_sps_s* p_sps, struct hl_codec_264_s* p_codec)
{
    HL_ERROR_T err;
    const hl_codec_264_layer_t* pc_layer;
    int32_t max_num_ref_frames;
    hl_bool_t b_svc_enhacement_layer;
    hl_size_t u_frame_width, u_frame_height;

    if (!p_sps || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // Set default active layer
    pc_layer = p_codec->layers.pc_active;

    // Check whether this is for an enhanced layer
    b_svc_enhacement_layer = p_codec->encoder.b_svc_enabled && (HL_CODEC_264_NAL(p_sps)->e_type == HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET);

    if (p_codec->encoder.b_svc_enabled) {
        if (p_sps->seq_parameter_set_id >= p_codec->pc_base->layers_active_count) {
            HL_DEBUG_ERROR("No valid SVC layer definition could be mapped to SPS with id = %d", p_sps->seq_parameter_set_id);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        u_frame_width = p_codec->pc_base->layers[p_sps->seq_parameter_set_id].u_width;
        u_frame_height = p_codec->pc_base->layers[p_sps->seq_parameter_set_id].u_height;
    }
    else {
        u_frame_width = p_codec->encoder.pc_frame->data_width[0];
        u_frame_height = p_codec->encoder.pc_frame->data_height[0];
    }

    // Guess level
    if (p_codec->pc_base->level <= 0) {
        err = hl_codec_264_utils_guess_level(u_frame_width, u_frame_height, &p_sps->level_idc);
        if (err) {
            return err;
        }
    }
    else {
        p_sps->level_idc = p_codec->pc_base->level;
    }

    switch (p_codec->pc_base->profile) {
    case HL_CODEC_PROFILE_H264_BASELINE_SVC: {
        p_sps->profile_idc = HL_CODEC_264_PROFILE_BASELINE_SVC;
        p_sps->constraint_set0_flag = 1;
        p_sps->constraint_set1_flag = 1;
        p_sps->constraint_set2_flag = 1;
        p_sps->constraint_set3_flag = 0;
        p_sps->constraint_set4_flag = 0;
        p_sps->constraint_set5_flag = 0;
        p_sps->chroma_format_idc = 1;
        p_sps->bit_depth_luma_minus8 = 0;
        p_sps->bit_depth_chroma_minus8 = 0;
        p_sps->separate_colour_plane_flag = 0;
        p_sps->qpprime_y_zero_transform_bypass_flag = 0;
        p_sps->frame_mbs_only_flag = 1;
        break;
    }
    default: { /* Base Line */
        p_sps->profile_idc = b_svc_enhacement_layer ? HL_CODEC_264_PROFILE_BASELINE_SVC : HL_CODEC_264_PROFILE_BASELINE;
        p_sps->constraint_set0_flag = 1;
        p_sps->constraint_set1_flag = b_svc_enhacement_layer ? 0 : 1;
        p_sps->constraint_set2_flag = b_svc_enhacement_layer ? 0 : 1;
        p_sps->constraint_set3_flag = 0;
        p_sps->constraint_set4_flag = 0;
        p_sps->constraint_set5_flag = 0;
        p_sps->chroma_format_idc = 1;
        p_sps->bit_depth_luma_minus8 = 0;
        p_sps->bit_depth_chroma_minus8 = 0;
        p_sps->separate_colour_plane_flag = 0;
        p_sps->qpprime_y_zero_transform_bypass_flag = 0;
        p_sps->frame_mbs_only_flag = 1;
        break;
    }
    }

    p_sps->reserved_zero_2bits = 0;
    p_sps->seq_scaling_matrix_present_flag = 0;

    p_sps->pic_order_cnt_type = 2;
    p_sps->log2_max_pic_order_cnt_lsb_minus4 = 0;
    p_sps->log2_max_frame_num_minus4=4;
    p_sps->log2_max_pic_order_cnt_lsb_minus4=0;
    p_sps->max_num_ref_frames=p_codec->pc_base->max_ref_frame; // will be updated later
    p_sps->gaps_in_frame_num_value_allowed_flag=0;
    p_sps->pic_width_in_mbs_minus1=(uint32_t)(u_frame_width >> 4) - 1;
    p_sps->pic_height_in_map_units_minus1=(uint32_t)(u_frame_height >> 4) - 1;
    p_sps->direct_8x8_inference_flag=0;
    p_sps->frame_cropping_flag=0;
    p_sps->vui_parameters_present_flag=0;

    // Derive() some useful values
    err = hl_codec_264_nal_sps_derive(p_sps, p_codec);
    if (err) {
        return err;
    }

    // Update "max_num_ref_frames" now that "uPicWidthInMbs" and "uFrameHeightInMbs" are known
    max_num_ref_frames = MaxDpbMbs[HL_CODEC_264_LEVEL_TO_ZERO_BASED_INDEX[p_sps->level_idc]] / (p_sps->uPicWidthInMbs * p_sps->uFrameHeightInMbs);
    p_sps->max_num_ref_frames = HL_MATH_MIN(max_num_ref_frames, p_codec->pc_base->max_ref_frame);

    //== Encode fields==//

    // u_forbidden_zero_bit f(1)
    hl_codec_264_bits_write_f1(p_codec->pobj_bits, HL_CODEC_264_NAL(p_sps)->u_forbidden_zero_bit);
    // ref_idc u(2)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, HL_CODEC_264_NAL(p_sps)->u_ref_idc, 2);
    // nal_unit_tye u(5)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, HL_CODEC_264_NAL(p_sps)->e_type, 5);

    //profile_idc u(8)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, p_sps->profile_idc, 8);
    // constraint_set(x)_flag u(1) and (x) from 0 to 5
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->constraint_set0_flag);
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->constraint_set1_flag);
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->constraint_set2_flag);
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->constraint_set3_flag);
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->constraint_set4_flag);
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->constraint_set5_flag);
    // reserved_zero_2bits u(2)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, p_sps->reserved_zero_2bits, 2); // Must be zeros
    // level_idc u(8)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, p_sps->level_idc, 8);
    // seq_parameter_set_id ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->seq_parameter_set_id);

    if (p_sps->seq_parameter_set_id >= HL_CODEC_264_SPS_MAX_COUNT) { //must never happen ...but who know?
        HL_DEBUG_ERROR("Invalid seq parameter set id (%d)", p_sps->seq_parameter_set_id);
        return HL_ERROR_INVALID_BITSTREAM;
    }

    if (p_sps->profile_idc == 100 || p_sps->profile_idc == 110 ||
            p_sps->profile_idc == 122 || p_sps->profile_idc == 244 || p_sps->profile_idc == 44 ||
            p_sps->profile_idc == 83 || p_sps->profile_idc == 86 || p_sps->profile_idc == 118 ||
            p_sps->profile_idc == 128) {
        register hl_size_t u_seq_scaling_list_present_flag_count = 8;
        // chroma_format_idc ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->chroma_format_idc);
        if (p_sps->chroma_format_idc == 3) {
            // separate_colour_plane_flag u(1)
            hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->separate_colour_plane_flag);
            u_seq_scaling_list_present_flag_count = 12;
        }
        // bit_depth_luma_minus8 ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->bit_depth_luma_minus8);
        // bit_depth_chroma_minus8 ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->bit_depth_chroma_minus8);
        // qpprime_y_zero_transform_bypass_flag u(1)
        hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->qpprime_y_zero_transform_bypass_flag);
        // seq_scaling_matrix_present_flag u(1)
        hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->seq_scaling_matrix_present_flag);
        if (p_sps->seq_scaling_matrix_present_flag) {
            hl_size_t i;
            for (i = 0; i < u_seq_scaling_list_present_flag_count; i++) {
                // seq_scaling_list_present_flag[i] u(1)
                hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->seq_scaling_list_present_flag[i]);
                if (p_sps->seq_scaling_list_present_flag[i]) {
                    if (i < 6) { //4x4
                        err = hl_codec_264_rbsp_avc_scaling_list_write(p_codec->pobj_bits, p_sps->ScalingList4x4[i], 16, p_sps->UseDefaultScalingMatrix4x4Flag[i]);
                        if (err) {
                            return err;
                        }
                    }
                    else { //8x8
                        err = hl_codec_264_rbsp_avc_scaling_list_write(p_codec->pobj_bits, p_sps->ScalingList8x8[i-6], 64, p_sps->UseDefaultScalingMatrix8x8Flag[i-6]);
                        if (err) {
                            return err;
                        }
                    }
                }
                else { // Table 7-2 – Assignment of mnemonic names to scaling list indices and specification of fall-back rule
                    HL_DEBUG_ERROR("Not implemented yet");
                    return HL_ERROR_NOT_IMPLEMENTED;
                }
            }
        }
    }

    if(!p_sps->seq_scaling_matrix_present_flag) {
#if 0
        register uint32_t i, j;
        // Flat_4x4_16 and Flat_8x8_16
        for(i=0; i<6; ++i) {
            for(j=0; j<16; ++j) {
                p_sps->ScalingList4x4[i][j] = 16;
            }
            for(j=0; j<64; ++j) {
                p_sps->ScalingList8x8[i][j] = 16;
            }
        }
#endif
    }

    // log2_max_frame_num_minus4 ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->log2_max_frame_num_minus4);
    // pic_order_cnt_type ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->pic_order_cnt_type);

    if(p_sps->pic_order_cnt_type == 0) {
        // log2_max_pic_order_cnt_lsb_minus4 ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->log2_max_pic_order_cnt_lsb_minus4);
    }
    else if(p_sps->pic_order_cnt_type == 1) {
        register uint32_t i;
        // delta_pic_order_always_zero_flag u(1)
        hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->delta_pic_order_always_zero_flag);
        // offset_for_non_ref_pic se(v)
        hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->offset_for_non_ref_pic);
        // offset_for_top_to_bottom_field se(v)
        hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->offset_for_top_to_bottom_field);
        // num_ref_frames_in_pic_order_cnt_cycle ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->num_ref_frames_in_pic_order_cnt_cycle);

        for(i = 0; i < p_sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
            // offset_for_ref_frame[ i ] se(v)
            hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->offset_for_ref_frame[i]);
        }
    }

    // max_num_ref_frames ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->max_num_ref_frames);
    // gaps_in_frame_num_value_allowed_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->gaps_in_frame_num_value_allowed_flag);
    // pic_width_in_mbs_minus1 ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->pic_width_in_mbs_minus1);
    // pic_height_in_map_units_minus1 ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->pic_height_in_map_units_minus1);
    // frame_mbs_only_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->frame_mbs_only_flag);

    if(!p_sps->frame_mbs_only_flag) {
        // mb_adaptive_frame_field_flag u(1)
        hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->mb_adaptive_frame_field_flag);
    }

    // direct_8x8_inference_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->direct_8x8_inference_flag);
    // frame_cropping_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->frame_cropping_flag);

    if (p_sps->frame_cropping_flag) {
        // frame_crop_left_offset ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->frame_crop_left_offset);
        // frame_crop_right_offset ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->frame_crop_right_offset);
        // frame_crop_top_offset ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->frame_crop_top_offset);
        // frame_crop_bottom_offset ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_sps->frame_crop_bottom_offset);
    }

    // vui_parameters_present_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->vui_parameters_present_flag);
    if (p_sps->vui_parameters_present_flag) {
        // vui_parameters( )
        // hlRbsp_WriteVuiParameters(SPS, p_codec->pobj_bits);
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    /*=== Scalable Video Coding (SVC) ===*/

    if (HL_CODEC_264_NAL(p_sps)->e_type == HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET) {
        // 7.3.2.1.3 Subset sequence parameter set RBSP syntax
        if (p_sps->profile_idc == HL_CODEC_264_PROFILE_BASELINE_SVC || p_sps->profile_idc == HL_CODEC_264_PROFILE_HIGH_SVC) {
            if (!p_sps->p_svc) {
                err = _hl_codec_264_nal_sps_svc_create(&p_sps->p_svc);
                if (err) {
                    return err;
                }
            }
            p_sps->b_svc = HL_TRUE;
            p_sps->p_svc->inter_layer_deblocking_filter_control_present_flag = 1;
            p_sps->p_svc->chroma_phase_x_plus1_flag = 1;
            p_sps->p_svc->chroma_phase_y_plus1 = 1;
            p_sps->p_svc->seq_ref_layer_chroma_phase_x_plus1_flag = p_sps->p_svc->chroma_phase_x_plus1_flag;
            p_sps->p_svc->seq_ref_layer_chroma_phase_y_plus1 = p_sps->p_svc->chroma_phase_y_plus1;

            // seq_parameter_set_svc_extension( ) /* G.7.3.2.1.4 Sequence parameter set SVC extension syntax */
            // inter_layer_deblocking_filter_control_present_flag u(1)
            hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->inter_layer_deblocking_filter_control_present_flag);
            // extended_spatial_scalability_idc u(2)
            hl_codec_264_bits_write_u(p_codec->pobj_bits, p_sps->p_svc->extended_spatial_scalability_idc, 2);
            if (p_sps->ChromaArrayType == 1 || p_sps->ChromaArrayType == 2) {
                // chroma_phase_x_plus1_flag u(1)
                hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->chroma_phase_x_plus1_flag);
                if (p_sps->ChromaArrayType == 1) {
                    // chroma_phase_y_plus1 u(2)
                    hl_codec_264_bits_write_u(p_codec->pobj_bits, p_sps->p_svc->chroma_phase_y_plus1, 2);
                }
            }
            if (p_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_SEQ) {
                if (p_sps->ChromaArrayType > 0) {
                    // seq_ref_layer_chroma_phase_x_plus1_flag u(1)
                    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->seq_ref_layer_chroma_phase_x_plus1_flag);
                    // seq_ref_layer_chroma_phase_y_plus1 u(2)
                    hl_codec_264_bits_write_u(p_codec->pobj_bits, p_sps->p_svc->seq_ref_layer_chroma_phase_y_plus1, 2);
                }
                //seq_scaled_ref_layer_left_offset se(v)
                hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->p_svc->seq_scaled_ref_layer_left_offset);
                // seq_scaled_ref_layer_top_offset se(v)
                hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->p_svc->seq_scaled_ref_layer_top_offset);
                // seq_scaled_ref_layer_right_offset se(v)
                hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->p_svc->seq_scaled_ref_layer_right_offset);
                // seq_scaled_ref_layer_bottom_offset se(v)
                hl_codec_264_bits_write_se(p_codec->pobj_bits, p_sps->p_svc->seq_scaled_ref_layer_bottom_offset);
            }
            // seq_tcoeff_level_prediction_flag u(1)
            hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->seq_tcoeff_level_prediction_flag);
            if (p_sps->p_svc->seq_tcoeff_level_prediction_flag) {
                // adaptive_tcoeff_level_prediction_flag u(1)
                hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->adaptive_tcoeff_level_prediction_flag);
            }
            hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->slice_header_restriction_flag);
            // end-of seq_parameter_set_svc_extension( )

            // svc_vui_parameters_present_flag u(1)
            hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_sps->p_svc->svc_vui_parameters_present_flag);
            if (p_sps->p_svc->svc_vui_parameters_present_flag) {
#if 1
                HL_DEBUG_ERROR("Not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
#else
                // svc_vui_parameters_extension( ) /* G.14.1 SVC VUI parameters extension syntax */
                // vui_ext_num_entries_minus1 ue(v)
                uint32_t vui_ext_num_entries_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                if (vui_ext_num_entries_minus1 >= HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT) {
                    HL_DEBUG_ERROR("Invalid 'vui_ext_num_entries_minus1'");
                    err = HL_ERROR_INVALID_BITSTREAM;
                    goto bail;
                }
                err = hl_codec_264_vui_svc_create(&p_sps->p_svc->p_vui);
                if (err) {
                    goto bail;
                }
                p_sps->p_svc->p_vui->vui_ext_num_entries_minus1 = vui_ext_num_entries_minus1;
                for (i = 0; i <= vui_ext_num_entries_minus1; i++) {
                    // vui_ext_dependency_id [ i ] u(3)
                    p_sps->p_svc->p_vui->vui_ext_dependency_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                    // vui_ext_quality_id[ i ]  u(4)
                    p_sps->p_svc->p_vui->vui_ext_quality_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 4);
                    // vui_ext_temporal_id[ i ] u(3)
                    p_sps->p_svc->p_vui->vui_ext_temporal_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                    // vui_ext_timing_info_present_flag[ i ] u(1)
                    p_sps->p_svc->p_vui->vui_ext_timing_info_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    if (p_sps->p_svc->p_vui->vui_ext_timing_info_present_flag[i]) {
                        // vui_ext_num_units_in_tick[ i ] u(32)
                        p_sps->p_svc->p_vui->vui_ext_num_units_in_tick[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 32);
                        // vui_ext_time_scale[ i ] u(32)
                        p_sps->p_svc->p_vui->vui_ext_time_scale[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, 32);
                        // vui_ext_fixed_frame_rate_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_fixed_frame_rate_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        // vui_ext_nal_hrd_parameters_present_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_nal_hrd_parameters_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        if (p_sps->p_svc->p_vui->vui_ext_nal_hrd_parameters_present_flag[i]) {
                            err = hl_codec_264_hrd_create(&p_sps->p_svc->p_vui->hrds_nal[i]);
                            if (err) {
                                goto bail;
                            }
                            err = hl_codec_264_rbsp_avc_hrd_parameters_read(p_sps->p_svc->p_vui->hrds_nal[i], p_codec->pobj_bits);
                            if (err) {
                                goto bail;
                            }
                        }
                        // vui_ext_vcl_hrd_parameters_present_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_vcl_hrd_parameters_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        if (p_sps->p_svc->p_vui->vui_ext_vcl_hrd_parameters_present_flag[i]) {
                            err = hl_codec_264_hrd_create(&p_sps->p_svc->p_vui->hrds_vcl[i]);
                            if (err) {
                                goto bail;
                            }
                            err = hl_codec_264_rbsp_avc_hrd_parameters_read(p_sps->p_svc->p_vui->hrds_vcl[i], p_codec->pobj_bits);
                            if (err) {
                                goto bail;
                            }
                        }
                        if (p_sps->p_svc->p_vui->vui_ext_nal_hrd_parameters_present_flag[i] || p_sps->p_svc->p_vui->vui_ext_vcl_hrd_parameters_present_flag[i]) {
                            // vui_ext_low_delay_hrd_flag[ i ] u(1)
                            p_sps->p_svc->p_vui->vui_ext_low_delay_hrd_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        }
                        // vui_ext_pic_struct_present_flag[ i ] u(1)
                        p_sps->p_svc->p_vui->vui_ext_pic_struct_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    }
                }
#endif
            }
        }
        else if (p_sps->profile_idc == 118 || p_sps->profile_idc == 128) {
            HL_DEBUG_ERROR("H.264 MVC not supported yet");
            return HL_ERROR_INVALID_BITSTREAM;
        }

        // additional_extension2_flag u(1)
        hl_codec_264_bits_write_u1(p_codec->pobj_bits, 0);
    } // end-of subsequent SPS parsing

    // rbsp_trailing_bits( )
    return hl_codec_264_rbsp_avc_trailing_bits_write(p_codec->pobj_bits);
}

/*** OBJECT DEFINITION FOR "hl_codec_264_nal_sps_t" ***/
static hl_object_t* hl_codec_264_nal_sps_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_nal_sps_t *p_sps = (hl_codec_264_nal_sps_t*)self;
    if (p_sps) {
    }
    return self;
}
static hl_object_t* hl_codec_264_nal_sps_dtor(hl_object_t * self)
{
    hl_codec_264_nal_sps_t *p_sps = (hl_codec_264_nal_sps_t*)self;
    if (p_sps) {
        HL_OBJECT_SAFE_FREE(p_sps->p_vui);
        HL_OBJECT_SAFE_FREE(p_sps->p_svc);
    }
    return self;
}
static int hl_codec_264_nal_sps_cmp(const hl_object_t *_sps1, const hl_object_t *_sps2)
{
    return (int)((int*)_sps1 - (int*)_sps2);
}
static const hl_object_def_t hl_codec_264_nal_sps_def_s = {
    sizeof(hl_codec_264_nal_sps_t),
    hl_codec_264_nal_sps_ctor,
    hl_codec_264_nal_sps_dtor,
    hl_codec_264_nal_sps_cmp,
};
const hl_object_def_t *hl_codec_264_nal_sps_def_t = &hl_codec_264_nal_sps_def_s;



/*** OBJECT DEFINITION FOR "hl_codec_264_nal_sps_svc_t" ***/
static hl_object_t* hl_codec_264_nal_sps_svc_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_nal_sps_svc_t *p_svc = (hl_codec_264_nal_sps_svc_t*)self;
    if (p_svc) {
        HL_OBJECT_SAFE_FREE(p_svc->p_vui);
    }
    return self;
}
static hl_object_t* hl_codec_264_nal_sps_svc_dtor(hl_object_t * self)
{
    hl_codec_264_nal_sps_svc_t *p_svc = (hl_codec_264_nal_sps_svc_t*)self;
    if (p_svc) {
        HL_OBJECT_SAFE_FREE(p_svc->p_vui);
    }
    return self;
}
static int hl_codec_264_nal_sps_svc_cmp(const hl_object_t *_v1, const hl_object_t *_v2)
{
    return (int)((int*)_v1 - (int*)_v2);
}
static const hl_object_def_t hl_codec_264_nal_sps_svc_def_s = {
    sizeof(hl_codec_264_nal_sps_svc_t),
    hl_codec_264_nal_sps_svc_ctor,
    hl_codec_264_nal_sps_svc_dtor,
    hl_codec_264_nal_sps_svc_cmp,
};
const hl_object_def_t *hl_codec_264_nal_sps_svc_def_t = &hl_codec_264_nal_sps_svc_def_s;