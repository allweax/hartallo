#include "hartallo/h264/hl_codec_264_pred_intra.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_transf.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

static void HL_SHOULD_INLINE _Intra_4x4_Vertical(int32_t pred4x4L[4][4], const int32_t p[13]);
static void HL_SHOULD_INLINE _Intra_4x4_Horizontal(int32_t pred4x4L[4][4], const int32_t p[13]);
static void HL_SHOULD_INLINE _Intra_4x4_DC(int32_t pred4x4L[4][4], const int32_t p[13], const hl_codec_264_t* p_codec);
static void HL_SHOULD_INLINE _Intra_4x4_Diagonal_Down_Left(int32_t pred4x4L[4][4], const int32_t p[13]);
static void HL_SHOULD_INLINE _Intra_4x4_Diagonal_Down_Right(int32_t pred4x4L[4][4], const int32_t p[13]);
static void HL_SHOULD_INLINE _Intra_4x4_Vertical_Right(int32_t pred4x4L[4][4], const int32_t p[13]);
static void HL_SHOULD_INLINE _Intra_4x4_Horizontal_Down(int32_t pred4x4L[4][4], const int32_t p[13]);
static void HL_SHOULD_INLINE _Intra_4x4_Vertical_Left(int32_t pred4x4L[4][4], const int32_t p[13], const hl_codec_264_t* p_codec);
static void HL_SHOULD_INLINE _Intra_4x4_Horizontal_Up(int32_t pred4x4L[4][4], const int32_t p[13]);

static void HL_SHOULD_INLINE _Intra_16x16_Vertical(int32_t pred16x16L[16][16], const int32_t p[33]);
static void HL_SHOULD_INLINE _Intra_16x16_Horizontal(int32_t pred16x16L[16][16], const int32_t p[33]);
static void HL_SHOULD_INLINE _Intra_16x16_DC(int32_t pred16x16L[16][16], const int32_t p[33], const hl_codec_264_t* p_codec);
static void HL_SHOULD_INLINE _Intra_16x16_Plane(int32_t pred16x16L[16][16], const int32_t p[33], int32_t BitDepthY);

static void HL_SHOULD_INLINE _Intra_Chroma_DC(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec);
static void HL_SHOULD_INLINE _Intra_Chroma_Horizontal(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec);
static void HL_SHOULD_INLINE _Intra_Chroma_Vertical(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec);
static void HL_SHOULD_INLINE _Intra_Chroma_Plane(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec);

