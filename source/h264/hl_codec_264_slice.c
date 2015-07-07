#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_rc.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_rbsp.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_residual.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_pred_intra.h"
#include "hartallo/h264/hl_codec_264_pred_inter.h"
#include "hartallo/h264/hl_codec_264_decode_svc.h"
#include "hartallo/h264/hl_codec_264_decode_avc.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_deblock.h"
#include "hartallo/h264/hl_codec_264_encode.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_md5.h" // FIXME: remove
#include "hartallo/hl_string.h" // FIXME: remove
#include "hartallo/hl_debug.h"

#if HL_UNDER_WINDOWS
#	include <windows.h>
#endif /* HL_UNDER_WINDOWS */

extern const hl_object_def_t *hl_codec_264_nal_slice_header_def_t;
extern const hl_object_def_t *hl_codec_264_slice_def_t;
static long __slice_id = 0;

HL_ERROR_T hl_codec_264_nal_slice_header_create(unsigned u_ref_idc, HL_CODEC_264_NAL_TYPE_T e_type, hl_codec_264_nal_slice_header_t** pp_slice_header)
{
    if (!pp_slice_header) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_slice_header = hl_object_create(hl_codec_264_nal_slice_header_def_t);
    if (!*pp_slice_header) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_slice_header)->l_id = hl_math_atomic_inc(&__slice_id);
    hl_codec_264_nal_init((hl_codec_264_nal_t*)*pp_slice_header, e_type, u_ref_idc);
    return HL_ERROR_SUCCESS;
}

