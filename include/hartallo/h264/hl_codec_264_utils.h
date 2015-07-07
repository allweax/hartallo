#ifndef _HARTALLO_CODEC_264_UTILS_H_
#define _HARTALLO_CODEC_264_UTILS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_tables.h"

struct hl_codec_264_s;

#define hl_codec_264_utils_is_all_neighbouringblocks_zero( \
	/* const hl_codec_264_residual_inv_xt* */ invType,  \
	/* const struct hl_codec_264_s* */ pc_codec,  \
	/* int32_t */ mbAddrN, \
	/*int32_t */ blkN) \
	/* The macroblock mbAddrN has mb_type not equal to I_PCM and all AC residual transform */ \
	/* coefficient levels of the neighbouring block blkN are equal to 0 due to the corresponding bit of */ \
	/* CodedBlockPatternLuma or CodedBlockPatternChroma being equal to 0. */ \
	((((invType)->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) || ((invType)->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL)) ? \
		(((pc_codec)->layers.pc_active->pp_list_macroblocks[(mbAddrN)]->CodedBlockPatternChroma & 2) == 0) : \
		(((pc_codec)->layers.pc_active->pp_list_macroblocks[(mbAddrN)]->CodedBlockPatternLuma & (1 << ((blkN) >> 2))) == 0)) \
 
// 6.4.1 Inverse macroblock scanning process
// Must use "currMb->xL" and "currMb->yL" if "mbAddr" = "CurrMbAddr"
#define hl_codec_264_utils_inverse_macroblock_scanning_process( \
	/* const struct hl_codec_264_s* */ pc_codec,  \
	/* int32_t */mbAddr,  \
	/* int32_t* */x,  \
	/* int32_t* */y \
) \
{ \
	if ((pc_codec)->layers.pc_active->pc_slice_hdr->MbaffFrameFlag){ /* SVC depends on "fieldMbFlag". See "G.6.1" */ \
		HL_DEBUG_ERROR("MbAFF not implemented yet"); \
        *(x) = *(y) = 0; \
	} \
	else { \
		*(x) = InverseRasterScan_Pow2((mbAddr), 16, 16, (pc_codec)->layers.pc_active->pc_slice_hdr->PicWidthInSamplesL, 0); /*(6-3)*/ \
		*(y) = InverseRasterScan_Pow2((mbAddr), 16, 16, (pc_codec)->layers.pc_active->pc_slice_hdr->PicWidthInSamplesL, 1); /*(6-4)*/ \
	} \
}

// 6.4.10.1 Derivation process for neighbouring macroblocks
#define hl_codec_264_utils_derivation_process_for_neighbouring_macroblocks( \
	/* const struct hl_codec_264_s* */ pc_codec, \
	/* const struct hl_codec_264_mb_s* */ pc_mb, \
	/* int32_t* */ mbAddrN, \
	/* int32_t A=0/B=1/C=2/D=3 */ N, \
	/* hlBool_t */ luma) \
{ \
	static int32_t __unused; \
	hl_codec_264_utils_derivation_process_for_neighbouring_locations((pc_codec), (pc_mb), xD_yD[(N)][0], xD_yD[(N)][1], (mbAddrN), (&__unused), (&__unused), (luma)); \
}

// 6.4.11 Derivation process for neighbouring locations
#define hl_codec_264_utils_derivation_process_for_neighbouring_locations( \
	/* const struct hl_codec_264_s* */ pc_codec, \
	/* const struct hl_codec_264_mb_s* */ pc_mb, \
	/* int32_t  */xN, /* int32_t */yN,  \
	/* int32_t* */ mbAddrN, \
	/* int32_t* */ xW, /* int32_t* */ yW,  \
	/* hlBool_t */ luma) \
{ \
	int32_t maxW = luma ? 16 : (pc_codec)->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->MbWidthC;/*(6-32) */ \
	int32_t maxH = luma ? 16 : (pc_codec)->layers.pc_active->pc_slice_hdr->pc_pps->pc_sps->MbHeightC; /*(6-33) */ \
	if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag) { \
		/* Otherwise (MbaffFrameFlag is equal to 1), the specification for neighbouring locations in MBAFF frames as */ \
		/* described in subclause 6.4.11.2 is applied. */ \
		HL_DEBUG_ERROR("MbAFF not implemented yet"); \
	} \
	else { \
		/* If MbaffFrameFlag is equal to 0, the specification for neighbouring locations in fields and non-MBAFF frames as */ \
		/* described in subclause 6.4.11.1 is applied. */ \
		 \
		/* 6.4.11.1 Specification for neighbouring locations in fields and non-MBAFF frames */ \
		hl_codec_264_utils_specification_for_neighbouring_locations_in_fields_and_non_MBAFF_frames(pc_codec, pc_mb, mbAddrN, xN, yN, maxW, maxH, xW, yW); \
	} \
}