static HL_ERROR_T HL_SHOULD_INLINE _hl_codec_264_pred_intra_get_intra4x4_pred_mode(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
static HL_ERROR_T HL_SHOULD_INLINE _hl_codec_264_pred_intra_decode_luma(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
static HL_ERROR_T HL_SHOULD_INLINE _hl_codec_264_pred_intra_decode_chroma(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);

// 8.3 Intra prediction process
// This process is invoked for I and SI macroblock types.
HL_ERROR_T hl_codec_264_pred_intra_decode(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    HL_ERROR_T err;
    err = _hl_codec_264_pred_intra_decode_luma(p_codec, p_mb);
    if (err) {
        return err;
    }
    return _hl_codec_264_pred_intra_decode_chroma(p_codec, p_mb);
}

static HL_SHOULD_INLINE HL_ERROR_T _hl_codec_264_pred_intra_decode_luma(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_memory_blocks_t* pc_mem_blocks;
    hl_int32_13_t *p13;
    hl_int32_33_t *p33;
    hl_int32_4x4_t *pred4x4L;
    hl_int32_16x16_t *pred16x16L;

    if (p_mb->u_addr == 16) { // FIXME
        int a = 0;
    }

    pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

    // map() memory
    hl_memory_blocks_map(pc_mem_blocks, &p13);
    hl_memory_blocks_map(pc_mem_blocks, &p33);
    hl_memory_blocks_map(pc_mem_blocks, &pred4x4L);
    hl_memory_blocks_map(pc_mem_blocks, &pred16x16L);

    // 8.3.1 Intra_4x4 prediction process for luma samples
    if (p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_4X4) {
        int32_t xO,yO,luma4x4BlkIdx;
        static hl_bool_t predLIs4x4SamplesOnlyTrue = HL_TRUE;

        // 8.3.1.1 Derivation process for Intra4x4PredMode
        _hl_codec_264_pred_intra_get_intra4x4_pred_mode(p_codec, p_mb);

        // 8.3.1.2 Intra_4x4 sample prediction
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
            // Gets the 13 neighbouring Samples
            err = hl_codec_264_pred_intra_get_neighbouring_samples_4x4L(p_codec, p_mb, luma4x4BlkIdx, &xO, &yO, p_codec->layers.pc_active->pc_fs_curr->p_pict->pc_data_y, (*p13));
            // Performs Intra4x4 prediction
            hl_codec_264_pred_intra_perform_prediction_4x4L(p_codec, p_mb, (*pred4x4L), (*p13), p_mb->Intra4x4PredMode[luma4x4BlkIdx]);
            // 8.5.1 Specification of transform decoding process for 4x4 luma residual blocks
            err = hl_codec_264_transf_decode_luma4x4(p_codec, p_mb, &luma4x4BlkIdx, 1, (const int32_t*)(*pred4x4L), 4, predLIs4x4SamplesOnlyTrue);
            if (err) {
                goto bail;
            }
        }

    }

    // 8.3.2 Intra_8x8 prediction process for luma samples
    else if (p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_8X8) {
        HL_DEBUG_ERROR("Intra8x8 prediction not supported yet");
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    }

    // 8.3.3 Intra_16x16 prediction process for luma samples
    else if (p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_16X16) {
        // Gets the 33 neighbouring Samples
        err = hl_codec_264_pred_intra_get_neighbouring_samples_16x16L(p_codec, p_mb, p_codec->layers.pc_active->pc_fs_curr->p_pict->pc_data_y, (*p33));
        // Performs Intra16x16 prediction
        hl_codec_264_pred_intra_perform_prediction_16x16L(p_codec, p_mb, (*pred16x16L), (*p33), p_mb->Intra16x16PredMode);
        // 8.5.2 Specification of transform decoding process for luma samples of Intra_16x16 macroblock prediction mode
        hl_codec_264_transf_decode_intra16x16_luma(p_codec, p_mb, (const int32_t(*)[16])(*pred16x16L));
    }

bail:
    // unmap() memory
    hl_memory_blocks_unmap(pc_mem_blocks, p13);
    hl_memory_blocks_unmap(pc_mem_blocks, p33);
    hl_memory_blocks_unmap(pc_mem_blocks, pred4x4L);
    hl_memory_blocks_unmap(pc_mem_blocks, pred16x16L);

    return err;
}

static HL_SHOULD_INLINE HL_ERROR_T _hl_codec_264_pred_intra_decode_chroma(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    if (p_mb->u_addr == 77) { // FIXME
        int a = 0;
    }

    // 8.3.4 Intra prediction process for chroma samples
    if (p_codec->sps.pc_active->ChromaArrayType == 1 || p_codec->sps.pc_active->ChromaArrayType == 2) {
        hl_memory_blocks_t* pc_mem_blocks;
        hl_int32_33_t *pCb33, *pCr33;
        hl_int32_16x16_t *predCb, *predCr;

        pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

        // map() memory
        hl_memory_blocks_map(pc_mem_blocks, &pCb33);
        hl_memory_blocks_map(pc_mem_blocks, &pCr33);
        hl_memory_blocks_map(pc_mem_blocks, &predCb);
        hl_memory_blocks_map(pc_mem_blocks, &predCr);

        // Get neighbouring chroma samples
        hl_codec_264_pred_intra_get_neighbouring_samples_C(
            p_codec,
            p_mb,
            p_codec->layers.pc_active->pc_fs_curr->p_pict->pc_data_u,
            p_codec->layers.pc_active->pc_fs_curr->p_pict->pc_data_v,
            (*pCb33),
            (*pCr33));
        // Performs IntraChroma prediction
        hl_codec_264_pred_intra_perform_prediction_chroma(
            p_codec,
            p_mb,
            (*predCb),
            (*predCr),
            (*pCb33),
            (*pCr33),
            p_mb->intra_chroma_pred_mode);

        // 8.5.4 Specification of transform decoding process for chroma samples
        hl_codec_264_transf_decode_chroma(p_codec, p_mb, (const int32_t(*)[16])(*predCb), 0); // U component
        hl_codec_264_transf_decode_chroma(p_codec, p_mb, (const int32_t(*)[16])(*predCr), 1); // V component


        // unmap() memory
        hl_memory_blocks_unmap(pc_mem_blocks, pCb33);
        hl_memory_blocks_unmap(pc_mem_blocks, pCr33);
        hl_memory_blocks_unmap(pc_mem_blocks, predCb);
        hl_memory_blocks_unmap(pc_mem_blocks, predCr);
    }
    else if (p_codec->sps.pc_active->ChromaArrayType == 3) {
        // subclause 8.3.4.5.
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return HL_ERROR_SUCCESS;
}

// 8.3.1.1 Derivation process for Intra4x4PredMode
static HL_SHOULD_INLINE HL_ERROR_T _hl_codec_264_pred_intra_get_intra4x4_pred_mode(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    int32_t dcPredModePredictedFlag;
    int32_t luma4x4BlkIdx, predIntra4x4PredMode;
    int32_t intraMxMPredModeA, intraMxMPredModeB;
    hl_codec_264_mb_t *mbA, *mbB;
    int32_t mbAddrA, luma4x4BlkIdxA, mbAddrB, luma4x4BlkIdxB;

    for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
        // 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
        mbAddrA = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_A;
        luma4x4BlkIdxA = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_A;
        mbAddrB = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_B;
        luma4x4BlkIdxB = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_B;

        mbA = HL_MATH_IS_POSITIVE_INT32(mbAddrA) ? p_codec->layers.pc_active->pp_list_macroblocks[mbAddrA] : HL_NULL;
        mbB = HL_MATH_IS_POSITIVE_INT32(mbAddrB) ? p_codec->layers.pc_active->pp_list_macroblocks[mbAddrB] : HL_NULL;
        //If any of the following conditions are true, dcPredModePredictedFlag is set equal to 1
        //	– the macroblock with address mbAddrA is not available
        //	– the macroblock with address mbAddrB is not available
        //	– the macroblock with address mbAddrA is available and coded in an Inter macroblock prediction
        //	mode and constrained_intra_pred_flag is equal to 1
        //	– the macroblock with address mbAddrB is available and coded in an Inter macroblock prediction
        //	mode and constrained_intra_pred_flag is equal to 1
        dcPredModePredictedFlag = 0;
        if (!mbA || !mbB || (p_codec->pps.pc_active->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(mbA) || HL_CODEC_264_MB_TYPE_IS_INTER(mbB)))) {
            dcPredModePredictedFlag = 1;
        }

        //A
        if (dcPredModePredictedFlag == 1 || (mbA && (mbA->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_4X4 && mbA->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_8X8))) {
            intraMxMPredModeA = 2;
        }
        else if (mbA) {
            if (mbA->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_4X4) {
                intraMxMPredModeA = mbA->Intra4x4PredMode[luma4x4BlkIdxA];
            }
            else if(mbA->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_8X8) {
                intraMxMPredModeA = mbA->Intra8x8PredMode[luma4x4BlkIdxA >> 2];
            }
        }
        //B
        if (dcPredModePredictedFlag == 1 || (mbB && (mbB->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_4X4 && mbB->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_8X8))) {
            intraMxMPredModeB = 2;
        }
        else if(mbB) {
            if(mbB->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_4X4) {
                intraMxMPredModeB = mbB->Intra4x4PredMode[luma4x4BlkIdxB];
            }
            else if(mbB->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_8X8) {
                intraMxMPredModeB = mbB->Intra8x8PredMode[luma4x4BlkIdxB >> 2];
            }
        }

        // (8-41)
        predIntra4x4PredMode = HL_MATH_MIN(intraMxMPredModeA, intraMxMPredModeB);
        if (p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
            p_mb->Intra4x4PredMode[luma4x4BlkIdx] = predIntra4x4PredMode;
        }
        else {
            if (p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] < predIntra4x4PredMode) {
                p_mb->Intra4x4PredMode[luma4x4BlkIdx] = p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx];
            }
            else {
                p_mb->Intra4x4PredMode[luma4x4BlkIdx] = p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] + 1;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// Predict IntraModes (fo encoding)
void hl_codec_264_pred_intra_set_intra4x4_pred_mode(hl_codec_264_t* p_codec, struct hl_codec_264_mb_s* p_mb)
{
    int32_t dcPredModePredictedFlag;
    int32_t luma4x4BlkIdx;
    HL_CODEC_264_I4x4_MODE_T predIntra4x4PredMode;
    int32_t intraMxMPredModeA, intraMxMPredModeB;
    hl_codec_264_mb_t *mbA, *mbB;
    int32_t mbAddrA, luma4x4BlkIdxA, mbAddrB, luma4x4BlkIdxB;

    for(luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
        // 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
        mbAddrA = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_A;
        luma4x4BlkIdxA = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_A;
        mbAddrB = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_B;
        luma4x4BlkIdxB = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_B;

        mbA = HL_MATH_IS_POSITIVE_INT32(mbAddrA) ? p_codec->layers.pc_active->pp_list_macroblocks[mbAddrA] : HL_NULL;
        mbB = HL_MATH_IS_POSITIVE_INT32(mbAddrB) ? p_codec->layers.pc_active->pp_list_macroblocks[mbAddrB] : HL_NULL;

        dcPredModePredictedFlag = 0;
        if(!mbA || !mbB || (p_codec->pps.pc_active->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(mbA) || HL_CODEC_264_MB_TYPE_IS_INTER(mbB)))) {
            dcPredModePredictedFlag = 1;
        }

        //A
        if(dcPredModePredictedFlag == 1 || (mbA && (mbA->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_4X4 && mbA->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_8X8))) {
            intraMxMPredModeA = Intra_4x4_DC;
        }
        else if(mbA) {
            if(mbA->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_4X4) {
                intraMxMPredModeA = mbA->Intra4x4PredMode[luma4x4BlkIdxA];
            }
            else if(mbA->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_8X8) {
                intraMxMPredModeA = mbA->Intra8x8PredMode[luma4x4BlkIdxA >> 2];
            }
        }
        //B
        if(dcPredModePredictedFlag == 1 || (mbB && (mbB->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_4X4 && mbB->MbPartPredMode[0] != HL_CODEC_264_MB_MODE_INTRA_8X8))) {
            intraMxMPredModeB = Intra_4x4_DC;
        }
        else if(mbB) {
            if(mbB->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_4X4) {
                intraMxMPredModeB = mbB->Intra4x4PredMode[luma4x4BlkIdxB];
            }
            else if(mbB->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_8X8) {
                intraMxMPredModeB = mbB->Intra8x8PredMode[luma4x4BlkIdxB >> 2];
            }
        }

        //probable prediction mode
        predIntra4x4PredMode = HL_MATH_MIN(intraMxMPredModeA, intraMxMPredModeB);

        if(predIntra4x4PredMode == p_mb->Intra4x4PredMode[luma4x4BlkIdx]) {
            p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]=1;
        }
        else {
            p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]=0;
            if(p_mb->Intra4x4PredMode[luma4x4BlkIdx] < predIntra4x4PredMode) {
                p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] = p_mb->Intra4x4PredMode[luma4x4BlkIdx];
            }
            else {
                p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] = p_mb->Intra4x4PredMode[luma4x4BlkIdx] - 1;
            }
        }
    }
}

