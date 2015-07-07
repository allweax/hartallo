#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_debug.h"

HL_ERROR_T hl_codec_264_utils_guess_level(hl_size_t width, hl_size_t height, enum HL_CODEC_264_LEVEL_E* p_level)
{
    typedef struct level_size_xs {
        HL_CODEC_264_LEVEL_T l;
        hl_size_t w;
        hl_size_t h;
    }
    level_size_xt;

    static const level_size_xt __level_sizes [] = {
        {HL_CODEC_264_LEVEL_10, 128, 96},
        {HL_CODEC_264_LEVEL_1B, 128, 96},
        {HL_CODEC_264_LEVEL_11, 176, 144},
        {HL_CODEC_264_LEVEL_12, 320, 240},
        {HL_CODEC_264_LEVEL_13, 352, 288},
        {HL_CODEC_264_LEVEL_20, 352, 288},
        {HL_CODEC_264_LEVEL_21, 352, 480},
        {HL_CODEC_264_LEVEL_22, 352, 480},
        {HL_CODEC_264_LEVEL_30, 720, 480},
        {HL_CODEC_264_LEVEL_31, 1280, 720},
        {HL_CODEC_264_LEVEL_32, 1280, 720},
        {HL_CODEC_264_LEVEL_40, 2048, 1024},
        {HL_CODEC_264_LEVEL_41, 2048, 1024},
        {HL_CODEC_264_LEVEL_42, 2048, 1080},
        {HL_CODEC_264_LEVEL_50, 2560, 1920},
        {HL_CODEC_264_LEVEL_51, 3840, 2160},
    };
    static const hl_size_t __level_count = sizeof(__level_sizes)/sizeof(__level_sizes[0]);
    hl_size_t u;

    if (!width || !height || !p_level) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    for (u = 0; u < __level_count; ++u) {
        if (__level_sizes[u].w >= width && __level_sizes[u].h >= height) {
            *p_level = __level_sizes[u].l;
            return HL_ERROR_SUCCESS;
        }
    }

    *p_level = HL_CODEC_264_LEVEL_51;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_utils_init_mb_current_avc(hl_codec_264_t* p_codec, uint32_t u_mb_addr, long l_slice_id, hl_bool_t b_pskip)
{
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    hl_codec_264_mb_t *p_mb, *_p_mb;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    if (!p_codec || u_mb_addr >= p_codec->layers.pc_active->u_list_macroblocks_count) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    pc_slice_header = p_codec->layers.pc_active->pc_slice_hdr;
    p_mb = p_codec->layers.pc_active->pp_list_macroblocks[u_mb_addr];

#if 0
    {
        // FIXME
        //static int32_t sizeOfObject = sizeof(struct hl_object_s);
        //static int32_t sizeOfMbMinusObject = sizeof(struct hl264Mb_s) - sizeof(struct hl_codec_264_mb_s);

        memset(((uint8_t*)p_mb) + 8, 0, sizeof(struct hl_codec_264_mb_s)-8);

        memset(p_mb->ChromaDCLevel, 0, sizeof(p_mb->ChromaDCLevel));
        memset(p_mb->ChromaACLevel, 0, sizeof(p_mb->ChromaACLevel));
        memset(p_mb->Intra16x16DCLevel, 0, sizeof(p_mb->Intra16x16DCLevel));
        memset(p_mb->Intra16x16ACLevel, 0, sizeof(p_mb->Intra16x16ACLevel));
        memset(p_mb->LumaLevel, 0, sizeof(p_mb->LumaLevel));
        memset(p_mb->LumaLevel8x8, 0, sizeof(p_mb->LumaLevel8x8));
    }
#endif

    p_mb->u_addr = u_mb_addr;
    p_mb->l_slice_id = l_slice_id;
    p_mb->mb_qp_delta = 0;
    p_mb->ext.svc.base_mode_flag = 0;
    p_mb->CodedBlockPatternLuma4x4 = p_mb->CodedBlockPatternLuma4x4 = 0;
    p_mb->CodedBlockPatternChromaAC4x4[0] = p_mb->CodedBlockPatternChromaAC4x4[1] = 0;
    p_mb->CodedBlockPatternChromaDC4x4[0] = p_mb->CodedBlockPatternChromaDC4x4[1] = 0;
    if (b_pskip) {
        // Set "mb_type" now because neither "hl_codec_264_mb_set_mb_type()" nor "hl_codec_264_mb_set_sub_mb_type()" will be called
        if (IsSliceHeaderB(pc_slice_header)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_B_SKIP;
            p_mb->flags_type = (HL_CODEC_264_MB_TYPE_FLAGS_INTER_B | HL_CODEC_264_MB_TYPE_FLAGS_SKIP);
            p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_DIRECT;
        }
        else {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_P_SKIP;
            p_mb->flags_type = (HL_CODEC_264_MB_TYPE_FLAGS_INTER_P | HL_CODEC_264_MB_TYPE_FLAGS_SKIP);
            p_mb->MbPartPredMode[0] = HL_CODEC_264_MB_MODE_PRED_L0;
        }
        p_mb->MbPartPredMode[1] = HL_CODEC_264_NA;
        p_mb->NumMbPart = 1;
        p_mb->NumSubMbPart[0] = 1;
        p_mb->MbPartWidth = p_mb->SubMbPartWidth[0] = p_mb->partWidth[0][0] = 16;
        p_mb->MbPartHeight = p_mb->SubMbPartHeight[0] = p_mb->partHeight[0][0] = 16;
        p_mb->partWidthC[0][0] = p_mb->partWidth[0][0] >> p_codec->sps.pc_active->SubWidthC_TrailingZeros;
        p_mb->partHeightC[0][0] = p_mb->partHeight[0][0] >> p_codec->sps.pc_active->SubHeightC_TrailingZeros;
    }

    if (pc_slice_header->MbaffFrameFlag) {
        p_mb->u_x = (u_mb_addr >> 1) % pc_slice_header->PicWidthInMbs;
        p_mb->u_y =  (((u_mb_addr / pc_slice_header->PicWidthInMbs) >> 1) << 1);
    }
    else {
        p_mb->u_x = u_mb_addr % pc_slice_header->PicWidthInMbs;
        p_mb->u_y = u_mb_addr / p_codec->sps.pc_active->uPicWidthInMbs;
    }

    //
    // 6.4.7 Derivation process of the availability for macroblock addresses
    //
    p_mb->neighbours.i_addr_A = u_mb_addr - 1;
    p_mb->neighbours.i_addr_B = u_mb_addr - p_codec->sps.pc_active->uPicWidthInMbs;
    p_mb->neighbours.i_addr_C = u_mb_addr - p_codec->sps.pc_active->uPicWidthInMbs + 1;
    p_mb->neighbours.i_addr_D = u_mb_addr - p_codec->sps.pc_active->uPicWidthInMbs - 1;

    p_mb->neighbours.b_avail_A = p_mb->neighbours.b_avail_B = p_mb->neighbours.b_avail_C = p_mb->neighbours.b_avail_D = HL_FALSE;
    if (p_mb->u_x) {
        p_mb->neighbours.b_avail_A = (_p_mb = p_codec->layers.pc_active->pp_list_macroblocks[p_mb->neighbours.i_addr_A]) && _p_mb->l_slice_id == p_mb->l_slice_id;
        if (p_mb->u_y) {
            p_mb->neighbours.b_avail_D = (_p_mb = p_codec->layers.pc_active->pp_list_macroblocks[p_mb->neighbours.i_addr_D]) && _p_mb->l_slice_id == p_mb->l_slice_id;
        }
    }
    if (p_mb->u_y) {
        p_mb->neighbours.b_avail_B = (_p_mb = p_codec->layers.pc_active->pp_list_macroblocks[p_mb->neighbours.i_addr_B]) && _p_mb->l_slice_id == p_mb->l_slice_id;
        if (p_mb->u_x < (p_codec->sps.pc_active->uPicWidthInMbs - 1)) {
            p_mb->neighbours.b_avail_C = (_p_mb = p_codec->layers.pc_active->pp_list_macroblocks[p_mb->neighbours.i_addr_C]) && _p_mb->l_slice_id == p_mb->l_slice_id;
        }
    }
    if (!p_mb->neighbours.b_avail_A) {
        p_mb->neighbours.i_addr_A = HL_CODEC_264_MB_ADDR_NOT_AVAIL;
    }
    if (!p_mb->neighbours.b_avail_B) {
        p_mb->neighbours.i_addr_B = HL_CODEC_264_MB_ADDR_NOT_AVAIL;
    }
    if (!p_mb->neighbours.b_avail_C) {
        p_mb->neighbours.i_addr_C = HL_CODEC_264_MB_ADDR_NOT_AVAIL;
    }
    if (!p_mb->neighbours.b_avail_D) {
        p_mb->neighbours.i_addr_D = HL_CODEC_264_MB_ADDR_NOT_AVAIL;
    }

    //
    // 6.4.1 Inverse macroblock scanning process
    //
    hl_codec_264_utils_inverse_macroblock_scanning_process(p_codec, u_mb_addr, &p_mb->xL, &p_mb->yL);
    p_mb->xC = p_mb->xL >> p_codec->sps.pc_active->SubWidthC_TrailingZeros;
    p_mb->yC = p_mb->yL >> p_codec->sps.pc_active->SubHeightC_TrailingZeros;

    //
    // 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
    // 6.4.10.5 Derivation process for neighbouring 4x4 chroma blocks
    //
    if (!b_pskip) {
        // 6.4.10.5 Derivation process for neighbouring 4x4 chroma blocks
#define HL_MB_SET_NBC4x4(chroma4x4BlkIdx) \
			err = hl_codec_264_utils_derivation_process_for_neighbouring_4x4_chroma_blocks(p_codec, p_mb, chroma4x4BlkIdx,  \
					&p_mb->neighbouringChromaBlock4x4[chroma4x4BlkIdx].i_addr_A,  \
					&p_mb->neighbouringChromaBlock4x4[chroma4x4BlkIdx].i_blk_idx_A,  \
					&p_mb->neighbouringChromaBlock4x4[chroma4x4BlkIdx].i_addr_B,  \
					&p_mb->neighbouringChromaBlock4x4[chroma4x4BlkIdx].i_blk_idx_B); \
					if (err) return err;

        HL_MB_SET_NBC4x4(0);
        HL_MB_SET_NBC4x4(1);
        HL_MB_SET_NBC4x4(2);
        HL_MB_SET_NBC4x4(3);
        if (p_codec->sps.pc_active->ChromaArrayType == 3) {
            HL_MB_SET_NBC4x4(4);
            HL_MB_SET_NBC4x4(5);
            HL_MB_SET_NBC4x4(6);
            HL_MB_SET_NBC4x4(7);
            HL_MB_SET_NBC4x4(8);
            HL_MB_SET_NBC4x4(9);
            HL_MB_SET_NBC4x4(10);
            HL_MB_SET_NBC4x4(11);
            HL_MB_SET_NBC4x4(12);
            HL_MB_SET_NBC4x4(13);
            HL_MB_SET_NBC4x4(14);
            HL_MB_SET_NBC4x4(15);
        }

        // 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
#define HL_MB_SET_NBL4x4(luma4x4BlkIdx) \
			err = hl_codec_264_utils_derivation_process_for_neighbouring_4x4_luma_blocks(p_codec, p_mb, luma4x4BlkIdx,  \
					&p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_A,  \
					&p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_A,  \
					&p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_addr_B,  \
					&p_mb->neighbouringLumaBlock4x4[luma4x4BlkIdx].i_blk_idx_B); \
					if (err) return err;

        HL_MB_SET_NBL4x4(0);
        HL_MB_SET_NBL4x4(1);
        HL_MB_SET_NBL4x4(2);
        HL_MB_SET_NBL4x4(3);
        HL_MB_SET_NBL4x4(4);
        HL_MB_SET_NBL4x4(5);
        HL_MB_SET_NBL4x4(6);
        HL_MB_SET_NBL4x4(7);
        HL_MB_SET_NBL4x4(8);
        HL_MB_SET_NBL4x4(9);
        HL_MB_SET_NBL4x4(10);
        HL_MB_SET_NBL4x4(11);
        HL_MB_SET_NBL4x4(12);
        HL_MB_SET_NBL4x4(13);
        HL_MB_SET_NBL4x4(14);
        HL_MB_SET_NBL4x4(15);
    }

    return err;
}

// This function must be called after "hl_codec_264_utils_init_mb_current()"
// This function must be called after "macroblock_layer( )" used to read() the macroblock syntax elements.
// This function execute "G.8.1.5.1 Macroblock initialisation process"
HL_ERROR_T hl_codec_264_utils_init_mb_current_svc(hl_codec_264_t* p_codec, hl_codec_264_mb_t *p_mb)
{
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    int32_t currDQId = p_codec->layers.currDQId;
    const hl_codec_264_mb_t* pc_mb_ref;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if(1) {
        HL_DEBUG_ERROR("This function is not expected to be called");
        return HL_ERROR_INVALID_PARAMETER;
    }

    if (!p_codec || !p_mb) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    pc_slice_header = pc_layer->pc_slice_hdr;
    pc_mb_ref = pc_layer->SpatialResolutionChangeFlag ? HL_NULL : (pc_layer->pc_ref ? pc_layer->pc_ref->pp_list_macroblocks[p_mb->u_addr] : HL_NULL);

    if (!pc_slice_header->SVCExtFlag) {
        HL_DEBUG_ERROR("SVC extension not enabled");
        return HL_ERROR_INVALID_BITSTREAM;
    }

    /* G.8.1.2.1 Array assignment and initialisation process */
    p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_UNSPECIFIED;
    // p_mb->ext.svc.mvCnt = 0;
    p_mb->ext.svc.tQPy = 0;
    p_mb->ext.svc.tQPCb = 0;
    p_mb->ext.svc.tQPCr = 0;
    // hl_memory_set((int32_t*)p_mb->ext.svc.ipred4x4, 16, -1);
    // hl_memory_set((int32_t*)p_mb->ext.svc.ipred8x8, 4, -1);
    p_mb->ext.svc.ipred16x16 = -1;
    p_mb->ext.svc.ipredChroma = -1;

    /* G.8.1.5.1 Macroblock initialisation process */
    p_mb->ext.svc.sliceIdc = ((pc_slice_header->first_mb_in_slice << 7) + p_codec->layers.currDQId);
    p_mb->ext.svc.baseModeFlag = p_mb->ext.svc.base_mode_flag;
    p_mb->ext.svc.fieldMbFlag = pc_slice_header->field_pic_flag ? pc_slice_header->field_pic_flag :
                                ( (!pc_layer->SpatialResolutionChangeFlag && pc_slice_header->ext.svc.slice_skip_flag && pc_mb_ref)
                                  ? pc_mb_ref->ext.svc.fieldMbFlag : p_mb->mb_field_decoding_flag );

    /* G.8.1.5.1.1 Derivation process for macroblock type, sub-macroblock type, and inter-layer predictors for
    		reference indices and motion vectors
       Outputs: "mbType", "subMbType", "refIdxILPredL0", "refIdxILPredL1", "mvILPredL0" and "mvILPredL1"
    */
    /*if (pc_layer->DQId > 0)*/ {
        if (p_mb->ext.svc.base_mode_flag ||
                (p_mb->ext.svc.motion_prediction_flag_lX[0][0] || p_mb->ext.svc.motion_prediction_flag_lX[0][1] || p_mb->ext.svc.motion_prediction_flag_lX[0][2] || p_mb->ext.svc.motion_prediction_flag_lX[0][3]) ||
                (p_mb->ext.svc.motion_prediction_flag_lX[1][0] || p_mb->ext.svc.motion_prediction_flag_lX[1][1] || p_mb->ext.svc.motion_prediction_flag_lX[1][2] || p_mb->ext.svc.motion_prediction_flag_lX[1][3])) {
            /* G.8.6.1 Derivation process for inter-layer predictors for macroblock type, sub-macroblock type, reference
            	indices, and motion vectors.
            	This process is only invoked when base_mode_flag is equal to 1 or any motion_prediction_flag_lX[ mbPartIdx ] with X
            	being replaced by 0 and 1 and mbPartIdx = 0..3 is equal to 1.
            */
            /* G.8.6.1.1 Derivation process for reference layer partition identifications
            	Outputs: "intraILPredFlag" and "refLayerPartId"
            */
            err = hl_codec_264_utils_derivation_process_for_ref_layer_partition_identifications_svc(p_codec, p_mb);
            if (err) {
                return err;
            }
            if (p_mb->ext.svc.intraILPredFlag == 1) {
                p_mb->ext.svc.refIdxILPredL0[0][0] = p_mb->ext.svc.refIdxILPredL0[0][1] = p_mb->ext.svc.refIdxILPredL0[1][0] = p_mb->ext.svc.refIdxILPredL0[1][1] = -1; // not avail
                p_mb->ext.svc.refIdxILPredL1[0][0] = p_mb->ext.svc.refIdxILPredL1[0][1] = p_mb->ext.svc.refIdxILPredL1[1][0] = p_mb->ext.svc.refIdxILPredL1[1][1] = -1; // not avail
                hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL0, 32, 0);
                hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL1, 32, 0);

                p_mb->ext.svc.subMbTypeILPred[0] = p_mb->ext.svc.subMbTypeILPred[1] = p_mb->ext.svc.subMbTypeILPred[2] = p_mb->ext.svc.subMbTypeILPred[3] = -1; // unspecified
                if (pc_slice_header->ext.svc.tcoeff_level_prediction_flag == 1) {
                    p_mb->ext.svc.mbTypeILPred = pc_mb_ref->e_type;
                    p_mb->ext.svc.mb_type_il_pred = pc_mb_ref->mb_type;
                }
                else {
                    p_mb->ext.svc.mbTypeILPred = HL_CODEC_264_MB_TYPE_SVC_I_BL;
                    p_mb->ext.svc.mb_type_il_pred = I_BL;
                }
            }
            else {
                /* G.8.6.1.2 Derivation process for inter-layer predictors for reference indices and motion vectors
                	Outputs: "refIdxILPredL0", "refIdxILPredL1", "mvILPredL0" and "mvILPredL1"
                */
                hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_ref_indices_and_mvs_svc(p_codec, p_mb);
                /*  G.8.6.1.3 Derivation process for inter-layer predictors for P and B macroblock and sub-macroblock types
                	Outputs: "mbTypeILPred", "subMbTypeILPred"
                */
                hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_P_and_B_mb_and_submb_types_svc(p_codec, p_mb);
            }
        }
        else {
            p_mb->ext.svc.mbTypeILPred = -1; // not avail
            p_mb->ext.svc.subMbTypeILPred[0] = p_mb->ext.svc.subMbTypeILPred[1] = p_mb->ext.svc.subMbTypeILPred[2] = p_mb->ext.svc.subMbTypeILPred[3] = -1; // not avail
            p_mb->ext.svc.refIdxILPredL0[0][0] = p_mb->ext.svc.refIdxILPredL0[0][1] = p_mb->ext.svc.refIdxILPredL0[1][0] = p_mb->ext.svc.refIdxILPredL0[1][1] = -1; // not avail
            p_mb->ext.svc.refIdxILPredL1[0][0] = p_mb->ext.svc.refIdxILPredL1[0][1] = p_mb->ext.svc.refIdxILPredL1[1][0] = p_mb->ext.svc.refIdxILPredL1[1][1] = -1; // not avail
            hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL0, 32, 0);
            hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL1, 32, 0);
        }

        if (p_mb->ext.svc.base_mode_flag == 1) {
            if (pc_layer->SpatialResolutionChangeFlag == 0 && pc_mb_ref->e_type == HL_CODEC_264_MB_TYPE_I_PCM && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0) {
                p_mb->e_type = HL_CODEC_264_MB_TYPE_I_PCM;
                p_mb->mb_type = I_PCM;
                p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_PCM;
            }
            else {
                p_mb->e_type = p_mb->ext.svc.mbTypeILPred;
                p_mb->mb_type = p_mb->ext.svc.mb_type_il_pred;
                // Update Flags
                p_mb->flags_type = (HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4 :
                                    (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8 :
                                     (HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16 :
                                      (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_PCM :
                                       (HL_CODEC_264_MB_TYPE_FLAGS_INTER)))));
            }
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_4X4;
            p_mb->mb_type = I_4x4;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4;
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_8X8;
            p_mb->mb_type = I_8x8;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8;
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_16X16;
            p_mb->mb_type = I_16x16;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16;
        }
        else {
            // mbType = "mb_type"
            // alreay done in mb_init()
        }

        // FIXME: make sure "MbPartPredMode[PartIdx]", "NumMbPart", "MbPartWidth" and "MbPartHeight" from the Macroblock are correct at this step.

        if (!HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb) && !HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) && !HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb)) {
            p_mb->SubMbPredType[0] = p_mb->SubMbPredType[1] = p_mb->SubMbPredType[2] = p_mb->SubMbPredType[3] = -1;
        }
        else if (p_mb->ext.svc.base_mode_flag) {
            p_mb->SubMbPredType[0] = p_mb->ext.svc.subMbTypeILPred[0];
            p_mb->SubMbPredType[1] = p_mb->ext.svc.subMbTypeILPred[1];
            p_mb->SubMbPredType[2] = p_mb->ext.svc.subMbTypeILPred[2];
            p_mb->SubMbPredType[3] = p_mb->ext.svc.subMbTypeILPred[3];
        }
        else {
            // subMbType[mbPartIdx] = "sub_mb_type"[mbPartIdx]
            // alreay done in mb_init()
        }
    } // End-of-G.8.1.5.1.1

    /* G.8.1.5.1.2 Derivation process for quantisation parameters and transform type
    	Outputs: "cTrafo", "tQPY", "tQPCb" and "tQPCr".
    */
    {
        // Setting "tQPY"
        if (
            pc_layer->SpatialResolutionChangeFlag == 0 &&
            (
                ((HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) && p_mb->ext.svc.base_mode_flag == 1 && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0)
                ||
                ((HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb)) && p_mb->ext.svc.residual_prediction_flag  == 1)
                ||
                ((HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb) || HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb)) && p_mb->ext.svc.residual_prediction_flag && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0)
            )
        ) {
            p_mb->ext.svc.tQPy = pc_mb_ref->QPy;
        }
        else {
            p_mb->ext.svc.tQPy = p_mb->QPy;
        }

        // Setting "tQPcb" and "tQPcr"
        if (pc_slice_header->pc_pps->pc_sps->ChromaArrayType != 0) {
            if (p_mb->ext.svc.tQPy == p_mb->QPy) {
                // 8.5.8 already computed using QPy == tQPy
                p_mb->ext.svc.tQPCb = p_mb->QPc[0];
                p_mb->ext.svc.tQPCr = p_mb->QPc[1];
            }
            else {
                // 8.5.8 Derivation process for chroma quantisation parameters
                // "8.5.8" is called with input "QPy" equal to "tQPy"
                int32_t qPOffsetCb = pc_slice_header->pc_pps->chroma_qp_index_offset; // (8-313)
                int32_t qPOffsetCr = pc_slice_header->pc_pps->second_chroma_qp_index_offset; // (8-314)
                int32_t qPICb = HL_MATH_CLIP3(-pc_slice_header->pc_pps->pc_sps->QpBdOffsetC, 51, p_mb->ext.svc.tQPy + qPOffsetCb); // (8-315)
                int32_t qPICr = HL_MATH_CLIP3(-pc_slice_header->pc_pps->pc_sps->QpBdOffsetC, 51, p_mb->ext.svc.tQPy + qPOffsetCr); // (8-315)
                p_mb->ext.svc.tQPCb = qPI2QPC[qPICb];
                p_mb->ext.svc.tQPCr = qPI2QPC[qPICr];
            }
        }

        // Setting "predTrafoFlag"
        p_mb->ext.svc.predTrafoFlag = (
                                          pc_layer->SpatialResolutionChangeFlag == 0 &&
                                          (
                                              (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref) && p_mb->CodedBlockPatternLuma == 0)
                                              ||
                                              (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0)
                                              ||
                                              (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && (HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref)) && p_mb->CodedBlockPatternLuma == 0)
                                              ||
                                              (p_mb->ext.svc.base_mode_flag && pc_slice_header->ext.svc.tcoeff_level_prediction_flag && (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref)))
                                              ||
                                              (p_mb->ext.svc.residual_prediction_flag && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)) && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) && p_mb->CodedBlockPatternLuma == 0)
                                          )
                                      );

        // Setting "cTrafo"
        if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)) {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_PCM;
        }
        else if (HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb)) {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_16X16;
        }
        else if (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || p_mb->transform_size_8x8_flag) {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_8X8;
        }
        else if (p_mb->ext.svc.predTrafoFlag) {
            p_mb->ext.svc.cTrafo = pc_mb_ref->ext.svc.cTrafo;
        }
        else {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_4X4;
        }
