#ifndef _HARTALLO_CODEC_264_NAL_SLICE_H_
#define _HARTALLO_CODEC_264_NAL_SLICE_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_nal.h"

HL_BEGIN_DECLS

struct hl_codec_264_nal_slice_header_s;
struct hl_codec_264_slice_s;

HL_ERROR_T hl_codec_264_nal_slice_header_create(unsigned u_ref_idc, HL_CODEC_264_NAL_TYPE_T e_type, struct hl_codec_264_nal_slice_header_s** pp_slice_header);
HL_ERROR_T hl_codec_264_nal_slice_header_decode(struct hl_codec_264_s* p_codec, struct hl_codec_264_nal_slice_header_s** pp_slice_header);
HL_ERROR_T hl_codec_264_nal_slice_header_encode(struct hl_codec_264_s* p_codec, hl_size_t u_idx, hl_bool_t b_derive, hl_bool_t b_write);
HL_ERROR_T hl_codec_264_nal_slice_data_decode(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_nal_slice_data_encode(struct hl_codec_264_s* p_codec, struct hl_codec_264_encode_slice_data_s* p_esd);

HL_ERROR_T hl_codec_264_slice_create(struct hl_codec_264_slice_s** pp_slice);

typedef struct hl_codec_264_slice_s {
    HL_DECLARE_OBJECT;

    hl_size_t u_idx;

    struct hl_codec_264_nal_slice_header_s* p_header;
}
hl_codec_264_slice_t;

typedef struct hl_codec_264_nal_slice_header_s {
    HL_DECLARE_264_NAL;

    uint32_t first_mb_in_slice;
    enum HL_CODEC_264_SLICE_TYPE_E slice_type;
    uint32_t pic_parameter_set_id;
    unsigned colour_plane_id:2;
    uint32_t frame_num;
    unsigned field_pic_flag:1;
    unsigned bottom_field_flag:1;
    uint32_t idr_pic_id;
    uint32_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt_bottom;
    int32_t delta_pic_order_cnt[2];
    uint32_t redundant_pic_cnt;
    unsigned direct_spatial_mv_pred_flag:1;
    unsigned num_ref_idx_active_override_flag:1;
    uint32_t num_ref_idx_l0_active_minus1;
    uint32_t num_ref_idx_l1_active_minus1;
    uint32_t cabac_init_idc;
    int32_t slice_qp_delta;
    unsigned sp_for_switch_flag:1;
    int32_t slice_qs_delta;
    uint32_t disable_deblocking_filter_idc;
    int32_t slice_alpha_c0_offset_div2;
    int32_t slice_beta_offset_div2;
    uint32_t slice_group_change_cycle;
    unsigned ref_pic_list_modification_flag_l0:1;
    uint32_t modification_of_pic_nums_idc_l0[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t abs_diff_pic_num_minus1_l0[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t long_term_pic_num_l0[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    unsigned ref_pic_list_modification_flag_l1:1;
    uint32_t modification_of_pic_nums_idc_l1[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t abs_diff_pic_num_minus1_l1[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t long_term_pic_num_l1[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];

    struct {
        union {
            struct {
                int32_t ref_layer_dq_id; // ue(v) but could be inferred as equal to -1
                uint32_t disable_inter_layer_deblocking_filter_idc;
                int32_t inter_layer_slice_alpha_c0_offset_div2;
                int32_t inter_layer_slice_beta_offset_div2;
                uint32_t constrained_intra_resampling_flag;
                uint32_t ref_layer_chroma_phase_x_plus1_flag;
                uint32_t ref_layer_chroma_phase_y_plus1;
                int32_t scaled_ref_layer_left_offset;
                int32_t scaled_ref_layer_top_offset;
                int32_t scaled_ref_layer_right_offset;
                int32_t scaled_ref_layer_bottom_offset;
                uint32_t slice_skip_flag:1;
                uint32_t num_mbs_in_slice_minus1;
                uint32_t adaptive_base_mode_flag:1;
                uint32_t default_base_mode_flag:1;
                uint32_t adaptive_motion_prediction_flag;
                uint32_t default_motion_prediction_flag;
                uint32_t adaptive_residual_prediction_flag;
                uint32_t default_residual_prediction_flag;
                uint32_t tcoeff_level_prediction_flag;
                uint32_t scan_idx_start;
                uint32_t scan_idx_end;

                struct hl_codec_264_svc_dec_ref_base_pic_marking_xs xs_dec_ref_base_pic_marking;

                // FIXME: remove all next fields
                uint32_t PriorityId; // syntax "priority_id" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t NoInterLayerPredFlag; // syntax "no_inter_layer_pred_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t DependencyId; // syntax "dependency_id" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t QualityId; // syntax "quality_id" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t TemporalId; // syntax "temporal_id" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t UseRefBasePicFlag; // syntax "use_ref_base_pic_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t DiscardableFlag; // syntax "discardable_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
                uint32_t OutputFlag; // syntax "output_flag" from NAL Prefix. /!\DO NOT derive() when decoding.

                // FIXME: remove all next fields
                uint32_t MinNoInterLayerPredFlag;
                uint32_t CroppingChangeFlag;
                uint32_t SpatialResolutionChangeFlag;
                int32_t MaxRefLayerDQId;
                uint32_t DQId;
                int32_t ScaledRefLayerLeftOffset;
                int32_t ScaledRefLayerRightOffset;
                int32_t ScaledRefLayerTopOffset;
                int32_t ScaledRefLayerBottomOffset;
                int32_t ScaledRefLayerPicWidthInSamplesL;
                int32_t ScaledRefLayerPicHeightInSamplesL;
                int32_t ScaledRefLayerPicWidthInSamplesC;
                int32_t ScaledRefLayerPicHeightInSamplesC;
                int32_t InterlayerFilterOffsetA;
                int32_t InterlayerFilterOffsetB;
            } svc;

            struct {
                unsigned NOT_IMPLEMENTED:1;
            } mvc;
        };
    } ext;

    struct hl_codec_264_avc_pred_weight_table_xs xs_pred_weight_table;
    struct hl_codec_264_avc_dec_ref_base_pic_marking_xs xs_dec_ref_base_pic_marking;
    const struct hl_codec_264_s* pc_codec;
    const struct hl_codec_264_nal_pps_s* pc_pps;

    long l_id; // Hartallo-specific: unique identifier update when the header is recycled
    enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5; // FIXME: not extensively used (e.g. all isSliceXX())
    uint32_t PicSizeInMbs;
    uint32_t PicWidthInMbs;
    uint32_t PicHeightInMbs;
    uint32_t PicHeightInMapUnits;
    uint32_t PicSizeInMapUnits;
    uint32_t FrameHeightInMbs;
    int32_t MapUnitsInSliceGroup0;
    int32_t* MbToSliceGroupMap; // size=PicSizeInMbs
    hl_size_t MbToSliceGroupMapCount;
    uint32_t MbaffFrameFlag;
    uint32_t PicWidthInSamplesL;
    uint32_t PicWidthInSamplesC;
    uint32_t PicHeightInSamplesL;
    uint32_t PicHeightInSamplesC;
    int32_t FilterOffsetA;
    int32_t FilterOffsetB;

    uint32_t IdrFlag; // syntax "idr_flag" from from NAL Prefix or IdrFlag(slice_type)
    uint32_t SVCExtFlag; // syntax "svc_extension_flag" from NAL Prefix. /!\DO NOT derive() when decoding. Defines whether SVC is enabled on the current slice.

    int32_t SliceQPY;
    int32_t QSY;
}
hl_codec_264_nal_slice_header_t;

HL_END_DECLS


#endif /* _HARTALLO_CODEC_264_NAL_SLICE_H_ */
