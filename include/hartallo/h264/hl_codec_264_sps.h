#ifndef _HARTALLO_CODEC_264_NAL_SPS_H_
#define _HARTALLO_CODEC_264_NAL_SPS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_nal.h"

HL_BEGIN_DECLS

struct hl_codec_264_nal_sps_s;
struct hl_codec_264_s;

HL_ERROR_T hl_codec_264_nal_sps_create(unsigned u_ref_idc, enum HL_CODEC_264_NAL_TYPE_E e_type, struct hl_codec_264_nal_sps_s** pp_sps);
HL_ERROR_T hl_codec_264_nal_sps_decode(HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc, struct hl_codec_264_s* p_codec, struct hl_codec_264_nal_sps_s** pp_sps);
HL_ERROR_T hl_codec_264_nal_sps_encode(struct hl_codec_264_nal_sps_s* p_sps, struct hl_codec_264_s* p_codec);

typedef struct hl_codec_264_nal_sps_svc_s {
    HL_DECLARE_OBJECT;

    unsigned inter_layer_deblocking_filter_control_present_flag:1;
    enum HL_CODEC_264_ESS_E extended_spatial_scalability_idc;
    unsigned chroma_phase_x_plus1_flag:1;
    unsigned chroma_phase_y_plus1; // u(2)
    unsigned seq_ref_layer_chroma_phase_x_plus1_flag:1;
    unsigned seq_ref_layer_chroma_phase_y_plus1; // u(2)
    int32_t seq_scaled_ref_layer_left_offset;
    int32_t seq_scaled_ref_layer_top_offset;
    int32_t seq_scaled_ref_layer_right_offset;
    int32_t seq_scaled_ref_layer_bottom_offset;
    unsigned seq_tcoeff_level_prediction_flag:1;
    unsigned adaptive_tcoeff_level_prediction_flag:1;
    unsigned slice_header_restriction_flag:1;
    unsigned svc_vui_parameters_present_flag:1;

    struct hl_codec_264_vui_svc_s* p_vui;
}
hl_codec_264_nal_sps_svc_t;

typedef struct hl_codec_264_nal_sps_s {
    HL_DECLARE_264_NAL;

    HL_CODEC_264_PROFILE_T profile_idc;
    unsigned constraint_set0_flag:1;
    unsigned constraint_set1_flag:1;
    unsigned constraint_set2_flag:1;
    unsigned constraint_set3_flag:1;
    unsigned constraint_set4_flag:1;
    unsigned constraint_set5_flag:1;
    unsigned reserved_zero_2bits:2;
    enum HL_CODEC_264_LEVEL_E level_idc;
    uint32_t seq_parameter_set_id;
    uint32_t chroma_format_idc;
    unsigned separate_colour_plane_flag:1;
    uint32_t bit_depth_luma_minus8;
    uint32_t bit_depth_chroma_minus8;
    unsigned qpprime_y_zero_transform_bypass_flag:1;
    unsigned seq_scaling_matrix_present_flag:1;
    //===
    uint8_t seq_scaling_list_present_flag[12];
    int32_t ScalingList4x4[6][16]; // (12/2)
    uint8_t UseDefaultScalingMatrix4x4Flag[6][16];
    int32_t ScalingList8x8[6][64]; // (12/2)
    uint8_t UseDefaultScalingMatrix8x8Flag[6][64];
    //===
    uint32_t log2_max_frame_num_minus4;
    uint32_t pic_order_cnt_type;
    uint32_t log2_max_pic_order_cnt_lsb_minus4;
    unsigned delta_pic_order_always_zero_flag:1;
    int32_t offset_for_non_ref_pic;
    int32_t offset_for_top_to_bottom_field;
    uint32_t num_ref_frames_in_pic_order_cnt_cycle;  //range of 0 to 255,
    int32_t offset_for_ref_frame[256]; // maxvalues= num_ref_frames_in_pic_order_cnt_cycle
    uint32_t max_num_ref_frames;
    unsigned gaps_in_frame_num_value_allowed_flag:1;
    uint32_t pic_width_in_mbs_minus1;
    uint32_t pic_height_in_map_units_minus1;
    unsigned frame_mbs_only_flag:1;
    unsigned mb_adaptive_frame_field_flag:1;
    unsigned direct_8x8_inference_flag:1;
    unsigned frame_cropping_flag:1;
    uint32_t frame_crop_left_offset;
    uint32_t frame_crop_right_offset;
    uint32_t frame_crop_top_offset;
    uint32_t frame_crop_bottom_offset;

    unsigned vui_parameters_present_flag:1;
    struct hl_codec_264_vui_s* p_vui;

    hl_bool_t b_svc;
    struct hl_codec_264_nal_sps_svc_s* p_svc;

    uint32_t uLog2MaxFrameNum;
    uint32_t uMaxFrameNum;
    uint32_t uFrameWidthInMbs;
    uint32_t uPicWidthInMbs;
    uint32_t uFrameHeightInMbs;
    uint32_t uPicHeightInMapUnit;
    uint32_t uPicSizeInMapUnits;

    uint32_t ExpectedDeltaPerPicOrderCntCycle;
    uint32_t ChromaArrayType;
    uint32_t MbWidthC;
    uint32_t MbHeightC;
    uint32_t SubWidthC;
    uint32_t SubHeightC;
    enum HL_CODEC_264_CHROMAFORMAT_E ChromaFormat;
    int32_t BitDepthY;
    int32_t QpBdOffsetY;
    int32_t BitDepthC;
    int32_t QpBdOffsetC;
    uint32_t SubWidthC_TrailingZeros;
    uint32_t SubHeightC_TrailingZeros;
}
hl_codec_264_nal_sps_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_NAL_SPS_H_ */