#if 0
        {
            int32_t constrainedCoeffFlag = (
                                               pc_layer->SpatialResolutionChangeFlag == 0 &&
                                               (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)
                                                ||
                                                (p_mb->ext.svc.residual_prediction_flag && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)) && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)))
                                               );
                                               // TODO: check "constrainedCoeffFlag"
        }
#endif

                                       // The variable mvCnt is set equal to 0.
                                       p_mb->ext.svc.mvCnt = 0;

    } // End-of-G.8.1.5.1.2

    return err;
}

// 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
// Should not call this function: use "ctx->NeighbouringLumaBlock4x4"
// FIXME: don't say should use -> USE IT in the macro
HL_ERROR_T hl_codec_264_utils_derivation_process_for_neighbouring_4x4_luma_blocks(
    HL_IN const hl_codec_264_t* pc_codec,
    HL_IN const hl_codec_264_mb_t* pc_mb,
    HL_IN int32_t luma4x4BlkIdx,
    HL_OUT int32_t* mbAddrA,
    HL_OUT int32_t* luma4x4BlkIdxA,
    HL_OUT int32_t* mbAddrB,
    HL_OUT int32_t* luma4x4BlkIdxB)
{
    // 1. The difference of luma location ( xD, yD ) is set according to Table 6-2.
    // xD_yD[A=0/B=1/C=2/D=3][xD=0/yD=1]
    int32_t x,y,xWA,yWA,xWB,yWB,xA,yA,xB,yB;
    const hl_codec_264_nal_slice_header_t* pc_slice_hr;
    const hl_codec_264_layer_t* pc_layer;

    //(6-31)
    static const int32_t maxW = 16;
    static const int32_t maxH = 16;

    pc_layer = pc_codec->layers.pc_active;
    pc_slice_hr = pc_layer->p_list_slices[pc_mb->u_slice_idx]->p_header;

    // 2. The inverse 4x4 luma block scanning process as specified in subclause 6.4.3 is invoked with luma4x4BlkIdx
    // as the input and ( x, y ) as the output.
    //FIXME: use "Inverse4x4LumaBlockScanOrderXY"
    //x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
    //				InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);//(6-17)
    //y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
    //				InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);//(6-18)
    x = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];//(6-17)
    y = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];//(6-18)

    // 3. The luma location ( xN, yN ) is specified by
    //(6-23)
    xA = x + xD_yD[0][0];
    xB = x + xD_yD[1][0];
    //(6-24)
    yA = y + xD_yD[0][1];
    yB = y + xD_yD[1][1];

    //4. The derivation process for neighbouring locations as specified in subclause 6.4.11 is invoked for luma
    //locations with ( xN, yN ) as the input and the output is assigned to mbAddrN and ( xW, yW ).
    if (pc_slice_hr->MbaffFrameFlag) {
        // Otherwise (MbaffFrameFlag is equal to 1), the specification for neighbouring locations in MBAFF frames as
        // described in subclause 6.4.11.2 is applied.
        HL_DEBUG_ERROR("MbAFF not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        // If MbaffFrameFlag is equal to 0, the specification for neighbouring locations in fields and non-MBAFF frames as
        // described in subclause 6.4.11.1 is applied.

        // 6.4.11.1 Specification for neighbouring locations in fields and non-MBAFF frames
        hl_codec_264_utils_specification_for_neighbouring_locations_in_fields_and_non_MBAFF_frames(pc_codec, pc_mb, mbAddrA, xA, yA, maxW, maxH, &xWA, &yWA);
        hl_codec_264_utils_specification_for_neighbouring_locations_in_fields_and_non_MBAFF_frames(pc_codec, pc_mb, mbAddrB, xB, yB, maxW, maxH, &xWB, &yWB);
    }

    // 5. The variable luma4x4BlkIdxN is derived as follows.
    //	– If mbAddrN is not available, luma4x4BlkIdxN is marked as not available.
    //	– Otherwise (mbAddrN is available), the derivation process for 4x4 luma block indices as specified in
    //		subclause 6.4.12.1 is invoked with the luma location ( xW, yW ) as the input and the output is assigned
    //		to luma4x4BlkIdxN.
    *luma4x4BlkIdxA = HL_CODEC_264_MB_IS_NOT_AVAIL(*mbAddrA, pc_codec, pc_slice_hr) ? HL_CODEC_264_MB_ADDR_NOT_AVAIL : hl_codec_264_utils_derivation_process_for_4x4_luma_block_indices(xWA, yWA);
    *luma4x4BlkIdxB = HL_CODEC_264_MB_IS_NOT_AVAIL(*mbAddrB, pc_codec, pc_slice_hr) ? HL_CODEC_264_MB_ADDR_NOT_AVAIL : hl_codec_264_utils_derivation_process_for_4x4_luma_block_indices(xWB, yWB);

    return HL_ERROR_SUCCESS;
}

// 6.4.10.5 Derivation process for neighbouring 4x4 chroma blocks
// This subclause is only invoked when ChromaArrayType is equal to 1 or 2.
// Should use "pc_codec->NeighbouringChromaBlock4x4"
// FIXME: Must use "NeighbouringChromaBlock4x4" in the macro instead of saying "should use..."
HL_ERROR_T hl_codec_264_utils_derivation_process_for_neighbouring_4x4_chroma_blocks(
    HL_IN const hl_codec_264_t* pc_codec,
    HL_IN const hl_codec_264_mb_t* pc_mb,
    HL_IN int32_t chroma4x4BlkIdx,
    HL_OUT int32_t* mbAddrA,
    HL_OUT int32_t* chroma4x4BlkIdxA,
    HL_OUT int32_t* mbAddrB,
    HL_OUT int32_t* chroma4x4BlkIdxB)
{
    // 1. The difference of luma location ( xD, yD ) is set according to Table 6-2.
    // xD_yD[A=0/B=1/C=2/D=3][xD=0/yD=1]
    int32_t xWA, yWA, xWB, yWB, x, y, xA, yA, xB, yB;
    const hl_codec_264_nal_slice_header_t* pc_slice_hr;
    const hl_codec_264_layer_t* pc_layer;


    const int32_t maxW = pc_codec->sps.pc_active->MbWidthC;//(6-32)
    const int32_t maxH = pc_codec->sps.pc_active->MbHeightC; //(6-33)

    pc_layer = pc_codec->layers.pc_active;
    pc_slice_hr = pc_layer->p_list_slices[pc_mb->u_slice_idx]->p_header;

    x = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][0];// (6-25)
    y = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][1];//(6-26)

    // 3. The chroma location ( xN, yN ) is specified by
    //(6-27)
    xA = x + xD_yD[0][0];
    xB = x + xD_yD[1][0];
    //(6-28)
    yA = y + xD_yD[0][1];
    yB = y + xD_yD[1][1];

    // 4. The derivation process for neighbouring locations as specified in subclause 6.4.11 is invoked for chroma
    // locations with ( xN, yN ) as the input and the output is assigned to mbAddrN and ( xW, yW ).

    if (pc_slice_hr->MbaffFrameFlag) {
        // Otherwise (MbaffFrameFlag is equal to 1), the specification for neighbouring locations in MBAFF frames as
        // described in subclause 6.4.11.2 is applied.
        HL_DEBUG_ERROR("MbAFF not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {

        // If MbaffFrameFlag is equal to 0, the specification for neighbouring locations in fields and non-MBAFF frames as
        // described in subclause 6.4.11.1 is applied.

        // 6.4.11.1 Specification for neighbouring locations in fields and non-MBAFF frames
        hl_codec_264_utils_specification_for_neighbouring_locations_in_fields_and_non_MBAFF_frames(pc_codec, pc_mb, mbAddrA, xA, yA, maxW, maxH, &xWA, &yWA);
        hl_codec_264_utils_specification_for_neighbouring_locations_in_fields_and_non_MBAFF_frames(pc_codec, pc_mb, mbAddrB, xB, yB, maxW, maxH, &xWB, &yWB);
    }

    //5. The variable chroma4x4BlkIdxN is derived as follows.
    //	– If mbAddrN is not available, chroma4x4BlkIdxN is marked as not available.
    //	– Otherwise (mbAddrN is available), the derivation process for 4x4 chroma block indices as specified in
    //	subclause 6.4.12.2 is invoked with the chroma location ( xW, yW ) as the input and the output is
    //	assigned to chroma4x4BlkIdxN
    *chroma4x4BlkIdxA = HL_CODEC_264_MB_IS_NOT_AVAIL(*mbAddrA, pc_codec, pc_slice_hr) ? HL_CODEC_264_MB_ADDR_NOT_AVAIL : hl_codec_264_utils_derivation_process_for_4x4_chroma_block_indices(xWA, yWA);
    *chroma4x4BlkIdxB = HL_CODEC_264_MB_IS_NOT_AVAIL(*mbAddrB, pc_codec, pc_slice_hr) ? HL_CODEC_264_MB_ADDR_NOT_AVAIL : hl_codec_264_utils_derivation_process_for_4x4_chroma_block_indices(xWB, yWB);

    return HL_ERROR_SUCCESS;
}

// 8.4.1 Derivation process for motion vector components and reference indices
HL_ERROR_T hl_codec_264_utils_derivation_process_for_movect_comps_and_ref_indices(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    HL_OUT struct hl_codec_264_mv_xs *mvL0,
    HL_OUT struct hl_codec_264_mv_xs *mvL1,
    HL_OUT struct hl_codec_264_mv_xs *mvCL0,
    HL_OUT struct hl_codec_264_mv_xs *mvCL1,
    HL_OUT int32_t* refIdxL0,
    HL_OUT int32_t* refIdxL1,
    HL_OUT int32_t* predFlagL0,
    HL_OUT int32_t* predFlagL1,
    HL_OUT int32_t* subMvCnt)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    // Derivation for 'mvL0 and mvL1 as well as refIdxL0 and refIdxL1'

    if(p_mb->e_type == HL_CODEC_264_MB_TYPE_P_SKIP) {
        // Derivation process for luma vectors for skipped mb in P and SP clices in subclause 8.4.11 is invoked
        *predFlagL0 = 1;
        *refIdxL1 = *predFlagL1 = 0;
        *subMvCnt = 1;
        // 8.4.1.1 Derivation process for luma motion vectors for skipped macroblocks in P and SP slices
        hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(pc_codec, p_mb, mvL0, refIdxL0);
    }
    else if(p_mb->e_type == HL_CODEC_264_MB_TYPE_B_SKIP || p_mb->e_type == HL_CODEC_264_MB_TYPE_B_DIRECT_16X16 || p_mb->SubMbPredType[mbPartIdx] == HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8) {
        // Derivation process for luma motion vectors for B_Skip, B_Direct_16x16 and B_Direct_8x8 in B slices in subclause 8.4.1.2
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        hl_codec_264_mv_xt mvpLX;
        HL_CODEC_264_SUBMB_TYPE_T currSubMbType =
            p_mb->e_type == HL_CODEC_264_MB_TYPE_B_8X8 ? p_mb->SubMbPredType[mbPartIdx] : HL_CODEC_264_SUBMB_TYPE_NA;

        // List0 [ MbPartPredMode( mb_type, mbPartIdx ) or SubMbPredMode( sub_mb_type[ mbPartIdx ] ) ]
        if (HL_CODEC_264_SUBMB_MODE_IS_PRED_L0(p_mb, mbPartIdx) || HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, mbPartIdx) || HL_CODEC_264_SUBMB_MODE_IS_BIPRED(p_mb, mbPartIdx) || HL_CODEC_264_MB_MODE_IS_BIPRED(p_mb, mbPartIdx)) {
            *refIdxL0 = p_mb->ref_idx_l0[mbPartIdx];
            *predFlagL0 = 1;
        }
        else {
            *refIdxL0 = -1;
            *predFlagL0 = 0;
        }
        if(*predFlagL0 == 1) {
            // 8.4.1.3 Derivation process for luma motion vector prediction
            hl_codec_264_utils_derivation_process_for_luma_movect_prediction(pc_codec, p_mb, mbPartIdx, subMbPartIdx, *refIdxL0, currSubMbType, &mvpLX, listSuffixFlag_0);
            mvL0->x = mvpLX.x + p_mb->mvd_l0[mbPartIdx][subMbPartIdx].x;// (8-174)
            mvL0->y = mvpLX.y + p_mb->mvd_l0[mbPartIdx][subMbPartIdx].y;// (8-175)
        }

        // List1 [ MbPartPredMode( mb_type, mbPartIdx ) or SubMbPredMode( sub_mb_type[ mbPartIdx ] ) ]
        if (HL_CODEC_264_SUBMB_MODE_IS_PRED_L1(p_mb, mbPartIdx) || HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, mbPartIdx) || HL_CODEC_264_SUBMB_MODE_IS_BIPRED(p_mb, mbPartIdx) || HL_CODEC_264_MB_MODE_IS_BIPRED(p_mb, mbPartIdx)) {
            *refIdxL1 = p_mb->ref_idx_l1[mbPartIdx];
            *predFlagL1 = 1;
        }
        else {
            *refIdxL1 = -1;
            *predFlagL1 = 0;
        }
        if (*predFlagL1 == 1) {
            // 8.4.1.3 Derivation process for luma motion vector predictio
            hl_codec_264_utils_derivation_process_for_luma_movect_prediction(pc_codec, p_mb, mbPartIdx, subMbPartIdx, *refIdxL1, currSubMbType, &mvpLX, listSuffixFlag_1);
            mvL1->x = mvpLX.x + p_mb->mvd_l0[mbPartIdx][subMbPartIdx].x;// (8-174)
            mvL1->y = mvpLX.y + p_mb->mvd_l0[mbPartIdx][subMbPartIdx].y;// (8-175)
        }
    }

    *subMvCnt = *predFlagL0 + *predFlagL1;

    // Chroma motion vector
    // 8.4.1.4 Derivation process for chroma motion vectors
    if(pc_codec->sps.pc_active->ChromaArrayType != 0) {
        if(*predFlagL0 == 1) {
            err = hl_codec_264_utils_derivation_process_for_chroma_movects(pc_codec, p_mb, mvL0, mvCL0, listSuffixFlag_0);
        }
        if(*predFlagL1 == 1) {
            err = hl_codec_264_utils_derivation_process_for_chroma_movects(pc_codec, p_mb, mvL1, mvCL1, listSuffixFlag_1);
        }
    }
    else {
        // FIXME: call "8.4.1.4"
        HL_DEBUG_ERROR("Call 8.4.1.4");
    }
    return err;
}

