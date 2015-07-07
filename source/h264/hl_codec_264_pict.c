#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_debug.h"

extern const hl_object_def_t *hl_codec_264_pict_def_t;
extern const hl_object_def_t *hl_codec_264_poc_def_t;

HL_ERROR_T hl_codec_264_pict_create(hl_codec_264_pict_t** pp_pict)
{
    if (!pp_pict) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_pict = hl_object_create(hl_codec_264_pict_def_t);
    if (!*pp_pict) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_pict)->e_type = HL_CODEC_264_PICTURE_TYPE_FRAME;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_poc_create(hl_codec_264_poc_t** pp_poc)
{
    if (!pp_poc) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_poc = hl_object_create(hl_codec_264_poc_def_t);
    if (!*pp_poc) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

// 8.2.1 Decoding process for picture order count
// Outputs of this process are TopFieldOrderCnt (if applicable) and BottomFieldOrderCnt (if applicable)
HL_ERROR_T hl_codec_264_poc_decode(hl_codec_264_poc_t* p_poc, hl_codec_264_t* p_codec)
{
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->sps.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = p_codec->layers.pc_active->pc_slice_hdr;
    switch (pc_sps->pic_order_cnt_type) {
    case 0: { /* 8.2.1.1 Decoding process for picture order count type 0 */
        //If the current picture is an IDR picture, prevPicOrderCntMsb is set equal to 0 and prevPicOrderCntLsb is set equal
        //to 0.
        if(pc_slice_header->IdrFlag) {
            p_poc->prevPicOrderCntLsb = p_poc->prevPicOrderCntMsb = 0;
        }
        else {
            if (p_poc->prevMemMgrCtrlOp == 5) {
                if (!p_poc->prevBottomFieldFlag) {
                    p_poc->prevPicOrderCntMsb = 0;
                    p_poc->prevPicOrderCntLsb = p_poc->TopFieldOrderCnt;
                }
                else {
                    p_poc->prevPicOrderCntMsb = 0;
                    p_poc->prevPicOrderCntLsb = 0;
                }
            }
            else {
                p_poc->prevPicOrderCntMsb = p_poc->PicOrderCntMsb;
                p_poc->prevPicOrderCntLsb = p_poc->PicOrderCntLsb;
            }
        }

        // Compute PicOrderCntMsb for the current picture
        if (((int32_t)pc_slice_header->pic_order_cnt_lsb < p_poc->prevPicOrderCntLsb) &&((p_poc->prevPicOrderCntLsb - (int32_t)pc_slice_header->pic_order_cnt_lsb) >= (p_poc->MaxPicOrderCntLsb / 2))) {
            p_poc->PicOrderCntMsb = p_poc->prevPicOrderCntMsb + p_poc->MaxPicOrderCntLsb; //(8-3)
        }
        else if (((int32_t)pc_slice_header->pic_order_cnt_lsb > p_poc->prevPicOrderCntLsb) &&(((int32_t)pc_slice_header->pic_order_cnt_lsb - p_poc->prevPicOrderCntLsb) > (p_poc->MaxPicOrderCntLsb / 2))) {
            p_poc->PicOrderCntMsb = p_poc->prevPicOrderCntMsb - p_poc->MaxPicOrderCntLsb;
        }
        else {
            p_poc->PicOrderCntMsb = p_poc->prevPicOrderCntMsb;
        }

        // When the current picture is not a bottom field, TopFieldOrderCnt is derived as (8-4)
        if (!pc_slice_header->bottom_field_flag) {
            p_poc->TopFieldOrderCnt = p_poc->PicOrderCntMsb + pc_slice_header->pic_order_cnt_lsb;
        }
        // When the current picture is not a top field, BottomFieldOrderCnt is derived as specified by the following pseudo-code: (8-5)
        if (/*FIXME:!pc_slice_header->top_field_falg*/HL_TRUE) {
            if(!pc_slice_header->field_pic_flag) {
                p_poc->BottomFieldOrderCnt = p_poc->TopFieldOrderCnt + pc_slice_header->delta_pic_order_cnt_bottom;
            }
            else {
                p_poc->BottomFieldOrderCnt = p_poc->PicOrderCntMsb + pc_slice_header->pic_order_cnt_lsb;
            }
        }

        break;
    }/*EndOfPicOrderCntType==0*/

    case 1: { /* 8.2.1.2 Decoding process for picture order count type 1 */
        if (!pc_slice_header->IdrFlag) {
            if (p_poc->prevMemMgrCtrlOp == 5) {
                p_poc->prevFrameNumOffset = 0;
            }
            else {
                p_poc->prevFrameNumOffset = p_poc->FrameNumOffset;
            }
        }

        //	NOTE – When gaps_in_frame_num_value_allowed_flag is equal to 1, the previous picture in decoding order may be a
        // "non-existing" frame inferred by the decoding process for gaps in frame_num specified in subclause 8.2.5.2.

        // Compute FrameNumOffset (8-6)
        if (pc_slice_header->IdrFlag) {
            p_poc->FrameNumOffset = 0;
        }
        else if (p_poc->prevFrameNum > pc_slice_header->frame_num) {
            p_poc->FrameNumOffset = p_poc->prevFrameNumOffset + p_poc->MaxFrameNum;
        }
        else {
            p_poc->FrameNumOffset = p_poc->prevFrameNumOffset;
        }

        // The variable absFrameNum is derived as specified by the following pseudo-code: (8-7)
        if (pc_sps->num_ref_frames_in_pic_order_cnt_cycle != 0) {
            p_poc->absFrameNum = p_poc->FrameNumOffset + pc_slice_header->frame_num;
        }
        else {
            p_poc->absFrameNum = 0;
        }
        if (HL_CODEC_264_NAL(pc_slice_header)->u_ref_idc == 0 && p_poc->absFrameNum > 0) {
            p_poc->absFrameNum = p_poc->absFrameNum - 1;
        }

        // When absFrameNum > 0, picOrderCntCycleCnt and frameNumInPicOrderCntCycle are derived as (8-8)
        if (p_poc->absFrameNum > 0) {
            p_poc->PicOrderCntLsb = (p_poc->absFrameNum - 1)/pc_sps->num_ref_frames_in_pic_order_cnt_cycle;
            p_poc->frameNumInPicOrderCntCycle = (p_poc->absFrameNum - 1) % pc_sps->num_ref_frames_in_pic_order_cnt_cycle;
        }
        // The variable expectedPicOrderCnt is derived as specified by the following pseudo-code: (8-9)
        if (p_poc->absFrameNum > 0) {
            uint32_t i;
            p_poc->expectedPicOrderCnt = p_poc->picOrderCntCycleCnt * pc_sps->ExpectedDeltaPerPicOrderCntCycle;
            for(i = 0; i <= p_poc->frameNumInPicOrderCntCycle; ++i) {
                p_poc->expectedPicOrderCnt = p_poc->expectedPicOrderCnt + pc_sps->offset_for_ref_frame[i];
            }
        }
        else {
            p_poc->expectedPicOrderCnt = 0;
        }
        if (HL_CODEC_264_NAL(pc_slice_header)->u_ref_idc == 0) {
            p_poc->expectedPicOrderCnt = p_poc->expectedPicOrderCnt + pc_sps->offset_for_non_ref_pic;
        }

        // The variables TopFieldOrderCnt or BottomFieldOrderCnt are derived as specified by the following pseudo-code: (8-10)
        if (!pc_slice_header->field_pic_flag) {
            p_poc->TopFieldOrderCnt = p_poc->expectedPicOrderCnt + pc_slice_header->delta_pic_order_cnt[0];
            p_poc->BottomFieldOrderCnt = p_poc->TopFieldOrderCnt + pc_sps->offset_for_top_to_bottom_field + pc_slice_header->delta_pic_order_cnt[1];
        }
        else if (!pc_slice_header->bottom_field_flag) {
            p_poc->TopFieldOrderCnt = p_poc->expectedPicOrderCnt + pc_slice_header->delta_pic_order_cnt[0];
        }
        else {
            p_poc->BottomFieldOrderCnt = p_poc->expectedPicOrderCnt + pc_sps->offset_for_top_to_bottom_field + pc_slice_header->delta_pic_order_cnt[0];
        }

        break;
    }/*EndOfPicOrderCntType==1*/

    case 2: { /* 8.2.1.3 Decoding process for picture order count type 2 */
        if (!pc_slice_header->IdrFlag) {
            if (p_poc->prevMemMgrCtrlOp == 5) {
                p_poc->prevFrameNumOffset = 0;
            }
            else {
                p_poc->prevFrameNumOffset = p_poc->FrameNumOffset;
            }
        }

        // The variable FrameNumOffset is derived as specified by the following pseudo-code: (8-11)
        if (pc_slice_header->IdrFlag) {
            p_poc->FrameNumOffset = 0;
        }
        else if (p_poc->prevFrameNum > pc_slice_header->frame_num) {
            p_poc->FrameNumOffset = p_poc->prevFrameNumOffset + p_poc->MaxFrameNum;
        }
        else {
            p_poc->FrameNumOffset = p_poc->prevFrameNumOffset;
        }

        // The variable tempPicOrderCnt is derived as specified by the following pseudo-code: (8-12)
        if (pc_slice_header->IdrFlag) {
            p_poc->tempPicOrderCnt = 0;
        }
        else if (HL_CODEC_264_NAL(pc_slice_header)->u_ref_idc == 0) {
            p_poc->tempPicOrderCnt = ((p_poc->FrameNumOffset + pc_slice_header->frame_num) << 1) - 1;
        }
        else {
            p_poc->tempPicOrderCnt = (p_poc->FrameNumOffset + pc_slice_header->frame_num) << 1;
        }

        // The variables TopFieldOrderCnt or BottomFieldOrderCnt are derived as specified by the following pseudo-code: (8-13)
        if (!pc_slice_header->field_pic_flag) {
            p_poc->TopFieldOrderCnt = p_poc->tempPicOrderCnt;
            p_poc->BottomFieldOrderCnt = p_poc->tempPicOrderCnt;
        }
        else if (pc_slice_header->bottom_field_flag) {
            p_poc->BottomFieldOrderCnt = p_poc->tempPicOrderCnt;
        }
        else {
            p_poc->TopFieldOrderCnt = p_poc->tempPicOrderCnt;
        }

        break;
    }/*EndOfPicOrderCntType==2*/
    }

    return HL_ERROR_SUCCESS;
}

// 8.5.14 Picture construction process prior to deblocking filter process (chroma only)
HL_ERROR_T hl_codec_264_pict_reconstruct_chroma(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, const int32_t u[16][16] , int32_t chroma4x4BlkIdx, hl_bool_t Cb)
{
    int32_t i,j,xO=0,yO=0,nW,nH,chroma_width;
    const hl_codec_264_layer_t *pc_layer = pc_codec->layers.pc_active;
    hl_pixel_t*const cSc = Cb ? (hl_pixel_t*const)pc_layer->pc_fs_curr->p_pict->pc_data_u : (hl_pixel_t*const)pc_layer->pc_fs_curr->p_pict->pc_data_v;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;

    nW = pc_codec->sps.pc_active->MbWidthC;
    nH = pc_codec->sps.pc_active->MbHeightC;
    chroma_width=pc_slice_header->PicWidthInSamplesC;

    // 6.4.1 Inverse macroblock scanning process
    // hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_codec->CurrMbAddr, &xP, &yP);
    // xP = pc_mb->xL;
    // yP = pc_mb->yL;

    if (nW==4 && nH==4) {
        if (pc_codec->sps.pc_active->ChromaArrayType == 1 || pc_codec->sps.pc_active->ChromaArrayType == 2) {
            xO = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][0];
            yO = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][1];
        }
        else {
            // xO,yO = subclause 6.4.4
            HL_DEBUG_ERROR("ChromaArrayType=%d not suported yet", pc_codec->sps.pc_active->ChromaArrayType);
            return HL_ERROR_NOT_IMPLEMENTED;
        }
    }
    else if (nW==8 && nH==8 && pc_codec->sps.pc_active->ChromaArrayType==3) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    if (pc_slice_header->MbaffFrameFlag && pc_mb->mb_field_decoding_flag) {
        int32_t _x = (pc_mb->xL / pc_codec->sps.pc_active->SubWidthC) + xO;
        int32_t _y = ((pc_mb->yL + pc_codec->sps.pc_active->SubHeightC - 1) / pc_codec->sps.pc_active->SubHeightC);
        for (i=0; i<nH; ++i) {
            for (j=0; j<nW; ++j) {
                cSc[(_x+j)+((_y+2*(yO+i))*chroma_width)]=u[i][j];
            }
        }
    }
    else {
        int32_t _x = pc_mb->xC + xO;
        int32_t _y = pc_mb->yC + yO;
        int32_t y = (_y * chroma_width);
        if (pc_codec->b_4pixels_aligned) { // FIXME: "chroma_width" not 16-bit aligned -> pad and use strides
            for (i = 0; i<nH; i+=4) {
                for (j = 0; j<nW; j+=4) {
                    (hl_memory_is_position_aligned((_x+j)+y) ? hl_memory_copy4x4 : hl_memory_copy4x4_unaligned)
                    ((int32_t*)&cSc[(_x+j)+y], chroma_width, (int32_t*)&u[i][j], 16);
                }
                y += (chroma_width << 2);
            }
        }
        else {
#if 1
            for (i = 0; i<nH; i+=4) {
                for (j = 0; j<nW; j+=4) {
                    hl_memory_copy4x4_u32_to_u8(&cSc[(_x+j)+y], chroma_width, &u[i][j], 16);
                }
                y += (chroma_width << 2);
            }
#else
            for (i = 0; i<nH; ++i) {
                for (j = 0; j<nW; j+=4) {
                    cSc[(_x+j)+y] = u[i][j];
                    cSc[(_x+j + 1)+y] = u[i][j + 1];
                    cSc[(_x+j + 2)+y] = u[i][j + 2];
                    cSc[(_x+j + 3)+y] = u[i][j + 3];
                }
                y += chroma_width;
            }
#endif
        }
    }

    return HL_ERROR_SUCCESS;
}

