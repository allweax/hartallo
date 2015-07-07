#ifndef _HARTALLO_CODEC_264_DEFS_H_
#define _HARTALLO_CODEC_264_DEFS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_CODEC_264_NA									-1
#define HL_CODEC_264_SAMPLE_NA							-0xFF
#define HL_CODEC_264_SVC_SAMPLE_NA_INTRA_BASE			-0xFFFF00 // "not available for Intra_Base prediction"

#define HL_CODEC_264_SLICES_MAX_COUNT					16 // Max number of slices per frame
#define HL_CODEC_264_SLICEHDR_MAX_BYTES_COUNT			4096 // Both slice and MBs headers. High becaue of PCM.
#define HL_CODEC_264_SPS_MAX_BYTES_COUNT				2048
#define HL_CODEC_264_SPS_MAX_COUNT						32		/* SPS NALs */
#define HL_CODEC_264_PPS_MAX_COUNT						256		/* PPS NALs */
#define HL_CODEC_264_PPS_MAX_BYTES_COUNT				2048
#define HL_CODEC_264_SVC_LAYERS_MAX_COUNT					8 // SVC
#define HL_CODEC_264_SVC_TEMP_LEVELS_MAX_COUNT				8 // SVC
#define HL_CODEC_264_SVC_QUALITY_LEVELS_MAX_COUNT			16 // SVC
#define HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT		1024 // syntax "vui_ext_num_entries_minus1"
#define HL_CODEC_264_NUM_REF_IDX_L0_ACTIVE_MINUS1_MAX_COUNT	0x40		// syntax "num_ref_idx_l0_active_minus1"
#define HL_CODEC_264_NUM_REF_IDX_L1_ACTIVE_MINUS1_MAX_COUNT	0x40		// syntax "num_ref_idx_l1_active_minus1"
#define HL_CODEC_264_MAX_NUM_REF_FRAMES_MAX_VAL				16 // syntax "max_num_ref_frames". From 0 to 16 inclusive
#define HL_CODEC_264_MAX_NUM_REF_FRAMES_MAX_COUNT			(HL_CODEC_264_MAX_NUM_REF_FRAMES_MAX_VAL + 1)
#define HL_CODEC_264_NUM_SLICE_GROUPS_MINUS1_MAX_VAL	8 // syntax "num_slice_groups_minus1"
#define HL_CODEC_264_FS_MAX_COUNT						16 // max number of DPBFS in the list
#define HL_CODEC_264_LEVEL_MAX_COUNT					16 // Table A-1 – Level limits: "1", "1b"... "5.1"

#define HL_CODEC_264_SVC_MB_TYPE_INFERRED				-2 // "Mb_Inferred" -> G.7.4.6 Macroblock layer in scalable extension semantics

#define HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT			(HL_CODEC_264_SVC_LAYERS_MAX_COUNT * HL_CODEC_264_SVC_TEMP_LEVELS_MAX_COUNT * HL_CODEC_264_SVC_QUALITY_LEVELS_MAX_COUNT)// SVC

#define HL_CODEC_264_REFPICT_LIST0_MAX_COUNT			32		/* RefPicList0 for P and SP slices */
#define HL_CODEC_264_REFPICT_LIST1_MAX_COUNT			32		/* RefPicList1 for B slices */

#define HL_CODEC_264_DPB_REFPICT_MAX_COUNT				(HL_CODEC_264_REFPICT_LIST0_MAX_COUNT + HL_CODEC_264_REFPICT_LIST1_MAX_COUNT)		/* Max number of reference pictures to store in the DPB */
#define HL_CODEC_264_DPB_FRAME_DATA_MAX_COUNT			(HL_CODEC_264_DPB_REFPICT_MAX_COUNT + 1) /* Extra "1" is for overflow */

#define HL_CODEC_264_MB_ADDR_NOT_AVAIL					-0xFFFF /* Invalid macroblock address used to signal inv. addr */
#define HL_CODEC_264_BLK_ADDR_NOT_AVAIL					HL_CODEC_264_MB_ADDR_NOT_AVAIL
#define HL_CODEC_264_MB_PART_IDX_NOT_AVAIL				-0xFFFF
#define HL_CODEC_264_MB_DISTORTION_PSKIP16x16_THRESHOLD	256
#define HL_CODEC_264_MB_DISTORTION_PSKIP16x8_THRESHOLD	128
#define HL_CODEC_264_MB_DISTORTION_PSKIP8x16_THRESHOLD	128
#define HL_CODEC_264_MB_DISTORTION_PSKIP8x8_THRESHOLD	64
#define HL_CODEC_264_MB_DISTORTION_PSKIP8x4_THRESHOLD	32
#define HL_CODEC_264_MB_DISTORTION_PSKIP4x8_THRESHOLD	32
#define HL_CODEC_264_MB_DISTORTION_PSKIP4x4_THRESHOLD	16
#define HL_CODEC_264_SUBMB_PART_IDX_NOT_AVAIL			-0xFFFF
#define HL_CODEC_264_ACTIVITY_SMOOTHNESS_THRESHOLD		100 /* (mbActitvity < x) -> smooth area -> use lower qp */
#define HL_CODEC_264_RDO_BUFFER_MAX_SIZE				2048
#define HL_CODEC_264_RDO_LAMBDA_FACT_ALL					0.852
#define HL_CODEC_264_RDO_LAMBDA_FACT_I_SAD					0.65
#define HL_CODEC_264_RDO_LAMBDA_FACT_P_SAD					0.68
#define HL_CODEC_264_RDO_LAMBDA_FACT_B_SAD					0.68
#define HL_CODEC_264_RDO_LAMBDA_FACT_I_SSD					0.852
#define HL_CODEC_264_RDO_LAMBDA_FACT_P_SSD					0.852
#define HL_CODEC_264_RDO_LAMBDA_FACT_B_SSD					0.852
#define HL_CODEC_264_RDO_HOMOGENEOUSITY_TH16X16				20000 /* threshold to detect homogeneousity on 16x16 blocks */
#define HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X8				5000 /* threshold to detect homogeneousity on 8x8 blocks */
#define HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X4				7500 /* threshold to detect homogeneousity on 8x4 blocks */
#define HL_CODEC_264_RDO_HOMOGENEOUSITY_TH4X8				HL_CODEC_264_RDO_HOMOGENEOUSITY_TH8X4 /* threshold to detect homogeneousity on 4x8 blocks */