// 8.3.1 Intra_4x4 prediction process for luma samples
HL_ERROR_T hl_codec_264_pred_intra_get_neighbouring_samples_4x4L(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t luma4x4BlkIdx, int32_t* xO, int32_t* yO, const hl_pixel_t *p_SL, int32_t p[13])
{
    int32_t i,mbAddrN,xN,yN,xW,yW,x,y,xM,yM;
    hl_bool_t p_x47_ym1_not_avail; //p[x,-1],with x=4...7
    const hl_codec_264_mb_t* mb;

    p_x47_ym1_not_avail = HL_TRUE;

    *xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
    *yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

    for (i = 0; i < 13; ++i) {
        x = INTRA_LUMA_NEIGHBOURING_SAMPLES4X4_X[i];
        y = INTRA_LUMA_NEIGHBOURING_SAMPLES4X4_Y[i];
        xN = *xO + x; // (8-42)
        yN = *yO + y; // (8-43)
        //6.4.11
        hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, xN, yN, &mbAddrN, &xW, &yW, HL_TRUE);
        mb = HL_MATH_IS_POSITIVE_INT32(mbAddrN) ? p_codec->layers.pc_active->pp_list_macroblocks[mbAddrN] : HL_NULL;
        if (!mb || (p_codec->pps.pc_active->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(mb) || mb->e_type == HL_CODEC_264_MB_TYPE_SI)) ||
                (x > 3 && (luma4x4BlkIdx == 3 || luma4x4BlkIdx == 11))) {
            p[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
        }
        else {
            if(p_x47_ym1_not_avail && (y == -1 && 4<=x && x<=7)) {
                p_x47_ym1_not_avail = HL_FALSE;
            }
            // 6.4.1 Inverse macroblock scanning process
            //hl_codec_264_utils_inverse_macroblock_scanning_process(p_codec, mbAddrN, &xM, &yM);
            xM = mb->xL;
            yM = mb->yL;
            if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag /*FIXME: && filed_macroblock*/) {
                // (8-44)
                HL_DEBUG_ERROR("MbAFF not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
            }
            else {
                p[i] = p_SL[(xM + xW) + ((yM + yW)*p_codec->layers.pc_active->pc_slice_hdr->PicWidthInSamplesL)];//(8-45)
            }
        }
    }
    if (p_x47_ym1_not_avail && p[8]/*p[3,-1]*/ != HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        //p[x,-1], with x=4..7
        p[9] = p[8];
        p[10] = p[8];
        p[11] = p[8];
        p[12] = p[8];
    }

    return HL_ERROR_SUCCESS;
}