// 6.4.11.1 Specification for neighbouring locations in fields and non-MBAFF frames
#define hl_codec_264_utils_specification_for_neighbouring_locations_in_fields_and_non_MBAFF_frames( \
	pc_codec, /* const struct hl_codec_264_s*: IN */ \
	pc_mb, /* const struct hl_codec_264_mb_s* : IN */ \
	mbAddrN, /* int32_t*: OUT*/ \
	xN, /* int32_t: IN*/ \
	yN, /* int32_t: IN*/ \
	maxW, /* int32_t: IN*/ \
	maxH, /* int32_t: IN*/ \
	xW, /* int32_t*: OUT*/ \
	yW /* int32_t*: OUT*/) \
{ \
	if ((0<=xN && xN<=maxW-1) && (0<=yN && yN<=maxH-1)) { \
		*mbAddrN = (pc_mb)->u_addr; \
	} \
	else if ((0<=xN && xN<=maxW-1) && yN<0) { \
		*mbAddrN = (pc_mb)->neighbours.i_addr_B; \
	} \
	else if (xN>maxW-1 && yN<0) { \
		*mbAddrN = (pc_mb)->neighbours.i_addr_C; \
	} \
	else if (xN <0 && yN<0){ \
		*mbAddrN = (pc_mb)->neighbours.i_addr_D; \
	} \
	else if (xN <0 && (0<= yN && yN<=maxH-1)) { \
		*mbAddrN = (pc_mb)->neighbours.i_addr_A; \
	} \
	else { \
		*mbAddrN = HL_CODEC_264_MB_ADDR_NOT_AVAIL; \
	} \
	\
	*xW = (xN + maxW) % maxW; /*(6-34)*/ \
	*yW = (yN + maxH) % maxH; /*(6-35)*/  \
}

// 6.4.12.1 Derivation process for 4x4 luma block indices
#define hl_codec_264_utils_derivation_process_for_4x4_luma_block_indices(xP, yP) LumaBlockIndices4x4_YX[(yP)][(xP)]

// 6.4.12.2 Derivation process for 4x4 chroma block indices
// See "wftp3.itu.int/av-arch/video-site/1101_Dae/VCEG-AP12.doc" Issue #15
// This subclause is only invoked when ChromaArrayType is equal to 1 or 2.
#define hl_codec_264_utils_derivation_process_for_4x4_chroma_block_indices(xP, yP) ChromaBlockIndices4x4_YX[(yP)][(xP)]

// 6.4.12.3 Derivation process for 8x8 luma block indices
#define hl_codec_264_utils_derivation_process_for_8x8_luma_block_indices(xP, yP) LumaBlockIndices8x8_YX[(yP)][(xP)]


HL_ERROR_T hl_codec_264_utils_guess_level(hl_size_t width, hl_size_t height, enum HL_CODEC_264_LEVEL_E* p_level);
HL_ERROR_T hl_codec_264_utils_init_mb_current_avc(struct hl_codec_264_s* p_codec, uint32_t u_mb_addr, long l_slice_id, hl_bool_t b_pskip);
HL_ERROR_T hl_codec_264_utils_init_mb_current_svc(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s *p_mb);
HL_ERROR_T  hl_codec_264_utils_derivation_process_for_neighbouring_4x4_luma_blocks(
    HL_IN const struct hl_codec_264_s* pc_codec,
    HL_IN const struct hl_codec_264_mb_s* pc_mb,
    HL_IN int32_t luma4x4BlkIdx,
    HL_OUT int32_t* mbAddrA,
    HL_OUT int32_t* luma4x4BlkIdxA,
    HL_OUT int32_t* mbAddrB,
    HL_OUT int32_t* luma4x4BlkIdxB);
HL_ERROR_T hl_codec_264_utils_derivation_process_for_neighbouring_4x4_chroma_blocks(
    HL_IN const struct hl_codec_264_s* pc_codec,
    HL_IN const struct hl_codec_264_mb_s* pc_mb,
    HL_IN int32_t chroma4x4BlkIdx,
    HL_OUT int32_t* mbAddrA,
    HL_OUT int32_t* chroma4x4BlkIdxA,
    HL_OUT int32_t* mbAddrB,
    HL_OUT int32_t* chroma4x4BlkIdxB);