// 8.4.1.1 Derivation process for luma motion vectors for skipped macroblocks in P and SP slices
void hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    struct hl_codec_264_mv_xs *mvL0,
    int32_t* refIdxL0)
{
    static int32_t mbPartIdx = 0;
    static int32_t subMbPartIdx = 0;
    // static enum HL_CODEC_264_LIST_IDX_E listSuffixFlag = listSuffixFlag_0;
    static HL_CODEC_264_SUBMB_TYPE_T currSubMbType = HL_CODEC_264_SUBMB_TYPE_NA;

    // FIXME: define inside p_codec ?
    hl_codec_264_mv_xt mvL0A, mvL0B, mvL0C, mvL0D;
    int32_t mbAddrA, mbPartIdxA, subMbPartIdxA, refIdxL0A;
    int32_t mbAddrB, mbPartIdxB, subMbPartIdxB, refIdxL0B;
    int32_t mbAddrC, mbPartIdxC, subMbPartIdxC, refIdxL0C;
    int32_t mbAddrD, mbPartIdxD, subMbPartIdxD, refIdxL0D;


    *refIdxL0 = 0;

    // 8.4.1.3.2 Derivation process for motion data of neighbouring partitions
    hl_codec_264_utils_derivation_process_for_modata_of_neighbouring_partitions(pc_codec, p_mb, mbPartIdx, currSubMbType, subMbPartIdx, HL_TRUE,
            &mbAddrA, &mbPartIdxA, &subMbPartIdxA, &mvL0A, &refIdxL0A,
            &mbAddrB, &mbPartIdxB, &subMbPartIdxB, &mvL0B, &refIdxL0B,
            &mbAddrC, &mbPartIdxC, &subMbPartIdxC, &mvL0C, &refIdxL0C,
            &mbAddrD, &mbPartIdxD, &subMbPartIdxD, &mvL0D, &refIdxL0D,
            listSuffixFlag_0);

    if (mbAddrA < 0 || mbAddrB < 0 ||
            (refIdxL0A == 0 && !mvL0A.x && !mvL0A.y) || (refIdxL0B == 0 && !mvL0B.x && !mvL0B.y)) {
        mvL0->x = mvL0->y = 0;
    }
    else {
        // 8.4.1.3 Derivation process for luma motion vector predictio
        // FIXME: 8.4.1.3.2 called again within "hl_codec_264_utils_derivation_process_for_luma_movect_prediction()"
        hl_codec_264_utils_derivation_process_for_luma_movect_prediction(pc_codec, p_mb, mbPartIdx, subMbPartIdx, *refIdxL0, currSubMbType,
                mvL0, listSuffixFlag_0);
    }
}

// 8.4.1.3 Derivation process for luma motion vector prediction
HL_ERROR_T hl_codec_264_utils_derivation_process_for_luma_movect_prediction(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    int32_t refIdxLX,
    enum HL_CODEC_264_SUBMB_TYPE_E currSubMbType,
    struct hl_codec_264_mv_xs *mvpLX,
    enum HL_CODEC_264_LIST_IDX_E listSuffix)
{
    // FIXME: pc_codec?
    hl_codec_264_mv_xt mvLXA, mvLXB, mvLXC, mvLXD;
    int32_t mbAddrA, mbPartIdxA, subMbPartIdxA, refIdxLXA;
    int32_t mbAddrB, mbPartIdxB, subMbPartIdxB, refIdxLXB;
    int32_t mbAddrC, mbPartIdxC, subMbPartIdxC, refIdxLXC;
    int32_t mbAddrD, mbPartIdxD, subMbPartIdxD, refIdxLXD;

    // 8.4.1.3.2 Derivation process for motion data of neighbouring partitions
    hl_codec_264_utils_derivation_process_for_modata_of_neighbouring_partitions(pc_codec, p_mb, mbPartIdx, currSubMbType, subMbPartIdx, HL_TRUE,
            &mbAddrA, &mbPartIdxA, &subMbPartIdxA, &mvLXA, &refIdxLXA,
            &mbAddrB, &mbPartIdxB, &subMbPartIdxB, &mvLXB, &refIdxLXB,
            &mbAddrC, &mbPartIdxC, &subMbPartIdxC, &mvLXC, &refIdxLXC,
            &mbAddrD, &mbPartIdxD, &subMbPartIdxD, &mvLXD, &refIdxLXD,
            listSuffix);

    if(p_mb->MbPartWidth == 16 && p_mb->MbPartHeight == 8 && mbPartIdx == 0 && (refIdxLXB == refIdxLX)) {
        mvpLX->x = mvLXB.x,mvpLX->y = mvLXB.y;  // (8-205)
    }
    else if(p_mb->MbPartWidth == 16 && p_mb->MbPartHeight == 8 && mbPartIdx == 1 && (refIdxLXA == refIdxLX)) {
        mvpLX->x = mvLXA.x,mvpLX->y = mvLXA.y;  // (8-206)
    }
    else if(p_mb->MbPartWidth == 8 && p_mb->MbPartHeight == 16 && mbPartIdx == 0 && (refIdxLXA == refIdxLX)) {
        mvpLX->x = mvLXA.x,mvpLX->y = mvLXA.y;  // (8-207)
    }
    else if(p_mb->MbPartWidth == 8 && p_mb->MbPartHeight == 16 && mbPartIdx == 1 && (refIdxLXC == refIdxLX)) {
        mvpLX->x = mvLXC.x,mvpLX->y = mvLXC.y;  // (8-208)
    }
    else {
        // 8.4.1.3.1 Derivation process for median luma motion vector prediction
        hl_codec_264_utils_derivation_process_for_median_luma_movect_prediction(pc_codec, p_mb, refIdxLX,
                mbAddrA, mbPartIdxA, subMbPartIdxA, mvLXA, refIdxLXA,
                mbAddrB, mbPartIdxB, subMbPartIdxB, mvLXB, refIdxLXB,
                mbAddrC, mbPartIdxC, subMbPartIdxC, mvLXC, refIdxLXC,
                mvpLX);
    }

    return HL_ERROR_SUCCESS;
}

// 8.4.1.3.1 Derivation process for median luma motion vector prediction
void	hl_codec_264_utils_derivation_process_for_median_luma_movect_prediction(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t refIdxLX,//currPartition
    int32_t mbAddrA, int32_t mbPartIdxA, int32_t subMbPartIdxA, struct hl_codec_264_mv_xs mvLXA, int32_t refIdxLXA,
    int32_t mbAddrB, int32_t mbPartIdxB, int32_t subMbPartIdxB, struct hl_codec_264_mv_xs mvLXB, int32_t refIdxLXB,
    int32_t mbAddrC, int32_t mbPartIdxC, int32_t subMbPartIdxC, struct hl_codec_264_mv_xs mvLXC, int32_t refIdxLXC,
    HL_OUT struct hl_codec_264_mv_xs *mvpLX)
{
    if ((mbAddrB < 0 || mbPartIdxB < 0 || subMbPartIdxB < 0) &&
            (mbAddrC < 0 || mbPartIdxC < 0 || subMbPartIdxC < 0) && (mbAddrA >= 0 && mbPartIdxA >= 0 && subMbPartIdxA >= 0)) {
        mvLXB.x = mvLXA.x, mvLXB.y = mvLXA.y;
        mvLXC.x = mvLXA.x,mvLXC.y = mvLXA.y;
        refIdxLXB = refIdxLXA;
        refIdxLXC = refIdxLXA;
    }

    if (refIdxLXA == refIdxLX && (refIdxLXB != refIdxLX && refIdxLXC != refIdxLX)) {
        mvpLX->x = mvLXA.x,mvpLX->y = mvLXA.y;
    }
    else if (refIdxLXB == refIdxLX && (refIdxLXC != refIdxLX && refIdxLXA != refIdxLX)) {
        mvpLX->x=mvLXB.x,mvpLX->y = mvLXB.y;
    }
    else if (refIdxLXC == refIdxLX && (refIdxLXB != refIdxLX && refIdxLXA != refIdxLX)) {
        mvpLX->x = mvLXC.x,mvpLX->y = mvLXC.y;
    }
    else {
        mvpLX->x = HL_MATH_MEDIAN(mvLXA.x, mvLXB.x, mvLXC.x);// (8-214)
        mvpLX->y = HL_MATH_MEDIAN(mvLXA.y, mvLXB.y, mvLXC.y);// (8-215)
    }
}

// 8.4.1.4 Derivation process for chroma motion vectors
HL_ERROR_T hl_codec_264_utils_derivation_process_for_chroma_movects(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    HL_IN const struct hl_codec_264_mv_xs *mvLX,//luma motion vector
    HL_OUT struct hl_codec_264_mv_xs *mvCLX, // chroma motion vector
    enum HL_CODEC_264_LIST_IDX_E listSuffix)
{
    if (pc_codec->sps.pc_active->ChromaArrayType != 1 || p_mb->mb_field_decoding_flag == 0) {
        mvCLX->x = mvLX->x;
        mvCLX->y = mvLX->y;
    }
    else { // ChromaArrayType==1 and field Macroblock
        mvCLX->x = mvLX->x;
        HL_DEBUG_ERROR("Table 8-10");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    return HL_ERROR_SUCCESS;
}

// 8.4.1.3.2 Derivation process for motion data of neighbouring partitions
void	hl_codec_264_utils_derivation_process_for_modata_of_neighbouring_partitions(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    enum HL_CODEC_264_SUBMB_TYPE_E currSubMbType,
    int32_t subMbPartIdx,
    hl_bool_t luma,
    int32_t* mbAddrA, int32_t* mbPartIdxA, int32_t* subMbPartIdxA, hl_codec_264_mv_xt* mvLXA, int32_t* refIdxLXA,
    int32_t* mbAddrB, int32_t* mbPartIdxB, int32_t* subMbPartIdxB, hl_codec_264_mv_xt* mvLXB, int32_t* refIdxLXB,
    int32_t* mbAddrC, int32_t* mbPartIdxC, int32_t* subMbPartIdxC, hl_codec_264_mv_xt* mvLXC, int32_t* refIdxLXC,
    int32_t* mbAddrD, int32_t* mbPartIdxD, int32_t* subMbPartIdxD, hl_codec_264_mv_xt* mvLXD, int32_t* refIdxLXD,
    enum HL_CODEC_264_LIST_IDX_E listSuffix)
{
    enum N_e { N_A, N_B, N_C, N_D };
    enum N_e N;
    typedef hl_codec_264_mv_xt array4x4Mv_t[4][4];

    hl_codec_264_mb_t *mbN;
    int32_t *mbAddrN, *mbPartIdxN, *subMbPartIdxN, *refIdxLXN;
    int32_t *predFlagLX;
    array4x4Mv_t *MvLX;
    hl_codec_264_mv_xt* mvLXN;
    int32_t *RefIdxLX;
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;

    // 6.4.10.7 Derivation process for neighbouring partitions
    hl_codec_264_mb_get_neighbouring_partitions(pc_codec, p_mb, mbPartIdx, currSubMbType, subMbPartIdx,luma,
            mbAddrA, mbPartIdxA, subMbPartIdxA,
            mbAddrB, mbPartIdxB, subMbPartIdxB,
            mbAddrC, mbPartIdxC, subMbPartIdxC,
            mbAddrD, mbPartIdxD, subMbPartIdxD);

    if (*mbAddrC<0 || *mbPartIdxC<0 || *subMbPartIdxC<0) {
        *mbAddrC=*mbAddrD;
        *mbPartIdxC=*mbPartIdxD;
        *subMbPartIdxC=*subMbPartIdxD;
    }


    for(N=N_A; N<=N_D; ++N) {
        switch(N) {
        case N_A: {
            mbAddrN=mbAddrA;
            mbPartIdxN=mbPartIdxA;
            subMbPartIdxN=subMbPartIdxA;
            mvLXN=mvLXA;
            refIdxLXN=refIdxLXA;
            break;
        }
        case N_B: {
            mbAddrN=mbAddrB;
            mbPartIdxN=mbPartIdxB;
            subMbPartIdxN=subMbPartIdxB;
            mvLXN=mvLXB;
            refIdxLXN=refIdxLXB;
            break;
        }
        case N_C: {
            mbAddrN=mbAddrC;
            mbPartIdxN=mbPartIdxC;
            subMbPartIdxN=subMbPartIdxC;
            mvLXN=mvLXC;
            refIdxLXN=refIdxLXC;
            break;
        }
        case N_D: {
            mbAddrN=mbAddrD;
            mbPartIdxN=mbPartIdxD;
            subMbPartIdxN=subMbPartIdxD;
            mvLXN=mvLXD;
            refIdxLXN=refIdxLXD;
            break;
        }
        }

        mbN = *mbAddrN<0 ? HL_NULL : pc_codec->layers.pc_active->pp_list_macroblocks[*mbAddrN];
        if (mbN) {
            switch(listSuffix) {
            case listSuffixFlag_0:
                predFlagLX = &mbN->predFlagL0[*mbPartIdxN];
                MvLX=pc_layer->SVCExtFlag ? (array4x4Mv_t*)mbN->mvL0 : (array4x4Mv_t*)mbN->MvL0;
                RefIdxLX=pc_layer->SVCExtFlag ? mbN->refIdxL0 : mbN->RefIdxL0;
                break;
            case listSuffixFlag_1:
                predFlagLX = &mbN->predFlagL1[*mbPartIdxN];
                MvLX=pc_layer->SVCExtFlag ? (array4x4Mv_t*)mbN->mvL1 : (array4x4Mv_t*)mbN->MvL1;
                RefIdxLX=pc_layer->SVCExtFlag ? mbN->refIdxL1 : mbN->RefIdxL1;
                break;
            }
        }

        if (!mbN || HL_CODEC_264_MB_TYPE_IS_INTRA(mbN) || *predFlagLX == 0) {
            mvLXN->x=mvLXN->y=0;
            *refIdxLXN=-1;
        }
        else {
            mvLXN->x=(*MvLX)[*mbPartIdxN][*subMbPartIdxN].x;
            mvLXN->y=(*MvLX)[*mbPartIdxN][*subMbPartIdxN].y;
            *refIdxLXN=RefIdxLX[*mbPartIdxN];
            if(p_mb->mb_field_decoding_flag && !mbN->mb_field_decoding_flag) {
                mvLXN->y = mvLXN->y >> 1;// FIXME: mvLXN[1] = mvLXN[1] / 2;
                *refIdxLXN = *refIdxLXN << 1;
            }
            else if(!p_mb->mb_field_decoding_flag && mbN->mb_field_decoding_flag) {
                mvLXN->y = mvLXN->y << 1; // FIXME: mvLXN[1] = mvLXN[1] * 2;
                *refIdxLXN = *refIdxLXN >> 1;
            }
        }
    }
}

// G.6.1 Derivation process for reference layer macroblocks
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_mb_svc(
    const hl_codec_264_t* pc_codec,
    const hl_codec_264_mb_t *pc_mb,
    int32_t xP, int32_t yP, // input luma location
    int32_t fieldMbFlag,
    int32_t* mbAddrRefLayer,
    int32_t* xB, int32_t* yB
)
{
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->p_list_slices[pc_mb->u_slice_idx]->p_header;
    int32_t xRef, yRef, xM, yM, xC, yC;
    int32_t refW = pc_layer->RefLayerPicWidthInSamplesL; // (G-1)
    int32_t refH = pc_layer->RefLayerPicHeightInSamplesL; // (G-2)
    int32_t scaledW = pc_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL; // (G-3)
    int32_t scaledH = pc_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL; // (G-4)
    int32_t offsetX = pc_slice_header->ext.svc.ScaledRefLayerLeftOffset; // (G-5)
    int32_t offsetY = pc_slice_header->ext.svc.ScaledRefLayerTopOffset / ( 1 + pc_slice_header->field_pic_flag ); // (G-6)

    // FIXME:
    // Let currDQId be the current value of DQId and let levelIdc be the value of level_idc in the SVC sequence parameter set
    // that is referred to in coded slice NAL units with DQId equal to (( currDQId >> 4 ) << 4).
    int32_t levelIdc = pc_codec->layers.p_list[(pc_codec->layers.currDQId >> 4) << 4]->pc_slice_hdr->pc_pps->pc_sps->level_idc;
    int32_t shiftX = ((levelIdc <= 30) ? 16 : (31 - (int32_t)HL_MATH_CEIL(HL_MATH_LOG2(refW)))); // (G-7)
    int32_t shiftY = (( levelIdc <= 30) ? 16 : (31 - (int32_t)HL_MATH_CEIL(HL_MATH_LOG2(refH)))); // (G-8)
    int32_t scaleX = (( refW << shiftX) + (scaledW >> 1)) / scaledW; // (G-9)
    int32_t scaleY = (( refH << shiftY) + (scaledH >> 1)) / scaledH; // (G-10)

    // 6.4.1 Inverse macroblock scanning process
    hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_mb->u_addr, &xM, &yM);

