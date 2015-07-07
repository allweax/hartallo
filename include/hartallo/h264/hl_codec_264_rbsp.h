#ifndef _HARTALLO_CODEC_264_RBSP_H_
#define _HARTALLO_CODEC_264_RBSP_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

struct hl_codec_264_hrd_s;
struct hl_codec_264_bits_s;
struct hl_codec_264_nal_slice_header_s;
struct hl_codec_264_nal_sps_s;
struct hl_codec_264_svc_dec_ref_base_pic_marking_xs;

hl_bool_t hl_codec_264_rbsp_avc_more_data_read(struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_scaling_list_read(struct hl_codec_264_bits_s* p_bits, int32_t scalingList[], uint32_t sizeOfScalingList, uint8_t useDefaultScalingMatrixFlag[]);
HL_ERROR_T hl_codec_264_rbsp_avc_scaling_list_write(struct hl_codec_264_bits_s* p_bits, int32_t scalingList[], uint32_t sizeOfScalingList, uint8_t useDefaultScalingMatrixFlag[]);
HL_ERROR_T hl_codec_264_rbsp_avc_vui_parameters_read(struct hl_codec_264_nal_sps_s* p_sps, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_hrd_parameters_read(struct hl_codec_264_hrd_s* p_hrd, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_trailing_bits_read(struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_trailing_bits_write(struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_ref_pic_list_modification_read(struct hl_codec_264_nal_slice_header_s *p_slice_header, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_ref_pic_list_modification_write(struct hl_codec_264_nal_slice_header_s *p_slice_header, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_pred_weight_table_read(struct hl_codec_264_nal_slice_header_s *p_slice_header, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_dec_ref_pic_marking_read(struct hl_codec_264_nal_slice_header_s *p_slice_header, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_dec_ref_pic_marking_write(struct hl_codec_264_nal_slice_header_s *p_slice_header, struct hl_codec_264_bits_s* p_bits);
HL_ERROR_T hl_codec_264_rbsp_avc_escape(uint8_t* p_rbsp_bytes, hl_size_t u_rbsp_bytes_num, hl_size_t u_rbsp_bytes_size);

/********** Scalable Video Coding (SVC) **********/
HL_ERROR_T hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_read(struct hl_codec_264_bits_s* p_bits, struct hl_codec_264_svc_dec_ref_base_pic_marking_xs *p_dec_ref);
HL_ERROR_T hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_write(struct hl_codec_264_bits_s* p_bits, struct hl_codec_264_svc_dec_ref_base_pic_marking_xs *p_dec_ref);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_RBSP_H_ */