#ifndef _HARTALLO_CODEC_264_DPB_H_
#define _HARTALLO_CODEC_264_DPB_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_dpb_s;
struct hl_codec_264_nal_sps_s;

HL_ERROR_T hl_codec_264_dpb_create(struct hl_codec_264_dpb_s** pp_dpb);
HL_ERROR_T hl_codec_264_dpb_init(struct hl_codec_264_dpb_s* p_dpb, const struct hl_codec_264_nal_sps_s* pc_sps);
HL_ERROR_T hl_codec_264_dpb_map_current(struct hl_codec_264_s* p_codec, struct hl_codec_264_dpb_s* p_dpb);
HL_ERROR_T hl_codec_264_dpb_add_decoded(struct hl_codec_264_s* p_codec);

typedef struct hl_codec_264_dpb_fs_s {
    HL_DECLARE_OBJECT;
    struct hl_codec_264_pict_s* p_pict;
    const struct hl_codec_264_dpb_s* pc_dpb;

    enum HL_CODEC_264_REF_TYPE_E RefType;

    hl_bool_t NonExisting;

    int32_t PicNum;
    int32_t LongTermPicNum;

    int32_t FrameNum;
    int32_t FrameNumWrap;
    int32_t LongTermFrameIdx;
    int32_t PicOrderCnt;
}
hl_codec_264_dpb_fs_t;

typedef struct hl_codec_264_dpb_s {
    HL_DECLARE_OBJECT;
    const struct hl_codec_264_s* pc_codec;
    struct hl_codec_264_nal_sps_s* p_sps;
    struct hl_codec_264_dpb_fs_s* p_list_fs[HL_CODEC_264_FS_MAX_COUNT];
    hl_size_t u_list_fs_count;
    hl_size_t u_list_fs_count_max;

    struct hl_codec_264_interpol_indices_s* p_list_interpol_indices[2/*mbaff=0/1*/];
    hl_size_t u_list_list_interpol_indices_count;

    hl_size_t numShortTerm;
    hl_size_t numLongTerm;

    // p_buff: 0....MAX_FS = reference frames, MAX_FS+1 = dec. picture
    hl_pixel_t* p_buff; // buffer shared by all reference frames and the current picture being decoded
    hl_size_t u_buff_size_overall; // (luma + chroma size for a single FS) * number of FSs - in pixel
    hl_size_t u_buff_size_inuse; // - in pixel
    hl_size_t u_buff_size_fs_y; // luma size for a single FS - in pixel
    hl_size_t u_buff_size_fs_c; // chroma size for a single FS - in pixel
    hl_size_t u_buff_size_fs_ycc; // luma, U, V
}
hl_codec_264_dpb_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_DPB_H_ */