#define HL_CODEC_264_QP_MIN									0
#define HL_CODEC_264_QP_MAX									51
#define HL_CODEC_264_QP_SHIFT								12

#define HL_CODEC_264_RC_GOP_WINDOW_SIZE_MAX				15 // Max number of GOPs to use to compute the QP for the Rate Control (RC). max("i") when you refer to JVT-O079.

#define HL_CODEC_264_ME_RANGE_MIN						1 // Min Range value
#define HL_CODEC_264_ME_RANGE_MAX						64 // Max Range value

#define HL_CODEC_264_BLKS_PER_MB_MAX_COUNT				16 /* Maximum number of blocks per macroblock (16 4x4 blocks) */
#define HL_CODEC_264_COMP_CHROMA_IDX_MAX_COUNT			2 /* Number of chroma components: Cb, Cr */
#define HL_CODEC_264_COMP_CHROMA_IDX_U					0 /* Cb */
#define HL_CODEC_264_COMP_CHROMA_IDX_V					1 /* Cr */

#define HL_CODEC_264_CABAC_TABLE_SIZE					0x420 /* Maximum number of ctxIdx cabac values supported */

#define HL_CODEC_264_SAMPLE_NOT_AVAIL					0xFFFF0000

#define HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT 64

#define HL_CODEC_264_EMULATION_PREVENTION_THREE_BYTE		0x03		// syntax "emulation_prevention_three_byte"

typedef enum HL_CODEC_264_INPUT_TYPE_E {
    HL_CODEC_264_INPUT_TYPE_NONE,
    HL_CODEC_264_INPUT_TYPE_ENCODE,
    HL_CODEC_264_INPUT_TYPE_DECODE
} HL_CODEC_264_INPUT_TYPE_T;

typedef enum HL_CODEC_264_PROFILE_E {
    HL_CODEC_264_PROFILE_BASELINE  = 66,
    HL_CODEC_264_PROFILE_MAIN      = 77,
    HL_CODEC_264_PROFILE_EXTENDED  = 88,

    HL_CODEC_264_PROFILE_HIGH      = 100,
    HL_CODEC_264_PROFILE_HIGH10   = 110,
    HL_CODEC_264_PROFILE_HIGH422  = 122,
    HL_CODEC_264_PROFILE_HIGH444  = 144,
    HL_CODEC_264_PROFILE_CAVLC444 = 244,

    HL_CODEC_264_PROFILE_BASELINE_SVC = 83,
    HL_CODEC_264_PROFILE_HIGH_SVC     = 86
}
HL_CODEC_264_PROFILE_T;

// Table A-1 – Level limits
typedef enum HL_CODEC_264_LEVEL_E {
    HL_CODEC_264_LEVEL_10 = 10,
    HL_CODEC_264_LEVEL_1B = 9,
    HL_CODEC_264_LEVEL_11 = 11,
    HL_CODEC_264_LEVEL_12 = 12,
    HL_CODEC_264_LEVEL_13 = 13,
    HL_CODEC_264_LEVEL_20 = 20,
    HL_CODEC_264_LEVEL_21 = 21,
    HL_CODEC_264_LEVEL_22 = 22,
    HL_CODEC_264_LEVEL_30 = 30,
    HL_CODEC_264_LEVEL_31 = 31,
    HL_CODEC_264_LEVEL_32 = 32,
    HL_CODEC_264_LEVEL_40 = 40,
    HL_CODEC_264_LEVEL_41 = 41,
    HL_CODEC_264_LEVEL_42 = 42,
    HL_CODEC_264_LEVEL_50 = 50,
    HL_CODEC_264_LEVEL_51 = 51,
}
HL_CODEC_264_LEVEL_T;

typedef enum HL_CODEC_264_PICTURE_TYPE_E {
    HL_CODEC_264_PICTURE_TYPE_NONE,
    HL_CODEC_264_PICTURE_TYPE_FIELD_TOP,
    HL_CODEC_264_PICTURE_TYPE_FIELD_BOTTOM,
    HL_CODEC_264_PICTURE_TYPE_FRAME,

    HL_CODEC_264_PICTURE_TYPE_MAX_COUNT,
}
HL_CODEC_264_PICTURE_TYPE_T;

typedef enum HL_CODEC_264_LIST_IDX_E {
    HL_CODEC_264_LIST_IDX_0,
    HL_CODEC_264_LIST_IDX_1,
    HL_CODEC_264_LIST_IDX_MAX_COUNT
}
HL_CODEC_264_LIST_IDX_T;
#define listSuffixFlag_0 HL_CODEC_264_LIST_IDX_0
#define listSuffixFlag_1 HL_CODEC_264_LIST_IDX_1

typedef enum HL_CODEC_264_REF_TYPE_E {
    HL_CODEC_264_REF_TYPE_UNUSED = 0X00,

    HL_CODEC_264_REF_TYPE_TOP_USED_FOR_LONG_TERM = (0X01 << 0),
    HL_CODEC_264_REF_TYPE_BOTTOM_USED_FOR_LONG_TERM = (0X01 << 1),
    HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM = (HL_CODEC_264_REF_TYPE_TOP_USED_FOR_LONG_TERM | HL_CODEC_264_REF_TYPE_BOTTOM_USED_FOR_LONG_TERM),

    HL_CODEC_264_REF_TYPE_TOP_USED_FOR_SHORT_TERM = (0X01 << 2),
    HL_CODEC_264_REF_TYPE_BOTTOM_USED_FOR_SHORT_TERM = (0X01 << 3),
    HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM = (HL_CODEC_264_REF_TYPE_TOP_USED_FOR_SHORT_TERM | HL_CODEC_264_REF_TYPE_BOTTOM_USED_FOR_SHORT_TERM),

    HL_CODEC_264_REF_TYPE_USED_FOR_SVC_BASE_REF = (0X01 << 4), // SVC: "reference base picture"

    HL_CODEC_264_REF_TYPE_TOP_USED = (HL_CODEC_264_REF_TYPE_TOP_USED_FOR_LONG_TERM | HL_CODEC_264_REF_TYPE_TOP_USED_FOR_SHORT_TERM),
    HL_CODEC_264_REF_TYPE_BOTTOM_USED = (HL_CODEC_264_REF_TYPE_BOTTOM_USED_FOR_LONG_TERM | HL_CODEC_264_REF_TYPE_BOTTOM_USED_FOR_SHORT_TERM),
    HL_CODEC_264_REF_TYPE_USED = (HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM | HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM | HL_CODEC_264_REF_TYPE_USED_FOR_SVC_BASE_REF)
}
HL_CODEC_264_REF_TYPE_T;

