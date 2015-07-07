#ifndef _HARTALLO_CODEC_264_NAL_PPS_H_
#define _HARTALLO_CODEC_264_NAL_PPS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_nal.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_nal_pps_s;

HL_ERROR_T hl_codec_264_nal_pps_create(unsigned u_ref_idc, HL_CODEC_264_NAL_TYPE_T e_type, struct hl_codec_264_nal_pps_s** pp_pps);
HL_ERROR_T hl_codec_264_nal_pps_decode(HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc, struct hl_codec_264_s* p_codec, struct hl_codec_264_nal_pps_s** pp_pps);
HL_ERROR_T hl_codec_264_nal_pps_encode(struct hl_codec_264_nal_pps_s* p_pps, uint32_t sps_id, struct hl_codec_264_s* p_codec);

typedef struct hl_codec_264_nal_pps_s {
    HL_DECLARE_264_NAL;

    uint32_t pic_parameter_set_id;
    uint32_t seq_parameter_set_id;
    unsigned entropy_coding_mode_flag:1;
    unsigned bottom_field_pic_order_in_frame_present_flag:1;
    uint32_t num_slice_groups_minus1; // 0-7
    uint32_t slice_group_map_type;
    uint32_t run_length_minus1[8/*0-num_slice_groups_minus1*/];
    uint32_t top_left[8/*0-num_slice_groups_minus1*/];
    uint32_t bottom_right[8/*0-num_slice_groups_minus1*/];
    unsigned slice_group_change_direction_flag:1;
    uint32_t slice_group_change_rate_minus1;
    uint32_t pic_size_in_map_units_minus1; // PicSizeInMapUnits ? 1. PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits
    uint32_t *slice_group_id; // sizeof(uint32_t) * pic_size_in_map_units_minus1
    uint32_t num_ref_idx_l0_default_active_minus1;
    uint32_t num_ref_idx_l1_default_active_minus1;
    unsigned weighted_pred_flag:1;
    unsigned weighted_bipred_idc:2;
    int32_t pic_init_qp_minus26;
    int32_t pic_init_qs_minus26;
    int32_t chroma_qp_index_offset;
    unsigned deblocking_filter_control_present_flag:1;
    unsigned constrained_intra_pred_flag:1;
    unsigned redundant_pic_cnt_present_flag:1;
    unsigned transform_8x8_mode_flag:1;
    unsigned pic_scaling_matrix_present_flag:1;
    //===
    uint8_t pic_scaling_list_present_flag[12];
    int32_t ScalingList4x4[6][16]; // (12/2)
    uint8_t UseDefaultScalingMatrix4x4Flag[6][16];
    int32_t ScalingList8x8[6][64]; // (12/2)
    uint8_t UseDefaultScalingMatrix8x8Flag[6][64];
    //===
    int32_t second_chroma_qp_index_offset;
    int32_t weightScale4x4[2/*mbIsInterFlag*/][3/*iYCbCr*/][4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t LevelScale4x4[2/*mbIsInterFlag*/][3/*iYCbCr*/][6/*m*/][4/*i*/][4/*j*/];


    uint32_t uaNumRefIdxActive[HL_CODEC_264_LIST_IDX_MAX_COUNT];
    uint32_t uPicInitQp;


    uint32_t SliceGroupChangeRate;

    const struct hl_codec_264_nal_sps_s* pc_sps;
}
hl_codec_264_nal_pps_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_NAL_PPS_H_ */
