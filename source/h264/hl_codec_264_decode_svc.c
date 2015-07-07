#include "hartallo/h264/hl_codec_264_decode_svc.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_deblock.h"
#include "hartallo/h264/hl_codec_264_transf.h"
#include "hartallo/h264/hl_codec_264_quant.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_pred_intra.h"
#include "hartallo/h264/hl_codec_264_pred_inter.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

static HL_ERROR_T _hl_codec_264_decode_svc_without_res_change(hl_codec_264_t* p_codec);
static HL_ERROR_T _hl_codec_264_decode_svc_layer_rep_with_res_change(hl_codec_264_t* p_codec);
static HL_ERROR_T _hl_codec_264_decode_svc_layer_target(hl_codec_264_t* p_codec, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_slice_with_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_slice_without_res_change(hl_codec_264_t* p_codec, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_mb_without_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_mb_with_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_mb_prior_to_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_mb_target(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_modes(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_modes_4x4(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbResCb[16][16],	HL_ALIGNED(16) const int32_t mbResCr[16][16],
    hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbResCb[16][16],	HL_ALIGNED(16) const int32_t mbResCr[16][16],
    hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma_cat12(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbResCb[16][16],	HL_ALIGNED(16) const int32_t mbResCr[16][16],
    hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_luma_or_cat3(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbResL[16][16],
    hl_pixel_t* picSamplesL
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_luma_and_cat3_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_4x4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_8x8(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
);
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_16x16(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
);
static HL_ERROR_T _hl_codec_264_decode_svc_inter(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t targetQId,
    hl_pixel_t* picSamplesL, hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_transform_coeff_scaling_and_refinement(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t refinementFlag
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construction_and_accumulation(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t accumulationFlag,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo,
    int32_t* picResL, int32_t* picResCb, int32_t* picResCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_luma_or_cat3(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iYCbCr,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, int32_t* sTCoeff, int32_t* tCoeffLevel,
    int32_t qPyPrime, int32_t qPCbPrime, int32_t qPCrPrime
);
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, int32_t* sTCoeff, int32_t* tCoeffLevel,
    int32_t qPCbPrime, int32_t qPCrPrime
);
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_residual_4x4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    int32_t qP,
    int32_t cO,
    const int32_t coeffLevel[16][16],
    int32_t* sTCoeff,
    int32_t* tCoeffLevel
);
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_intra_16x16(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    int32_t qP,
    int32_t cO,
    const int32_t coeffDCLevel[16],
    const int32_t coeffACLevel[16][16],
    const int32_t coeffLevel[16][16],
    int32_t* sTCoeff,
    int32_t* tCoeffLevel
);
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_cat12(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iCbCr,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, int32_t* sTCoeff, int32_t* tCoeffLevel,
    int32_t qPCbPrime, int32_t qPCrPrime
);
static HL_ERROR_T _hl_codec_264_decode_svc_transform_coeff_level_scaling_prior_to_refinement(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t tQPY, int32_t tQPCb, int32_t tQPCr,
    int32_t refQPY, int32_t refQPCb, int32_t refQPCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t iYCbCr, HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, HL_ALIGNED(16) int32_t mbRes[16][16]);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct4x4(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t cO, int32_t bitDepth,
    HL_OUT int32_t mbRes[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_intra_16x16(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t cO, int32_t bitDepth,
    HL_OUT int32_t mbRes[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, const int32_t* sTCoeff,
    HL_ALIGNED(16) int32_t mbResCb[16][16], HL_ALIGNED(16) int32_t mbResCr[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_chroma_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iCbCr, const int32_t* sTCoeff,
    HL_ALIGNED(16) int32_t mbRes[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_chroma_not_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iCbCr, const int32_t* sTCoeff,
    HL_ALIGNED(16) int32_t mbRes[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_accumulation(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    const int32_t* picResL, const int32_t* picResCb, const int32_t* picResCr,
    hl_pixel_t* picSamplesL, hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_pict_construct(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    void* picArrayL, void* picArrayCb, void* picArrayCr, hl_size_t picSampleSize,
    HL_ALIGNED(16) const int32_t mbArrayL[16][16], HL_ALIGNED(16) const int32_t mbArrayCb[16][16], HL_ALIGNED(16) const int32_t mbArrayCr[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_extract(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_pixel_t* picArrayL, const hl_pixel_t* picArrayCb, const hl_pixel_t* picArrayCr,
    HL_ALIGNED(16) int32_t mbArrayL[16][16], HL_ALIGNED(16) int32_t mbArrayCb[16][16], HL_ALIGNED(16) int32_t mbArrayCr[16][16]
);
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_pict_construct_comps(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    void* picArray, hl_size_t picSampleSize,
    HL_ALIGNED(16) const int32_t mbArray[16][16], int32_t mbW, int32_t mbH
);
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_extract_colour_comp(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    const hl_pixel_t* picArray,
    HL_ALIGNED(16) int32_t mbArray[16][16], int32_t mbW, int32_t mbH
);
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_reinit(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    void* picSamplesL, void* picSamplesCb, void* picSamplesCr, hl_size_t picSampleSize
);
static HL_ERROR_T _hl_codec_264_decode_svc_resample_intra(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    hl_pixel_t* picSamplesL, hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
);
/*static*/ HL_ERROR_T _hl_codec_264_decode_svc_resample_intra_colour_comps(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t chromaFlag, int32_t iCbCr,
        int32_t mbW, int32_t mbH, HL_OUT int32_t mbPred[16][16]
                                                                          );
static HL_ERROR_T _hl_codec_264_decode_svc_ref_layer_array_construct_prior_to_intra_resampling(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
        int32_t chromaFlag, int32_t iCbCr, int32_t mbW, int32_t mbH, int32_t botFieldFlag,
        int32_t* refArrayW, int32_t* refArrayH, HL_OUT int32_t* refSampleArray, HL_OUT uint8_t* refSampleArrayAvailability,
        int32_t* xOffset, int32_t* yOffset
                                                                                              );
static HL_ERROR_T _hl_codec_264_decode_svc_interpol_intra_base(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t filteringModeFlag,
    int32_t chromaFlag,
    int32_t botFieldFlag,
    int32_t fldPrdInFrmMbFlag,
    int32_t yBorder,
    int32_t refArrayW, int32_t refArrayH, int32_t* refSampleArray,
    int32_t xOffset, int32_t yOffset,
    int32_t mbW, int32_t mbH, HL_OUT int32_t* predArray, int32_t predArrayStride
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_resampling(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    const int32_t* refLayerPicSamplesL, const int32_t* refLayerPicSamplesCb, const int32_t* refLayerPicSamplesCr,
    int32_t* picSamplesL, int32_t* picSamplesCb, int32_t* picSamplesCr
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_resampling_mb_colour_comps(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t chromaFlag,
    int32_t mbW, int32_t mbH,
    int32_t mbPred[16][16],
    const int32_t* refLayerPicSamples
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_ref_layer_construction_prior_to_resampling(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t chromaFlag,
    int32_t mbW, int32_t mbH,
    int32_t botFieldFlag,
    int32_t yBorder,
    const int32_t* refLayerPicSamples,
    HL_OUT int32_t *refArrayW, int32_t *refArrayH,
    HL_OUT int32_t* refSampleArray,
    HL_OUT int32_t* refTransBlkIdc,
    HL_OUT int32_t* xOffset, int32_t *yOffset
);
static HL_ERROR_T _hl_codec_264_decode_svc_residual_interpol(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t chromaFlag,
    int32_t mbW, int32_t mbH,
    int32_t botFieldFlag,
    int32_t fldPrdInFrmMbFlag,
    int32_t yBorder,
    int32_t refArrayW, int32_t refArrayH,
    const int32_t* refSampleArray,
    const int32_t* refTransBlkIdc,
    int32_t xOffset, int32_t yOffset,
    int32_t* predArray, int32_t predArrayStride
);
// FIXME: remove flowchart
// http://www.scribd.com/doc/19835181/H264G8Flowchartv1

// G.8 SVC decoding process
// G.8.1.3 Layer representation decoding processes
// Subclause G.8.1.3.1 specifies the base decoding process for layer representations without resolution change.
// Subclause G.8.1.3.2 specifies the base decoding process for layer representations with resolution change.
// Subclause G.8.1.3.3 specifies the target layer representation decoding process
HL_ERROR_T hl_codec_264_decode_svc(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t *pc_layer_active = p_codec->layers.pc_active;
    hl_bool_t b_decode_layer = HL_FALSE, b_target_layer = HL_FALSE; // standard-defined
    hl_bool_t b_output_layer = HL_FALSE; // user-defined

    if (pc_layer_active->DQId > 0/* &&  pc_layer_active->i_pict_decode_count >= 2*/) {
        // FIXME:
        int kaka = 0;
    }

    // TODO: Check "currDQId" in "dqIdList"
    b_decode_layer = (p_codec->layers.currDQId >= p_codec->layers.DQIdMin && p_codec->layers.currDQId <= p_codec->layers.DQIdMax)
                     && (p_codec->pc_base->dqid_max < 0 || p_codec->layers.currDQId <= p_codec->pc_base->dqid_max); // decode up to "dqid_max"
    if (!b_decode_layer) {
        return HL_ERROR_SUCCESS;
    }

    //!\ begin-of-"DQIdMin..DQIdMax" processing...

    err = (pc_layer_active->SpatialResolutionChangeFlag)
          ? _hl_codec_264_decode_svc_layer_rep_with_res_change(p_codec)
          : _hl_codec_264_decode_svc_without_res_change(p_codec);
    if (err) {
        return err;
    }

    //!\ At this step we're sure we're dealing with a SVC layer

    if (p_codec->layers.currDQId == (p_codec->layers.DependencyIdMax << 4)) {
        hl_codec_264_layer_t *pc_layer;
        if (!hl_codec_264_layer_find_by_dependency_id(p_codec->layers.p_list, HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT, p_codec->layers.DependencyIdMax, &pc_layer) && pc_layer->StoreRefBasePicFlag) {
            // When currDQId is equal to (DependencyIdMax << 4) and store_ref_base_pic_flag for the dependency
            // representation with dependency_id equal to DependencyIdMax is equal to 1, the target layer representation
            // decoding process as specified in subclause G.8.1.3.3 is invoked with currDQId, refLayerVars (when
            // spatResChangeFlag is equal to 1), and currentVars as the inputs and the outputs are assigned to the sample
            // array BL and, when ChromaArrayType is not equal to 0, the sample arrays BCb and BCr.
            b_target_layer = HL_TRUE;
        }
    }

    //!\ end-of-"DQIdMin..DQIdMax" processing...

    b_output_layer |=
        (p_codec->pc_base->dqid_min < 0 || p_codec->layers.currDQId >= p_codec->pc_base->dqid_min) // output from "dqid_min"
        && (p_codec->pc_base->dqid_max < 0 || p_codec->layers.currDQId <= p_codec->pc_base->dqid_max); // output up to "dqid_max"

    b_target_layer |= (p_codec->layers.currDQId == p_codec->layers.DQIdMax);

    //!\ "G.8.1.3.3" is only called for the layers for which we really want to output a picture. This is way the standrd says to put "currDQId" equal to "DQIdMax".
    // We output all layers requested by the user (or at least its dependencies)
    if (b_target_layer) {
        // The target layer representation decoding process as specified in subclause G.8.1.3.3 is invoked with currDQId set equal
        // to DQIdMax, refLayerVars (when the variable SpatialResolutionChangeFlag of the layer representation with DQId
        // equal to DQIdMax is equal to 1), and currentVars as the inputs and the outputs are assigned to the sample array SL and,
        // when ChromaArrayType is not equal to 0, the sample arrays SCb and SCr.
        // NOTE 3 – The sample arrays SL, SCb, and SCr represent the decoded picture for the access unit.

        // G.8.1.3.3 Target layer representation decoding process
        err = _hl_codec_264_decode_svc_layer_target(p_codec, p_codec->layers.currDQId);
        if (err) {
            return err;
        }
    }

    // FIXME:
    // The SVC decoded reference picture marking process as specified in subclause G.8.2.4 is invoked with dqIdList as the
    // input.

    // FIXME:
    if (b_output_layer) {
        char name[1024] = {0};
        FILE* p_file;
        hl_size_t u;

        sprintf(name, "./svc_DQId[%d]_Frame[%d].yuv", pc_layer_active->DQId, pc_layer_active->i_pict_decode_count);
        p_file = fopen(name, "wb+");


        u = (pc_layer_active->pc_slice_hdr->PicSizeInMbs * (16 * 16));
        fwrite(pc_layer_active->pc_fs_curr->p_pict->pc_data_y, sizeof(hl_pixel_t), u, p_file);
        u = (pc_layer_active->pc_slice_hdr->PicSizeInMbs * (pc_layer_active->pc_slice_hdr->pc_pps->pc_sps->MbWidthC * pc_layer_active->pc_slice_hdr->pc_pps->pc_sps->MbHeightC));
        fwrite(pc_layer_active->pc_fs_curr->p_pict->pc_data_u, sizeof(hl_pixel_t), u, p_file);
        fwrite(pc_layer_active->pc_fs_curr->p_pict->pc_data_v, sizeof(hl_pixel_t), u, p_file);
        fclose(p_file);
    }


    return err;
}

// G.8.1.3.1 Base decoding process for layer representations without resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_without_res_change(hl_codec_264_t* p_codec)
{
    // This function could be called for the AVC base layer to rewrite some varialbles (e.g. cTrafo, sTCoeff...)
    hl_codec_264_layer_t *pc_layer = p_codec->layers.pc_active;
    int32_t currDQId = pc_layer->DQId;

    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (pc_layer->MinNoInterLayerPredFlag) {
        // G.8.1.2.1 Array assignment and initialisation process
        // FIXME: do we really need to set all values to "unspecified"?
        // Looks like yes (at least to reset "sTCoeff" )
        // G.8.1.2.1 Array assignment and initialisation process
        //err = hl_codec_264_utils_array_assignment_and_initialisation_svc(p_codec);
        //if (err) {
        //	return err;
        //}
    }
    else {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
        // if (!pc_layer->MaxTCoeffLevelPredFlag) {
        //
        // }
    }

    // G.8.1.4.1 Base decoding process for slices without resolution change
    err = _hl_codec_264_decode_svc_slice_without_res_change(p_codec, currDQId);
    if (err) {
        return err;
    }

    return err;
}

// G.8.1.3.2 Base decoding process for layer representations with resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_layer_rep_with_res_change(hl_codec_264_t* p_codec)
{
    HL_ERROR_T err;
    hl_codec_264_layer_t *pc_layer_active, *pc_layer_new;
    hl_codec_264_mb_t* pc_mb;
    int32_t currDQId, mbAddr, i_list_macroblocks_count;

    // Save active layer
    pc_layer_active = p_codec->layers.pc_active;

    (pc_layer_new);

#if 0 // FIXME: part of the standard but decode the lower layer twice - > WHY?
    /*	G.8.1.5.5 Macroblock decoding process prior to resolution change
    	The macroblock address mbAddr proceeds over the values 0..(RefLayerPicSizeInMbs - 1), and for each
    	macroblock address mbAddr, the macroblock decoding process prior to resolution change as specified in
    	subclause G.8.1.5.5 is invoked with currDQId set equal to MaxRefLayerDQId, mbAddr, and currentVars as
    	the inputs and the output is a modified version of currentVars.
    */
    currDQId = p_codec->layers.pc_active->MaxRefLayerDQId;
    pc_layer_new = p_codec->layers.p_list[currDQId];
    if (!pc_layer_new) {
        HL_DEBUG_ERROR("Failed to find base layer with DQId=%d", currDQId);
        return HL_ERROR_INVALID_BITSTREAM;
    }
    // Change new active layer
    p_codec->layers.pc_active = pc_layer_new;
    // Make sure the current frame store is not used as reference
    if (!pc_layer_new->pc_fs_curr || HL_CODEC_264_REF_TYPE_IS_USED(pc_layer_new->pc_fs_curr->RefType)) {
        // map current FS memory to the right place in the DPB
        err = hl_codec_264_dpb_map_current(p_codec, pc_layer_new->pobj_dpb);
        if (err) {
            return err;
        }
    }
    i_list_macroblocks_count = pc_layer_active->RefLayerPicSizeInMbs;
    for (mbAddr = 0; mbAddr < i_list_macroblocks_count; ++mbAddr) {
        if (!(pc_mb = p_codec->layers.pc_active->pp_list_macroblocks[mbAddr])) {
            HL_DEBUG_ERROR("Failed to find macrobloc with addr=%d in layer with DQId=%d", mbAddr, currDQId);
            p_codec->layers.pc_active = pc_layer_active; // restore active layer
            return HL_ERROR_INVALID_BITSTREAM;
        }
        // G.8.1.5.5 Macroblock decoding process prior to resolution change
        err = _hl_codec_264_decode_svc_mb_prior_to_res_change(p_codec, pc_mb, currDQId);
        if (err) {
            return err;
        }
    }
#endif

    // Restore active layer
    p_codec->layers.pc_active = pc_layer_active;
    currDQId = pc_layer_active->DQId;
    i_list_macroblocks_count = (int32_t)pc_layer_active->u_list_macroblocks_count;

    // G.8.7.1 Deblocking filter process for Intra_Base prediction
    err = hl_codec_264_deblock_intra_base_svc(p_codec, currDQId);
    if (err) {
        return err;
    }

    // 4. The array assignment and initialisation process as specified in subclause G.8.1.2.1 is invoked and the output is
    // assigned to the collective term currentVars.
    // FIXME: done in "hl_codec_264_utils_derivation_process_initialisation_svc()"

    // FIXME: Up to the caller to be sure macroblocks are in slice with layer with "DQId=MaxRefLayerDQId".
    // Let setOfSlices be the set of all slices of the current layer representation with DQId equal to currDQId. For
    // each slice of the set setOfSlices, the base decoding process for slices with resolution change as specified in
    // subclause G.8.1.4.2 is invoked with currSlice representing the currently processed slice, currDQId,
    // refLayerVars, and currentVars as the inputs and the output is a modified version of currentVars.
    for (mbAddr = 0; mbAddr < i_list_macroblocks_count; ++mbAddr) {
        if (!(pc_mb = p_codec->layers.pc_active->pp_list_macroblocks[mbAddr])) {
            HL_DEBUG_ERROR("Failed to find macrobloc with addr=%d in layer with DQId=%d", mbAddr, currDQId);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        err = _hl_codec_264_decode_svc_slice_with_res_change(p_codec, pc_mb, currDQId);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.1.3.3 Target layer representation decoding process
static HL_ERROR_T _hl_codec_264_decode_svc_layer_target(hl_codec_264_t* p_codec, int32_t currDQId)
{
    hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    // const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    int32_t sliceType, refPicList0, refPicList1;
    uint32_t mbAddr, firstMbInSlice, firstMbAddrInSlice;
    hl_codec_264_mb_t* pc_mb;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    for (mbAddr = 0; mbAddr < pc_layer->pc_slice_hdr->PicSizeInMbs; ++mbAddr) {
        pc_mb = pc_layer->pp_list_macroblocks[mbAddr];
        // FIXME
        // Let currSlice specify the slice of the layer representation with DQId equal to
        // ( ( ( sliceIdc[ mbAddr ] & 127 ) >> 4 ) << 4 ) that covers the macroblock with macroblock address
        // ( ( sliceIdc[ mbAddr ] >> 7 ) * ( 1 + MbaffFrameFlag ) ).

        firstMbInSlice = pc_slice_header->first_mb_in_slice;
        sliceType = pc_slice_header->slice_type;
        firstMbAddrInSlice  = (firstMbInSlice * ( 1 + pc_slice_header->MbaffFrameFlag ));
        refPicList0 = -1;
        refPicList1 = -1;

        if (currDQId == 16 && pc_layer->i_pict_decode_count == 2 && pc_mb->u_addr == 124) {
            int kaka = 0; // FIXME
        }

        if (pc_slice_header->SliceTypeModulo5 < 2) {
            if (mbAddr > firstMbAddrInSlice) {
                // FIXME
                // the reference picture list refPicList0 is set equal to the
                // reference picture list refPicList0 that was derived for the macroblock address mbAddr equal to
                // firstMbAddrInSlice inside this subclause and, when (sliceType % 5) is equal to 1, the reference
                // picture list refPicList1 is set equal to the reference picture list refPicList1 that was derived for the
                // macroblock address mbAddr equal to firstMbAddrInSlice inside this subclause.
            }
            else {
                // G.8.2.3 SVC decoding process for reference picture lists construction
                // HL_DEBUG_ERROR("Not implemented");
                // return HL_ERROR_NOT_IMPLEMENTED;
                // FIXME: standard says to call "G.8.2.3"
            }
        }
        // G.8.1.5.6 Target macroblock decoding process
        err = _hl_codec_264_decode_svc_mb_target(p_codec, pc_mb, currDQId);
        if (err) {
            return err;
        }
        ++pc_layer->i_mb_decode_count;

#if 0
        // FIXME
        if (currDQId == 16 && pc_layer->i_pict_decode_count == (1 - 1) && pc_mb->u_addr >= 1) {
            err = hl_codec_264_mb_print_samples(pc_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_slice_header->PicWidthInSamplesL, 0/*Y*/);
            // err = hl_codec_264_mb_print_samples(pc_mb, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_slice_header->PicWidthInSamplesC, 1/*U*/);
        }
#endif
    }

    // G.8.7.2 Deblocking filter process for target representations
    err = hl_codec_264_deblock_target_reps_svc(p_codec, currDQId);
    if (err) {
        return err;
    }

    return err;
}

// G.8.1.4.1 Base decoding process for slices without resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_slice_without_res_change(hl_codec_264_t* p_codec, int32_t currDQId)
{
    hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];
    hl_codec_264_mb_t* pc_mb;
    hl_size_t u;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    // FIXME: Not for a macroblock but entire slice
    // When currDQId is equal to 0 and (slice_type % 5) is equal to 1, the SVC decoding process for reference picture lists
    // construction as specified in subclause G.8.2.3 is invoked with currDependencyId equal to 0, useRefBasePicFlag equal
    // to use_ref_base_pic_flag, and the current slice as input and the output is the reference picture list refPicList1.
    // if (currDQId == 0 && pc_layer->pc_slice_hdr->SliceTypeModulo5 == 1) {
    // }

    // G.8.1.5.2 Base decoding process for macroblocks in slices without resolution change
    for (u = 0; u < pc_layer->pc_slice_hdr->PicSizeInMbs; ++u) {
        pc_mb = pc_layer->pp_list_macroblocks[u];
        err = _hl_codec_264_decode_svc_mb_without_res_change(p_codec, pc_mb, currDQId);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.1.4.2 Base decoding process for slices with resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_slice_with_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId)
{
    hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];
    // const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    // const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    if (pc_layer->DQId == 16 && pc_layer->i_pict_decode_count == (1 - 1) && p_mb->u_addr == 1) {
        // FIXME:
        int kaka = 0;
    }

    if (pc_layer->CroppingChangeFlag == 1 && p_codec->layers.pc_active->pc_slice_hdr->SliceTypeModulo5 < 2) {
        /*	FIXME:
        	When CroppingChangeFlag is equal to 1 and (slice_type % 5) is less than 2, the SVC decoding process for reference
        	picture lists construction as specified in subclause G.8.2.3 is invoked with currDependencyId equal to dependency_id,
        	useRefBasePicFlag equal to use_ref_base_pic_flag, and the current slice as the inputs and the outputs are the reference
        	picture list refPicList0 and, when (sliceType % 5) is equal to 1, the reference picture list refPicList1.
        */
    }
    // G.8.1.5.3 Base decoding process for macroblocks in slices with resolution change
    return _hl_codec_264_decode_svc_mb_with_res_change(p_codec, p_mb, currDQId);
}

// G.8.1.5.2 Base decoding process for macroblocks in slices without resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_mb_without_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId)
{
    hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    int32_t refQPY, refQPCb, refQPCr, refLayerIntraBLFlag, resPredFlag;
    HL_ERROR_T err;

    if (pc_slice_header->ext.svc.tcoeff_level_prediction_flag) {
        refQPY = p_mb->ext.svc.tQPy;
        refQPCb = p_mb->ext.svc.tQPCb;
        refQPCr = p_mb->ext.svc.tQPCr;
    }
    refLayerIntraBLFlag =  (!p_codec->nal_current.ext.svc.no_inter_layer_pred_flag) && HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb);
    resPredFlag = p_mb->ext.svc.residual_prediction_flag &&
                  (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) ||
                   HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ||
                   HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ||
                   HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) ||
                   HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb) ||
                   HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb));

    // G.8.1.5.1 Macroblock initialisation process
    err = hl_codec_264_utils_derivation_process_initialisation_svc(p_codec, p_mb);
    if (err) {
        return err;
    }

    // G.8.4.1 SVC derivation process for motion vector components and reference indices
    err = hl_codec_264_utils_derivation_process_for_mv_comps_and_ref_indices_svc(p_codec, p_mb);
    if (err) {
        return err;
    }

    if (
        HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) ||
        HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) ||
        HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) ||
        HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)
    ) {
        if (!p_mb->ext.svc.base_mode_flag) {
            // G.8.3.1 SVC derivation process for intra prediction modes
            err = _hl_codec_264_decode_svc_intra_pred_modes(p_codec, p_mb);
            if (err) {
                return err;
            }
        }
        if (p_mb->ext.svc.base_mode_flag && pc_slice_header->ext.svc.tcoeff_level_prediction_flag) {
            // G.8.5.2
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }

        // G.8.5.1 Transform coefficient scaling and refinement process
        err = _hl_codec_264_decode_svc_transform_coeff_scaling_and_refinement(p_codec, p_mb, p_mb->ext.svc.base_mode_flag /* refinementFlag */);
        if (err) {
            return err;
        }

        // G.8.5.5 Sample array re-initialisation process
        err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr, sizeof(pc_layer->rSL[0]));
        if (err) {
            return err;
        }
        // G.8.5.5 Sample array re-initialisation process
        err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v, sizeof(pc_layer->pc_fs_curr->p_pict->pc_data_y[0]));
        if (err) {
            return err;
        }
    }
    else if (HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) {
        // G.8.5.1
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        if (pc_slice_header->ext.svc.tcoeff_level_prediction_flag && resPredFlag) {
            // G.8.5.2
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }

        // G.8.5.1 Transform coefficient scaling and refinement process
        err = _hl_codec_264_decode_svc_transform_coeff_scaling_and_refinement(p_codec, p_mb, resPredFlag /* refinementFlag */);
        if (err) {
            return err;
        }

        if (!resPredFlag) {
            // G.8.5.5 Sample array re-initialisation process
            err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr, sizeof(pc_layer->rSL[0]));
            if (err) {
                return err;
            }
            // G.8.5.5 Sample array re-initialisation process
            err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v, sizeof(pc_layer->pc_fs_curr->p_pict->pc_data_y[0]));
            if (err) {
                return err;
            }
        }
    }

    p_mb->MvCnt = p_mb->ext.svc.mvCnt;
    return err;
}

// G.8.1.5.3 Base decoding process for macroblocks in slices with resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_mb_with_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId)
{
    hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    int32_t intraResamplingFlag;
    HL_ERROR_T err;
    static const int32_t refinementFlag = 0;

    // G.8.1.5.1 Macroblock initialisation process
    err = hl_codec_264_utils_derivation_process_initialisation_svc(p_codec, p_mb);
    if (err) {
        return err;
    }

    // G.8.4.1 SVC derivation process for motion vector components and reference indices
    err = hl_codec_264_utils_derivation_process_for_mv_comps_and_ref_indices_svc(p_codec, p_mb);
    if (err) {
        return err;
    }

    intraResamplingFlag = HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb) ||
                          (pc_layer->RestrictedSpatialResolutionChangeFlag == 0 && pc_slice_header->MbaffFrameFlag == 0 && pc_layer->RefLayerMbaffFrameFlag == 0 && p_mb->ext.svc.base_mode_flag == 1);

    if (intraResamplingFlag) {
        // G.8.6.2 Resampling process for intra samples
        err = _hl_codec_264_decode_svc_resample_intra(p_codec, p_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v);
        if (err) {
            return err;
        }
    }

    if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)) {
        // G.8.3.1 SVC derivation process for intra prediction modes
        err = _hl_codec_264_decode_svc_intra_pred_modes(p_codec, p_mb);
        if (err) {
            return err;
        }
    }
    else if (!HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb) && p_mb->ext.svc.residual_prediction_flag) {
        // G.8.6.3 Resampling process for residual samples
        err = _hl_codec_264_decode_svc_residual_resampling(
                  p_codec, p_mb,
                  pc_layer->pc_ref->rSL, pc_layer->pc_ref->rSCb, pc_layer->pc_ref->rSCr,
                  pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr);
        if (err) {
            return err;
        }
    }
    else if (HL_CODEC_264_MB_TYPE_IS_INTER(p_mb)) { // FIXME: not part of the standard but used to avoid calling G.8.1.2.1
        err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr, sizeof(pc_layer->rSL[0]));
        if (err) {
            return err;
        }
    }

    // G.8.5.1 Transform coefficient scaling and refinement process
    err = _hl_codec_264_decode_svc_transform_coeff_scaling_and_refinement(p_codec, p_mb, refinementFlag);
    if (err) {
        return err;
    }

    p_mb->MvCnt = p_mb->ext.svc.mvCnt;
    return err;
}

// G.8.1.5.5 Macroblock decoding process prior to resolution change
static HL_ERROR_T _hl_codec_264_decode_svc_mb_prior_to_res_change(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];

    if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)) {
        // G.8.3.2 SVC intra sample prediction and construction process
        err = _hl_codec_264_decode_svc_intra_pred_and_construction(p_codec, p_mb);
        if (err) {
            return err;
        }
    }
    else if (HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) {
        static const int32_t accumulationFlag = 0;
        // G.8.5.3 Residual construction and accumulation process
        err = _hl_codec_264_decode_svc_residual_construction_and_accumulation(p_codec, p_mb, accumulationFlag, p_mb->ext.svc.cTrafo,
                pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr);
        if (err) {
            return err;
        }
        // G.8.5.4 Sample array accumulation process
        err = _hl_codec_264_decode_svc_sample_array_accumulation(p_codec, p_mb, pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v);
        if (err) {
            return err;
        }
        // G.8.5.5 Sample array re-initialisation process
        err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr, sizeof(pc_layer->rSL[0]));
        if (err) {
            return err;
        }
    }
    else {
        static const int32_t accumulationFlag = 1;
        // G.8.5.3 Residual construction and accumulation process
        err = _hl_codec_264_decode_svc_residual_construction_and_accumulation(p_codec, p_mb, accumulationFlag, p_mb->ext.svc.cTrafo,
                pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr);
        if (err) {
            return err;
        }
        // G.8.5.5 Sample array re-initialisation process
        err = _hl_codec_264_decode_svc_sample_array_reinit(p_codec, p_mb, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v, sizeof(pc_layer->pc_fs_curr->p_pict->pc_data_y[0]));
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.1.5.6 Target macroblock decoding process
static HL_ERROR_T _hl_codec_264_decode_svc_mb_target(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t currDQId)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.p_list[currDQId];
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    if (currDQId == 16 && pc_layer->i_pict_decode_count == (1 - 1) && p_mb->u_addr == 1) {
        int kaka = 0;// FIXME
    }

    if (pc_layer->MaxTCoeffLevelPredFlag && (p_mb->ext.svc.sliceIdc & 127) != currDQId && pc_sps->ChromaArrayType) {
        int32_t cQPy, cQPCb, cQPCr, qPOffsetCb, qPOffsetCr, qPICb, qPICr;

        cQPy = p_mb->ext.svc.tQPy;
        // 8.5.8 Derivation process for chroma quantisation parameters
        // The variable cQPY is set equal to tQPY[ mbAddr ], and for CX being replaced by Cb and Cr, the variable
        // cQPCX is set equal to the value of QPCX that corresponds to a value of cQPY for QPY as specified in
        // subclause 8.5.8.
        qPOffsetCb = pc_slice_header->pc_pps->chroma_qp_index_offset; // (8-313)
        qPOffsetCr = pc_slice_header->pc_pps->second_chroma_qp_index_offset; // (8-314)
        qPICb = HL_MATH_CLIP3(-pc_slice_header->pc_pps->pc_sps->QpBdOffsetC, 51, p_mb->ext.svc.tQPy + qPOffsetCb); // (8-315)
        qPICr = HL_MATH_CLIP3(-pc_slice_header->pc_pps->pc_sps->QpBdOffsetC, 51, p_mb->ext.svc.tQPy + qPOffsetCr); // (8-315)
        cQPCb = qPI2QPC[qPICb];
        cQPCr = qPI2QPC[qPICr];

        // G.8.5.2 Transform coefficient level scaling process prior to transform coefficient refinement
        err = _hl_codec_264_decode_svc_transform_coeff_level_scaling_prior_to_refinement(
                  p_codec,
                  p_mb,
                  cQPy, cQPCb, cQPCr,
                  p_mb->ext.svc.tQPy, p_mb->ext.svc.tQPCb, p_mb->ext.svc.tQPCr);
        if (err) {
            return err;
        }
        p_mb->ext.svc.tQPCb = cQPCb;
        p_mb->ext.svc.tQPCr = cQPCr;

        // G.8.5.1
        // FIXME: ...inferred to be equal...
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)) {
        int32_t intraPredFlag;

        intraPredFlag = ((p_mb->ext.svc.sliceIdc & 127) == currDQId || pc_layer->MaxTCoeffLevelPredFlag);
        if (intraPredFlag) {
            // G.8.3.2 SVC intra sample prediction and construction process
            err = _hl_codec_264_decode_svc_intra_pred_and_construction(p_codec, p_mb);
            if (err) {
                return err;
            }
        }
    }
    else if (HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)) {
        static const int32_t accumulationFlag = 0;

        // G.8.5.3 Residual construction and accumulation process
        err = _hl_codec_264_decode_svc_residual_construction_and_accumulation(p_codec, p_mb, accumulationFlag, p_mb->ext.svc.cTrafo,
                pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr);
        if (err) {
            return err;
        }

        // G.8.5.4 Sample array accumulation process
        err = _hl_codec_264_decode_svc_sample_array_accumulation(p_codec, p_mb, pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v);
        if (err) {
            return err;
        }
    }
    else {
        int32_t targetQId;
        static const int32_t accumulationFlag = 1;

        targetQId = (currDQId & 15);
        // G.8.4.2 SVC decoding process for Inter prediction samples
        err = _hl_codec_264_decode_svc_inter(
                  p_codec,
                  p_mb,
                  targetQId,
                  pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v);
        if (err) {
            return err;
        }

        if (!HL_CODEC_264_MB_TYPE_IS_SKIP(p_mb)) { // FIXME: not part of the standard but added for performance reasons (/!\gathered residual is not correct)
            // G.8.5.3 Residual construction and accumulation process
            err = _hl_codec_264_decode_svc_residual_construction_and_accumulation(p_codec, p_mb, accumulationFlag, p_mb->ext.svc.cTrafo,
                    pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr);
            if (err) {
                return err;
            }

            // G.8.5.4 Sample array accumulation process
            err = _hl_codec_264_decode_svc_sample_array_accumulation(p_codec, p_mb,
                    pc_layer->rSL, pc_layer->rSCb, pc_layer->rSCr,
                    pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v);
            if (err) {
                return err;
            }
        }
    }

    return err;
}

