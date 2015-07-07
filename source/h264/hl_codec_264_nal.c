#include "hartallo/h264/hl_codec_264_nal.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_fmo.h"
#include "hartallo/h264/hl_codec_264_reflist.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_debug.h"

HL_ERROR_T hl_codec_264_nal_init(hl_codec_264_nal_t* self, HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc)
{
    if (!self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    self->u_forbidden_zero_bit = 0;
    self->e_type = e_type;
    self->u_ref_idc = u_ref_idc;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_nal_decode(HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc, hl_codec_264_t* p_codec, hl_codec_264_nal_t** pp_nal)
{
    if (!p_codec || !pp_nal) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    switch(e_type) {
    case HL_CODEC_264_NAL_TYPE_SPS:
    case HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET:
        return hl_codec_264_nal_sps_decode(e_type, u_ref_idc, p_codec, (hl_codec_264_nal_sps_t**)pp_nal);
    case HL_CODEC_264_NAL_TYPE_PPS:
        return hl_codec_264_nal_pps_decode(e_type, u_ref_idc, p_codec, (hl_codec_264_nal_pps_t**)pp_nal);
    case HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE:
    case HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_A_NON_IDR_PICTURE:
    case HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION: {
        // "pp_nal" is useless
        /* 7.3.2.8 Slice layer without partitioning RBSP syntax */
        // slice_layer_without_partitioning_rbsp( ) {
        // slice_header( )
        HL_ERROR_T err = HL_ERROR_SUCCESS;
        hl_codec_264_slice_t *pc_slice;
        hl_codec_264_layer_t* pc_layer;

        // Compute "currDQId"
        if (p_codec->nal_current.svc_extension_flag) {
            p_codec->layers.currDQId = p_codec->nal_current.ext.svc.DQId;
        }
        else {
            p_codec->layers.currDQId = 0;
        }

        // Create current Layer representation
        pc_layer = p_codec->layers.p_list[p_codec->layers.currDQId];
        if (!pc_layer) {
            err = hl_codec_264_layer_create(&pc_layer, p_codec->layers.currDQId);
            if (err) {
                return err;
            }
            p_codec->layers.p_list[p_codec->layers.currDQId] = pc_layer;
        }
        p_codec->layers.pc_active = pc_layer;
        p_codec->pc_dpb = pc_layer->pobj_dpb;
        p_codec->pc_poc = pc_layer->pobj_poc;

        // Starting new SVC/AVC Access Unit ?
        if (pc_layer->DQId == 0 && pc_layer->i_mb_decode_count == 0 && pc_layer->i_mb_read_count == 0) {
            int dqId;
            hl_codec_264_layer_t* _pc_layer;
            for (dqId = p_codec->layers.DQIdMin;  dqId <= p_codec->layers.DQIdMax; ++dqId) {
                if (dqId >= 0 && dqId < HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT && (_pc_layer = p_codec->layers.p_list[dqId])) {
                    _pc_layer->MaxRefLayerDQId = -1;
                    _pc_layer->MinNoInterLayerPredFlag = 1;
                    _pc_layer->MaxTCoeffLevelPredFlag = 0;
                }
            }
            p_codec->layers.DependencyIdMax = 0;
            p_codec->layers.DQIdMax = 0;
            p_codec->layers.DQIdMin = 0;
            p_codec->layers.maxDQId = 0;
        }

        // Set current slice idx
        if (pc_layer->i_mb_read_count == 0) { // first time to try to read macroblocks
            pc_layer->u_list_slices_idx = 0; // reset to allow overriding
        }
        else {
            pc_layer->u_list_slices_idx = (pc_layer->u_list_slices_idx + 1) % HL_CODEC_264_SLICES_MAX_COUNT;
        }

        // create slice object
        pc_slice = pc_layer->p_list_slices[pc_layer->u_list_slices_idx];
        if (!pc_slice) {
            hl_codec_264_slice_t *_pc_slice;
            err = hl_codec_264_slice_create(&pc_slice);
            if (err) {
                return err;
            }
            _pc_slice = pc_slice;
            _pc_slice->u_idx = pc_layer->u_list_slices_idx;
            HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
                pc_layer->p_list_slices,
                pc_layer->u_list_slices_count,
                HL_CODEC_264_SLICES_MAX_COUNT,
                _pc_slice,
                _pc_slice->u_idx, err);

            if (err) {
                HL_OBJECT_SAFE_FREE(_pc_slice);
                return err;
            }
        }
        // set current slice
        pc_layer->pc_slice_curr = pc_slice;

        // Decode slice header
        err = hl_codec_264_nal_slice_header_decode(p_codec, (hl_codec_264_nal_slice_header_t**)&pc_slice->p_header);
        if (err) {
            return err;
        }

        // set current PPS using current slice header
        if (p_codec->pps.pc_active != pc_slice->p_header->pc_pps) {
            p_codec->pps.pc_active = pc_slice->p_header->pc_pps;
        }
        // set current CurrPicNum
        p_codec->pc_poc->CurrPicNum = pc_slice->p_header->frame_num;

        // Init current Layer representation
        err = hl_codec_264_layer_init(pc_layer, pc_slice, p_codec);
        if (err) {
            return err;
        }

        /* G.8.1.1 Derivation process for the set of layer representations required for decoding */
        if (pc_slice->p_header->SVCExtFlag) {
            int32_t currDQId, numEntries = 0;
            const hl_codec_264_layer_t* _pc_layer;
            // pseudo code (G-73)
            p_codec->layers.maxDQId = HL_MATH_MAX(p_codec->layers.maxDQId, pc_layer->DQId);
            currDQId = p_codec->layers.maxDQId;
            p_codec->layers.dqIdList[numEntries++] = currDQId;
            p_codec->layers.DQIdMax = HL_MATH_MAX(currDQId, p_codec->layers.DQIdMax);
            p_codec->layers.DQIdMin = HL_MATH_MIN(currDQId, p_codec->layers.DQIdMin);
            while ((_pc_layer = p_codec->layers.p_list[currDQId]) && _pc_layer->MaxRefLayerDQId >= 0) {
                p_codec->layers.dqIdList[numEntries] = _pc_layer->MaxRefLayerDQId;
                currDQId = p_codec->layers.dqIdList[numEntries];
                p_codec->layers.DQIdMax = HL_MATH_MAX(currDQId, p_codec->layers.DQIdMax);
                p_codec->layers.DQIdMin = HL_MATH_MIN(currDQId, p_codec->layers.DQIdMin);
                ++numEntries;
            }
            p_codec->layers.DependencyIdMax = (p_codec->layers.DQIdMax >> 4);
        }

        // Init variables depending on SPS
        if (p_codec->sps.pc_active != pc_slice->p_header->pc_pps->pc_sps) {
            p_codec->sps.pc_active = pc_slice->p_header->pc_pps->pc_sps;
            // Init DPB
            err = hl_codec_264_dpb_init(p_codec->pc_dpb, p_codec->sps.pc_active);
            if (err) {
                return err;
            }
            // Allocate Macroblocks
            if (pc_layer->u_list_macroblocks_count < p_codec->sps.pc_active->uPicSizeInMapUnits) {
                HL_LIST_STATIC_SAFE_FREE_OBJECTS(pc_layer->pp_list_macroblocks, pc_layer->u_list_macroblocks_count); // Must not realloc as the list contains objects
                if (!(pc_layer->pp_list_macroblocks = hl_memory_calloc(p_codec->sps.pc_active->uPicSizeInMapUnits, sizeof(hl_codec_264_mb_t*)))) {
                    pc_layer->u_list_macroblocks_count = 0;
                    return HL_ERROR_OUTOFMEMMORY;
                }
                pc_layer->u_list_macroblocks_count = p_codec->sps.pc_active->uPicSizeInMapUnits;
            }
            // FIXME: is "MaxPicNum" needed?
            p_codec->pc_poc->MaxFrameNum = p_codec->pc_poc->MaxPicNum = p_codec->sps.pc_active->uMaxFrameNum;
        }

        if (pc_layer->b_new_pict) {
            // map current FS memory to the right place in the DPB
            err = hl_codec_264_dpb_map_current(p_codec, p_codec->pc_dpb);
            if (err) {
                return err;
            }
        }

        // 8.2.1 Decoding process for picture order count
        // Must be called even if not new pict (e.g. for BOT, TOP)
        err = hl_codec_264_poc_decode(p_codec->pc_poc, p_codec);
        if (err) {
            return err;
        }

        // 8.2.2 Decoding process for macroblock to slice group map
        err = hl_codec_264_fmo_read_mb_to_slice_group_map(p_codec);
        if (err) {
            return err;
        }

        // 8.2.4 Decoding process for reference picture lists construction
        err = hl_codec_264_reflist_init(p_codec);
        if (err) {
            return err;
        }

        //if (p_codec->layers.currDQId == 0) {
        // AVC: slice_data( ) /* 7.3.4 Slice data syntax, 7.4.4 Slice data semantics */
        // SVC: slice_data_in_scalable_extension( ) /* G.7.3.4.1 Slice data in scalable extension syntax */
        err = hl_codec_264_nal_slice_data_decode(p_codec);
        if (err) {
            return err;
        }
        //}
        //else {
        //	if (pc_layer->SpatialResolutionChangeFlag) {
        //		/* G.8.1.3.2 Base decoding process for layer representations with resolution change */
        //		err = hl_codec_264_nal_slice_data_svc_decode_with_res_change(p_codec);
        //		if (err) {
        //			return err;
        //		}
        //	}
        //	else {
        //		/* G.8.1.3.1 Base decoding process for layer representations without resolution change */
        //		err = hl_codec_264_nal_slice_data_svc_decode_without_res_change(p_codec);
        //		if (err) {
        //			return err;
        //		}
        //	}
        //}

        // Update whether it's new pic or not
        pc_layer->b_got_frame = (pc_layer->i_mb_decode_count >= (int32_t)pc_layer->pc_slice_hdr->PicSizeInMbs) ;
        pc_layer->b_new_pict = pc_layer->b_got_frame; // FIXME: is it correct? is it needed?

        // Add picture to the reference list
        if (pc_layer->b_got_frame) {
            ++pc_layer->i_pict_decode_count;
            // 8.2.5 Decoded reference picture marking process
            err = hl_codec_264_dpb_add_decoded(p_codec);
            if (err) {
                return err;
            }
        }

        // }
        return err;
    }
    default:
        HL_DEBUG_ERROR("%d not supported as valid NAL type", (int)e_type);
        return HL_ERROR_NOT_IMPLEMENTED;
    }
}