// 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
#define InverseScan4x4(in16, out4x4) \
{ \
	/* FIXME: use ZigZag4x4FieldScanYX for field macroblock */  \
	/* Formula: out4x4[ZigZag4x4BlockScanYX[i][0]][ZigZag4x4BlockScanYX[i][1]] = in16[i] */ \
	out4x4[0][0] = in16[0]; \
	out4x4[0][1] = in16[1]; \
	out4x4[1][0] = in16[2]; \
	out4x4[2][0] = in16[3]; \
	out4x4[1][1] = in16[4]; \
	out4x4[0][2] = in16[5]; \
	out4x4[0][3] = in16[6]; \
	out4x4[1][2] = in16[7]; \
	out4x4[2][1] = in16[8]; \
	out4x4[3][0] = in16[9]; \
	out4x4[3][1] = in16[10]; \
	out4x4[2][2] = in16[11]; \
	out4x4[1][3] = in16[12]; \
	out4x4[2][3] = in16[13]; \
	out4x4[3][2] = in16[14]; \
	out4x4[3][3] = in16[15]; \
}

#define Scan4x4_L(in4x4, out16, isIntra16x16AC) \
{ \
	/* FIXME: use ZigZag4x4FieldScanYX for field macroblock */  \
	 \
	register int32_t i; \
	if (isIntra16x16AC) { \
		for (i=1; i<16; ++i) { /*!\ not multiple of 4 */ \
			out16[i-1] = in4x4[ZigZag4x4BlockScanYX[i][0]][ZigZag4x4BlockScanYX[i][1]]; \
		} \
	} \
	else{ \
		for (i=0; i<16; ++i){ \
			out16[i] = in4x4[ZigZag4x4BlockScanYX[i][0]][ZigZag4x4BlockScanYX[i][1]]; \
		} \
	} \
}

#define Scan4x4_AC_C(in4x4,  out16) \
{ \
	/* FIXME: use ZigZag4x4FieldScanYX for field macroblock */  \
	register int32_t i;  \
	for (i=1; i<16; ++i){ /*!\ not multiple of 4 */ \
		out16[i-1] = in4x4[ZigZag4x4BlockScanYX[i][0]][ZigZag4x4BlockScanYX[i][1]]; \
	} \
}

#define Scan2x2_DC_C(in2x2, out4) \
{ \
	/* FIXME: use ZigZag2x2FieldScanYX for field macroblock */  \
	out4[0] = in2x2[ZigZag2x2BlockScanYX[0][0]][ZigZag2x2BlockScanYX[0][1]]; \
	out4[1] = in2x2[ZigZag2x2BlockScanYX[1][0]][ZigZag2x2BlockScanYX[1][1]]; \
	out4[2] = in2x2[ZigZag2x2BlockScanYX[2][0]][ZigZag2x2BlockScanYX[2][1]]; \
	out4[3] = in2x2[ZigZag2x2BlockScanYX[3][0]][ZigZag2x2BlockScanYX[3][1]]; \
}

// 8.4.1 Derivation process for motion vector components and reference indices
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_movect_comps_and_ref_indices(
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
    HL_OUT int32_t* subMvCnt);

// 8.4.1.1 Derivation process for luma motion vectors for skipped macroblocks in P and SP slices
void	hl_codec_264_utils_derivation_process_for_luma_movect_for_skipped_mb_in_p_and_sp_slices(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    struct hl_codec_264_mv_xs *mvL0,
    int32_t* refIdxL0);

// 8.4.1.3 Derivation process for luma motion vector prediction
// FIXME: used in ME: must be really optimized
HL_ERROR_T hl_codec_264_utils_derivation_process_for_luma_movect_prediction(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    int32_t refIdxLX,
    enum HL_CODEC_264_SUBMB_TYPE_E currSubMbType,
    struct hl_codec_264_mv_xs *mvpLX,
    enum HL_CODEC_264_LIST_IDX_E listSuffix);

// 8.4.1.3.1 Derivation process for median luma motion vector prediction
void	hl_codec_264_utils_derivation_process_for_median_luma_movect_prediction(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t refIdxLX,//currPartition
    int32_t mbAddrA, int32_t mbPartIdxA, int32_t subMbPartIdxA, struct hl_codec_264_mv_xs mvLXA, int32_t refIdxLXA,
    int32_t mbAddrB, int32_t mbPartIdxB, int32_t subMbPartIdxB, struct hl_codec_264_mv_xs mvLXB, int32_t refIdxLXB,
    int32_t mbAddrC, int32_t mbPartIdxC, int32_t subMbPartIdxC, struct hl_codec_264_mv_xs mvLXC, int32_t refIdxLXC,
    HL_OUT struct hl_codec_264_mv_xs *mvpLX);

// 8.4.1.4 Derivation process for chroma motion vectors
HL_ERROR_T hl_codec_264_utils_derivation_process_for_chroma_movects(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s* p_mb,
    HL_IN const struct hl_codec_264_mv_xs *mvLX,//luma motion vector
    HL_OUT struct hl_codec_264_mv_xs *mvCLX, // chroma motion vector
    enum HL_CODEC_264_LIST_IDX_E listSuffix);

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
    enum HL_CODEC_264_LIST_IDX_E listSuffix);

