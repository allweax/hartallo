#ifndef _HARTALLO_CODEC_264_PICT_H_
#define _HARTALLO_CODEC_264_PICT_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_pict_s;
struct hl_codec_264_mb_s;
struct hl_codec_264_poc_s;

HL_ERROR_T hl_codec_264_pict_create(struct hl_codec_264_pict_s** pp_pict);
HL_ERROR_T hl_codec_264_pict_reconstruct_chroma(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, const int32_t u[16][16] , int32_t chroma4x4BlkIdx, hl_bool_t Cb);
HL_ERROR_T hl_codec_264_pict_reconstruct_luma4x4(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, const int32_t* pc_SL, int32_t SL_stride, int32_t luma4x4BlkIdx);
HL_ERROR_T hl_codec_264_pict_reconstruct_luma16x16(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, const int32_t u[16][16]);

HL_ERROR_T hl_codec_264_poc_create(struct hl_codec_264_poc_s** pp_poc);
HL_ERROR_T hl_codec_264_poc_decode(struct hl_codec_264_poc_s* p_poc, struct hl_codec_264_s* p_codec);

typedef struct hl_codec_264_pict_s {
    HL_DECLARE_OBJECT;

    enum HL_CODEC_264_PICTURE_TYPE_E e_type;

    uint32_t uWidthL;
    uint32_t uHeightL;
    uint32_t uWidthC;
    uint32_t uHeightC;

    hl_pixel_t* pc_data_y;
    hl_pixel_t* pc_data_u;
    hl_pixel_t* pc_data_v;
}
hl_codec_264_pict_t;


typedef struct hl_codec_264_poc_s {
    HL_DECLARE_OBJECT;

    struct hl_codec_264_dpb_fs_s* RefPicList0[HL_CODEC_264_REFPICT_LIST0_MAX_COUNT];
    hl_size_t RefPicList0Count;
    struct hl_codec_264_dpb_fs_s* RefPicList1[HL_CODEC_264_REFPICT_LIST1_MAX_COUNT];
    hl_size_t RefPicList1Count;

    struct hl_codec_264_dpb_fs_s* refFrameList0ShortTerm[HL_CODEC_264_REFPICT_LIST0_MAX_COUNT];
    hl_size_t refFrameList0ShortTermCount;
    struct hl_codec_264_dpb_fs_s* refFrameList0LongTerm[HL_CODEC_264_REFPICT_LIST0_MAX_COUNT];
    hl_size_t refFrameList0LongTermCount;

    uint32_t TopFieldOrderCnt;
    uint32_t BottomFieldOrderCnt;
    uint32_t prevPicOrderCntMsb;
    uint32_t prevPicOrderCntLsb;
    hl_bool_t prevBottomFieldFlag;
    uint32_t PicOrderCntMsb;
    uint32_t PicOrderCntLsb;
    uint32_t MaxPicOrderCntLsb;
    uint32_t FrameNumOffset;
    uint32_t prevFrameNumOffset;
    uint32_t prevFrameNum;
    uint32_t MaxFrameNum;
    uint32_t MaxPicNum;
    int32_t MaxLongTermFrameIdx;
    uint32_t CurrPicNum;
    uint32_t absFrameNum;
    uint32_t picOrderCntCycleCnt;
    uint32_t frameNumInPicOrderCntCycle;
    uint32_t expectedPicOrderCnt;
    uint32_t tempPicOrderCnt;
    //Previous "memory_management_control_operation" field in pic dec order
    uint32_t prevMemMgrCtrlOp;
}
hl_codec_264_poc_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_PICT_H_ */