// 8.3.3 Intra_16x16 prediction process for luma samples
HL_ERROR_T hl_codec_264_pred_intra_get_neighbouring_samples_16x16L(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, const hl_pixel_t *p_SL, int32_t p[33])
{
    int32_t i,x,y,xW,yW,mbAddrN,xM,yM;
    const hl_codec_264_mb_t* pc_mb;
    const hl_codec_264_layer_t *pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_pps_t* pc_pps = pc_layer->pc_slice_hdr->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;

    for (i=0; i<33; ++i) {
        x = INTRA_LUMA_NEIGHBOURING_SAMPLES16X16_X[i];
        y = INTRA_LUMA_NEIGHBOURING_SAMPLES16X16_Y[i];
        // 6.4.11
        hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, x, y, &mbAddrN, &xW, &yW, HL_TRUE);
        pc_mb = HL_MATH_IS_POSITIVE_INT32(mbAddrN) ? pc_layer->pp_list_macroblocks[mbAddrN] : HL_NULL;
        if (!pc_mb || (pc_pps->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(pc_mb) || pc_mb->e_type == HL_CODEC_264_MB_TYPE_SI))) {
            p[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
        }
        else {
            // 6.4.1 Inverse macroblock scanning process
            //hl_codec_264_utils_inverse_macroblock_scanning_process(p_codec, mbAddrN, &xM, &yM);
            xM = pc_mb->xL;
            yM = pc_mb->yL;
            if (pc_layer->pc_slice_hdr->MbaffFrameFlag /*FIXME: && filed_macroblock*/) {
                // (8-114)
                HL_DEBUG_ERROR("MbAFF not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
            }
            else {
                p[i] = p_SL[(xM + xW)+(yM + yW)*pc_layer->pc_slice_hdr->PicWidthInSamplesL]; //(8-115)
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// returns the number of samples in each chroma
hl_size_t hl_codec_264_pred_intra_get_neighbouring_samples_C(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, const hl_pixel_t *p_SU, const hl_pixel_t *p_SV, int32_t pCb[33], int32_t pCr[33])
{
    int32_t i,x,y,mbAddrN,xW,yW,xM,yM,xL,yL,p_size,p_pad;
    const hl_codec_264_mb_t* pc_mb;
    const hl_codec_264_layer_t *pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_sps_t* pc_sps = pc_layer->pc_slice_hdr->pc_pps->pc_sps;
    p_size = (pc_sps->MbHeightC + 1) + (pc_sps->MbWidthC);
    p_pad = (16-pc_sps->MbHeightC)+1;
    for (i=0; i<p_size; ++i) {
        //x = INTRA_CHROMA_NEIGHBOURING_SAMPLES_X[i+p_pad];
        //y = i>p_pad ? -1 : INTRA_CHROMA_NEIGHBOURING_SAMPLES_Y[i];
        x = INTRA_CHROMA_NEIGHBOURING_SAMPLES_X[i];
        y = INTRA_CHROMA_NEIGHBOURING_SAMPLES_Y[i];
        // 6.4.11
        /*ret = */hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, x, y, &mbAddrN, &xW, &yW, HL_FALSE);
        pc_mb = HL_MATH_IS_POSITIVE_INT32(mbAddrN) ? pc_layer->pp_list_macroblocks[mbAddrN] : HL_NULL;
        if (!pc_mb || (p_codec->pps.pc_active->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(pc_mb) || pc_mb->e_type == HL_CODEC_264_MB_TYPE_SI))) {
            pCb[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
            pCr[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
        }
        else {
            // 6.4.1 Inverse macroblock scanning process
            //hl_codec_264_utils_inverse_macroblock_scanning_process(p_codec, mbAddrN, &xL, &yL);
            xL = pc_mb->xL;
            yL = pc_mb->yL;
            xM = ((xL >> 4) * pc_sps->MbWidthC);// (8-128)
            yM = ((yL >> 4)* pc_sps->MbHeightC) + (yL & 1);// (8-129)

            if (pc_layer->pc_slice_hdr->MbaffFrameFlag /*FIXME: && filed_macroblock*/) {
                // (8-130)
                HL_DEBUG_ERROR("MbAFF not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
            }
            else {
                pCb[i] = p_SU[(xM+xW)+((yM+yW)*pc_layer->pc_slice_hdr->PicWidthInSamplesC)];// (8-131)
                pCr[i] = p_SV[(xM+xW)+((yM+yW)*pc_layer->pc_slice_hdr->PicWidthInSamplesC)];
            }
        }
    }

    return p_size;
}

void hl_codec_264_pred_intra_perform_prediction_4x4L(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t pred4x4L[4][4], const int32_t p[13], HL_CODEC_264_I4x4_MODE_T i4x4PredMode)
{
    switch(i4x4PredMode) {
    case Intra_4x4_Vertical:
        _Intra_4x4_Vertical(pred4x4L, p);
        break;
    case Intra_4x4_Horizontal:
        _Intra_4x4_Horizontal(pred4x4L, p);
        break;
    case Intra_4x4_DC:
        _Intra_4x4_DC(pred4x4L, p, p_codec);
        break;
    case Intra_4x4_Diagonal_Down_Left:
        _Intra_4x4_Diagonal_Down_Left(pred4x4L, p);
        break;
    case Intra_4x4_Diagonal_Down_Right:
        _Intra_4x4_Diagonal_Down_Right(pred4x4L, p);
        break;
    case Intra_4x4_Vertical_Right:
        _Intra_4x4_Vertical_Right(pred4x4L, p);
        break;
    case Intra_4x4_Horizontal_Down:
        _Intra_4x4_Horizontal_Down(pred4x4L, p);
        break;
    case Intra_4x4_Vertical_Left:
        _Intra_4x4_Vertical_Left(pred4x4L, p, p_codec);
        break;
    case Intra_4x4_Horizontal_Up:
        _Intra_4x4_Horizontal_Up(pred4x4L, p);
        break;
    }
}
void hl_codec_264_pred_intra_perform_prediction_16x16L(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t pred16x16L[16][16], const int32_t p[33], HL_CODEC_264_I16x16_MODE_T i16x16PredMode)
{
    switch(i16x16PredMode) {
    case Intra_16x16_Vertical: {
        _Intra_16x16_Vertical(pred16x16L, p);
        break;
    }
    case Intra_16x16_Horizontal: {
        _Intra_16x16_Horizontal(pred16x16L, p);
        break;
    }
    case Intra_16x16_DC: {
        _Intra_16x16_DC(pred16x16L, p, p_codec);
        break;
    }
    case Intra_16x16_Plane: {
        _Intra_16x16_Plane(pred16x16L, p, p_codec->sps.pc_active->BitDepthY);
        break;
    }
    }
}

void hl_codec_264_pred_intra_perform_prediction_chroma(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t predCb[16][16], int32_t predCr[16][16], const int32_t pCb[33], const int32_t pCr[33], HL_CODEC_264_INTRA_CHROMA_MODE_T iChomaPredMode)
{
    switch(iChomaPredMode) {
    case Intra_Chroma_DC: {
        _Intra_Chroma_DC(predCb, pCb, p_codec);
        _Intra_Chroma_DC(predCr, pCr, p_codec);
        break;
    }
    case Intra_Chroma_Horizontal: {
        _Intra_Chroma_Horizontal(predCb, pCb, p_codec);
        _Intra_Chroma_Horizontal(predCr, pCr, p_codec);
        break;
    }
    case Intra_Chroma_Vertical: {
        _Intra_Chroma_Vertical(predCb, pCb, p_codec);
        _Intra_Chroma_Vertical(predCr, pCr, p_codec);
        break;
    }
    case Intra_Chroma_Plane: {
        _Intra_Chroma_Plane(predCb, pCb, p_codec);
        _Intra_Chroma_Plane(predCr, pCr, p_codec);
        break;
    }
    }
}

// Predict 4x4 Intra modes
// Outputs: "prev_intra4x4_pred_mode_flag" and "rem_intra4x4_pred_mode"
void hl_codec_264_pred_intra_perform_prediction_modes_4x4(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    int32_t dcPredModePredictedFlag;
    int32_t luma4x4BlkIdx;
    HL_CODEC_264_I4x4_MODE_T predIntra4x4PredMode;
    HL_CODEC_264_I4x4_MODE_T intraMxMPredModeA, intraMxMPredModeB;
    const hl_codec_264_mb_t *pc_mbA, *pc_mbB;
    int32_t mbAddrA, luma4x4BlkIdxA, mbAddrB, luma4x4BlkIdxB;
    const hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->pc_slice_hdr;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
        // 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
        mbAddrA = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_A;
        luma4x4BlkIdxA = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_A;
        mbAddrB = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_B;
        luma4x4BlkIdxB = p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_B;

        pc_mbA = HL_MATH_IS_POSITIVE_INT32(mbAddrA) ? pc_layer->pp_list_macroblocks[mbAddrA] : HL_NULL;
        pc_mbB = HL_MATH_IS_POSITIVE_INT32(mbAddrB) ? pc_layer->pp_list_macroblocks[mbAddrB] : HL_NULL;

        dcPredModePredictedFlag = 0;
        if (!pc_mbA || !pc_mbB || (pc_pps->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(pc_mbA) || HL_CODEC_264_MB_TYPE_IS_INTER(pc_mbB)))) {
            dcPredModePredictedFlag = 1;
        }

        //== A ==//
        if ( dcPredModePredictedFlag == 1 || (pc_mbA && (!HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mbA, 0) && !HL_CODEC_264_MB_MODE_IS_INTRA_8X8(pc_mbA, 0)))) {
            intraMxMPredModeA = Intra_4x4_DC;
        }
        else if (pc_mbA) {
            if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mbA, 0)) {
                intraMxMPredModeA = pc_mbA->Intra4x4PredMode[luma4x4BlkIdxA];
            }
            else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(pc_mbA, 0)) {
                intraMxMPredModeA = pc_mbA->Intra8x8PredMode[luma4x4BlkIdxA >> 2];
            }
        }
        //== B ==//
        if (dcPredModePredictedFlag == 1 || (pc_mbB && (!HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mbB, 0) && !HL_CODEC_264_MB_MODE_IS_INTRA_8X8(pc_mbB, 0)))) {
            intraMxMPredModeB = Intra_4x4_DC;
        }
        else if (pc_mbB) {
            if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(pc_mbB, 0)) {
                intraMxMPredModeB = pc_mbB->Intra4x4PredMode[luma4x4BlkIdxB];
            }
            else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(pc_mbB, 0)) {
                intraMxMPredModeB = pc_mbB->Intra8x8PredMode[luma4x4BlkIdxB >> 2];
            }
        }

        //probable prediction mode
        predIntra4x4PredMode = HL_MATH_MIN(intraMxMPredModeA, intraMxMPredModeB);

        if (predIntra4x4PredMode == p_mb->Intra4x4PredMode[luma4x4BlkIdx]) {
            p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]=1;
        }
        else {
            p_mb->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]=0;
            if (p_mb->Intra4x4PredMode[luma4x4BlkIdx] < predIntra4x4PredMode) {
                p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] = p_mb->Intra4x4PredMode[luma4x4BlkIdx];
            }
            else {
                p_mb->rem_intra4x4_pred_mode[luma4x4BlkIdx] = p_mb->Intra4x4PredMode[luma4x4BlkIdx] - 1;
            }
        }
    }
}