typedef enum HL_CODEC_264_MB_INIT_TYPE_E {
    HL_CODEC_264_MB_INIT_NONE = 0x00,
    HL_CODEC_264_MB_INIT_NEIGHBOURING = (0x01 << 1),
    HL_CODEC_264_MB_INIT_INCROPWINDOW = (0x01 << 2),
    HL_CODEC_264_MB_INIT_ALL = 0xFF
}
HL_CODEC_264_MB_INIT_TYPE_T;

typedef enum HL_CODEC_264_RESISUAL_INV_TYPE_E {
    // Must be in this order because of Table 9-42
    // value = ctxBlockCat
    HL_CODEC_264_RESISUAL_INV_TYPE_NONE = -1,

    HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL = 0,
    HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL = 1,
    HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL = 2,
    HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL = 3,
    HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL = 4,
    HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL8X8 = 5,
    HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_DCLEVEL = 6,
    HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_ACLEVEL = 7,
    HL_CODEC_264_RESISUAL_INV_TYPE_CB_LEVEL = 8,
    HL_CODEC_264_RESISUAL_INV_TYPE_CB_LEVEL8X8 = 9,
    HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_DCLEVEL = 10,
    HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_ACLEVEL = 11,
    HL_CODEC_264_RESISUAL_INV_TYPE_CR_LEVEL = 12,
    HL_CODEC_264_RESISUAL_INV_TYPE_CR_LEVEL8X8 = 13
}
HL_CODEC_264_RESISUAL_INV_TYPE_T;

// values for SVS's "extended_spatial_scalability_idc" syntax element
typedef enum HL_CODEC_264_ESS_E {
    HL_CODEC_264_ESS_NONE,
    HL_CODEC_264_ESS_SEQ,
    HL_CODEC_264_ESS_PICT
}
HL_CODEC_264_ESS_T;


// Table 6-1
typedef enum HL_CODEC_264_CHROMAFORMAT_E {
    HL_CODEC_264_CHROMAFORMAT_MONOCHROME,
    HL_CODEC_264_CHROMAFORMAT_420,
    HL_CODEC_264_CHROMAFORMAT_422,
    HL_CODEC_264_CHROMAFORMAT_444,

    HL_CODEC_264_CHROMAFORMAT_MAX_COUNT
}
HL_CODEC_264_CHROMAFORMAT_T;

/**
* Table 7-1 – NAL unit type codes, syntax element categories, and NAL unit type classes
*/
typedef enum HL_CODEC_264_NAL_TYPE_E {
    HL_CODEC_264_NAL_TYPE_UNSPECIFIED_0 = 0,
    HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_A_NON_IDR_PICTURE = 1,
    HL_CODEC_264_NAL_TYPE_CODED_SLICE_DATA_PARTITION_A = 2,
    HL_CODEC_264_NAL_TYPE_CODED_SLICE_DATA_PARTITION_B = 3,
    HL_CODEC_264_NAL_TYPE_CODED_SLICE_DATA_PARTITION_C = 4,
    HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE = 5,
    HL_CODEC_264_NAL_TYPE_SEI = 6,
    HL_CODEC_264_NAL_TYPE_SPS = 7,
    HL_CODEC_264_NAL_TYPE_PPS = 8,
    HL_CODEC_264_NAL_TYPE_ACCESS_UNIT_DELIMITER = 9,
    HL_CODEC_264_NAL_TYPE_END_OF_SEQUENCE = 10,
    HL_CODEC_264_NAL_TYPE_END_OF_STREAM = 11,
    HL_CODEC_264_NAL_TYPE_FILLER_DATA = 12,
    HL_CODEC_264_NAL_TYPE_SEQUENCE_PARAMETER_SET_EXTENSION = 13,
    HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT = 14,
    HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET = 15,
    HL_CODEC_264_NAL_TYPE_RESERVED_16 = 16,
    HL_CODEC_264_NAL_TYPE_RESERVED_17 = 17,
    HL_CODEC_264_NAL_TYPE_RESERVED_18 = 17,
    HL_CODEC_264_NAL_TYPE_CSACPWP = 19, /* Coded slice of an auxiliary coded picture without partitioning */
    HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION = 20,
    HL_CODEC_264_NAL_TYPE_RESERVED_21 = 21,
    HL_CODEC_264_NAL_TYPE_RESERVED_22 = 22,
    HL_CODEC_264_NAL_TYPE_RESERVED_23 = 23,
    // 24..31 Unspecified
}
HL_CODEC_264_NAL_TYPE_T;

typedef enum HL_CODEC_264_SLICE_TYPE_E {
    HL_CODEC_264_SLICE_TYPE_NONE = -1,

    // Table 7-6 – Name association to slice_type
    HL_CODEC_264_SLICE_TYPE_P = 0,
    HL_CODEC_264_SLICE_TYPE_B = 1,
    HL_CODEC_264_SLICE_TYPE_I = 2,
    HL_CODEC_264_SLICE_TYPE_SP = 3,
    HL_CODEC_264_SLICE_TYPE_SI = 4,
    HL_CODEC_264_SLICE_TYPE_P_ = 5,
    HL_CODEC_264_SLICE_TYPE_B_ = 6,
    HL_CODEC_264_SLICE_TYPE_I_ = 7,
    HL_CODEC_264_SLICE_TYPE_SP_ = 8,
    HL_CODEC_264_SLICE_TYPE_SI_ = 9,

    // Table G-1 – Name association to slice_type for NAL units with nal_unit_type equal to 20.
    HL_CODEC_264_SLICE_TYPE_EP = HL_CODEC_264_SLICE_TYPE_P,
    HL_CODEC_264_SLICE_TYPE_EB = HL_CODEC_264_SLICE_TYPE_B,
    HL_CODEC_264_SLICE_TYPE_EI = HL_CODEC_264_SLICE_TYPE_I,
    HL_CODEC_264_SLICE_TYPE_EP_ = HL_CODEC_264_SLICE_TYPE_P_,
    HL_CODEC_264_SLICE_TYPE_EB_ = HL_CODEC_264_SLICE_TYPE_B_,
    HL_CODEC_264_SLICE_TYPE_EI_ = HL_CODEC_264_SLICE_TYPE_I_,
}
HL_CODEC_264_SLICE_TYPE_T;