static HL_ALWAYS_INLINE HL_ERROR_T _hl_codec_264_nal_slice_header_derive(hl_codec_264_nal_slice_header_t* p_slice_header, const hl_codec_264_t* pc_codec)
{
    // FIXME: many values could be move into p_codec and computed only if 'field_pic_flag' change
    p_slice_header->SliceTypeModulo5 = (p_slice_header->slice_type % 5);
    // PicWidthInMbs = pic_width_in_mbs_minus1 + 1 (7-12)
    p_slice_header->PicWidthInMbs = p_slice_header->pc_pps->pc_sps->pic_width_in_mbs_minus1 + 1;
    // PicHeightInMapUnits = pic_height_in_map_units_minus1 + 1 (7-15)
    p_slice_header->PicHeightInMapUnits = p_slice_header->pc_pps->pc_sps->pic_height_in_map_units_minus1 + 1;
    // MbaffFrameFlag = ( mb_adaptive_frame_field_flag && !field_pic_flag ) (7-24)
    p_slice_header->MbaffFrameFlag = p_slice_header->pc_pps->pc_sps->mb_adaptive_frame_field_flag && !p_slice_header->field_pic_flag;
    // FrameHeightInMbs = ( 2 ? frame_mbs_only_flag ) * PicHeightInMapUnits (7-17)
    p_slice_header->FrameHeightInMbs = (2 - p_slice_header->pc_pps->pc_sps->frame_mbs_only_flag) * p_slice_header->PicHeightInMapUnits;
    // PicHeightInMbs = FrameHeightInMbs / ( 1 + field_pic_flag ) (7-25)
    p_slice_header->PicHeightInMbs = p_slice_header->FrameHeightInMbs / (1 + p_slice_header->field_pic_flag);
    // PicSizeInMbs = PicWidthInMbs * PicHeightInMbs (7-28)
    p_slice_header->PicSizeInMbs = p_slice_header->PicWidthInMbs * p_slice_header->PicHeightInMbs;
    // PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits
    p_slice_header->PicSizeInMapUnits = p_slice_header->PicWidthInMbs * p_slice_header->PicHeightInMapUnits;
    // PicWidthInSamplesL = PicWidthInMbs * 16 (7-13)
    p_slice_header->PicWidthInSamplesL = p_slice_header->PicWidthInMbs << 4;
    // PicWidthInSamplesC = PicWidthInMbs * MbWidthC
    p_slice_header->PicWidthInSamplesC = p_slice_header->PicWidthInMbs * p_slice_header->pc_pps->pc_sps->MbWidthC;
    // PicHeightInSamplesL = PicHeightInMbs * 16 (7-26)
    p_slice_header->PicHeightInSamplesL = p_slice_header->PicHeightInMbs << 4;
    // PicHeightInSamplesC = PicHeightInMbs * MbHeightC (7-27)
    p_slice_header->PicHeightInSamplesC = p_slice_header->PicHeightInMbs * p_slice_header->pc_pps->pc_sps->MbHeightC;

    // MapUnitsInSliceGroup0 = Min( slice_group_change_cycle * SliceGroupChangeRate, PicSizeInMapUnits)
    p_slice_header->MapUnitsInSliceGroup0 = HL_MATH_MIN(p_slice_header->slice_group_change_cycle * p_slice_header->pc_pps->SliceGroupChangeRate, p_slice_header->PicSizeInMapUnits);
    if (p_slice_header->MbToSliceGroupMapCount != p_slice_header->PicSizeInMbs) {
        p_slice_header->MbToSliceGroupMap = (int32_t*)hl_memory_realloc(p_slice_header->MbToSliceGroupMap, sizeof(int32_t)*p_slice_header->PicSizeInMbs);
        if (!p_slice_header->MbToSliceGroupMap) {
            p_slice_header->MbToSliceGroupMapCount = 0;
            return HL_ERROR_OUTOFMEMMORY;
        }
        p_slice_header->MbToSliceGroupMapCount = p_slice_header->PicSizeInMbs;
    }
    // SliceQPY = 26 + pic_init_qp_minus26 + slice_qp_delta (7-29)
    p_slice_header->SliceQPY = 26 + p_slice_header->pc_pps->pic_init_qp_minus26 + p_slice_header->slice_qp_delta;
    // QSY = 26 + pic_init_qs_minus26 + slice_qs_delta (7-30)
    p_slice_header->QSY = 26 + p_slice_header->pc_pps->pic_init_qs_minus26 + p_slice_header->slice_qs_delta;

    // FilterOffsetA = slice_alpha_c0_offset_div2 << 1 (7-31)
    p_slice_header->FilterOffsetA = p_slice_header->slice_alpha_c0_offset_div2 << 1;
    // FilterOffsetB = slice_beta_offset_div2 << 1 (7-32)
    p_slice_header->FilterOffsetB = p_slice_header->slice_beta_offset_div2 << 1;

    if (p_slice_header->SVCExtFlag) {
        int32_t scaledLeftOffset, scaledTopOffset, scaledRightOffset, scaledBottomOffset;
        if (pc_codec->nal_current.ext.svc.no_inter_layer_pred_flag || pc_codec->nal_current.ext.svc.quality_id != 0) {
            uint32_t DQId = (pc_codec->nal_current.ext.svc.dependency_id << 4) + pc_codec->nal_current.ext.svc.quality_id; // (G-61)
            // ref_layer_dq_id not present
            p_slice_header->ext.svc.ref_layer_dq_id = (pc_codec->nal_current.ext.svc.quality_id > 0) ? (p_slice_header->ext.svc.DQId - 1) : -1;
        }

        p_slice_header->ext.svc.InterlayerFilterOffsetA = p_slice_header->ext.svc.inter_layer_slice_alpha_c0_offset_div2 << 1; // (G-62)
        p_slice_header->ext.svc.InterlayerFilterOffsetB = p_slice_header->ext.svc.inter_layer_slice_beta_offset_div2 << 1; // (G-63)

        if (!p_slice_header->pc_pps->pc_sps->p_svc || p_slice_header->pc_pps->pc_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT) {
            // 'scaled_ref_layer_xxx' present
        }
        else {
            // 'scaled_ref_layer_xxx' not present
            p_slice_header->ext.svc.scaled_ref_layer_left_offset = (pc_codec->nal_current.ext.svc.quality_id > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_left_offset;
            p_slice_header->ext.svc.scaled_ref_layer_top_offset = (pc_codec->nal_current.ext.svc.quality_id > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_top_offset;
            p_slice_header->ext.svc.scaled_ref_layer_right_offset = (pc_codec->nal_current.ext.svc.quality_id > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_right_offset;
            p_slice_header->ext.svc.scaled_ref_layer_bottom_offset = (pc_codec->nal_current.ext.svc.quality_id > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_bottom_offset;

            // "ref_layer_chroma_phase_x_plus1_flag" and "ref_layer_chroma_phase_y_plus1" not present
            if (p_slice_header->ext.svc.QualityId > 0) {
                p_slice_header->ext.svc.ref_layer_chroma_phase_x_plus1_flag = p_slice_header->pc_pps->pc_sps->p_svc->chroma_phase_x_plus1_flag;
                p_slice_header->ext.svc.ref_layer_chroma_phase_y_plus1 = p_slice_header->pc_pps->pc_sps->p_svc->chroma_phase_y_plus1;
            }
            else {
                p_slice_header->ext.svc.ref_layer_chroma_phase_x_plus1_flag = p_slice_header->pc_pps->pc_sps->p_svc->seq_ref_layer_chroma_phase_x_plus1_flag;
                p_slice_header->ext.svc.ref_layer_chroma_phase_y_plus1 = p_slice_header->pc_pps->pc_sps->p_svc->seq_ref_layer_chroma_phase_y_plus1;
            }
        }


        // FIXME
        // – If MinNoInterLayerPredFlag is equal to 0, scaledLeftOffset, scaledRightOffset, scaledTopOffset, and
        // 	scaledBottomOffset are set equal to the values of scaled_ref_layer_left_offset, scaled_ref_layer_right_offset,
        // 	scaled_ref_layer_top_offset, and scaled_ref_layer_bottom_offset, respectively, for the slices of the current layer
        // 	representation that have no_inter_layer_pred_flag equal to 0.
        // – Otherwise (MinNoInterLayerPredFlag is equal to 1), scaledLeftOffset, scaledRightOffset, scaledTopOffset, and
        // 	scaledBottomOffset are set equal to the values of scaled_ref_layer_left_offset, scaled_ref_layer_right_offset,
        // 	scaled_ref_layer_top_offset, and scaled_ref_layer_bottom_offset, respectively.
        if(p_slice_header->ext.svc.MinNoInterLayerPredFlag) {
            scaledLeftOffset = p_slice_header->ext.svc.scaled_ref_layer_left_offset;
            scaledTopOffset = p_slice_header->ext.svc.scaled_ref_layer_top_offset;
            scaledRightOffset = p_slice_header->ext.svc.scaled_ref_layer_right_offset;
            scaledBottomOffset = p_slice_header->ext.svc.scaled_ref_layer_bottom_offset;
        }
        else {
            scaledLeftOffset = p_slice_header->ext.svc.scaled_ref_layer_left_offset;
            scaledTopOffset = p_slice_header->ext.svc.scaled_ref_layer_top_offset;
            scaledRightOffset = p_slice_header->ext.svc.scaled_ref_layer_right_offset;
            scaledBottomOffset = p_slice_header->ext.svc.scaled_ref_layer_bottom_offset;
        }

        p_slice_header->ext.svc.ScaledRefLayerLeftOffset = (scaledLeftOffset << 1); // (G-64)
        p_slice_header->ext.svc.ScaledRefLayerRightOffset = (scaledRightOffset << 1); // (G-65)
        p_slice_header->ext.svc.ScaledRefLayerTopOffset = ((scaledTopOffset << 1) << (2 - p_slice_header->pc_pps->pc_sps->frame_mbs_only_flag - 1)); // (G-66)
        p_slice_header->ext.svc.ScaledRefLayerBottomOffset = ((scaledBottomOffset << 1) << (2 - p_slice_header->pc_pps->pc_sps->frame_mbs_only_flag - 1)); // (G-67)
        p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL = (p_slice_header->PicWidthInMbs << 4) - p_slice_header->ext.svc.ScaledRefLayerLeftOffset - p_slice_header->ext.svc.ScaledRefLayerRightOffset; // (G-68)
        p_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL = (p_slice_header->PicHeightInMbs << 4) - ((p_slice_header->ext.svc.ScaledRefLayerTopOffset + p_slice_header->ext.svc.ScaledRefLayerBottomOffset) >> (1 + p_slice_header->field_pic_flag - 1)); //(G-69)

        if(p_slice_header->pc_pps->pc_sps->ChromaArrayType != 0) {
            p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesC = (p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL >> p_slice_header->pc_pps->pc_sps->SubWidthC_TrailingZeros);// (G-70)
            p_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesC = (p_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL >> p_slice_header->pc_pps->pc_sps->SubHeightC_TrailingZeros);// (G-71)
        }

        p_slice_header->ext.svc.CroppingChangeFlag = (!p_slice_header->ext.svc.MinNoInterLayerPredFlag && pc_codec->nal_current.ext.svc.quality_id >0 && p_slice_header->pc_pps->pc_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT);




//
//
//
////		int32_t scaledLeftOffset, scaledRightOffset, scaledTopOffset, scaledBottomOffset;
//        p_slice_header->ext.svc.DQId = (p_slice_header->ext.svc.DependencyId << 4) + p_slice_header->ext.svc.QualityId; // (G-61)
//
//        // maximum value of DQId for all coded slice NAL units
//
//        if (p_slice_header->ext.svc.NoInterLayerPredFlag || p_slice_header->ext.svc.QualityId != 0) {
//            // ref_layer_dq_id not present
//            p_slice_header->ext.svc.ref_layer_dq_id = (p_slice_header->ext.svc.QualityId > 0) ? (p_slice_header->ext.svc.DQId - 1) : -1;
//        }
//
//        // The variable MinNoInterLayerPredFlag is set equal to the minimum value of no_inter_layer_pred_flag for the slices of
//        // the layer representation.
//        // FIXME: must be set to zero when we finish decoding all layers for this picture
//        // FIXME: MIN(Flag) = if (0) = 0;
//        p_slice_header->ext.svc.MinNoInterLayerPredFlag = HL_MATH_MIN(p_slice_header->ext.svc.MinNoInterLayerPredFlag, p_slice_header->ext.svc.NoInterLayerPredFlag);
//
//        // The variable MaxRefLayerDQId is set equal to the maximum value of ref_layer_dq_id for the slices of the current layer
//        // representation.
//        // FIXME: must be set to zero when we finish decoding all layers for this picture
//        p_slice_header->ext.svc.MaxRefLayerDQId = HL_MATH_MAX(p_slice_header->ext.svc.MaxRefLayerDQId, p_slice_header->ext.svc.ref_layer_dq_id);
//
//        // (G-62)
//        p_slice_header->ext.svc.InterlayerFilterOffsetA = p_slice_header->ext.svc.inter_layer_slice_alpha_c0_offset_div2 << 1;
//
//        // (G-63)
//        p_slice_header->ext.svc.InterlayerFilterOffsetB = p_slice_header->ext.svc.inter_layer_slice_beta_offset_div2 << 1;

        //if (p_slice_header->pc_pps->pc_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT) {
        //	// 'scaled_ref_layer_xxx' present
        //
        //}
        //else {
        //	// 'scaled_ref_layer_xxx' not present
        //	p_slice_header->ext.svc.scaled_ref_layer_left_offset = (p_slice_header->QualityId > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_left_offset;
        //	p_slice_header->ext.svc.scaled_ref_layer_top_offset = (p_slice_header->QualityId > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_top_offset;
        //	p_slice_header->ext.svc.scaled_ref_layer_right_offset = (p_slice_header->QualityId > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_right_offset;
        //	p_slice_header->ext.svc.scaled_ref_layer_bottom_offset = (p_slice_header->QualityId > 0) ? 0 : p_slice_header->pc_pps->pc_sps->p_svc->seq_scaled_ref_layer_bottom_offset;
        //}

        //// FIXME
        //// – If MinNoInterLayerPredFlag is equal to 0, scaledLeftOffset, scaledRightOffset, scaledTopOffset, and
        //// 	scaledBottomOffset are set equal to the values of scaled_ref_layer_left_offset, scaled_ref_layer_right_offset,
        //// 	scaled_ref_layer_top_offset, and scaled_ref_layer_bottom_offset, respectively, for the slices of the current layer
        //// 	representation that have no_inter_layer_pred_flag equal to 0.
        //// – Otherwise (MinNoInterLayerPredFlag is equal to 1), scaledLeftOffset, scaledRightOffset, scaledTopOffset, and
        //// 	scaledBottomOffset are set equal to the values of scaled_ref_layer_left_offset, scaled_ref_layer_right_offset,
        //// 	scaled_ref_layer_top_offset, and scaled_ref_layer_bottom_offset, respectively.
        //if(p_slice_header->ext.svc.MinNoInterLayerPredFlag){
        //	scaledLeftOffset = p_slice_header->ext.svc.scaled_ref_layer_left_offset;
        //	scaledTopOffset = p_slice_header->ext.svc.scaled_ref_layer_top_offset;
        //	scaledRightOffset = p_slice_header->ext.svc.scaled_ref_layer_right_offset;
        //	scaledBottomOffset = p_slice_header->ext.svc.scaled_ref_layer_bottom_offset;
        //}
        //else{
        //	scaledLeftOffset = p_slice_header->ext.svc.scaled_ref_layer_left_offset;
        //	scaledTopOffset = p_slice_header->ext.svc.scaled_ref_layer_top_offset;
        //	scaledRightOffset = p_slice_header->ext.svc.scaled_ref_layer_right_offset;
        //	scaledBottomOffset = p_slice_header->ext.svc.scaled_ref_layer_bottom_offset;
        //}

        //p_slice_header->ext.svc.ScaledRefLayerLeftOffset = (scaledLeftOffset << 1); // (G-64)
        //p_slice_header->ext.svc.ScaledRefLayerRightOffset = (scaledRightOffset << 1); // (G-65)
        //p_slice_header->ext.svc.ScaledRefLayerTopOffset = ((scaledTopOffset << 1) << (2 - p_slice_header->pc_pps->pc_sps->frame_mbs_only_flag - 1)); // (G-66)
        //p_slice_header->ext.svc.ScaledRefLayerBottomOffset = ((scaledBottomOffset << 1) << (2 - p_slice_header->pc_pps->pc_sps->frame_mbs_only_flag - 1)); // (G-67)
        //p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL = (p_slice_header->PicWidthInMbs << 4) - p_slice_header->ext.svc.ScaledRefLayerLeftOffset - p_slice_header->ext.svc.ScaledRefLayerRightOffset; // (G-68)
        //p_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL = (p_slice_header->PicHeightInMbs << 4) - ((p_slice_header->ext.svc.ScaledRefLayerTopOffset + p_slice_header->ext.svc.ScaledRefLayerBottomOffset) >> (1 + p_slice_header->field_pic_flag - 1)); //(G-69)

        //if(p_slice_header->pc_pps->pc_sps->ChromaArrayType != 0){
        //	p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesC = (p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL >> p_slice_header->pc_pps->pc_sps->SubWidthC_TrailingZeros);// (G-70)
        //	p_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesC = (p_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL >> p_slice_header->pc_pps->pc_sps->SubWidthC_TrailingZeros);// (G-71)
        //}

        //p_slice_header->ext.svc.CroppingChangeFlag = (!p_slice_header->ext.svc.MinNoInterLayerPredFlag && p_slice_header->QualityId >0 && p_slice_header->pc_pps->pc_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT);

        ///*if((p_slice_header->ext.svc.MinNoInterLayerPredFlag)
        //	|| (p_slice_header->QualityId > 0)
        //	||
        //	(
        //		page 450
        //		(!p_slice_header->ext.svc.CroppingChangeFlag) &&
        //		(p_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL == p_slice_header->ext.svc.RefLay)
        //	)
        //	)
        //{
        //	(!p_slice_header->ext.svc.CroppingChangeFlag
        //}
        //else{
        //	p_slice_header->ext.svc.SpatialResolutionChangeFlag = 1;
        //}*/
    } // end-of 'if(p_slice_header->SVCExtFlag)'

    return HL_ERROR_SUCCESS;
}

static HL_ALWAYS_INLINE HL_ERROR_T _hl_codec_264_nal_slice_header_reset(hl_codec_264_nal_slice_header_t* p_header, HL_CODEC_264_NAL_TYPE_T e_nal_type, uint32_t u_nal_ref_idc)
{
    // Set all values to zero. must not use memset() because the struct contains pointers

    // set nal header values
    HL_CODEC_264_NAL(p_header)->u_forbidden_zero_bit = 0;
    HL_CODEC_264_NAL(p_header)->e_type = e_nal_type;
    HL_CODEC_264_NAL(p_header)->u_ref_idc = u_nal_ref_idc;
    // reset values

    p_header->l_id = hl_math_atomic_inc(&__slice_id);
    p_header->colour_plane_id = 0;
    p_header->field_pic_flag = 0;
    p_header->bottom_field_flag = 0;
    p_header->idr_pic_id = 0;
    p_header->pic_order_cnt_lsb = 0;
    p_header->delta_pic_order_cnt_bottom = 0;
    p_header->delta_pic_order_cnt[0] = p_header->delta_pic_order_cnt[1] = 0;
    p_header->redundant_pic_cnt = 0;
    p_header->direct_spatial_mv_pred_flag = 0;
    p_header->num_ref_idx_active_override_flag = 0;
    p_header->num_ref_idx_l0_active_minus1 = 0;
    p_header->num_ref_idx_l1_active_minus1 = 0;
    p_header->num_ref_idx_l0_active_minus1 = 0;
    p_header->num_ref_idx_l1_active_minus1 = 0;
    p_header->cabac_init_idc = 0;
    p_header->sp_for_switch_flag = 0;
    p_header->slice_qs_delta = 0;
    p_header->disable_deblocking_filter_idc = 0;
    p_header->slice_alpha_c0_offset_div2 = 0;
    p_header->slice_beta_offset_div2 = 0;
    p_header->slice_group_change_cycle = 0;

    if (p_header->MbToSliceGroupMap) {
        memset(p_header->MbToSliceGroupMap, 0, sizeof(int32_t) * p_header->PicSizeInMbs);
    }
    if (e_nal_type == HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION) {
        memset(&p_header->ext, 0, sizeof(p_header->ext));
    }

    return HL_ERROR_SUCCESS;
}


// 7.3.3 Slice header syntax
// 7.4.3 Slice header semantics
// G.7.3.3.4 Slice header in scalable extension syntax
// slice_header( ) or slice_header_in_scalable_extension( )
// Always "slice_header_in_scalable_extension ( )" as it extends "slice_header ()" while keeping compatibility
// If "*pp_slice_header" not null then reset().
HL_ERROR_T hl_codec_264_nal_slice_header_decode(hl_codec_264_t* p_codec, hl_codec_264_nal_slice_header_t** pp_slice_header)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    const hl_codec_264_nal_pps_t *pc_pps  = HL_NULL;
    hl_codec_264_nal_slice_header_t* p_slice_header;

    uint32_t u_tmp, u_first_mb_in_slice, u_pic_parameter_set_id;
    HL_CODEC_264_SLICE_TYPE_T e_slice_type;

    if (!p_codec || !p_codec->pobj_bits || !pp_slice_header) {
        HL_DEBUG_ERROR("Invalid parameter");
        err = HL_ERROR_INVALID_PARAMETER;
        goto bail;
    }

    // Reset slice header if already exist
    if ((p_slice_header = (*pp_slice_header))) {
        err = _hl_codec_264_nal_slice_header_reset(p_slice_header, p_codec->nal_current.e_nal_type, p_codec->nal_current.i_nal_ref_idc);
        if (err) {
            goto bail;
        }
    }

    // first_mb_in_slice ue(v)
    u_first_mb_in_slice = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // slice_type ue(v)
    e_slice_type = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    // pic_parameter_set_id ue(v)
    u_pic_parameter_set_id = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    if (u_pic_parameter_set_id >= HL_CODEC_264_PPS_MAX_COUNT) {
        HL_DEBUG_ERROR("Invalid 'pic_parameter_set_id' (%d)", u_pic_parameter_set_id);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    if (!(pc_pps = p_codec->pps.p_list[u_pic_parameter_set_id])) {
        HL_DEBUG_ERROR("'pic_parameter_set_id'(%d) doesn't exist", u_pic_parameter_set_id);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }

    // Create the slice header
    if (!p_slice_header) {
        err = hl_codec_264_nal_slice_header_create(p_codec->nal_current.i_nal_ref_idc, p_codec->nal_current.e_nal_type, pp_slice_header);
        if (err) {
            goto bail;
        }
        p_slice_header = *pp_slice_header;
    }

    p_slice_header->first_mb_in_slice = u_first_mb_in_slice;
    p_slice_header->slice_type = e_slice_type;
    p_slice_header->pic_parameter_set_id = u_pic_parameter_set_id;
    p_slice_header->pc_pps = pc_pps; // used by pred_weight_table( )
    p_slice_header->SliceTypeModulo5 = (e_slice_type % 5); // used by pred_weight_table( )
    p_slice_header->SVCExtFlag = p_codec->nal_current.svc_extension_flag;

    if (p_slice_header->SVCExtFlag) {
        p_slice_header->IdrFlag = p_codec->nal_current.ext.svc.idr_flag;
        p_slice_header->ext.svc.PriorityId = p_codec->nal_current.ext.svc.priority_id;
        p_slice_header->ext.svc.NoInterLayerPredFlag = p_codec->nal_current.ext.svc.no_inter_layer_pred_flag;
        p_slice_header->ext.svc.DependencyId = p_codec->nal_current.ext.svc.dependency_id;
        p_slice_header->ext.svc.QualityId = p_codec->nal_current.ext.svc.quality_id;
        p_slice_header->ext.svc.TemporalId = p_codec->nal_current.ext.svc.temporal_id;
        p_slice_header->ext.svc.UseRefBasePicFlag = p_codec->nal_current.ext.svc.use_ref_base_pic_flag;
        p_slice_header->ext.svc.DiscardableFlag = p_codec->nal_current.ext.svc.discardable_flag;
        p_slice_header->ext.svc.OutputFlag = p_codec->nal_current.ext.svc.output_flag;
    }
    else {
        p_slice_header->IdrFlag = IdrPicFlag(p_codec->nal_current.e_nal_type);
    }

    if (pc_pps->pc_sps->separate_colour_plane_flag) {
        // colour_plane_id u(2)
        p_slice_header->colour_plane_id = hl_codec_264_bits_read_u(p_codec->pobj_bits, 2);
    }
    // frame_num u(v)
    p_slice_header->frame_num = hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_pps->pc_sps->log2_max_frame_num_minus4 + 4);

    if (!pc_pps->pc_sps->frame_mbs_only_flag) {
        // field_pic_flag u(1)
        p_slice_header->field_pic_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        if (p_slice_header->field_pic_flag) {
            // bottom_field_flag u(1)
            p_slice_header->bottom_field_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        }
    }

    if (p_slice_header->IdrFlag) {
        // idr_pic_id ue(v)
        p_slice_header->idr_pic_id = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    }

    if (pc_pps->pc_sps->pic_order_cnt_type == 0) {
        // pic_order_cnt_lsb u(v)
        p_slice_header->pic_order_cnt_lsb = hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_pps->pc_sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pc_pps->bottom_field_pic_order_in_frame_present_flag && !p_slice_header->field_pic_flag) {
            // delta_pic_order_cnt_bottom se(v)
            p_slice_header->delta_pic_order_cnt_bottom = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        }
    }

    if (pc_pps->pc_sps->pic_order_cnt_type == 1 && !pc_pps->pc_sps->delta_pic_order_always_zero_flag) {
        // delta_pic_order_cnt[ 0 ] se(v)
        p_slice_header->delta_pic_order_cnt[0] = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        if (pc_pps->bottom_field_pic_order_in_frame_present_flag && !p_slice_header->field_pic_flag) {
            // delta_pic_order_cnt[ 1 ] se(v)
            p_slice_header->delta_pic_order_cnt[1] = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        }
    }

    if (pc_pps->redundant_pic_cnt_present_flag) {
        // redundant_pic_cnt ue(v)
        p_slice_header->redundant_pic_cnt = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    }

    if (p_slice_header->ext.svc.QualityId == 0) {
        if (IsSliceHeaderEB(p_slice_header)) {
            // direct_spatial_mv_pred_flag u(1)
            p_slice_header->direct_spatial_mv_pred_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        }

        if (IsSliceHeaderEP(p_slice_header) || IsSliceHeaderEB(p_slice_header) || IsSliceHeaderSP(p_slice_header)) {
            // num_ref_idx_active_override_flag u(1)
            p_slice_header->num_ref_idx_active_override_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            if (p_slice_header->num_ref_idx_active_override_flag) {
                // num_ref_idx_l0_active_minus1 ue(v)
                p_slice_header->num_ref_idx_l0_active_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                if (IsSliceHeaderB(p_slice_header)) {
                    // num_ref_idx_l1_active_minus1 ue(v)
                    p_slice_header->num_ref_idx_l1_active_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                }
            }
            else {
                p_slice_header->num_ref_idx_l0_active_minus1 = pc_pps->num_ref_idx_l0_default_active_minus1;
                if (IsSliceHeaderB(p_slice_header)) {
                    p_slice_header->num_ref_idx_l1_active_minus1 = pc_pps->num_ref_idx_l1_default_active_minus1;
                }
            }
            // The range of num_ref_idx_l0_active_minus1 is specified as follows.
            // 	– If field_pic_flag is equal to 0, num_ref_idx_l0_active_minus1 shall be in the range of 0 to 15, inclusive. When
            // 		MbaffFrameFlag is equal to 1, num_ref_idx_l0_active_minus1 is the maximum index value for the decoding of
            // 		frame macroblocks and 2 * num_ref_idx_l0_active_minus1 + 1 is the maximum index value for the decoding of
            // 		field macroblocks.
            // 	– Otherwise (field_pic_flag is equal to 1), num_ref_idx_l0_active_minus1 shall be in the range of 0 to 31, inclusive.
        }

        if (!p_slice_header->SVCExtFlag && p_codec->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION) {
            // ref_pic_list_mvc_modification( ) /* specified in Annex H */
            err = HL_ERROR_INVALID_BITSTREAM;
            HL_DEBUG_ERROR("MVC ext. not supported");
            goto bail;
        }
        else {
            // ref_pic_list_modification( )
            err = hl_codec_264_rbsp_avc_ref_pic_list_modification_read(p_slice_header, p_codec->pobj_bits);
            if (err) {
                goto bail;
            }
        }

        if ((pc_pps->weighted_pred_flag && (IsSliceHeaderP(p_slice_header) || IsSliceHeaderSP(p_slice_header))) ||
                (pc_pps->weighted_bipred_idc == 1 && IsSliceHeaderB(p_slice_header))) {
            int32_t base_pred_weight_table_flag;
            if (!p_slice_header->ext.svc.NoInterLayerPredFlag) {
                // base_pred_weight_table_flag u(1)
                base_pred_weight_table_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            }
            if (p_slice_header->ext.svc.NoInterLayerPredFlag || !base_pred_weight_table_flag) {
                // pred_weight_table()
                err = hl_codec_264_rbsp_avc_pred_weight_table_read(p_slice_header, p_codec->pobj_bits);
                if (err) {
                    goto bail;
                }
            }
        }

        if (p_codec->nal_current.i_nal_ref_idc) {
            // dec_ref_pic_marking( )
            err = hl_codec_264_rbsp_avc_dec_ref_pic_marking_read(p_slice_header, p_codec->pobj_bits);
            if (err) {
                goto bail;
            }

            if (p_slice_header->SVCExtFlag) {
                if(pc_pps->pc_sps->p_svc && !pc_pps->pc_sps->p_svc->slice_header_restriction_flag) {
                    // store_ref_base_pic_flag u(1)
                    int32_t store_ref_base_pic_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    if ((p_slice_header->ext.svc.UseRefBasePicFlag || store_ref_base_pic_flag) && !p_slice_header->IdrFlag) {
                        // dec_ref_base_pic_marking( )
                        err = hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_read(p_codec->pobj_bits, &p_slice_header->ext.svc.xs_dec_ref_base_pic_marking);
                        if (err) {
                            goto bail;
                        }
                    }
                }
            }

        }
    } // end-of- quality==0

    if(pc_pps->entropy_coding_mode_flag && !IsSliceHeaderI(p_slice_header)) {
        // cabac_init_idc ue(v)
        p_slice_header->cabac_init_idc = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
    }
    // slice_qp_delta se(v)
    p_slice_header->slice_qp_delta = hl_codec_264_bits_read_se(p_codec->pobj_bits);

    if (IsSliceHeaderSP(p_slice_header) || IsSliceHeaderSI(p_slice_header)) {
        if (IsSliceHeaderSP(p_slice_header)) {
            // sp_for_switch_flag u(1)
            p_slice_header->sp_for_switch_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
        }
        // slice_qs_delta se(v)
        p_slice_header->slice_qs_delta = hl_codec_264_bits_read_se(p_codec->pobj_bits);
    }

    if (pc_pps->deblocking_filter_control_present_flag) {
        // disable_deblocking_filter_idc ue(v)
        p_slice_header->disable_deblocking_filter_idc = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
        if(p_slice_header->disable_deblocking_filter_idc != 1) {
            // slice_alpha_c0_offset_div2 se(v)
            p_slice_header->slice_alpha_c0_offset_div2 = hl_codec_264_bits_read_se(p_codec->pobj_bits);
            // slice_beta_offset_div2 se(v)
            p_slice_header->slice_beta_offset_div2 = hl_codec_264_bits_read_se(p_codec->pobj_bits);
        }
    }

    if (pc_pps->num_slice_groups_minus1 > 0 && pc_pps->slice_group_map_type >= 3 && pc_pps->slice_group_map_type <= 5) {
        uint32_t uFrameSizeInMbs = pc_pps->pc_sps->uFrameHeightInMbs * pc_pps->pc_sps->uFrameWidthInMbs;
        u_tmp = uFrameSizeInMbs / (pc_pps->slice_group_change_rate_minus1 + 1);
        if ((uFrameSizeInMbs) % (pc_pps->slice_group_change_rate_minus1+1)) {
            ++u_tmp;
        }
        u_tmp = (uint32_t)HL_MATH_CEIL(HL_MATH_LOG2(u_tmp + 1));
        p_slice_header->slice_group_change_cycle = hl_codec_264_bits_read_u(p_codec->pobj_bits, u_tmp);
    }

    // Check validity
    if ((p_slice_header->SVCExtFlag && p_slice_header->disable_deblocking_filter_idc > 6)
            || (!p_slice_header->SVCExtFlag && p_slice_header->disable_deblocking_filter_idc > 2)
            || (p_slice_header->slice_alpha_c0_offset_div2 < -6 || p_slice_header->slice_alpha_c0_offset_div2 > 6)
            || (p_slice_header->slice_beta_offset_div2 < -6 || p_slice_header->slice_beta_offset_div2 > 6)
       ) {
        HL_DEBUG_ERROR("Invalid bitstream");
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }

    if (p_slice_header->SVCExtFlag) {
        if (!p_slice_header->ext.svc.NoInterLayerPredFlag && p_slice_header->ext.svc.QualityId == 0) {
            // ref_layer_dq_id ue(v)
            p_slice_header->ext.svc.ref_layer_dq_id = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
            if (pc_pps->pc_sps->p_svc && pc_pps->pc_sps->p_svc->inter_layer_deblocking_filter_control_present_flag) {
                // SVCExtDisableInterLayerDeblockingFilterIdc 2 ue(v)
                p_slice_header->ext.svc.disable_inter_layer_deblocking_filter_idc = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                if(p_slice_header->ext.svc.disable_inter_layer_deblocking_filter_idc != 1) {
                    // inter_layer_slice_alpha_c0_offset_div2 2 se(v)
                    p_slice_header->ext.svc.inter_layer_slice_alpha_c0_offset_div2 = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                    // inter_layer_slice_beta_offset_div2 2 se(v)
                    p_slice_header->ext.svc.inter_layer_slice_beta_offset_div2 = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                }
            }
            // constrained_intra_resampling_flag 2 u(1)
            p_slice_header->ext.svc.constrained_intra_resampling_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);

            if (pc_pps->pc_sps->p_svc && pc_pps->pc_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT) {
                if(pc_pps->pc_sps->ChromaArrayType > 0) {
                    u_tmp = hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                    // ref_layer_chroma_phase_x_plus1_flag 2 u(1)
                    p_slice_header->ext.svc.ref_layer_chroma_phase_x_plus1_flag = (u_tmp >> 2);
                    // ref_layer_chroma_phase_y_plus1 2 u(2)
                    p_slice_header->ext.svc.ref_layer_chroma_phase_y_plus1 = (u_tmp & 0x03);
                }
                // scaled_ref_layer_left_offset 2 se(v)
                p_slice_header->ext.svc.scaled_ref_layer_left_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                // scaled_ref_layer_top_offset 2 se(v)
                p_slice_header->ext.svc.scaled_ref_layer_top_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                // scaled_ref_layer_right_offset 2 se(v)
                p_slice_header->ext.svc.scaled_ref_layer_right_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
                // scaled_ref_layer_bottom_offset 2 se(v)
                p_slice_header->ext.svc.scaled_ref_layer_bottom_offset = hl_codec_264_bits_read_se(p_codec->pobj_bits);
            }
        }

        if (!p_slice_header->ext.svc.NoInterLayerPredFlag) {
            // slice_skip_flag u(1)
            p_slice_header->ext.svc.slice_skip_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            if (p_slice_header->ext.svc.slice_skip_flag) {
                // num_mbs_in_slice_minus1 ue(v)
                p_slice_header->ext.svc.num_mbs_in_slice_minus1 = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
            }
            else {
                // adaptive_base_mode_flag u(1)
                p_slice_header->ext.svc.adaptive_base_mode_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                if (!p_slice_header->ext.svc.adaptive_base_mode_flag) {
                    // default_base_mode_flag u(1)
                    p_slice_header->ext.svc.default_base_mode_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                }
                if (!p_slice_header->ext.svc.default_base_mode_flag) {
                    // adaptive_motion_prediction_flag u(1)
                    p_slice_header->ext.svc.adaptive_motion_prediction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    if(!p_slice_header->ext.svc.adaptive_motion_prediction_flag) {
                        // default_motion_prediction_flag u(1)
                        p_slice_header->ext.svc.default_motion_prediction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    }
                }
                // adaptive_residual_prediction_flag u(1)
                p_slice_header->ext.svc.adaptive_residual_prediction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                if (!p_slice_header->ext.svc.adaptive_residual_prediction_flag) {
                    // default_residual_prediction_flag u(1)
                    p_slice_header->ext.svc.default_residual_prediction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                }
            }

            if (pc_pps->pc_sps->p_svc && pc_pps->pc_sps->p_svc->adaptive_tcoeff_level_prediction_flag) {
                // tcoeff_level_prediction_flag u(1)
                p_slice_header->ext.svc.tcoeff_level_prediction_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);
            }
        }

        if (pc_pps->pc_sps->p_svc && !pc_pps->pc_sps->p_svc->slice_header_restriction_flag && !p_slice_header->ext.svc.slice_skip_flag) {
            u_tmp = hl_codec_264_bits_read_u(p_codec->pobj_bits, 8);
            // scan_idx_start 2 u(4)
            p_slice_header->ext.svc.scan_idx_start = (u_tmp >> 4);
            // scan_idx_end 2 u(4)
            p_slice_header->ext.svc.scan_idx_end = (u_tmp & 0x0F);
        }
        else {
            p_slice_header->ext.svc.scan_idx_start = 0;
            p_slice_header->ext.svc.scan_idx_end = 15;
        }

    }// end-of 'p_slice_header->SVCExtFlag'

    // derive()
    err = _hl_codec_264_nal_slice_header_derive(p_slice_header, p_codec);

bail:
    return err;
}