// G.8.3.1 SVC derivation process for intra prediction modes
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_modes(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    if (!HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)) {
        if (HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)) {
            // G.8.3.1.1 SVC derivation process for Intra_4x4 prediction modes
            HL_ERROR_T err = _hl_codec_264_decode_svc_intra_pred_modes_4x4(p_codec, p_mb);
            if (err) {
                return err;
            }
        }
        else if (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb)) {
            // G.8.3.1.2
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
        else { // I_16x16
            p_mb->ext.svc.ipred16x16 = p_mb->Intra16x16PredMode;
        }

        if (p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->ChromaArrayType == 1 || p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->ChromaArrayType == 2) {
            p_mb->ext.svc.ipredChroma = p_mb->intra_chroma_pred_mode;
        }
    }
    return HL_ERROR_SUCCESS;
}

// G.8.3.1.1 SVC derivation process for Intra_4x4 prediction modes
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_modes_4x4(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    int32_t c4x4BlkIdx, mbAddrA, c4x4BlkIdxA, mbAddrB, c4x4BlkIdxB, availableFlagA, availableFlagB, dcPredModePredictedFlag, intraMxMPredModeA, intraMxMPredModeB, predIntra4x4PredMode;
    const hl_codec_264_mb_t *pc_mbA, *pc_mbB;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;

    for (c4x4BlkIdx = 0; c4x4BlkIdx < 16; ++c4x4BlkIdx) {
        // 6.4.10.4 Derivation process for neighbouring 4x4 luma blocks
        mbAddrA = p_mb->neighbouringLumaBlock4x4[c4x4BlkIdx].i_addr_A;
        c4x4BlkIdxA = p_mb->neighbouringLumaBlock4x4[c4x4BlkIdx].i_blk_idx_A;
        mbAddrB = p_mb->neighbouringLumaBlock4x4[c4x4BlkIdx].i_addr_B;
        c4x4BlkIdxB = p_mb->neighbouringLumaBlock4x4[c4x4BlkIdx].i_blk_idx_B;

        availableFlagA = (mbAddrA >= 0) && (pc_mbA = pc_layer->pp_list_macroblocks[mbAddrA]) &&
                         (
                             !pc_slice_header->pc_pps->constrained_intra_pred_flag
                             || (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbA) && pc_slice_header->ext.svc.tcoeff_level_prediction_flag)
                             || (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbA) && !pc_mbA->ext.svc.baseModeFlag)
                             || (HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mbA) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mbA) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mbA))
                         );
        availableFlagB = (mbAddrB >= 0) && (pc_mbB = pc_layer->pp_list_macroblocks[mbAddrB]) &&
                         (
                             !pc_slice_header->pc_pps->constrained_intra_pred_flag
                             || (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbB) && pc_slice_header->ext.svc.tcoeff_level_prediction_flag)
                             || (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbB) && !pc_mbB->ext.svc.baseModeFlag)
                             || (HL_CODEC_264_MB_TYPE_IS_I_16X16(pc_mbB) || HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mbB) || HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mbB))
                         );

        dcPredModePredictedFlag = (!availableFlagA || !availableFlagB);

        if (!dcPredModePredictedFlag) {
            intraMxMPredModeA = HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mbA)
                                ? pc_mbA->ext.svc.ipred4x4[c4x4BlkIdxA]
                                : (HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mbA) ? pc_mbA->ext.svc.ipred8x8[c4x4BlkIdx] >> 2 : 2);
            intraMxMPredModeB = HL_CODEC_264_MB_TYPE_IS_I_4X4(pc_mbB)
                                ? pc_mbB->ext.svc.ipred4x4[c4x4BlkIdxB]
                                : (HL_CODEC_264_MB_TYPE_IS_I_8X8(pc_mbB) ? pc_mbB->ext.svc.ipred8x8[c4x4BlkIdx] >> 2 : 2);
        }
        else {
            intraMxMPredModeA = 2;
            intraMxMPredModeB = 2;
        }

        // (G-83)
        predIntra4x4PredMode = HL_MATH_MIN( intraMxMPredModeA, intraMxMPredModeB );
        if( p_mb->prev_intra4x4_pred_mode_flag[ c4x4BlkIdx ] ) {
            p_mb->ext.svc.ipred4x4[ c4x4BlkIdx ] = predIntra4x4PredMode;
        }
        else if( p_mb->rem_intra4x4_pred_mode[ c4x4BlkIdx ] < predIntra4x4PredMode ) {
            p_mb->ext.svc.ipred4x4[ c4x4BlkIdx ] = p_mb->rem_intra4x4_pred_mode[ c4x4BlkIdx ];
        }
        else {
            p_mb->ext.svc.ipred4x4[ c4x4BlkIdx ] = p_mb->rem_intra4x4_pred_mode[ c4x4BlkIdx ] + 1;
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.3.2 SVC intra sample prediction and construction process
// Outputs: "picSamplesL", "picSamplesCb" and "picSamplesCr"
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb)
{
    HL_ALIGN(HL_ALIGN_V) int32_t mbResL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbResCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbResCr[16][16];
    HL_ERROR_T err;
    hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;

    /*
    	FIXME:
    	In subclauses 8.3.1.2, 8.3.2.2, 8.3.3, and 8.3.4, the variables Intra4x4PredMode, Intra8x8PredMode,
    	Intra16x16PredMode, and intra_chroma_pred_mode are replaced by ipred4x4, ipred8x8, ipred16x16, and
    	ipredChroma, respectively.

    	In subclauses 8.3.1.2, 8.3.2.2, 8.3.3, and 8.3.4, a macroblock with mbAddrN is treated as coded in an Inter
    	macroblock prediction mode when all of the following conditions are false:
    */

    // G.8.5.3.1 Construction process for luma residuals or chroma residuals with ChromaArrayType equal to 3
    err = _hl_codec_264_decode_svc_residual_construct(p_codec, p_mb, 0, p_mb->ext.svc.cTrafo, mbResL);
    if (err) {
        return err;
    }

    // G.8.3.2.1 SVC intra prediction and construction process for luma samples or chroma samples with ChromaArrayType equal to 3
    err = _hl_codec_264_decode_svc_intra_pred_and_construction_for_luma_or_cat3(p_codec, p_mb, pc_sps->BitDepthY, mbResL, pc_layer->pc_fs_curr->p_pict->pc_data_y);
    if (err) {
        return err;
    }

    if (pc_sps->ChromaArrayType) {
        // G.8.5.3.2 Construction process for chroma residuals
        err = _hl_codec_264_decode_svc_residual_construct_chroma(p_codec, p_mb, p_mb->ext.svc.cTrafo, p_mb->ext.svc.sTCoeff, mbResCb, mbResCr);
        if (err) {
            return err;
        }

        // G.8.3.2.2 SVC intra prediction and construction process for chroma samples
        err = _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma(p_codec, p_mb, mbResCb, mbResCr, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->p_pict->pc_data_v);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.3.2.2 SVC intra prediction and construction process for chroma samples
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbResCb[16][16],	HL_ALIGNED(16) const int32_t mbResCr[16][16],
    hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;

    if (pc_sps->ChromaArrayType == 1 || pc_sps->ChromaArrayType == 2) {
        if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)) {
            // G.8.3.2.2.1 SVC construction process for chroma samples of I_PCM macroblocks
            return _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma_pcm(p_codec, p_mb, mbResCb, mbResCr, picSamplesCb, picSamplesCr);
        }
        else {
            // G.8.3.2.2.2 SVC intra prediction and construction process for chroma samples with ChromaArrayType equal to 1 or 2
            return _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma_cat12(p_codec, p_mb, mbResCb, mbResCr, picSamplesCb, picSamplesCr);
        }
    }
    else {
        HL_DEBUG_ERROR("ChromaArrayType=3 not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return err;
}

// G.8.3.2.2.1 SVC construction process for chroma samples of I_PCM macroblocks
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbResCb[16][16],	HL_ALIGNED(16) const int32_t mbResCr[16][16],
    hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
)
{
    HL_DEBUG_ERROR("Not implemented");
    return HL_ERROR_NOT_IMPLEMENTED;
}