/* Table 7-11 – Macroblock types for I slices ("mb_type") */
typedef enum HL_CODEC_264_MB_I_TYPE_E {
    I_NxN = 0, // Intra4x4
    I_16x16_0_0_0 = 1,
    I_16x16_1_0_0 = 2,
    I_16x16_2_0_0 = 3,
    I_16x16_3_0_0 = 4,
    I_16x16_0_1_0 = 5,
    I_16x16_1_1_0 = 6,
    I_16x16_2_1_0 = 7,
    I_16x16_3_1_0 = 8,
    I_16x16_0_2_0 = 9,
    I_16x16_1_2_0 = 10,
    I_16x16_2_2_0 = 11,
    I_16x16_3_2_0 = 12,
    I_16x16_0_0_1 = 13,
    I_16x16_1_0_1 = 14,
    I_16x16_2_0_1 = 15,
    I_16x16_3_0_1 = 16,
    I_16x16_0_1_1 = 17,
    I_16x16_1_1_1 = 18,
    I_16x16_2_1_1 = 19,
    I_16x16_3_1_1 = 20,
    I_16x16_0_2_1 = 21,
    I_16x16_1_2_1 = 22,
    I_16x16_2_2_1 = 23,
    I_16x16_3_2_1 = 24,
    I_PCM = 25,

    /* SVC */
    I_BL,
    I_4x4,
    I_8x8,
    I_16x16,

    HL_CODEC_264_MB_I_TYPE_MAX_COUNT,
}
HL_CODEC_264_MB_I_TYPE_T;

/* Table 7-12 – Macroblock type with value 0 for SI slices ("mb_type") */
typedef enum HL_CODEC_264_MB_SI_TYPE_E {
    SI/* = 0*/,
}
HL_CODEC_264_MB_SI_TYPE_T;

// Table 7-13 – Macroblock type values 0 to 4 for P and SP slices ("mb_type")
typedef enum HL_CODEC_264_MB_P_TYPE_E {
    P_L0_16x16 = 0,
    P_L0_L0_16x8 = 1,
    P_L0_L0_8x16 = 2,
    P_8x8 = 3,
    P_8x8ref0 = 4,

    HL_CODEC_264_MB_P_TYPE_MAX_COUNT,

    P_Skip/*inferred*/
}
HL_CODEC_264_MB_P_TYPE_T;

// Table 7-14 – Macroblock type values 0 to 22 for B slices ("mb_type")
typedef enum HL_CODEC_264_MB_B_TYPE_E {
    B_Direct_16x16 = 0,
    B_L0_16x16 = 1,
    B_L1_16x16 = 2,
    B_Bi_16x16 = 3,
    B_L0_L0_16x8 = 4,
    B_L0_L0_8x16 = 5,
    B_L1_L1_16x8 = 6,
    B_L1_L1_8x16 = 7,
    B_L0_L1_16x8 = 8,
    B_L0_L1_8x16 = 9,
    B_L1_L0_16x8 = 10,
    B_L1_L0_8x16 = 11,
    B_L0_Bi_16x8 = 12,
    B_L0_Bi_8x16 = 13,
    B_L1_Bi_16x8 = 14,
    B_L1_Bi_8x16 = 15,
    B_Bi_L0_16x8 = 16,
    B_Bi_L0_8x16 = 17,
    B_Bi_L1_16x8 = 18,
    B_Bi_L1_8x16 = 19,
    B_Bi_Bi_16x8 = 20,
    B_Bi_Bi_8x16 = 21,
    B_8x8 = 22,

    HL_CODEC_264_MB_B_TYPE_MAX_COUNT,

    B_Skip/*inferred*/
}
HL_CODEC_264_MB_B_TYPE_T;

// Table 7-17 – Sub-macroblock types in P macroblocks ("sub_mb_type")
typedef enum HL_CODEC_264_SUBMB_P_TYPE_E {
    P_L0_8x8 = 0,
    P_L0_8x4 = 1,
    P_L0_4x8 = 2,
    P_L0_4x4 = 3,

    HL_CODEC_264_SUBMB_P_TYPE_MAX_COUNT,
}
HL_CODEC_264_SUBMB_P_TYPE_T;

// Table 7-18 – Sub-macroblock types in B macroblocks ("sub_mb_type")
typedef enum HL_CODEC_264_SUBMB_B_TYPE_E {
    B_Direct_8x8,
    B_L0_8x8,
    B_L1_8x8,
    B_Bi_8x8,
    B_L0_8x4,
    B_L0_4x8,
    B_L1_8x4,
    B_L1_4x8,
    B_Bi_8x4,
    B_Bi_4x8,
    B_L0_4x4,
    B_L1_4x4,
    B_Bi_4x4,

    HL_CODEC_264_SUBMB_B_TYPE_E_MAX_COUNT
}
HL_CODEC_264_SUBMB_B_TYPE_T;