// 8.5.14 Picture construction process prior to deblocking filter process (4x4 luma only)
HL_ERROR_T hl_codec_264_pict_reconstruct_luma4x4(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, const int32_t* pc_SL, int32_t SL_stride, int32_t luma4x4BlkIdx)
{
    int32_t xP, yP, xO, yO, luma_width, k;
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    hl_pixel_t*const cSl = (hl_pixel_t*const)pc_layer->pc_fs_curr->p_pict->pc_data_y;

    // 6.4.1 Inverse macroblock scanning process
    //hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_codec->CurrMbAddr, &xP, &yP);
    xP = pc_mb->xL;
    yP = pc_mb->yL;

    // 6.4.3 Inverse 4x4 luma block scanning process
    xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
    yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

    luma_width = pc_slice_header->PicWidthInSamplesL;
    if (!pc_slice_header->MbaffFrameFlag) {
        //(8-412)
        k = (xP + xO)+(yP + yO)*luma_width;
        // FIXME: not good if "luma_width" not 16-bytes aligned -> pad and use strides
        if (pc_codec->b_4pixels_aligned) {
            (hl_memory_is_position_aligned(k) ? hl_memory_copy4x4 : hl_memory_copy4x4_unaligned)((int32_t*)&cSl[k], luma_width, (int32_t*)pc_SL, SL_stride);
        }
        else {
#if 1
            hl_memory_copy4x4_u32_to_u8(&cSl[k], luma_width, pc_SL, SL_stride);
#else
            cSl[k] = pc_SL[0][0], cSl[k+1] = pc_SL[0][1], cSl[k+2] = pc_SL[0][2], cSl[k+3] = pc_SL[0][3];
            k += luma_width;
            cSl[k] = pc_SL[1][0], cSl[k+1] = pc_SL[1][1], cSl[k+2] = pc_SL[1][2], cSl[k+3] = pc_SL[1][3];
            k += luma_width;
            cSl[k] = pc_SL[2][0], cSl[k+1] = pc_SL[2][1], cSl[k+2] = pc_SL[2][2], cSl[k+3] = pc_SL[2][3];
            k += luma_width;
            cSl[k] = pc_SL[3][0], cSl[k+1] = pc_SL[3][1], cSl[k+2] = pc_SL[3][2], cSl[k+3] = pc_SL[3][3];
#endif
        }
    }
    else {
        //(8-411)
        // FIXME: add support for hl_memory_copy4x4_mbaff
        k = (xP + xO) + (yP + ((yO) << 1))*luma_width;
        cSl[k] = pc_SL[0], cSl[k+1] = pc_SL[1], cSl[k+2] = pc_SL[2], cSl[k+3] = pc_SL[3];
        k = (xP + xO) + (yP + ((yO + 1) << 1))*luma_width;
        pc_SL += SL_stride;
        cSl[k] = pc_SL[0], cSl[k+1] = pc_SL[1], cSl[k+2] = pc_SL[2], cSl[k+3] = pc_SL[3];
        k = (xP + xO) + (yP + ((yO + 2) << 1))*luma_width;
        pc_SL += SL_stride;
        cSl[k] = pc_SL[0], cSl[k+1] = pc_SL[1], cSl[k+2] = pc_SL[2], cSl[k+3] = pc_SL[3];
        k = (xP + xO) + (yP + ((yO + 3) << 1))*luma_width;
        pc_SL += SL_stride;
        cSl[k] = pc_SL[0], cSl[k+1] = pc_SL[1], cSl[k+2] = pc_SL[2], cSl[k+3] = pc_SL[3];
    }

    return HL_ERROR_SUCCESS;
}