    xC = xM + xP; // (G-11)
    yC = yM + yP * (1 + fieldMbFlag - pc_slice_header->field_pic_flag); // (G-12)
    xRef = ((xC - offsetX) * scaleX + (1 << (shiftX - 1))) >> shiftX; // (G-13)
    yRef = ((yC - offsetY) * scaleY + (1 << (shiftY - 1))) >> shiftY; // (G-14)
    xRef = HL_MATH_MIN(pc_layer->RefLayerPicWidthInSamplesL - 1, xRef);// (G-13bis)
    yRef = HL_MATH_MIN(pc_layer->RefLayerPicHeightInSamplesL - 1, yRef);// (G-14ter)

    if (!pc_slice_header->MbaffFrameFlag && !pc_layer->RefLayerMbaffFrameFlag) {
        *mbAddrRefLayer = (yRef >> 4) * pc_layer->RefLayerPicWidthInMbs + (xRef >> 4); // (G-15)
        if (HL_CODEC_264_MB_IS_NOT_AVAIL((*mbAddrRefLayer), pc_codec, pc_slice_header)) {
            *xB = *yB = -1;
        }
        else {
            *xB = xRef & 15;
            *yB = yRef & 15;
        }
    }
    else {
        //int32_t virtMbAddrRefLayer;
        //const hl_codec_264_mb_t *pc_mb_ref;
        //if (pc_layer->RefLayerMbaffFrameFlag) {
        //	virtMbAddrRefLayer = (((yRef >> 5) * pc_layer->RefLayerPicWidthInMbs + (xRef >> 4)) << 1) + ((yRef & 31) >> 4); // (G-16)
        //}
        //else {
        //	virtMbAddrRefLayer = (yRef >> 4) * pc_layer->RefLayerPicWidthInMbs + (xRef >> 4);// (G-17)
        //}
        HL_DEBUG_ERROR("Not implemented");
        *xB = *yB = 0;
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    return HL_ERROR_SUCCESS;
}

// FIXME: macro
// G.6.2 Derivation process for reference layer partitions
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_partitions_svc(
    const hl_codec_264_t* pc_codec,
    const hl_codec_264_mb_t *pc_mb,
    int32_t xP, int32_t yP, // input luma location
    int32_t *mbAddrRefLayer,
    int32_t *mbPartIdxRefLayer,
    int32_t *subMbPartIdxRefLayer
)
{
    HL_ERROR_T err;
    int32_t xB, yB;
    // G.6.1 Derivation process for reference layer macroblocks
    err = hl_codec_264_utils_derivation_process_for_ref_layer_mb_svc(pc_codec, pc_mb, xP, yP, pc_mb->ext.svc.fieldMbFlag, mbAddrRefLayer, &xB, &yB);
    if (err) {
        return err;
    }
    else {
        // G.6.4 SVC derivation process for macroblock and sub-macroblock partition indices
        const hl_codec_264_mb_t *pc_mb_ref;
        int32_t currDQId = pc_codec->layers.pc_active->pc_slice_hdr->ext.svc.ref_layer_dq_id;
        hl_codec_264_layer_t* pc_layer_ref = pc_codec->layers.p_list[currDQId];
        if (!pc_layer_ref) {
            HL_DEBUG_ERROR("Failed to find layer with id = %d", currDQId);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        if (*mbAddrRefLayer >= (int32_t)pc_layer_ref->u_list_macroblocks_count || !(pc_mb_ref = pc_layer_ref->pp_list_macroblocks[*mbAddrRefLayer])) {
            HL_DEBUG_ERROR("Failed to find MB with id = %d in layer with id = %d", *mbAddrRefLayer, pc_layer_ref->DQId);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        hl_codec_264_utils_derivation_process_for_mb_and_submb_partition_indices_svc(pc_codec, pc_mb_ref, currDQId, xB, yB, mbPartIdxRefLayer, subMbPartIdxRefLayer);
        return HL_ERROR_SUCCESS;
    }
}

// G.6.3 Derivation process for reference layer sample locations in resampling
void	hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t chromaFlag, int32_t xP, int32_t yP, int32_t botFieldFlag,
    int32_t* xRef16, int32_t* yRef16
)
{
    int32_t refW, refH, scaledW, scaledH, refPhaseX, refPhaseY, phaseX, phaseY, subW, subH, xC, yC, xM, yM, shiftX, shiftY, levelIdc, addX, addY, offsetX, offsetY, scaleX, scaleY, deltaX, deltaY;
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    // FIXME:
    // levelIdc be the value of level_idc in the SVC sequence parameter set
    // that is referred to in coded slice NAL units with DQId equal to (( currDQId >> 4 ) << 4).
    levelIdc = pc_codec->layers.p_list[(pc_codec->layers.currDQId >> 4) << 4]->pc_slice_hdr->pc_pps->pc_sps->level_idc;

    // FIXME: Compute all values (except "xC" and "yC" when the layer is created)
    // NOTE – The variables subW, subH, shiftX, shiftY, scaleX, scaleY, offsetX, offsetY, addX, addY, deltaX, and deltaY do not
    // depend on the input sample location ( xP, yP ), the input variable fieldMbFlag, or the current macroblock address CurrMbAddr.

    if (chromaFlag == 0) {
        refW = pc_layer->RefLayerPicWidthInSamplesL;// (G-28)
        refH = pc_layer->RefLayerPicHeightInSamplesL * ( 1 + pc_layer->RefLayerFieldPicFlag );// (G-29)
        scaledW = pc_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesL;// (G-30)
        scaledH = pc_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesL * ( 1 + pc_slice_header->field_pic_flag );// (G-31)
    }
    else {
        refW = pc_layer->RefLayerPicWidthInSamplesC;// (G-28)
        refH = pc_layer->RefLayerPicHeightInSamplesC * ( 1 + pc_layer->RefLayerFieldPicFlag );// (G-29)
        scaledW = pc_slice_header->ext.svc.ScaledRefLayerPicWidthInSamplesC;// (G-30)
        scaledH = pc_slice_header->ext.svc.ScaledRefLayerPicHeightInSamplesC * ( 1 + pc_slice_header->field_pic_flag );// (G-31)
    }

    if (!pc_sps->frame_mbs_only_flag && pc_layer->RefLayerFrameMbsOnlyFlag) {
        scaledH >>= 1; // (G-32);
    }

    refPhaseX = ( ( chromaFlag == 0 ) ? 0 : ( pc_slice_header->ext.svc.ref_layer_chroma_phase_x_plus1_flag - 1 ) );// (G-33)
    refPhaseY = ( ( chromaFlag == 0 ) ? 0 : ( pc_slice_header->ext.svc.ref_layer_chroma_phase_y_plus1 - 1 ) );// (G-34)
    phaseX = ( ( chromaFlag == 0 ) ? 0 : ( pc_sps->p_svc->chroma_phase_x_plus1_flag - 1 ) );// (G-35)
    phaseY = ( ( chromaFlag == 0 ) ? 0 : ( pc_sps->p_svc->chroma_phase_y_plus1 - 1 ) );// (G-36)
    subW = ( ( chromaFlag == 0 ) ? 1 : pc_sps->SubWidthC );// (G-37)
    subH = ( ( chromaFlag == 0 ) ? 1 : pc_sps->SubHeightC );// (G-38)

    if (!pc_layer->RefLayerFrameMbsOnlyFlag || !pc_sps->frame_mbs_only_flag) {
        if (pc_layer->RefLayerFrameMbsOnlyFlag) {
            phaseY = phaseY + 4 * botFieldFlag + 3 - subH;// (G-39)
            refPhaseY = 2 * refPhaseY + 2;// (G-40)
        }
        else {
            phaseY = phaseY + 4 * botFieldFlag;// (G-41)
            refPhaseY = refPhaseY + 4 * botFieldFlag;// (G-42)
        }
    }

    shiftX = ( ( levelIdc <= 30 ) ? 16 : ( 31 - (int32_t)HL_MATH_CEIL( HL_MATH_LOG2( refW ) ) ) );// (G-43)
    shiftY = ( ( levelIdc <= 30 ) ? 16 : ( 31 - (int32_t)HL_MATH_CEIL( HL_MATH_LOG2( refH ) ) ) );// (G-44)

    scaleX = ( ( refW << shiftX ) + ( scaledW >> 1 ) ) / scaledW;// (G-45)
    scaleY = ( ( refH << shiftY ) + ( scaledH >> 1 ) ) / scaledH;// (G-46)

    offsetX = pc_slice_header->ext.svc.ScaledRefLayerLeftOffset / subW;// (G-47)
    addX = ( ( ( refW * ( 2 + phaseX ) ) << ( shiftX - 2 ) ) + ( scaledW >> 1 ) ) / scaledW + ( 1 << ( shiftX - 5 ) ); // (G-48)
    deltaX = 4 * ( 2 + refPhaseX );// (G-49)

    if (pc_layer->RefLayerFrameMbsOnlyFlag && pc_sps->frame_mbs_only_flag) {
        offsetY = pc_slice_header->ext.svc.ScaledRefLayerTopOffset / subH;// (G-50)
        addY = ( ( ( refH * ( 2 + phaseY ) ) << ( shiftY - 2 ) ) + ( scaledH >> 1 ) ) / scaledH + ( 1 << ( shiftY - 5 ) );// (G-51)
        deltaY = 4 * ( 2 + refPhaseY );// (G-52)
    }
    else {
        offsetY = pc_slice_header->ext.svc.ScaledRefLayerTopOffset / ( 2 * subH ); // (G-53)
        addY = ( ( ( refH * ( 2 + phaseY ) ) << ( shiftY - 3 ) ) + ( scaledH >> 1 ) ) / scaledH + ( 1 << ( shiftY - 5 ) ); // (G-54)
        deltaY = 2 * ( 2 + refPhaseY ); // (G-55)
    }

    // 6.4.1 Inverse macroblock scanning process
    xM = pc_mb->xL;
    yM = pc_mb->yL;

    xC = xP + ( xM >> ( subW - 1 ) ); // (G-56)
    yC = yP + ( yM >> ( subH - 1 + pc_mb->ext.svc.fieldMbFlag - pc_slice_header->field_pic_flag ) ); // (G-57)

    if (!pc_layer->RefLayerFrameMbsOnlyFlag || !pc_sps->frame_mbs_only_flag) {
        yC = yC >> ( 1 - pc_mb->ext.svc.fieldMbFlag ); // (G-58)
    }

    *xRef16 = ( ( ( xC - offsetX ) * scaleX + addX ) >> ( shiftX - 4 ) ) - deltaX; // (G-59)
    *yRef16 = ( ( ( yC - offsetY ) * scaleY + addY ) >> ( shiftY - 4 ) ) - deltaY; // (G-60)
}

// FIXME: macro
// G.6.4 SVC derivation process for macroblock and sub-macroblock partition indices
void	hl_codec_264_utils_derivation_process_for_mb_and_submb_partition_indices_svc(
    const hl_codec_264_t* pc_codec,
    const hl_codec_264_mb_t *pc_mb,
    int32_t currDQId,
    int32_t xP, int32_t yP, // input luma location
    int32_t *mbPartIdx,
    int32_t *subMbPartIdx
)
{
    int32_t svcDirectModeFlag = 0;
    if (currDQId > 0 && ((HL_CODEC_264_MB_TYPE_IS_B_SKIP(pc_mb) || HL_CODEC_264_MB_TYPE_IS_B_DIRECT_16X16(pc_mb)) || ( HL_CODEC_264_MB_TYPE_IS_B_8X8(pc_mb) &&  HL_CODEC_264_SUBMB_TYPE_IS_B_DIRECT_8X8(pc_mb, (((yP>>3)<<1)+(xP>>3)))))) {
        svcDirectModeFlag = 1;
    }
    if (svcDirectModeFlag == 0) {
        // 6.4.12.4 Derivation process for macroblock and sub-macroblock partition indices
        hl_codec_264_mb_get_sub_partition_indices(pc_mb, xP, yP, mbPartIdx, subMbPartIdx);
    }
    else if (HL_CODEC_264_MB_TYPE_IS_B_SKIP(pc_mb) || HL_CODEC_264_MB_TYPE_IS_B_DIRECT_16X16(pc_mb)) {
        *mbPartIdx = 0;
        *subMbPartIdx = 0;
    }
    else {
        *mbPartIdx =  ((yP >> 3) << 1) + (xP >> 3);
        *subMbPartIdx  = 0;
    }
}

// G.8.1.2.1 Array assignment and initialisation process
HL_ERROR_T	hl_codec_264_utils_array_assignment_and_initialisation_svc(
    struct hl_codec_264_s* pc_codec
)
{
    uint32_t u;
    struct hl_codec_264_mb_s* pc_mb;
    struct hl_codec_264_layer_s* pc_layer = pc_codec->layers.pc_active;
    for (u = 0; u < pc_layer->pc_slice_hdr->PicSizeInMbs; ++u) {
        pc_mb = pc_layer->pp_list_macroblocks[u];

        pc_mb->ext.svc.sliceIdc = -1;
        pc_mb->ext.svc.fieldMbFlag = -1;
        pc_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_UNSPECIFIED;
        pc_mb->ext.svc.baseModeFlag = -1;
        pc_mb->ext.svc.mvCnt = 0;
        pc_mb->ext.svc.tQPy = 0;
        pc_mb->ext.svc.tQPCb = 0;
        pc_mb->ext.svc.tQPCr = 0;
        pc_mb->predFlagL0[0] = pc_mb->predFlagL0[1] = pc_mb->predFlagL0[2] = pc_mb->predFlagL0[3] = 0;
        pc_mb->predFlagL1[0] = pc_mb->predFlagL1[1] = pc_mb->predFlagL1[2] = pc_mb->predFlagL1[3] = 0;
        pc_mb->refIdxL0[0] = pc_mb->refIdxL0[1] = pc_mb->refIdxL0[2] = pc_mb->refIdxL0[3] = 0;
        pc_mb->refIdxL1[0] = pc_mb->refIdxL1[1] = pc_mb->refIdxL1[2] = pc_mb->refIdxL1[3] = 0;
        hl_memory_set((int32_t*)pc_mb->mvL0, 16, 0);
        hl_memory_set((int32_t*)pc_mb->mvL1, 16, 0);
        hl_memory_set((int32_t*)pc_mb->ext.svc.tCoeffLevel, (sizeof(pc_mb->ext.svc.tCoeffLevel)/sizeof(pc_mb->ext.svc.tCoeffLevel[0])), 0);
        hl_memory_set((int32_t*)pc_mb->ext.svc.sTCoeff, (sizeof(pc_mb->ext.svc.sTCoeff)/sizeof(pc_mb->ext.svc.sTCoeff[0])), 0);
    }

    // FIXME: more to do?
    // rSL, rSCb, rSCr, cSL, cSCb and cSCr


    return HL_ERROR_SUCCESS;
}

// G.8.1.5.1 Macroblock initialisation process
HL_ERROR_T	hl_codec_264_utils_derivation_process_initialisation_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
)
{
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    int32_t currDQId = pc_codec->layers.currDQId;
    const hl_codec_264_mb_t* pc_mb_ref = pc_layer->SpatialResolutionChangeFlag ? HL_NULL : (pc_layer->pc_ref ? pc_layer->pc_ref->pp_list_macroblocks[p_mb->u_addr] : HL_NULL);
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    /* G.8.1.5.1 Macroblock initialisation process */
    p_mb->ext.svc.sliceIdc = ((pc_slice_header->first_mb_in_slice << 7) + pc_codec->layers.currDQId);
    p_mb->ext.svc.baseModeFlag = p_mb->ext.svc.base_mode_flag;
    p_mb->ext.svc.fieldMbFlag = pc_slice_header->field_pic_flag ? pc_slice_header->field_pic_flag :
                                ( (!pc_layer->SpatialResolutionChangeFlag && pc_slice_header->ext.svc.slice_skip_flag && pc_mb_ref)
                                  ? pc_mb_ref->ext.svc.fieldMbFlag : p_mb->mb_field_decoding_flag );

    if (pc_layer->DQId == 16 && p_mb->u_addr == 56 && pc_layer->i_pict_decode_count == 5) { // FIXME
        int a = 0;
#define MEDIAN(a,b,c)  ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b))
        a = MEDIAN(-1, 126, 126);
        ++a;
        printf("%d", a);
    }

    // G.8.1.2.1 Array assignment and initialisation process (Not part of the standard)
    // hl_memory_set((int32_t*)p_mb->ext.svc.tCoeffLevel, (sizeof(p_mb->ext.svc.tCoeffLevel)/sizeof(p_mb->ext.svc.tCoeffLevel[0])), 0);
    // hl_memory_set((int32_t*)p_mb->ext.svc.sTCoeff, (sizeof(p_mb->ext.svc.sTCoeff)/sizeof(p_mb->ext.svc.sTCoeff[0])), 0);

    /* G.8.1.5.1.1 Derivation process for macroblock type, sub-macroblock type, and inter-layer predictors for
    		reference indices and motion vectors
       Outputs: "mbType", "subMbType", "refIdxILPredL0", "refIdxILPredL1", "mvILPredL0" and "mvILPredL1"
    */
    {
        if (p_mb->ext.svc.base_mode_flag ||
                (p_mb->ext.svc.motion_prediction_flag_lX[0][0] || p_mb->ext.svc.motion_prediction_flag_lX[0][1] || p_mb->ext.svc.motion_prediction_flag_lX[0][2] || p_mb->ext.svc.motion_prediction_flag_lX[0][3]) ||
                (p_mb->ext.svc.motion_prediction_flag_lX[1][0] || p_mb->ext.svc.motion_prediction_flag_lX[1][1] || p_mb->ext.svc.motion_prediction_flag_lX[1][2] || p_mb->ext.svc.motion_prediction_flag_lX[1][3])) {
            /* G.8.6.1 Derivation process for inter-layer predictors for macroblock type, sub-macroblock type, reference
            	indices, and motion vectors.
            	This process is only invoked when base_mode_flag is equal to 1 or any motion_prediction_flag_lX[ mbPartIdx ] with X
            	being replaced by 0 and 1 and mbPartIdx = 0..3 is equal to 1.
            */
            /* G.8.6.1.1 Derivation process for reference layer partition identifications
            	Outputs: "intraILPredFlag" and "refLayerPartIdc"
            */
            err = hl_codec_264_utils_derivation_process_for_ref_layer_partition_identifications_svc(pc_codec, p_mb);
            if (err) {
                return err;
            }
            if (p_mb->ext.svc.intraILPredFlag == 1) {
                p_mb->ext.svc.refIdxILPredL0[0][0] = p_mb->ext.svc.refIdxILPredL0[0][1] = p_mb->ext.svc.refIdxILPredL0[1][0] = p_mb->ext.svc.refIdxILPredL0[1][1] = -1; // not avail
                p_mb->ext.svc.refIdxILPredL1[0][0] = p_mb->ext.svc.refIdxILPredL1[0][1] = p_mb->ext.svc.refIdxILPredL1[1][0] = p_mb->ext.svc.refIdxILPredL1[1][1] = -1; // not avail
                hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL0, 32, 0);
                hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL1, 32, 0);

                p_mb->ext.svc.subMbTypeILPred[0] = p_mb->ext.svc.subMbTypeILPred[1] = p_mb->ext.svc.subMbTypeILPred[2] = p_mb->ext.svc.subMbTypeILPred[3] = -1; // unspecified
                if (pc_slice_header->ext.svc.tcoeff_level_prediction_flag == 1) {
                    p_mb->ext.svc.mbTypeILPred = pc_mb_ref->e_type;
                    p_mb->ext.svc.mb_type_il_pred = pc_mb_ref->mb_type;
                }
                else {
                    p_mb->ext.svc.mbTypeILPred = HL_CODEC_264_MB_TYPE_SVC_I_BL;
                    p_mb->ext.svc.mb_type_il_pred = I_BL;
                }
            }
            else {
                if (pc_layer->DQId) { // FIXME
                    int a = 0;
                }
                /* G.8.6.1.2 Derivation process for inter-layer predictors for reference indices and motion vectors
                	Outputs: "refIdxILPredL0", "refIdxILPredL1", "mvILPredL0" and "mvILPredL1"
                */
                hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_ref_indices_and_mvs_svc(pc_codec, p_mb);
                /*  G.8.6.1.3 Derivation process for inter-layer predictors for P and B macroblock and sub-macroblock types
                	Outputs: "mbTypeILPred", "subMbTypeILPred"
                */
                hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_P_and_B_mb_and_submb_types_svc(pc_codec, p_mb);
            }
        }
        else {
            p_mb->ext.svc.mbTypeILPred = -1; // not avail
            p_mb->ext.svc.subMbTypeILPred[0] = p_mb->ext.svc.subMbTypeILPred[1] = p_mb->ext.svc.subMbTypeILPred[2] = p_mb->ext.svc.subMbTypeILPred[3] = -1; // not avail
            p_mb->ext.svc.refIdxILPredL0[0][0] = p_mb->ext.svc.refIdxILPredL0[0][1] = p_mb->ext.svc.refIdxILPredL0[1][0] = p_mb->ext.svc.refIdxILPredL0[1][1] = -1; // not avail
            p_mb->ext.svc.refIdxILPredL1[0][0] = p_mb->ext.svc.refIdxILPredL1[0][1] = p_mb->ext.svc.refIdxILPredL1[1][0] = p_mb->ext.svc.refIdxILPredL1[1][1] = -1; // not avail
            hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL0, 32, 0);
            hl_memory_set((int32_t*)p_mb->ext.svc.mvILPredL1, 32, 0);
        }

        if (p_mb->ext.svc.base_mode_flag == 1) {
            if (pc_layer->SpatialResolutionChangeFlag == 0 && pc_mb_ref->e_type == HL_CODEC_264_MB_TYPE_I_PCM && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0) {
                p_mb->e_type = HL_CODEC_264_MB_TYPE_I_PCM;
                p_mb->mb_type = I_PCM;
                p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_PCM;
            }
            else {
                p_mb->e_type = p_mb->ext.svc.mbTypeILPred;
                if (p_mb->ext.svc.intraILPredFlag == 1) {
                    p_mb->mb_type = p_mb->ext.svc.mb_type_il_pred;
                    p_mb->flags_type = (HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA :
                                        (HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4 :
                                         (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8 :
                                          (HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16 :
                                           (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_PCM :
                                            (HL_CODEC_264_MB_TYPE_FLAGS_INTRA))))));
                }
                else {
#if 1
                    err = hl_codec_264_mb_set_mb_type(p_mb, p_mb->ext.svc.mb_type_il_pred, pc_codec);
                    if (err) {
                        return err;
                    }
#else
                    p_mb->e_type = p_mb->ext.svc.mbTypeILPred;
                    p_mb->mb_type = p_mb->ext.svc.mb_type_il_pred;
                    // Update Flags
                    p_mb->flags_type = (HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4 :
                                        (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8 :
                                         (HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16 :
                                          (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_PCM :
                                           (HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb) ? HL_CODEC_264_MB_TYPE_FLAGS_INTRA :
                                            (HL_CODEC_264_MB_TYPE_FLAGS_INTER))))));
                    if (p_mb->ext.svc.subMbTypeILPred[0] == -1) { // "sub_mb_type" avail?
                        err = hl_codec_264_mb_set_sub_mb_type_when_not_present(p_mb, pc_codec);
                        if (err) {
                            return err;
                        }
                    }
#endif
                }
            }
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_4X4;
            p_mb->mb_type = I_4x4;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4;
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_8X8;
            p_mb->mb_type = I_8x8;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8;
        }
        else if (HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
            p_mb->e_type = HL_CODEC_264_MB_TYPE_SVC_I_16X16;
            p_mb->mb_type = I_16x16;
            p_mb->flags_type = HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16;
        }
        else {
            // mbType = "mb_type"
            // alreay done in mb_init()
        }

        if (pc_layer->DQId > 0) {
            // FIXME: make sure "MbPartPredMode[PartIdx]", "NumMbPart", "MbPartWidth" and "MbPartHeight" from the Macroblock are correct at this step.

            if (!HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb) && !HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) && !HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb)) {
                p_mb->SubMbPredType[0] = p_mb->SubMbPredType[1] = p_mb->SubMbPredType[2] = p_mb->SubMbPredType[3] = -1;
                // (P/B)_SKI contains "inferred" MB_TYPE "unusable" with "hl_codec_264_mb_set_sub_mb_type_when_not_present"
                if (!HL_CODEC_264_MB_TYPE_IS_SKIP(p_mb) && !HL_CODEC_264_MB_TYPE_IS_INTRA(p_mb)) {
                    err = hl_codec_264_mb_set_sub_mb_type_when_not_present(p_mb, pc_codec);
                    if (err) {
                        return err;
                    }
                }
            }
            else if (p_mb->ext.svc.base_mode_flag) {
                p_mb->SubMbPredType[0] = p_mb->ext.svc.subMbTypeILPred[0];
                p_mb->SubMbPredType[1] = p_mb->ext.svc.subMbTypeILPred[1];
                p_mb->SubMbPredType[2] = p_mb->ext.svc.subMbTypeILPred[2];
                p_mb->SubMbPredType[3] = p_mb->ext.svc.subMbTypeILPred[3];
                err = hl_codec_264_mb_set_sub_mb_type(p_mb, &p_mb->ext.svc.sub_mb_type_il_pred, pc_codec);
                if (err) {
                    return err;
                }
            }
        }
    } // End-of-G.8.1.5.1.1

    /* G.8.1.5.1.2 Derivation process for quantisation parameters and transform type
    	Outputs: "cTrafo", "tQPY", "tQPCb" and "tQPCr".
    */
    {
        // Setting "tQPY"
        if (
            pc_layer->SpatialResolutionChangeFlag == 0 &&
            (
                ((HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) && p_mb->ext.svc.base_mode_flag == 1 && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0)
                ||
                ((HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb)) && p_mb->ext.svc.residual_prediction_flag  == 1)
                ||
                ((HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb) || HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb)) && p_mb->ext.svc.residual_prediction_flag && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0)
            )
        ) {
            p_mb->ext.svc.tQPy = pc_mb_ref->QPy;
        }
        else {
            p_mb->ext.svc.tQPy = p_mb->QPy;
        }

        // Setting "tQPcb" and "tQPcr"
        if (pc_slice_header->pc_pps->pc_sps->ChromaArrayType != 0) {
            if (p_mb->ext.svc.tQPy == p_mb->QPy) {
                // 8.5.8 already compiuted using QPy == tQPy
                p_mb->ext.svc.tQPCb = p_mb->QPc[0];
                p_mb->ext.svc.tQPCr = p_mb->QPc[1];
            }
            else {
                // 8.5.8 Derivation process for chroma quantisation parameters
                // "8.5.8" is called with input "QPy" equal to "tQPy"
                int32_t qPOffsetCb = pc_slice_header->pc_pps->chroma_qp_index_offset; // (8-313)
                int32_t qPOffsetCr = pc_slice_header->pc_pps->second_chroma_qp_index_offset; // (8-314)
                int32_t qPICb = HL_MATH_CLIP3(-pc_slice_header->pc_pps->pc_sps->QpBdOffsetC, 51, p_mb->ext.svc.tQPy + qPOffsetCb); // (8-315)
                int32_t qPICr = HL_MATH_CLIP3(-pc_slice_header->pc_pps->pc_sps->QpBdOffsetC, 51, p_mb->ext.svc.tQPy + qPOffsetCr); // (8-315)
                p_mb->ext.svc.tQPCb = qPI2QPC[qPICb];
                p_mb->ext.svc.tQPCr = qPI2QPC[qPICr];
            }
        }

        // Setting "predTrafoFlag"
        p_mb->ext.svc.predTrafoFlag = (
                                          pc_layer->SpatialResolutionChangeFlag == 0 &&
                                          (
                                              (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref) && p_mb->CodedBlockPatternLuma == 0)
                                              ||
                                              (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) && p_mb->CodedBlockPatternLuma == 0 && p_mb->CodedBlockPatternChroma == 0)
                                              ||
                                              (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && (HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref)) && p_mb->CodedBlockPatternLuma == 0)
                                              ||
                                              (p_mb->ext.svc.base_mode_flag && pc_slice_header->ext.svc.tcoeff_level_prediction_flag && (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref)))
                                              ||
                                              (p_mb->ext.svc.residual_prediction_flag && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)) && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) && p_mb->CodedBlockPatternLuma == 0)
                                          )
                                      );

        // Setting "cTrafo"
        if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)) {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_PCM;
        }
        else if (HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb)) {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_16X16;
        }
        else if (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || p_mb->transform_size_8x8_flag) {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_8X8;
        }
        else if (p_mb->ext.svc.predTrafoFlag) {
            p_mb->ext.svc.cTrafo = pc_mb_ref->ext.svc.cTrafo;
        }
        else {
            p_mb->ext.svc.cTrafo = HL_CODEC_264_TRANSFORM_TYPE_4X4;
        }