// Must be in this order: I, SI, P/SP, B
typedef enum HL_CODEC_264_MB_TYPE_E {
    /* I slices */
    HL_CODEC_264_MB_TYPE_START_SLICE_I = 100,
    HL_CODEC_264_MB_TYPE_I_NXN,
    HL_CODEC_264_MB_TYPE_I_16X16_0_0_0,
    HL_CODEC_264_MB_TYPE_I_16X16_1_0_0,
    HL_CODEC_264_MB_TYPE_I_16X16_2_0_0,
    HL_CODEC_264_MB_TYPE_I_16X16_3_0_0,
    HL_CODEC_264_MB_TYPE_I_16X16_0_1_0,
    HL_CODEC_264_MB_TYPE_I_16X16_1_1_0,
    HL_CODEC_264_MB_TYPE_I_16X16_2_1_0,
    HL_CODEC_264_MB_TYPE_I_16X16_3_1_0,
    HL_CODEC_264_MB_TYPE_I_16X16_0_2_0,
    HL_CODEC_264_MB_TYPE_I_16X16_1_2_0,
    HL_CODEC_264_MB_TYPE_I_16X16_2_2_0,
    HL_CODEC_264_MB_TYPE_I_16X16_3_2_0,
    HL_CODEC_264_MB_TYPE_I_16X16_0_0_1,
    HL_CODEC_264_MB_TYPE_I_16X16_1_0_1,
    HL_CODEC_264_MB_TYPE_I_16X16_2_0_1,
    HL_CODEC_264_MB_TYPE_I_16X16_3_0_1,
    HL_CODEC_264_MB_TYPE_I_16X16_0_1_1,
    HL_CODEC_264_MB_TYPE_I_16X16_1_1_1,
    HL_CODEC_264_MB_TYPE_I_16X16_2_1_1,
    HL_CODEC_264_MB_TYPE_I_16X16_3_1_1,
    HL_CODEC_264_MB_TYPE_I_16X16_0_2_1,
    HL_CODEC_264_MB_TYPE_I_16X16_1_2_1,
    HL_CODEC_264_MB_TYPE_I_16X16_2_2_1,
    HL_CODEC_264_MB_TYPE_I_16X16_3_2_1,
    HL_CODEC_264_MB_TYPE_I_PCM,
    HL_CODEC_264_MB_TYPE_END_SLICE_I,

    /* SI slices */
    HL_CODEC_264_MB_TYPE_START_SLICE_SI = 200,
    HL_CODEC_264_MB_TYPE_SI,
    HL_CODEC_264_MB_TYPE_END_SLICE_SI,

    /* P and SP slices */
    HL_CODEC_264_MB_TYPE_START_SLICE_P_AND_SP = 300,
    HL_CODEC_264_MB_TYPE_P_L0_16X16,
    HL_CODEC_264_MB_TYPE_P_L0_L0_16X8,
    HL_CODEC_264_MB_TYPE_P_L0_L0_8X16,
    HL_CODEC_264_MB_TYPE_P_8X8,
    HL_CODEC_264_MB_TYPE_P_8X8REF0,
    HL_CODEC_264_MB_TYPE_P_SKIP,/*inferred*/
    HL_CODEC_264_MB_TYPE_END_SLICE_P_AND_SP,

    /* B slices */
    HL_CODEC_264_MB_TYPE_START_SLICE_B = 400,
    HL_CODEC_264_MB_TYPE_B_DIRECT_16X16,
    HL_CODEC_264_MB_TYPE_B_L0_16X16,
    HL_CODEC_264_MB_TYPE_B_L1_16X16,
    HL_CODEC_264_MB_TYPE_B_BI_16X16,
    HL_CODEC_264_MB_TYPE_B_L0_L0_16X8,
    HL_CODEC_264_MB_TYPE_B_L0_L0_8X16,
    HL_CODEC_264_MB_TYPE_B_L1_L1_16X8,
    HL_CODEC_264_MB_TYPE_B_L1_L1_8X16,
    HL_CODEC_264_MB_TYPE_B_L0_L1_16X8,
    HL_CODEC_264_MB_TYPE_B_L0_L1_8X16,
    HL_CODEC_264_MB_TYPE_B_L1_L0_16X8,
    HL_CODEC_264_MB_TYPE_B_L1_L0_8X16,
    HL_CODEC_264_MB_TYPE_B_L0_BI_16X8,
    HL_CODEC_264_MB_TYPE_B_L0_BI_8X16,
    HL_CODEC_264_MB_TYPE_B_L1_BI_16X8,
    HL_CODEC_264_MB_TYPE_B_L1_BI_8X16,
    HL_CODEC_264_MB_TYPE_B_BI_L0_16X8,
    HL_CODEC_264_MB_TYPE_B_BI_L0_8X16,
    HL_CODEC_264_MB_TYPE_B_BI_L1_16X8,
    HL_CODEC_264_MB_TYPE_B_BI_L1_8X16,
    HL_CODEC_264_MB_TYPE_B_BI_BI_16X8,
    HL_CODEC_264_MB_TYPE_B_BI_BI_8X16,
    HL_CODEC_264_MB_TYPE_B_8X8,
    HL_CODEC_264_MB_TYPE_B_SKIP,/*inferred*/
    HL_CODEC_264_MB_TYPE_END_SLICE_B,

    /* "Mb_Inferred" for SVC: could be in any slice */
    HL_CODEC_264_MB_TYPE_SVC_I_BL, /* "Mb_Inferred" -> "G.7.4.6 Macroblock layer in scalable extension semantics" */
    HL_CODEC_264_MB_TYPE_SVC_I_4X4, // FIXME: used in SVC only???
    HL_CODEC_264_MB_TYPE_SVC_I_8X8, // FIXME: used in SVC only???
    HL_CODEC_264_MB_TYPE_SVC_I_16X16, // FIXME: used in SVC only???
}
HL_CODEC_264_MB_TYPE_T;

// Must be in this order: I/SI, P and B
typedef enum HL_CODEC_264_MB_MODE_E {
    HL_CODEC_264_MB_MODE_NA = HL_CODEC_264_NA,

    /* Table 7-11 – Macroblock types for I slices */
    HL_CODEC_264_MB_MODE_START_SLICE_I_AND_SI = 100,
    HL_CODEC_264_MB_MODE_INTRA_4X4,
    HL_CODEC_264_MB_MODE_INTRA_8X8,
    HL_CODEC_264_MB_MODE_INTRA_16X16,
    HL_CODEC_264_MB_MODE_END_SLICE_I_AND_SI,

    /* Table 7-12 – Macroblock type with value 0 for SI slices */
    // HL_CODEC_264_MB_MODE_Intra_4x4

    /* Table 7-13 – Macroblock type values 0 to 4 for P and SP slices */
    HL_CODEC_264_MB_MODE_START_SLICE_P = 200,
    HL_CODEC_264_MB_MODE_PRED_L0,
    HL_CODEC_264_MB_MODE_END_SLICE_P,

    /* Table 7-14 – Macroblock type values 0 to 22 for B slices */
    HL_CODEC_264_MB_MODE_START_SLICE_B = 300,
    HL_CODEC_264_MB_MODE_DIRECT,
    // HL_CODEC_264_MB_MODE_Pred_L0
    HL_CODEC_264_MB_MODE_PRED_L1,
    HL_CODEC_264_MB_MODE_BIPRED,
    HL_CODEC_264_MB_MODE_END_SLICE_B,

    /* Table G-5 – Inferred macroblock type I_BL for EI slices. */
    HL_CODEC_264_MB_MODE_START_SVC = 400,
    HL_CODEC_264_MB_MODE_INTRA_BL, // "Intra_Base", Predicted from base layer.
    HL_CODEC_264_MB_MODE_INTER_BL, // "Hartallo specific"
    HL_CODEC_264_MB_MODE_END_SVC
}
HL_CODEC_264_MB_MODE_T;