HL_ERROR_T hl_codec_264_nal_slice_header_encode(hl_codec_264_t* p_codec, hl_size_t u_idx, hl_bool_t b_derive, hl_bool_t b_write)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const hl_codec_264_nal_pps_t *pc_pps;
    const hl_codec_264_nal_sps_t *pc_sps;
    hl_codec_264_encode_slice_data_t *pc_esd;
    hl_codec_264_nal_slice_header_t *pc_slice_header;
    int32_t i_rc_bits_count;

    if ((u_idx >= HL_CODEC_264_SLICES_MAX_COUNT) || !(pc_esd = p_codec->layers.pc_active->encoder.p_list_esd[u_idx]) || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    // Rate Control
    if (p_codec->encoder.rc.b_enabled && b_write) { // If(!b_write) means we're trying to initialize a fake slice header.
        if (pc_slice_header->first_mb_in_slice == 0) { // First slice?
            err = hl_codec_264_rc_pict_start(p_codec);
            if (err) {
                return err;
            }
        }
        i_rc_bits_count = (int32_t)hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits);
    }

    pc_slice_header->SVCExtFlag = p_codec->nal_current.svc_extension_flag;
    if (pc_slice_header->SVCExtFlag) {
        pc_slice_header->IdrFlag = p_codec->nal_current.ext.svc.idr_flag;
        pc_slice_header->ext.svc.PriorityId = p_codec->nal_current.ext.svc.priority_id;
        pc_slice_header->ext.svc.NoInterLayerPredFlag = p_codec->nal_current.ext.svc.no_inter_layer_pred_flag;
        pc_slice_header->ext.svc.DependencyId = p_codec->nal_current.ext.svc.dependency_id;
        pc_slice_header->ext.svc.QualityId = p_codec->nal_current.ext.svc.quality_id;
        pc_slice_header->ext.svc.TemporalId = p_codec->nal_current.ext.svc.temporal_id;
        pc_slice_header->ext.svc.UseRefBasePicFlag = p_codec->nal_current.ext.svc.use_ref_base_pic_flag;
        pc_slice_header->ext.svc.DiscardableFlag = p_codec->nal_current.ext.svc.discardable_flag;
        pc_slice_header->ext.svc.OutputFlag = p_codec->nal_current.ext.svc.output_flag;

        pc_slice_header->ext.svc.ref_layer_dq_id = ((pc_slice_header->ext.svc.DependencyId - 1) << 4) + 0/*quality_id*/;
        pc_slice_header->disable_deblocking_filter_idc = p_codec->pc_base->deblock_flag ? 0 : 1; // deblocking type ([0-6] SVC, 1 = disabled, 0 = all)
        pc_slice_header->ext.svc.disable_inter_layer_deblocking_filter_idc = p_codec->pc_base->deblock_inter_layer_flag ? 0 : 1; // deblocking type ([0-6] SVC, 1 = disabled, 0 = all)
    }
    else {
        pc_slice_header->IdrFlag = IdrPicFlag(p_codec->nal_current.e_nal_type);
        pc_slice_header->disable_deblocking_filter_idc = p_codec->pc_base->deblock_flag ? 0 : 1; // deblocking type ([0-2] AVC, 0 = all, 1 = )
    }

    if (b_derive) {
        err = _hl_codec_264_nal_slice_header_derive(pc_slice_header, p_codec);
        if (err) {
            return err;
        }
    }

    // if (!b_write) == fake slice header
    if (!b_write) {
        return err;
    }

    if (HL_CODEC_264_NAL(pc_slice_header)->e_type != HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION) { // NALU header already writen in "h264_encode()"
        // forbidden_zero_bit f(1)
        hl_codec_264_bits_write_f1(pc_esd->pobj_bits, HL_CODEC_264_NAL(pc_slice_header)->u_forbidden_zero_bit);
        // ref_idc u(2)
        hl_codec_264_bits_write_u(pc_esd->pobj_bits, HL_CODEC_264_NAL(pc_slice_header)->u_ref_idc, 2);
        // nal_unit_tye u(5)
        hl_codec_264_bits_write_u(pc_esd->pobj_bits, HL_CODEC_264_NAL(pc_slice_header)->e_type, 5);
    }

    // first_mb_in_slice ue(v)
    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->first_mb_in_slice);
    // slice_type ue(v)
    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->slice_type);
    // pic_parameter_set_id ue(v)
    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->pic_parameter_set_id);

    if (pc_sps->separate_colour_plane_flag) {
        // colour_plane_id u(2)
        hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->colour_plane_id, 2);
    }
    // frame_num u(v)
    hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->frame_num, pc_sps->log2_max_frame_num_minus4 + 4);

    if (!pc_sps->frame_mbs_only_flag) {
        // field_pic_flag u(1)
        hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->field_pic_flag);
        if (pc_slice_header->field_pic_flag) {
            // bottom_field_flag u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->bottom_field_flag);
        }
    }

    if (pc_slice_header->IdrFlag) {
        // idr_pic_id ue(v)
        hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->idr_pic_id);
    }

    if (pc_sps->pic_order_cnt_type == 0) {
        // pic_order_cnt_lsb u(v)
        hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->pic_order_cnt_lsb, pc_sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pc_pps->bottom_field_pic_order_in_frame_present_flag && !pc_slice_header->field_pic_flag) {
            // delta_pic_order_cnt_bottom se(v)
            hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->delta_pic_order_cnt_bottom);
        }
    }

    if (pc_sps->pic_order_cnt_type == 1 && !pc_sps->delta_pic_order_always_zero_flag) {
        // delta_pic_order_cnt[ 0 ] se(v)
        hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->delta_pic_order_cnt[0]);
        if (pc_pps->bottom_field_pic_order_in_frame_present_flag && !pc_slice_header->field_pic_flag) {
            // delta_pic_order_cnt[ 1 ] se(v)
            hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->delta_pic_order_cnt[1]);
        }
    }

    if (pc_pps->redundant_pic_cnt_present_flag) {
        // redundant_pic_cnt ue(v)
        hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->redundant_pic_cnt);
    }

    if (pc_slice_header->ext.svc.QualityId == 0) {
        if (IsSliceHeaderEB(pc_slice_header)) {
            // direct_spatial_mv_pred_flag u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->direct_spatial_mv_pred_flag);
        }

        if (IsSliceHeaderEP(pc_slice_header) || IsSliceHeaderEB(pc_slice_header) || IsSliceHeaderSP(pc_slice_header)) {
            // num_ref_idx_active_override_flag u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->num_ref_idx_active_override_flag);
            if (pc_slice_header->num_ref_idx_active_override_flag) {
                // num_ref_idx_l0_active_minus1 ue(v)
                hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->num_ref_idx_l0_active_minus1);
                if(IsSliceHeaderB(pc_slice_header)) {
                    // num_ref_idx_l1_active_minus1 ue(v)
                    hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->num_ref_idx_l1_active_minus1);
                }
            }
            else {
                pc_slice_header->num_ref_idx_l0_active_minus1 = pc_pps->num_ref_idx_l0_default_active_minus1;
                if (IsSliceHeaderB(pc_slice_header)) {
                    pc_slice_header->num_ref_idx_l1_active_minus1 = pc_pps->num_ref_idx_l1_default_active_minus1;
                }
            }
            // The range of num_ref_idx_l0_active_minus1 is specified as follows.
            // 	– If field_pic_flag is equal to 0, num_ref_idx_l0_active_minus1 shall be in the range of 0 to 15, inclusive. When
            // 		MbaffFrameFlag is equal to 1, num_ref_idx_l0_active_minus1 is the maximum index value for the decoding of
            // 		frame macroblocks and 2 * num_ref_idx_l0_active_minus1 + 1 is the maximum index value for the decoding of
            // 		field macroblocks.
            // 	– Otherwise (field_pic_flag is equal to 1), num_ref_idx_l0_active_minus1 shall be in the range of 0 to 31, inclusive.
        }

        if (!pc_slice_header->SVCExtFlag && p_codec->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION) {
            // ref_pic_list_mvc_modification( ) /* specified in Annex H */
            HL_DEBUG_ERROR("MVC ext. not supported");
            return HL_ERROR_INVALID_BITSTREAM;
        }
        else {
            // ref_pic_list_modification( ) - AVC or SVC
            err = hl_codec_264_rbsp_avc_ref_pic_list_modification_write(pc_slice_header, pc_esd->pobj_bits);
            if (err) {
                return err;
            }
        }


        if ((pc_pps->weighted_pred_flag && (IsSliceHeaderP(pc_slice_header) || IsSliceHeaderSP(pc_slice_header))) ||
                (pc_pps->weighted_bipred_idc == 1 && IsSliceHeaderB(pc_slice_header))) {
            HL_DEBUG_ERROR("Not implemented yet");
            return HL_ERROR_NOT_IMPLEMENTED;
#if 0
            // pred_weight_table()
            if((ret = hlRbsp_WriteWeightTable(pc_slice_header, pc_pps, pc_sps, pc_esd->pobj_bits)) != hlError_Success) {
                HL_DEBUG_ERROR("hlRbsp_WriteWeightTable() failed");
                return ret;
            }
#endif
        }

        if (p_codec->nal_current.i_nal_ref_idc) {
            // dec_ref_pic_marking( )
            err = hl_codec_264_rbsp_avc_dec_ref_pic_marking_write(pc_slice_header, pc_esd->pobj_bits);
            if (err) {
                HL_DEBUG_ERROR("hl_codec_264_rbsp_avc_dec_ref_pic_marking_write() failed");
                return err;
            }
            if (pc_slice_header->SVCExtFlag) {
                if (pc_pps->pc_sps->p_svc && !pc_pps->pc_sps->p_svc->slice_header_restriction_flag) {
                    static const int32_t store_ref_base_pic_flag = 0; // TODO: allow changing at runtime
                    // store_ref_base_pic_flag u(1)
                    hl_codec_264_bits_write_u1(pc_esd->pobj_bits, store_ref_base_pic_flag);
                    if ((pc_slice_header->ext.svc.UseRefBasePicFlag || store_ref_base_pic_flag) && !pc_slice_header->IdrFlag) {
                        // dec_ref_base_pic_marking( )
                        err = hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_write(pc_esd->pobj_bits, &pc_slice_header->ext.svc.xs_dec_ref_base_pic_marking);
                        if (err) {
                            return err;
                        }
                    }
                }
            }
        }
    } // end-of- quality==0

    if (pc_pps->entropy_coding_mode_flag && !IsSliceHeaderI(pc_slice_header) && !IsSliceHeaderSI(pc_slice_header)) {
        // cabac_init_idc ue(v)
        hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->cabac_init_idc);
    }
    // slice_qp_delta se(v)
    hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->slice_qp_delta);

    if (IsSliceHeaderSP(pc_slice_header) || IsSliceHeaderSI(pc_slice_header)) {
        if (IsSliceHeaderSP(pc_slice_header)) {
            // sp_for_switch_flag u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->sp_for_switch_flag);
        }
        // slice_qs_delta se(v)
        hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->slice_qs_delta);
    }

    if (pc_pps->deblocking_filter_control_present_flag) {
        // disable_deblocking_filter_idc ue(v)
        hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->disable_deblocking_filter_idc);
        if (pc_slice_header->disable_deblocking_filter_idc != 1) {
            // slice_alpha_c0_offset_div2 se(v)
            hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->slice_alpha_c0_offset_div2);
            // slice_beta_offset_div2 se(v)
            hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->slice_beta_offset_div2);
        }
    }

    if (pc_pps->num_slice_groups_minus1 > 0 && pc_pps->slice_group_map_type >= 3 && pc_pps->slice_group_map_type <= 5) {
        uint32_t tmp = (pc_sps->pic_height_in_map_units_minus1 + 1) * (pc_sps->pic_width_in_mbs_minus1 + 1)/(pc_pps->slice_group_change_rate_minus1 + 1);
        if(((pc_sps->pic_height_in_map_units_minus1+1)*(pc_sps->pic_width_in_mbs_minus1+1))%(pc_pps->slice_group_change_rate_minus1+1)) {
            ++tmp;
        }
        tmp = (uint32_t)HL_MATH_CEIL(HL_MATH_LOG2(tmp + 1));
        hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->slice_group_change_cycle, tmp);
    }

    // Check validity
    if ((pc_slice_header->SVCExtFlag && pc_slice_header->disable_deblocking_filter_idc > 6)
            || (!pc_slice_header->SVCExtFlag && pc_slice_header->disable_deblocking_filter_idc > 2)
            || (pc_slice_header->slice_alpha_c0_offset_div2 < -6 || pc_slice_header->slice_alpha_c0_offset_div2 > 6)
            || (pc_slice_header->slice_beta_offset_div2 < -6 || pc_slice_header->slice_beta_offset_div2 > 6)
       ) {
        HL_DEBUG_ERROR("Invalid bitstream");
        return HL_ERROR_INVALID_BITSTREAM;
    }

    if (pc_slice_header->SVCExtFlag) {
        if (!pc_slice_header->ext.svc.NoInterLayerPredFlag && pc_slice_header->ext.svc.QualityId == 0) {
            // ref_layer_dq_id ue(v)
            hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->ext.svc.ref_layer_dq_id);
            if (pc_pps->pc_sps->p_svc && pc_pps->pc_sps->p_svc->inter_layer_deblocking_filter_control_present_flag) {
                // SVCExtDisableInterLayerDeblockingFilterIdc 2 ue(v)
                hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->ext.svc.disable_inter_layer_deblocking_filter_idc);
                if(pc_slice_header->ext.svc.disable_inter_layer_deblocking_filter_idc != 1) {
                    // inter_layer_slice_alpha_c0_offset_div2 2 se(v)
                    hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->ext.svc.inter_layer_slice_alpha_c0_offset_div2);
                    // inter_layer_slice_beta_offset_div2 2 se(v)
                    hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->ext.svc.inter_layer_slice_beta_offset_div2);
                }
            }
            // constrained_intra_resampling_flag 2 u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.constrained_intra_resampling_flag);

            if (pc_pps->pc_sps->p_svc && pc_pps->pc_sps->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT) {
                if(pc_pps->pc_sps->ChromaArrayType > 0) {
                    // ref_layer_chroma_phase_x_plus1_flag 2 u(1)
                    hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.ref_layer_chroma_phase_x_plus1_flag);
                    // ref_layer_chroma_phase_y_plus1 2 u(2)
                    hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->ext.svc.ref_layer_chroma_phase_y_plus1, 2);
                }
                // scaled_ref_layer_left_offset 2 se(v)
                hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->ext.svc.scaled_ref_layer_left_offset);
                // scaled_ref_layer_top_offset 2 se(v)
                hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->ext.svc.scaled_ref_layer_top_offset);
                // scaled_ref_layer_right_offset 2 se(v)
                hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->ext.svc.scaled_ref_layer_right_offset);
                // scaled_ref_layer_bottom_offset 2 se(v)
                hl_codec_264_bits_write_se(pc_esd->pobj_bits, pc_slice_header->ext.svc.scaled_ref_layer_bottom_offset);
            }
        }

        if (!pc_slice_header->ext.svc.NoInterLayerPredFlag) {
            // slice_skip_flag u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.slice_skip_flag);
            if (pc_slice_header->ext.svc.slice_skip_flag) {
                // num_mbs_in_slice_minus1 ue(v)
                hl_codec_264_bits_write_ue(pc_esd->pobj_bits, pc_slice_header->ext.svc.num_mbs_in_slice_minus1);
            }
            else {
                // adaptive_base_mode_flag u(1)
                hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.adaptive_base_mode_flag);
                if (!pc_slice_header->ext.svc.adaptive_base_mode_flag) {
                    // default_base_mode_flag u(1)
                    hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.default_base_mode_flag);
                }
                if (!pc_slice_header->ext.svc.default_base_mode_flag) {
                    // adaptive_motion_prediction_flag u(1)
                    hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.adaptive_motion_prediction_flag);
                    if(!pc_slice_header->ext.svc.adaptive_motion_prediction_flag) {
                        // default_motion_prediction_flag u(1)
                        hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.default_motion_prediction_flag);
                    }
                }
                // adaptive_residual_prediction_flag u(1)
                hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.adaptive_residual_prediction_flag);
                if (!pc_slice_header->ext.svc.adaptive_residual_prediction_flag) {
                    // default_residual_prediction_flag u(1)
                    hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.default_residual_prediction_flag);
                }
            }

            if (pc_pps->pc_sps->p_svc && pc_pps->pc_sps->p_svc->adaptive_tcoeff_level_prediction_flag) {
                // tcoeff_level_prediction_flag u(1)
                hl_codec_264_bits_write_u1(pc_esd->pobj_bits, pc_slice_header->ext.svc.tcoeff_level_prediction_flag);
            }
        }

        if (pc_pps->pc_sps->p_svc && !pc_pps->pc_sps->p_svc->slice_header_restriction_flag && !pc_slice_header->ext.svc.slice_skip_flag) {
            // scan_idx_start 2 u(4)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->ext.svc.scan_idx_start, 4);
            // scan_idx_end 2 u(4)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, pc_slice_header->ext.svc.scan_idx_end, 4);
        }

    }// end-of 'pc_slice_header->SVCExtFlag'

    // Rate Control
    if (p_codec->encoder.rc.b_enabled) {
        // p_codec->encoder.rc.i_pict_bits_hdr += (int32_t);

        int32_t len = (int32_t)(hl_codec_264_bits_get_stream_index(pc_esd->pobj_bits) - i_rc_bits_count);
        err = hl_codec_264_rc_store_slice_header_bits( p_codec, len );
        if (err) {
            return err;
        }
    }

    return err;
}