// G.8.3.2.2.2 SVC intra prediction and construction process for chroma samples with ChromaArrayType equal to 1 or 2
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_chroma_cat12(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbResCb[16][16],	HL_ALIGNED(16) const int32_t mbResCr[16][16],
    hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
)
{
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    int32_t mbAddrN, xW, yW, p_size, i, x, y, xL, yL, xM, yM, nW, nH;
    HL_ALIGN(HL_ALIGN_V) int32_t predCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t predCr[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbSamplesCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbSamplesCr[16][16];
    HL_ERROR_T err;
    static hl_bool_t IsLumaFalse = HL_FALSE;

    nW = pc_sps->MbWidthC;
    nH = pc_sps->MbHeightC;
    {
        /** 8.3.4 Intra prediction process for chroma samples **/
        //FIXME: use "p_codec->tmp33_0_0" and "p_codec->tmp33_0_1"
        int32_t pCb[33] = { HL_CODEC_264_SAMPLE_NOT_AVAIL }, pCr[33]= { HL_CODEC_264_SAMPLE_NOT_AVAIL }; // x = -1, y = -1..MbHeightC - 1 and with x = 0..MbWidthC - 1, y = -1,
        hl_codec_264_mb_t* pc_mb;

        p_size = (pc_sps->MbHeightC + 1) + (pc_sps->MbWidthC); // FIXME: "+1" missing?
        for (i = 0; i < p_size; ++i) {
            x = INTRA_CHROMA_NEIGHBOURING_SAMPLES_X[i];
            y = INTRA_CHROMA_NEIGHBOURING_SAMPLES_Y[i];
            // 6.4.11 Derivation process for neighbouring locations
            hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, x, y, &mbAddrN, &xW, &yW, IsLumaFalse);
            pc_mb = HL_MATH_IS_POSITIVE_INT32(mbAddrN) ? pc_layer->pp_list_macroblocks[mbAddrN] : HL_NULL;
            // FIXME: IsInter(pc_mb) is modified by caller
            if (!pc_mb || (pc_pps->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(pc_mb) || pc_mb->e_type == HL_CODEC_264_MB_TYPE_SI))) {
                pCb[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
                pCr[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
            }
            else {
                // 6.4.1 Inverse macroblock scanning process
                xL = pc_mb->xL;
                yL = pc_mb->yL;
                xM = ((xL >> 4) * nW);// (8-128)
                yM = ((yL >> 4) * nH) + (yL & 1);// (8-129)

                if (pc_slice_header->MbaffFrameFlag /*FIXME: && filed_macroblock*/) {
                    // (8-130)
                    HL_DEBUG_ERROR("MbAFF not implemented yet");
                    return HL_ERROR_NOT_IMPLEMENTED;
                }
                else {
                    pCb[i] = picSamplesCb[(xM+xW)+((yM+yW)*pc_slice_header->PicWidthInSamplesC)];// (8-131)
                    pCr[i] = picSamplesCr[(xM+xW)+((yM+yW)*pc_slice_header->PicWidthInSamplesC)];
                }
            }
        }

        // Performs IntraChroma prediction
        hl_codec_264_pred_intra_perform_prediction_chroma(p_codec, p_mb, predCb, predCr, pCb, pCr, p_mb->ext.svc.ipredChroma);
    } //end-of-8.3.4

    for (x = 0; x < nW; ++x) {
        for (y = 0; y < nH; ++y) {
            mbSamplesCb[y][x] = HL_MATH_CLIP1C(predCb[y][x] + mbResCb[y][x], pc_sps->BitDepthC); // (G-88)
            mbSamplesCr[y][x] = HL_MATH_CLIP1C(predCr[y][x] + mbResCr[y][x], pc_sps->BitDepthC); // (G-88)
        }
    }

    err = _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picSamplesCb, sizeof(picSamplesCb[0]), mbSamplesCb, nW, nH);
    if (err) {
        return err;
    }
    return _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picSamplesCr, sizeof(picSamplesCr[0]), mbSamplesCr, nW, nH);
}

// G.8.3.2.1 SVC intra prediction and construction process for luma samples or chroma samples with ChromaArrayType equal to 3
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_luma_or_cat3(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbResL[16][16],
    hl_pixel_t* picSamplesL
)
{
    if (HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)) {
        // G.8.3.2.1.1 SVC construction process for luma samples and chroma samples with ChromaArrayType equal to 3 of I_PCM macroblocks
        return _hl_codec_264_decode_svc_intra_pred_and_construction_for_luma_and_cat3_pcm(p_codec, p_mb, mbResL, picSamplesL);
    }
    else if (HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)) {
        // G.8.3.2.1.2 SVC Intra_4x4 sample prediction and construction process
        return _hl_codec_264_decode_svc_intra_pred_and_construction_4x4(p_codec, p_mb,bitDepth, mbResL, picSamplesL);
    }
    else  if (HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb)) {
        // G.8.3.2.1.3 SVC Intra_8x8 sample prediction and construction process
        return _hl_codec_264_decode_svc_intra_pred_and_construction_8x8(p_codec, p_mb,bitDepth, mbResL, picSamplesL);
    }
    else { // I_16x16
        // G.8.3.2.1.4 SVC Intra_16x16 sample prediction and construction process
        return _hl_codec_264_decode_svc_intra_pred_and_construction_16x16(p_codec, p_mb,bitDepth, mbResL, picSamplesL);
    }
}

// G.8.3.2.1.1 SVC construction process for luma samples and chroma samples with ChromaArrayType equal to 3 of I_PCM macroblocks
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_for_luma_and_cat3_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
)
{
    HL_DEBUG_ERROR("Not implemented");
    return HL_ERROR_NOT_IMPLEMENTED;
}

// G.8.3.2.1.2 SVC Intra_4x4 sample prediction and construction process
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_4x4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
)
{
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    HL_ALIGN(HL_ALIGN_V) int32_t mbSamples[16][16] = { 0 }; // must
    int32_t pred4x4[4][4];
    const hl_codec_264_mb_t* mb;
    int32_t c4x4BlkIdx, xO, yO, i, x, y, mbAddrN, xN, yN, xW, yW, xM, yM, xP, yP, p[13], maxv;
    hl_bool_t p_x47_ym1_not_avail; //p[x,-1],with x=4...7
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    static hl_bool_t IsLumaTrue = HL_TRUE;

    maxv = ((1 << bitDepth) - 1);

    for (c4x4BlkIdx = 0; c4x4BlkIdx < 16; ++c4x4BlkIdx) {
        {
            /*** 8.3.1.2 Intra_4x4 sample prediction (outputs: "pred4x4")****/
            p_x47_ym1_not_avail = HL_TRUE;
            // 6.4.3 Inverse 4x4 luma block scanning process
            xO = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][0];
            yO = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][1];
            for (i = 0; i < 13; ++i) {
                x = INTRA_LUMA_NEIGHBOURING_SAMPLES4X4_X[i];
                y = INTRA_LUMA_NEIGHBOURING_SAMPLES4X4_Y[i];
                xN = xO + x; // (8-42)
                yN = yO + y; // (8-43)
                // 6.4.11 Derivation process for neighbouring locations
                hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, xN, yN, &mbAddrN, &xW, &yW, IsLumaTrue);
                mb = HL_MATH_IS_POSITIVE_INT32(mbAddrN) ? pc_layer->pp_list_macroblocks[mbAddrN] : HL_NULL;
                if (!mb || (pc_pps->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(mb) || mb->e_type == HL_CODEC_264_MB_TYPE_SI)) ||
                        (x > 3 && (c4x4BlkIdx == 3 || c4x4BlkIdx == 11))) {
                    p[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
                }
                else {
                    if(p_x47_ym1_not_avail && (y == -1 && 4<=x && x<=7)) {
                        p_x47_ym1_not_avail = HL_FALSE;
                    }
                    // 6.4.1 Inverse macroblock scanning process
                    xM = mb->xL;
                    yM = mb->yL;
                    if (pc_slice_header->MbaffFrameFlag /*FIXME: && filed_macroblock*/) {
                        // (8-44)
                        HL_DEBUG_ERROR("MbAFF not implemented yet");
                        return HL_ERROR_NOT_IMPLEMENTED;
                    }
                    else {
                        p[i] = picSamples[(xM + xW) + ((yM + yW)*pc_slice_header->PicWidthInSamplesL)];//(8-45)
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
            hl_codec_264_pred_intra_perform_prediction_4x4L(p_codec, p_mb, pred4x4, p, p_mb->ext.svc.ipred4x4[c4x4BlkIdx]);
        } // end-of-8.3.1.2

        // 6.4.3 Inverse 4x4 luma block scanning process
        xP = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][0];
        yP = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][1];
        for (x = xP; x < (xP + 4); ++x) {
            for (y = yP; y < (yP + 4); ++y) { // FIXME: unroll
                mbSamples[y][x] = HL_MATH_CLIP3(0, maxv, pred4x4[y - yP][x - xP] + mbRes[y][x]); // (G-85)
            }
        }

        // G.8.5.4.3 Picture sample array construction process for a colour component
        err = _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picSamples, sizeof(picSamples[0]), mbSamples, 16, 16);
        if (err) {
            return err;
        }
    } //end-of-for()

    return err;
}

// G.8.3.2.1.3 SVC Intra_8x8 sample prediction and construction process
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_8x8(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
)
{
    HL_DEBUG_ERROR("Not implemented");
    return HL_ERROR_NOT_IMPLEMENTED;
}

// G.8.3.2.1.4 SVC Intra_16x16 sample prediction and construction process
static HL_ERROR_T _hl_codec_264_decode_svc_intra_pred_and_construction_16x16(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    HL_ALIGNED(16) const int32_t mbRes[16][16],
    hl_pixel_t* picSamples
)
{
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    hl_memory_blocks_t* pc_mem_blocks;
    int32_t p[33], maxv, x, y;
    hl_int32_16x16_t *pred16x16, *mbSamples;
    HL_ERROR_T err;

    pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

    // map() memory
    hl_memory_blocks_map(pc_mem_blocks, &pred16x16);
    hl_memory_blocks_map(pc_mem_blocks, &mbSamples);

    maxv = ((1 << bitDepth) - 1);

    {
        /** 8.3.3 Intra_16x16 prediction process for luma samples **/
        int32_t i ,x ,y, xW, yW, mbAddrN, xM, yM;
        const hl_codec_264_mb_t* pc_mb;

        for (i = 0; i < 33; ++i) {
            x = INTRA_LUMA_NEIGHBOURING_SAMPLES16X16_X[i];
            y = INTRA_LUMA_NEIGHBOURING_SAMPLES16X16_Y[i];
            // 6.4.11 Derivation process for neighbouring locations
            hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, x, y, &mbAddrN, &xW, &yW, HL_TRUE);
            pc_mb = HL_MATH_IS_POSITIVE_INT32(mbAddrN) ? p_codec->layers.pc_active->pp_list_macroblocks[mbAddrN] : HL_NULL;
            // FIXME: IsInter(pc_mb) modified by caller
            if (!pc_mb || (pc_pps->constrained_intra_pred_flag == 1 && (HL_CODEC_264_MB_TYPE_IS_INTER(pc_mb) || pc_mb->e_type == HL_CODEC_264_MB_TYPE_SI))) {
                p[i] = HL_CODEC_264_SAMPLE_NOT_AVAIL;
            }
            else {
                // 6.4.1 Inverse macroblock scanning process
                //hl_codec_264_utils_inverse_macroblock_scanning_process(p_codec, mbAddrN, &xM, &yM);
                xM = pc_mb->xL;
                yM = pc_mb->yL;
                if (pc_slice_header->MbaffFrameFlag /*FIXME: && filed_macroblock*/) {
                    // (8-114)
                    HL_DEBUG_ERROR("MbAFF not implemented yet");
                    return HL_ERROR_NOT_IMPLEMENTED;
                }
                else {
                    p[i] = picSamples[(xM + xW)+(yM + yW)*pc_slice_header->PicWidthInSamplesL]; //(8-115)
                }
            }
        }
        // Performs Intra16x16 prediction
        hl_codec_264_pred_intra_perform_prediction_16x16L(p_codec, p_mb, (*pred16x16), p, p_mb->ext.svc.ipred16x16);
    }// end-of-8.3.3

    for (x = 0; x < 16; ++x) {
        for (y = 0; y < 16; ++y) { // FIXME: unrol
            (*mbSamples)[y][x] = HL_MATH_CLIP3(0, maxv, (*pred16x16)[y][x] + mbRes[y][x]); //(G-87)
        }
    }

    // G.8.5.4.3 Picture sample array construction process for a colour component
    err = _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picSamples, sizeof(picSamples[0]), (*mbSamples), 16, 16);

    // unmap() memory
    hl_memory_blocks_unmap(pc_mem_blocks, pred16x16);
    hl_memory_blocks_unmap(pc_mem_blocks, mbSamples);

    return err;
}