// Must be in this order: P, B
typedef enum HL_CODEC_264_SUBMB_TYPE_E {
    HL_CODEC_264_SUBMB_TYPE_NA = HL_CODEC_264_NA,
    /* P MB */
    HL_CODEC_264_SUBMB_TYPE_START_MB_P = 100,
    HL_CODEC_264_SUBMB_TYPE_P_L0_8X8,
    HL_CODEC_264_SUBMB_TYPE_P_L0_8X4,
    HL_CODEC_264_SUBMB_TYPE_P_L0_4X8,
    HL_CODEC_264_SUBMB_TYPE_P_L0_4X4,
    HL_CODEC_264_SUBMB_TYPE_END_MB_P,

    /* B MB */
    HL_CODEC_264_SUBMB_TYPE_START_MB_B = 200,
    HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8,
    HL_CODEC_264_SUBMB_TYPE_B_L0_8X8,
    HL_CODEC_264_SUBMB_TYPE_B_L1_8X8,
    HL_CODEC_264_SUBMB_TYPE_B_BI_8X8,
    HL_CODEC_264_SUBMB_TYPE_B_L0_8X4,
    HL_CODEC_264_SUBMB_TYPE_B_L0_4X8,
    HL_CODEC_264_SUBMB_TYPE_B_L1_8X4,
    HL_CODEC_264_SUBMB_TYPE_B_L1_4X8,
    HL_CODEC_264_SUBMB_TYPE_B_BI_8X4,
    HL_CODEC_264_SUBMB_TYPE_B_BI_4X8,
    HL_CODEC_264_SUBMB_TYPE_B_L0_4X4,
    HL_CODEC_264_SUBMB_TYPE_B_L1_4X4,
    HL_CODEC_264_SUBMB_TYPE_B_BI_4X4,
    HL_CODEC_264_SUBMB_TYPE_END_MB_B,
}
HL_CODEC_264_SUBMB_TYPE_T;

typedef enum HL_CODEC_264_SUBPART_SIZE_E {
    // Must not change (see table G-7)
    HL_CODEC_264_SUBPART_SIZE_8X8,
    HL_CODEC_264_SUBPART_SIZE_8X4,
    HL_CODEC_264_SUBPART_SIZE_4X8,
    HL_CODEC_264_SUBPART_SIZE_4X4,

    HL_CODEC_264_SUBPART_SIZE_MAX_COUNT
}
HL_CODEC_264_SUBPART_SIZE_T;

typedef enum HL_CODEC_264_PART_SIZE_E {
    // Must not change (see table G-7)
    HL_CODEC_264_PART_SIZE_16X16,
    HL_CODEC_264_PART_SIZE_16X8,
    HL_CODEC_264_PART_SIZE_8X16,
    HL_CODEC_264_PART_SIZE_8X8,

    HL_CODEC_264_PART_SIZE_MAX_COUNT,
}
HL_CODEC_264_PART_SIZE_T;

// Must be in this order: P, B
typedef enum HL_CODEC_264_SUBMB_MODE_E {
    HL_CODEC_264_SUBMB_MODE_NA = HL_CODEC_264_NA,

    /* == P == */
    HL_CODEC_264_SUBMB_MODE_START_MB_P = 100,
    HL_CODEC_264_SUBMB_MODE_PRED_L0,

    /* == B == */
    HL_CODEC_264_SUBMB_MODE_START_MB_B = 200,
    HL_CODEC_264_SUBMB_MODE_DIRECT,
    // HL_CODEC_264_SUBMB_MODE_PRED_L0
    HL_CODEC_264_SUBMB_MODE_PRED_L1,
    HL_CODEC_264_SUBMB_MODE_BIPRED
}
HL_CODEC_264_SUBMB_MODE_T;

typedef enum HL_CODEC_264_TRANSFORM_TYPE_E {
    HL_CODEC_264_TRANSFORM_TYPE_UNSPECIFIED = -1,

    HL_CODEC_264_TRANSFORM_TYPE_4X4, // T_4x4
    HL_CODEC_264_TRANSFORM_TYPE_8X8, // T_8x8
    HL_CODEC_264_TRANSFORM_TYPE_16X16, // T_16x16
    HL_CODEC_264_TRANSFORM_TYPE_PCM // // T_PCM
}
HL_CODEC_264_TRANSFORM_TYPE_T;

// Table 8-2 – Specification of Intra4x4PredMode[ luma4x4BlkIdx ] and associated names
typedef enum HL_CODEC_264_I4x4_MODE_E {
    Intra_4x4_Vertical,
    Intra_4x4_Horizontal,
    Intra_4x4_DC,
    Intra_4x4_Diagonal_Down_Left,
    Intra_4x4_Diagonal_Down_Right,
    Intra_4x4_Vertical_Right,
    Intra_4x4_Horizontal_Down,
    Intra_4x4_Vertical_Left,
    Intra_4x4_Horizontal_Up,

    HL_CODEC_264_I4x4_MODE_MAX_COUNT
}
HL_CODEC_264_I4x4_MODE_T;

// Table 8-3 – Specification of Intra8x8PredMode[ luma8x8BlkIdx ] and associated names
typedef enum HL_CODEC_264_I8x8_MODE_E {
    Intra_8x8_Vertical,
    Intra_8x8_Horizontal,
    Intra_8x8_DC,
    Intra_8x8_Diagonal_Down_Left,
    Intra_8x8_Diagonal_Down_Right,
    Intra_8x8_Vertical_Right,
    Intra_8x8_Horizontal_Down,
    Intra_8x8_Vertical_Left,
    Intra_8x8_Horizontal_Up,

    HL_CODEC_264_I8x8_MODE_COUNT
}
HL_CODEC_264_I8x8_MODE_T;