// G.7.3.4.1 Slice data in scalable extension syntax
// 7.3.4 Slice data syntax
// 7.4.4 Slice data semantics
// 9.3 CABAC parsing process for slice data
// slice_data_in_scalable_extension( )
// slice_data()
HL_ERROR_T hl_codec_264_nal_slice_data_decode(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    uint32_t u_mb_addr, u_slice_group_id, u_prev_mb_skipped, u_mb_skip_run;
    uint32_t u_scan_idx_start = 0, u_scan_idx_end = 15;
    hl_codec_264_layer_t *pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_slice_t *pc_slice = pc_layer->pc_slice_curr;
    const hl_codec_264_nal_slice_header_t *pc_slice_header = pc_slice->p_header;
    const hl_codec_264_nal_pps_t *pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t *pc_sps = pc_pps->pc_sps;
    hl_codec_264_mb_t* p_mb;
    register uint32_t i;
    hl_bool_t b_more_data_flag, b_quant_values_computed;
    static const hl_bool_t bPSkipTrue = HL_TRUE;
    static const hl_bool_t bPSkipFalse = HL_FALSE;

#define NextMbAddressAndCreate() \
	NextMbAddressWithoutCreate() \
	if (u_mb_addr < p_codec->layers.pc_active->u_list_macroblocks_count && !(p_mb = pc_layer->pp_list_macroblocks[u_mb_addr])) { \
        err = hl_codec_264_mb_create(u_mb_addr, &p_mb); \
        if (err) { \
            goto bail; \
        } \
        pc_layer->pp_list_macroblocks[u_mb_addr] = p_mb; \
		p_mb->l_slice_id = pc_slice_header->l_id; \
		p_mb->u_slice_idx = pc_slice->u_idx; \
    } \
	/* End-Of MoveToNextMB */

#define NextMbAddressWithoutCreate() \
	u_slice_group_id = pc_slice_header->MbToSliceGroupMap[u_mb_addr]; \
	NextMbAddress(u_mb_addr, u_slice_group_id, pc_slice_header); \
	/* End-Of NextMbAddress2 */

    if (pc_pps->entropy_coding_mode_flag) {
        // while( !byte_aligned( ) )
        //	cabac_alignment_one_bit f(1)
        hl_codec_264_bits_align(p_codec->pobj_bits);

        // CABAC is supported by Hartallo 1.0 -> get the code from there.
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // u_mb_addr = first_mb_in_slice * ( 1 + MbaffFrameFlag )
    u_mb_addr = pc_slice_header->first_mb_in_slice << pc_slice_header->MbaffFrameFlag;
    b_more_data_flag = HL_TRUE;
    u_prev_mb_skipped = 0;

    HL_DEBUG_INFO("Decode Frame=%d, DQId=%d, u_mb_addr_start=%u", pc_layer->i_pict_decode_count, pc_layer->DQId, u_mb_addr);

    do {// Get slice group id

        // FIXME:
        //if (pc_layer->DQId == 0 && u_mb_addr == 0 && pc_layer->i_pict_decode_count >= 0) { // FIXME
        //    int a = 0;
        //}


        // guard to make sure Quant values will be updated only if "mb_qp_delta" is different than zero.
        b_quant_values_computed = HL_FALSE;

        // Scalable Video Coding (SVC)
        if (pc_slice_header->SVCExtFlag) {
            u_scan_idx_start = pc_slice_header->ext.svc.scan_idx_start;
            u_scan_idx_end = pc_slice_header->ext.svc.scan_idx_end;
        }

        // Create current macroblock
        if (!(p_mb = pc_layer->pp_list_macroblocks[u_mb_addr])) {
            err = hl_codec_264_mb_create(u_mb_addr, &p_mb);
            if (err) {
                goto bail;
            }
            pc_layer->pp_list_macroblocks[u_mb_addr] = p_mb;
        }
        p_mb->l_slice_id = pc_slice_header->l_id;
        p_mb->u_slice_idx = pc_slice->u_idx;

        if (!IsSliceHeaderI(pc_slice_header) && !IsSliceHeaderSI(pc_slice_header)) {
            if (pc_pps->entropy_coding_mode_flag) {
                // FIXME:CABAC is supported by Hartallo 1.0 -> get the code from there.
                HL_DEBUG_ERROR("Not implemented");
                return HL_ERROR_NOT_IMPLEMENTED;
            }
            else if (!u_prev_mb_skipped) {
                // mb_skip_run ue(v)
                u_mb_skip_run = hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                u_prev_mb_skipped = (u_mb_skip_run > 0);
                for (i = 0; i< u_mb_skip_run && u_mb_addr < pc_slice_header->PicSizeInMbs; ++i) {
                    if (i) { /* First P-Skip macroblock? */
                        // request new macroblock for next P-Skip MB
                        NextMbAddressAndCreate();
                    }

                    // Init Neighbouring before decoding
                    err = hl_codec_264_utils_init_mb_current_avc(p_codec, u_mb_addr, pc_slice_header->l_id, bPSkipTrue);
                    if (err) {
                        goto bail;
                    }
                    // Update Quant values
                    err = hl_codec_264_mb_set_default_quant_values(p_mb, p_codec);
                    if (err) {
                        goto bail;
                    }
                    // 8.4 Inter prediction process
                    //err = hl_codec_264_pred_inter_decode(p_codec, p_mb);
                    //if (err) {
                    //    goto bail;
                    //}
                    // Deblock macroblock
                    //--err = hl264Deblock_MB(ctx, CurrMb);
                    if (err) {
                        goto bail;
                    }
                }
                if (u_mb_skip_run > 0) {
                    b_more_data_flag = hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits);
                    pc_layer->i_mb_read_count += u_mb_skip_run;
                    if (u_mb_addr >= pc_slice_header->PicSizeInMbs) {
                        goto Next;
                    }
                    // u_mb_addr = NextMbAddress(u_mb_addr);
                    NextMbAddressWithoutCreate();
                    continue;
                }
            }
        }

        u_prev_mb_skipped = 0;

        if (b_more_data_flag) {
            int32_t mbPartIdx;
            uint32_t noSubMbPartSizeLessThan8x8Flag;
            static int _transform_size_8x8_flag = 0; // FIXME: FRExt not supported yet

            // Derivation process of the availability for macroblock addresses
            // not part of macroblock_layer()
            err = hl_codec_264_utils_init_mb_current_avc(p_codec, u_mb_addr, pc_slice_header->l_id, bPSkipFalse);
            if (err) {
                goto bail;
            }
            // Update Quant values
            err = hl_codec_264_mb_set_default_quant_values(p_mb, p_codec);
            if (err) {
                goto bail;
            }

            if (pc_slice_header->MbaffFrameFlag && ((u_mb_addr & 1) == 0 || ((u_mb_addr & 1) == 1 && u_prev_mb_skipped))) {
                // mb_field_decoding_flag u(1) | ae(v)
                p_mb->mb_field_decoding_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);//FIXME: ae(v) for CABAC
            }

            // 7.3.5 Macroblock layer syntax
            //== begin of macroblock_layer( )
            if (pc_slice_header->SVCExtFlag) {
                int32_t base_layer_available = !pc_slice_header->ext.svc.NoInterLayerPredFlag;
                { /* G.7.4.6 Macroblock layer in scalable extension semantics */
                    int32_t mbX, mbY0, mbY1, scalMbH;
                    mbX = p_mb->u_x;
                    mbY0 = p_mb->u_y;
                    mbY1 = pc_slice_header->MbaffFrameFlag ? (mbY0 + 1) : mbY0;
                    scalMbH = ((1 + pc_slice_header->field_pic_flag) << 4);

                    // FIXME= 'scalMbH' is power of 2 and less than or equal to 32 -> replace '/ scalMbH' per '>> log2[scalMbH]'
                    p_mb->ext.svc.InCropWindow = (!pc_slice_header->ext.svc.NoInterLayerPredFlag &&
                                                  (mbX >= ((pc_slice_header->ext.svc.ScaledRefLayerLeftOffset + 15 ) >> 4)) &&
                                                  (mbX < ((pc_slice_header->ext.svc.ScaledRefLayerLeftOffset + pc_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL) >> 4)) &&
                                                  (mbY0 >= ((pc_slice_header->ext.svc.ScaledRefLayerTopOffset + scalMbH - 1) / scalMbH)) &&
                                                  (mbY1 < ((pc_slice_header->ext.svc.ScaledRefLayerTopOffset + pc_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL) / scalMbH)));
                }

                if (base_layer_available) {
                    if (p_mb->ext.svc.InCropWindow) {
                        if (pc_slice_header->ext.svc.adaptive_base_mode_flag) {
                            p_mb->ext.svc.base_mode_flag = pc_pps->entropy_coding_mode_flag
                                                           ? /* FIXME:CABAC hl264CABAC_Read_base_mod_flag(ctx, CurrMb) */ 0
                                                           : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        }
                        else {
                            p_mb->ext.svc.base_mode_flag = pc_slice_header->ext.svc.default_base_mode_flag;
                        }
                    }
                    else {
                        p_mb->ext.svc.base_mode_flag = 0;
                    }
                }
                else {
                    p_mb->ext.svc.base_mode_flag = 0;
                }
            }


            if (!p_mb->ext.svc.base_mode_flag) {
                if (pc_pps->entropy_coding_mode_flag) {
                    switch(pc_slice_header->SliceTypeModulo5) {
                    case HL_CODEC_264_SLICE_TYPE_I: {
                        // mb_type: ue(v) | ae(v)
                        /* FIXME:CABAC - p_mb->mb_type = hl264CABAC_Read_mb_type_I(ctx, CurrMb); */
                        break;
                    }
                    case HL_CODEC_264_SLICE_TYPE_P:
                    case HL_CODEC_264_SLICE_TYPE_SP: {
                        // mb_type: ue(v) | ae(v)
                        /* FIXME:CABAC p_mb->mb_type = hl264CABAC_Read_mb_type_PandSP(ctx, CurrMb); */
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
                    p_mb->mb_type = hl_codec_264_bits_read_ue(p_codec->pobj_bits); // FIXME: CABAC
                }
            }
            else {
                // p_mb->ext.svc.base_mode_flag==1 -> "mb_type" not present
                p_mb->mb_type = HL_CODEC_264_SVC_MB_TYPE_INFERRED;
            }

            // set "mb_type" (will defines PredType, PredMode, NumPart, NumSubParts...)
            err = hl_codec_264_mb_set_mb_type(p_mb, p_mb->mb_type, p_codec);
            if (err) {
                goto bail;
            }

            if (IsSliceHeaderI(pc_slice_header) && HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)) {
                uint32_t i,j,xL,yL,xC,yC;
                hl_pixel_t *cSL, *cSCb, *cSCr;
                // 6.4.1 Inverse macroblock scanning process
                hl_codec_264_utils_inverse_macroblock_scanning_process(p_codec, u_mb_addr, &xL, &yL);
                xC = xL >> p_codec->sps.pc_active->SubWidthC_TrailingZeros;
                yC = yL >> p_codec->sps.pc_active->SubHeightC_TrailingZeros;

                cSL = (hl_pixel_t *)pc_layer->pc_fs_curr->p_pict->pc_data_y + ((yL*pc_slice_header->PicWidthInSamplesL) + xL);
                cSCb = (hl_pixel_t *)pc_layer->pc_fs_curr->p_pict->pc_data_u + ((yC*pc_slice_header->PicWidthInSamplesC) + xC);
                cSCr = (hl_pixel_t *)pc_layer->pc_fs_curr->p_pict->pc_data_v + ((yC*pc_slice_header->PicWidthInSamplesC) + xC);

                // HL_DEBUG_INFO("mb_addr=%d, %u", p_mb->u_addr, (p_codec->pobj_bits->pc_current - p_codec->pobj_bits->pc_start));

                hl_codec_264_bits_align(p_codec->pobj_bits);

                for (i=0; i<16; ++i) {
                    for (j=0; j<16; j+=4) {
                        cSL[j]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthY);
                        cSL[j + 1]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthY);
                        cSL[j + 2]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthY);
                        cSL[j + 3]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthY);
                    }
                    cSL+=pc_slice_header->PicWidthInSamplesL;
                }
                for (i=0; i<pc_sps->MbHeightC; ++i) {
                    for (j=0; j<pc_sps->MbWidthC; j+=4) {
                        cSCb[j]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                        cSCb[j + 1]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                        cSCb[j + 2]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                        cSCb[j + 3]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                    }
                    cSCb+=pc_slice_header->PicWidthInSamplesC;
                }
                for (i=0; i<pc_sps->MbHeightC; ++i) {
                    for (j=0; j<pc_sps->MbWidthC; j+=4) {
                        cSCr[j]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                        cSCr[j + 1]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                        cSCr[j + 2]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                        cSCr[j + 3]=hl_codec_264_bits_read_u(p_codec->pobj_bits, pc_sps->BitDepthC);
                    }
                    cSCr+=pc_slice_header->PicWidthInSamplesC;
                }
                ++pc_layer->i_mb_decode_count;
            }
            else { // P I B
                if (!p_mb->ext.svc.base_mode_flag) {
                    noSubMbPartSizeLessThan8x8Flag = 1;
                    if (!HL_CODEC_264_MB_TYPE_IS_I_NxN(p_mb) && !HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0) && p_mb->NumMbPart == 4) {
                        // 7.3.5.2 Sub-macroblock prediction syntax
                        // G.7.3.6.2 Sub-macroblock prediction in scalable extension syntax
                        // sub_mb_pred( mb_type )
                        // sub_mb_pred_in_scalable_extension( mb_type )
                        int32_t subMbPartIdx,sub_mb_type,max_ref_idx0,max_ref_idx1;
                        //FIXME:CABAC hl264CABAC_Read_sub_mb_type_f hl264CABAC_Read_sub_mb_type = IsSliceHeaderB(pc_slice_header) ? hl264CABAC_Read_sub_mb_type_B : hl264CABAC_Read_sub_mb_type_PandSP;

                        max_ref_idx0 = pc_slice_header->num_ref_idx_l0_active_minus1;
                        max_ref_idx1 = pc_slice_header->num_ref_idx_l1_active_minus1;

                        // Read "sub_mb_type" values
                        for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                            sub_mb_type = pc_pps->entropy_coding_mode_flag
                                          ? 0/*FIXME:CABAC hl264CABAC_Read_sub_mb_type(ctx, CurrMb) */
                                          : hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                            p_mb->sub_mb_type[mbPartIdx] = sub_mb_type;
                        }

                        // Compute "sub_mb_type" (will define SubMbPredType, NumSubMbPart, SubMbPartWidth, SubMbPartHeight, noSubMbPartSizeLessThan8x8Flag, partWidth, partHeight...)
                        err = hl_codec_264_mb_set_sub_mb_type(p_mb, &p_mb->sub_mb_type, p_codec);
                        if (err) {
                            goto bail;
                        }

                        if (pc_slice_header->SVCExtFlag && p_mb->ext.svc.InCropWindow) {
                            if (pc_slice_header->ext.svc.adaptive_motion_prediction_flag) {
                                for (mbPartIdx = 0; mbPartIdx < 4; ++mbPartIdx) {
                                    if (p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_DIRECT && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L1) {
                                        // motion_prediction_flag_l0[ mbPartIdx ] 2 u(1) | ae(v)
                                        p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                ? 0/*FIXME:CABAC hl264CABAC_motion_prediction_flag_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx) */
                                                : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                                    }
                                }
                                for (mbPartIdx = 0; mbPartIdx < 4; ++mbPartIdx) {
                                    if (p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_DIRECT && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L0) {
                                        // motion_prediction_flag_l1[ mbPartIdx ] 2 u(1) | ae(v)
                                        p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                ? 0/*FIXME:CABAC hl264CABAC_motion_prediction_flag_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx) */
                                                : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                                    }
                                }
                            }
                            else {
                                // "motion_prediction_flag_lX" syntax NOT available
                                for (mbPartIdx=0; mbPartIdx < 4; ++mbPartIdx) {
                                    p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx] = pc_slice_header->ext.svc.default_motion_prediction_flag;
                                    p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx] = pc_slice_header->ext.svc.default_motion_prediction_flag;
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
                                p_mb->ref_idx_l0[mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                              ? 0/*FIXME:CABAC hl264CABAC_Read_ref_idx_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx) */
                                                              : hl_codec_264_bits_read_te(p_codec->pobj_bits, max_ref_idx0);
                            }
                            else {
                                p_mb->ref_idx_l0[mbPartIdx] = 0;
                            }
                        }
                        for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                            if ((pc_slice_header->num_ref_idx_l1_active_minus1 > 0 ||
                                    p_mb->mb_field_decoding_flag != pc_slice_header->field_pic_flag) &&
                                    p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 &&
                                    p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L0 &&
                                    (!pc_slice_header->SVCExtFlag || !p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx])) {
                                // ref_idx_l0 te(v) | ae(v)
                                p_mb->ref_idx_l1[mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                              ? 0/*FIXME:CABAC hl264CABAC_Read_ref_idx_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx) */
                                                              : hl_codec_264_bits_read_te(p_codec->pobj_bits, max_ref_idx1);
                            }
                            else {
                                p_mb->ref_idx_l1[mbPartIdx] = 0;
                            }
                        }

                        for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                            if (p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L1) {
                                for (subMbPartIdx=0; subMbPartIdx<p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                                    // mvd_l0 se(v)
                                    p_mb->mvd_l0[mbPartIdx][subMbPartIdx].x = pc_pps->entropy_coding_mode_flag
                                            ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx, subMbPartIdx, 0) */
                                            : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                    p_mb->mvd_l0[mbPartIdx][subMbPartIdx].y = pc_pps->entropy_coding_mode_flag
                                            ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx, subMbPartIdx, 1) */
                                            : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                }
                            }
                        }
                        for (mbPartIdx=0; mbPartIdx<4; ++mbPartIdx) {
                            if (p_mb->SubMbPredType[mbPartIdx] != HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8 && p_mb->SubMbPredMode[mbPartIdx] != HL_CODEC_264_SUBMB_MODE_PRED_L0) {
                                for (subMbPartIdx=0; subMbPartIdx<p_mb->NumSubMbPart[mbPartIdx]; ++subMbPartIdx) {
                                    // mvd_l1 se(v)
                                    p_mb->mvd_l1[mbPartIdx][subMbPartIdx].x = pc_pps->entropy_coding_mode_flag
                                            ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx, subMbPartIdx, 0) */
                                            : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                    p_mb->mvd_l1[mbPartIdx][subMbPartIdx].y = pc_pps->entropy_coding_mode_flag
                                            ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx, subMbPartIdx, 1) */
                                            : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                }
                            }
                        }
                    }
                    else {
                        if(_transform_size_8x8_flag && p_mb->e_type == HL_CODEC_264_MB_TYPE_I_NXN) {
                            p_mb->transform_size_8x8_flag = hl_codec_264_bits_read_u1(p_codec->pobj_bits);//FIXME: ae(v) for CABAC
                        }
                        // "mb_sub_type" not present
                        if (HL_CODEC_264_MB_TYPE_IS_INTER(p_mb)) {
                            err = hl_codec_264_mb_set_sub_mb_type_when_not_present(p_mb, p_codec);
                            if (err) {
                                goto bail;
                            }
                        }

                        // 7.3.5.1 Macroblock prediction syntax
                        // G.7.3.6.1 Macroblock prediction in scalable extension syntax
                        // begin-of-mb_pred(mb_type)
                        // mb_pred_in_scalable_extension( mb_type )
                        if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0) || HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0) || HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                            if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0)) {
                                int32_t luma4x4BlkIdx;
                                for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
                                    p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = pc_pps->entropy_coding_mode_flag
                                            ?0/*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_prev_intra4x4_pred_mode_flag) */
                                            : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                                    if (!p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
                                        p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] = pc_pps->entropy_coding_mode_flag
                                                ? 0/*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_rem_intra4x4_pred_mode) */
                                                : hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                                    }
                                }
                            }
                            else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0)) {
                                int32_t luma8x8BlkIdx;
                                for (luma8x8BlkIdx = 0; luma8x8BlkIdx < 4; ++luma8x8BlkIdx) {
                                    p_mb->prev_intra8x8_pred_mode_flag[luma8x8BlkIdx] = pc_pps->entropy_coding_mode_flag
                                            ? 0/*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_prev_intra8x8_pred_mode_flag)*/
                                            : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                                }
                                if (!p_mb->prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]) {
                                    p_mb->rem_intra8x8_pred_mode[luma8x8BlkIdx] = pc_pps->entropy_coding_mode_flag
                                            ? 0/*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_rem_intra8x8_pred_mode) */
                                            : hl_codec_264_bits_read_u(p_codec->pobj_bits, 3);
                                }
                            }
                            // NOTE: ChromaArrayType must be equal to 1 (YCbCr 420) for Baseline
                            if (pc_sps->ChromaArrayType) {
                                p_mb->intra_chroma_pred_mode = pc_pps->entropy_coding_mode_flag
                                                               ? 0/*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_intra_chroma_pred_mode) */
                                                               : hl_codec_264_bits_read_ue(p_codec->pobj_bits);
                            }
                        }
                        else if (!HL_CODEC_264_MB_MODE_IS_DIRECT(p_mb, 0)) {
                            int32_t max_ref_idx0, max_ref_idx1;

                            if (pc_slice_header->SVCExtFlag && p_mb->ext.svc.InCropWindow) {
                                if (pc_slice_header->ext.svc.adaptive_motion_prediction_flag) {
                                    // "motion_prediction_flag_lX" syntax IS available
                                    for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                                        if (!HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, mbPartIdx)) {
                                            // motion_prediction_flag_l0[ mbPartIdx ] 2 u(1) | ae(v)
                                            p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                    ? 0/*FIXME:CABAC hl264CABAC_motion_prediction_flag_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx) */
                                                    : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                                        }
                                    }
                                    for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                                        if (!HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx)) {
                                            // motion_prediction_flag_l1[ mbPartIdx ] 2 u(1) | ae(v)
                                            p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                    ? 0/*FIXME:CABAC hl264CABAC_motion_prediction_flag_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx) */
                                                    : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                                        }
                                    }
                                }
                                else {
                                    // "motion_prediction_flag_lX" syntax NOT available
                                    for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                                        p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_0][mbPartIdx] = pc_slice_header->ext.svc.default_motion_prediction_flag;
                                        p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx] = pc_slice_header->ext.svc.default_motion_prediction_flag;
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
                                    p_mb->ref_idx_l0[mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                                  ? 0/*FIXME:CABAC hl264CABAC_Read_ref_idx_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx) */
                                                                  : hl_codec_264_bits_read_te(p_codec->pobj_bits, max_ref_idx0);
                                }
                                else {
                                    p_mb->ref_idx_l0[mbPartIdx] = 0;
                                }
                            }
                            for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart ; ++mbPartIdx) {
                                if ((pc_slice_header->num_ref_idx_l1_active_minus1 > 0 ||
                                        p_mb->mb_field_decoding_flag != pc_slice_header->field_pic_flag) &&
                                        !HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx) &&
                                        (!pc_slice_header->SVCExtFlag || !p_mb->ext.svc.motion_prediction_flag_lX[listSuffixFlag_1][mbPartIdx])) {
                                    // ref_idx_l1 te(v) | ae(v)
                                    p_mb->ref_idx_l1[mbPartIdx] = pc_pps->entropy_coding_mode_flag
                                                                  ? 0/*FIXME:CABAC hl264CABAC_Read_ref_idx_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx) */
                                                                  : hl_codec_264_bits_read_te(p_codec->pobj_bits, max_ref_idx1);
                                }
                                else {
                                    p_mb->ref_idx_l1[mbPartIdx] = 0;
                                }
                            }
                            for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart; ++mbPartIdx) {
                                if (!HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, mbPartIdx)) {
                                    p_mb->mvd_l0[mbPartIdx][0].x = pc_pps->entropy_coding_mode_flag
                                                                   ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx, 0, 0) */
                                                                   : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                    p_mb->mvd_l0[mbPartIdx][0].y = pc_pps->entropy_coding_mode_flag
                                                                   ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_0, mbPartIdx, 0, 1) */
                                                                   : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                }
                            }
                            for (mbPartIdx=0; mbPartIdx<p_mb->NumMbPart ; ++mbPartIdx) {
                                if (!HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx)) {
                                    p_mb->mvd_l1[mbPartIdx][0].x = pc_pps->entropy_coding_mode_flag
                                                                   ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx, 0, 0) */
                                                                   : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                                    p_mb->mvd_l1[mbPartIdx][0].y = pc_pps->entropy_coding_mode_flag
                                                                   ? 0/*FIXME:CABAC hl264CABAC_Read_mvd_lX(ctx, CurrMb, listSuffixFlag_1, mbPartIdx, 0, 1) */
                                                                   : hl_codec_264_bits_read_se(p_codec->pobj_bits);
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
                        p_mb->ext.svc.residual_prediction_flag = pc_pps->entropy_coding_mode_flag
                                ? 0/*FIXME:CABAC hl264CABAC_residual_prediction_flag(ctx, CurrMb) */
                                : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                    }
                    else if(!IsSliceHeaderEI(pc_slice_header) && p_mb->ext.svc.InCropWindow && (p_mb->ext.svc.base_mode_flag || HL_CODEC_264_MB_TYPE_IS_INTRA(p_mb))) {
                        p_mb->ext.svc.residual_prediction_flag = pc_slice_header->ext.svc.default_residual_prediction_flag;
                    }
                    else {
                        p_mb->ext.svc.residual_prediction_flag = 0;
                    }
                }

                if (u_scan_idx_end >= u_scan_idx_start) {
                    if (!HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                        // coded_block_pattern me(v)|ae(v)
                        if (pc_pps->entropy_coding_mode_flag) {
                            // Up to "hl264CABAC_ReadSE" to compute "CodedBlockPatternLuma" and "CodedBlockPatternChroma"
                            p_mb->coded_block_pattern = 0/*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_coded_block_pattern)*/;
                        }
                        else {
                            p_mb->coded_block_pattern = hl_codec_264_bits_read_me(p_codec->pobj_bits, pc_sps->ChromaArrayType, (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0)));

                            // FIXME: see (7-35)
                            p_mb->CodedBlockPatternLuma = (p_mb->coded_block_pattern & 15); // %16
                            p_mb->CodedBlockPatternChroma = (p_mb->coded_block_pattern >> 4);
                        }

                        if (p_mb->CodedBlockPatternLuma > 0 &&
                                pc_pps->transform_8x8_mode_flag &&
                                (p_mb->ext.svc.base_mode_flag ||
                                 (p_mb->e_type != HL_CODEC_264_MB_TYPE_I_NXN &&
                                  noSubMbPartSizeLessThan8x8Flag &&
                                  (p_mb->e_type != HL_CODEC_264_MB_TYPE_B_DIRECT_16X16 ||
                                   pc_sps->direct_8x8_inference_flag)))) {
                            p_mb->transform_size_8x8_flag = pc_pps->entropy_coding_mode_flag
                                                            ?0 /*FIXME:CABAC hl264CABAC_transform_size_8x8_flag(ctx, CurrMb) */
                                                            : hl_codec_264_bits_read_u1(p_codec->pobj_bits);
                        }
                    }
                    else {
                        p_mb->CodedBlockPatternChroma = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][3];
                        p_mb->CodedBlockPatternLuma = HL_CODEC_264_MB_I_TABLE[p_mb->transform_size_8x8_flag][p_mb->mb_type][4];
                        p_mb->coded_block_pattern = (p_mb->CodedBlockPatternChroma << 4);
                    }

                    if (p_mb->CodedBlockPatternLuma > 0 || p_mb->CodedBlockPatternChroma > 0 || HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                        // mb_qp_delta se(v)|ae(v)
                        p_mb->mb_qp_delta = pc_pps->entropy_coding_mode_flag
                                            ?0 /*FIXME:CABAC hl264CABAC_ReadSE(ctx, hl264CABAC_SE_Type_mb_qp_delta) */
                                            : hl_codec_264_bits_read_se(p_codec->pobj_bits);
                        // compute Quantization values againt: because of mb_qp_delta
                        if (p_mb->mb_qp_delta) {
                            err = hl_codec_264_mb_set_default_quant_values(p_mb, p_codec);
                        }
                        if (err) {
                            goto bail;
                        }
                        b_quant_values_computed = HL_TRUE;
                    }

                    if (p_mb->CodedBlockPatternLuma > 0 || p_mb->CodedBlockPatternChroma > 0 || HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                        // residual( 0, 15 )
                        // residual( scan_idx_start, scan_idx_end )
                        err = hl_codec_264_residual_read(p_codec, p_mb, u_scan_idx_start, u_scan_idx_end);
                        if (err) {
                            goto bail;
                        }
                    }
                    else {
                        // FIXME: Instead, track all code using these variables and disable
                        hl_memory_set((int32_t*)p_mb->ChromaDCLevel, sizeof(p_mb->ChromaDCLevel)/sizeof(int32_t), 0);
                        hl_memory_set((int32_t*)p_mb->ChromaACLevel, sizeof(p_mb->ChromaACLevel)/sizeof(int32_t), 0);
                        hl_memory_set((int32_t*)p_mb->Intra16x16DCLevel, sizeof(p_mb->Intra16x16DCLevel)/sizeof(int32_t), 0);
                        hl_memory_set((int32_t*)p_mb->Intra16x16ACLevel, sizeof(p_mb->ChromaDCLevel)/sizeof(int32_t), 0);
                        hl_memory_set((int32_t*)p_mb->LumaLevel, sizeof(p_mb->LumaLevel)/sizeof(int32_t), 0);
                        hl_memory_set((int32_t*)p_mb->LumaLevel8x8, sizeof(p_mb->LumaLevel8x8)/sizeof(int32_t), 0);
                    }
                } // end-of (scab_idx_end >= scan_idx_start)
                else {
                    // FIXME: not tested yet
                    HL_DEBUG_ERROR("Not tested yet");
                    p_mb->coded_block_pattern = 0;
                    // G.7.4.6 Macroblock layer in scalable extension semantics
                    if (p_mb->ext.svc.base_mode_flag || (!(HL_CODEC_264_MB_TYPE_IS_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb))) && !HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                        p_mb->CodedBlockPatternLuma = p_mb->CodedBlockPatternChroma = 0;
                    }
                }

                // == end-of- macroblock_layer( )
            }
        } // end-of-if(b_more_data_flag)

        // Deblock macroblock
        //--ret = hl264Deblock_MB(ctx, CurrMb);

