#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_rbsp.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

HL_ERROR_T hl_codec_264_nal_pps_create(unsigned u_ref_idc, HL_CODEC_264_NAL_TYPE_T e_type, hl_codec_264_nal_pps_t** pp_pps)
{
    extern const hl_object_def_t *hl_codec_264_nal_pps_def_t;
    if (!pp_pps) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_pps = hl_object_create(hl_codec_264_nal_pps_def_t);
    if (!*pp_pps) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    hl_codec_264_nal_init((hl_codec_264_nal_t*)*pp_pps, e_type, u_ref_idc);
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _hl_codec_264_nal_pps_derive(hl_codec_264_nal_pps_t* p_pps, const hl_codec_264_nal_sps_t* p_sps)
{
    int32_t i, mbIsInterFlag, iYCbCr, m, offset;

    // FIXME: SIMD
    if (!p_pps->pic_scaling_matrix_present_flag) {
        memcpy(p_pps->ScalingList4x4, p_sps->ScalingList4x4, sizeof(p_pps->ScalingList4x4));
        memcpy(p_pps->ScalingList8x8, p_sps->ScalingList8x8, sizeof(p_pps->ScalingList8x8));
    }

    // 8.5.9 Derivation process for scaling functions
    // weightScale4x4

    // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
    // FIXME: SIMD
    for (mbIsInterFlag=0; mbIsInterFlag<2; ++mbIsInterFlag) {
        offset = (mbIsInterFlag == 1) ? 3 : 0;
        for(iYCbCr=0; iYCbCr<3; ++iYCbCr) {
            for(i=0; i<16; i+=4) {
                p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][ZigZag4x4BlockScanYX[i][0]][ZigZag4x4BlockScanYX[i][1]] = p_pps->ScalingList4x4[iYCbCr + offset][i];
                p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][ZigZag4x4BlockScanYX[i + 1][0]][ZigZag4x4BlockScanYX[i + 1][1]] = p_pps->ScalingList4x4[iYCbCr + offset][i + 1];
                p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][ZigZag4x4BlockScanYX[i + 2][0]][ZigZag4x4BlockScanYX[i + 2][1]] = p_pps->ScalingList4x4[iYCbCr + offset][i + 2];
                p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][ZigZag4x4BlockScanYX[i + 3][0]][ZigZag4x4BlockScanYX[i + 3][1]] = p_pps->ScalingList4x4[iYCbCr + offset][i + 3];
            }
        }
    }
    // FIXME: SIMD
    for (mbIsInterFlag=0; mbIsInterFlag<2; ++mbIsInterFlag) {
        for (iYCbCr=0; iYCbCr<3; ++iYCbCr) {
            for (m = 0; m<6; ++m) {
                // (8-317)
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][0][0] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][0][0] * normAdjust4x4(m, 0, 0);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][0][1] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][0][1] * normAdjust4x4(m, 0, 1);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][0][2] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][0][2] * normAdjust4x4(m, 0, 2);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][0][3] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][0][3] * normAdjust4x4(m, 0, 3);

                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][1][0] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][1][0] * normAdjust4x4(m, 1, 0);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][1][1] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][1][1] * normAdjust4x4(m, 1, 1);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][1][2] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][1][2] * normAdjust4x4(m, 1, 2);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][1][3] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][1][3] * normAdjust4x4(m, 1, 3);

                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][2][0] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][2][0] * normAdjust4x4(m, 2, 0);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][2][1] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][2][1] * normAdjust4x4(m, 2, 1);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][2][2] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][2][2] * normAdjust4x4(m, 2, 2);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][2][3] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][2][3] * normAdjust4x4(m, 2, 3);

                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][3][0] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][3][0] * normAdjust4x4(m, 3, 0);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][3][1] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][3][1] * normAdjust4x4(m, 3, 1);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][3][2] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][3][2] * normAdjust4x4(m, 3, 2);
                p_pps->LevelScale4x4[mbIsInterFlag][iYCbCr][m][3][3] = p_pps->weightScale4x4[mbIsInterFlag][iYCbCr][3][3] * normAdjust4x4(m, 3, 3);
            }
        }
    }

    p_pps->SliceGroupChangeRate = p_pps->slice_group_change_rate_minus1 + 1;
    p_pps->uaNumRefIdxActive[HL_CODEC_264_LIST_IDX_0] = p_pps->num_ref_idx_l0_default_active_minus1 + 1;
    p_pps->uaNumRefIdxActive[HL_CODEC_264_LIST_IDX_1] = p_pps->num_ref_idx_l1_default_active_minus1 + 1;
    p_pps->uPicInitQp = p_pps->pic_init_qp_minus26 + 26;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_nal_pps_decode(HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc, struct hl_codec_264_s* p_codec, hl_codec_264_nal_pps_t** pp_pps)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_nal_pps_t* p_pps;
    hl_codec_264_nal_sps_t* p_sps;
    register hl_size_t i;
    uint32_t pic_parameter_set_id, seq_parameter_set_id;

    if (!pp_pps || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // pic_parameter_set_id ue()
    pic_parameter_set_id = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    if (pic_parameter_set_id >= HL_CODEC_264_PPS_MAX_COUNT) {
        HL_DEBUG_ERROR("Invalid 'pic_parameter_set_id'. (%d) >= %d", pic_parameter_set_id, HL_CODEC_264_PPS_MAX_COUNT);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    // seq_parameter_set_id ue(v)
    seq_parameter_set_id = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    if (seq_parameter_set_id >= HL_CODEC_264_SPS_MAX_COUNT) {
        HL_DEBUG_ERROR("Invalid 'seq_parameter_set_id'. (%d) >= %d ", seq_parameter_set_id, HL_CODEC_264_SPS_MAX_COUNT);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    if (!(p_sps = p_codec->sps.p_list[seq_parameter_set_id])) {
        HL_DEBUG_ERROR("No 'seq_parameter_set_id' with value =%d could be found", seq_parameter_set_id);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }

    // Create pps object
    err = hl_codec_264_nal_pps_create(u_ref_idc, e_type, pp_pps);
    if (err) {
        goto bail;
    }
    p_pps = *pp_pps;
    p_pps->pic_parameter_set_id = pic_parameter_set_id;
    p_pps->seq_parameter_set_id = seq_parameter_set_id;
    p_pps->pc_sps = p_sps;

    // entropy_coding_mode_flag u(1)
    p_pps->entropy_coding_mode_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // bottom_field_pic_order_in_frame_present_flag u(1)
    p_pps->bottom_field_pic_order_in_frame_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // num_slice_groups_minus1 ue(v)
    p_pps->num_slice_groups_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    if (p_pps->num_slice_groups_minus1 > HL_CODEC_264_NUM_SLICE_GROUPS_MINUS1_MAX_VAL) {
        HL_DEBUG_ERROR("num_slice_groups_minus1 > %d", HL_CODEC_264_NUM_SLICE_GROUPS_MINUS1_MAX_VAL);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }

    if (p_pps->num_slice_groups_minus1 > 0) {
        // slice_group_map_type ue(v)
        p_pps->slice_group_map_type = hl_codec_264_bits_read_ue(p_codec->pobj_bits);

        if (p_pps->slice_group_map_type == 0) {
            for (i = 0; i <= p_pps->num_slice_groups_minus1; i++) {
                // run_length_minus1 ue(v)
                p_pps->run_length_minus1[i] = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
            }
        }
        else if (p_pps->slice_group_map_type == 2) {
            for (i = 0; i < p_pps->num_slice_groups_minus1; i++) {
                // top_left ue(v)
                p_pps->top_left[i] = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                // bottom_right ue(v)
                p_pps->bottom_right[i] = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
            }
        }
        else if (p_pps->slice_group_map_type == 3 || p_pps->slice_group_map_type == 4 || p_pps->slice_group_map_type == 5) {
            // slice_group_change_direction_flag u(1)
            p_pps->slice_group_change_direction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            // slice_group_change_rate_minus1 ue(v)
            p_pps->slice_group_change_rate_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        }
        else if (p_pps->slice_group_map_type == 6) {
            // pic_size_in_map_units_minus1 ue(v)
            p_pps->pic_size_in_map_units_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
            if((p_pps->slice_group_id = hl_memory_calloc(p_pps->pic_size_in_map_units_minus1+1, sizeof(uint32_t)))) {
                uint32_t u_num_bits = (uint32_t)HL_MATH_CEIL(HL_MATH_LOG2(p_pps->num_slice_groups_minus1 + 1));
                for (i = 0; i <= p_pps->pic_size_in_map_units_minus1; i++) {
                    // slice_group_id u(v)
                    // length = Ceil( Log2( num_slice_groups_minus1 + 1 ) )
                    p_pps->slice_group_id[i] = hl_codec_264_bits_read_u(p_codec->pobj_bits, u_num_bits);
                }
            }
        }
    }

    // num_ref_idx_l0_default_active_minus1 ue(v)
    p_pps->num_ref_idx_l0_default_active_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // num_ref_idx_l1_default_active_minus1 ue(v)
    p_pps->num_ref_idx_l1_default_active_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // weighted_pred_flag u(1)
    p_pps->weighted_pred_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // weighted_bipred_idc u(2)
    p_pps->weighted_bipred_idc = hl_codec_264_bits_read_u(p_codec->pobj_bits, 2);
    // pic_init_qp_minus26 /* relative to 26 */ se(v)
    p_pps->pic_init_qp_minus26 = hl_codec_264_bits_read_se(p_codec->pobj_bits);
    if (p_pps->pic_init_qp_minus26 < -26 || p_pps->pic_init_qp_minus26 > 25) {
        HL_DEBUG_ERROR("pic_init_qp_minus26(%u) < -26 || pic_init_qp_minus26(%u) > 25", p_pps->pic_init_qp_minus26, p_pps->pic_init_qp_minus26);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    // pic_init_qs_minus26 /* relative to 26 */ se(v)
    p_pps->pic_init_qs_minus26 = hl_codec_264_bits_read_se(p_codec->pobj_bits);
    // chroma_qp_index_offset se(v)
    p_pps->chroma_qp_index_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
    p_pps->second_chroma_qp_index_offset = p_pps->chroma_qp_index_offset; // default
    if (p_pps->chroma_qp_index_offset < -12 || p_pps->chroma_qp_index_offset > 12) {
        HL_DEBUG_ERROR("chroma_qp_index_offset(%u) < -12 || pic_init_qp_minus26(%u) > 12", p_pps->chroma_qp_index_offset, p_pps->chroma_qp_index_offset);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    // deblocking_filter_control_present_flag u(1)
    p_pps->deblocking_filter_control_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // constrained_intra_pred_flag u(1)
    p_pps->constrained_intra_pred_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
    // redundant_pic_cnt_present_flag u(1)
    p_pps->redundant_pic_cnt_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);

    if (hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits)) {
        // transform_8x8_mode_flag u1
        p_pps->transform_8x8_mode_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        // pic_scaling_matrix_present_flag u1
        p_pps->pic_scaling_matrix_present_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);

        if (p_pps->pic_scaling_matrix_present_flag) {
            register hl_size_t u_seq_scaling_list_present_flag_count = 6 + ((p_sps->chroma_format_idc != 3) ? 2 : 6) * p_pps->transform_8x8_mode_flag;
            for (i = 0; i< u_seq_scaling_list_present_flag_count; ++i) {
                static const uint32_t u_size_of_int32x16 = sizeof(int32_t) << 4;
                static const uint32_t u_size_of_int32x64 = sizeof(int32_t) << 6;
                // pic_scaling_list_present_flag[i] u(1)
                p_pps->pic_scaling_list_present_flag[i] = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                if (p_pps->pic_scaling_list_present_flag[i]) {
                    if (i < 6) {
                        /*err = */ hl_codec_264_rbsp_avc_scaling_list_read(p_codec->pobj_bits, p_pps->ScalingList4x4[i], 16, p_pps->UseDefaultScalingMatrix4x4Flag[i]);
                        if (p_pps->UseDefaultScalingMatrix4x4Flag[i]) {
                            memcpy(p_pps->ScalingList4x4[i], i<=2 ? Scaling_Default_4x4_Intra : Scaling_Default_4x4_Inter, u_size_of_int32x16);
                        }
                    }
                    else {
                        /*err = */ hl_codec_264_rbsp_avc_scaling_list_read(p_codec->pobj_bits, p_pps->ScalingList8x8[i-6], 64, p_pps->UseDefaultScalingMatrix8x8Flag[i-6]);
                        if (p_pps->UseDefaultScalingMatrix8x8Flag[i-6]) {
                            memcpy(p_pps->ScalingList4x4[i], (i-6)<=2 ? Scaling_Default_8x8_Intra : Scaling_Default_8x8_Inter, u_size_of_int32x64);
                        }
                    }
                }
                else { // Table 7-2 – Assignment of mnemonic names to scaling list indices and specification of fall-back rule
                    HL_DEBUG_ERROR("Not checked code");
                    err = HL_ERROR_NOT_IMPLEMENTED;
                    goto bail;
                }
            }
        }
        // second_chroma_qp_index_offset se(v)
        p_pps->second_chroma_qp_index_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
    }

    // rbsp_trailing_bits( )
    hl_codec_264_rbsp_avc_trailing_bits_read(p_codec->pobj_bits);

    err = _hl_codec_264_nal_pps_derive(p_pps, p_sps);

bail:
    if (err && pp_pps) {
        HL_OBJECT_SAFE_FREE(*pp_pps);
    }
    return err;
}

HL_ERROR_T hl_codec_264_nal_pps_encode(hl_codec_264_nal_pps_t* p_pps, uint32_t sps_id, hl_codec_264_t* p_codec)
{
    const hl_codec_264_layer_t* pc_layer;
    HL_ERROR_T err;

    if (!p_pps || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // Set default active layer
    pc_layer = p_codec->layers.pc_active;

    // Set SPS
    if (!(p_pps->pc_sps = p_codec->sps.p_list[sps_id])) {
        HL_DEBUG_ERROR("Failed to find SPS with id = %u", sps_id);
        return HL_ERROR_INVALID_BITSTREAM;
    }

    /* Set PPS values */

    p_pps->seq_parameter_set_id = sps_id;
    p_pps->entropy_coding_mode_flag = 0;
    p_pps->bottom_field_pic_order_in_frame_present_flag = 0;
    p_pps->slice_group_map_type = 0;
    p_pps->num_slice_groups_minus1 = (pc_layer->encoder.i_slice_count - 1);
    p_pps->num_ref_idx_l0_default_active_minus1 = (p_pps->pc_sps->max_num_ref_frames > 0 ? (p_pps->pc_sps->max_num_ref_frames - 1) : 0);
    p_pps->num_ref_idx_l1_default_active_minus1 = 0;
    p_pps->weighted_pred_flag = 0;
    p_pps->weighted_bipred_idc = 0;
    p_pps->pic_init_qp_minus26 = (p_codec->pc_base->qp - 26);
    p_pps->pic_init_qs_minus26 = 0;
    p_pps->chroma_qp_index_offset = 0;
    p_pps->deblocking_filter_control_present_flag = 1;
    p_pps->constrained_intra_pred_flag = 0;
    p_pps->redundant_pic_cnt_present_flag = 0;

    // Derive() default values
    err = _hl_codec_264_nal_pps_derive(p_pps, p_pps->pc_sps);
    if (err) {
        return err;
    }

    /* Write PPS values */

    // forbidden_zero_bit f(1)
    hl_codec_264_bits_write_f1(p_codec->pobj_bits, HL_CODEC_264_NAL(p_pps)->u_forbidden_zero_bit);
    // ref_idc u(2)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, HL_CODEC_264_NAL(p_pps)->u_ref_idc, 2);
    // nal_unit_tye u(5)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, HL_CODEC_264_NAL(p_pps)->e_type, 5);

    // pic_parameter_set_id ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->pic_parameter_set_id);
    // seq_parameter_set_id ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->seq_parameter_set_id);
    // entropy_coding_mode_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_pps->entropy_coding_mode_flag);
    // bottom_field_pic_order_in_frame_present_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_pps->bottom_field_pic_order_in_frame_present_flag);
    // num_slice_groups_minus1 ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->num_slice_groups_minus1);

    if (p_pps->num_slice_groups_minus1 > 0) {
        // slice_group_map_type ue(v)
        hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->slice_group_map_type);

        if(p_pps->slice_group_map_type == 0) {
            register uint32_t iGroup;
            int32_t i_min_mbs_per_slice = ((int32_t)p_pps->pc_sps->uPicSizeInMapUnits / (p_pps->num_slice_groups_minus1 + 1));
            int32_t i_mbs_in_last_slice = i_min_mbs_per_slice + ((int32_t)p_pps->pc_sps->uPicSizeInMapUnits % (i_min_mbs_per_slice * (p_pps->num_slice_groups_minus1 + 1)));
            for (iGroup = 0; iGroup <= p_pps->num_slice_groups_minus1; iGroup++) {
                p_pps->run_length_minus1[iGroup] = ((iGroup == p_pps->num_slice_groups_minus1) ? i_mbs_in_last_slice : i_min_mbs_per_slice) - 1;
                // run_length_minus1 ue(v)
                hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->run_length_minus1[iGroup]);
            }
        }
#if 1
        else {
            HL_DEBUG_ERROR("Not implemented yet");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
#else
        else if(p_pps->slice_group_map_type == 2) {
            register uint32_t iGroup;
            for(iGroup = 0; iGroup < p_pps->num_slice_groups_minus1; iGroup++) {
                // top_left ue(v)
                p_pps->top_left[iGroup] = hl264Bits_ReadUE(p_codec->pobj_bits);
                // bottom_right ue(v)
                p_pps->bottom_right[iGroup] = hl264Bits_ReadUE(p_codec->pobj_bits);
            }
        }
        else if(p_pps->slice_group_map_type == 3 || p_pps->slice_group_map_type == 4 || p_pps->slice_group_map_type == 5) {
            // slice_group_change_direction_flag u(1)
            p_pps->slice_group_change_direction_flag = hl264Bits_ReadU1(p_codec->pobj_bits);
            // slice_group_change_rate_minus1 ue(v)
            p_pps->slice_group_change_rate_minus1 = hl264Bits_ReadUE(p_codec->pobj_bits);
        }
        else if(p_pps->slice_group_map_type == 6) {
            register uint32_t i;
            // pic_size_in_map_units_minus1 ue(v)
            p_pps->pic_size_in_map_units_minus1 = hl264Bits_ReadUE(p_codec->pobj_bits);
            if((p_pps->slice_group_id = hlMemory_Calloc(p_pps->pic_size_in_map_units_minus1+1, sizeof(uint32_t)))) {
                for(i = 0; i <= p_pps->pic_size_in_map_units_minus1; i++) {
                    // slice_group_id u(v)
                    // length = Ceil( Log2( num_slice_groups_minus1 + 1 ) )
                    p_pps->slice_group_id[i] = hl264Bits_ReadU(p_codec->pobj_bits, (uint32_t)HL_MATH_CEIL(HL_MATH_LOG2(p_pps->num_slice_groups_minus1 + 1)));
                }
            }
        }
#endif
    }

    // num_ref_idx_l0_default_active_minus1 ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->num_ref_idx_l0_default_active_minus1);
    // num_ref_idx_l1_default_active_minus1 ue(v)
    hl_codec_264_bits_write_ue(p_codec->pobj_bits, p_pps->num_ref_idx_l1_default_active_minus1);
    // weighted_pred_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_pps->weighted_pred_flag);
    // weighted_bipred_idc u(2)
    hl_codec_264_bits_write_u(p_codec->pobj_bits, p_pps->weighted_bipred_idc, 2);
    // pic_init_qp_minus26 /* relative to 26 */ se(v)
    hl_codec_264_bits_write_se(p_codec->pobj_bits, p_pps->pic_init_qp_minus26);
    // pic_init_qs_minus26 /* relative to 26 */ se(v)
    hl_codec_264_bits_write_se(p_codec->pobj_bits, p_pps->pic_init_qs_minus26);
    // chroma_qp_index_offset se(v)
    hl_codec_264_bits_write_se(p_codec->pobj_bits, p_pps->chroma_qp_index_offset);
    // deblocking_filter_control_present_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_pps->deblocking_filter_control_present_flag);
    // constrained_intra_pred_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_pps->constrained_intra_pred_flag);
    // redundant_pic_cnt_present_flag u(1)
    hl_codec_264_bits_write_u1(p_codec->pobj_bits, p_pps->redundant_pic_cnt_present_flag);

#if 0
    SPS = ctx->enc.encObj->listSPS[p_pps->seq_parameter_set_id];
    if (!SPS) {
        HL_DEBUG_ERROR("Failed to find SPS with id = %d", p_pps->seq_parameter_set_id);
    }

    if (hlRbsp_ReadMoreData(p_codec->pobj_bits, HL_FALSE)) {
        // transform_8x8_mode_flag u1
        p_pps->transform_8x8_mode_flag = hl264Bits_ReadU1(p_codec->pobj_bits);
        // pic_scaling_matrix_present_flag u1
        p_pps->pic_scaling_matrix_present_flag = hl264Bits_ReadU1(p_codec->pobj_bits);

        if(p_pps->pic_scaling_matrix_present_flag) {
            HL_DEBUG_ERROR("Not checked code");
            if(SPS) {
                register uint32_t i;
                for(i = 0; i < 6 + ((SPS->chroma_format_idc != 3) ? 2 : 6) * p_pps->transform_8x8_mode_flag; i++) {
                    // pic_scaling_list_present_flag[i] u(1)
                    p_pps->pic_scaling_list_present_flag[i] = hl264Bits_ReadU1(p_codec->pobj_bits);
                    if(p_pps->pic_scaling_list_present_flag[i]) {
                        if(i < 6) {
                            hlRbsp_ReadScalingList(p_codec->pobj_bits, p_pps->ScalingList4x4[i], 16, p_pps->UseDefaultScalingMatrix4x4Flag[i]);
                            if(p_pps->UseDefaultScalingMatrix4x4Flag[i]) {
                                memcpy(p_pps->ScalingList4x4[i], i<=2 ? Default_4x4_Intra : Default_4x4_Inter, sizeof(int32_t) * 16);
                            }
                        }
                        else {
                            hlRbsp_ReadScalingList(p_codec->pobj_bits, p_pps->ScalingList8x8[i-6], 64, p_pps->UseDefaultScalingMatrix8x8Flag[i-6]);
                            if(p_pps->UseDefaultScalingMatrix8x8Flag[i-6]) {
                                memcpy(p_pps->ScalingList4x4[i], (i-6)<=2 ? Default_8x8_Intra : Default_8x8_Inter, sizeof(int32_t) * 64);
                            }
                        }
                    }
                    else {
                        HL_DEBUG_ERROR("TODO: Table 7-2. See SPS");
                    }
                }
            }
            else {
                HL_DEBUG_ERROR("chroma_format_idc needed but SPS is null");
            }
        }
        // second_chroma_qp_index_offset se(v)
        p_pps->second_chroma_qp_index_offset = hl264Bits_ReadSE(p_codec->pobj_bits);
    }

    if(!p_pps->pic_scaling_matrix_present_flag && SPS) {
        memcpy(p_pps->ScalingList4x4, SPS->ScalingList4x4, sizeof(p_pps->ScalingList4x4));
        memcpy(p_pps->ScalingList8x8, SPS->ScalingList8x8, sizeof(p_pps->ScalingList8x8));
    }
#endif

    // rbsp_trailing_bits( )
    return hl_codec_264_rbsp_avc_trailing_bits_write(p_codec->pobj_bits);
}


/*** OBJECT DEFINITION FOR "hl_codec_264_nal_pps_t" ***/
static hl_object_t* hl_codec_264_nal_pps_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_nal_pps_t *p_pps = (hl_codec_264_nal_pps_t*)self;
    if (p_pps) {

    }
    return self;
}
static hl_object_t* hl_codec_264_nal_pps_dtor(hl_object_t * self)
{
    hl_codec_264_nal_pps_t *p_pps = (hl_codec_264_nal_pps_t*)self;
    if (p_pps) {
        HL_SAFE_FREE(p_pps->slice_group_id);
    }
    return self;
}
static int hl_codec_264_nal_pps_cmp(const hl_object_t *_v1, const hl_object_t *_v2)
{
    return (int)((int*)_v1 - (int*)_v2);
}
static const hl_object_def_t hl_codec_264_nal_pps_def_s = {
    sizeof(hl_codec_264_nal_pps_t),
    hl_codec_264_nal_pps_ctor,
    hl_codec_264_nal_pps_dtor,
    hl_codec_264_nal_pps_cmp,
    HL_TRUE, // aligned
};
const hl_object_def_t *hl_codec_264_nal_pps_def_t = &hl_codec_264_nal_pps_def_s;
