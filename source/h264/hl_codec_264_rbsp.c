#include "hartallo/h264/hl_codec_264_rbsp.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_vui.h"
#include "hartallo/h264/hl_codec_264_hrd.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

// 7.2 Specification of syntax functions, categories, and descriptors
hl_bool_t hl_codec_264_rbsp_avc_more_data_read(hl_codec_264_bits_t* p_bits)
{
    /* more_rbsp_data( ) is specified as follows.
    – If there is no more data in the RBSP, the return value of more_rbsp_data( ) is equal to FALSE.
    – Otherwise, the RBSP data is searched for the last (least significant, right-most) bit equal to 1 that is present in
    the RBSP. Given the position of this bit, which is the first bit (rbsp_stop_one_bit) of the rbsp_trailing_bits( )
    syntax structure, the following applies.
    – If there is more data in an RBSP before the rbsp_trailing_bits( ) syntax structure, the return value of
    more_rbsp_data( ) is equal to TRUE.
    – Otherwise, the return value of more_rbsp_data( ) is equal to FALSE.
    */
    const static uint8_t g_trailing_bits[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    int32_t i_left_bytes_count = (int32_t)(p_bits->pc_end - p_bits->pc_current);
    //int32_t i_left_bits_count = (i_left_bytes_count == 0) ? (8 - bits->bits_count - 1) :
    //	((i_left_bytes_count == 1) ? ((p_bits->bits_count == 7) ? 8 : (8 - bits->bits_count - 1)) : -1);
    int32_t i_left_bits_count = (i_left_bytes_count == 1) ? (8 - (7 - p_bits->i_bits_count)) : -1;
    if (i_left_bits_count > 0) {
        return (hl_codec_264_bits_show_u(p_bits, i_left_bits_count) != g_trailing_bits[i_left_bits_count]);
    }
    return (i_left_bytes_count > 0);
}

// 7.3.2.1.1.1 Scaling list syntax
// scaling_list( scalingList, sizeOfScalingList, useDefaultScalingMatrixFlag )
HL_ERROR_T hl_codec_264_rbsp_avc_scaling_list_read(hl_codec_264_bits_t* p_bits, int32_t scalingList[], uint32_t sizeOfScalingList, uint8_t useDefaultScalingMatrixFlag[])
{
    int32_t lastScale = 8, nextScale = 8, delta_scale;
    uint32_t j;

    // FIXME: SIMD
    for (j = 0; j < sizeOfScalingList; j++) {
        if (nextScale != 0) {
            // delta_scale se(v)
            delta_scale =  hl_codec_264_bits_read_se(p_bits);
            nextScale = HL_MATH_MOD_POW2_INT32((lastScale + delta_scale + 256), 256);
            useDefaultScalingMatrixFlag[j] = (j == 0 && nextScale == 0);
        }
        scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
        lastScale = scalingList[j];
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rbsp_avc_scaling_list_write(struct hl_codec_264_bits_s* p_bits, int32_t scalingList[], uint32_t sizeOfScalingList, uint8_t useDefaultScalingMatrixFlag[])
{
    HL_DEBUG_ERROR("Not implemented");
    return HL_ERROR_NOT_IMPLEMENTED;
}

// 7.3.2.8 Slice layer without partitioning RBSP syntax or 7.3.2.13 Slice layer extension RBSP syntax
// slice_layer_without_partitioning_rbsp( ) or slice_layer_extension_rbsp( )
// Must be called for NAL types:
//	- (1)"Coded slice of a non-IDR picture", (5)"Coded slice of an IDR picture", (19)"Coded slice of an auxiliary coded picture without partitioning"
//	- Desision based on: "Table 7-1 – NAL unit type codes, syntax element categories, and NAL unit type classes"
//HL_ERROR_T hl_codec_264_rbsp_avc_slice_layer_read(hl_codec_264_t* p_codec)
//{
//    HL_ERROR_T err = HL_ERROR_SUCCESS;
//
//    /* decision based on Table 7-1 – NAL unit type codes, syntax element categories, and NAL unit type classes */
//	switch(p_codec->nal_current.e_nal_type)
//	{
//		case HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE:
//		case HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_A_NON_IDR_PICTURE:
//		case HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION:
//			{
//				/* 7.3.2.8 Slice layer without partitioning RBSP syntax */
//				// slice_layer_without_partitioning_rbsp( ) {
//				// slice_header( )
//				err = hl_codec_264_nal_slice_header_decode(p_codec, &p_codec->slice_current.p_header);
//				if (err) {
//					return err;
//				}
//				// }
//				break;
//			}
//		default:
//			{
//				HL_DEBUG_ERROR("%d not supported as valid NAL Unit type", (int32_t)p_codec->nal_current.e_nal_type);
//				err = HL_ERROR_INVALID_BITSTREAM;
//				break;
//			}
//	}
//
//    return err;
//}

// 7.3.2.12 Prefix NAL unit RBSP syntax
// prefix_nal_unit_rbsp( )
HL_ERROR_T hl_codec_264_rbsp_avc_prefix_nal_unit_read(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    uint32_t additional_prefix_nal_unit_extension_flag;

    if (!p_codec->nal_current.svc_extension_flag) {
        HL_DEBUG_ERROR("Calling 'prefix_nal_unit_rbsp( )' for non-SVC extension");
        return HL_ERROR_UNEXPECTED_CALL;
    }
    // G.7.3.2.12.1 Prefix NAL unit SVC syntax
    // prefix_nal_unit_svc( ) /* specified in Annex G */
    if (p_codec->nal_current.i_nal_ref_idc != 0) {
        // store_ref_base_pic_flag 2 u(1)
        p_codec->nal_current.prefix.store_ref_base_pic_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        if ((p_codec->nal_current.ext.svc.use_ref_base_pic_flag || p_codec->nal_current.prefix.store_ref_base_pic_flag) && !p_codec->nal_current.ext.svc.idr_flag ) {
            // dec_ref_base_pic_marking( )
            err = hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_read(p_codec->pobj_bits, &p_codec->nal_current.prefix.xs_dec_ref_base_pic_marking);
            if (err) {
                return err;
            }
        }
        // additional_prefix_nal_unit_extension_flag u(1)
        additional_prefix_nal_unit_extension_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        if (additional_prefix_nal_unit_extension_flag) {
            while (hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits)) {
                additional_prefix_nal_unit_extension_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            }
        }
        // rbsp_trailing_bits( )
        hl_codec_264_rbsp_avc_trailing_bits_read(p_codec->pobj_bits);
    }
    else if (hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits)) {
        while (hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits)) {
            additional_prefix_nal_unit_extension_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        }
        // rbsp_trailing_bits( )
        hl_codec_264_rbsp_avc_trailing_bits_read(p_codec->pobj_bits);
    }

    return err;
}