#if 0
        {
            int32_t constrainedCoeffFlag = (
                                               pc_layer->SpatialResolutionChangeFlag == 0 &&
                                               (p_mb->ext.svc.base_mode_flag && !pc_slice_header->ext.svc.tcoeff_level_prediction_flag && HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)
                                                ||
                                                (p_mb->ext.svc.residual_prediction_flag && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)) && !(HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)))
                                               );
                                               // TODO: check "constrainedCoeffFlag"
        }
#endif

                                       // The variable mvCnt is set equal to 0.
                                       p_mb->ext.svc.mvCnt = 0;

    } // End-of-G.8.1.5.1.2

    return err;
}


// G.8.4.1 SVC derivation process for motion vector components and reference indices
// Outputs: "predFlagL0", "predFlagL1", "refIdxL0", "refIdxL1", "mvL0", "mvL1" and "mvCnt"
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_mv_comps_and_ref_indices_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) ||
            HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ||
            HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) ||
            HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) ||
            HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) {
        int32_t i;
        // (G-89)
        p_mb->predFlagL0[0] = p_mb->predFlagL0[1] = p_mb->predFlagL0[2] = p_mb->predFlagL0[3] = 0;
        p_mb->predFlagL1[0] = p_mb->predFlagL1[1] = p_mb->predFlagL1[2] = p_mb->predFlagL1[3] = 0;
        // (G-90)
        p_mb->refIdxL0[0] = p_mb->refIdxL0[1] = p_mb->refIdxL0[2] = p_mb->refIdxL0[3] = -1;
        p_mb->refIdxL1[0] = p_mb->refIdxL1[1] = p_mb->refIdxL1[2] = p_mb->refIdxL1[3] = -1;
        // (G-91)
        for (i = 0; i < 4; ++i) {
            p_mb->mvL0[i][0].x = p_mb->mvL0[i][1].x = p_mb->mvL0[i][2].x = p_mb->mvL0[i][3].x = 0;
            p_mb->mvL0[i][0].y = p_mb->mvL0[i][1].y = p_mb->mvL0[i][2].y = p_mb->mvL0[i][3].y = 0;
            p_mb->mvL1[i][0].x = p_mb->mvL1[i][1].x = p_mb->mvL1[i][2].x = p_mb->mvL1[i][3].x = 0;
            p_mb->mvL1[i][0].y = p_mb->mvL1[i][1].y = p_mb->mvL1[i][2].y = p_mb->mvL1[i][3].y = 0;
        }
        // (G-92)
        p_mb->ext.svc.mvCnt = 0;
    }
    else {
        int32_t numMbPart, numSubMbPart, mbPartIdx, subMbPartIdx, isDirectFlag, isBSkipOrBDirect16x16, isB8x8;
        const hl_codec_264_layer_t* pc_layer_active = pc_codec->layers.pc_active;
        const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer_active->pc_slice_hdr;

        if (pc_layer_active->DQId) { // FIXME
            int a = 0;
        }

        isBSkipOrBDirect16x16 = HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_DIRECT_16X16(p_mb);
        isB8x8 = HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb);
        numMbPart = isBSkipOrBDirect16x16 ? (pc_layer_active->DQId == 0 ? 4 : 1) : p_mb->NumMbPart;

        for (mbPartIdx = 0; mbPartIdx < numMbPart; ++mbPartIdx) {
            isDirectFlag = isBSkipOrBDirect16x16 || (isB8x8 && HL_CODEC_264_SUBMB_TYPE_IS_B_DIRECT_8X8(p_mb, mbPartIdx));
            numSubMbPart = isDirectFlag ? (pc_layer_active->DQId == 0 ? 4 : 1) : p_mb->NumSubMbPart[mbPartIdx];
            for (subMbPartIdx = 0; subMbPartIdx < numSubMbPart; ++subMbPartIdx) {
                // G.8.4.1.1 SVC derivation process for luma motion vector components and reference indices of a macroblock or sub-macroblock partition
                err = hl_codec_264_utils_derivation_process_for_luma_vectcomps_and_ref_indices_of_mb_and_submb_partition(pc_codec, p_mb, mbPartIdx, subMbPartIdx, isDirectFlag);
                if (err) {
                    return err;
                }
            }
        }
    }

    return err;
}