Next:
        if (!pc_pps->entropy_coding_mode_flag) {
            b_more_data_flag = hl_codec_264_rbsp_avc_more_data_read(p_codec->pobj_bits);
        }
        else {
            if (!IsSliceHeaderI(pc_slice_header) && !IsSliceHeaderSI(pc_slice_header)) {
                u_prev_mb_skipped = p_mb->mb_skip_flag;
            }
            if (pc_slice_header->MbaffFrameFlag && !(u_mb_addr & 1)) {
                b_more_data_flag = 1;
            }
            else {
                // end_of_slice_flag ae(v)
                // b_more_data_flag = !end_of_slice_flag;
                b_more_data_flag = 0/*FIXME: CABAC !hl264CABAC_Read_end_of_slice_flag(ctx)*/;
            }
        }

        // u_mb_addr = NexMbAddress(u_mb_addr);
        NextMbAddressWithoutCreate();
        ++pc_layer->i_mb_read_count;

        // Guard
        if (b_more_data_flag && (pc_layer->i_mb_read_count >= (int32_t)pc_slice_header->PicSizeInMbs || u_mb_addr >= pc_slice_header->PicSizeInMbs)) {
            // Ignore few additional bytes -> not an error
            HL_DEBUG_INFO("Macroblock address overflow");
            break;
        }
    }
    while(b_more_data_flag); // end-of do()