// G.8.4.2 SVC decoding process for Inter prediction samples
static HL_ERROR_T _hl_codec_264_decode_svc_inter(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t targetQId,
    hl_pixel_t* picSamplesL, hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
)
{
    HL_ALIGN(HL_ALIGN_V) int32_t predMbL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t predMbCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t predMbCr[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartCr[16][16];
    int32_t numMbPart, numSubMbPart, subMbPartIdx, mbPartIdx, isDirectFlag, derivePredWeights, xP, yP, xS, yS, partWidth, partHeight, partWidthC, partHeightC;
    int32_t isBDirect16x16OrSkipFlag, isP8x8OrP8x8Ref0OrB8x8, x, y;
    const hl_codec_264_pict_t *refPicLXL, *refPicLXCb, *refPicLXCr;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    // FIXME
    // In subclauses 8.4.3 and 8.4.2.1, any occurrence of RefPicList0 or RefPicList1 is replaced with refPicList0 or
    // refPicList1, respectively, with refPicList0 and refPicList1 being the reference picture lists specified as inputs
    // to this subclause.

    // FIXME:
    // In subclause 8.4.1.4, the reference picture referred by refIdxLX is specified by refPicListX[ refIdxLX ] with
    // refPicList0 and refPicList1 specified as inputs to this subclause.

    // FIXME
    // In subclauses 8.4.2.2.1 and 8.4.2.2.2, any occurrence of mb_field_decoding_flag is replaced by fieldMbFlag.

    if (pc_layer->DQId == 16 && p_mb->u_addr == 119 && pc_layer->i_pict_decode_count == 1) { // FIXME
        int a = 0;
    }

    derivePredWeights = (pc_pps->weighted_pred_flag && (pc_slice_header->SliceTypeModulo5 == 0)) || (pc_pps->weighted_bipred_idc > 0 && (pc_slice_header->SliceTypeModulo5 == 1));
    isBDirect16x16OrSkipFlag = (HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_DIRECT_16X16(p_mb));
    isP8x8OrP8x8Ref0OrB8x8 = (HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb) || HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb) || HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb));

    if (isBDirect16x16OrSkipFlag) {
        numMbPart = pc_layer->DQId == 0 ? 4 : 1;
    }
    else {
        numMbPart = p_mb->NumMbPart;
    }

    for (mbPartIdx = 0; mbPartIdx < numMbPart; ++mbPartIdx) {
        isDirectFlag = isBDirect16x16OrSkipFlag || (HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb) && HL_CODEC_264_SUBMB_TYPE_IS_B_DIRECT_8X8(p_mb, mbPartIdx));
        if (derivePredWeights) {
            // G.8.4.2.1 SVC derivation process for prediction weights
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
        // "6.4.2.1" -> InverseRasterScan_Pow2Full
        xP = isBDirect16x16OrSkipFlag ? (( mbPartIdx & 1 ) << 3) : InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 0);
        yP = isBDirect16x16OrSkipFlag ? (( mbPartIdx >> 1 ) << 3) : InverseRasterScan_Pow2Full(mbPartIdx, p_mb->MbPartWidth, p_mb->MbPartHeight, 16, 1);

        numSubMbPart = (isDirectFlag && pc_layer->DQId == 0) ? 4 : (isDirectFlag ? 1 : p_mb->NumSubMbPart[mbPartIdx]);
        for (subMbPartIdx = 0; subMbPartIdx < numSubMbPart; ++subMbPartIdx) {
            if (isDirectFlag) {
                if (pc_layer->DQId == 0) {
                    partWidth = 4;
                    partHeight = 4;
                }
                else {
                    if (isBDirect16x16OrSkipFlag) {
                        partWidth = 16;
                        partHeight = 16;
                    }
                    else {
                        partWidth = 8;
                        partHeight = 8;
                    }
                }
            }
            else {
                if (isP8x8OrP8x8Ref0OrB8x8) {
                    partWidth = p_mb->SubMbPartWidth[mbPartIdx]; // (G-103)
                    partHeight = p_mb->SubMbPartHeight[mbPartIdx]; // (G-104)
                }
                else {
                    partWidth = p_mb->MbPartWidth; // (G-101)
                    partHeight = p_mb->MbPartHeight; // (G-102)
                }
            }

            if (pc_sps->ChromaArrayType) {
                partWidthC = partWidth / pc_sps->SubWidthC; // (G-105)
                partHeightC = partHeight / pc_sps->SubHeightC; // (G-106): FIXME: standard says "partHeight / SubWidthC" but looks like an error

                if (p_mb->predFlagL0[mbPartIdx]) {
                    // 8.4.1.4 Derivation process for chroma motion vectors
                    err = hl_codec_264_utils_derivation_process_for_chroma_movects(p_codec, p_mb, &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx], HL_CODEC_264_LIST_IDX_0);
                    if (err) {
                        return err;
                    }
                }
                else if (p_mb->predFlagL1[mbPartIdx]) {
                    // 8.4.1.4 Derivation process for chroma motion vectors
                    err = hl_codec_264_utils_derivation_process_for_chroma_movects(p_codec, p_mb, &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx], HL_CODEC_264_LIST_IDX_1);
                    if (err) {
                        return err;
                    }
                }
            }

            if (HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb) && HL_CODEC_264_SUBMB_TYPE_IS_B_DIRECT_8X8(p_mb, mbPartIdx)) {
                xS = ((subMbPartIdx & 1) << 2);
                yS = ((subMbPartIdx >> 1) << 2);
            }
            else {
                // 6.4.2.2 - upper-left sample of the sub-macroblock partition
                hl_codec_264_mb_inverse_sub_partion_scan(p_mb, mbPartIdx, subMbPartIdx, &xS, &yS);
            }

            // Compute "xL_Idx" and "yL_Idx", required by "hl_codec_264_pred_inter_predict()"
            p_mb->xL_Idx = p_mb->xL + (xP + xS);
            p_mb->yL_Idx = p_mb->yL + (yP + yS);

            {
                /* 8.4.2 Decoding process for Inter prediction samples */

                // Get reference pictures for the partition (ignore sub-parts)
                if (subMbPartIdx == 0) {
                    // 8.4.2.1 Reference picture selection process
                    err = hl_codec_264_pred_inter_select_refpic(p_codec, p_mb,
                            p_mb->predFlagL1[mbPartIdx] == 1 ? pc_layer->pobj_poc->RefPicList1 : pc_layer->pobj_poc->RefPicList0,
                            p_mb->predFlagL1[mbPartIdx] == 1 ? p_mb->refIdxL1[mbPartIdx] : p_mb->refIdxL0[mbPartIdx],
                            &refPicLXL, &refPicLXCb, &refPicLXCr);
                    if (err) {
                        return err;
                    }
                }

                if(p_mb->predFlagL0[mbPartIdx] == 1) {
                    // 8.4.2.2 Fractional sample interpolation process
                    err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                          &p_mb->mvL0[mbPartIdx][subMbPartIdx], &p_mb->mvCL0[mbPartIdx][subMbPartIdx],
                                                          refPicLXL, refPicLXCb, refPicLXCr,
                                                          predPartL, predPartCb, predPartCr);
                    if (err) {
                        return err;
                    }
                }
                else if(p_mb->predFlagL1[mbPartIdx] == 1) {
                    // 8.4.2.2 Fractional sample interpolation process
                    err = hl_codec_264_pred_inter_predict(p_codec, p_mb, mbPartIdx, subMbPartIdx,
                                                          &p_mb->mvL1[mbPartIdx][subMbPartIdx], &p_mb->mvCL1[mbPartIdx][subMbPartIdx],
                                                          refPicLXL, refPicLXCb, refPicLXCr,
                                                          predPartL, predPartCb, predPartCr);
                    if (err) {
                        return err;
                    }
                }
            } // end-of-8.4.2

            for (y = 0; y < partHeight; ++y) {
                for (x = 0; x < partWidth; ++x) {
                    predMbL[yP + yS + y][xP + xS + x] = predPartL[y][x]; // (G-107)
                }
            }

            if (pc_sps->ChromaArrayType) {
                for (y = 0; y < partHeightC; ++y) {
                    for (x = 0; x < partWidthC; ++x) {
                        predMbCb[( yP + yS ) / pc_sps->SubHeightC + y][( xP + xS ) / pc_sps->SubWidthC + x] = predPartCb[y][x]; // (G-108)
                        predMbCr[( yP + yS ) / pc_sps->SubHeightC + y][( xP + xS ) / pc_sps->SubWidthC + x] = predPartCr[y][x]; // (G-109)
                    }
                }
            }
        } // end-of-for(subMbPartIdx)
    } // end-of-for (mbPartIdx)

    if (targetQId == 0 && p_mb->ext.svc.base_mode_flag && pc_slice_header->MbaffFrameFlag == 0 && pc_layer->RefLayerMbaffFrameFlag == 0 && pc_layer->RestrictedSpatialResolutionChangeFlag == 0) {
        // G.8.4.2.2
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // G.8.5.4.1 Picture sample array construction process
    err = _hl_codec_264_decode_svc_sample_array_pict_construct(p_codec, p_mb, picSamplesL, picSamplesCb, picSamplesCr, sizeof(picSamplesL[0]), predMbL, predMbCb, predMbCr);
    if (err) {
        return err;
    }

    return err;
}

// G.8.5.1 Transform coefficient scaling and refinement process
static HL_ERROR_T _hl_codec_264_decode_svc_transform_coeff_scaling_and_refinement(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t refinementFlag
)
{
    int32_t qPyPrime, qPCbPrime, qPCrPrime;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    qPyPrime = (p_mb->ext.svc.tQPy + pc_sps->QpBdOffsetY);
    qPCbPrime = (p_mb->ext.svc.tQPCb + pc_sps->QpBdOffsetC);
    qPCrPrime = (p_mb->ext.svc.tQPCr + pc_sps->QpBdOffsetC);

    if (!refinementFlag) {
        hl_memory_set((int32_t*)p_mb->ext.svc.sTCoeff, sizeof(p_mb->ext.svc.sTCoeff)/sizeof(p_mb->ext.svc.sTCoeff[0]), 0);
        hl_memory_set((int32_t*)p_mb->ext.svc.tCoeffLevel, sizeof(p_mb->ext.svc.tCoeffLevel)/sizeof(p_mb->ext.svc.tCoeffLevel[0]), 0);
    }

    // G.8.5.1.1 Refinement process for luma transform coefficients or chroma transform coefficients with ChromaArrayType equal to 3
    err = _hl_codec_264_decode_svc_refinement_process_transform_coeff_luma_or_cat3(p_codec, p_mb, 0, p_mb->ext.svc.cTrafo, p_mb->ext.svc.sTCoeff, p_mb->ext.svc.tCoeffLevel, qPyPrime, qPCbPrime, qPCrPrime);
    if (err) {
        return err;
    }

    if (pc_sps->ChromaArrayType != 0) {
        // G.8.5.1.2 Refinement process for chroma transform coefficients
        err = _hl_codec_264_decode_svc_refinement_process_transform_coeff_chroma(p_codec, p_mb, p_mb->ext.svc.cTrafo, p_mb->ext.svc.sTCoeff, p_mb->ext.svc.tCoeffLevel, qPCbPrime, qPCrPrime);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.5.1.1 Refinement process for luma transform coefficients or chroma transform coefficients with ChromaArrayType equal to 3
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_luma_or_cat3(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iYCbCr,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, int32_t* sTCoeff, int32_t* tCoeffLevel,
    int32_t qPyPrime, int32_t qPCbPrime, int32_t qPCrPrime
)
{
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    int32_t bitDepth, qP, cO;
    hl_int32_16x16_t *coeffLevel, *coeffACLevel;
    hl_int32_8x64_t* coeffLevel8x8;
    hl_int32_16_t* coeffDCLevel;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (iYCbCr == 0) {
        bitDepth = pc_sps->BitDepthY;
        qP = qPyPrime;
        cO = 0;
        coeffLevel = &p_mb->LumaLevel;
        coeffLevel8x8 = &p_mb->LumaLevel8x8;
        coeffDCLevel = &p_mb->Intra16x16DCLevel;
        coeffACLevel = &p_mb->Intra16x16ACLevel;
    }
    else if (iYCbCr == 1) {
        // Function called for "Luma" or "ChromaArrayType==3"
        HL_DEBUG_ERROR("ChromaArrayType is equal to 3 not supported");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_PCM) {
        // G.8.5.1.1.1
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else if(cTrafo == HL_CODEC_264_TRANSFORM_TYPE_4X4) {
        // G.8.5.1.1.2 Refinement process for transform coefficients of residual 4x4 blocks
        err = _hl_codec_264_decode_svc_refinement_process_transform_coeff_residual_4x4(p_codec, p_mb, bitDepth, qP, cO, (*coeffLevel), sTCoeff, tCoeffLevel);
        if (err) {
            return err;
        }
    }
    else if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_8X8) {
        // G.8.5.1.1.3
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else { // 16x16
        // G.8.5.1.1.4 Refinement process for transform coefficients of Intra_16x16 macroblocks
        err = _hl_codec_264_decode_svc_refinement_process_transform_coeff_intra_16x16(p_codec, p_mb, bitDepth, qP, cO,
                (*coeffDCLevel), (*coeffACLevel), (*coeffLevel), sTCoeff, tCoeffLevel);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.5.1.2 Refinement process for chroma transform coefficients
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, int32_t* sTCoeff, int32_t* tCoeffLevel,
    int32_t qPCbPrime, int32_t qPCrPrime
)
{
    static const int32_t iCbCrEqualZero = 0;
    static const int32_t iCbCrEqualOne = 1;
    int32_t ChromaArrayType = p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->ChromaArrayType;
    //!\ For both chroma components indexed by iCbCr = 0..1, the following applies.
    if (ChromaArrayType == 1 || ChromaArrayType == 2) {
        if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_PCM) {
            // G.8.5.1.2.1
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
        else {
            // G.8.5.1.2.2 Refinement process for chroma transform coefficients with ChromaArrayType equal to 1 or 2
            HL_ERROR_T err = _hl_codec_264_decode_svc_refinement_process_transform_coeff_cat12(p_codec, p_mb, iCbCrEqualZero, cTrafo, sTCoeff, tCoeffLevel, qPCbPrime, qPCrPrime);
            if (err) {
                return err;
            }
            err = _hl_codec_264_decode_svc_refinement_process_transform_coeff_cat12(p_codec, p_mb, iCbCrEqualOne, cTrafo, sTCoeff, tCoeffLevel, qPCbPrime, qPCrPrime);
            if (err) {
                return err;
            }
        }
    }
    else {
        // G.8.5.1.1
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.1.1.2 Refinement process for transform coefficients of residual 4x4 blocks
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_residual_4x4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    int32_t qP,
    int32_t cO,
    const int32_t coeffLevel[16][16],
    int32_t* sTCoeff,
    int32_t* tCoeffLevel
)
{
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    int32_t i, j, c4x4BlkIdx;
    HL_ALIGN(HL_ALIGN_V) int32_t c[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
    if (pc_slice_header->ext.svc.tcoeff_level_prediction_flag) {
        hl_memory_set((int32_t*)&p_mb->ext.svc.sTCoeff[cO], 255, 0); // (G-126)
    }
    else {
        hl_memory_set((int32_t*)&p_mb->ext.svc.tCoeffLevel[cO], 255, 0); // (G-127)
    }

    for (c4x4BlkIdx = 0; c4x4BlkIdx < 16; ++c4x4BlkIdx) {
        {
            // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
            for (i = 0; i <16; i+=4) {
                c[ZigZag4x4BlockScanYX[i][0]][ZigZag4x4BlockScanYX[i][1]] = coeffLevel[c4x4BlkIdx][i];
                c[ZigZag4x4BlockScanYX[i + 1][0]][ZigZag4x4BlockScanYX[i + 1][1]] = coeffLevel[c4x4BlkIdx][i + 1];
                c[ZigZag4x4BlockScanYX[i + 2][0]][ZigZag4x4BlockScanYX[i + 2][1]] = coeffLevel[c4x4BlkIdx][i + 2];
                c[ZigZag4x4BlockScanYX[i + 3][0]][ZigZag4x4BlockScanYX[i + 3][1]] = coeffLevel[c4x4BlkIdx][i + 3];
            }
        } // end-of-8.5.6

        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + j ] += c[i][j]; // (G-128)
                c[i][j] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + j ]; // (G-129)
            }
        }

        // 8.5.12.1 Scaling process for residual 4x4 blocks
        hl_codec_264_quant_scale_residual4x4(p_codec, p_mb, bitDepth, qP, c, HL_TRUE/*IsLuma*/, HL_FALSE/*Not Intra_16x16*/, -1, d);

        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + j ] += d[i][j]; //(G-130)
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.1.1.4 Refinement process for transform coefficients of Intra_16x16 macroblocks
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_intra_16x16(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t bitDepth,
    int32_t qP,
    int32_t cO,
    const int32_t coeffDCLevel[16],
    const int32_t coeffACLevel[16][16],
    const int32_t coeffLevel[16][16],
    int32_t* sTCoeff,
    int32_t* tCoeffLevel
)
{
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    int32_t i, j, c4x4BlkIdx, k;
    HL_ALIGN(HL_ALIGN_V) int32_t c[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t e[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t c4x4List[16];

    if (!p_mb->ext.svc.base_mode_flag) {
        // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
        InverseScan4x4(coeffDCLevel, c);
    }
    else {
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) { // FIXME: unrol
                c[i][j] = coeffLevel[ 8 * ( i / 2 ) + 4 * ( j / 2 ) + 2 * ( i % 2 ) + ( j % 2 ) ][ 0 ]; // (G-136)
            }
        }
    }

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) { // FIXME: unrol
            tCoeffLevel[ cO + 128 * ( i / 2 ) + 64 * ( j / 2 ) + 32 * ( i % 2 ) + 16 * ( j % 2 ) ] += c[i][j]; //(G-137)
            c[i][j] = tCoeffLevel[ cO + 128 * ( i / 2 ) + 64 * ( j / 2 ) + 32 * ( i % 2 ) + 16 * ( j % 2 ) ]; // (G-138)
        }
    }

    // 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
    hl_codec_264_transf_scale_luma_dc_coeff_intra16x16(p_codec, p_mb, qP, bitDepth, c, d);

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) { // FIXME: unrol
            sTCoeff[ cO + 128 * ( i / 2 ) + 64 * ( j / 2 ) + 32 * ( i % 2 ) + 16 * ( j % 2 ) ] = d[i][j]; // (G-139)
        }
    }

    for (c4x4BlkIdx = 0; c4x4BlkIdx < 16; ++c4x4BlkIdx) {
        if (!p_mb->ext.svc.base_mode_flag) {
            for (k = 0; k < 16; ++k) { // FIXME: exit k=0 to avoid 16x testing
                c4x4List[ k ] = ( ( k == 0 ) ? 0 : coeffACLevel[ c4x4BlkIdx ][ k - 1 ] ); // (G-140)
            }
        }
        else {
            for (k = 0; k < 16; ++k) { // FIXME: exit k=0 to avoid 16x testing
                c4x4List[ k ] = ( ( k == 0 ) ? 0 : coeffLevel[ c4x4BlkIdx ][ k ] ); // (G-141)
            }
        }
        // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
        InverseScan4x4(c4x4List, e);

        for (i = 0; i < 4; ++i) {
            if (i) { // with i, j = 0..3 and i + j > 0
                tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 0 ] += e[i][0]; // (G-142)
                e[i][0] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 0 ]; // (G-143)
            }

            tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 1 ] += e[i][1]; // (G-142)
            tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 2 ] += e[i][2]; // (G-142)
            tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 3 ] += e[i][3]; // (G-142)

            e[i][1] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 1 ]; // (G-143)
            e[i][2] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 2 ]; // (G-143)
            e[i][3] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 3 ]; // (G-143)
        }

        // 8.5.12.1 Scaling process for residual 4x4 blocks
        hl_codec_264_quant_scale_residual4x4(p_codec, p_mb, bitDepth, qP, e, HL_TRUE/*IsLuma*/, HL_TRUE/*Not Intra_16x16*/, -1, d);

        for (i = 0; i < 4; ++i) {
            if (i) { // with i, j = 0..3 and i + j > 0
                sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 0 ] = d[i][0]; // (G-144)
            }
            sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 1 ] = d[i][1]; // (G-144)
            sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 2 ] = d[i][2]; // (G-144)
            sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 3 ] = d[i][3]; // (G-144)
        }
    }
    return HL_ERROR_SUCCESS;
}


