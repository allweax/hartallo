#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

extern const hl_object_def_t *hl_codec_264_layer_def_t;

HL_ERROR_T hl_codec_264_layer_create(hl_codec_264_layer_t** pp_layer, uint32_t DQId)
{
    HL_ERROR_T err;
    if (!pp_layer) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_layer = hl_object_create(hl_codec_264_layer_def_t);
    if (!*pp_layer) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    err = hl_codec_264_dpb_create(&(*pp_layer)->pobj_dpb);
    if (err) {
        HL_OBJECT_SAFE_FREE((*pp_layer));
        return err;
    }
    err = hl_codec_264_poc_create(&(*pp_layer)->pobj_poc);
    if (err) {
        HL_OBJECT_SAFE_FREE((*pp_layer));
        return err;
    }

    // FIXME: "MaxRefLayerDQId", "MinNoInterLayerPredFlag", ... must be reset when we finish decoding the layers

    (*pp_layer)->DQId = DQId;
    (*pp_layer)->MaxRefLayerDQId = -1;
    (*pp_layer)->MinNoInterLayerPredFlag = 1;
    (*pp_layer)->MaxTCoeffLevelPredFlag = 0;
    (*pp_layer)->b_new_pict = HL_TRUE;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_layer_init(hl_codec_264_layer_t* p_layer, struct hl_codec_264_slice_s* pc_slice, const struct hl_codec_264_s* pc_codec)
{
    if (!p_layer || !pc_slice) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    p_layer->pc_slice_curr = pc_slice;
    p_layer->pc_slice_hdr = p_layer->pc_slice_curr->p_header;
    p_layer->pc_ref = HL_NULL;
    if (p_layer->pc_slice_hdr->SVCExtFlag) {
        hl_size_t u;
        p_layer->SVCExtFlag = 1;
        p_layer->DQId = pc_codec->nal_current.ext.svc.DQId;
        p_layer->PriorityId = pc_codec->nal_current.ext.svc.priority_id;
        p_layer->NoInterLayerPredFlag = pc_codec->nal_current.ext.svc.no_inter_layer_pred_flag;
        p_layer->DependencyId = pc_codec->nal_current.ext.svc.dependency_id;
        p_layer->QualityId = pc_codec->nal_current.ext.svc.quality_id;
        p_layer->TemporalId = pc_codec->nal_current.ext.svc.temporal_id;
        p_layer->UseRefBasePicFlag = pc_codec->nal_current.ext.svc.use_ref_base_pic_flag;
        p_layer->StoreRefBasePicFlag = pc_codec->nal_current.prefix.store_ref_base_pic_flag;
        p_layer->DiscardableFlag = pc_codec->nal_current.ext.svc.discardable_flag;
        p_layer->OutputFlag = pc_codec->nal_current.ext.svc.output_flag;

        // "refLayerDQId( dqId )" ---> "Layers[dqId]->MaxRefLayerDQId"

        // The variable MinNoInterLayerPredFlag is set equal to the minimum value of no_inter_layer_pred_flag for the slices of
        // the layer representation.
        p_layer->MinNoInterLayerPredFlag = HL_MATH_MIN(p_layer->MinNoInterLayerPredFlag, p_layer->NoInterLayerPredFlag);

        // The variable MaxTCoeffLevelPredFlag is set equal to the maximum value of tcoeff_level_prediction_flag for the slices
        // of the current layer representation.
        p_layer->MaxTCoeffLevelPredFlag = HL_MATH_MAX(p_layer->MaxTCoeffLevelPredFlag, p_layer->pc_slice_hdr->ext.svc.tcoeff_level_prediction_flag );

        // The variable MaxRefLayerDQId is set equal to the maximum value of ref_layer_dq_id for the slices of the current layer
        // representation.
        // FIXME: should be set to -1 when we finish decoding all layers for this picture
        p_layer->MaxRefLayerDQId = HL_MATH_MAX(p_layer->MaxRefLayerDQId, p_layer->pc_slice_hdr->ext.svc.ref_layer_dq_id);
        if (p_layer->MaxRefLayerDQId >= HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT) {
            HL_DEBUG_ERROR("%d not valid as 'MaxRefLayerDQId' value", p_layer->MaxRefLayerDQId);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        // When MinNoInterLayerPredFlag is equal to 0, the layer representation inside the current coded picture that has a value
        // of DQId equal MaxRefLayerDQId is also referred to as reference layer representation.
        //if (p_layer->MinNoInterLayerPredFlag == 0 && p_layer->MaxRefLayerDQId >= 0) {
        //	p_layer->pc_ref = pc_codec->layers.p_list[p_layer->MaxRefLayerDQId];
        //}
        if (p_layer->MaxRefLayerDQId != -1) {
            const hl_codec_264_nal_slice_header_t* pc_slice_hdr_ref;
            const hl_codec_264_nal_sps_t* pc_sps_ref;
            if (!(p_layer->pc_ref = pc_codec->layers.p_list[p_layer->MaxRefLayerDQId])) {
                HL_DEBUG_WARN("Failed to find reference layer representaion");
                p_layer->pc_ref = p_layer;
            }

            pc_slice_hdr_ref = p_layer->pc_ref->pc_slice_hdr;
            pc_sps_ref = pc_slice_hdr_ref->pc_pps->pc_sps;

            p_layer->CroppingChangeFlag = (!pc_slice_hdr_ref->ext.svc.MinNoInterLayerPredFlag && pc_codec->nal_current.ext.svc.quality_id >0 && (pc_sps_ref->p_svc && pc_sps_ref->p_svc->extended_spatial_scalability_idc == HL_CODEC_264_ESS_PICT));

            p_layer->RefLayerPicSizeInMbs = pc_slice_hdr_ref->PicSizeInMbs;
            p_layer->RefLayerPicWidthInMbs = pc_slice_hdr_ref->PicWidthInMbs;
            p_layer->RefLayerPicHeightInMbs = pc_slice_hdr_ref->PicHeightInMbs;
            p_layer->RefLayerChromaFormatIdc = pc_sps_ref->chroma_format_idc;
            p_layer->RefLayerChromaArrayType = pc_sps_ref->ChromaArrayType;
            p_layer->RefLayerPicWidthInSamplesL = pc_slice_hdr_ref->PicWidthInSamplesL;
            p_layer->RefLayerPicHeightInSamplesL = pc_slice_hdr_ref->PicHeightInSamplesL;
            p_layer->RefLayerPicWidthInSamplesC = pc_slice_hdr_ref->PicWidthInSamplesC;
            p_layer->RefLayerPicHeightInSamplesC = pc_slice_hdr_ref->PicHeightInSamplesC;
            p_layer->RefLayerMbWidthC = pc_sps_ref->MbWidthC;
            p_layer->RefLayerMbHeightC = pc_sps_ref->MbHeightC;
            p_layer->RefLayerFrameMbsOnlyFlag = pc_sps_ref->frame_mbs_only_flag;
            p_layer->RefLayerFieldPicFlag = pc_slice_hdr_ref->field_pic_flag;
            p_layer->RefLayerBottomFieldFlag = pc_slice_hdr_ref->bottom_field_flag;
            p_layer->RefLayerMbaffFrameFlag = pc_slice_hdr_ref->MbaffFrameFlag;

            p_layer->SpatialResolutionChangeFlag = !(
                    p_layer->MinNoInterLayerPredFlag == 1
                    || (p_layer->QualityId > 0)
                    || (
                        p_layer->CroppingChangeFlag &&
                        p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerPicWidthInSamplesL == p_layer->RefLayerPicWidthInSamplesL &&
                        p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerPicHeightInSamplesL == p_layer->RefLayerPicHeightInSamplesL &&
                        (p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerLeftOffset & 15) == 0 &&
                        (p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerTopOffset % (16 * (1 + p_layer->pc_slice_hdr->field_pic_flag + p_layer->pc_slice_hdr->MbaffFrameFlag))) == 0 &&
                        p_layer->pc_slice_hdr->field_pic_flag == p_layer->RefLayerFieldPicFlag &&
                        p_layer->pc_slice_hdr->MbaffFrameFlag == p_layer->RefLayerMbaffFrameFlag &&
                        p_layer->pc_slice_hdr->pc_pps->pc_sps->chroma_format_idc == p_layer->RefLayerChromaFormatIdc
                        /*
                        	FIXME:
                        	– chroma_phase_x_plus1_flag is equal to ref_layer_chroma_phase_x_plus1_flag for the slices with
                        		no_inter_layer_pred_flag equal to 0,
                        	– chroma_phase_y_plus1 is equal to ref_layer_chroma_phase_y_plus1 for the slices with
                        		no_inter_layer_pred_flag equal to 0.
                        */
                    ));

            p_layer->RestrictedSpatialResolutionChangeFlag = (
                        p_layer->SpatialResolutionChangeFlag == 0 ||
                        (
                            (p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerPicWidthInSamplesL == p_layer->RefLayerPicWidthInSamplesL || p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerPicWidthInSamplesL == (p_layer->RefLayerPicWidthInSamplesL << 1)) &&
                            (p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerPicHeightInSamplesL == p_layer->RefLayerPicHeightInSamplesL || p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerPicHeightInSamplesL == (p_layer->RefLayerPicHeightInSamplesL << 1)) &&
                            (p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerLeftOffset & 15) == 0 &&
                            (p_layer->pc_slice_hdr->ext.svc.ScaledRefLayerTopOffset % (16 * (1 + p_layer->pc_slice_hdr->field_pic_flag))) == 0 &&
                            p_layer->pc_slice_hdr->MbaffFrameFlag == 0 &&
                            p_layer->RefLayerMbaffFrameFlag == 0 &&
                            p_layer->pc_slice_hdr->field_pic_flag == p_layer->RefLayerFieldPicFlag
                        ));
        }

        // allocate residual and constructed data (MUST be Zeros)
        {
            // Luma (residual and constructed)
            u = (p_layer->pc_slice_hdr->PicSizeInMbs * (16 * 16));
            if (!p_layer->rSL && !(p_layer->rSL = hl_memory_calloc(u, sizeof(int32_t)))) {
                return HL_ERROR_OUTOFMEMMORY;
            }
            // Chroma (residual and constructed)
            u = (p_layer->pc_slice_hdr->PicSizeInMbs * (p_layer->pc_slice_hdr->pc_pps->pc_sps->MbWidthC * p_layer->pc_slice_hdr->pc_pps->pc_sps->MbHeightC));
            if (!p_layer->rSCb && !(p_layer->rSCb = hl_memory_calloc(u, sizeof(int32_t)))) {
                return HL_ERROR_OUTOFMEMMORY;
            }
            if (!p_layer->rSCr && !(p_layer->rSCr = hl_memory_calloc(u, sizeof(int32_t)))) {
                return HL_ERROR_OUTOFMEMMORY;
            }
        }
    } // end-of "SVCExtFalg==1"
    else {
        p_layer->SVCExtFlag = 0;
        p_layer->DQId = 0;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_layer_find_by_dependency_id(hl_codec_264_layer_t** pp_layers, hl_size_t u_layers_count, int32_t dependency_id, struct hl_codec_264_layer_s** ppc_layer)
{
    hl_size_t u;
    for (u = 0; u < u_layers_count; ++u) {
        if (pp_layers[u] && pp_layers[u]->DependencyId == dependency_id) {
            *ppc_layer =  pp_layers[u];
            return HL_ERROR_SUCCESS;
        }
    }
    return HL_ERROR_NOT_FOUND;
}

// Alloc: "refSampleArray_ListPtr[u_idx]", "refSampleArrayAvailability_ListPtr[u_idx]" and "refTransBlkIdc_ListPtr[u_idx]"
HL_ERROR_T hl_codec_264_layer_alloc_refsamplearray_svc(hl_codec_264_layer_t* p_layer, hl_size_t u_idx)
{
    uint32_t u;
    if (!p_layer || u_idx >= HL_CODEC_264_SLICES_MAX_COUNT) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    u = (p_layer->pc_slice_hdr->PicSizeInMbs << 8); // FIXME: allocate only memory needed for this slice instead of overral picture.
    if (!(p_layer->refSampleArray_ListPtr[u_idx] = hl_memory_realloc(p_layer->refSampleArray_ListPtr[u_idx], u))) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    if (!(p_layer->refSampleArrayAvailability_ListPtr[u_idx] = hl_memory_realloc(p_layer->refSampleArrayAvailability_ListPtr[u_idx], u))) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    if (!(p_layer->refTransBlkIdc_ListPtr[u_idx] = hl_memory_realloc(p_layer->refTransBlkIdc_ListPtr[u_idx], u))) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

// Alloc: "tempArray_ListPtr[u_idx]"
HL_ERROR_T hl_codec_264_layer_alloc_temparray_svc(hl_codec_264_layer_t* p_layer, hl_size_t u_idx, hl_size_t count)
{
    hl_size_t size;
    if (!p_layer || u_idx >= HL_CODEC_264_SLICES_MAX_COUNT) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    size = count * sizeof(int32_t);
    if (!(p_layer->tempArray_ListPtr[u_idx] = hl_memory_realloc(p_layer->tempArray_ListPtr[u_idx], size))) {
        p_layer->tempArray_ListCount[u_idx] = 0;
        return HL_ERROR_OUTOFMEMMORY;
    }
    p_layer->tempArray_ListCount[u_idx] = count;
    return HL_ERROR_SUCCESS;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_layer_t" ***/
static hl_object_t* hl_codec_264_layer_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_layer_t *p_layer = (hl_codec_264_layer_t*)self;
    if (p_layer) {

    }
    return self;
}
static hl_object_t* hl_codec_264_layer_dtor(hl_object_t * self)
{
    hl_codec_264_layer_t *p_layer = (hl_codec_264_layer_t*)self;
    if (p_layer) {
        uint32_t u_idx;
        // Common
        HL_OBJECT_SAFE_FREE(p_layer->pobj_dpb);
        HL_OBJECT_SAFE_FREE(p_layer->pobj_poc);
        HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_layer->pp_list_macroblocks, p_layer->u_list_macroblocks_count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_layer->p_list_slices, p_layer->u_list_slices_count);
        HL_SAFE_FREE(p_layer->rSL);
        HL_SAFE_FREE(p_layer->rSCb);
        HL_SAFE_FREE(p_layer->rSCr);
        for (u_idx = 0; u_idx < HL_CODEC_264_SLICES_MAX_COUNT; ++u_idx) {
            HL_SAFE_FREE(p_layer->refSampleArray_ListPtr[u_idx]);
            HL_SAFE_FREE(p_layer->refSampleArrayAvailability_ListPtr[u_idx]);
            HL_SAFE_FREE(p_layer->refTransBlkIdc_ListPtr[u_idx]);
            HL_SAFE_FREE(p_layer->tempArray_ListPtr[u_idx]);
        }

        // Encoder
        HL_LIST_STATIC_CLEAR_OBJECTS(p_layer->encoder.p_list_esd, p_layer->encoder.u_p_list_esd_count);
    }
    return self;
}
static int hl_codec_264_layer_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_layer_def_s = {
    sizeof(hl_codec_264_layer_t),
    hl_codec_264_layer_ctor,
    hl_codec_264_layer_dtor,
    hl_codec_264_layer_cmp,
};
const hl_object_def_t *hl_codec_264_layer_def_t = &hl_codec_264_layer_def_s;