// Table 8-4 – Specification of Intra16x16PredMode and associated names
typedef enum HL_CODEC_264_I16x16_MODE_E {
    Intra_16x16_Vertical,
    Intra_16x16_Horizontal,
    Intra_16x16_DC,
    Intra_16x16_Plane,

    HL_CODEC_264_I16x16_MODE_MAX_COUNT
}
HL_CODEC_264_I16x16_MODE_T;

// Table 8-5 – Specification of Intra chroma prediction modes and associated names
// intra_chroma_pred_mode
typedef enum HL_CODEC_264_INTRA_CHROMA_MODE_E {
    Intra_Chroma_DC = 0,
    Intra_Chroma_Horizontal = 1,
    Intra_Chroma_Vertical = 2,
    Intra_Chroma_Plane = 3,

    HL_CODEC_264_INTRA_CHROMA_MODE_MAX_COUNT
}
HL_CODEC_264_INTRA_CHROMA_MODE_T;

//
// Table 9-11
//
typedef enum HL_CODEC_264_CABAC_SLICETYPE_E {
    HL_CODEC_264_CABAC_SLICETYPE_SI = 0,
    HL_CODEC_264_CABAC_SLICETYPE_I = HL_CODEC_264_CABAC_SLICETYPE_SI,
    HL_CODEC_264_CABAC_SLICETYPE_P = 1,
    HL_CODEC_264_CABAC_SLICETYPE_SP = HL_CODEC_264_CABAC_SLICETYPE_P,
    HL_CODEC_264_CABAC_SLICETYPE_B = 2
}
HL_CODEC_264_CABAC_SLICETYPE_T;

//
// Table 9-11
//
typedef enum HL_CODEC_264_CABAC_SE_e {
    // == slice_data( ) ==
    HL_CODEC_264_CABAC_SE_MB_SKIP_FLAG,
    HL_CODEC_264_CABAC_SE_MB_FIELD_DECODING_FLAG,

    // == macroblock_layer( ) ==
    HL_CODEC_264_CABAC_SE_MB_TYPE,
    HL_CODEC_264_CABAC_SE_TRANSFORM_SIZE_8X8_FLAG,
    HL_CODEC_264_CABAC_SE_CODED_BLOCK_PATTERN_LUMA,
    HL_CODEC_264_CABAC_SE_CODED_BLOCK_PATTERN_CHROMA,
    HL_CODEC_264_CABAC_SE_MB_QP_DELTA,

    // == mb_pred( ) ==
    HL_CODEC_264_CABAC_SE_PREV_INTRA4X4_PRED_MODE_FLAG,
    HL_CODEC_264_CABAC_SE_REM_INTRA4X4_PRED_MODE,
    HL_CODEC_264_CABAC_SE_PREV_INTRA8X8_PRED_MODE_FLAG,
    HL_CODEC_264_CABAC_SE_REM_INTRA8X8_PRED_MODE,
    HL_CODEC_264_CABAC_SE_INTRA_CHROMA_PRED_MODE,

    // == mb_pred( ) and sub_mb_pred( ) ==
    HL_CODEC_264_CABAC_SE_REF_IDX_L0,
    HL_CODEC_264_CABAC_SE_REF_IDX_L1,
    HL_CODEC_264_CABAC_SE_MVD_L0_0, //[ ][ ][ 0 ]
    HL_CODEC_264_CABAC_SE_MVD_L1_0, // [ ][ ][ 0 ]
    HL_CODEC_264_CABAC_SE_MVD_L0_1, // [ ][ ][ 1 ]
    HL_CODEC_264_CABAC_SE_MVD_L1_1, //[ ][ ][ 1 ]

    // == sub_mb_pred( ) ==
    HL_CODEC_264_CABAC_SE_SUB_MB_TYPE,

    // == residual_block_cabac( ) ==
    HL_CODEC_264_CABAC_SE_CODED_BLOCK_FLAG,
    HL_CODEC_264_CABAC_SE_SIGNIFICANT_COEFF_FLAG,
    HL_CODEC_264_CABAC_SE_LAST_SIGNIFICANT_COEFF_FLAG,
    HL_CODEC_264_CABAC_SE_COEFF_ABS_LEVEL_MINUS1,
}
HL_CODEC_264_CABAC_SE_t;


/**
*	Table E-1 – Meaning of sample aspect ratio indicator
*/
typedef enum HL_CODEC_264_ASPECT_RATIO_E {
    HL_CODEC_264_ASPECT_RATIO_UNSPECIFIED = 0,
    HL_CODEC_264_ASPECT_RATIO_1_1 = 1,
    HL_CODEC_264_ASPECT_RATIO_12_11 = 2,
    HL_CODEC_264_ASPECT_RATIO_10_11 = 3,
    HL_CODEC_264_ASPECT_RATIO_16_11 = 4,
    HL_CODEC_264_ASPECT_RATIO_40_33 = 5,
    HL_CODEC_264_ASPECT_RATIO_24_11 = 6,
    HL_CODEC_264_ASPECT_RATIO_20_11 = 7,
    HL_CODEC_264_ASPECT_RATIO_32_11 = 8,
    HL_CODEC_264_ASPECT_RATIO_80_33 = 9,
    HL_CODEC_264_ASPECT_RATIO_18_11 = 10,
    HL_CODEC_264_ASPECT_RATIO_15_11 = 11,
    HL_CODEC_264_ASPECT_RATIO_64_33 = 12,
    HL_CODEC_264_ASPECT_RATIO_160_99 = 13,
    HL_CODEC_264_ASPECT_RATIO_4_3 = 14,
    HL_CODEC_264_ASPECT_RATIO_3_2 = 15,
    HL_CODEC_264_ASPECT_RATIO_2_1 = 16,
    /* 17..254 => RESERVED*/
    HL_CODEC_264_ASPECT_RATIO_EXTENDED_SAR = 255,
}
HL_CODEC_264_ASPECT_RATIO_T;

typedef enum HL_CODEC_264_ASYNCTOKEN_IDX_E {
    HL_CODEC_264_ASYNCTOKEN_IDX_TRANSF_DECODE_LUMA4X4,
    HL_CODEC_264_ASYNCTOKEN_IDX_TRANSF_DECODE_CHROMA,
    HL_CODEC_264_ASYNCTOKEN_IDX_INTERPOL_LUMA,
    HL_CODEC_264_ASYNCTOKEN_IDX_INTERPOL_CHROMA,

    HL_CODEC_264_ASYNCTOKEN_IDX_MAX_COUNT,
}
HL_CODEC_264_ASYNCTOKEN_IDX_T;