// G.8.5.1.2.2 Refinement process for chroma transform coefficients with ChromaArrayType equal to 1 or 2
static HL_ERROR_T _hl_codec_264_decode_svc_refinement_process_transform_coeff_cat12(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iCbCr,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, int32_t* sTCoeff, int32_t* tCoeffLevel,
    int32_t qPCbPrime, int32_t qPCrPrime
)
{
    int32_t nW, nH, numB, cO, qP, i, j, qPDC, c4x4BlkIdx, k;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_pps_t* pc_pps = pc_slice_header->pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps = pc_pps->pc_sps;
    HL_ALIGN(HL_ALIGN_V) int32_t c[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
    int32_t LevelScale4x4, mbIsInterFlag;
    HL_ALIGN(HL_ALIGN_V) int32_t c4x4List[16];
    HL_ALIGN(HL_ALIGN_V) int32_t e[4][4];

    nW = pc_sps->MbWidthC >> 2; // (G-147)
    nH = pc_sps->MbHeightC >> 2; // (G-148)
    numB = nW * nH; // (G-149)
    cO = 256 + ( iCbCr * pc_sps->MbWidthC * pc_sps->MbHeightC ); // (G-150)
    qP = ( ( iCbCr == 0 ) ? qPCbPrime : qPCrPrime ); // (G-151)

    if (pc_slice_header->ext.svc.tcoeff_level_prediction_flag) {
        hl_memory_set((int32_t*)&sTCoeff[cO], (pc_sps->MbWidthC * pc_sps->MbHeightC), 0); // (G-152)
    }
    else {
        hl_memory_set((int32_t*)&tCoeffLevel[cO], (pc_sps->MbWidthC * pc_sps->MbHeightC), 0); // (G-153)
    }

    if (pc_sps->ChromaArrayType == 1) {
        for (i = 0; i < nH; ++i) {
            for (j = 0; j < nW; ++j) {
                c[i][j] = p_mb->ChromaDCLevel[iCbCr][2 * i + j]; // (G-154)
            }
        }
    }
    else {
        static const int32_t scan422ChromaDC[] = { 0, 2, 1, 5, 3, 6, 4, 7 };
        for (i = 0; i < nH; ++i) {
            for (j = 0; j < nW; ++j) {
                c[i][j] = p_mb->ChromaDCLevel[ iCbCr ][ scan422ChromaDC[ 2 * i + j ] ]; // (G-155)
            }
        }
    }

    for (i = 0; i < nH; ++i) {
        for (j = 0; j < nW; ++j) {
            tCoeffLevel[ cO + 32 * i + 16 * j ] += c[i][j]; //(G-156)
        }
    }

    // FIXME: move to the above loop
    for (i = 0; i < nH; ++i) {
        for (j = 0; j < nW; ++j) {
            c[i][j] = tCoeffLevel[ cO + 32 * i + 16 * j ]; // (G-157)
        }
    }

    qPDC = ( ( pc_sps->ChromaArrayType == 1 ) ? qP : ( qP + 3 ) ); // (G-158)

    mbIsInterFlag = HL_CODEC_264_MB_TYPE_IS_INTER(p_mb) ? 1 : 0; // "HL_CODEC_264_MB_TYPE_IS_INTER()" could be any non-zero value
    LevelScale4x4 = pc_pps->LevelScale4x4[mbIsInterFlag][iCbCr+1][qPDC % 6][0][0] << (qPDC / 6);
    for (i = 0; i < nH; ++i) {
        for (j = 0; j < nW; ++j) {
            d[i][j] = c[i][j] * LevelScale4x4; // (G-159)
        }
    }

    // FIXME: move into previous loop
    for (i = 0; i < nH; ++i) {
        for (j = 0; j < nW; ++j) {
            sTCoeff[ cO + 32 * i + 16 * j ] += d[i][j]; // (G-160)
        }
    }

    for (c4x4BlkIdx = 0; c4x4BlkIdx < numB; ++c4x4BlkIdx) {
        for (k = 0; k < 16; ++k) {
            c4x4List[k] = ( ( k == 0 ) ? 0 : p_mb->ChromaACLevel[ iCbCr ][ c4x4BlkIdx ][ k - 1 ] ); //(G-161)
        }

        // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
        InverseScan4x4(c4x4List, e);

        for (i = 0; i < 4; ++i) {
            if (i) { // skip (i = j = 0)
                tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 0 ] += e[i][0]; // (G-162)
                e[i][0] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 0 ]; // (G-163)
            }
            tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 1 ] += e[i][1]; // (G-162)
            tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 2 ] += e[i][2]; // (G-162)
            tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 3 ] += e[i][3]; // (G-162)

            e[i][1] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 1 ]; // (G-163)
            e[i][2] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 2 ]; // (G-163)
            e[i][3] = tCoeffLevel[ cO + 16 * c4x4BlkIdx + 4 * i + 3 ]; // (G-163)
        }

        // 8.5.12.1 Scaling process for residual 4x4 blocks
        hl_codec_264_quant_scale_residual4x4(p_codec, p_mb, pc_sps->BitDepthC, qP, e, HL_FALSE/*IsLuma*/, HL_FALSE/*Not Intra_16x16*/, iCbCr, d);

        for (i = 0; i < 4; ++i) {
            if (i) { // skip (i = j = 0)
                sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 0 ] += d[i][0]; // (G-164)
            }
            sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 1 ] += d[i][1]; // (G-164)
            sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 2 ] += d[i][2]; // (G-164)
            sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + 3 ] += d[i][3]; // (G-164)
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.2 Transform coefficient level scaling process prior to transform coefficient refinement
static HL_ERROR_T _hl_codec_264_decode_svc_transform_coeff_level_scaling_prior_to_refinement(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t tQPY, int32_t tQPCb, int32_t tQPCr,
    int32_t refQPY, int32_t refQPCb, int32_t refQPCr
)
{
    int32_t iYCbCr, iYCbCrMax, cO, iMax, cQP, refQP, cS, rShift, i, *tCoeffLevel;
    // Table G-6 – Scale values cS for transform coefficient level scaling
    static const int32_t __cS[] = { 8, 9, 10, 11, 13, 14 };

    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    iYCbCrMax = pc_sps->ChromaArrayType ? 2 : 0;
    tCoeffLevel = p_mb->ext.svc.tCoeffLevel;

    for (iYCbCr = 0; iYCbCr <= iYCbCrMax; ++iYCbCr) {
        cO = ( ( iYCbCr == 0 ) ? 0 : ( 256 + ( iYCbCr - 1 ) * pc_sps->MbWidthC * pc_sps->MbHeightC ) ); // (G-165)
        iMax = ( ( iYCbCr == 0 ) ? 255 : ( pc_sps->MbWidthC * pc_sps->MbHeightC - 1 ) ); // (G-166)
        cQP = ( ( iYCbCr == 0 ) ? tQPY : ( iYCbCr == 1 ? tQPCb : tQPCr ) ); // (G-167)
        refQP = ( ( iYCbCr == 0 ) ? refQPY : ( iYCbCr == 1 ? refQPCb : refQPCr ) ); // (G-168)

        cS = __cS[( refQP - cQP + 54 ) % 6];

        rShift = ( refQP - cQP + 54 ) / 6; // (G-169)

        for (i = 0; i <= iMax; ++i) {
            tCoeffLevel[ cO + i ] = ( ( cS * tCoeffLevel[ cO + i ] ) << rShift ) >> 12; // (G-170)
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.3 Residual construction and accumulation process
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construction_and_accumulation(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t accumulationFlag,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo,
    int32_t* picResL, int32_t* picResCb, int32_t* picResCr
)
{
    HL_ERROR_T err;
    HL_ALIGN(HL_ALIGN_V) int32_t mbResL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbResCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbResCr[16][16];
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    // G.8.5.3.1 Construction process for luma residuals or chroma residuals with ChromaArrayType equal to 3
    err = _hl_codec_264_decode_svc_residual_construct(p_codec, p_mb, 0, cTrafo, mbResL);
    if (err) {
        return err;
    }

    // Chroma
    if (pc_sps->ChromaArrayType != 0) {
        // G.8.5.3.2 Construction process for chroma residuals
        err = _hl_codec_264_decode_svc_residual_construct_chroma(p_codec, p_mb, cTrafo, p_mb->ext.svc.sTCoeff, mbResCb, mbResCr);
        if (err) {
            return err;
        }
    }

#if 0 // Moved to the next block
    if (accumulationFlag == 1) {
        HL_ALIGN(HL_ALIGN_V) int32_t refLayerMbResL[16][16];
        HL_ALIGN(HL_ALIGN_V) int32_t refLayerMbResCb[16][16];
        HL_ALIGN(HL_ALIGN_V) int32_t refLayerMbResCr[16][16];
        int32_t _min, _max, s;
        uint32_t x, y;
        // G.8.5.4.2 Macroblock sample array extraction process
        err = _hl_codec_264_decode_svc_sample_array_extract(
                  p_codec,
                  p_mb,
                  picResL, picResCb, picResCr,
                  refLayerMbResL, refLayerMbResCb, refLayerMbResCr);
        if (err) {
            return err;
        }
        // FIXME: SIMD
        _min = -(1 << pc_sps->BitDepthY) + 1; // (G-172)
        _max = (1 << pc_sps->BitDepthY) - 1; // (G-173)
        for (y = 0; y < 16; ++y) {
            for (x = 0; x < 16; ++x) {
                s = (mbResL[y][x] + refLayerMbResL[y][x]);
                mbResL[y][x] = HL_MATH_CLIP3(_min, _max, s); // (G-171)
            }
        }
        if (pc_sps->ChromaArrayType != 0) {
            // FIXME: SIMD
            _min = -(1 << pc_sps->BitDepthC) + 1; // (G-175)
            _max = (1 << pc_sps->BitDepthC) - 1; // (G-176)
            for (y = 0; y < pc_sps->MbHeightC; ++y) {
                for (x = 0; x < pc_sps->MbWidthC; ++x) {
                    s = (mbResCb[y][x] + refLayerMbResCb[y][x]);
                    mbResCb[y][x] = HL_MATH_CLIP3(_min, _max, s); // (G-174)
                    s = (mbResCr[y][x] + refLayerMbResCr[y][x]);
                    mbResCr[y][x] = HL_MATH_CLIP3(_min, _max, s); // (G-174)
                }
            }
        }
    }
#endif

    {
        /* G.8.5.4.1 Picture sample array construction process */
        int32_t xO, yO, xP_L, yP_L, xP_C, yP_C, minL, maxL, minC, maxC;
        uint32_t x, y;
        int32_t i_stride_L, i_stride_C;
        int32_t *picArrayCb, *picArrayCr, *picArrayL; // signed version because residual could be neg.

        // 6.4.1 Inverse macroblock scanning process
        xO = p_mb->xL;
        yO = p_mb->yL;

        xP_L = ( xO >> 4 ) << 4; // (G-199)
        yP_L = ( ( yO >> 4 ) << 4 ) + ( yO & 1 ); // (G-200)
        xP_C = ( xO >> 4 ) * pc_sps->MbWidthC; // (G-199)
        yP_C = ( ( yO >> 4 ) * pc_sps->MbHeightC ) + ( yO & 1 ); // (G-200)

        i_stride_L = pc_slice_header->PicWidthInSamplesL;
        i_stride_C = pc_slice_header->PicWidthInSamplesC;

        minL = -(1 << pc_sps->BitDepthY) + 1; // (G-172)
        maxL = (1 << pc_sps->BitDepthY) - 1; // (G-173)
        minC = -(1 << pc_sps->BitDepthC) + 1; // (G-175)
        maxC = (1 << pc_sps->BitDepthC) - 1; // (G-176)

        if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag && p_mb->ext.svc.fieldMbFlag) {
            // FIXME: SIMD
            //== Chroma==//
            if (pc_sps->ChromaArrayType != 0) {
                if (accumulationFlag) {
                    for (y = 0; y < pc_sps->MbHeightC; ++ y) {
                        picArrayCb = (picResCb + (i_stride_C * (yP_C + (y << 1)))) + xP_C;
                        picArrayCr = (picResCr + (i_stride_C * (yP_C + (y << 1)))) + xP_C;
                        for (x = 0; x < pc_sps->MbWidthC; ++x) {
                            picArrayCb[x] += mbResCb[y][x];
                            picArrayCb[x] = HL_MATH_CLIP3(minC, maxC, picArrayCb[x]); // (G-174), (G-201)
                            picArrayCr[x] += mbResCr[y][x];
                            picArrayCr[x] = HL_MATH_CLIP3(minC, maxC, picArrayCr[x]); // (G-174), (G-201)
                        }
                    }
                }
                else {
                    for (y = 0; y < pc_sps->MbHeightC; ++ y) {
                        picArrayCb = (picResCb + (i_stride_C * (yP_C + (y << 1)))) + xP_C;
                        picArrayCr = (picResCr + (i_stride_C * (yP_C + (y << 1)))) + xP_C;
                        for (x = 0; x < pc_sps->MbWidthC; ++x) {
                            picArrayCb[x] = mbResCb[y][x]; // (G-201)
                            picArrayCr[x] = mbResCr[y][x]; // (G-201)
                        }
                    }
                }
            }
            //== Luma==//
            if (accumulationFlag) {
                for (y = 0; y < 16; ++ y) {
                    picArrayL = (picResL + (i_stride_L * (yP_L + (y << 1)))) + xP_L;
                    for (x = 0; x < 16; ++x) {
                        picArrayL[x] += mbResL[y][x];
                        picArrayL[x] = HL_MATH_CLIP3(minL, maxL, picArrayL[x]); // (G-171), (G-201)
                    }
                }
            }
            else {
                for (y = 0; y < 16; ++ y) {
                    picArrayL = (picResL + (i_stride_L * (yP_L + (y << 1)))) + xP_L;
                    for (x = 0; x < 16; ++x) {
                        picArrayL[x] = mbResL[y][x]; // (G-201)
                    }
                }
            }
        }
        else {
            // FIXME: SIMD
            //== Chroma==//
            if (pc_sps->ChromaArrayType != 0) {
                picArrayCb = (picResCb + (i_stride_C * yP_C)) + xP_C;
                picArrayCr = (picResCr + (i_stride_C * yP_C)) + xP_C;
                if (accumulationFlag) {
                    for (y = 0; y < pc_sps->MbHeightC; ++ y) {
                        for (x = 0; x < pc_sps->MbWidthC; ++x) {
                            picArrayCb[x] += mbResCb[y][x];
                            picArrayCb[x] = HL_MATH_CLIP3(minC, maxC, picArrayCb[x]); // (G-174), (G-201)
                            picArrayCr[x] += mbResCr[y][x];
                            picArrayCr[x] = HL_MATH_CLIP3(minC, maxC, picArrayCr[x]); // (G-174), (G-201)
                        }
                        picArrayCb += i_stride_C;
                        picArrayCr += i_stride_C;
                    }
                }
                else {
                    for (y = 0; y < pc_sps->MbHeightC; ++ y) {
                        for (x = 0; x < pc_sps->MbWidthC; ++x) {
                            picArrayCb[x] = mbResCb[y][x]; // (G-201)
                            picArrayCr[x] = mbResCr[y][x]; // (G-201)
                        }
                        picArrayCb += i_stride_C;
                        picArrayCr += i_stride_C;
                    }
                }
            }
            //== Luma==//
            if (accumulationFlag) {
                picArrayL = (picResL + (i_stride_L * yP_L)) + xP_L;
                for (y = 0; y < 16; ++ y) {
                    for (x = 0; x < 16; ++x) {
                        picArrayL[x] += mbResL[y][x];
                        picArrayL[x] = HL_MATH_CLIP3(minL, maxL, picArrayL[x]); // (G-171), (G-201)
                    }
                    picArrayL += i_stride_L;
                }
            }
            else {
                picArrayL = (picResL + (i_stride_L * yP_L)) + xP_L;
                for (y = 0; y < 16; ++ y) {
                    for (x = 0; x < 16; ++x) {
                        picArrayL[x] = mbResL[y][x]; // (G-201)
                    }
                    picArrayL += i_stride_L;
                }
            }
        }
    }

    return err;
}

// G.8.5.3.1 Construction process for luma residuals or chroma residuals with ChromaArrayType equal to 3
// "iYCbCr" == 0 (luma), 1(Cb), 2(Cr)
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t iYCbCr, HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, HL_ALIGNED(16) int32_t mbRes[16][16])
{
    int32_t bitDepth, cO;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    bitDepth = (iYCbCr == 0) ? pc_sps->BitDepthY : pc_sps->BitDepthC;
    cO = (iYCbCr == 0) ? 0 : (iYCbCr == 1 ? 256 : (256 + pc_sps->MbWidthC * pc_sps->MbHeightC));

    if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_PCM) {
        HL_DEBUG_ERROR("G.8.5.3.1.1 not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_4X4) {
        // G.8.5.3.1.2 Construction process for residual 4x4 blocks
        return _hl_codec_264_decode_svc_residual_construct4x4(p_codec, p_mb, cO, bitDepth, mbRes);
    }
    else if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_8X8) {
        HL_DEBUG_ERROR("G.8.5.3.1.3 not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else { /* HL_CODEC_264_TRANSFORM_TYPE_16X16 */
        // G.8.5.3.1.4 Construction process for residuals of Intra_16x16 macroblocks
        return _hl_codec_264_decode_svc_residual_construct_intra_16x16(p_codec, p_mb, cO, bitDepth, mbRes);
    }

    return err;
}

// G.8.5.3.1.2 Construction process for residual 4x4 blocks
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct4x4(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t cO, int32_t bitDepth,
    HL_OUT int32_t mbRes[16][16])
{
    int32_t c4x4BlkIdx, i, j, xP, yP;
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t r[4][4];
    // dij = sTCoeff[ cO + 16 * c4x4BlkIdx + 4 * i + j ] with i, j = 0..3 (G-178)
    for (c4x4BlkIdx = 0; c4x4BlkIdx < 16; ++c4x4BlkIdx) {
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) { // FIXME: unrol
                d[i][j] = p_mb->ext.svc.sTCoeff[cO + 16 * c4x4BlkIdx + 4 * i + j]; // (G-178)
            }
        }
        // 8.5.12.2 Transformation process for residual 4x4 blocks
        hl_codec_264_transf_inverse_residual4x4(bitDepth, d, r);
        // 6.4.3 Inverse 4x4 luma block scanning process
        xP = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][0];
        yP = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][1];

        for (i = 0; i < 4; ++i) {
            mbRes[yP + i][xP] = r[i][0]; // (G-179)
            mbRes[yP + i][xP + 1] = r[i][1]; // (G-179)
            mbRes[yP + i][xP + 2] = r[i][2]; // (G-179)
            mbRes[yP + i][xP + 3] = r[i][3]; // (G-179)
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.3.1.4 Construction process for residuals of Intra_16x16 macroblocks
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_intra_16x16(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t cO, int32_t bitDepth,
    HL_OUT int32_t mbRes[16][16])
{
    int32_t c4x4BlkIdx, i, j, xP, yP;
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t r[4][4];

    for (c4x4BlkIdx = 0; c4x4BlkIdx < 16; ++c4x4BlkIdx) {
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) { // FIXME: unrol
                d[i][j] = p_mb->ext.svc.sTCoeff[cO + 16 * c4x4BlkIdx + 4 * i + j]; // (G-182)
            }
        }
        // 8.5.12.2 Transformation process for residual 4x4 blocks
        hl_codec_264_transf_inverse_residual4x4(bitDepth, d, r);
        // 6.4.3 Inverse 4x4 luma block scanning process
        xP = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][0];
        yP = Inverse4x4LumaBlockScanOrderXY[c4x4BlkIdx][1];

        for (i = 0; i < 4; ++i) {
            mbRes[yP + i][xP] = r[i][0]; // (G-183)
            mbRes[yP + i][xP + 1] = r[i][1]; // (G-183)
            mbRes[yP + i][xP + 2] = r[i][2]; // (G-183)
            mbRes[yP + i][xP + 3] = r[i][3]; // (G-183)
        }
    }

    return HL_ERROR_SUCCESS;
}