// G.8.4.1.1 SVC derivation process for luma motion vector components and reference indices of a macroblock or sub-macroblock partition
// Outputs: "predFlagL0", "predFlagL1", "refIdxL0", "refIdxL1", "mvL0", "mvL1" and "mvCnt"
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_luma_vectcomps_and_ref_indices_of_mb_and_submb_partition(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    int32_t isDirectFlag
)
{
    int32_t mvCntInc;
    const hl_codec_264_layer_t* pc_layer_active = pc_codec->layers.pc_active;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    // FIXME:
    // In subclause 6.4.7, a macroblock with address mbAddr is treated to belong to a different slice than the current
    // macroblock CurrMbAddr, when sliceIdc[ mbAddr ] is not equal to sliceIdc[ CurrMbAddr ].

    if (HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb)) {
        p_mb->refIdxL1[mbPartIdx] = -1;
        p_mb->predFlagL0[mbPartIdx] = 1;
        p_mb->predFlagL1[mbPartIdx] = 0;
        p_mb->mvL1[mbPartIdx][subMbPartIdx].x = p_mb->mvL1[mbPartIdx][subMbPartIdx].y = 0;
        mvCntInc = 1;
        // 8.4.1.1 Derivation process for luma motion vectors for skipped macroblocks in P and SP slices
        hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(pc_codec, p_mb, &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->refIdxL0[mbPartIdx]);
    }
    else if (isDirectFlag == 1) {
        if (pc_layer_active->DQId == 0) {
            // FIXME: 8.4.1.2.2
            HL_DEBUG_ERROR("'8.4.1.2.2' not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
        else {
            // FIXME: G.8.4.1.2
            HL_DEBUG_ERROR("'G.8.4.1.2' not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
    }
    else {
        hl_codec_264_mv_xt mvPredL0, mvPredL1;
        int32_t xP, yP, *refIdxPredLX, motion_prediction_flag_lX, refIdxPredL0, refIdxPredL1, X;
        HL_CODEC_264_MB_MODE_T Pred_LX;
        HL_CODEC_264_SUBMB_MODE_T SubPred_LX;
        hl_codec_264_mv_xt* mvPredLX;
        mvCntInc = 0;
        for (X = 0; X < 2; ++X) {
            Pred_LX = (X == 0) ? HL_CODEC_264_MB_MODE_PRED_L0 : HL_CODEC_264_MB_MODE_PRED_L1;
            SubPred_LX = (X == 0) ? HL_CODEC_264_SUBMB_MODE_PRED_L0 : HL_CODEC_264_SUBMB_MODE_PRED_L1;
            mvPredLX = (X == 0) ? &mvPredL0 : &mvPredL1;
            refIdxPredLX = (X == 0) ? &refIdxPredL0 : &refIdxPredL1;
            motion_prediction_flag_lX = p_mb->ext.svc.motion_prediction_flag_lX[X][mbPartIdx];
            if (
                (!(HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb)) && !(p_mb->MbPartPredMode[mbPartIdx] == Pred_LX || p_mb->MbPartPredMode[mbPartIdx] == HL_CODEC_264_MB_MODE_BIPRED))
                ||
                ((HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb)) && !(p_mb->SubMbPredMode[mbPartIdx] == SubPred_LX || p_mb->SubMbPredMode[mbPartIdx] == HL_CODEC_264_SUBMB_MODE_BIPRED))
            ) {
                *refIdxPredLX = -1;
                mvPredLX->x = mvPredLX->y = 0;
            }
            else if (p_mb->ext.svc.base_mode_flag == 1 || motion_prediction_flag_lX == 1) {
                int32_t xS, yS;
                hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;
                // 6.4.2.1 - upper-left sample of the macroblock partition
                // FIXME: Make sure "MbPartWidth" and "MbPartHeight" are correctly defined.
                xP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);// (6-11)
                yP = InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);// (6-12)
                // 6.4.2.2 - upper-left sample of the sub-macroblock partition
                hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);

                *refIdxPredLX = (X == 0) ? p_mb->ext.svc.refIdxILPredL0[(yP + yS) >> 3][(xP + xS) >> 3] : p_mb->ext.svc.refIdxILPredL1[(yP + yS) >> 3][(xP + xS) >> 3];
                mvPredLX->x = (*mvILPredLX)[(yP + yS)>>2][(xP+xS)>>2][0]; // (G-93)
                mvPredLX->y = (*mvILPredLX)[(yP + yS)>>2][(xP+xS)>>2][1]; // (G-93)
                ++mvCntInc;
            }
            else {
                *refIdxPredLX = HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) ? 0 : (X == 0 ? p_mb->ref_idx_l0[mbPartIdx] : p_mb->ref_idx_l1[mbPartIdx]);
                // 8.4.1.3 Derivation process for luma motion vector prediction
                err = hl_codec_264_utils_derivation_process_for_luma_movect_prediction(
                          pc_codec,
                          p_mb,
                          mbPartIdx,
                          subMbPartIdx,
                          *refIdxPredLX,
                          p_mb->SubMbPredType[mbPartIdx], // FIXME: not correctly set in svc_init()
                          mvPredLX,
                          (X == 0) ? HL_CODEC_264_LIST_IDX_0 : HL_CODEC_264_LIST_IDX_1);
                if (err) {
                    return err;
                }
                ++mvCntInc;
            }

            if (subMbPartIdx == 0) {
                ((X == 0) ? p_mb->refIdxL0 : p_mb->refIdxL1)[mbPartIdx] = *refIdxPredLX; // (G-94)
                ((X == 0) ? p_mb->predFlagL0 : p_mb->predFlagL1)[mbPartIdx] = ((*refIdxPredLX < 0) ? 0 : 1); // (G-95)
            }

            ((X == 0) ? p_mb->mvL0 : p_mb->mvL1)[mbPartIdx][subMbPartIdx] = *mvPredLX; // (G-96)

            if (((X == 0) ? p_mb->predFlagL0 : p_mb->predFlagL1)[mbPartIdx] == 1 && p_mb->ext.svc.base_mode_flag == 0 && isDirectFlag == 0 && !HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb)) {
                ((X == 0) ? p_mb->mvL0 : p_mb->mvL1)[mbPartIdx][subMbPartIdx].x += ((X == 0) ? p_mb->mvd_l0 : p_mb->mvd_l1)[mbPartIdx][subMbPartIdx].x; // (G-97)
                ((X == 0) ? p_mb->mvL0 : p_mb->mvL1)[mbPartIdx][subMbPartIdx].y += ((X == 0) ? p_mb->mvd_l0 : p_mb->mvd_l1)[mbPartIdx][subMbPartIdx].y; // (G-97)
            }
        }
    }

    if (mbPartIdx == 0 && subMbPartIdx == 0) {
        p_mb->ext.svc.mvCnt = mvCntInc;
    }
    else {
        p_mb->ext.svc.mvCnt += mvCntInc;
    }

    return err;
}


// G.8.6.1.1 Derivation process for reference layer partition identifications
// Outputs: "intraILPredFlag" and "refLayerPartId"
// JVSM: MotionUpsampling::xSetPartIdcArray()
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_partition_identifications_svc(
    const hl_codec_264_t* pc_codec,
    hl_codec_264_mb_t *p_mb
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t x, y, xP, yP, refMbAddr, refMbPartIdx, refSubMbPartIdx;
    const hl_codec_264_layer_t* pc_layer_active = pc_codec->layers.pc_active;
    const hl_codec_264_layer_t* pc_layer_ref = pc_layer_active->pc_ref;
    const hl_codec_264_mb_t *pc_mb_ref;

    p_mb->ext.svc.intraILPredFlag = 1;

    // ForEach (4x4 blocks)
    for (y = 0; y < 4; ++y) {
        yP = (y << 2) + 1;
        for (x = 0; x < 4; ++x) {
            xP = (x << 2) + 1;
            // G.6.2 Derivation process for reference layer partitions
            err = hl_codec_264_utils_derivation_process_for_ref_layer_partitions_svc(pc_codec, p_mb, xP, yP, &refMbAddr, &refMbPartIdx, &refSubMbPartIdx);
            if (err) {
                return err;
            }
            pc_mb_ref = pc_layer_ref->pp_list_macroblocks[refMbAddr];
            if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref) || HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref)) {
                p_mb->ext.svc.refLayerPartIdc[y][x] = -1;
            }
            else {
                p_mb->ext.svc.refLayerPartIdc[y][x] = (refMbAddr << 4) + (refMbPartIdx << 2) + refSubMbPartIdx; // (G-209)
                p_mb->ext.svc.intraILPredFlag = 0;
            }
        }
    }

    if (!p_mb->ext.svc.intraILPredFlag && !pc_layer_active->RestrictedSpatialResolutionChangeFlag) {
        int32_t xO, yO, xS, yS;
        int32_t procI4x4Blk[2/*yS*/][2/*xS*/] = {0};
        int32_t procI8x8Blk[2/*yP*/][2/*xP*/] = {0};
        // ForEach (8x8 blocks)
        for (yP = 0; yP < 2; ++yP) {
            yO = (yP << 1);
            for (xP = 0; xP < 2; ++xP) {
                xO = (xP << 1);
                // ForEach (4x4 blocks) in the current 8x8 block
                for (yS = 0; yS < 2; ++yS) {
                    for (xS = 0; xS < 2; ++xS) {
                        if (p_mb->ext.svc.refLayerPartIdc[yO + yS][xO + xS] == -1) {
                            procI4x4Blk[yS][xS] = 1;
                            if (procI4x4Blk[yS][1 - xS] == 0 && p_mb->ext.svc.refLayerPartIdc[yO + yS][xO + 1] == -1) {
                                p_mb->ext.svc.refLayerPartIdc[yO + yS][xO + xS] = p_mb->ext.svc.refLayerPartIdc[yO + yS][xO + 1 - xS]; // (G-210)
                            }
                            else if (procI4x4Blk[1 - yS][xS] == 0 && p_mb->ext.svc.refLayerPartIdc[yO + 1 - yS][xO + xS] != -1) {
                                p_mb->ext.svc.refLayerPartIdc[yO + yS][xO + xS] = p_mb->ext.svc.refLayerPartIdc[yO + 1 - yS][xO + xS]; // (G-211)
                            }
                            else if (procI4x4Blk[1 - yS][1 - xS] == 0 && p_mb->ext.svc.refLayerPartIdc[yO + 1 - yS][xO + 1 - xS] != -1) {
                                p_mb->ext.svc.refLayerPartIdc[yO + yS][xO + xS] = p_mb->ext.svc.refLayerPartIdc[yO + 1 - yS][xO + 1 - xS]; // (G-212)
                            }
                            // Otherwise, the element refLayerPartIdc[ xO + xS, yO + yS ] is not modified.
                        }
                    }
                }
            }
        }

        // ForEach (8x8 blocks)
        for (yP = 0; yP < 2; ++yP) {
            for (xP = 0; xP < 2; ++xP) {
                if (p_mb->ext.svc.refLayerPartIdc[yP << 1][xP << 1] == -1) {
                    procI8x8Blk[yP][xP] = 1;

                    if (procI8x8Blk[yP][1 - xP] == 0 && p_mb->ext.svc.refLayerPartIdc[yP << 1][2 - xP] != -1) {
                        for (yS = 0; yS < 2; ++yS) {
                            for (xS = 0; xS < 2; ++xS) {
                                p_mb->ext.svc.refLayerPartIdc[(yP << 1) + yS][(xP << 1) + xS] = p_mb->ext.svc.refLayerPartIdc[(yP << 1) + yS][2 - xP]; // (G-213)
                            }
                        }
                    }
                    else if (procI8x8Blk[1 - yP][xP] == 0 && p_mb->ext.svc.refLayerPartIdc[2 - yP][xP << 1] != -1) {
                        for (yS = 0; yS < 2; ++yS) {
                            for (xS = 0; xS < 2; ++xS) {
                                p_mb->ext.svc.refLayerPartIdc[(yP << 1) + yS][(xP << 1) + xS] = p_mb->ext.svc.refLayerPartIdc[2 - yP][(xP << 1) + xS]; // (G-214)
                            }
                        }
                    }
                    else if (procI8x8Blk[1 - yP][1 - xP] == 0 && p_mb->ext.svc.refLayerPartIdc[2 - yP][2 - xP] != -1) {
                        for (yS = 0; yS < 2; ++yS) {
                            for (xS = 0; xS < 2; ++xS) {
                                p_mb->ext.svc.refLayerPartIdc[(yP << 1) + yS][(xP << 1) + xS] = p_mb->ext.svc.refLayerPartIdc[2 - yP][2 - xP]; // (G-215)
                            }
                        }
                    }
                    // Otherwise, the elements refLayerPartIdc[ 2 * xP + xS, 2 * yP + yS ] with xS, yS = 0..1 are not modified.
                }
            }
        }
    }

    return err;
}