// 8.3.1.2.1 Specification of Intra_4x4_Vertical prediction mode
static void _Intra_4x4_Vertical(int32_t pred4x4L[4][4], const int32_t p[13])
{
    //(8-46)
    pred4x4L[0][0] = pred4x4L[1][0] = pred4x4L[2][0] = pred4x4L[3][0] = p[5];
    pred4x4L[0][1] = pred4x4L[1][1] = pred4x4L[2][1] = pred4x4L[3][1] = p[6];
    pred4x4L[0][2] = pred4x4L[1][2] = pred4x4L[2][2] = pred4x4L[3][2] = p[7];
    pred4x4L[0][3] = pred4x4L[1][3] = pred4x4L[2][3] = pred4x4L[3][3] = p[8];
}

// 8.3.1.2.2 Specification of _Intra_4x4_Horizontal prediction mode
static void _Intra_4x4_Horizontal(int32_t pred4x4L[4][4], const int32_t p[13])
{
    int32_t r,y;
    for(y=0; y<4; ++y) {
        r = hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y);//(8-47)
        pred4x4L[y][0] = r;
        pred4x4L[y][1] = r;
        pred4x4L[y][2] = r;
        pred4x4L[y][3] = r;
    }
}

// 8.3.1.2.3 Specification of _Intra_4x4_DC prediction mode
static void _Intra_4x4_DC(int32_t pred4x4L[4][4], const int32_t p[13], const hl_codec_264_t* p_codec)
{
    int32_t r;
    hl_bool_t x_avail = HL_TRUE, y_avail = HL_TRUE;

    if (p[5] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        x_avail = HL_FALSE;
    }
    else if (p[6] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        x_avail = HL_FALSE;
    }
    else if (p[7] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        x_avail = HL_FALSE;
    }
    else if (p[8] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        x_avail = HL_FALSE;
    }

    if (p[1] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        y_avail = HL_FALSE;
    }
    else if (p[2] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        y_avail = HL_FALSE;
    }
    else if (p[3] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        y_avail = HL_FALSE;
    }
    else if (p[4] == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
        y_avail = HL_FALSE;
    }

    if (x_avail && y_avail) {
        r = (p[5]+p[6]+p[7]+p[8] + p[1]+p[2]+p[3]+p[4]+4)>>3;
    }
    else if (!x_avail && y_avail) {
        r = (p[1]+p[2]+p[3]+p[4]+2)>>2;
    }
    else if (!y_avail && x_avail) {
        r = (p[5]+p[6]+p[7]+p[8]+2)>>2;
    }
    else {
        r = (1 << (p_codec->sps.pc_active->BitDepthY - 1));
    }

    pred4x4L[0][0]=r;
    pred4x4L[0][1]=r;
    pred4x4L[0][2]=r;
    pred4x4L[0][3]=r;
    pred4x4L[1][0]=r;
    pred4x4L[1][1]=r;
    pred4x4L[1][2]=r;
    pred4x4L[1][3]=r;
    pred4x4L[2][0]=r;
    pred4x4L[2][1]=r;
    pred4x4L[2][2]=r;
    pred4x4L[2][3]=r;
    pred4x4L[3][0]=r;
    pred4x4L[3][1]=r;
    pred4x4L[3][2]=r;
    pred4x4L[3][3]=r;

}