// G.8.5.3.2 Construction process for chroma residuals
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_CODEC_264_TRANSFORM_TYPE_T cTrafo, const int32_t* sTCoeff,
    HL_ALIGNED(16) int32_t mbResCb[16][16], HL_ALIGNED(16) int32_t mbResCr[16][16]
)
{
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (pc_sps->ChromaArrayType == 1 || pc_sps->ChromaArrayType == 2) {
        if (cTrafo == HL_CODEC_264_TRANSFORM_TYPE_PCM) {
            // G.8.5.3.2.1 Construction process for chroma residuals of I_PCM macroblocks
            err = _hl_codec_264_decode_svc_residual_construct_chroma_pcm(p_codec, p_mb, 0, sTCoeff, mbResCb);
            if (err) {
                return err;
            }
            err = _hl_codec_264_decode_svc_residual_construct_chroma_pcm(p_codec, p_mb, 1, sTCoeff, mbResCr);
            if (err) {
                return err;
            }
        }
        else {
            // G.8.5.3.2.2 Construction process for chroma residuals with ChromaArrayType equal to 1 or 2
            err = _hl_codec_264_decode_svc_residual_construct_chroma_not_pcm(p_codec, p_mb, 0, sTCoeff, mbResCb);
            if (err) {
                return err;
            }
            err = _hl_codec_264_decode_svc_residual_construct_chroma_not_pcm(p_codec, p_mb, 1, sTCoeff, mbResCr);
            if (err) {
                return err;
            }
        }
    }
    else {
        // G.8.5.3.1 Construction process for luma residuals or chroma residuals with ChromaArrayType equal to 3
        err = _hl_codec_264_decode_svc_residual_construct(p_codec, p_mb, 1, cTrafo, mbResCb);
        if (err) {
            return err;
        }
        err = _hl_codec_264_decode_svc_residual_construct(p_codec, p_mb, 2, cTrafo, mbResCr);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.5.3.2.1 Construction process for chroma residuals of I_PCM macroblocks
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_chroma_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iCbCr, const int32_t* sTCoeff,
    HL_ALIGNED(16) int32_t mbRes[16][16]
)
{
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps;
    uint32_t x, y;
    for (y = 0; y < pc_sps->MbHeightC; ++y) {
        for (x = 0; x < pc_sps->MbWidthC; ++y) {
            mbRes[y][x] = sTCoeff[256 + iCbCr * pc_sps->MbWidthC * pc_sps->MbHeightC + y * pc_sps->MbWidthC + x]; // (G-184)
        }
    }
    return HL_ERROR_SUCCESS;
}

// G.8.5.3.2.2 Construction process for chroma residuals with ChromaArrayType equal to 1 or 2
static HL_ERROR_T _hl_codec_264_decode_svc_residual_construct_chroma_not_pcm(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t iCbCr, const int32_t* sTCoeff,
    HL_ALIGNED(16) int32_t mbRes[16][16]
)
{
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps;
    int32_t nW = pc_sps->MbWidthC >> 2; // (G-185)
    int32_t nH = pc_sps->MbHeightC >> 2; // (G-186)
    int32_t numB = nW * nH; // (G-187)
    int32_t cO = 256 + ( iCbCr * pc_sps->MbWidthC * pc_sps->MbHeightC ); // (G-188)
    HL_ALIGN(HL_ALIGN_V) int32_t c[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t f[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t dcC[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t r[4][4];
    int32_t i, j, c4x4BlkIdx, xP, yP;

    for (i = 0; i < nH; ++i) {
        for (j = 0; j < nW; ++j) { // FIXME: unroll
            c[i][j] = sTCoeff[cO + 32 * i + 16 * j]; // (G-189)
        }
    }

    {
        // 8.5.11.1 Transformation process for chroma DC transform coefficients
        // FIXME: also called in "hl_codec_264_transf_scale_chroma_dc_coeff" ->create seperate fuction (ASM, CPP, INTRIN)
        static const int32_t f2x2[2][2] = {{1,1}, {1,-1}};
        static const int32_t f4x4[4][4] = {{1,1,1,1}, {1,1,-1,-1}, {1,-1,-1,1}, {1,-1,1-1}};
        if (pc_sps->ChromaArrayType == 1) {
            int32_t tmp[2][2];
            // (8-328)
            // tmp = (f2x2) MUL (c)
            hl_math_mul2x2(f2x2, c, tmp);
            // f = (tmp) MUL (f2x2)
            hl_math_mul2x2(tmp, f2x2, f);
        }
        else { //ChromaArrayType==2
            // FIXME: (8-329) very good for SIMD
            int32_t tmp[4][4]= {0};
            //FIXME:hl_math_mul4x4x4x2_C
            for (i=0; i<4; ++i) {
                for (j=0; j<2; ++j) {
                    tmp[i][j]+= f4x4[i][0]*c[0][j];
                    tmp[i][j]+= f4x4[i][1]*c[1][j];
                    tmp[i][j]+= f4x4[i][2]*c[2][j];
                    tmp[i][j]+= f4x4[i][3]*c[3][j];
                }
            }
            //FIXME:hlMath_MUL4x2x2x2_C
            for (i=0; i<4; ++i) {
                f[i][0]+= tmp[i][0]*f2x2[0][0];
                f[i][0]+= tmp[i][1]*f2x2[1][0];
                f[i][1]+= tmp[i][0]*f2x2[0][1];
                f[i][1]+= tmp[i][1]*f2x2[1][1];
            }
        }
    }

    if (pc_sps->ChromaArrayType == 1) {
        for (i = 0; i < nH; ++i) {
            for (j = 0; j < nW; ++j) {// FIXME: unrol
                dcC[i][j] = f[i][j] >> 5; // (G-190)
            }
        }
    }
    else {
        for (i = 0; i < nH; ++i) {
            for (j = 0; j < nW; ++j) {// FIXME: unrol
                dcC[i][j] = ( f[i][j] + ( 1 << 5 ) ) >> 6; // (G-191)
            }
        }
    }

    for (c4x4BlkIdx = 0; c4x4BlkIdx < numB; ++c4x4BlkIdx) {
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) { // FIXME: unrol
                d[i][j] = sTCoeff[cO + 16 * c4x4BlkIdx + 4 * i + j]; // (G-193)
            }
        }
        d[0][0] = dcC[c4x4BlkIdx >> 1][c4x4BlkIdx & 1]; // (G-192)
        // 8.5.12.2 Transformation process for residual 4x4 blocks
        hl_codec_264_transf_inverse_residual4x4(pc_sps->BitDepthC, d, r);

        xP = ( c4x4BlkIdx & 1 ) << 2; // (G-194)
        yP = ( c4x4BlkIdx >> 1 ) << 2; // (G-195)

        for (i = 0; i < 4; ++i) {
            mbRes[yP + i][xP + 0] = r[i][0]; // (G-196)
            mbRes[yP + i][xP + 1] = r[i][1]; // (G-196)
            mbRes[yP + i][xP + 2] = r[i][2]; // (G-196)
            mbRes[yP + i][xP + 3] = r[i][3]; // (G-196)
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.4 Sample array accumulation process
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_accumulation(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    const int32_t* picResL, const int32_t* picResCb, const int32_t* picResCr,
    hl_pixel_t* picSamplesL, hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
)
{
#if 1
    const hl_codec_264_nal_slice_header_t* pc_slice_header = p_codec->layers.pc_active->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    uint32_t xO, yO, xP_L, xP_C, yP_L, yP_C, x, y;
    const int32_t *_picResL, *_picResCb, *_picResCr;
    hl_pixel_t *_picSamplesL, *_picSamplesCb, *_picSamplesCr;

    // 6.4.1 Inverse macroblock scanning process
    xO = p_mb->xL;
    yO = p_mb->yL;

    xP_L = ( xO >> 4 ) << 4; // (G-203)
    xP_C = ( xO >> 4 ) * pc_sps->MbWidthC; // (G-203)
    yP_L = ( ( yO >> 4 ) << 4 ) + ( yO & 1 ); // (G-204)
    yP_C = ( ( yO >> 4 ) * pc_sps->MbHeightC ) + ( yO & 1 ); // (G-204)

    if (pc_slice_header->MbaffFrameFlag && p_mb->ext.svc.fieldMbFlag) {
        // Use code in "_hl_codec_264_decode_svc_sample_array_extract()"
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        // FIXME: SIMD
        // == CHROMA == //
        _picResCb = (picResCb + (pc_slice_header->PicWidthInSamplesC * yP_C)) + xP_C;
        _picResCr = (picResCr + (pc_slice_header->PicWidthInSamplesC * yP_C)) + xP_C;
        _picSamplesCb = (picSamplesCb + (pc_slice_header->PicWidthInSamplesC * yP_C)) + xP_C;
        _picSamplesCr = (picSamplesCr + (pc_slice_header->PicWidthInSamplesC * yP_C)) + xP_C;
        for (y = 0; y < pc_sps->MbHeightC; ++ y) {
            for (x = 0; x < pc_sps->MbWidthC; ++x) {
                _picSamplesCb[x] = HL_MATH_CLIP1C( _picSamplesCb[x] + _picResCb[x], pc_sps->BitDepthC ); // (G-198)
                _picSamplesCr[x] = HL_MATH_CLIP1C( _picSamplesCr[x] + _picResCr[x], pc_sps->BitDepthC ); // (G-198)
            }
            _picResCb += pc_slice_header->PicWidthInSamplesC;
            _picResCr += pc_slice_header->PicWidthInSamplesC;
            _picSamplesCb += pc_slice_header->PicWidthInSamplesC;
            _picSamplesCr += pc_slice_header->PicWidthInSamplesC;
        }
        // == LUMA == //
        _picResL = (picResL + (pc_slice_header->PicWidthInSamplesL * yP_L)) + xP_L;
        _picSamplesL = (picSamplesL + (pc_slice_header->PicWidthInSamplesL * yP_L)) + xP_L;
        for (y = 0; y < 16; ++ y) {
            for (x = 0; x < 16; ++x) {
                _picSamplesL[x] = HL_MATH_CLIP1Y( _picSamplesL[x] + _picResL[x], pc_sps->BitDepthY ); // (G-197)
            }
            _picResL += pc_slice_header->PicWidthInSamplesL;
            _picSamplesL += pc_slice_header->PicWidthInSamplesL;
        }
    }
    return HL_ERROR_SUCCESS;

#else
    HL_ALIGN(HL_ALIGN_V) int32_t mbResL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbResCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbResCr[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredCr[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbSamplesL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbSamplesCr[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbSamplesCb[16][16];
    uint32_t x, y;
    HL_ERROR_T err;
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps;

    // G.8.5.4.2 Macroblock sample array extraction process
    err = _hl_codec_264_decode_svc_sample_array_extract(p_codec, p_mb, picResL, picResCb, picResCr, mbResL, mbResCb, mbResCr);
    if (err) {
        return err;
    }

    // G.8.5.4.2 Macroblock sample array extraction process
    err = _hl_codec_264_decode_svc_sample_array_extract(p_codec, p_mb, picSamplesL, picSamplesCb, picSamplesCr, mbPredL, mbPredCb, mbPredCr);
    if (err) {
        return err;
    }

    // FIXME: SIMD
    for (y = 0; y < 16; ++y) {
        for(x = 0; x < 16; ++x) {
            mbSamplesL[y][x] = HL_MATH_CLIP1Y( mbPredL[y][x] + mbResL[y][x], pc_sps->BitDepthY ); //(G-197)
        }
    }

    // FIXME: SIMD
    if (pc_sps->ChromaArrayType != 0) {
        for (y = 0; y < pc_sps->MbHeightC; ++y) {
            for(x = 0; x < pc_sps->MbWidthC; ++x) {
                mbSamplesCb[y][x] = HL_MATH_CLIP1C( mbPredCb[y][x] + mbResCb[y][x], pc_sps->BitDepthC ); // (G-198)
                mbSamplesCr[y][x] = HL_MATH_CLIP1C( mbPredCr[y][x] + mbResCr[y][x], pc_sps->BitDepthC ); // (G-198)
            }
        }
    }

    // G.8.5.4.1 Picture sample array construction process
    err = _hl_codec_264_decode_svc_sample_array_pict_construct(p_codec, p_mb, picSamplesL, picSamplesCb, picSamplesCr, mbSamplesL, mbSamplesCb, mbSamplesCr);
    if (err) {
        return err;
    }

    return err;
#endif
}

// G.8.5.4.1 Picture sample array construction process
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_pict_construct(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    void* picArrayL, void* picArrayCb, void* picArrayCr, hl_size_t picSampleSize,
    HL_ALIGNED(16) const int32_t mbArrayL[16][16], HL_ALIGNED(16) const int32_t mbArrayCb[16][16], HL_ALIGNED(16) const int32_t mbArrayCr[16][16]
)
{
    HL_ERROR_T err;
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps;

    // G.8.5.4.3 Picture sample array construction process for a colour component
    err = _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picArrayL, picSampleSize, mbArrayL, 16, 16);
    if (err) {
        return err;
    }
    if (pc_sps->ChromaArrayType != 0) {
        err = _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picArrayCb, picSampleSize, mbArrayCb, pc_sps->MbWidthC, pc_sps->MbHeightC);
        if (err) {
            return err;
        }
        err = _hl_codec_264_decode_svc_sample_array_pict_construct_comps(p_codec, p_mb, picArrayCr, picSampleSize, mbArrayCr, pc_sps->MbWidthC, pc_sps->MbHeightC);
        if (err) {
            return err;
        }
    }
    return err;
}

// G.8.5.4.2 Macroblock sample array extraction process
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_extract(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_pixel_t* picArrayL, const hl_pixel_t* picArrayCb, const hl_pixel_t* picArrayCr,
    HL_ALIGNED(16) int32_t mbArrayL[16][16], HL_ALIGNED(16) int32_t mbArrayCb[16][16], HL_ALIGNED(16) int32_t mbArrayCr[16][16]
)
{
    const hl_codec_264_nal_slice_header_t* pc_slice_header = p_codec->layers.pc_active->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    HL_ERROR_T err;

    // G.8.5.4.4 Macroblock sample array extraction process for a colour component
    err = _hl_codec_264_decode_svc_sample_array_extract_colour_comp(p_codec, p_mb, picArrayL, mbArrayL, 16, 16);
    if (err) {
        return err;
    }
    if (pc_sps->ChromaArrayType != 0) {
        err = _hl_codec_264_decode_svc_sample_array_extract_colour_comp(p_codec, p_mb, picArrayCb, mbArrayCb, pc_sps->MbWidthC, pc_sps->MbHeightC);
        if (err) {
            return err;
        }
        err = _hl_codec_264_decode_svc_sample_array_extract_colour_comp(p_codec, p_mb, picArrayCr, mbArrayCr, pc_sps->MbWidthC, pc_sps->MbHeightC);
        if (err) {
            return err;
        }
    }

    return err;
}

// G.8.5.4.3 Picture sample array construction process for a colour component
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_pict_construct_comps(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    void* picArray, hl_size_t picSampleSize,
    HL_ALIGNED(16) const int32_t mbArray[16][16], int32_t mbW, int32_t mbH
)
{
    int32_t xO, yO, xP, yP, x, y;
    hl_size_t u_stride, u_stride_mul4;

    if (picSampleSize != 1 && picSampleSize != 4) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // 6.4.1 Inverse macroblock scanning process
    //hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_codec->CurrMbAddr, &xP, &yP);
    xO = p_mb->xL;
    yO = p_mb->yL;

    xP = ( xO >> 4 ) * mbW; // (G-199)
    yP = ( ( yO >> 4 ) * mbH ) + ( yO & 1 ); // (G-200)
    u_stride = (mbW * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs);
    u_stride_mul4 = u_stride << 2;

    if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag && p_mb->ext.svc.fieldMbFlag) {
#if 1
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
#else
        // TODO: SIMD
        hl_pixel_t* picArray_start;
        for (y = 0; y < mbH; ++ y) {
            picArray_start = (picArray + (i_stride * (yP + (y << 1)))) + xP;
            for (x = 0; x < mbW; ++x) {
                picArray_start[x] = mbArray[y][x]; // (G-201)
            }
        }
#endif
    }
    else {
        if (picSampleSize == 1) {
            uint8_t* picArray_start = ((uint8_t*)picArray) + ((u_stride * yP) + xP);
            for (y = 0; y < mbH; y+=4) {
                for (x = 0; x < mbW; x+=4) {
                    hl_memory_copy4x4_u32_to_u8(&picArray_start[x], u_stride, &mbArray[y][x], 16); // (G-201)
                }
                picArray_start += u_stride_mul4;
            }
        }
        else { // picSampleSize == 4
            int32_t* picArray_start = ((int32_t*)picArray) + ((u_stride * yP) + xP);
            for (y = 0; y < mbH; y+=4) {
                for (x = 0; x < mbW; x+=4) {
                    hl_memory_copy4x4(&picArray_start[x], u_stride, &mbArray[y][x], 16); // (G-201)
                }
                picArray_start += u_stride_mul4;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


// G.8.5.4.4 Macroblock sample array extraction process for a colour component
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_extract_colour_comp(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    const hl_pixel_t* picArray,
    HL_ALIGNED(16) int32_t mbArray[16][16], int32_t mbW, int32_t mbH
)
{
    int32_t xO, yO, xP, yP, x, y;
    int32_t i_stride;
    // 6.4.1 Inverse macroblock scanning process
    //hl_codec_264_utils_inverse_macroblock_scanning_process(pc_codec, pc_codec->CurrMbAddr, &xP, &yP);
    xO = p_mb->xL;
    yO = p_mb->yL;

    xP = ( xO >> 4 ) * mbW; // (G-203)
    yP = ( ( yO >> 4 ) * mbH ) + ( yO & 1 ); // (G-204)
    i_stride = (mbW * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs);

    if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag && p_mb->ext.svc.fieldMbFlag) {
        // FIXME: SIMD
        const hl_pixel_t* picArray_start;
        for (y = 0; y < mbH; ++ y) {
            picArray_start = (picArray + (i_stride * (yP + (y << 1)))) + xP;
            for (x = 0; x < mbW; ++x) {
                mbArray[y][x] = picArray_start[x]; // (G-205)
            }
        }
    }
    else {
        // FIXME: SIMD
        const hl_pixel_t* picArray_start = (picArray + (i_stride * yP)) + xP;
        for (y = 0; y < mbH; ++ y) {
            for (x = 0; x < mbW; ++x) {
                mbArray[y][x] = picArray_start[x]; // (G-206)
            }
            picArray_start += i_stride;
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.5.5 Sample array re-initialisation process
static HL_ERROR_T _hl_codec_264_decode_svc_sample_array_reinit(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    void* picSamplesL, void* picSamplesCb, void* picSamplesCr, hl_size_t picSampleSize
)
{
    HL_ALIGN(HL_ALIGN_V) static int32_t mbSamplesL[16][16] = {0}; // must
    HL_ALIGN(HL_ALIGN_V) static int32_t mbSamplesCb[16][16] = {0}; // must
    HL_ALIGN(HL_ALIGN_V) static int32_t mbSamplesCr[16][16] = {0}; // must

    // G.8.5.4.1 Picture sample array construction process
    return _hl_codec_264_decode_svc_sample_array_pict_construct(p_codec, p_mb, picSamplesL, picSamplesCb, picSamplesCr, picSampleSize, mbSamplesL, mbSamplesCb, mbSamplesCr);
}

// G.8.6.2 Resampling process for intra samples
static HL_ERROR_T _hl_codec_264_decode_svc_resample_intra(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    hl_pixel_t* picSamplesL, hl_pixel_t* picSamplesCb, hl_pixel_t* picSamplesCr
)
{
    // FIXME: map() pred arrays
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredCr[16][16];
    HL_ERROR_T err;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    static const int32_t chromaFlagZero = 0;
    static const int32_t chromaFlagOne = 1;
    static const int32_t iCbCrMinus1 = -1;
    static const int32_t iCbCrZero = 0;
    static const int32_t iCbCrOne = 1;

    // G.8.6.2.1 Resampling process for intra samples of a macroblock colour component
    err = _hl_codec_264_decode_svc_resample_intra_colour_comps(p_codec, p_mb, chromaFlagZero, iCbCrMinus1, 16, 16, mbPredL);
    if (err) {
        return err;
    }
    if (pc_sps->ChromaArrayType != 0) {
        // G.8.6.2.1 Resampling process for intra samples of a macroblock colour component
        err = _hl_codec_264_decode_svc_resample_intra_colour_comps(p_codec, p_mb, chromaFlagOne, iCbCrZero, pc_sps->MbWidthC, pc_sps->MbHeightC, mbPredCb);
        if (err) {
            return err;
        }
        err = _hl_codec_264_decode_svc_resample_intra_colour_comps(p_codec, p_mb, chromaFlagOne, iCbCrOne, pc_sps->MbWidthC, pc_sps->MbHeightC, mbPredCr);
        if (err) {
            return err;
        }
    }

    // G.8.5.4.1 Picture sample array construction process
    err = _hl_codec_264_decode_svc_sample_array_pict_construct(p_codec, p_mb, picSamplesL, picSamplesCb, picSamplesCr, sizeof(picSamplesL[0]), mbPredL, mbPredCb, mbPredCr);
    if (err) {
        return err;
    }

    return err;
}

// G.8.6.2.1 Resampling process for intra samples of a macroblock colour component
/*static*/ HL_ERROR_T _hl_codec_264_decode_svc_resample_intra_colour_comps(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t chromaFlag, int32_t iCbCr,
        int32_t mbW, int32_t mbH, HL_OUT int32_t mbPred[16][16]
                                                                          )
{
    int32_t botFieldFlag, frameBasedResamplingFlag, topAndBotResamplingFlag, botFieldFrameMbsOnlyRefFlag, filteringModeFlag;
    //int32_t predArray[16][16];
    hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;

    if (pc_layer->RefLayerFrameMbsOnlyFlag) {
        botFieldFlag = 0;
    }
    else if (pc_slice_header->field_pic_flag) {
        botFieldFlag = pc_slice_header->bottom_field_flag;
    }
    else if (pc_layer->RefLayerFieldPicFlag) {
        botFieldFlag = pc_layer->RefLayerBottomFieldFlag;
    }
    else if (p_mb->ext.svc.fieldMbFlag) {
        botFieldFlag = p_mb->u_addr & 1;
    }
    else {
        botFieldFlag = 0;
    }

    frameBasedResamplingFlag = pc_layer->RefLayerFrameMbsOnlyFlag && pc_sps->frame_mbs_only_flag;
    topAndBotResamplingFlag = (!pc_layer->RefLayerFrameMbsOnlyFlag && !pc_layer->RefLayerFieldPicFlag && !pc_sps->frame_mbs_only_flag && !p_mb->ext.svc.fieldMbFlag);
    botFieldFrameMbsOnlyRefFlag = (pc_layer->RefLayerFrameMbsOnlyFlag && p_mb->ext.svc.fieldMbFlag) &&
                                  ((pc_slice_header->field_pic_flag && pc_slice_header->bottom_field_flag) || (!pc_slice_header->field_pic_flag && (p_mb->u_addr & 1)));
    filteringModeFlag = (chromaFlag && pc_sps->ChromaArrayType != 3);

    if (botFieldFrameMbsOnlyRefFlag) {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else if (frameBasedResamplingFlag || p_mb->ext.svc.fieldMbFlag) {
        int32_t refArrayW, refArrayH, xOffset, yOffset, *refSampleArray = pc_layer->refSampleArray_ListPtr[p_mb->u_slice_idx];
        uint8_t* refSampleArrayAvailability = pc_layer->refSampleArrayAvailability_ListPtr[p_mb->u_slice_idx];
        static const int32_t fldPrdInFrmMbFlag = 0;
        static const int32_t yBorder = 0; // because of this "mbPred" is allowed to be 16x16 (max would be 16x20 -> yBorder = 2)
        static const int32_t predArrayStride = 16; // "mbPred" stride
        HL_ERROR_T err;

        // Allocate temp memory if not already done
        if (!refSampleArray || !refSampleArrayAvailability) {
            err = hl_codec_264_layer_alloc_refsamplearray_svc(pc_layer, p_mb->u_slice_idx);
            if (err) {
                return err;
            }
            refSampleArray = pc_layer->refSampleArray_ListPtr[p_mb->u_slice_idx];
            refSampleArrayAvailability = pc_layer->refSampleArrayAvailability_ListPtr[p_mb->u_slice_idx];
        }

        // G.8.6.2.2 Reference layer sample array construction process prior to intra resampling
        err = _hl_codec_264_decode_svc_ref_layer_array_construct_prior_to_intra_resampling(
                  p_codec, p_mb, chromaFlag, iCbCr, mbW, mbH, botFieldFlag, &refArrayW, &refArrayH, refSampleArray, refSampleArrayAvailability, &xOffset, &yOffset);
        if (err) {
            return err;
        }

        // G.8.6.2.3 Interpolation process for Intra_Base prediction
        err = _hl_codec_264_decode_svc_interpol_intra_base(
                  p_codec, p_mb,
                  filteringModeFlag,
                  chromaFlag,
                  botFieldFlag,
                  fldPrdInFrmMbFlag,
                  yBorder,
                  refArrayW, refArrayH, refSampleArray,
                  xOffset, yOffset,
                  mbW, mbH, (int32_t*)mbPred, predArrayStride);
        if (err) {
            return err;
        }
    }
    else if (!topAndBotResamplingFlag) {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    return HL_ERROR_SUCCESS;
}

// G.8.6.2.2 Reference layer sample array construction process prior to intra resampling
static HL_ERROR_T _hl_codec_264_decode_svc_ref_layer_array_construct_prior_to_intra_resampling(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
        int32_t chromaFlag, int32_t iCbCr, int32_t mbW, int32_t mbH, int32_t botFieldFlag,
        int32_t* refArrayW, int32_t* refArrayH, HL_OUT int32_t* refSampleArray, HL_OUT uint8_t* refSampleArrayAvailability,
        int32_t* xOffset, int32_t* yOffset
                                                                                              )
{
    int32_t xRefMin16, yRefMin16, xRefMax16, yRefMax16, refW, refH, refMbW, refMbH, xMin, yMin, xMax, yMax, yRefScale, yRefAdd;
    int32_t x, y, refSliceIdcMb, refSliceIdc, refIntraMbFlag, xRef, yRef;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    int32_t constrained_intra_resampling_flag;
    const hl_pixel_t *refLayerPicSamples;
    int32_t refLayerPicSamplesStride, *refSampleArrayPtr = refSampleArray;
    uint8_t* refSampleArrayAvailabilityPtr = refSampleArrayAvailability;
    hl_bool_t b_contains_not_avail_for_intrabase = HL_FALSE;

    constrained_intra_resampling_flag = pc_slice_header->ext.svc.constrained_intra_resampling_flag;
    refLayerPicSamples = chromaFlag ? (iCbCr ? pc_layer->pc_ref->pc_fs_curr->p_pict->pc_data_v : pc_layer->pc_ref->pc_fs_curr->p_pict->pc_data_u) : pc_layer->pc_ref->pc_fs_curr->p_pict->pc_data_y;
    refLayerPicSamplesStride = chromaFlag ? pc_layer->RefLayerPicWidthInSamplesC : pc_layer->RefLayerPicWidthInSamplesL;

    // G.6.3 Derivation process for reference layer sample locations in resampling
    hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, 0/*xP*/, 0/*yP*/, botFieldFlag, &xRefMin16, &yRefMin16);
    hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, (mbW - 1)/*xP*/, (mbH - 1)/*yP*/, botFieldFlag, &xRefMax16, &yRefMax16);

    if (chromaFlag == 0) {
        refW = pc_layer->RefLayerPicWidthInSamplesL; //(G-266)
        refH = pc_layer->RefLayerPicHeightInSamplesL; // (G-267)
        refMbW = 16; // (G-268)
        refMbH = 16; // (G-269)
    }
    else {
        refW = pc_layer->RefLayerPicWidthInSamplesC; //(G-266)
        refH = pc_layer->RefLayerPicHeightInSamplesC; // (G-267)
        refMbW = pc_layer->RefLayerMbWidthC; // (G-268)
        refMbH = pc_layer->RefLayerMbHeightC; // (G-269)
    }

    *xOffset = ( ( ( xRefMin16 - 64 ) >> 8 ) << 4 ) - ( refMbW >> 1 ); // (G-270)
    *yOffset = ( ( ( yRefMin16 - 64 ) >> 8 ) << 4 ) - ( refMbH >> 1 ); // (G-271)
    *refArrayW = ( ( ( xRefMax16 + 79 ) >> 8 ) << 4 ) + 3 * ( refMbW >> 1 ) - *xOffset; // (G-272)
    *refArrayH = ( ( ( yRefMax16 + 79 ) >> 8 ) << 4 ) + 3 * ( refMbH >> 1 ) - *yOffset; // (G-273)

    xMin = ( xRefMin16 >> 4 ) - *xOffset; // (G-274)
    yMin = ( yRefMin16 >> 4 ) - *yOffset; // (G-275)
    xMax = ( ( xRefMax16 + 15 ) >> 4 ) - *xOffset; // (G-276)
    yMax = ( ( yRefMax16 + 15 ) >> 4 ) - *yOffset; // (G-277)

    if (pc_layer->RefLayerFrameMbsOnlyFlag || pc_layer->RefLayerFieldPicFlag) {
        yRefScale = 1;
        yRefAdd = 0;
    }
    else {
        yRefScale = 2;
        yRefAdd = botFieldFlag;
    }

    // FIXME: should "refSliceIdcMb" be an array?

    refSliceIdcMb = -1;

    if (constrained_intra_resampling_flag) {
        // int32_t useIntraPredFlag, mbAddrRefLayer;
        for (y = (yMin + 1); y < yMax; ++y) {
            for (x = (xMin + 1);  x < xMax; ++x) {
                xRef = HL_MATH_MAX( 0, HL_MATH_MIN( refW - 1, x + *xOffset ) ); // (G-278)
                yRef = yRefScale * HL_MATH_MAX( 0, HL_MATH_MIN( refH / yRefScale - 1, y + *yOffset ) ) + yRefAdd; // (G-279)
                // G.8.6.2.2.1 Derivation process for reference layer slice and intra macroblock identifications
                hl_codec_264_utils_derivation_process_for_ref_layer_slice_and_intra_mb_identifications_svc(p_codec, p_mb, xRef, yRef, refMbW, refMbH, &refSliceIdc, &refIntraMbFlag);
                if (refIntraMbFlag && refSliceIdcMb < 0) {
                    refSliceIdcMb = refSliceIdc;
                }
            }
        }
#if 0 // Use to test for conformance
        useIntraPredFlag = 0;
        for (y = 0; y < 16; ++y) {
            for (x = 0; x < 16; ++x) {
                // G.6.1 Derivation process for reference layer macroblocks
                hl_codec_264_utils_derivation_process_for_ref_layer_mb_svc(p_codec, p_mb, x, y, p_mb->ext.svc.fieldMbFlag, &mbAddrRefLayer, &xRef, &yRef);
            }
        }
#endif
    }

    for (y = 0; y < *refArrayH; ++y) {
        yRef = yRefScale * HL_MATH_MAX( 0, HL_MATH_MIN( refH / yRefScale - 1, y + *yOffset ) ) + yRefAdd; // (G-281)
        for (x = 0; x < *refArrayW; ++x) {
            xRef = HL_MATH_MAX( 0, HL_MATH_MIN( refW - 1, x + *xOffset ) ); // (G-280)
            // G.8.6.2.2.1 Derivation process for reference layer slice and intra macroblock identifications
            hl_codec_264_utils_derivation_process_for_ref_layer_slice_and_intra_mb_identifications_svc(p_codec, p_mb, xRef, yRef, refMbW, refMbH, &refSliceIdc, &refIntraMbFlag);
            if (!refIntraMbFlag || (constrained_intra_resampling_flag && (refSliceIdcMb < 0 || refSliceIdc != refSliceIdcMb))) {
                refSampleArrayPtr[x] = 0;
                refSampleArrayAvailabilityPtr[x] = 0;
                b_contains_not_avail_for_intrabase = HL_TRUE;
            }
            else {
                refSampleArrayPtr[x] = refLayerPicSamples[(yRef * refLayerPicSamplesStride) + xRef];
                refSampleArrayAvailabilityPtr[x] = 1;
            }
        }
        refSampleArrayPtr += *refArrayW;
        refSampleArrayAvailabilityPtr += *refArrayW;
    }

    if (b_contains_not_avail_for_intrabase) {
        // G.8.6.2.2.2 Construction process for not available sample values prior to intra resampling
        hl_codec_264_utils_derivation_process_for_not_avail_samples_prior_to_intra_resampling_svc(
            p_codec,
            p_mb,
            refMbW, refMbH,
            *refArrayW, *refArrayH, refSampleArray, refSampleArrayAvailability,
            *xOffset, *yOffset);
    }

    return HL_ERROR_SUCCESS;
}

// G.8.6.2.3 Interpolation process for Intra_Base prediction
static HL_ERROR_T _hl_codec_264_decode_svc_interpol_intra_base(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t filteringModeFlag,
    int32_t chromaFlag,
    int32_t botFieldFlag,
    int32_t fldPrdInFrmMbFlag,
    int32_t yBorder,
    int32_t refArrayW, int32_t refArrayH, int32_t* refSampleArray,
    int32_t xOffset, int32_t yOffset,
    int32_t mbW, int32_t mbH, HL_OUT int32_t* predArray, int32_t predArrayStride
)
{
    int32_t yP, xRef16, yRef16, xRef, yRef, xPhase, yPhase, tempArrayH, tempArrayW, *tempArrayPtr, x, y, predArrayW, predArrayH, maxv, v;
    int32_t *tempArray;
    hl_size_t tempArrayCount;
    hl_codec_264_layer_t* pc_layer;

    pc_layer = p_codec->layers.pc_active;
    maxv = (1 << (chromaFlag ? p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->BitDepthC : p_codec->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->BitDepthY)) - 1;
    tempArray = pc_layer->tempArray_ListPtr[p_mb->u_slice_idx];

    tempArrayW = refArrayW;
    tempArrayH = (mbH / ( 1 + fldPrdInFrmMbFlag ) + 2 * yBorder);
    tempArrayCount = tempArrayW * tempArrayH;
    if (!tempArray || pc_layer->tempArray_ListCount[p_mb->u_slice_idx] < tempArrayCount) {
        HL_ERROR_T err = hl_codec_264_layer_alloc_temparray_svc(pc_layer, p_mb->u_slice_idx, tempArrayCount);
        if (err) {
            return err;
        }
        tempArray = pc_layer->tempArray_ListPtr[p_mb->u_slice_idx];
    }

    tempArrayPtr = tempArray;
    for (y = 0; y < tempArrayH; ++y) {
        yP = ( y - yBorder ) * ( 1 + fldPrdInFrmMbFlag ) + botFieldFlag; // (G-298)

        // G.6.3 Derivation process for reference layer sample locations in resampling
        hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, 0/*xP*/, yP, botFieldFlag, &xRef16, &yRef16);

        yRef = ( yRef16 >> 4 ) - yOffset; // (G-299)
        yPhase = ( yRef16 - 16 * yOffset ) & 15; // (G-300)

        // FIXME: use "refSampleArrayPtr" to avoid computing refSampleArray's indices for each call

        if (chromaFlag) {
            for(x = 0; x < tempArrayW; ++x) {
                tempArrayPtr[x] = HL_CODEC_264_SVC_16_PHASE_CHROMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[yPhase][0] * refSampleArray[((yRef) * refArrayW) + x] +
                                  HL_CODEC_264_SVC_16_PHASE_CHROMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[yPhase][1] * refSampleArray[((yRef + 1) * refArrayW) + x];
            }
        }
        else {
            if (filteringModeFlag) {
                for(x = 0; x < tempArrayW; ++x) {
                    tempArrayPtr[x] = ( 16 - yPhase ) * refSampleArray[((yRef) * refArrayW) + x] + yPhase * refSampleArray[((yRef + 1) * refArrayW) + x]; // (G-302)
                }
            }
            else {
                for(x = 0; x < tempArrayW; ++x) {
                    tempArrayPtr[x] = HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[yPhase][0] * refSampleArray[((yRef - 1) * refArrayW) + x] +
                                      HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[yPhase][1] * refSampleArray[((yRef) * refArrayW) + x] +
                                      HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[yPhase][2] * refSampleArray[((yRef + 1) * refArrayW) + x] +
                                      HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[yPhase][3] * refSampleArray[((yRef + 2) * refArrayW) + x]; // (G-301)
                }
            }
        }
        tempArrayPtr += tempArrayW;
    }

    predArrayW = mbW;
    predArrayH = (mbH / ( 1 + fldPrdInFrmMbFlag ) + 2 * yBorder);
    // FIXME: Do not compute indices several times
    for (x = 0; x < predArrayW; ++x) {
        // G.6.3 Derivation process for reference layer sample locations in resampling
        hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, x/*xP*/, 0/*yP*/, botFieldFlag, &xRef16, &yRef16);

        xRef = ( xRef16 >> 4 ) - xOffset; // (G-303)
        xPhase = ( xRef16 - 16 * xOffset ) & 15; // (G-304)

        if (chromaFlag) {
            for (y = 0; y < predArrayH; ++y) {
                v = ( HL_CODEC_264_SVC_16_PHASE_CHROMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[xPhase][0] * tempArray[(y * tempArrayW) + xRef] +
                      HL_CODEC_264_SVC_16_PHASE_CHROMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[xPhase][1] * tempArray[(y * tempArrayW) + xRef + 1] + 512 ) >> 10; // (G-305)
                predArray[(y * predArrayStride) + x] = HL_MATH_CLIP3(0, maxv, v);
            }
        }
        else {
            if (filteringModeFlag) {
                for (y = 0; y < predArrayH; ++y) {
                    predArray[(y * predArrayStride) + x] = ( ( 16 - xPhase ) * tempArray[(y * tempArrayW) + xRef] + xPhase * tempArray[(y * tempArrayW) + xRef + 1] + 128 ) >> 8; // (G-306)
                }
            }
            else {
                for (y = 0; y < predArrayH; ++y) {
                    v = ( HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[xPhase][0] * tempArray[(y * tempArrayW) + xRef - 1] +
                          HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[xPhase][1] * tempArray[(y * tempArrayW) + xRef] +
                          HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[xPhase][2] * tempArray[(y * tempArrayW) + xRef + 1] +
                          HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[xPhase][3] * tempArray[(y * tempArrayW) + xRef + 2] + 512 ) >> 10; // (G-305)
                    predArray[(y * predArrayStride) + x] = HL_MATH_CLIP3(0, maxv, v);
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// G.8.6.3 Resampling process for residual samples
static HL_ERROR_T _hl_codec_264_decode_svc_residual_resampling(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    const int32_t* refLayerPicSamplesL, const int32_t* refLayerPicSamplesCb, const int32_t* refLayerPicSamplesCr,
    int32_t* picSamplesL, int32_t* picSamplesCb, int32_t* picSamplesCr
)
{
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredL[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredCb[16][16];
    HL_ALIGN(HL_ALIGN_V) int32_t mbPredCr[16][16];
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    static const int32_t chromaFlagZero = 0;
    static const int32_t chromaFlagOne = 1;
    HL_ERROR_T err;

    // G.8.6.3.1 Resampling process for residual samples of a macroblock colour component
    err =_hl_codec_264_decode_svc_residual_resampling_mb_colour_comps(p_codec, p_mb, chromaFlagZero, 16, 16, mbPredL, refLayerPicSamplesL);
    if (err) {
        return err;
    }

    if (pc_sps->ChromaArrayType) {
        // G.8.6.3.1 Resampling process for residual samples of a macroblock colour component
        err =_hl_codec_264_decode_svc_residual_resampling_mb_colour_comps(p_codec, p_mb, chromaFlagOne, pc_sps->MbWidthC, pc_sps->MbHeightC, mbPredCb, refLayerPicSamplesCb);
        if (err) {
            return err;
        }
        err =_hl_codec_264_decode_svc_residual_resampling_mb_colour_comps(p_codec, p_mb, chromaFlagOne, pc_sps->MbWidthC, pc_sps->MbHeightC, mbPredCr, refLayerPicSamplesCr);
        if (err) {
            return err;
        }
    }

    // G.8.5.4.1 Picture sample array construction process
    err = _hl_codec_264_decode_svc_sample_array_pict_construct(p_codec, p_mb, picSamplesL, picSamplesCb, picSamplesCr, sizeof(picSamplesL[0]), mbPredL, mbPredCb, mbPredCr);
    if (err) {
        return err;
    }

    return err;
}

// G.8.6.3.1 Resampling process for residual samples of a macroblock colour component
static HL_ERROR_T _hl_codec_264_decode_svc_residual_resampling_mb_colour_comps(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t chromaFlag,
    int32_t mbW, int32_t mbH,
    int32_t mbPred[16][16],
    const int32_t* refLayerPicSamples
)
{
    int32_t botFieldFlag, frameBasedResamplingFlag, topAndBotResamplingFlag, botFieldFrameMbsOnlyRefFlag;
    hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (pc_layer->RefLayerFrameMbsOnlyFlag) {
        botFieldFlag = 0;
    }
    else if (pc_slice_header->field_pic_flag) {
        botFieldFlag = pc_slice_header->bottom_field_flag;
    }
    else if (pc_layer->RefLayerFieldPicFlag) {
        botFieldFlag = pc_layer->RefLayerBottomFieldFlag;
    }
    else if (p_mb->ext.svc.fieldMbFlag) {
        botFieldFlag = p_mb->u_addr & 1;
    }
    else {
        botFieldFlag = 0;
    }

    frameBasedResamplingFlag = pc_layer->RefLayerFrameMbsOnlyFlag && pc_sps->frame_mbs_only_flag;
    topAndBotResamplingFlag = (!pc_layer->RefLayerFrameMbsOnlyFlag && !pc_layer->RefLayerFieldPicFlag && !pc_sps->frame_mbs_only_flag && !p_mb->ext.svc.fieldMbFlag);
    botFieldFrameMbsOnlyRefFlag = (pc_layer->RefLayerFrameMbsOnlyFlag && p_mb->ext.svc.fieldMbFlag) &&
                                  ((pc_slice_header->field_pic_flag && pc_slice_header->bottom_field_flag) || (!pc_slice_header->field_pic_flag && (p_mb->u_addr & 1)));

    if (botFieldFrameMbsOnlyRefFlag) {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else if (frameBasedResamplingFlag || p_mb->ext.svc.fieldMbFlag) {
        static const int32_t yBorder = 0;
        static const int32_t fldPrdInFrmMbFlag = 0;
        static const int32_t predArrayStride = 16;
        int32_t refArrayW, refArrayH, xOffset, yOffset, *refSampleArray = pc_layer->refSampleArray_ListPtr[ p_mb->u_slice_idx], *refTransBlkIdc = pc_layer->refTransBlkIdc_ListPtr[ p_mb->u_slice_idx];

        // Allocate temp memory if not already done
        if (!refSampleArray || !refTransBlkIdc) {
            err = hl_codec_264_layer_alloc_refsamplearray_svc(pc_layer, p_mb->u_slice_idx);
            if (err) {
                return err;
            }
            refSampleArray = pc_layer->refSampleArray_ListPtr[ p_mb->u_slice_idx];
            refTransBlkIdc = pc_layer->refTransBlkIdc_ListPtr[ p_mb->u_slice_idx];
        }

        // G.8.6.3.2 Reference layer sample array construction process prior to residual resampling
        err = _hl_codec_264_decode_svc_residual_ref_layer_construction_prior_to_resampling(
                  p_codec, p_mb,
                  chromaFlag,
                  mbW, mbH,
                  botFieldFlag,
                  yBorder,
                  refLayerPicSamples,
                  &refArrayW, &refArrayH,
                  refSampleArray,
                  refTransBlkIdc,
                  &xOffset, &yOffset);
        if (err) {
            return err;
        }
        // G.8.6.3.3 Interpolation process for residual prediction
        err = _hl_codec_264_decode_svc_residual_interpol(
                  p_codec, p_mb,
                  chromaFlag,
                  mbW, mbH,
                  botFieldFlag,
                  fldPrdInFrmMbFlag,
                  yBorder,
                  refArrayW, refArrayH,
                  refSampleArray,
                  refTransBlkIdc,
                  xOffset, yOffset,
                  (int32_t*)mbPred, predArrayStride);
        if (err) {
            return err;
        }
    }
    else if (!topAndBotResamplingFlag) {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return err;
}

// G.8.6.3.2 Reference layer sample array construction process prior to residual resampling
static HL_ERROR_T _hl_codec_264_decode_svc_residual_ref_layer_construction_prior_to_resampling(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t chromaFlag,
    int32_t mbW, int32_t mbH,
    int32_t botFieldFlag,
    int32_t yBorder,
    const int32_t* refLayerPicSamples,
    HL_OUT int32_t *refArrayW, int32_t *refArrayH,
    HL_OUT int32_t* refSampleArray,
    HL_OUT int32_t* refTransBlkIdc,
    HL_OUT int32_t* xOffset, int32_t *yOffset
)
{
    int32_t x, y, refW, refH, refMbW, refMbH,  yRefScale, yRefAdd, xRefMin16, yRefMin16, xRefMax16, yRefMax16, xRef, yRef;
    const hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    const hl_codec_264_nal_slice_header_t* pc_slice_header = pc_layer->pc_slice_hdr;
    const hl_codec_264_nal_sps_t* pc_sps = pc_slice_header->pc_pps->pc_sps;
    int32_t *refSampleArrayPtr, *refTransBlkIdcPtr;
    const int32_t* refLayerPicSamplesPtr;

    HL_ERROR_T err;

    // G.6.3 Derivation process for reference layer sample locations in resampling
    hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, 0, -yBorder, botFieldFlag, &xRefMin16, &yRefMin16);
    // G.6.3 Derivation process for reference layer sample locations in resampling
    hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, (mbW - 1), (mbH - 1 + yBorder), botFieldFlag, &xRefMax16, &yRefMax16);

    if (chromaFlag) {
        refW = pc_layer->RefLayerPicWidthInSamplesC; // (G-312)
        refH = pc_layer->RefLayerPicHeightInSamplesC; // (G-313)
        refMbW = pc_layer->RefLayerMbWidthC; // (G-314)
        refMbH = pc_layer->RefLayerMbHeightC; // (G-315)
    }
    else {
        refW = pc_layer->RefLayerPicWidthInSamplesL; // (G-312)
        refH = pc_layer->RefLayerPicHeightInSamplesL; // (G-313)
        refMbW = 16; // (G-314)
        refMbH = 16; // (G-315)
    }

    *xOffset = ( xRefMin16 >> 4 ); // (G-316)
    *yOffset = ( yRefMin16 >> 4 ); // (G-317)
    *refArrayW = ( xRefMax16 >> 4 ) - *xOffset + 2; // (G-318)
    *refArrayH = ( yRefMax16 >> 4 ) - *yOffset + 2; // (G-319)

    if (pc_layer->RefLayerFrameMbsOnlyFlag || pc_layer->RefLayerFieldPicFlag) {
        yRefScale = 1;
        yRefAdd = 0;
    }
    else {
        yRefScale = 2;
        yRefAdd = botFieldFlag;
    }

    refSampleArrayPtr = refSampleArray;
    refLayerPicSamplesPtr = refLayerPicSamples;
    refTransBlkIdcPtr = refTransBlkIdc;
    for (y = 0; y < *refArrayH; ++y) {
        yRef = (int32_t)yRefScale * HL_MATH_MAX( 0, HL_MATH_MIN( refH / yRefScale - 1, y + *yOffset ) ) + yRefAdd; // (G-321)
        refLayerPicSamplesPtr = (refLayerPicSamples + (refW * yRef));
        for (x = 0; x < *refArrayW; ++x) {
            xRef = (int32_t)HL_MATH_MAX( 0, HL_MATH_MIN( refW - 1, x + *xOffset ) ); // (G-320)
            refSampleArrayPtr[x] = refLayerPicSamplesPtr[xRef]; // (G-322)
            // FIXME: inline "G.8.6.3.2.1"
            // G.8.6.3.2.1 Derivation process for reference layer transform block identifications
            err = hl_codec_264_utils_derivation_process_for_ref_layer_transform_block_identifications_svc(p_codec, p_mb, xRef, yRef, chromaFlag, refMbW, refMbH, &refTransBlkIdcPtr[x]);
            if (err) {
                return err;
            }
        }
        refSampleArrayPtr += *refArrayW;
        refTransBlkIdcPtr += *refArrayW;
    }

    return err;
}

// G.8.6.3.3 Interpolation process for residual prediction
static HL_ERROR_T _hl_codec_264_decode_svc_residual_interpol(
    hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb,
    int32_t chromaFlag,
    int32_t mbW, int32_t mbH,
    int32_t botFieldFlag,
    int32_t fldPrdInFrmMbFlag,
    int32_t yBorder,
    int32_t refArrayW, int32_t refArrayH,
    const int32_t* refSampleArray,
    const int32_t* refTransBlkIdc,
    int32_t xOffset, int32_t yOffset,
    int32_t* predArray, int32_t predArrayStride
)
{
    int32_t x, y, yP, predArrayW, predArrayH, xRef, yRef, xPhase, yPhase, tempPred[2], xRefRound, xRef16, yRef16;
    const int32_t *refTransBlkIdcPtr0, *refTransBlkIdcPtr1;
    int32_t *predArrayPtr;

    predArrayW = mbW;
    predArrayH = mbH / ( 1 + fldPrdInFrmMbFlag ) + 2 *yBorder;
    predArrayPtr = predArray;

    for (y = 0; y < predArrayH; ++y) {
        yP = ( y - yBorder ) * ( 1 + fldPrdInFrmMbFlag ) + botFieldFlag; // (G-334)
        for (x = 0; x < predArrayW; ++x) {
            // G.6.3 Derivation process for reference layer sample locations in resampling
            hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(p_codec, p_mb, chromaFlag, x, yP, botFieldFlag, &xRef16, &yRef16);
            xRef = ( xRef16 >> 4 ) - xOffset; // (G-335)
            yRef = ( yRef16 >> 4 ) - yOffset; // (G-336)
            xPhase = ( xRef16 - 16 * xOffset ) % 16; // (G-337)
            yPhase = ( yRef16 - 16 * yOffset ) % 16; // (G-338)

            refTransBlkIdcPtr0 = refTransBlkIdc + (yRef * refArrayW); // dY = 0
            refTransBlkIdcPtr1 = refTransBlkIdcPtr0 + refArrayW; // dY = 1
            if (refTransBlkIdcPtr0[xRef] == refTransBlkIdcPtr0[xRef + 1]) {
                tempPred[0] = ( 16 - xPhase ) * refSampleArray[(yRef * refArrayW) + xRef] + xPhase * refSampleArray[(yRef * refArrayW) + xRef + 1]; // (G-339)
            }
            else {
                tempPred[ 0 ] = ( ( xPhase < 8 ) ? refSampleArray[(yRef * refArrayW) + xRef] : refSampleArray[(yRef * refArrayW) + xRef + 1] ) << 4; // (G-340)
            }
            if (refTransBlkIdcPtr1[xRef] == refTransBlkIdcPtr1[xRef + 1]) {
                tempPred[1] = ( 16 - xPhase ) * refSampleArray[((yRef + 1) * refArrayW) + xRef] + xPhase * refSampleArray[((yRef + 1) * refArrayW) + xRef + 1]; // (G-339)
            }
            else {
                tempPred[ 1 ] = ( ( xPhase < 8 ) ? refSampleArray[((yRef + 1) * refArrayW) + xRef] : refSampleArray[((yRef + 1) * refArrayW) + xRef + 1] ) << 4; // (G-340)
            }

            xRefRound = (xRef + ( xPhase / 8 ));
            refTransBlkIdcPtr0 = refTransBlkIdc + (yRef * refArrayW) + xRefRound;
            refTransBlkIdcPtr1 = refTransBlkIdcPtr0 + refArrayW;
            if (*refTransBlkIdcPtr0 == *refTransBlkIdcPtr1) {
                predArrayPtr[x] = ( ( 16 - yPhase ) * tempPred[0] + yPhase * tempPred[1] + 128 ) >> 8; // (G-341)
            }
            else {
                predArrayPtr[x] = ( ( ( yPhase < 8 ) ? tempPred[0] : tempPred[1] ) + 8 ) >> 4; // (G-342)
            }
        }
        predArrayPtr += predArrayStride;
    }

    return HL_ERROR_SUCCESS;
}