#ifndef _HARTALLO_CODEC_264_MB_H_
#define _HARTALLO_CODEC_264_MB_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"
#include "hartallo/h264/hl_codec_264_tables.h"

HL_BEGIN_DECLS

struct hl_codec_264_mb_s;
struct hl_codec_264_s;

HL_ERROR_T hl_codec_264_mb_create(uint32_t u_addr, struct hl_codec_264_mb_s** pp_mb);
HL_ERROR_T hl_codec_264_mb_set_default_quant_values(struct hl_codec_264_mb_s* p_mb, const struct hl_codec_264_s* pc_codec);
HL_ERROR_T hl_codec_264_mb_set_mb_type(struct hl_codec_264_mb_s* p_mb, uint32_t mb_type, const struct hl_codec_264_s* pc_codec);
HL_ERROR_T hl_codec_264_mb_set_sub_mb_type(struct hl_codec_264_mb_s* p_mb, const uint32_t (* sub_mb_types)[4], const struct hl_codec_264_s* pc_codec);
HL_ERROR_T hl_codec_264_mb_set_sub_mb_type_when_not_present(struct hl_codec_264_mb_s* p_mb, const struct hl_codec_264_s* pc_codec);
void hl_codec_264_mb_get_neighbouring_partitions(
    const struct hl_codec_264_s* p_codec,
    const struct hl_codec_264_mb_s* self,
    int32_t mbPartIdx,
    enum HL_CODEC_264_SUBMB_TYPE_E currSubMbType,
    int32_t subMbPartIdx,
    hl_bool_t luma,
    int32_t* mbAddrA, int32_t* mbPartIdxA, int32_t* subMbPartIdxA,
    int32_t* mbAddrB, int32_t* mbPartIdxB, int32_t* subMbPartIdxB,
    int32_t* mbAddrC, int32_t* mbPartIdxC, int32_t* subMbPartIdxC,
    int32_t* mbAddrD, int32_t* mbPartIdxD, int32_t* subMbPartIdxD
);
HL_ERROR_T hl_codec_264_mb_encode_pcm(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_mb_encode_intra(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec, int32_t *pi_mad, int32_t* pi_bits_hdr_count, int32_t* pi_bits_data_count);
HL_ERROR_T hl_codec_264_mb_encode_inter(struct hl_codec_264_mb_s* p_mb, struct hl_codec_264_s* p_codec, int32_t *pi_mad, int32_t* pi_bits_hdr_count, int32_t* pi_bits_data_count);
HL_ERROR_T hl_codec_264_mb_print_samples(const struct hl_codec_264_mb_s* pc_mb, const hl_pixel_t *pc_samples, int32_t i_stride, int32_t i_line);
HL_ERROR_T hl_codec_264_mb_get_md5(const struct hl_codec_264_mb_s* pc_mb, const hl_pixel_t *pc_samples, int32_t i_stride, int32_t i_line, const char** ppc_md5);
HL_ERROR_T hl_codec_264_mb_print_md5(const struct hl_codec_264_mb_s* pc_mb, const hl_pixel_t *pc_samples, int32_t i_stride, int32_t i_line);


enum {
    HL_CODEC_264_MB_TYPE_FLAGS_NONE = 0,
    HL_CODEC_264_MB_TYPE_FLAGS_INTRA = (1 << 0),
    HL_CODEC_264_MB_TYPE_FLAGS_INTER = (1 << 1),
    HL_CODEC_264_MB_TYPE_FLAGS_SKIP = (1 << 2),
    HL_CODEC_264_MB_TYPE_FLAGS_PCM = (HL_CODEC_264_MB_TYPE_FLAGS_INTRA | (1 << 3)), // unuset
    HL_CODEC_264_MB_TYPE_FLAGS_INFERRED = (HL_CODEC_264_MB_TYPE_FLAGS_INTER | (1 << 4)),
    HL_CODEC_264_MB_TYPE_FLAGS_INTRA_BL = (HL_CODEC_264_MB_TYPE_FLAGS_INTRA | (1 << 5)), // unuset
    HL_CODEC_264_MB_TYPE_FLAGS_INTRA_8x8 = (HL_CODEC_264_MB_TYPE_FLAGS_INTRA | (1 << 6)), // unuset
    HL_CODEC_264_MB_TYPE_FLAGS_INTRA_4x4 = (HL_CODEC_264_MB_TYPE_FLAGS_INTRA | (1 << 7)), // unuset
    HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16 = (HL_CODEC_264_MB_TYPE_FLAGS_INTRA | (1 << 8)),
    HL_CODEC_264_MB_TYPE_FLAGS_INTER_8x8 = (HL_CODEC_264_MB_TYPE_FLAGS_INTER | (1 << 9)), // unuset
    HL_CODEC_264_MB_TYPE_FLAGS_INTER_P = (HL_CODEC_264_MB_TYPE_FLAGS_INTER | (1 << 10)),
    HL_CODEC_264_MB_TYPE_FLAGS_INTER_B = (HL_CODEC_264_MB_TYPE_FLAGS_INTER | (1 << 11)),
};

#define HL_CODEC_264_MB_TYPE_IS_SKIP(p_mb)								((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_SKIP)
#define HL_CODEC_264_MB_TYPE_IS_INTRA(p_mb)								((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_INTRA)
#define HL_CODEC_264_MB_TYPE_IS_INTER(p_mb)								((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_INTER)
#define HL_CODEC_264_MB_TYPE_IS_INFERRED(p_mb)							(((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_INFERRED) == HL_CODEC_264_MB_TYPE_FLAGS_INFERRED)
#define HL_CODEC_264_MB_TYPE_IS_INTRA_16x16(p_mb)						(((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16) == HL_CODEC_264_MB_TYPE_FLAGS_INTRA_16x16)
#define HL_CODEC_264_MB_TYPE_IS_PCM(p_mb)								(((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_PCM) == HL_CODEC_264_MB_TYPE_FLAGS_PCM)


#define HL_CODEC_264_MB_TYPE_IS_P_SKIP(p_mb)							((p_mb)->e_type == HL_CODEC_264_MB_TYPE_P_SKIP)
#define HL_CODEC_264_MB_TYPE_IS_P_16X16(p_mb)							((p_mb)->e_type == HL_CODEC_264_MB_TYPE_P_L0_16X16 || (p_mb)->e_type == HL_CODEC_264_MB_TYPE_B_L0_16X16 || (p_mb)->e_type == HL_CODEC_264_MB_TYPE_B_L1_16X16 || (p_mb)->e_type == HL_CODEC_264_MB_TYPE_B_BI_16X16)
#define HL_CODEC_264_MB_TYPE_IS_P_L0_16X16(p_mb)						((p_mb)->e_type == HL_CODEC_264_MB_TYPE_P_L0_16X16)
#define HL_CODEC_264_MB_TYPE_IS_P_8X8(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_P_8X8)
#define HL_CODEC_264_MB_TYPE_IS_P_8X8REF0(p_mb)							((p_mb)->e_type == HL_CODEC_264_MB_TYPE_P_8X8REF0)
#define HL_CODEC_264_MB_TYPE_IS_B_SKIP(p_mb)							((p_mb)->e_type == HL_CODEC_264_MB_TYPE_B_SKIP)
#define HL_CODEC_264_MB_TYPE_IS_B_DIRECT_16X16(p_mb)					((p_mb)->e_type == HL_CODEC_264_MB_TYPE_B_DIRECT_16X16)
#define HL_CODEC_264_MB_TYPE_IS_B_8X8(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_B_8X8)
#define HL_CODEC_264_MB_TYPE_IS_I_NxN(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_I_NXN)
#define HL_CODEC_264_MB_TYPE_IS_I_16X16(p_mb)							((p_mb)->e_type == HL_CODEC_264_MB_TYPE_SVC_I_16X16 || ((p_mb)->e_type >= HL_CODEC_264_MB_TYPE_I_16X16_0_0_0 && (p_mb)->e_type <= HL_CODEC_264_MB_TYPE_I_16X16_3_2_1))
#define HL_CODEC_264_MB_TYPE_IS_I_8X8(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_SVC_I_8X8)
#define HL_CODEC_264_MB_TYPE_IS_I_4X4(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_SVC_I_4X4)
#define HL_CODEC_264_MB_TYPE_IS_I_BL(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_SVC_I_BL)
#define HL_CODEC_264_MB_TYPE_IS_I_PCM(p_mb)								((p_mb)->e_type == HL_CODEC_264_MB_TYPE_I_PCM)

#define HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, sub_part_idx)			((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_INTRA_16X16)
#define HL_CODEC_264_MB_MODE_IS_INTRA_4X4(p_mb, sub_part_idx)			((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_INTRA_4X4)
#define HL_CODEC_264_MB_MODE_IS_INTRA_8X8(p_mb, sub_part_idx)			((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_INTRA_8X8)
#define HL_CODEC_264_MB_MODE_IS_DIRECT(p_mb, sub_part_idx)				((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_DIRECT)
#define HL_CODEC_264_MB_MODE_IS_BIPRED(p_mb, sub_part_idx)				((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_BIPRED)
#define HL_CODEC_264_MB_MODE_IS_PRED_L0(p_mb, sub_part_idx)				((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_PRED_L0)
#define HL_CODEC_264_MB_MODE_IS_PRED_L1(p_mb, sub_part_idx)				((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_PRED_L1)
#define HL_CODEC_264_MB_MODE_IS_INTRA_BL(p_mb, sub_part_idx)			((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_INTRA_BL)
#define HL_CODEC_264_MB_MODE_IS_INTER_BL(p_mb, sub_part_idx)			((p_mb)->MbPartPredMode[(sub_part_idx)] == HL_CODEC_264_MB_MODE_INTER_BL)

#define HL_CODEC_264_SUBMB_TYPE_IS_B_DIRECT_8X8(p_mb, sub_part_idx)		((p_mb)->SubMbPredType[(sub_part_idx)] == HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8)

#define HL_CODEC_264_SUBMB_MODE_IS_DIRECT(p_mb, sub_part_idx)			((p_mb)->SubMbPredMode[(sub_part_idx)] == HL_CODEC_264_SUBMB_MODE_DIRECT)
#define HL_CODEC_264_SUBMB_MODE_IS_PRED_L0(p_mb, sub_part_idx)			((p_mb)->SubMbPredMode[(sub_part_idx)] == HL_CODEC_264_SUBMB_MODE_PRED_L0)
#define HL_CODEC_264_SUBMB_MODE_IS_PRED_L1(p_mb, sub_part_idx)			((p_mb)->SubMbPredMode[(sub_part_idx)] == HL_CODEC_264_SUBMB_MODE_PRED_L1)
#define HL_CODEC_264_SUBMB_MODE_IS_BIPRED(p_mb, sub_part_idx)			((p_mb)->SubMbPredMode[(sub_part_idx)] == HL_CODEC_264_SUBMB_MODE_BIPRED)

#define HL_CODEC_264_MB_TYPE_IS_INTER_P(p_mb)							(((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_INTER_P) == HL_CODEC_264_MB_TYPE_FLAGS_INTER_P)
#define HL_CODEC_264_MB_TYPE_IS_INTER_B(p_mb)							(((p_mb)->flags_type & HL_CODEC_264_MB_TYPE_FLAGS_INTER_B) == HL_CODEC_264_MB_TYPE_FLAGS_INTER_B)

typedef struct hl_codec_264_mb_s {
    HL_DECLARE_OBJECT;

    uint32_t u_addr;
    long l_slice_id; // slice's unique id
    hl_size_t u_slice_idx; // slice's idx: must not used for comparaison beacuse of recycling
    uint32_t u_x;
    uint32_t u_y;
    int32_t xL;
    int32_t yL;
    int32_t xC;
    int32_t yC;
    int32_t xL_Idx;
    int32_t yL_Idx;
    enum HL_CODEC_264_MB_TYPE_E e_type;
    enum HL_CODEC_264_MB_MODE_E MbPartPredMode[4/*mbPartIdx*/];
    hl_mb_type_flags_t flags_type; // use "HL_CODEC_264_MB_TYPE_FLAGS_XXX"

    struct {
        int32_t i_addr_A;
        hl_bool_t b_avail_A;
        int32_t i_addr_B;
        hl_bool_t b_avail_B;
        int32_t i_addr_C;
        hl_bool_t b_avail_C;
        int32_t i_addr_D;
        hl_bool_t b_avail_D;
    } neighbours;

    int32_t prev_intra4x4_pred_mode_flag[16];
    int32_t prev_intra8x8_pred_mode_flag[4];
    int32_t rem_intra8x8_pred_mode[4];
    int32_t rem_intra4x4_pred_mode[16];
    int32_t noSubMbPartSizeLessThan8x8Flag;
    int32_t TransformBypassModeFlag;
    uint32_t mb_type; // ue(v)
    uint32_t sub_mb_type[4/*mbPartIdx*/]; // ue(v)
    uint32_t mb_field_decoding_flag; // u(1)
    int32_t mb_skip_flag; // u(1)
    int32_t mb_qp_delta;
    int32_t transform_size_8x8_flag; // u(1)
    int32_t QPy;
    int32_t QSy;
    int32_t QPyprime;//QPy'
    int32_t qPOffset[2]; //0=Cb/1=Cr
    int32_t QPprimeC[2];//QP'C 0=Cb/1=Cr
    int32_t QPc[2]; //0=Cb/1=Cr
    int32_t QSc[2]; //0=Cb/1=Cr

    uint32_t coded_block_pattern; // se(v) or me(v), depends on entropy coding type
    uint32_t CodedBlockPatternLuma;
    uint32_t CodedBlockPatternChroma;
    uint32_t CodedBlockPatternLuma4x4;
    uint32_t CodedBlockPatternChromaDC4x4[2/*0=Cb/1=Cr*/];
    uint32_t CodedBlockPatternChromaAC4x4[2/*0=Cb/1=Cr*/];

    // == Intra ==//
    enum HL_CODEC_264_INTRA_CHROMA_MODE_E intra_chroma_pred_mode; // "intra_chroma_pred_mode", ue(v)
    enum HL_CODEC_264_I16x16_MODE_E Intra16x16PredMode;
    enum HL_CODEC_264_I8x8_MODE_E Intra8x8PredMode[4];
    enum HL_CODEC_264_I4x4_MODE_E Intra4x4PredMode[16];
    // ==       ==//

    //== Inter ==//
    int32_t ref_idx_l0[4/*blk8x8*/];
    int32_t ref_idx_l1[4/*blk8x8*/];
    int32_t RefIdxL0[4/*mbPartIdx*/];
    int32_t RefIdxL1[4/*mbPartIdx*/];
    int32_t refIdxL0[4/*mbPartIdx*/];
    int32_t refIdxL1[4/*mbPartIdx*/];
    int32_t PredFlagL0[4/*mbPartIdx*/];
    int32_t PredFlagL1[4/*mbPartIdx*/];
    int32_t predFlagL0[4/*mbPartIdx*/];
    int32_t predFlagL1[4/*mbPartIdx*/];
    int32_t subMvCnt;
    int32_t MvCnt;
    struct hl_codec_264_mv_xs mvL0[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs mvL1[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs mvCL0[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs mvCL1[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs mvd_l0[4]/*mbPartIdx*/[4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs mvd_l1[4/*mbPartIdx*/][4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs MvL0[4/*mbPartIdx*/][4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs MvL1[4/*mbPartIdx*/][4/*subMbPartIdx*/];
    struct hl_codec_264_mv_xs MvCL0[4/*mbPartIdx*/][4/**subMbPartIdx*/];
    struct hl_codec_264_mv_xs MvCL1[4/*mbPartIdx*/][4/**subMbPartIdx*/];
    enum HL_CODEC_264_SUBMB_TYPE_E SubMbPredType[4/*mbPartIdx*/];
    enum HL_CODEC_264_SUBMB_MODE_E SubMbPredMode[4/*mbPartIdx*/];
    int32_t NumMbPart;
    int32_t NumSubMbPart[4/*mbPartIdx*/];
    int32_t SubMbPartWidth[4/*mbPartIdx*/];
    int32_t SubMbPartHeight[4/*mbPartIdx*/];
    int32_t partWidth[4/*mbPartIdx*/][4/**subMbPartIdx*/];
    int32_t partHeight[4/*mbPartIdx*/][4/**subMbPartIdx*/];
    int32_t partWidthC[4/*mbPartIdx*/][4/**subMbPartIdx*/];
    int32_t partHeightC[4/*mbPartIdx*/][4/**subMbPartIdx*/];
    int32_t MbPartWidth;//MbPartWidth( mb_type )
    int32_t MbPartHeight;//MbPartHeight( mb_type )

    //==		==//

    //== CAVLC ==//
    int32_t TotalCoeffsLuma[HL_CODEC_264_BLKS_PER_MB_MAX_COUNT]; // TotalCoeff(coeff_token) for Y (AC only?)
    int32_t TotalCoeffsChromaACCbCr[2/*0=Cb/1=Cr*/][HL_CODEC_264_BLKS_PER_MB_MAX_COUNT]; // TotalCoeff(coeff_token) for Cb or Cr
    int32_t TotalCoeffsChromaDCCbCr[2/*0=Cb/1=Cr*/][HL_CODEC_264_BLKS_PER_MB_MAX_COUNT]; // TotalCoeff(coeff_token) for Cb or Cr
    //==     ==//

    HL_ALIGN(HL_ALIGN_V) int32_t ChromaDCLevel[2/*iCbCr*/][16/*DC coeff val*/];
    HL_ALIGN(HL_ALIGN_V) int32_t ChromaACLevel[2/*iCbCr*/][4/*chroma4x4BlkIdx*/][16/*AC coeff val*/];
    HL_ALIGN(HL_ALIGN_V) int32_t Intra16x16DCLevel[16/*luma4x4BlkIdx*/];
    HL_ALIGN(HL_ALIGN_V) int32_t Intra16x16ACLevel[16/*luma4x4BlkIdx*/][16];
    HL_ALIGN(HL_ALIGN_V) int32_t LumaLevel[16/*luma4x4BlkIdx*/][16];
    HL_ALIGN(HL_ALIGN_V) int32_t LumaLevel8x8[8][64];

    //== Encoding ==//
    int32_t i_activity; // FIXME: remove (see JVT-O079 to know what should be used)
    //==		==//

    //== Deblocking ==//
    struct {
        int32_t fieldMbInFrameFlag;
        int32_t filterInternalEdgesFlag;
        int32_t filterLeftMbEdgeFlag;
        int32_t filterTopMbEdgeFlag;
    } deblock;

    // MVC and SVC extensions
    struct {
        struct {
            uint32_t base_mode_flag; // u(1)
            uint32_t residual_prediction_flag;  // u(1)
            int32_t motion_prediction_flag_lX[2/*0=list0, 1=list1*/][4/*mbPartIdx*/];
            hl_bool_t InCropWindow;
            int32_t sliceIdc;
            int32_t baseModeFlag;
            int32_t fieldMbFlag;
            int32_t intraILPredFlag;
            int32_t mvCnt;
            int32_t tQPy;
            int32_t tQPCb;
            int32_t tQPCr;
            enum HL_CODEC_264_I4x4_MODE_E ipred4x4[16];
            enum HL_CODEC_264_I8x8_MODE_E ipred8x8[4];
            enum HL_CODEC_264_I16x16_MODE_E ipred16x16;
            enum HL_CODEC_264_INTRA_CHROMA_MODE_E ipredChroma;
            int32_t tCoeffLevel[768/* 256 + 2 * MbWidthC * MbHeightC) */];
            int32_t sTCoeff[768/* 256 + 2 * MbWidthC * MbHeightC) */];
            int32_t predTrafoFlag;
            enum HL_CODEC_264_MB_TYPE_E mbTypeILPred;
            int32_t mb_type_il_pred; // "Hartallo-specific"
            enum HL_CODEC_264_SUBMB_TYPE_E subMbTypeILPred[4/*mbPartIdx*/];
            int32_t sub_mb_type_il_pred[4/*mbPartIdx*/]; // "Hartallo-specific"
            enum HL_CODEC_264_TRANSFORM_TYPE_E cTrafo;
            HL_ALIGN(HL_ALIGN_V) int32_t refIdxILPredL0[2][2];
            HL_ALIGN(HL_ALIGN_V) int32_t refIdxILPredL1[2][2];
            HL_ALIGN(HL_ALIGN_V) int32_t mvILPredL0[4][4][2];
            HL_ALIGN(HL_ALIGN_V) int32_t mvILPredL1[4][4][2];
            HL_ALIGN(HL_ALIGN_V) int32_t refLayerPartIdc[4/*y*/][4/*x*/]; // reference layer partition identifications for the current macroblock
        } svc;
        struct {
            unsigned NOT_IMPLEMENTED: 1;
        } mvc;
    }
    ext;

    // Used to store result from "6.4.10.4 Derivation process for neighbouring 4x4 luma blocks"
    hl_codec_264_neighbouring_luma_block4x4_xt neighbouringLumaBlock4x4[16 /* luma4x4BlkIdx */];
    // Used to store result from "6.4.10.5 Derivation process for neighbouring 4x4 chroma blocks"
    hl_codec_264_neighbouring_chroma_block4x4_xt neighbouringChromaBlock4x4[16 /* chroma4x4BlkIdx */];
}
hl_codec_264_mb_t;


// 6.4.2.1 Inverse macroblock partition scanning process
#define hl_codec_264_mb_inverse_partion_scan( \
		/* const hl_codec_264_mb_t**/ _pc_mb_, \
		/*int32_t*/ _mbPartIdx_, \
		/*int32_t **/_x_, \
		/*int32_t **/_y_ \
) \
{ \
	*(_x_) = InverseRasterScan_Pow2Full((_mbPartIdx_), (_pc_mb_)->MbPartWidth, (_pc_mb_)->MbPartHeight, 16, 0);/* (6-11) */ \
	*(_y_) = InverseRasterScan_Pow2Full((_mbPartIdx_), (_pc_mb_)->MbPartWidth, (_pc_mb_)->MbPartHeight, 16, 1);/* (6-12) */  \
}

// 6.4.2.2 Inverse sub-macroblock partition scanning process
#define hl_codec_264_mb_inverse_sub_partion_scan( \
	/* const hl_codec_264_mb_t**/ _pc_mb_, \
	/*int32_t*/ _mbPartIdx_, \
	/*int32_t*/ _subMbPartIdx_,  \
	/*int32_t **/_x_, \
	/*int32_t **/_y_ \
	) \
{ \
	switch((_pc_mb_)->e_type) \
	{ \
		case HL_CODEC_264_MB_TYPE_P_8X8: \
		case HL_CODEC_264_MB_TYPE_P_8X8REF0: \
		case HL_CODEC_264_MB_TYPE_B_8X8: \
			{ \
				*(_x_) = InverseRasterScan_Pow2Full((_subMbPartIdx_), (_pc_mb_)->SubMbPartWidth[(_mbPartIdx_)], (_pc_mb_)->SubMbPartHeight[(_mbPartIdx_)], 8, 0 ); /* (6-13) */ \
				*(_y_) = InverseRasterScan_Pow2Full((_subMbPartIdx_), (_pc_mb_)->SubMbPartWidth[(_mbPartIdx_)], (_pc_mb_)->SubMbPartHeight[(_mbPartIdx_)], 8, 1 ); /* (6-14) */ \
				break; \
			} \
		default: \
			{ \
				*(_x_) = InverseRasterScan16_4x4[(_subMbPartIdx_)][8][0];/* (6-15) */ \
				*(_y_) = InverseRasterScan16_4x4[(_subMbPartIdx_)][8][1];/* (6-16) */ \
				break; \
			} \
	} \
}

// 6.4.12.4 Derivation process for macroblock and sub-macroblock partition indices
#define hl_codec_264_mb_get_sub_partition_indices( \
	/* const hl_codec_264_mb_t**/ _pc_mb_, \
	/*int32_t*/ _xP_,  \
	/*int32_t*/ _yP_,  \
	/*int32_t**/ _mbPartIdx_,  \
	/*int32_t**/ _subMbPartIdx_ \
	) \
{ \
	if (HL_CODEC_264_MB_TYPE_IS_INTRA((_pc_mb_))){ \
		*(_mbPartIdx_) = 0; \
	} \
	else { \
		*(_mbPartIdx_) = (16 / (_pc_mb_)->MbPartWidth) * ((_yP_) / (_pc_mb_)->MbPartHeight) + ((_xP_) / (_pc_mb_)->MbPartWidth);/* (6-41) */ \
	} \
 \
	if ((_pc_mb_)->e_type != HL_CODEC_264_MB_TYPE_P_8X8 && (_pc_mb_)->e_type != HL_CODEC_264_MB_TYPE_P_8X8REF0 && (_pc_mb_)->e_type != HL_CODEC_264_MB_TYPE_B_8X8 && \
		(_pc_mb_)->e_type != HL_CODEC_264_MB_TYPE_B_SKIP && (_pc_mb_)->e_type != HL_CODEC_264_MB_TYPE_B_DIRECT_16X16) \
	{ \
		*(_subMbPartIdx_) = 0; \
	} \
	else if ((_pc_mb_)->e_type == HL_CODEC_264_MB_TYPE_B_SKIP || (_pc_mb_)->e_type == HL_CODEC_264_MB_TYPE_B_DIRECT_16X16){ \
		*(_subMbPartIdx_) = ((HL_MATH_MOD_POW2_INT32((_yP_), 8) >> 2) << 1) + (HL_MATH_MOD_POW2_INT32((_xP_), 8) >> 2);/* (6-42) */ \
	} \
	else { \
		*(_subMbPartIdx_) = (8 / (_pc_mb_)->SubMbPartWidth[*(_mbPartIdx_)]) * (HL_MATH_MOD_POW2_INT32((_yP_), 8) / (_pc_mb_)->SubMbPartHeight[*(_mbPartIdx_)]) + (HL_MATH_MOD_POW2_INT32((_xP_), 8) / (_pc_mb_)->SubMbPartWidth[*(_mbPartIdx_)]);/* (6-43) */ \
	} \
}

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_MB_H_ */