// 8.5.14 Picture construction process prior to deblocking filter process (16x16 luma only)
HL_ERROR_T hl_codec_264_pict_reconstruct_luma16x16(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, const int32_t u[16][16])
{
    int32_t xP, yP, xO, yO, i, luma_width, y;
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    hl_pixel_t*const cSl = (hl_pixel_t*const)pc_layer->pc_fs_curr->p_pict->pc_data_y;

    // 6.4.1 Inverse macroblock scanning process
    // hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_codec->CurrMbAddr, &xP, &yP);
    xP = pc_mb->xL;
    yP = pc_mb->yL;

    xO = 0;
    yO = 0;

    luma_width = pc_slice_header->PicWidthInSamplesL;
    if (pc_slice_header->MbaffFrameFlag == 0) {
        y = (yP + yO) * luma_width;
        for (i=0; i<16; ++i) {
            // FIXME: not good if "luma_width" not 16-bytes aligned -> pad and use strides
            //(8-412)
            if (pc_codec->b_4pixels_aligned) { // FIXME: use sizeof(pixel) == 4
                hl_pixel_t* pc_start = (cSl + (xP+xO)+y);
                (hl_memory_is_address_aligned(pc_start) ? hl_memory_copy4x4 : hl_memory_copy4x4_unaligned)((int32_t*)pc_start, 4, &u[i][0], 4);
            }
            else {
                hl_memory_copy4x4_u32_to_u8_stride4x4(&cSl[(xP+xO)+y], &u[i][0]);
            }

            y += luma_width;
        }
    }
    else {
        for (i=0; i<16; ++i) {
            // FIXME: use hl_memory_copy16x16_mbaff()
            y = (yP+2*(yO+i))*luma_width;
            //(8-411)
            cSl[(xP+xO+0)+y]=u[i][0];
            cSl[(xP+xO+1)+y]=u[i][1];
            cSl[(xP+xO+2)+y]=u[i][2];
            cSl[(xP+xO+3)+y]=u[i][3];
            cSl[(xP+xO+4)+y]=u[i][4];
            cSl[(xP+xO+5)+y]=u[i][5];
            cSl[(xP+xO+6)+y]=u[i][6];
            cSl[(xP+xO+7)+y]=u[i][7];
            cSl[(xP+xO+8)+y]=u[i][8];
            cSl[(xP+xO+9)+y]=u[i][9];
            cSl[(xP+xO+10)+y]=u[i][10];
            cSl[(xP+xO+11)+y]=u[i][11];
            cSl[(xP+xO+12)+y]=u[i][12];
            cSl[(xP+xO+13)+y]=u[i][13];
            cSl[(xP+xO+14)+y]=u[i][14];
            cSl[(xP+xO+15)+y]=u[i][15];
        }
    }

    return HL_ERROR_SUCCESS;
}