typedef enum HL_CODEC_264_MODE_E {
    HL_CODEC_264_MODE_16X16 = 1,
    HL_CODEC_264_MODE_16X8 = 2,
    HL_CODEC_264_MODE_8X16 = 3,
    HL_CODEC_264_MODE_8X8_SUB8X8 = 4,
    HL_CODEC_264_MODE_8X8_SUB8X4 = 5,
    HL_CODEC_264_MODE_8X8_SUB4X8 = 6,
    HL_CODEC_264_MODE_8X8_SUB4X4 = 7
}
HL_CODEC_264_MODE_T;

typedef struct hl_codec_264_cavlc_level_xs {
    int32_t level_prefix;
    int32_t level_suffix;
    int32_t levelSuffixSize;
}
hl_codec_264_cavlc_level_xt;

typedef struct hl_codec_264_mv_xs {
    int32_t x; // [0]
    int32_t y; // [1]
}
hl_codec_264_mv_xt;

typedef struct hl_codec_264_residual_inv_xs {
    // Inv. type. Also equal to "ctxBlockCat" for CABAC entropy
    enum HL_CODEC_264_RESISUAL_INV_TYPE_E e_type;

    hl_bool_t b_rdo;
    int32_t i_iCbCr;
    int32_t i_luma4x4BlkIdx;
    int32_t i_luma8x8BlkIdx;
    int32_t i_cbr4x4BlkIdx;
    int32_t i_cbr8x8BlkIdx;
}
hl_codec_264_residual_inv_xt;

// Used to store result from "6.4.10.4 Derivation process for neighbouring 4x4 luma blocks"
typedef struct hl_codec_264_neighbouring_luma_block4x4_xs {
    int32_t i_addr_A;
    int32_t i_blk_idx_A;
    int32_t i_addr_B;
    int32_t i_blk_idx_B;
}
hl_codec_264_neighbouring_luma_block4x4_xt;
typedef struct hl_codec_264_neighbouring_chroma_block4x4_xs {
    int32_t i_addr_A;
    int32_t i_blk_idx_A;
    int32_t i_addr_B;
    int32_t i_blk_idx_B;
}
hl_codec_264_neighbouring_chroma_block4x4_xt;

// 7.3.3.2 Prediction weight table syntax
typedef struct hl_codec_264_avc_pred_weight_table_xs {
    uint32_t luma_log2_weight_denom;
    uint32_t chroma_log2_weight_denom;
    uint32_t luma_weight_l0_flag;
    int32_t luma_weight_l0[HL_CODEC_264_NUM_REF_IDX_L0_ACTIVE_MINUS1_MAX_COUNT];
    int32_t luma_offset_l0[HL_CODEC_264_NUM_REF_IDX_L0_ACTIVE_MINUS1_MAX_COUNT];
    uint32_t chroma_weight_l0_flag;
    int32_t chroma_weight_l0[HL_CODEC_264_NUM_REF_IDX_L0_ACTIVE_MINUS1_MAX_COUNT][2];
    int32_t chroma_offset_l0[HL_CODEC_264_NUM_REF_IDX_L0_ACTIVE_MINUS1_MAX_COUNT][2];
    uint32_t luma_weight_l1_flag;
    int32_t luma_weight_l1[HL_CODEC_264_NUM_REF_IDX_L1_ACTIVE_MINUS1_MAX_COUNT];
    int32_t luma_offset_l1[HL_CODEC_264_NUM_REF_IDX_L1_ACTIVE_MINUS1_MAX_COUNT];
    uint32_t chroma_weight_l1_flag;
    int32_t chroma_weight_l1[HL_CODEC_264_NUM_REF_IDX_L1_ACTIVE_MINUS1_MAX_COUNT][2];
    int32_t chroma_offset_l1[HL_CODEC_264_NUM_REF_IDX_L1_ACTIVE_MINUS1_MAX_COUNT][2];
}
hl_codec_264_avc_pred_weight_table_xt;

// 7.3.3.3 Decoded reference picture marking syntax
typedef struct hl_codec_264_avc_dec_ref_base_pic_marking_xs {
    uint32_t no_output_of_prior_pics_flag;
    uint32_t long_term_reference_flag;
    uint32_t adaptive_ref_pic_marking_mode_flag;
    uint32_t memory_management_control_operation[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t difference_of_pic_nums_minus1[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t long_term_pic_num[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t long_term_frame_idx[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t max_long_term_frame_idx_plus1[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
}
hl_codec_264_avc_dec_ref_base_pic_marking_xt;

// G.7.3.3.5 Decoded reference base picture marking syntax
// dec_ref_base_pic_marking( )
typedef struct hl_codec_264_svc_dec_ref_base_pic_marking_xs {
    uint32_t adaptive_ref_base_pic_marking_mode_flag;
    uint32_t memory_management_base_control_operation[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t difference_of_base_pic_nums_minus1[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
    uint32_t long_term_base_pic_num[HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT];
}
hl_codec_264_svc_dec_ref_base_pic_marking_xt;

// Used by the encoder for motion estimation
typedef struct hl_codec_264_me_part_xs {
    int32_t NumMbPart;
    enum HL_CODEC_264_SUBMB_TYPE_E SubMbPredType[4/*mbPartIdx*/];
    int32_t NumSubMbPart[4/*mbPartIdx*/];
    int32_t SubMbPartWidth[4/*mbPartIdx*/];
    int32_t SubMbPartHeight[4/*mbPartIdx*/];
    enum HL_CODEC_264_SUBMB_MODE_E SubMbPredMode[4/*mbPartIdx*/];
    int32_t MbPartWidth;// MbPartWidth( mb_type )
    int32_t MbPartHeight;// MbPartHeight( mb_type )
    int32_t NumHeaderBitsCAVLC; // Number of bts for the header (mb_type, sub_types, ref_idx_lX, mb_skip_run, ...) - Without Motion Vectors
    int32_t Mode;
}
hl_codec_264_me_part_xt;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_DEFS_H_ */