bail:
    if (!err && pc_layer->i_mb_read_count >= (int32_t)pc_slice_header->PicSizeInMbs) { // success and we read all macroblocks
#if 1
        if (pc_slice_header->SVCExtFlag) {
            /**** SVC (Scalable Video Coding) ****/
            err = hl_codec_264_decode_svc(p_codec);
            if (err) {
                return err;
            }
        }
        else {
            /**** AVC (Advanced Video Coding) ****/
            err = hl_codec_264_decode_avc(p_codec);
            if (err) {
                return err;
            }
        }
#else // testing AVC and ignoring SVC
        if (pc_layer->DQId == 0) {
            err = hl_codec_264_decode_avc(p_codec);
            if (err) {
                return err;
            }
        }
#endif
    }
    return err;
}

HL_ERROR_T hl_codec_264_nal_slice_data_encode(hl_codec_264_t* p_codec, hl_codec_264_encode_slice_data_t* p_esd)
{
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    hl_codec_264_nal_slice_header_t* pc_header;
    hl_codec_264_layer_t* pc_layer;
    hl_codec_264_slice_t* pc_slice;
    hl_codec_264_mb_t* p_mb;
    uint32_t u_mb_addr;
    int32_t i_qp_sum, i_qp_count, i_qp_mb, i_qp_slice, i_mad_mb, *pi_mad_mb, i_bits_hdr_count_mb, *pi_bits_hdr_count_mb, i_bits_data_count_mb, *pi_bits_data_count_mb;
    HL_VIDEO_ENCODING_TYPE_T encoding;
    HL_VIDEO_DISTORTION_MESURE_TYPE_T distortion_mesure_type;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    static const hl_bool_t __b_pskip_false = HL_FALSE;
    static const hl_bool_t __b_pskip_true = HL_TRUE;
    hl_bool_t b_intra;

    if (!p_esd || !p_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    pc_layer = p_codec->layers.pc_active;
    pc_slice = p_esd->pc_slice;
    pc_header = pc_slice->p_header;
    pc_pps = pc_header->pc_pps;
    pc_sps = pc_pps->pc_sps;
    encoding = p_codec->encoder.encoding_curr;

    pi_mad_mb = p_codec->encoder.rc.b_enabled ? &i_mad_mb : HL_NULL;
    pi_bits_hdr_count_mb = p_codec->encoder.rc.b_enabled ? &i_bits_hdr_count_mb : HL_NULL;
    pi_bits_data_count_mb = p_codec->encoder.rc.b_enabled ? &i_bits_data_count_mb : HL_NULL;
    i_qp_slice = i_qp_mb = p_codec->encoder.rc.b_enabled ? p_codec->encoder.rc.qp : p_codec->encoder.i_qp;
    u_mb_addr = pc_header->first_mb_in_slice << pc_header->MbaffFrameFlag;
    b_intra = (encoding == HL_VIDEO_ENCODING_TYPE_INTRA);
    i_qp_sum = 0, i_qp_count = 0;

    // Compute Bsize: JVT-O079 2.1.3.1.2.2 Early termination
    if (p_codec->pc_base->me_early_term_flag) {
        if (i_qp_slice != p_esd->i_qp) {
            int32_t gb_qp_per = i_qp_slice/6;
            int32_t gb_qp_rem = i_qp_slice%6;
            int32_t gb_q_bits = 15 + gb_qp_per;
            int32_t gb_qp_const = (1<<gb_q_bits)/6;
            int32_t Thresh4x4 = ((1<<gb_q_bits) - gb_qp_const)/HL_CODEC_264_QUANT_MF[gb_qp_rem][0][0];
            double Quantize_step = Thresh4x4/(4*5.61f);
            p_esd->rdo.Bsize[7] = (16*16)*Quantize_step;
            p_esd->rdo.Bsize[6] = p_esd->rdo.Bsize[7]*4;
            p_esd->rdo.Bsize[5] = p_esd->rdo.Bsize[7]*4;
            p_esd->rdo.Bsize[4] = p_esd->rdo.Bsize[5]*4;
            p_esd->rdo.Bsize[3] = p_esd->rdo.Bsize[4]*4;
            p_esd->rdo.Bsize[2] = p_esd->rdo.Bsize[4]*4;
            p_esd->rdo.Bsize[1] = p_esd->rdo.Bsize[2]*4;
            p_esd->i_qp = i_qp_slice;
        }
    }

    // Distorsion mesure type. If(AUTO) -> SSD for INTRA & SAD for INTER
    distortion_mesure_type = (p_codec->pc_base->distortion_mesure_type == HL_VIDEO_DISTORTION_MESURE_TYPE_AUTO)
                             ? (b_intra ? HL_VIDEO_DISTORTION_MESURE_TYPE_SSD : HL_VIDEO_DISTORTION_MESURE_TYPE_SAD)
                                 : p_codec->pc_base->distortion_mesure_type;

    // Update lambda (Lagrange multiplier) - FIXME: use table[I:P][QP]

    // Update method used to compute distortion and RDO data
    p_codec->encoder.rdo.d_lambda_mode = (HL_CODEC_264_RDO_LAMBDA_FACT_ALL * (1 << ((i_qp_slice - 12)/3))); // lambda_fact * 2^(QP-12)/3
    switch (distortion_mesure_type) {
    case HL_VIDEO_DISTORTION_MESURE_TYPE_SSD: {
        p_codec->encoder.rdo.d_lambda_motion = p_codec->encoder.rdo.d_lambda_mode;
        p_codec->encoder.fn_distortion_compute_4x4_u8 = hl_math_ssd4x4_u8;
        break;
    }
    case HL_VIDEO_DISTORTION_MESURE_TYPE_SAD: {
        p_codec->encoder.rdo.d_lambda_motion = HL_MATH_SQRT(p_codec->encoder.rdo.d_lambda_mode); // FIXME
        p_codec->encoder.fn_distortion_compute_4x4_u8 = hl_math_sad4x4_u8;
        break;
    }
    default: {
        HL_DEBUG_ERROR("%d not valid as distortion mesure type", p_codec->pc_base->distortion_mesure_type);
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    }

    p_esd->u_mb_skip_run = 0;

    do {
        if (!(p_mb = pc_layer->pp_list_macroblocks[u_mb_addr])) {
            err = hl_codec_264_mb_create(u_mb_addr, &p_mb);
            if (err) {
                return err;
            }
            pc_layer->pp_list_macroblocks[u_mb_addr] = p_mb;
        }
        p_mb->u_slice_idx = pc_slice->u_idx;

        // FIXME
        if (pc_layer->i_pict_encode_count >= 30 && u_mb_addr >= 90) {
            int a = 0;
        }

        // Init current macroblock (availability...)
        err = hl_codec_264_utils_init_mb_current_avc(p_codec, u_mb_addr, pc_header->l_id, __b_pskip_false);
        if (err) {
            return err;
        }

#if 0 // TODO: MB level RC not supported
        // RC, get MB QP
        if (p_codec->encoder.rc.b_enabled) {
            err = hl_codec_264_rc_rc_handle_mb(p_codec, p_mb, &i_qp_mb);
            if (err) {
                return err;
            }
            // TODO: update "d_lambda_motion" using new "i_qp_mb"
            // TODO: update "mb_qp_delta"
        }
#endif

        switch (encoding) {
        case HL_VIDEO_ENCODING_TYPE_RAW: {
            // Encode MB
            err = hl_codec_264_mb_encode_pcm(p_mb, p_codec);
            if (err) {
                return err;
            }
            break;
        }
        case HL_VIDEO_ENCODING_TYPE_INTRA:
        case HL_VIDEO_ENCODING_TYPE_INTER: {
            // Set default SVC values (to be update by prediction engine)
            if (pc_header->SVCExtFlag) {
                p_mb->mb_type = HL_CODEC_264_SVC_MB_TYPE_INFERRED;
                if (pc_sps->profile_idc == HL_CODEC_264_PROFILE_BASELINE_SVC) {
                    p_mb->ext.svc.InCropWindow = 1;
                }
                else {
                    /* G.7.4.6 Macroblock layer in scalable extension semantics */
                    int32_t mbX, mbY0, mbY1, scalMbH;
                    mbX = p_mb->u_x;
                    mbY0 = p_mb->u_y;
                    mbY1 = pc_header->MbaffFrameFlag ? (mbY0 + 1) : mbY0;
                    scalMbH = ((1 + pc_header->field_pic_flag) << 4);

                    // FIXME= 'scalMbH' is power of 2 and less than or equal to 32 -> replace '/ scalMbH' per '>> log2[scalMbH]'
                    p_mb->ext.svc.InCropWindow = (!pc_header->ext.svc.NoInterLayerPredFlag &&
                                                  (mbX >= ((pc_header->ext.svc.ScaledRefLayerLeftOffset + 15 ) >> 4)) &&
                                                  (mbX < ((pc_header->ext.svc.ScaledRefLayerLeftOffset + pc_header->ext.svc.ScaledRefLayerPicWidthInSamplesL) >> 4)) &&
                                                  (mbY0 >= ((pc_header->ext.svc.ScaledRefLayerTopOffset + scalMbH - 1) / scalMbH)) &&
                                                  (mbY1 < ((pc_header->ext.svc.ScaledRefLayerTopOffset + pc_header->ext.svc.ScaledRefLayerPicHeightInSamplesL) / scalMbH)));
                }
                p_mb->ext.svc.base_mode_flag = 1;
            }

            // Set default quantisation values
            err = hl_codec_264_mb_set_default_quant_values(p_mb, p_codec);
            if (err) {
                return err;
            }
            i_qp_sum += p_mb->QPy;
            ++i_qp_count;

            // Encode MB
            err = b_intra
                  ? hl_codec_264_mb_encode_intra(p_mb, p_codec, pi_mad_mb, pi_bits_hdr_count_mb, pi_bits_data_count_mb)
                  : hl_codec_264_mb_encode_inter(p_mb, p_codec, pi_mad_mb, pi_bits_hdr_count_mb, pi_bits_data_count_mb);
            if (err) {
                return err;
            }

            // RC - Store MAD
            if (p_codec->encoder.rc.b_enabled) {
                err = hl_codec_264_rc_rc_store_mad_and_bitscount(p_codec, p_mb, *pi_mad_mb, *pi_bits_hdr_count_mb, *pi_bits_data_count_mb);
                if (err) {
                    return err;
                }
            }

#if 0
            if (/*pc_layer->DQId > 0*/pc_layer->i_pict_encode_count == 0) {
                // Print reconstructed MB
                HL_DEBUG_INFO("===BEFORE deblocking===");
                err = hl_codec_264_mb_print_samples(p_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_header->PicWidthInSamplesL, 0/*Y*/);
                if (err) {
                    return err;
                }
            }
#endif
            break;
        }
        }

        ++pc_layer->i_mb_encode_count;
    }
    while ((int32_t)++u_mb_addr < p_esd->i_mb_end);

    /* Loop Filter */
    if (p_codec->pc_base->deblock_flag && pc_layer->i_mb_encode_count >= (int32_t)pc_header->PicSizeInMbs) {
        err = hl_codec_264_deblock_avc(p_codec);
        if (err) {
            return err;
        }
    }

    // Update RC stats
    if (p_codec->encoder.rc.b_enabled) { // RC module enabled?
        /*if (encoding == HL_VIDEO_ENCODING_TYPE_INTER) {
            int32_t i_qp_avg = (i_qp_sum/i_qp_count);
            // the sum of average QPs for all P-frames in the ith GOP. Use "hl_codec_264_rc_start_gop()" to reset this value.
            p_codec->encoder.rc.i_qp_p_avg_sum_acc += (i_qp_sum/i_qp_count);
            if (u_mb_addr == pc_header->PicSizeInMbs) { // Last slice for this picture?
                p_codec->encoder.rc.i_np++; // Number of P-Frames in the current GOP. Use "hl_codec_264_rc_start_gop()" to reset this value.
                p_codec->encoder.rc.i_qp_p_avg_sum += p_codec->encoder.rc.i_qp_p_avg_sum_acc;
            }
        }
        if (u_mb_addr == pc_header->PicSizeInMbs && p_codec->encoder.gop_left == 1) { // Last frame in the GOP?
            ++p_codec->encoder.rc.i_gop_idx;
        }*/
        if (u_mb_addr == pc_sps->uPicSizeInMapUnits) {
#if 0 // More than #1 pass supported
            if (p_codec->encoder.rc.RDPictureDecision) {
                err = hl_codec_264_rc_save_state(p_codec);
                if (err) {
                    return err;
                }
            }
#endif
        }
    }

    // rbsp_trailing_bits( )
    return hl_codec_264_rbsp_avc_trailing_bits_write(p_esd->pobj_bits);
}

HL_ERROR_T hl_codec_264_slice_create(hl_codec_264_slice_t** pp_slice)
{
    if (!pp_slice) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_slice = hl_object_create(hl_codec_264_slice_def_t);
    if (!*pp_slice) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_nal_slice_header_t" ***/
static hl_object_t* hl_codec_264_nal_slice_header_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_nal_slice_header_t *p_slice_header = (hl_codec_264_nal_slice_header_t*)self;
    if (p_slice_header) {

    }
    return self;
}
static hl_object_t* hl_codec_264_nal_slice_header_dtor(hl_object_t * self)
{
    hl_codec_264_nal_slice_header_t *p_slice_header = (hl_codec_264_nal_slice_header_t*)self;
    if (p_slice_header) {
        HL_SAFE_FREE(p_slice_header->MbToSliceGroupMap);
    }
    return self;
}
static int hl_codec_264_nal_slice_header_cmp(const hl_object_t *_v1, const hl_object_t *_v2)
{
    return (int)((int*)_v1 - (int*)_v2);
}
static const hl_object_def_t hl_codec_264_nal_slice_header_def_s = {
    sizeof(hl_codec_264_nal_slice_header_t),
    hl_codec_264_nal_slice_header_ctor,
    hl_codec_264_nal_slice_header_dtor,
    hl_codec_264_nal_slice_header_cmp,
};
const hl_object_def_t *hl_codec_264_nal_slice_header_def_t = &hl_codec_264_nal_slice_header_def_s;



/*** OBJECT DEFINITION FOR "hl_codec_264_slice_t" ***/
static hl_object_t* hl_codec_264_slice_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_slice_t *p_slice = (hl_codec_264_slice_t*)self;
    if (p_slice) {

    }
    return self;
}
static hl_object_t* hl_codec_264_slice_dtor(hl_object_t * self)
{
    hl_codec_264_slice_t *p_slice = (hl_codec_264_slice_t*)self;
    if (p_slice) {
        HL_OBJECT_SAFE_FREE(p_slice->p_header);
    }
    return self;
}
static int hl_codec_264_slice_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_slice_def_s = {
    sizeof(hl_codec_264_slice_t),
    hl_codec_264_slice_ctor,
    hl_codec_264_slice_dtor,
    hl_codec_264_slice_cmp,
};
const hl_object_def_t *hl_codec_264_slice_def_t = &hl_codec_264_slice_def_s;