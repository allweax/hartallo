#ifndef _HARTALLO_CODEC_264_VUI_H_
#define _HARTALLO_CODEC_264_VUI_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

struct hl_codec_264_vui_s;
struct hl_codec_264_vui_svc_s;

HL_ERROR_T hl_codec_264_vui_create(struct hl_codec_264_vui_s** pp_vui);
HL_ERROR_T hl_codec_264_vui_svc_create(struct hl_codec_264_vui_svc_s** pp_vui);

typedef struct hl_codec_264_vui_s {
    HL_DECLARE_OBJECT;

    unsigned aspect_ratio_info_present_flag:1;
    enum HL_CODEC_264_ASPECT_RATIO_E aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    unsigned overscan_info_present_flag:1;
    unsigned overscan_appropriate_flag:1;
    unsigned video_signal_type_present_flag:1;
    unsigned video_format:3;
    unsigned video_full_range_flag:1;
    unsigned colour_description_present_flag:1;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
    unsigned chroma_loc_info_present_flag:1;
    uint32_t chroma_sample_loc_type_top_field;
    uint32_t chroma_sample_loc_type_bottom_field;
    unsigned timing_info_present_flag:1;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    unsigned fixed_frame_rate_flag:1;
    unsigned nal_hrd_parameters_present_flag:1;
    unsigned vcl_hrd_parameters_present_flag:1;
    unsigned low_delay_hrd_flag:1;
    unsigned pic_struct_present_flag:1;
    unsigned bitstream_restriction_flag:1;
    unsigned motion_vectors_over_pic_boundaries_flag:1;
    uint32_t max_bytes_per_pic_denom;
    uint32_t max_bits_per_mb_denom;
    uint32_t log2_max_mv_length_horizontal;
    uint32_t log2_max_mv_length_vertical;
    uint32_t num_reorder_frames;
    uint32_t max_dec_frame_buffering;

    struct hl_codec_264_hrd_s* p_hrd_nal; // depends on "nal_hrd_parameters_present_flag"
    struct hl_codec_264_hrd_s* p_hrd_vcl; // depends  on "vcl_hrd_parameters_present_flag"
}
hl_codec_264_vui_t;

typedef struct hl_codec_264_vui_svc_s {
    HL_DECLARE_OBJECT;

    uint32_t vui_ext_num_entries_minus1;
    uint32_t vui_ext_dependency_id[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_quality_id[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_temporal_id[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_timing_info_present_flag[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_num_units_in_tick[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_time_scale[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_fixed_frame_rate_flag[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_nal_hrd_parameters_present_flag[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_vcl_hrd_parameters_present_flag[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_low_delay_hrd_flag[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];
    uint32_t vui_ext_pic_struct_present_flag[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT];

    struct hl_codec_264_hrd_s* hrds_nal[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT]; // depends on "vui_ext_nal_hrd_parameters_present_flag[]"
    struct hl_codec_264_hrd_s* hrds_vcl[HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT]; // depends on "vui_ext_vcl_hrd_parameters_present_flag[]"
}
hl_codec_264_vui_svc_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_VUI_H_ */
