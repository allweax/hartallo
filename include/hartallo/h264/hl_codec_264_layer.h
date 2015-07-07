#ifndef _HARTALLO_CODEC_264_LAYER_H_
#define _HARTALLO_CODEC_264_LAYER_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_nal.h"

HL_BEGIN_DECLS

HL_ERROR_T hl_codec_264_layer_create(struct hl_codec_264_layer_s** pp_layer, uint32_t DQId);
HL_ERROR_T hl_codec_264_layer_init(struct hl_codec_264_layer_s* p_layer, struct hl_codec_264_slice_s* pc_slice, const struct hl_codec_264_s* pc_codec);
HL_ERROR_T hl_codec_264_layer_find_by_dependency_id(struct hl_codec_264_layer_s** pp_layers, hl_size_t u_layers_count, int32_t dependency_id, struct hl_codec_264_layer_s** ppc_layer);
HL_ERROR_T hl_codec_264_layer_alloc_refsamplearray_svc(struct hl_codec_264_layer_s* p_layer, hl_size_t u_idx);
HL_ERROR_T hl_codec_264_layer_alloc_temparray_svc(struct hl_codec_264_layer_s* p_layer, hl_size_t u_idx, hl_size_t count);

typedef struct hl_codec_264_layer_s {
    HL_DECLARE_OBJECT;

    struct hl_codec_264_dpb_s* pobj_dpb;

    struct hl_codec_264_poc_s* pobj_poc;

    struct hl_codec_264_mb_s** pp_list_macroblocks;
    hl_size_t u_list_macroblocks_count; // number of macroblocks in the slices

    struct hl_codec_264_slice_s* p_list_slices[HL_CODEC_264_SLICES_MAX_COUNT]; // slice objetcs
    hl_size_t u_list_slices_count; // number of slices
    hl_size_t u_list_slices_idx; // index of the current slice
    struct hl_codec_264_slice_s* pc_slice_curr; // current slice

    int32_t i_mb_decode_count;
    int32_t i_mb_read_count;
    int32_t i_mb_encode_count;
    int32_t i_mb_write_count;
    int32_t i_pict_decode_count;
    int32_t i_pict_encode_count;
    hl_bool_t b_new_pict;
    hl_bool_t b_got_frame;

    uint32_t SVCExtFlag;

    int32_t* rSL; // only for SVC (residual) - FIXME: thread-safe?
    int32_t* rSCb; // only for SVC (residual) - FIXME: thread-safe?
    int32_t* rSCr; // only for SVC (residual) - FIXME: thread-safe?
    // hl_pixel_t* cSL; // only for SVC (constructed) - FIXME: use dpb->reconstructed?
    // hl_pixel_t* cSCb; // only for SVC (constructed) - FIXME: use dpb->reconstructed?
    // hl_pixel_t* cSCr; // only for SVC (constructed) - FIXME: use dpb->reconstructed?
    int32_t* refSampleArray_ListPtr[HL_CODEC_264_SLICES_MAX_COUNT]; // Only for SVC (temp variables)
    uint8_t* refSampleArrayAvailability_ListPtr[HL_CODEC_264_SLICES_MAX_COUNT]; // Only for SVC (temp variables)
    int32_t* refTransBlkIdc_ListPtr[HL_CODEC_264_SLICES_MAX_COUNT]; // Only for SVC (temp variables)

    int32_t* tempArray_ListPtr[HL_CODEC_264_SLICES_MAX_COUNT]; // Only for SVC (temp variables)
    hl_size_t tempArray_ListCount[HL_CODEC_264_SLICES_MAX_COUNT]; // Only for SVC (temp variables)

    uint32_t PriorityId; // syntax "priority_id" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t NoInterLayerPredFlag; // syntax "no_inter_layer_pred_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t DependencyId; // syntax "dependency_id" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t QualityId; // syntax "quality_id" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t TemporalId; // syntax "temporal_id" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t UseRefBasePicFlag; // syntax "use_ref_base_pic_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t StoreRefBasePicFlag; // syntax "store_ref_base_pic_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t DiscardableFlag; // syntax "discardable_flag" from NAL Prefix. /!\DO NOT derive() when decoding.
    uint32_t OutputFlag; // syntax "output_flag" from NAL Prefix. /!\DO NOT derive() when decoding.

    // derive()
    int32_t DQId;
    uint32_t MinNoInterLayerPredFlag;
    uint32_t CroppingChangeFlag;
    uint32_t SpatialResolutionChangeFlag;
    uint32_t RestrictedSpatialResolutionChangeFlag;
    int32_t MaxRefLayerDQId;
    int32_t ScaledRefLayerLeftOffset;
    int32_t ScaledRefLayerRightOffset;
    int32_t ScaledRefLayerTopOffset;
    int32_t ScaledRefLayerBottomOffset;
    int32_t ScaledRefLayerPicWidthInSamplesL;
    int32_t ScaledRefLayerPicHeightInSamplesL;
    int32_t ScaledRefLayerPicWidthInSamplesC;
    int32_t ScaledRefLayerPicHeightInSamplesC;
    int32_t InterlayerFilterOffsetA;
    int32_t InterlayerFilterOffsetB;
    uint32_t MaxTCoeffLevelPredFlag;

    int32_t RefLayerPicSizeInMbs;
    int32_t RefLayerPicWidthInMbs;
    int32_t RefLayerPicHeightInMbs;
    int32_t RefLayerChromaFormatIdc;
    int32_t RefLayerChromaArrayType;
    int32_t RefLayerPicWidthInSamplesL;
    int32_t RefLayerPicHeightInSamplesL;
    int32_t RefLayerPicWidthInSamplesC;
    int32_t RefLayerPicHeightInSamplesC;
    int32_t RefLayerMbWidthC;
    int32_t RefLayerMbHeightC;
    int32_t RefLayerFrameMbsOnlyFlag;
    int32_t RefLayerFieldPicFlag;
    int32_t RefLayerBottomFieldFlag;
    int32_t RefLayerMbaffFrameFlag;

    struct hl_codec_264_dpb_fs_s* pc_fs_curr; // current FS
    struct hl_codec_264_nal_slice_header_s* pc_slice_hdr;
    struct hl_codec_264_layer_s* pc_ref;

    struct {
        int32_t i_idr_pic_id; // "idr_pic_id" used in slice header
        int32_t i_slice_count; // number of slices to use for each frame

        struct hl_codec_264_encode_slice_data_s* p_list_esd[HL_CODEC_264_SLICES_MAX_COUNT];
        hl_size_t u_p_list_esd_count;
        int32_t async_mb_start[HL_CODEC_264_SLICES_MAX_COUNT];
        int32_t async_mb_end[HL_CODEC_264_SLICES_MAX_COUNT];
    } encoder;
}
hl_codec_264_layer_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_LAYER_H_ */