// G.8.6.1.2 Derivation process for inter-layer predictors for reference indices and motion vectors
// Outputs: "refIdxILPredL0", "refIdxILPredL1", "mvILPredL0" and "mvILPredL1"
// JVSM: "MotionUpsampling::xGetRefIdxAndInitialMvPred()"
void	hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_ref_indices_and_mvs_svc(
    const hl_codec_264_t* pc_codec,
    hl_codec_264_mb_t *p_mb
)
{
    int32_t refMbAddr, refMbPartIdx, refSubMbPartIdx, x, y, X, xP, yP, xS, yS, xO, yO;
    int32_t tempRefIdxPredL0[4][4], tempRefIdxPredL1[4][4], tempMv[2], maxX, tempMvALX[2], tempMvBLX[2];
    hl_codec_264_mv_xt aMv;
    HL_CODEC_264_SUBPART_SIZE_T subPartSize;
    const hl_codec_264_layer_t* pc_layer_active = pc_codec->layers.pc_active;
    const hl_codec_264_layer_t* pc_layer_ref = pc_layer_active->pc_ref;
    const hl_codec_264_mb_t *pc_mb_ref;

    // ForEach (4x4 block)
    for (y = 0; y < 4; ++y) {
        for (x = 0; x < 4; ++x) {
            refMbAddr = p_mb->ext.svc.refLayerPartIdc[y][x] >> 4;  // (G-219)
            refMbPartIdx = (p_mb->ext.svc.refLayerPartIdc[y][x] & 15) >> 2;  // (G-220)

            if (refMbAddr >= (int32_t)pc_layer_ref->u_list_macroblocks_count) {
                HL_DEBUG_ERROR("%d not valid as 'refLayerPredFlagLXMbAddr' value", refMbAddr);
                continue;
            }
            pc_mb_ref = pc_layer_ref->pp_list_macroblocks[refMbAddr];

            // ForEach (List0, List1)
            for (X = 0; X < 2; ++X) {
                const hl_int32_4_t* predFlagLX = (X == 0) ?  &pc_mb_ref->predFlagL0 : &pc_mb_ref->predFlagL1;
                hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;
                hl_int32_4x4_t* tempRefIdxPredLX = (X == 0) ? &tempRefIdxPredL0 : &tempRefIdxPredL1;

                if ((*predFlagLX)[refMbPartIdx] == 0) {
                    (*tempRefIdxPredLX)[y][x] = -1; // (G-216)
                    (*mvILPredLX)[y][x][0] = 0; // (G-217)
                    (*mvILPredLX)[y][x][1] = 0; // (G-218)
                }
                else {
                    int32_t dOX, dOY, dSW, dSH;
                    int32_t scaleX, scaleY;
                    const hl_codec_264_mb_t *pc_refMb = pc_layer_ref->pp_list_macroblocks[refMbAddr];
                    int32_t scaledW = pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerPicWidthInSamplesL; // (G-224)
                    int32_t scaledH = pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerPicHeightInSamplesL * ( 1 + pc_layer_active->pc_slice_hdr->field_pic_flag ); // (G-225)
                    int32_t refLayerW = pc_layer_active->RefLayerPicWidthInSamplesL; // (G-226)
                    int32_t refLayerH = pc_layer_active->RefLayerPicHeightInSamplesL * (1 + pc_layer_active->RefLayerFieldPicFlag); // (G-227)
                    struct hl_codec_264_dpb_fs_s** RefPicListX = (X == 0) ? pc_layer_active->pobj_poc->RefPicList0 : pc_layer_active->pobj_poc->RefPicList1;
                    const hl_int32_4_t* refIdxLX = (X == 0) ? &pc_refMb->refIdxL0 : &pc_refMb->refIdxL1;
                    hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;
                    refSubMbPartIdx = (p_mb->ext.svc.refLayerPartIdc[y][x] & 3); // (G-221)

                    (*tempRefIdxPredLX)[y][x] = (*refIdxLX)[refMbPartIdx]
                                                * ( 1 + p_mb->ext.svc.fieldMbFlag - pc_layer_active->pc_slice_hdr->field_pic_flag )
                                                / ( 1 + pc_refMb->ext.svc.fieldMbFlag - pc_layer_active->RefLayerFieldPicFlag); // (G-222)

                    aMv = ((X == 0) ? pc_refMb->mvL0 : pc_refMb->mvL1)[refMbPartIdx][refSubMbPartIdx];
                    aMv.y = aMv.y * (1 + pc_refMb->ext.svc.fieldMbFlag); // (G-223)

                    // FIXME: norm -> "refPicList0" instead of "RefPicList0"
                    if (pc_layer_active->CroppingChangeFlag == 0 || (!RefPicListX[(*tempRefIdxPredLX)[y][x]] || HL_CODEC_264_REF_TYPE_IS_UNUSED(RefPicListX[(*tempRefIdxPredLX)[y][x]]->RefType))) {
                        dOX = dOY = dSW = dSH = 0;
                    }
                    else {
                        int32_t refPicScaledRefLayerLeftOffset = pc_layer_ref->pc_slice_hdr->ext.svc.ScaledRefLayerLeftOffset;
                        int32_t refPicScaledRefLayerRightOffset = pc_layer_ref->pc_slice_hdr->ext.svc.ScaledRefLayerRightOffset;
                        int32_t refPicScaledRefLayerTopOffset = pc_layer_ref->pc_slice_hdr->ext.svc.ScaledRefLayerTopOffset;
                        int32_t refPicScaledRefLayerBottomOffset = pc_layer_ref->pc_slice_hdr->ext.svc.ScaledRefLayerBottomOffset;
                        dOX = pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerLeftOffset - refPicScaledRefLayerLeftOffset; // (G-228)
                        dOY = pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerTopOffset - refPicScaledRefLayerTopOffset; // (G-229)
                        dSW = pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerRightOffset - refPicScaledRefLayerRightOffset + dOX; // (G-230)
                        dSH = pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerBottomOffset - refPicScaledRefLayerBottomOffset + dOY; // (G-231)
                    }

                    scaleX = (((scaledW + dSW) << 16) + (refLayerW >> 1)) / refLayerW; // (G-232)
                    scaleY = (((scaledH + dSH) << 16) + (refLayerH >> 1)) / refLayerH; // (G-233)

                    aMv.x = (aMv.x * scaleX + 32768) >> 16; // (G-234)
                    aMv.y = (aMv.y * scaleY + 32768) >> 16; // (G-235)

                    if (aMv.x || aMv.y) {
                        int a = 0; // FIXME
                    }

                    if (pc_layer_active->CroppingChangeFlag) {
                        int32_t xMbPic, yMbPic, xFrm, yFrm;
                        // 6.4.1 Inverse macroblock scanning process
                        // hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_codec->CurrMbAddr, &xMbPic, &yMbPic);
                        xMbPic = p_mb->xL;
                        yMbPic = p_mb->yL;

                        xFrm = ( xMbPic + ( 4 * x + 1 ) ); // (G-236)
                        yFrm = ( yMbPic + ( 4 * y + 1 ) * ( 1 + p_mb->ext.svc.fieldMbFlag - pc_layer_active->pc_slice_hdr->field_pic_flag ) ) * ( 1 + pc_layer_active->pc_slice_hdr->field_pic_flag ); // (G-237)

                        scaleX = ( ( ( 4 * dSW ) << 16 ) + ( scaledW >> 1 ) ) / scaledW; // (G-238)
                        scaleY = ( ( ( 4 * dSH ) << 16 ) + ( scaledH >> 1 ) ) / scaledH; // (G-239)

                        aMv.x += ( ( ( xFrm - pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerLeftOffset ) * scaleX + 32768 ) >> 16 ) - 4 * dOX; // (G-240)
                        aMv.y += ( ( ( yFrm - pc_layer_active->pc_slice_hdr->ext.svc.ScaledRefLayerTopOffset ) * scaleY + 32768 ) >> 16 ) - 4 * dOY; // (G-241)
                    }

                    (*mvILPredLX)[y][x][0] = aMv.x; // (G-242)
                    (*mvILPredLX)[y][x][1] = aMv.y / ( 1 + p_mb->ext.svc.fieldMbFlag ); // (G-243)
                }
            }
        }
    }


    // ForEach (List0, List1)
    for (X = 0; X < 2; ++X) {
        hl_int32_4x4_t* tempRefIdxPredLX = (X == 0) ? &tempRefIdxPredL0 : &tempRefIdxPredL1;
        hl_int32_2x2_t* refIdxILPredLX = (X == 0) ? &p_mb->ext.svc.refIdxILPredL0 : &p_mb->ext.svc.refIdxILPredL1;
        hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;

        // ForEach (8x8 block)
        for (yP = 0; yP < 2; ++yP) {
            for (xP = 0; xP < 2; ++xP) {
                (*refIdxILPredLX)[yP][xP] = (*tempRefIdxPredLX)[yP << 1][xP << 1];
                if (pc_layer_active->RestrictedSpatialResolutionChangeFlag == 0) {
                    // ForEach (4x4 block)
                    for (yS = 0; yS < 2; ++yS) {
                        for (xS = 0; xS < 2; ++xS) {
                            (*refIdxILPredLX)[yP][xP] = HL_MATH_MIN_POSITIVE((*refIdxILPredLX)[yP][xP], (*tempRefIdxPredLX)[2 * yP + yS][2 * xP + xS]); // (G-244)
                            if ((*tempRefIdxPredLX)[2 * yP + yS][2 * xP + xS] != (*refIdxILPredLX)[yP][xP]) {
                                if ((*tempRefIdxPredLX)[2 * yP + yS][2 * xP + 1 - xS] == (*refIdxILPredLX)[yP][xP]) {
                                    (*mvILPredLX)[2 * yP + yS][2 * xP + xS][0] = (*mvILPredLX)[2 * yP + yS][2 * xP + 1 - xS][0]; // (G-246)
                                    (*mvILPredLX)[2 * yP + yS][2 * xP + xS][1] = (*mvILPredLX)[2 * yP + yS][2 * xP + 1 - xS][1]; // (G-246)
                                }
                                else if ((*tempRefIdxPredLX)[2 * yP + 1 - yS][2 * xP + xS] == (*refIdxILPredLX)[yP][xP]) {
                                    (*mvILPredLX)[2 * yP + yS][2 * xP + xS][0] = (*mvILPredLX)[2 * yP + 1 - yS][2 * xP + xS][0]; // (G-247)
                                    (*mvILPredLX)[2 * yP + yS][2 * xP + xS][1] = (*mvILPredLX)[2 * yP + 1 - yS][2 * xP + xS][1]; // (G-247)
                                }
                                else {
                                    (*mvILPredLX)[2 * yP + yS][2 * xP + xS][0] = (*mvILPredLX)[2 * yP + 1 - yS][2 * xP + 1 - xS][0]; // (G-248)
                                    (*mvILPredLX)[2 * yP + yS][2 * xP + xS][1] = (*mvILPredLX)[2 * yP + 1 - yS][2 * xP + 1 - xS][1]; // (G-248)
                                }
                            }
                        }
                    }
                    if (IsSliceHeaderEB(pc_layer_active->pc_slice_hdr) && pc_layer_active->pc_slice_hdr->pc_pps->pc_sps->direct_8x8_inference_flag) {
                        tempMv[0] = (*mvILPredLX)[3 * yP][3 * xP][0]; // (G-249)
                        tempMv[1] = (*mvILPredLX)[3 * yP][3 * xP][1]; // (G-249)
                        (*mvILPredLX)[2 * yP + yS][2 * xP + xS][0] = tempMv[0]; // (G-250)
                        (*mvILPredLX)[2 * yP + yS][2 * xP + xS][1] = tempMv[1]; // (G-250)
                    }
                }
            }
        }
    }

    if (pc_layer_active->RestrictedSpatialResolutionChangeFlag == 0) {
        // ForEach (8x8 block)
        for (yP = 0; yP < 2; ++yP) {
            for (xP = 0; xP < 2; ++xP) {
                maxX = IsSliceHeaderEB(pc_layer_active->pc_slice_hdr) ? 1 : 0;
                xO = (xP << 1);
                yO = (yP << 1);
                for (X = 0; X <= maxX; ++X) {
                    hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;
                    if (
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO][xO], (*mvILPredLX)[yO][xO + 1]) <= 1 &&
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO][xO], (*mvILPredLX)[yO + 1][xO]) <= 1 &&
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO][xO], (*mvILPredLX)[yO + 1][xO + 1]) <= 1
                    ) {
                        subPartSize = HL_CODEC_264_SUBPART_SIZE_8X8;
                    }
                    else if (
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO][xO], (*mvILPredLX)[yO][xO + 1]) <= 1 &&
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO + 1][xO], (*mvILPredLX)[yO + 1][xO + 1]) <= 1
                    ) {
                        subPartSize = HL_CODEC_264_SUBPART_SIZE_8X4;
                    }
                    else if (
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO][xO], (*mvILPredLX)[yO + 1][xO]) <= 1 &&
                        HL_MATH_MV_DIFF((*mvILPredLX)[yO][xO + 1], (*mvILPredLX)[yO + 1][xO + 1]) <= 1
                    ) {
                        subPartSize = HL_CODEC_264_SUBPART_SIZE_4X8;
                    }
                    else {
                        subPartSize = HL_CODEC_264_SUBPART_SIZE_4X4;
                    }

                    if (subPartSize != HL_CODEC_264_SUBPART_SIZE_4X4) {
                        if (subPartSize == HL_CODEC_264_SUBPART_SIZE_8X8) {
                            tempMvALX[ 0 ] = ( (*mvILPredLX)[yO][xO][ 0 ] + (*mvILPredLX)[yO][xO + 1][ 0 ] + (*mvILPredLX)[yO + 1][xO][ 0 ] + (*mvILPredLX)[yO + 1][xO + 1][0] + 2 ) >> 2; // (G-252)
                            tempMvALX[ 1 ] = ( (*mvILPredLX)[yO][xO][ 1 ] + (*mvILPredLX)[yO][xO + 1][ 1 ] + (*mvILPredLX)[yO + 1][xO][ 1 ] + (*mvILPredLX)[yO + 1][xO + 1][1] + 2 ) >> 2; // (G-252)
                            for (yS = 0; yS < 2; ++yS) {
                                for (xS = 0; xS < 2; ++xS) {
                                    (*mvILPredLX)[yO + yS][xO + xS][ 0 ] = tempMvALX[ 0 ]; // (G-257)
                                    (*mvILPredLX)[yO + yS][xO + xS][ 1 ] = tempMvALX[ 1 ]; // (G-257)
                                }
                            }
                        }
                        else if (subPartSize == HL_CODEC_264_SUBPART_SIZE_8X4) {
                            tempMvALX[ 0 ] = ( (*mvILPredLX)[yO][xO][ 0 ] + (*mvILPredLX)[yO][xO + 1][ 0 ] + 1 ) >> 1; //(G-253)
                            tempMvALX[ 1 ] = ( (*mvILPredLX)[yO][xO][ 1 ] + (*mvILPredLX)[yO][xO + 1][ 1 ] + 1 ) >> 1; //(G-253)
                            tempMvBLX[ 0 ] = ( (*mvILPredLX)[yO + 1][xO][ 0 ] + (*mvILPredLX)[yO + 1][xO + 1][ 0 ] + 1 ) >> 1; //(G-254)
                            tempMvBLX[ 1 ] = ( (*mvILPredLX)[yO + 1][xO][ 1 ] + (*mvILPredLX)[yO + 1][xO + 1][ 1 ] + 1 ) >> 1; //(G-254)
                            for (yS = 0; yS < 2; ++yS) {
                                for (xS = 0; xS < 2; ++xS) {
                                    (*mvILPredLX)[yO][xO + xS][ 0 ] = tempMvALX[ 0 ]; // (G-258)
                                    (*mvILPredLX)[yO][xO + xS][ 1 ] = tempMvALX[ 1 ]; // (G-258)
                                    (*mvILPredLX)[yO + 1][xO + xS][ 0 ] = tempMvBLX[ 0 ]; // (G-259)
                                    (*mvILPredLX)[yO + 1][xO + xS][ 1 ] = tempMvBLX[ 1 ]; // (G-259)
                                }
                            }
                        }
                        else {
                            tempMvALX[ 0 ] = ( (*mvILPredLX)[yO][xO][ 0 ] + (*mvILPredLX)[yO + 1][xO][ 0 ] + 1 ) >> 1; // (G-255)
                            tempMvALX[ 1 ] = ( (*mvILPredLX)[yO][xO][ 1 ] + (*mvILPredLX)[yO + 1][xO][ 1 ] + 1 ) >> 1; // (G-255)
                            tempMvBLX[ 0 ] = ( (*mvILPredLX)[yO][xO + 1][ 0 ] + (*mvILPredLX)[yO + 1][xO + 1][ 0 ] + 1 ) >> 1; // (G-256)
                            tempMvBLX[ 1 ] = ( (*mvILPredLX)[yO][xO + 1][ 1 ] + (*mvILPredLX)[yO + 1][xO + 1][ 1 ] + 1 ) >> 1; // (G-256)

                            for (yS = 0; yS < 2; ++yS) {
                                for (xS = 0; xS < 2; ++xS) {
                                    (*mvILPredLX)[yO + yS][xO][ 0 ] = tempMvALX[ 0 ]; // (G-260)
                                    (*mvILPredLX)[yO + yS][xO][ 1 ] = tempMvALX[ 1 ]; // (G-260)
                                    (*mvILPredLX)[yO + yS][xO + 1][ 0 ] = tempMvBLX[ 0 ]; // (G-261)
                                    (*mvILPredLX)[yO + yS][xO + 1][ 1 ] = tempMvBLX[ 1 ]; // (G-261)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// G.8.6.1.3 Derivation process for inter-layer predictors for P and B macroblock and sub-macroblock types
// This process is only invoked when slice_type is equal to EP or EB.
// Outputs: "mbTypeILPred", "subMbTypeILPred"
void	hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_P_and_B_mb_and_submb_types_svc(
    const hl_codec_264_t* pc_codec,
    hl_codec_264_mb_t *p_mb
)
{
    int32_t maxX, X, x, y;
    hl_bool_t b_cond, i_tmp0, i_tmp1;
    const hl_codec_264_layer_t* pc_layer_active = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer_active->pc_slice_hdr;
    const hl_codec_264_layer_t* pc_layer_ref = pc_layer_active->pc_ref;
    HL_CODEC_264_PART_SIZE_T partitionSize = HL_CODEC_264_PART_SIZE_8X8;
    int32_t partitionSizeConds[2/*X=L0/L1*/][HL_CODEC_264_PART_SIZE_MAX_COUNT] = { 0 };
    int32_t partPredModeA = 0, partPredModeB = 0;

    maxX = IsSliceHeaderEB(pc_slice_header) ? 1 : 0;

    for (X = 0; X <= maxX; ++X) {
        hl_int32_2x2_t* refIdxILPredLX = (X == 0) ? &p_mb->ext.svc.refIdxILPredL0 : &p_mb->ext.svc.refIdxILPredL1;
        hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;

        /* Check for 16x16 partition size */
        b_cond = (*refIdxILPredLX)[0][0] == (*refIdxILPredLX)[0][1] &&
                 (*refIdxILPredLX)[0][0] == (*refIdxILPredLX)[1][0] &&
                 (*refIdxILPredLX)[0][0] == (*refIdxILPredLX)[1][1];
        if (b_cond) {
            i_tmp0 = (*mvILPredLX)[0][0][0];
            i_tmp1 = (*mvILPredLX)[0][0][1];
            for (y = 0; y < 4 && b_cond; ++y) {
                for (x = 0; x < 4; ++x) {
                    if ((*mvILPredLX)[y][x][0] != i_tmp0 || (*mvILPredLX)[y][x][1] != i_tmp1) {
                        b_cond = HL_FALSE;
                        break;
                    }
                }
            }
            if (b_cond) {
                partitionSizeConds[X][HL_CODEC_264_PART_SIZE_16X16] = 1;
                if (maxX == 0 || (X == 1 && partitionSizeConds[0][HL_CODEC_264_PART_SIZE_16X16])) { // single list to check or Cond(L0)=Cond(L1)=1
                    partitionSize = HL_CODEC_264_PART_SIZE_16X16;
                    break;
                }
            }
        }

        /* Check for 16x8 partition size */
        b_cond = (*refIdxILPredLX)[0][0] == (*refIdxILPredLX)[0][1] &&
                 (*refIdxILPredLX)[1][0] == (*refIdxILPredLX)[1][1];
        if (b_cond) {
            i_tmp0 = (*mvILPredLX)[0][0][0];
            i_tmp1 = (*mvILPredLX)[0][0][1];
            for (y = 0; y < 2 && b_cond; ++y) {
                for (x = 0; x < 4; ++x) {
                    if ((*mvILPredLX)[y][x][0] != i_tmp0 || (*mvILPredLX)[y][x][1] != i_tmp1) {
                        b_cond = HL_FALSE;
                        break;
                    }
                }
            }
            if (b_cond) {
                i_tmp0 = (*mvILPredLX)[2][0][0];
                i_tmp1 = (*mvILPredLX)[2][0][1];
                for (y = 2; y < 4 && b_cond; ++y) {
                    for (x = 0; x < 4; ++x) {
                        if ((*mvILPredLX)[y][x][0] != i_tmp0 || (*mvILPredLX)[y][x][1] != i_tmp1) {
                            b_cond = HL_FALSE;
                            break;
                        }
                    }
                }
                if (b_cond) {
                    partitionSizeConds[X][HL_CODEC_264_PART_SIZE_16X8] = 1;
                    if (maxX == 0 || (X == 1 && partitionSizeConds[0][HL_CODEC_264_PART_SIZE_16X8])) { // single list to check or Cond(L0)=Cond(L1)=1
                        partitionSize = HL_CODEC_264_PART_SIZE_16X8;
                        break;
                    }
                }
            }
        }

        /* Check for 8x16 partition size */
        b_cond = (*refIdxILPredLX)[0][0] == (*refIdxILPredLX)[1][0] &&
                 (*refIdxILPredLX)[0][1] == (*refIdxILPredLX)[1][1];
        if (b_cond) {
            i_tmp0 = (*mvILPredLX)[0][0][0];
            i_tmp1 = (*mvILPredLX)[0][0][1];
            for (y = 0; y < 4 && b_cond; ++y) {
                for (x = 0; x < 2; ++x) {
                    if ((*mvILPredLX)[y][x][0] != i_tmp0 || (*mvILPredLX)[y][x][1] != i_tmp1) {
                        b_cond = HL_FALSE;
                        break;
                    }
                }
            }
            if (b_cond) {
                i_tmp0 = (*mvILPredLX)[0][2][0];
                i_tmp1 = (*mvILPredLX)[0][2][1];
                for (y = 0; y < 4 && b_cond; ++y) {
                    for (x = 2; x < 4; ++x) {
                        if ((*mvILPredLX)[y][x][0] != i_tmp0 || (*mvILPredLX)[y][x][1] != i_tmp1) {
                            b_cond = HL_FALSE;
                            break;
                        }
                    }
                }
                if (b_cond) {
                    partitionSizeConds[X][HL_CODEC_264_PART_SIZE_8X16] = 1;
                    if (maxX == 0 || (X == 1 && partitionSizeConds[0][HL_CODEC_264_PART_SIZE_8X16])) { // single list to check or Cond(L0)=Cond(L1)=1
                        partitionSize = HL_CODEC_264_PART_SIZE_8X16;
                        break;
                    }
                }
            }
        }
    }

    if (IsSliceHeaderEB(pc_slice_header) && partitionSize != HL_CODEC_264_PART_SIZE_8X8) {
        partPredModeA = ( ( p_mb->ext.svc.refIdxILPredL1[0][0] >= 0 ) ? 2 : 0 ) + ( ( p_mb->ext.svc.refIdxILPredL0[0][0] >= 0 ) ? 1 : 0 ); // (G-262)
        if (partitionSize == HL_CODEC_264_PART_SIZE_16X8 || partitionSize == HL_CODEC_264_PART_SIZE_8X16) {
            partPredModeB = ( ( p_mb->ext.svc.refIdxILPredL1[1][1] >= 0 ) ? 2 : 0 ) + ( ( p_mb->ext.svc.refIdxILPredL0[1][1] >= 0 ) ? 1 : 0 ); // (G-263)
        }
    }

    // mbTypeILPred is derived as specified in Table G-7.
    p_mb->ext.svc.mbTypeILPred = IsSliceHeaderEB(pc_slice_header)
                                 ? HL_CODEC_264_SVC_MB_TYPE_ILPRED_EB[partitionSize][partPredModeA][partPredModeB]
                                 : HL_CODEC_264_SVC_MB_TYPE_ILPRED_EP[partitionSize];
    p_mb->ext.svc.mb_type_il_pred = IsSliceHeaderEB(pc_slice_header)
                                    ? HL_CODEC_264_SVC_MB_TYPE_ILPRED_EB_STANDARD[partitionSize][partPredModeA][partPredModeB]
                                    : HL_CODEC_264_SVC_MB_TYPE_ILPRED_EP_STANDARD[partitionSize];

    if (p_mb->ext.svc.mbTypeILPred == HL_CODEC_264_MB_TYPE_P_8X8 || p_mb->ext.svc.mbTypeILPred == HL_CODEC_264_MB_TYPE_B_8X8) {
        int32_t mbPartIdx, xO, yO;
        HL_CODEC_264_SUBPART_SIZE_T subPartitionSize[4] = { HL_CODEC_264_SUBPART_SIZE_4X4 };
        int32_t subPartitionSizeSizeConds[2/*X=L0/L1*/][HL_CODEC_264_SUBPART_SIZE_MAX_COUNT] = { 0 };
        int32_t partPredMode = 0;
        for (mbPartIdx = 0; mbPartIdx < 4; ++mbPartIdx) {
            xO = ((mbPartIdx & 1) << 1);
            yO = ((mbPartIdx >> 1) << 1);
            for (X = 0; X <= maxX; ++X) {
                hl_int32_2x2_t* refIdxILPredLX = (X == 0) ? &p_mb->ext.svc.refIdxILPredL0 : &p_mb->ext.svc.refIdxILPredL1;
                hl_int32_4x4x2_t* mvILPredLX = (X == 0) ? &p_mb->ext.svc.mvILPredL0 : &p_mb->ext.svc.mvILPredL1;
                /* Check for SubPart 8x8 */
                if (
                    (*mvILPredLX)[yO][xO][0] == (*mvILPredLX)[yO][xO + 1][0] && (*mvILPredLX)[yO][xO][0] == (*mvILPredLX)[yO + 1][xO][0] && (*mvILPredLX)[yO][xO][0] == (*mvILPredLX)[yO + 1][xO + 1][0] &&
                    (*mvILPredLX)[yO][xO][1] == (*mvILPredLX)[yO][xO + 1][1] && (*mvILPredLX)[yO][xO][1] == (*mvILPredLX)[yO + 1][xO][1] && (*mvILPredLX)[yO][xO][1] == (*mvILPredLX)[yO + 1][xO + 1][1]
                ) {
                    subPartitionSizeSizeConds[X][HL_CODEC_264_SUBPART_SIZE_8X8] = 1;
                    if (maxX == 0 || (X == 1 && subPartitionSizeSizeConds[0][HL_CODEC_264_SUBPART_SIZE_8X8])) { // single list to check or Cond(L0)=Cond(L1)=1
                        subPartitionSize[mbPartIdx] = HL_CODEC_264_SUBPART_SIZE_8X8;
                        break;
                    }
                }
                /* Check for SubPart 8x4 */
                if (
                    (*mvILPredLX)[yO][xO][0] == (*mvILPredLX)[yO][xO + 1][0] && (*mvILPredLX)[yO + 1][xO][0] == (*mvILPredLX)[yO + 1][xO + 1][0] &&
                    (*mvILPredLX)[yO][xO][1] == (*mvILPredLX)[yO][xO + 1][1] && (*mvILPredLX)[yO + 1][xO][1] == (*mvILPredLX)[yO + 1][xO + 1][1]
                ) {
                    subPartitionSizeSizeConds[X][HL_CODEC_264_SUBPART_SIZE_8X4] = 1;
                    if (maxX == 0 || (X == 1 && subPartitionSizeSizeConds[0][HL_CODEC_264_SUBPART_SIZE_8X4])) { // single list to check or Cond(L0)=Cond(L1)=1
                        subPartitionSize[mbPartIdx] = HL_CODEC_264_SUBPART_SIZE_8X4;
                        break;
                    }
                }
                /* Check for SubPart 4x8 */
                if (
                    (*mvILPredLX)[yO][xO][0] == (*mvILPredLX)[yO + 1][xO][0] && (*mvILPredLX)[yO][xO + 1][0] == (*mvILPredLX)[yO + 1][xO + 1][0] &&
                    (*mvILPredLX)[yO][xO][1] == (*mvILPredLX)[yO + 1][xO][1] && (*mvILPredLX)[yO][xO + 1][1] == (*mvILPredLX)[yO + 1][xO + 1][1]
                ) {
                    subPartitionSizeSizeConds[X][HL_CODEC_264_SUBPART_SIZE_4X8] = 1;
                    if (maxX == 0 || (X == 1 && subPartitionSizeSizeConds[0][HL_CODEC_264_SUBPART_SIZE_4X8])) { // single list to check or Cond(L0)=Cond(L1)=1
                        subPartitionSize[mbPartIdx] = HL_CODEC_264_SUBPART_SIZE_4X8;
                        break;
                    }
                }
            }
        } // end-of-for (mbPartIdx)

        // the sub-macroblock type predictor subMbTypeILPred[ mbPartIdx ] is derived as specified in Table G-8.
        if (IsSliceHeaderEB(pc_slice_header)) { // EB
            partPredMode = ( ( p_mb->ext.svc.refIdxILPredL1[yO >> 1][xO >> 1] >= 0 ) ? 2 : 0 ) + ( ( p_mb->ext.svc.refIdxILPredL0[yO >> 1][xO >> 1] >= 0 ) ? 1 : 0 ); // (G-264)
            p_mb->ext.svc.subMbTypeILPred[0] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB[subPartitionSize[0]][partPredMode];
            p_mb->ext.svc.subMbTypeILPred[1] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB[subPartitionSize[1]][partPredMode];
            p_mb->ext.svc.subMbTypeILPred[2] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB[subPartitionSize[2]][partPredMode];
            p_mb->ext.svc.subMbTypeILPred[3] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB[subPartitionSize[3]][partPredMode];

            p_mb->ext.svc.sub_mb_type_il_pred[0] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB_STANDARD[subPartitionSize[0]][partPredMode];
            p_mb->ext.svc.sub_mb_type_il_pred[1] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB_STANDARD[subPartitionSize[1]][partPredMode];
            p_mb->ext.svc.sub_mb_type_il_pred[2] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB_STANDARD[subPartitionSize[2]][partPredMode];
            p_mb->ext.svc.sub_mb_type_il_pred[3] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB_STANDARD[subPartitionSize[3]][partPredMode];
        }
        else { // EP
            p_mb->ext.svc.subMbTypeILPred[0] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP[subPartitionSize[0]];
            p_mb->ext.svc.subMbTypeILPred[1] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP[subPartitionSize[1]];
            p_mb->ext.svc.subMbTypeILPred[2] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP[subPartitionSize[2]];
            p_mb->ext.svc.subMbTypeILPred[3] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP[subPartitionSize[3]];

            p_mb->ext.svc.sub_mb_type_il_pred[0] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP_STANDARD[subPartitionSize[0]];
            p_mb->ext.svc.sub_mb_type_il_pred[1] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP_STANDARD[subPartitionSize[1]];
            p_mb->ext.svc.sub_mb_type_il_pred[2] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP_STANDARD[subPartitionSize[2]];
            p_mb->ext.svc.sub_mb_type_il_pred[3] = HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP_STANDARD[subPartitionSize[3]];
        }
    } // end-of-if (mbTypeILPred==8x8)
    else {
        p_mb->ext.svc.subMbTypeILPred[0] = p_mb->ext.svc.subMbTypeILPred[1] = p_mb->ext.svc.subMbTypeILPred[2] = p_mb->ext.svc.subMbTypeILPred[3] = -1; // unspecified
        p_mb->ext.svc.sub_mb_type_il_pred[0] = p_mb->ext.svc.sub_mb_type_il_pred[1] = p_mb->ext.svc.sub_mb_type_il_pred[2] = p_mb->ext.svc.sub_mb_type_il_pred[3] = -1; // unspecified
    }
}

// G.8.6.2.2.1 Derivation process for reference layer slice and intra macroblock identifications
void	hl_codec_264_utils_derivation_process_for_ref_layer_slice_and_intra_mb_identifications_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t xRef, int32_t yRef, int32_t refMbW, int32_t refMbH,
    int32_t *refSliceIdc, int32_t *refIntraMbFlag
)
{
    int32_t refMbAddr;
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    const struct hl_codec_264_mb_s *pc_mb_ref;

    if (pc_layer->RefLayerMbaffFrameFlag == 0) {
        refMbAddr = ( yRef / refMbH ) * pc_layer->RefLayerPicWidthInMbs + ( xRef / refMbW ); // (G-283)
    }
    else {
        int32_t refMbAddrTop = 2 * ( ( yRef / ( 2 * refMbH ) ) * pc_layer->RefLayerPicWidthInMbs + ( xRef / refMbW ) ); // (G-284)
        if (refMbAddrTop < (int32_t)pc_layer->pc_ref->u_list_macroblocks_count && pc_layer->pc_ref->pp_list_macroblocks[refMbAddrTop]->ext.svc.fieldMbFlag == 0) {
            refMbAddr = refMbAddrTop + ( yRef % ( 2 * refMbH ) ) / refMbH; //(G-285)
        }
        else {
            refMbAddr = refMbAddrTop + ( yRef & 1 ); // (G-286)
        }
    }

    pc_mb_ref = (refMbAddr < (int32_t)pc_layer->pc_ref->u_list_macroblocks_count) ? pc_layer->pc_ref->pp_list_macroblocks[refMbAddr] : HL_NULL;
    if (pc_mb_ref) {
        *refSliceIdc = pc_mb_ref->ext.svc.sliceIdc;
        *refIntraMbFlag =
            HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mb_ref)
            || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mb_ref)
            || HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mb_ref)
            || HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb_ref)
            || HL_CODEC_264_MB_TYPE_IS_I_BL(pc_mb_ref);
    }
    else {
        HL_DEBUG_ERROR("%d not valid 'refMbAddr' value", refMbAddr);
        *refSliceIdc = 0;
        *refIntraMbFlag = 1;
    }
}