/*** OBJECT DEFINITION FOR "hl_codec_264_pict_t" ***/
static hl_object_t* hl_codec_264_pict_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_pict_t *p_pict = (hl_codec_264_pict_t*)self;
    if (p_pict) {

    }
    return self;
}
static hl_object_t* hl_codec_264_pict_dtor(hl_object_t * self)
{
    hl_codec_264_pict_t *p_pict = (hl_codec_264_pict_t*)self;
    if (p_pict) {

    }
    return self;
}
static int hl_codec_264_pict_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_pict_def_s = {
    sizeof(hl_codec_264_pict_t),
    hl_codec_264_pict_ctor,
    hl_codec_264_pict_dtor,
    hl_codec_264_pict_cmp,
};
const hl_object_def_t *hl_codec_264_pict_def_t = &hl_codec_264_pict_def_s;


/*** OBJECT DEFINITION FOR "hl_codec_264_poc_t" ***/
static hl_object_t* hl_codec_264_poc_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_poc_t *p_poc = (hl_codec_264_poc_t*)self;
    if (p_poc) {

    }
    return self;
}
static hl_object_t* hl_codec_264_poc_dtor(hl_object_t * self)
{
    hl_codec_264_poc_t *p_poc = (hl_codec_264_poc_t*)self;
    if (p_poc) {
        HL_LIST_STATIC_CLEAR_OBJECTS(p_poc->RefPicList0, p_poc->RefPicList0Count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_poc->RefPicList1, p_poc->RefPicList1Count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_poc->refFrameList0ShortTerm, p_poc->refFrameList0ShortTermCount);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_poc->refFrameList0LongTerm, p_poc->refFrameList0LongTermCount);
    }
    return self;
}
static int hl_codec_264_poc_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_poc_def_s = {
    sizeof(hl_codec_264_poc_t),
    hl_codec_264_poc_ctor,
    hl_codec_264_poc_dtor,
    hl_codec_264_poc_cmp,
};
const hl_object_def_t *hl_codec_264_poc_def_t = &hl_codec_264_poc_def_s;