// 7.3.2.11 RBSP trailing bits syntax
// rbsp_trailing_bits( )
HL_ERROR_T hl_codec_264_rbsp_avc_trailing_bits_read(hl_codec_264_bits_t* p_bits)
{
#if 0
    if (!hl_codec_264_bits_is_aligned(p_bits)) {
        uint32_t rbsp_stop_one_bit = hl_codec_264_bits_read_f1(p_bits);
        while (!hl_codec_264_bits_is_aligned(p_bits)) {
            /* rbsp_alignment_zero_bit = */ hl_codec_264_bits_read_f1(p_bits);
        }
    }
#else
    hl_codec_264_bits_align(p_bits);
#endif
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rbsp_avc_trailing_bits_write(hl_codec_264_bits_t* p_bits)
{
    if (p_bits->i_bits_count != 7 || !(p_bits->pc_current[-1] & 0x01)) {
        const static uint8_t g_trailing_bits[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
        const int32_t i_left_bits_count = 8 - (7 - p_bits->i_bits_count);
        hl_codec_264_bits_write_u(p_bits, g_trailing_bits[i_left_bits_count], i_left_bits_count);
    }
    return HL_ERROR_SUCCESS;
}

// 7.3.3.1 Reference picture list modification syntax
// ref_pic_list_modification( )
HL_ERROR_T hl_codec_264_rbsp_avc_ref_pic_list_modification_read(hl_codec_264_nal_slice_header_t *p_slice_header, hl_codec_264_bits_t* p_bits)
{
    int32_t i;

    if (p_slice_header->SliceTypeModulo5 != HL_CODEC_264_SLICE_TYPE_I && p_slice_header->SliceTypeModulo5 != HL_CODEC_264_SLICE_TYPE_SI) {
        // ref_pic_list_modification_flag_l0 u(1)
        p_slice_header->ref_pic_list_modification_flag_l0 = hl_codec_264_bits_read_u1(p_bits);
        if (p_slice_header->ref_pic_list_modification_flag_l0) {
            i = 0;
            do {
                // modification_of_pic_nums_idc ue(v)
                p_slice_header->modification_of_pic_nums_idc_l0[i] = hl_codec_264_bits_read_ue(p_bits);
                if (p_slice_header->modification_of_pic_nums_idc_l0[i] == 0 || p_slice_header->modification_of_pic_nums_idc_l0[i] == 1) {
                    // abs_diff_pic_num_minus1 ue(v)
                    p_slice_header->abs_diff_pic_num_minus1_l0[i] = hl_codec_264_bits_read_ue(p_bits);
                }
                else if (p_slice_header->modification_of_pic_nums_idc_l0[i] == 2) {
                    // long_term_pic_num ue(v)
                    p_slice_header->long_term_pic_num_l0[i] = hl_codec_264_bits_read_ue(p_bits);
                }
            }
            while (p_slice_header->modification_of_pic_nums_idc_l0[i++] != 3 && i< HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
            if (i == HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
                HL_DEBUG_ERROR("modification_of_pic_nums_idc_l0 idx(%d) > %u", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
                return HL_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    if (p_slice_header->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B) {
        // ref_pic_list_modification_flag_l1
        p_slice_header->ref_pic_list_modification_flag_l1 = hl_codec_264_bits_read_u1(p_bits);
        if (p_slice_header->ref_pic_list_modification_flag_l1) {
            i = 0;
            do {
                // modification_of_pic_nums_idc ue(v)
                p_slice_header->modification_of_pic_nums_idc_l1[i] = hl_codec_264_bits_read_ue(p_bits);
                if (p_slice_header->modification_of_pic_nums_idc_l1[i] == 0 || p_slice_header->modification_of_pic_nums_idc_l1[i] == 1) {
                    // abs_diff_pic_num_minus1 ue(v)
                    p_slice_header->abs_diff_pic_num_minus1_l1[i] = hl_codec_264_bits_read_ue(p_bits);
                }
                else if (p_slice_header->modification_of_pic_nums_idc_l1[i] == 2) {
                    // long_term_pic_num ue(v)
                    p_slice_header->long_term_pic_num_l1[i] = hl_codec_264_bits_read_ue(p_bits);
                }
            }
            while (p_slice_header->modification_of_pic_nums_idc_l1[i++] != 3 && i< HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
            if (i == HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
                HL_DEBUG_ERROR("modification_of_pic_nums_idc_l1 idx(%d) > %u", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
                return HL_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rbsp_avc_ref_pic_list_modification_write(hl_codec_264_nal_slice_header_t *p_slice_header, hl_codec_264_bits_t* p_bits)
{
    int32_t i;

    if (p_slice_header->SliceTypeModulo5 != HL_CODEC_264_SLICE_TYPE_I && p_slice_header->SliceTypeModulo5 != HL_CODEC_264_SLICE_TYPE_SI) {
        // ref_pic_list_modification_flag_l0 u(1)
        hl_codec_264_bits_write_u1(p_bits,  p_slice_header->ref_pic_list_modification_flag_l0);
        if (p_slice_header->ref_pic_list_modification_flag_l0) {
            i = 0;
            do {
                // modification_of_pic_nums_idc ue(v)
                hl_codec_264_bits_write_ue(p_bits, p_slice_header->modification_of_pic_nums_idc_l0[i]);
                if (p_slice_header->modification_of_pic_nums_idc_l0[i] == 0 || p_slice_header->modification_of_pic_nums_idc_l0[i] == 1) {
                    // abs_diff_pic_num_minus1 ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->abs_diff_pic_num_minus1_l0[i]);
                }
                else if (p_slice_header->modification_of_pic_nums_idc_l0[i] == 2) {
                    // long_term_pic_num ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->long_term_pic_num_l0[i]);
                }
            }
            while (p_slice_header->modification_of_pic_nums_idc_l0[i++] != 3 && i< HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
            if (i == HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
                HL_DEBUG_ERROR("modification_of_pic_nums_idc_l0 idx(%d) > %u", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
                return HL_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    if (p_slice_header->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B) {
        // ref_pic_list_modification_flag_l1
        hl_codec_264_bits_write_u1(p_bits, p_slice_header->ref_pic_list_modification_flag_l1);
        if (p_slice_header->ref_pic_list_modification_flag_l1) {
            i = 0;
            do {
                // modification_of_pic_nums_idc ue(v)
                hl_codec_264_bits_write_ue(p_bits, p_slice_header->modification_of_pic_nums_idc_l1[i]);
                if (p_slice_header->modification_of_pic_nums_idc_l1[i] == 0 || p_slice_header->modification_of_pic_nums_idc_l1[i] == 1) {
                    // abs_diff_pic_num_minus1 ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->abs_diff_pic_num_minus1_l1[i]);
                }
                else if (p_slice_header->modification_of_pic_nums_idc_l1[i] == 2) {
                    // long_term_pic_num ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->long_term_pic_num_l1[i]);
                }
            }
            while (p_slice_header->modification_of_pic_nums_idc_l1[i++] != 3 && i< HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
            if (i == HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
                HL_DEBUG_ERROR("modification_of_pic_nums_idc_l1 idx(%d) > %u", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
                return HL_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// 7.3.3.2 Prediction weight table syntax
// pred_weight_table( )
HL_ERROR_T hl_codec_264_rbsp_avc_pred_weight_table_read(hl_codec_264_nal_slice_header_t *p_slice_header, hl_codec_264_bits_t* p_bits)
{
    register uint32_t i;

    // luma_log2_weight_denom ue()
    p_slice_header->xs_pred_weight_table.luma_log2_weight_denom = hl_codec_264_bits_read_ue(p_bits);
    if(p_slice_header->pc_pps->pc_sps->ChromaArrayType != 0) {
        // chroma_log2_weight_denom ue(v)
        p_slice_header->xs_pred_weight_table.chroma_log2_weight_denom = hl_codec_264_bits_read_ue(p_bits);
    }

    for (i = 0; i <= p_slice_header->num_ref_idx_l0_active_minus1; i++) {
        // luma_weight_l0_flag u(1)
        p_slice_header->xs_pred_weight_table.luma_weight_l0_flag = hl_codec_264_bits_read_u1(p_bits);
        if (p_slice_header->xs_pred_weight_table.luma_weight_l0_flag) {
            // luma_weight_l0[ i ] se(v)
            p_slice_header->xs_pred_weight_table.luma_weight_l0[i] = hl_codec_264_bits_read_se(p_bits);
            // luma_offset_l0[ i ] se(v)
            p_slice_header->xs_pred_weight_table.luma_offset_l0[i] = hl_codec_264_bits_read_se(p_bits);
        }
        if (p_slice_header->pc_pps->pc_sps->ChromaArrayType != 0) {
            // chroma_weight_l0_flag u(1)
            p_slice_header->xs_pred_weight_table.chroma_weight_l0_flag = hl_codec_264_bits_read_u1(p_bits);
            if (p_slice_header->xs_pred_weight_table.chroma_weight_l0_flag) {
                //for(j = 0; j < 2; j++){
                // chroma_weight_l0[i][j] se(v)
                p_slice_header->xs_pred_weight_table.chroma_weight_l0[i][0] = hl_codec_264_bits_read_se(p_bits);
                p_slice_header->xs_pred_weight_table.chroma_weight_l0[i][1] = hl_codec_264_bits_read_se(p_bits);
                // chroma_offset_l0[i][j] se(v)
                p_slice_header->xs_pred_weight_table.chroma_offset_l0[i][0] = hl_codec_264_bits_read_se(p_bits);
                p_slice_header->xs_pred_weight_table.chroma_offset_l0[i][1] = hl_codec_264_bits_read_se(p_bits);
                //}
            }
        }
    }

    if (p_slice_header->SliceTypeModulo5 == 1) {
        for (i = 0; i <= p_slice_header->num_ref_idx_l1_active_minus1; i++) {
            // luma_weight_l1_flag u(1)
            p_slice_header->xs_pred_weight_table.luma_weight_l1_flag = hl_codec_264_bits_read_u1(p_bits);
            if (p_slice_header->xs_pred_weight_table.luma_weight_l1_flag) {
                // luma_weight_l1[ i ] se(v)
                p_slice_header->xs_pred_weight_table.luma_weight_l1[i] = hl_codec_264_bits_read_se(p_bits);
                // luma_offset_l1[ i ] se(v)
                p_slice_header->xs_pred_weight_table.luma_offset_l1[i] = hl_codec_264_bits_read_se(p_bits);
            }
            if (p_slice_header->pc_pps->pc_sps->ChromaArrayType != 0) {
                // chroma_weight_l1_flag u(1)
                p_slice_header->xs_pred_weight_table.chroma_weight_l1_flag = hl_codec_264_bits_read_u1(p_bits);
                if (p_slice_header->xs_pred_weight_table.chroma_weight_l1_flag) {
                    //for( j = 0; j < 2; j++ ) {
                    // chroma_weight_l1[ i ][ j ] se(v)
                    p_slice_header->xs_pred_weight_table.chroma_weight_l1[i][0] = hl_codec_264_bits_read_se(p_bits);
                    p_slice_header->xs_pred_weight_table.chroma_weight_l1[i][1] = hl_codec_264_bits_read_se(p_bits);
                    // chroma_offset_l1[ i ][ j ] se(v)
                    p_slice_header->xs_pred_weight_table.chroma_offset_l1[i][0] = hl_codec_264_bits_read_se(p_bits);
                    p_slice_header->xs_pred_weight_table.chroma_offset_l1[i][1] = hl_codec_264_bits_read_se(p_bits);
                    //}
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// 7.3.3.3 Decoded reference picture marking syntax
// dec_ref_pic_marking( )
HL_ERROR_T hl_codec_264_rbsp_avc_dec_ref_pic_marking_read(hl_codec_264_nal_slice_header_t *p_slice_header, hl_codec_264_bits_t* p_bits)
{
    if (p_slice_header->IdrFlag) { // IdrPicFlag()
        // no_output_of_prior_pics_flag u(1)
        p_slice_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = hl_codec_264_bits_read_u1(p_bits);
        // long_term_reference_flag u(1)
        p_slice_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = hl_codec_264_bits_read_u1(p_bits);
    }
    else {
        register uint32_t i = 0;
        p_slice_header->xs_dec_ref_base_pic_marking.adaptive_ref_pic_marking_mode_flag = hl_codec_264_bits_read_u1(p_bits);
        if (p_slice_header->xs_dec_ref_base_pic_marking.adaptive_ref_pic_marking_mode_flag) {
            do {
                // memory_management_control_operation ue(v)
                p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] = hl_codec_264_bits_read_ue(p_bits);
                if(p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 1 || p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 3) {
                    // difference_of_pic_nums_minus1 ue(v)
                    p_slice_header->xs_dec_ref_base_pic_marking.difference_of_pic_nums_minus1[i] = hl_codec_264_bits_read_ue(p_bits);
                }
                if(p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 2 ) {
                    // long_term_pic_num ue(v)
                    p_slice_header->xs_dec_ref_base_pic_marking.long_term_pic_num[i] = hl_codec_264_bits_read_ue(p_bits);
                }
                if(p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 3 || p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 6) {
                    // long_term_frame_idx ue(v)
                    p_slice_header->xs_dec_ref_base_pic_marking.long_term_frame_idx[i] = hl_codec_264_bits_read_ue(p_bits);
                }
                if(p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 4) {
                    // max_long_term_frame_idx_plus1 ue(v)
                    p_slice_header->xs_dec_ref_base_pic_marking.max_long_term_frame_idx_plus1[i] = hl_codec_264_bits_read_ue(p_bits);
                }
            }
            while (p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i++] != 0 && i < HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);

            if(i == HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
                HL_DEBUG_ERROR("memory_management_control_operation idx(%d) > %d", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
                return HL_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rbsp_avc_dec_ref_pic_marking_write(hl_codec_264_nal_slice_header_t *p_slice_header, hl_codec_264_bits_t* p_bits)
{
    if (!p_bits || !p_slice_header) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    if (p_slice_header->IdrFlag) {
        // no_output_of_prior_pics_flag u(1)
        hl_codec_264_bits_write_u1(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag);
        // long_term_reference_flag u(1)
        hl_codec_264_bits_write_u1(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.long_term_reference_flag);
    }
    else {
        uint32_t i = 0;
        hl_codec_264_bits_write_u1(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.adaptive_ref_pic_marking_mode_flag);
        if (p_slice_header->xs_dec_ref_base_pic_marking.adaptive_ref_pic_marking_mode_flag) {
            do {
                // memory_management_control_operation ue(v)
                hl_codec_264_bits_write_ue(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i]);
                if (p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 1 || p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 3) {
                    // difference_of_pic_nums_minus1 ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.difference_of_pic_nums_minus1[i]);
                }
                if (p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 2 ) {
                    // long_term_pic_num ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.long_term_pic_num[i]);
                }
                if (p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 3 || p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 6) {
                    // long_term_frame_idx ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.long_term_frame_idx[i]);
                }
                if(p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] == 4) {
                    // max_long_term_frame_idx_plus1 ue(v)
                    hl_codec_264_bits_write_ue(p_bits, p_slice_header->xs_dec_ref_base_pic_marking.max_long_term_frame_idx_plus1[i]);
                }
            }
            while (p_slice_header->xs_dec_ref_base_pic_marking.memory_management_control_operation[i++] != 0 && i < HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);

            if (i >= HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
                HL_DEBUG_ERROR("Index Overflow");
                return HL_ERROR_INVALID_BITSTREAM;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// E.1.1 VUI parameters syntax
// vui_parameters( )
HL_ERROR_T hl_codec_264_rbsp_avc_vui_parameters_read(hl_codec_264_nal_sps_t* p_sps, hl_codec_264_bits_t* p_bits)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    // aspect_ratio_info_present_flag u(1)
    p_sps->p_vui->aspect_ratio_info_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->aspect_ratio_info_present_flag) {
        // aspect_ratio_idc u(8)
        p_sps->p_vui->aspect_ratio_idc = hl_codec_264_bits_read_u(p_bits, 8);
        if (p_sps->p_vui->aspect_ratio_idc == HL_CODEC_264_ASPECT_RATIO_EXTENDED_SAR) {
            // sar_width u(16)
            p_sps->p_vui->sar_width = hl_codec_264_bits_read_u(p_bits, 16);
            // sar_height u(16)
            p_sps->p_vui->sar_height = hl_codec_264_bits_read_u(p_bits, 16);
        }
    }

    // overscan_info_present_flag u(1)
    p_sps->p_vui->overscan_info_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->overscan_info_present_flag) {
        // overscan_appropriate_flag u(1)
        p_sps->p_vui->overscan_appropriate_flag = hl_codec_264_bits_read_u1(p_bits);
    }

    // video_signal_type_present_flag u(1)
    p_sps->p_vui->video_signal_type_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->video_signal_type_present_flag) {
        // video_format u(3)
        p_sps->p_vui->video_format = hl_codec_264_bits_read_u(p_bits, 3);
        // video_full_range_flag u(1)
        p_sps->p_vui->video_full_range_flag = hl_codec_264_bits_read_u1(p_bits);
        // colour_description_present_flag u(1)
        p_sps->p_vui->colour_description_present_flag = hl_codec_264_bits_read_u1(p_bits);
        if (p_sps->p_vui->colour_description_present_flag) {
            // colour_primaries u(8)
            p_sps->p_vui->colour_primaries = hl_codec_264_bits_read_u(p_bits, 8);
            // transfer_characteristics u(8)
            p_sps->p_vui->transfer_characteristics = hl_codec_264_bits_read_u(p_bits, 8);
            // matrix_coefficients u(8)
            p_sps->p_vui->matrix_coefficients = hl_codec_264_bits_read_u(p_bits, 8);
        }
    }

    // chroma_loc_info_present_flag u(1)
    p_sps->p_vui->chroma_loc_info_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->chroma_loc_info_present_flag) {
        // chroma_sample_loc_type_top_field ue(v)
        p_sps->p_vui->chroma_sample_loc_type_top_field = hl_codec_264_bits_read_ue(p_bits);
        // chroma_sample_loc_type_bottom_field ue(v)
        p_sps->p_vui->chroma_sample_loc_type_bottom_field = hl_codec_264_bits_read_ue(p_bits);
    }

    // timing_info_present_flag u(1)
    p_sps->p_vui->timing_info_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->timing_info_present_flag) {
        // num_units_in_tick u(32)
        p_sps->p_vui->num_units_in_tick = hl_codec_264_bits_read_u(p_bits, 32);
        // time_scale u(32)
        p_sps->p_vui->time_scale = hl_codec_264_bits_read_u(p_bits, 32);
        // fixed_frame_rate_flag u(1)
        p_sps->p_vui->fixed_frame_rate_flag = hl_codec_264_bits_read_u1(p_bits);
    }

    // nal_hrd_parameters_present_flag u(1)
    p_sps->p_vui->nal_hrd_parameters_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->nal_hrd_parameters_present_flag) {
        // hrd_parameters( )
        err = hl_codec_264_hrd_create(&p_sps->p_vui->p_hrd_nal);
        if (err) {
            return err;
        }
        err = hl_codec_264_rbsp_avc_hrd_parameters_read(p_sps->p_vui->p_hrd_nal, p_bits);
        if (err) {
            return err;
        }
    }

    // vcl_hrd_parameters_present_flag u(1)
    p_sps->p_vui->vcl_hrd_parameters_present_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_sps->p_vui->vcl_hrd_parameters_present_flag) {
        // hrd_parameters( )
        err = hl_codec_264_hrd_create(&p_sps->p_vui->p_hrd_vcl);
        if (err) {
            return err;
        }
        err = hl_codec_264_rbsp_avc_hrd_parameters_read(p_sps->p_vui->p_hrd_vcl, p_bits);
        if (err) {
            return err;
        }
    }

    if (p_sps->p_vui->nal_hrd_parameters_present_flag || p_sps->p_vui->vcl_hrd_parameters_present_flag) {
        // low_delay_hrd_flag u(1)
        p_sps->p_vui->low_delay_hrd_flag = hl_codec_264_bits_read_u1(p_bits);
    }

    // pic_struct_present_flag u(1)
    p_sps->p_vui->pic_struct_present_flag = hl_codec_264_bits_read_u1(p_bits);
    // bitstream_restriction_flag u(1)
    p_sps->p_vui->bitstream_restriction_flag = hl_codec_264_bits_read_u1(p_bits);

    if (p_sps->p_vui->bitstream_restriction_flag) {
        // motion_vectors_over_pic_boundaries_flag u(1)
        p_sps->p_vui->motion_vectors_over_pic_boundaries_flag = hl_codec_264_bits_read_u1(p_bits);
        // max_bytes_per_pic_denom ue(v)
        p_sps->p_vui->max_bytes_per_pic_denom = hl_codec_264_bits_read_ue(p_bits);
        // max_bits_per_mb_denom ue(v)
        p_sps->p_vui->max_bits_per_mb_denom = hl_codec_264_bits_read_ue(p_bits);
        // log2_max_mv_length_horizontal ue(v)
        p_sps->p_vui->log2_max_mv_length_horizontal = hl_codec_264_bits_read_ue(p_bits);
        // log2_max_mv_length_vertical ue(v)
        p_sps->p_vui->log2_max_mv_length_vertical = hl_codec_264_bits_read_ue(p_bits);
        // num_reorder_frames ue(v)
        p_sps->p_vui->num_reorder_frames = hl_codec_264_bits_read_ue(p_bits);
        // max_dec_frame_buffering ue(v)
        p_sps->p_vui->max_dec_frame_buffering = hl_codec_264_bits_read_ue(p_bits);
    }

    return err;
}

// E.1.2 HRD parameters syntax
// hrd_parameters( )
HL_ERROR_T hl_codec_264_rbsp_avc_hrd_parameters_read(hl_codec_264_hrd_t* p_hrd, hl_codec_264_bits_t* p_bits)
{
    register uint32_t SchedSelIdx;

    // cpb_cnt_minus1 ue(v)
    p_hrd->cpb_cnt_minus1 = hl_codec_264_bits_read_ue(p_bits);
    // bit_rate_scale u(4)
    p_hrd->bit_rate_scale = hl_codec_264_bits_read_u(p_bits, 4);
    // cpb_size_scale u(4)
    p_hrd->cpb_size_scale = hl_codec_264_bits_read_u(p_bits, 4);

    for (SchedSelIdx = 0; SchedSelIdx <= p_hrd->cpb_cnt_minus1; SchedSelIdx++) {
        //cbit_rate_value_minus1[ SchedSelIdx ] ue(v)
        p_hrd->bit_rate_value_minus1[SchedSelIdx] = hl_codec_264_bits_read_ue(p_bits);
        //cpb_size_value_minus1[ SchedSelIdx ] ue(v)
        p_hrd->cpb_size_value_minus1[SchedSelIdx] = hl_codec_264_bits_read_ue(p_bits);
        //cbr_flag[ SchedSelIdx ] u(1)
        p_hrd->cbr_flag[SchedSelIdx] = hl_codec_264_bits_read_u1(p_bits);
    }

    // initial_cpb_removal_delay_length_minus1 u(5)
    p_hrd->initial_cpb_removal_delay_length_minus1 = hl_codec_264_bits_read_u(p_bits, 5);
    // cpb_removal_delay_length_minus1 u(5)
    p_hrd->cpb_removal_delay_length_minus1 = hl_codec_264_bits_read_u(p_bits, 5);
    // dpb_output_delay_length_minus1 u(5)
    p_hrd->dpb_output_delay_length_minus1 = hl_codec_264_bits_read_u(p_bits, 5);
    // time_offset_length u(5)
    p_hrd->time_offset_length = hl_codec_264_bits_read_u(p_bits, 5);

    return HL_ERROR_SUCCESS;
}


HL_ERROR_T hl_codec_264_rbsp_avc_escape(uint8_t* p_rbsp_bytes, hl_size_t u_rbsp_bytes_num, hl_size_t u_rbsp_bytes_size)
{
    hl_size_t i, zeroBytesCount;
    for (i = 0, zeroBytesCount = 0; i < u_rbsp_bytes_num; ++i) {
        if (zeroBytesCount == 2) {
            if (p_rbsp_bytes[i]==0x01 || (p_rbsp_bytes[i]==0x00 && i+1 < u_rbsp_bytes_num && p_rbsp_bytes[i]==0x01)) {
                // TODO: not fully tested code
                HL_DEBUG_INFO("zeroBytesCount == 2");
                if ((u_rbsp_bytes_num + 1) >= u_rbsp_bytes_size) {
                    HL_DEBUG_ERROR("Memory too short"); // Must never happen
                    return HL_ERROR_TOOSHORT;
                }
                hl_memory_copy(&p_rbsp_bytes[i + 1], &p_rbsp_bytes[i], (u_rbsp_bytes_num - i + 1));
                u_rbsp_bytes_num++;
                p_rbsp_bytes[i++] = 0x03; //emulation_prevention_three_byte
            }
            zeroBytesCount = 0;
        }

        zeroBytesCount = p_rbsp_bytes[i] ? 0 : (zeroBytesCount + 1);
    }

    return HL_ERROR_SUCCESS;
}




/********** Scalable Video Coding (SVC) **********/

// G.7.3.3.5 Decoded reference base picture marking syntax
// dec_ref_base_pic_marking( )
HL_ERROR_T hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_read(hl_codec_264_bits_t* p_bits, hl_codec_264_svc_dec_ref_base_pic_marking_xt *p_dec_ref)
{
    // adaptive_ref_base_pic_marking_mode_flag u(1)
    p_dec_ref->adaptive_ref_base_pic_marking_mode_flag = hl_codec_264_bits_read_u1(p_bits);
    if (p_dec_ref->adaptive_ref_base_pic_marking_mode_flag) {
        register uint32_t i = 0;
        do {
            // memory_management_base_control_operation 2 ue(v)
            p_dec_ref->memory_management_base_control_operation[i] = hl_codec_264_bits_read_ue(p_bits);
            if (p_dec_ref->memory_management_base_control_operation[i] == 1) {
                // difference_of_base_pic_nums_minus1 2 ue(v)
                p_dec_ref->difference_of_base_pic_nums_minus1[i] = hl_codec_264_bits_read_ue(p_bits);
            }
            if (p_dec_ref->memory_management_base_control_operation[i] == 2) {
                // long_term_base_pic_num 2 ue(v)
                p_dec_ref->long_term_base_pic_num[i] = hl_codec_264_bits_read_ue(p_bits);
            }
        }
        while (p_dec_ref->memory_management_base_control_operation[i++] != 0 && i < HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);

        if (i >= HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
            HL_DEBUG_ERROR("memory_management_base_control_operation(%d) > %d", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
            return HL_ERROR_INVALID_BITSTREAM;
        }
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_write(hl_codec_264_bits_t* p_bits, hl_codec_264_svc_dec_ref_base_pic_marking_xt *p_dec_ref)
{
    // adaptive_ref_base_pic_marking_mode_flag u(1)
    hl_codec_264_bits_write_u1(p_bits, p_dec_ref->adaptive_ref_base_pic_marking_mode_flag);
    if (p_dec_ref->adaptive_ref_base_pic_marking_mode_flag) {
        register uint32_t i = 0;
        do {
            // memory_management_base_control_operation 2 ue(v)
            hl_codec_264_bits_write_ue(p_bits, p_dec_ref->memory_management_base_control_operation[i]);
            if (p_dec_ref->memory_management_base_control_operation[i] == 1) {
                // difference_of_base_pic_nums_minus1 2 ue(v)
                hl_codec_264_bits_write_ue(p_bits, p_dec_ref->difference_of_base_pic_nums_minus1[i]);
            }
            if (p_dec_ref->memory_management_base_control_operation[i] == 2) {
                // long_term_base_pic_num 2 ue(v)
                hl_codec_264_bits_write_ue(p_bits, p_dec_ref->long_term_base_pic_num[i]);
            }
        }
        while (p_dec_ref->memory_management_base_control_operation[i++] != 0 && i < HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);

        if (i >= HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT) {
            HL_DEBUG_ERROR("memory_management_base_control_operation(%d) > %d", i, HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT);
            return HL_ERROR_INVALID_BITSTREAM;
        }
    }
    return HL_ERROR_SUCCESS;
}