// G.8.6.2.2.2 Construction process for not available sample values prior to intra resampling
void	hl_codec_264_utils_derivation_process_for_not_avail_samples_prior_to_intra_resampling_svc(
    const hl_codec_264_t* pc_codec,
    const hl_codec_264_mb_t *pc_mb,
    int32_t refMbW, int32_t refMbH,
    int32_t refArrayW, int32_t refArrayH, int32_t* refSampleArray, const uint8_t* refSampleArrayAvailability,
    int32_t xOffset, int32_t yOffset
)
{
#define refSampleArrayAt(_x_, _y_) ( refSampleArray[((_y_)*refArrayW)+(_x_)] )
#define refSampleArrayAtIsAvail(_x_, _y_) ( refSampleArrayAvailability[((_y_)*refArrayW)+(_x_)] == 1 )
#define refSampleArrayAtIsNotAvail(_x_, _y_) ( refSampleArrayAvailability[((_y_)*refArrayW)+(_x_)] == 0 )

    int32_t x, y, refMbW_div2 = (refMbW >> 1), refMbH_div2 = (refMbH >> 1);
    int32_t maxY = (refArrayH - refMbH_div2), maxX = (refArrayW - refMbW_div2);
    int32_t* refSampleArrayPtr;
    int32_t xR, yR, xD, yD, yA, xC, yC, cornerSampleAvailableFlag, diffHorVer, sgnXY, cornerSample;
    hl_bool_t b_cond, b_check_cond;

    // NOTE – The variable yD is never set equal to yA when RefLayerFrameMbsOnlyFlag is equal to 1 or
    // RefLayerFieldPicFlag is equal to 1.
    b_check_cond = (!pc_codec->layers.pc_active->RefLayerFrameMbsOnlyFlag && !pc_codec->layers.pc_active->RefLayerFieldPicFlag);

    y = refMbH_div2;
    refSampleArrayPtr = refSampleArray + (y * refArrayW);
    for (; y < maxY; ++y) {
        for (x = refMbW_div2; x < maxX; ++x) {
            if (refSampleArrayAtIsNotAvail(x, y)) {
                xR = ( x + xOffset ) % refMbW; // (G-287)
                yR = ( y + yOffset ) % refMbH; // (G-288)
                xD = ( ( xR >= refMbW_div2 ) ? ( xR - refMbW ) : ( xR + 1 ) ); // (G-289)
                yD = ( ( yR >= refMbH_div2 ) ? ( yR - refMbH ) : ( yR + 1 ) ); // (G-290)
                yA = yD - ( refMbH_div2 + 1 ) * HL_MATH_SIGN( yD ); // (G-291)

                if (b_check_cond) {
                    b_cond =
                        (refSampleArrayAtIsNotAvail(x, y - yD))
                        && (refSampleArrayAtIsAvail(x, y - yA))
                        && (refSampleArrayAtIsAvail(x - xD, y));
                    if (!b_cond) {
                        b_cond =
                            (refSampleArrayAtIsNotAvail(x - xD, y) && refSampleArrayAtIsNotAvail(x, y - yD) && refSampleArrayAtIsNotAvail(x - xD, y - yD))
                            &&
                            (refSampleArrayAtIsAvail(x, y - yA) || refSampleArrayAtIsAvail(x - xD, y - yA));
                        if (!b_cond) {
                            b_cond =
                                (HL_MATH_ABS_INT32(yA) < HL_MATH_ABS_INT32(yD))
                                &&
                                (
                                    (refSampleArrayAtIsAvail(x, y - yD)&& refSampleArrayAtIsAvail(x, y - yA))
                                    ||
                                    (
                                        (refSampleArrayAtIsAvail(x, y - yD) || refSampleArrayAtIsAvail(x - xD, y - yD))
                                        && (refSampleArrayAtIsAvail(x, y - yA) || refSampleArrayAtIsAvail(x - xD, y - yA))
                                        && (refSampleArrayAtIsNotAvail(x - xD, y))
                                    )
                                );
                        }
                    }

                    if (b_cond) {
                        yD = yA;
                    }
                }
                if (refSampleArrayAtIsAvail(x - xD, y) && refSampleArrayAtIsAvail(x, y - yD)) {
                    cornerSampleAvailableFlag = refSampleArrayAtIsAvail(x - xD, y - yD);
                    // G.8.6.2.2.2.1 Diagonal construction process for not available sample values
                    {
                        diffHorVer = HL_MATH_ABS( xD ) - HL_MATH_ABS( yD ); // (G-292)
                        sgnXY = HL_MATH_SIGN( xD * yD ); // (G-293)
                        if (cornerSampleAvailableFlag == 0) {
                            cornerSample = refSampleArrayAt(x - xD, y - yD);
                            xC = x - xD + HL_MATH_SIGN(xD);
                            yC = y - yD + HL_MATH_SIGN(yD);
                            refSampleArrayAt(x - xD, y - yD) = (refSampleArrayAt(x - xD, yC) + refSampleArrayAt(xC, y - yD) + 1) >> 1; // (G-294)
                        }
                        if (diffHorVer > 0) {
                            xC = x - sgnXY * yD;
                            yC = y - yD;
                            refSampleArrayAt(x, y) = ( refSampleArrayAt(xC - 1, yC) + 2 * refSampleArrayAt(xC, yC) + refSampleArrayAt(xC + 1, yC) + 2 ) >> 2; // (G-295)
                        }
                        else if (diffHorVer < 0) {
                            xC = x - xD;
                            yC = y - sgnXY * xD;
                            refSampleArrayAt(x, y) = ( refSampleArrayAt(xC, yC - 1) + 2 * refSampleArrayAt(xC, yC) + refSampleArrayAt(xC, yC + 1) + 2 ) >> 2; // (G-296)
                        }
                        else {
                            xC = x - xD + HL_MATH_SIGN( xD );
                            yC = y - yD + HL_MATH_SIGN( yD );
                            refSampleArrayAt(x, y) = ( refSampleArrayAt(xC, y - yD) + 2 * refSampleArrayAt(x - xD, y - yD) + refSampleArrayAt(x - xD, yC) + 2 ) >> 2; // (G-297)
                        }
                        if (cornerSampleAvailableFlag == 0) {
                            refSampleArrayAt(x - xD, y - yD) = cornerSample;
                        }
                    } // end-of-G.8.6.2.2.2.1
                }
                else {
                    if (refSampleArrayAtIsAvail(x - xD, y)) {
                        refSampleArrayPtr[x] = refSampleArrayAt(x - xD, y);
                    }
                    else if (refSampleArrayAtIsAvail(x, y - yD)) {
                        refSampleArrayPtr[x] = refSampleArrayAt(x, y - yD);
                    }
                    else if (refSampleArrayAtIsAvail(x - xD, y - yD)) {
                        refSampleArrayPtr[x] = refSampleArrayAt(x - xD, y - yD);
                    }
                    // else; sample value refSampleArray[ x, y ] is not modified.
                } //end-of-else()
            } // end-of-if(not avail)
        }
        refSampleArrayPtr += refArrayW;
    }
#if 0
    refSampleArrayPtr = refSampleArray;
    for (y = 0; y < refArrayH; ++y) {
        for (x = 0; x < refArrayW; ++x) {
            if (refSampleArrayPtr[x] == HL_CODEC_264_SVC_SAMPLE_NA_INTRA_BASE) {
                refSampleArrayPtr[x] = 0;
            }
        }
        refSampleArrayPtr += refArrayW;
    }
#endif

#undef refSampleArrayAt
#undef refSampleArrayAtIsAvail
#undef refSampleArrayAtIsNotAvail
}

// G.8.6.3.2.1 Derivation process for reference layer transform block identifications
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_transform_block_identifications_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t xRef, int32_t yRef,
    int32_t chromaFlag,
    int32_t refMbW, int32_t refMbH,
    int32_t *refTransBlkIdc)
{
    int32_t refMbAddr, xM, yM;
    const hl_codec_264_layer_t* pc_layer = pc_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    const struct hl_codec_264_mb_s *pc_mb_ref;

    if (!pc_layer->RefLayerMbaffFrameFlag) {
        refMbAddr = ( yRef / refMbH ) * pc_layer->RefLayerPicWidthInMbs + ( xRef / refMbW ); // (G-323)
        xM = xRef % refMbW; // (G-324)
        yM = yRef % refMbH; // (G-325)
    }
    else {
        int32_t refMbAddrTop = 2 * ( ( yRef / ( 2 * refMbH ) ) * pc_layer->RefLayerPicWidthInMbs + ( xRef / refMbW ) ); // (G-326)
        xM = xRef % refMbW; // (G-327)
        pc_mb_ref = pc_layer->pc_ref->pp_list_macroblocks[refMbAddrTop];
        if (!pc_mb_ref->ext.svc.fieldMbFlag) {
            refMbAddr = refMbAddrTop + ( yRef % ( 2 * refMbH) ) / refMbH; // (G-328)
            yM = yRef % refMbH; // (G-329)
        }
        else {
            refMbAddr = refMbAddrTop + ( yRef % 2 ); // (G-330)
            yM = ( yRef % ( 2 * refMbH ) ) >> 1; // (G-331)
        }
    }

    if (refMbAddr >= (int32_t)pc_layer->pc_ref->u_list_macroblocks_count) {
        HL_DEBUG_ERROR("%d not valid for 'refMbAddr' value", refMbAddr);
        return HL_ERROR_INVALID_BITSTREAM;
    }
    pc_mb_ref = pc_layer->pc_ref->pp_list_macroblocks[refMbAddr];

    if ((!chromaFlag || pc_layer->RefLayerChromaArrayType == 3) && pc_mb_ref->ext.svc.cTrafo == HL_CODEC_264_TRANSFORM_TYPE_8X8) {
        *refTransBlkIdc = 1 + 2 * ( 4 * refMbAddr + 2 * ( yM / 8 ) + ( xM / 8 ) ); // (G-332)
    }
    else {
        *refTransBlkIdc = 2 * ( 16 * refMbAddr + 4 * ( yM / 4 ) + ( xM / 4 ) ); // (G-333)
    }

    return HL_ERROR_SUCCESS;
}