// 8.3.1.2.4 Specification of _Intra_4x4_Diagonal_Down_Left prediction mode
static void _Intra_4x4_Diagonal_Down_Left(int32_t pred4x4L[4][4], const int32_t p[13])
{
    int32_t x;
    for (x=0; x<4; ++x) {
        pred4x4L[0][x] = (p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 0, -1)] + 2 * p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 0 + 1, -1)] + p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 0 + 2, -1)] + 2) >> 2;
        pred4x4L[1][x] = (p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 1, -1)] + 2 * p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 1 + 1, -1)] + p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 1 + 2, -1)] + 2) >> 2;
        pred4x4L[2][x] = (p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 2, -1)] + 2 * p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 2 + 1, -1)] + p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 2 + 2, -1)] + 2) >> 2;
        pred4x4L[3][x] = (p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 3, -1)] + 2 * p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 3 + 1, -1)] + p[hl_codec_264_intra_luma_neighbouring_samples4x4_i(x + 3 + 2, -1)] + 2) >> 2;
    }

    // x=3 and y=3
    pred4x4L[3][3] = (p[11] + 3 * p[12] + 2) >> 2;
}

// 8.3.1.2.5 Specification of _Intra_4x4_Diagonal_Down_Right prediction mode
static void _Intra_4x4_Diagonal_Down_Right(int32_t pred4x4L[4][4], const int32_t p[13])
{
    int32_t x,y;
    for (y=0; y<4; ++y) {
        for (x=0; x<4; ++x) {
            if( x>y) {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - y - 2, -1) + (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - y - 1, -1) << 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - y, -1) + 2) >> 2;
            }
            else if (x < y) {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - x - 2) + (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - x - 1) << 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - x) + 2) >> 2;
            }
            else {
                pred4x4L[y][x] = (p[5] + (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, -1) << 1) + p[1] + 2) >> 2;
            }
        }
    }
}

// 8.3.1.2.6 Specification of _Intra_4x4_Vertical_Right prediction mode
static void _Intra_4x4_Vertical_Right(int32_t pred4x4L[4][4], const int32_t p[13])
{
    int32_t x,y,zVR;
    for (y=0; y<4; ++y) {
        for (x=0; x<4; ++x) {
            zVR=(x<<1)-y;
            switch(zVR) {
            case 0:
            case 2:
            case 4:
            case 6: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - (y >> 1) - 1, -1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - (y >> 1), -1) + 1) >> 1;
                break;
            }
            case 1:
            case 3:
            case 5: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - (y >> 1) - 2, -1) + (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - (y >> 1) - 1, -1) << 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - (y >> 1), -1) + 2) >> 2;
                break;
            }
            case -1: {
                pred4x4L[y][x] = (p[1] + (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, -1) << 1) + p[5] + 2) >> 2;
                break;
            }
            case -2:
            case -3:
            default: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - 1) + (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - 2) << 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - 3) + 2) >> 2;
                break;
            }
            }
        }
    }
}

// 8.3.1.2.7 Specification of _Intra_4x4_Horizontal_Down prediction mode
static void _Intra_4x4_Horizontal_Down(int32_t pred4x4L[4][4], const int32_t p[13])
{
    int32_t x,y,zHD;
    for(y=0; y<4; ++y) {
        for(x=0; x<4; ++x) {
            zHD = (y<<1)-x;
            switch(zHD) {
            case 0:
            case 2:
            case 4:
            case 6: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - (x >> 1) - 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - (x >> 1)) + 1) >> 1;
                break;
            }
            case 1:
            case 3:
            case 5: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - (x >> 1) - 2) + 2 * hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - (x >> 1) - 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y - (x >> 1)) + 2) >> 2;
                break;
            }
            case -1: {
                pred4x4L[y][x] = (p[1] + 2 * hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, -1) + p[5] + 2) >> 2;
                break;
            }
            case -2:
            case -3:
            default: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - 1, -1) + 2 * hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - 2, -1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x - 3, -1) + 2) >> 2;
                break;
            }
            }
        }
    }
}

// 8.3.1.2.8 Specification of _Intra_4x4_Vertical_Left prediction mode
static void _Intra_4x4_Vertical_Left(int32_t pred4x4L[4][4], const int32_t p[13], const hl_codec_264_t* p_codec)
{
    int32_t x;
    for(x=0; x<4; ++x) {
        pred4x4L[0][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x, -1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1, -1) + 1) >> 1;
        pred4x4L[2][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1, -1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1 + 1, -1) + 1) >> 1;

        pred4x4L[1][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x, -1) + 2 * hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1, -1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 2, -1) + 2) >> 2;
        pred4x4L[3][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1, -1) + 2 * hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1 + 1, -1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, x + 1 + 2, -1) + 2) >> 2;
    }
}

// 8.3.1.2.9 Specification of _Intra_4x4_Horizontal_Up prediction mode
static void _Intra_4x4_Horizontal_Up(int32_t pred4x4L[4][4], const int32_t p[13])
{
    int32_t x,y,zHU;
    for (x=0; x<4; ++x) {
        for (y=0; y<4; ++y) {
            zHU = x+(y<<1);
            switch(zHU) {
            case 0:
            case 2 :
            case 4: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y + (x >> 1)) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y + (x >> 1) + 1) + 1) >> 1;
                break;
            }
            case 1:
            case 3: {
                pred4x4L[y][x] = (hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y + (x >> 1)) + 2 * hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y + (x >> 1) + 1) + hl_codec_264_intra_luma_neighbouring_samples4x4_p(p, -1, y + (x >> 1) + 2) + 2) >> 2;
                break;
            }
            case 5: {
                pred4x4L[y][x] = (p[3] + 3 * p[4] + 2) >> 2;
                break;
            }
            default: {
                pred4x4L[y][x] = p[4];
                break;
            }
            }
        }
    }
}

// 8.3.3.1 Specification of _Intra_16x16_Vertical prediction mode
static void _Intra_16x16_Vertical(int32_t pred16x16L[16][16], const int32_t p[33])
{
    const int32_t* px = &p[17];
    int32_t r, x;
    for (x = 0; x<16; ++x) {
        r = px[x];//(8-116)
        pred16x16L[0][x] = r;
        pred16x16L[1][x] = r;
        pred16x16L[2][x] = r;
        pred16x16L[3][x] = r;
        pred16x16L[4][x] = r;
        pred16x16L[5][x] = r;
        pred16x16L[6][x] = r;
        pred16x16L[7][x] = r;
        pred16x16L[8][x] = r;
        pred16x16L[9][x] = r;
        pred16x16L[10][x] = r;
        pred16x16L[11][x] = r;
        pred16x16L[12][x] = r;
        pred16x16L[13][x] = r;
        pred16x16L[14][x] = r;
        pred16x16L[15][x] = r;
    }
}