// FIXME: MACRO
// G.6.1 Derivation process for reference layer macroblocks
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_mb_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t xP, int32_t yP, // input luma location
    int32_t fieldMbFlag,
    int32_t* mbAddrRefLayer,
    int32_t* xB, int32_t* yB
);
// FIXME: macro
// G.6.2 Derivation process for reference layer partitions
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_partitions_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t xP, int32_t yP, // input luma location
    int32_t *mbAddrRefLayer,
    int32_t *mbPartIdxRefLayer,
    int32_t *subMbPartIdxRefLayer
);
// FIXME: macro
// G.6.3 Derivation process for reference layer sample locations in resampling
void	hl_codec_264_utils_derivation_process_for_ref_layer_sample_locs_in_resampling_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t chromaFlag, int32_t xP, int32_t yP, int32_t botFieldFlag,
    int32_t* xRef16, int32_t* yRef16
);
// FIXME: macro
// G.6.4 SVC derivation process for macroblock and sub-macroblock partition indices
void	hl_codec_264_utils_derivation_process_for_mb_and_submb_partition_indices_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t currDQId,
    int32_t xP, int32_t yP, // input luma location
    int32_t *mbPartIdx,
    int32_t *subMbPartIdx
);
// FIMXE: MACRO
// G.8.1.2.1 Array assignment and initialisation process
HL_ERROR_T	hl_codec_264_utils_array_assignment_and_initialisation_svc(
    struct hl_codec_264_s* pc_codec
);
// G.8.1.5.1 Macroblock initialisation process
HL_ERROR_T	hl_codec_264_utils_derivation_process_initialisation_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
);

// FIXME: MACRO
// G.8.4.1 SVC derivation process for motion vector components and reference indices
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_mv_comps_and_ref_indices_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
);
// FIXME: MACRO
// G.8.4.1.1 SVC derivation process for luma motion vector components and reference indices of a macroblock or sub-macroblock partition
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_luma_vectcomps_and_ref_indices_of_mb_and_submb_partition(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb,
    int32_t mbPartIdx,
    int32_t subMbPartIdx,
    int32_t isDirectFlag
);
// FIXME: MACRO
// G.8.6.1.1 Derivation process for reference layer partition identifications
// Outputs: "intraILPredFlag" and "refLayerPartId"
// JVSM: MotionUpsampling::xSetPartIdcArray()
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_partition_identifications_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
);
// FIXME: MACRO
// G.8.6.1.2 Derivation process for inter-layer predictors for reference indices and motion vectors
// Outputs: "refIdxILPredL0", "refIdxILPredL1", "mvILPredL0" and "mvILPredL1"
// JVSM: "MotionUpsampling::xGetRefIdxAndInitialMvPred()"
void	hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_ref_indices_and_mvs_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
);
// FIXME: MACRO
// G.8.6.1.3 Derivation process for inter-layer predictors for P and B macroblock and sub-macroblock types
// This process is only invoked when slice_type is equal to EP or EB.
// Outputs: "mbTypeILPred", "subMbTypeILPred"
void	hl_codec_264_utils_derivation_process_for_inter_layer_pred_for_P_and_B_mb_and_submb_types_svc(
    const struct hl_codec_264_s* pc_codec,
    struct hl_codec_264_mb_s *p_mb
);
// FIXME: MACRO
// G.8.6.2.2.1 Derivation process for reference layer slice and intra macroblock identifications
void	hl_codec_264_utils_derivation_process_for_ref_layer_slice_and_intra_mb_identifications_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t xRef, int32_t yRef, int32_t refMbW, int32_t refMbH,
    int32_t *refSliceIdc, int32_t *refIntraMbFlag
);
// G.8.6.2.2.2 Construction process for not available sample values prior to intra resampling
void	hl_codec_264_utils_derivation_process_for_not_avail_samples_prior_to_intra_resampling_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t refMbW, int32_t refMbH,
    int32_t refArrayW, int32_t refArrayH, int32_t* refSampleArray, const uint8_t* refSampleArrayAvailability,
    int32_t xOffset, int32_t yOffset
);

// FIXME: MACRO
// G.8.6.3.2.1 Derivation process for reference layer transform block identifications
HL_ERROR_T	hl_codec_264_utils_derivation_process_for_ref_layer_transform_block_identifications_svc(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s *pc_mb,
    int32_t xRef, int32_t yRef,
    int32_t chromaFlag,
    int32_t refMbW, int32_t refMbH,
    int32_t *refTransBlkIdc);

#endif /* _HARTALLO_CODEC_264_UTILS_H_ */