// 8.3.3.2 Specification of _Intra_16x16_Horizontal prediction mode
static void _Intra_16x16_Horizontal(int32_t pred16x16L[16][16], const int32_t p[33])
{
    const int32_t *py = &p[1];
    int32_t y;
    int32_t r;
    for(y=0; y<16; ++y) {
        r = py[y];//(8-117)
        pred16x16L[y][0] = r;
        pred16x16L[y][1] = r;
        pred16x16L[y][2] = r;
        pred16x16L[y][3] = r;
        pred16x16L[y][4] = r;
        pred16x16L[y][5] = r;
        pred16x16L[y][6] = r;
        pred16x16L[y][7] = r;
        pred16x16L[y][8] = r;
        pred16x16L[y][9] = r;
        pred16x16L[y][10] = r;
        pred16x16L[y][11] = r;
        pred16x16L[y][12] = r;
        pred16x16L[y][13] = r;
        pred16x16L[y][14] = r;
        pred16x16L[y][15] = r;
    }
}

// 8.3.3.3 Specification of _Intra_16x16_DC prediction mode
static void _Intra_16x16_DC(int32_t pred16x16L[16][16], const int32_t p[33], const hl_codec_264_t* p_codec)
{
    hl_bool_t x_all_avail = HL_TRUE, y_all_avail = HL_TRUE;
    int32_t y, r, x_sum = 0, y_sum = 0;
    const int32_t *px = &p[17], *py = &p[1];

    int32_t x, tmp;

    for(x = 0; x<16; x+=4) {
        if((tmp = px[x]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            x_all_avail = HL_FALSE;
            break;
        }
        x_sum += tmp;
        if((tmp = px[x + 1]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            x_all_avail = HL_FALSE;
            break;
        }
        x_sum += tmp;
        if((tmp = px[x + 2]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            x_all_avail = HL_FALSE;
            break;
        }
        x_sum += tmp;
        if((tmp = px[x + 3]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            x_all_avail = HL_FALSE;
            break;
        }
        x_sum += tmp;
    }

    for(y = 0; y<16; y += 4) {
        if((tmp = py[y]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            y_all_avail = HL_FALSE;
            break;
        }
        y_sum += tmp;
        if((tmp = py[y + 1]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            y_all_avail = HL_FALSE;
            break;
        }
        y_sum += tmp;
        if((tmp = py[y + 2]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            y_all_avail = HL_FALSE;
            break;
        }
        y_sum += tmp;
        if((tmp = py[y + 3]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
            y_all_avail = HL_FALSE;
            break;
        }
        y_sum += tmp;
    }

    if(x_all_avail && y_all_avail) {
        r = (x_sum + y_sum + 16)>>5;
    }
    else if(!x_all_avail && y_all_avail) {
        r = (y_sum + 8)>>4;
    }
    else if(!y_all_avail && x_all_avail) {
        r = (x_sum + 8)>>4;
    }
    else {
        r = (1 << (p_codec->sps.pc_active->BitDepthY-1));
    }

    for(y=0; y<16; ++y) {
        pred16x16L[y][0]=r;
        pred16x16L[y][1]=r;
        pred16x16L[y][2]=r;
        pred16x16L[y][3]=r;
        pred16x16L[y][4]=r;
        pred16x16L[y][5]=r;
        pred16x16L[y][6]=r;
        pred16x16L[y][7]=r;
        pred16x16L[y][8]=r;
        pred16x16L[y][9]=r;
        pred16x16L[y][10]=r;
        pred16x16L[y][11]=r;
        pred16x16L[y][12]=r;
        pred16x16L[y][13]=r;
        pred16x16L[y][14]=r;
        pred16x16L[y][15]=r;
    }
}

// 8.3.3.4 Specification of _Intra_16x16_Plane prediction mode
static void _Intra_16x16_Plane(int32_t pred16x16L[16][16], const int32_t p[33], int32_t BitDepthY)
{
    int32_t a,b,c,y,H=0,V=0;

    H += (1)*(p[25]-p[23]);
    H += (2)*(p[26]-p[22]);
    H += (3)*(p[27]-p[21]);
    H += (4)*(p[28]-p[20]);
    H += (5)*(p[29]-p[19]);
    H += (6)*(p[30]-p[18]);
    H += (7)*(p[31]-p[17]);
    H += (8)*(p[32]-p[0]);

    V += (1)*(p[9]-p[7]);
    V += (2)*(p[10]-p[6]);
    V += (3)*(p[11]-p[5]);
    V += (4)*(p[12]-p[4]);
    V += (5)*(p[13]-p[3]);
    V += (6)*(p[14]-p[2]);
    V += (7)*(p[15]-p[1]);
    V += (8)*(p[16]-p[0]);

    a = (p[16] + p[32])<<4;
    b = (5*H+32)>>6;
    c = (5*V+32)>>6;

    for (y=0; y<16; ++y) {
        pred16x16L[y][0]=HL_MATH_CLIP1Y(((a + b * -7 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][1]=HL_MATH_CLIP1Y(((a + b * -6 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][2]=HL_MATH_CLIP1Y(((a + b * -5 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][3]=HL_MATH_CLIP1Y(((a + b * -4 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][4]=HL_MATH_CLIP1Y(((a + b * -3 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][5]=HL_MATH_CLIP1Y(((a + b * -2 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][6]=HL_MATH_CLIP1Y(((a + b * -1 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][7]=HL_MATH_CLIP1Y(((a + b * 0 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][8]=HL_MATH_CLIP1Y(((a + b * 1 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][9]=HL_MATH_CLIP1Y(((a + b * 2 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][10]=HL_MATH_CLIP1Y(((a + b * 3 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][11]=HL_MATH_CLIP1Y(((a + b * 4 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][12]=HL_MATH_CLIP1Y(((a + b * 5 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][13]=HL_MATH_CLIP1Y(((a + b * 6 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][14]=HL_MATH_CLIP1Y(((a + b * 7 + c * (y - 7) + 16) >> 5), BitDepthY);
        pred16x16L[y][15]=HL_MATH_CLIP1Y(((a + b * 8 + c * (y - 7) + 16) >> 5), BitDepthY);
    }
}

// 8.3.4.1 Specification of _Intra_Chroma_DC prediction mode
static void _Intra_Chroma_DC(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec)
{
    hl_bool_t x_all_avail = HL_TRUE, y_all_avail = HL_TRUE;
    int32_t chroma4x4BlkIdx, chroma4x4BlkSize, xO, yO, x_sum, y_sum, tmp, y;
    const int32_t *px = &p[p_codec->sps.pc_active->MbWidthC + 1];
    const int32_t *py = &p[1];
    const int32_t default_tmp = (1 << (p_codec->sps.pc_active->BitDepthC - 1));

    // FIXME: SSE4

    chroma4x4BlkSize = (1 << (p_codec->sps.pc_active->ChromaArrayType + 1)) - 1;

    for(chroma4x4BlkIdx = 0; chroma4x4BlkIdx <= chroma4x4BlkSize; ++chroma4x4BlkIdx) {
        xO = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][0];// (8-132)
        yO = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][1];// (8-133)

        x_sum = y_sum = 0;

        do {
            if ((tmp = px[xO]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                x_all_avail = HL_FALSE;
                break;
            }
            x_sum += tmp;
            if ((tmp = px[xO + 1]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                x_all_avail = HL_FALSE;
                break;
            }
            x_sum += tmp;
            if ((tmp = px[xO + 2]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                x_all_avail = HL_FALSE;
                break;
            }
            x_sum += tmp;
            if ((tmp = px[xO + 3]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                x_all_avail = HL_FALSE;
                break;
            }
            x_sum += tmp;
        }
        while (0);

        do {
            if ((tmp = py[yO]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                y_all_avail = HL_FALSE;
                break;
            }
            y_sum += tmp;
            if ((tmp = py[yO + 1]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                y_all_avail = HL_FALSE;
                break;
            }
            y_sum += tmp;
            if ((tmp = py[yO + 2]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                y_all_avail = HL_FALSE;
                break;
            }
            y_sum += tmp;
            if ((tmp = py[yO + 3]) == HL_CODEC_264_SAMPLE_NOT_AVAIL) {
                y_all_avail = HL_FALSE;
                break;
            }
            y_sum += tmp;
        }
        while(0);

        if ((xO == 0 && yO == 0) || (xO>0 && yO>0)) {
            if (x_all_avail && y_all_avail) {
                tmp = (x_sum + y_sum + 4)>>3;
            }
            else if (!x_all_avail && y_all_avail) {
                tmp = (y_sum + 2)>>2;
            }
            else if (!y_all_avail && x_all_avail) {
                tmp = (x_sum + 2)>>2;
            }
            else {
                tmp = default_tmp;
            }
        }
        else if (xO>0 && yO==0) {
            if (x_all_avail) {
                tmp = (x_sum + 2)>>2;
            }
            else if (y_all_avail) {
                tmp = (y_sum + 2)>>2;
            }
            else {
                tmp = default_tmp;
            }
        }
        else if (xO==0 && yO>0) {
            if (y_all_avail) {
                tmp = (y_sum + 2)>>2;
            }
            else if (x_all_avail) {
                tmp = (x_sum + 2)>>2;
            }
            else {
                tmp = default_tmp;
            }
        }

        for (y=0; y<4; ++y) {
            predC[y+yO][xO]=tmp;
            predC[y+yO][xO+1]=tmp;
            predC[y+yO][xO+2]=tmp;
            predC[y+yO][xO+3]=tmp;
        }
    }

}

// 8.3.4.2 Specification of _Intra_Chroma_Horizontal prediction mode
static void _Intra_Chroma_Horizontal(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec)
{
    const int32_t *py = &p[1];
    uint32_t x,y;
    int32_t r;

    for (y=0; y<p_codec->sps.pc_active->MbHeightC; ++y) {
        r = py[y]; // (8-144)
        for (x=0; x<p_codec->sps.pc_active->MbWidthC; x+=4) {
            predC[y][x] = r;
            predC[y][x+1] = r;
            predC[y][x+2] = r;
            predC[y][x+3] = r;
        }
    }
}

// 8.3.4.3 Specification of _Intra_Chroma_Vertical prediction mode
static void _Intra_Chroma_Vertical(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec)
{
    const int32_t *px = &p[p_codec->sps.pc_active->MbWidthC + 1];
    uint32_t x,y;
    for (y = 0; y < p_codec->sps.pc_active->MbHeightC; ++y) {
        for (x = 0; x< p_codec->sps.pc_active->MbWidthC; x+=4) {
            predC[y][x] = px[x];
            predC[y][x + 1] = px[x + 1];
            predC[y][x + 2] = px[x + 2];
            predC[y][x + 3] = px[x + 3];
        }
    }
}

// 8.3.4.4 Specification of _Intra_Chroma_Plane prediction mode
static void _Intra_Chroma_Plane(int32_t predC[16][16], const int32_t *p, const hl_codec_264_t* p_codec)
{
    int32_t a, b, c, H, V;
    uint32_t x, y;

    // TODO: "xCF" and "yCF" always equal to zero because only "ChromaArrayType=1" is supported for now

    H = 1 * (p[13] - p[11]);
    H += 2 * (p[14] - p[10]);
    H += 3 * (p[15] - p[9]);
    H += 4 * (p[16] - p[0]);

    V = 1 * (p[5] - p[3]);
    V += 2 * (p[6] - p[2]);
    V += 3 * (p[7] - p[1]);
    V += 4 * (p[8] - p[0]);

    a = (p[8] + p[16]) << 4;
    b = ((34 - 29 * (p_codec->sps.pc_active->ChromaArrayType == 3)) * H + 32) >> 6;
    c = ((34 - 29 * (p_codec->sps.pc_active->ChromaArrayType != 1)) * V + 32) >> 6;

    for (y=0; y<p_codec->sps.pc_active->MbHeightC; ++y) {
        for (x=0; x<p_codec->sps.pc_active->MbWidthC; x+=4) {
            predC[y][x] = HL_MATH_CLIP1C((int32_t)((a + b * (x - 3) + c * (y - 3) + 16) >> 5), p_codec->sps.pc_active->BitDepthC);
            predC[y][x + 1] = HL_MATH_CLIP1C((int32_t)((a + b * (x - 2) + c * (y - 3) + 16) >> 5), p_codec->sps.pc_active->BitDepthC);
            predC[y][x + 2] = HL_MATH_CLIP1C((int32_t)((a + b * (x - 1) + c * (y - 3) + 16) >> 5), p_codec->sps.pc_active->BitDepthC);
            predC[y][x + 3] = HL_MATH_CLIP1C((int32_t)((a + b * (x) + c * (y - 3) + 16) >> 5), p_codec->sps.pc_active->BitDepthC);
        }
    }
}
