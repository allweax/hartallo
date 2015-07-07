#ifndef _HARTALLO_TYPES_H_
#define _HARTALLO_TYPES_H_

#include "hl_config.h"

HL_BEGIN_DECLS

typedef uint8_t hl_pixel_t;
typedef int8_t hl_spixel_t;
typedef int32_t hl_blk8x8_idx_t;
typedef int32_t hl_blk4x4_idx_t;
typedef int32_t hl_int32_4_t[4];
typedef int32_t hl_int32_4x4_t[4][4];
typedef int32_t hl_int32_4x4x2_t[4][4][2];
typedef int32_t hl_int32_4x2_t[4][2];
typedef int32_t hl_int32_2x2_t[2][2];
typedef int32_t hl_int32_16_t[16];
typedef int32_t hl_int32_16x16_t[16][16];
typedef int32_t hl_int32_16x4x4_t[16][4][4];
typedef int32_t hl_int32_8x64_t[8][64];
typedef int32_t hl_int32_33_t[33];
typedef int32_t hl_int32_13_t[13];
typedef uint8_t hl_uint8_4x4_t[4][4];
typedef uint8_t hl_uint8_16x16_t[16][16];
typedef uint8_t hl_uint8_16_t[16];
typedef int32_t hl_mb_type_flags_t;
typedef int hl_boolean_t;
#define hl_bool_t hl_boolean_t
#define HL_BOOL_T hl_bool_t
#define HL_BOOLEAN_T hl_boolean_t
#define hl_true		1
#define hl_false	0
#define HL_TRUE		hl_true
#define HL_FALSE	hl_false

#ifdef NULL
#	define HL_NULL    NULL
#else
#	define HL_NULL    0
#endif
#define hl_null			HL_NULL

#define HL_SIZE_T		size_t
#define hl_size_t		HL_SIZE_T

#define HL_OUT
#define HL_IN
#define HL_IN_ALIGNED(x)	HL_IN
#define HL_OUT_ALIGNED(x)	HL_OUT
#define HL_IN_OUT HL_IN HL_OUT
#define HL_IN_OUT_ALIGNED(x) HL_IN_ALIGNED(x) HL_OUT_ALIGNED(x)
#define HL_ASYNC_CALL_DIRECT
#define HL_ASYNC_CALL_INDIRECT

#define HL_DEFAULT_VIDEO_GOP_SIZE						25 /**< Default video gop (Group Of Pictures) size. */
#define HL_DEFAULT_VIDEO_QP								23 /**< Default video QP (Quantization Parameter) value. */
#define HL_DEFAULT_VIDEO_FPS							15 /**< Default video frames per second. */
#define HL_DEFAULT_VIDEO_RC_LEVEL						HL_RC_LEVEL_ALL /**< Default video RC (Rate Control) type. */
#define HL_DEFAULT_VIDEO_RC_BITRATE						-1 /**< Default target bitrate value. */
#define HL_DEFAULT_VIDEO_RC_BITRATE_MIN					-1 /**< Default min bitrate value. */
#define HL_DEFAULT_VIDEO_RC_BITRATE_MAX					-1 /**< Default max bitrate value. */
#define HL_DEFAULT_VIDEO_RC_QP_MIN						-1 /**< Default min qp value. */
#define HL_DEFAULT_VIDEO_RC_QP_MAX						-1 /**< Default max qp value. */
#define HL_DEFAULT_VIDEO_RC_BASICUNIT					-1 /**< Default number of elements per basic unit. */
#define HL_DEFAULT_VIDEO_ME_RANGE						8 /** < Defaul video motion estimation search range. */
#define HL_DEFAULT_VIDEO_ME_TYPE						HL_VIDEO_ME_TYPE_ALL /**< Default video motion estimation type. */
#define HL_DEFAULT_VIDEO_ME_EARLY_TERM_FLAG				1 /**< Default value to know whether to enable ME early termination. */
#define HL_DEFAULT_VIDEO_MAX_REF_FRAMES					1 /**< Default maximum number of frames to use for motion estimation. */
#define HL_DEFAULT_VIDEO_DEBLOCK_BASE_FLAG				1 /**< Default value to know whether to enable deblocking filter on base layer. */
#define HL_DEFAULT_VIDEO_DEBLOCK_INTER_LAYER_FLAG		1 /**< Default value to know whether to enable deblocking filter on enhacement layers. */
#define HL_DEFAULT_VIDEO_INTER_PRED_SEARCH_TYPE			HL_VIDEO_INTER_PRED_SEARCH_TYPE_FAST_THREE_STEP /**< Default intra search type. */
#define HL_DEFAULT_VIDEO_DISTORTION_MESURE_TYPE			HL_VIDEO_DISTORTION_MESURE_TYPE_AUTO /**< Default function to use to compute image distortion. */
#define HL_DEFAULT_VIDEO_ME_PART_TYPE					HL_VIDEO_ME_PART_TYPE_ALL /**< Default partition types to check when performing motion estimation. */
#define HL_DEFAULT_VIDEO_ME_SUBPART_TYPE				HL_VIDEO_ME_SUBPART_TYPE_ALL /**< Default sub-partition types to check when performing motion estimation. */
#define HL_DEFAULT_VIDEO_DQID_MIN						-1 /**< Default minimum DQId to decode. */
#define HL_DEFAULT_VIDEO_DQID_MAX						-1 /**< Default maximum DQId to decode. */

#define HL_ENCODER_MAX_LAYERS							10

#ifndef HL_BUFFER_PADDING_SIZE
#	define HL_BUFFER_PADDING_SIZE	sizeof(int64_t)
#endif /* HL_BUFFER_PADDING_SIZE */

/**
* Make "DQId" value using "dependency_id" and "quality_id" as defined in H.264 SVC standard Annex G. section (G-61)
* @param dependency_id Dependency Id.
* @param quality_id Quality Id.
* @retval DQId value.
*/
#define HL_SVC_H264_MAKE_DQID(dependency_id, quality_id) (((dependency_id) << 4) + (quality_id))

#if defined(va_copy)
#	define hl_va_copy(D, S)       va_copy((D), (S))
#elif defined(__va_copy)
#	define hl_va_copy(D, S)       __va_copy((D), (S))
#else
#	define hl_va_copy(D, S)       ((D) = (S))
#endif


typedef enum HL_ERROR_E {
    HL_ERROR_SUCCESS = 0,

    HL_ERROR_INVALID_PARAMETER,
    HL_ERROR_INVALID_BITSTREAM,
    HL_ERROR_INVALID_STATE,
    HL_ERROR_INVALID_FORMAT,
    HL_ERROR_INVALID_OPERATION,
    HL_ERROR_NOT_FOUND,
    HL_ERROR_NOT_IMPLEMENTED,
    HL_ERROR_OUTOFMEMMORY,
    HL_ERROR_OUTOFBOUND,
    HL_ERROR_OUTOFCAPACITY,
    HL_ERROR_UNEXPECTED_CALL,
    HL_ERROR_ACCESS_DENIED,
    HL_ERROR_SYSTEM,
    HL_ERROR_TIMEDOUT,
    HL_ERROR_TOOSHORT,

    HL_ERROR_TEST_FAILED, /* Use for unit tests */
}
HL_ERROR_T;


typedef enum HL_CODEC_TYPE_E {
    HL_CODEC_TYPE_NONE,

    HL_CODEC_TYPE_H264,
    /* For backward compatibility */
    HL_CODEC_TYPE_H264_AVC = HL_CODEC_TYPE_H264,
    HL_CODEC_TYPE_H264_SVC = HL_CODEC_TYPE_H264,
    HL_CODEC_TYPE_H264_MVC = HL_CODEC_TYPE_H264,

#if HL_RELEASE_VERSION >= 0x002000000
    HL_CODEC_TYPE_H265,
    HL_CODEC_TYPE_G729,
#endif
}
HL_CODEC_TYPE_T;

typedef enum HL_CODEC_PROFILE_E {
    HL_CODEC_PROFILE_NONE,

    HL_CODEC_PROFILE_H264_BASELINE		/*= 66*/,
    HL_CODEC_PROFILE_H264_MAIN			/*= 77*/,
    HL_CODEC_PROFILE_H264_EXTENDED		/*= 88*/,
    HL_CODEC_PROFILE_H264_HIGH			/*= 100*/,
    HL_CODEC_PROFILE_H264_HIGH10		/*= 110*/,
    HL_CODEC_PROFILE_H264_HIGH422		/*= 122*/,
    HL_CODEC_PROFILE_H264_HIGH444		/*= 144*/,
    HL_CODEC_PROFILE_H264_CAVLC444		/*= 244*/,
    HL_CODEC_PROFILE_H264_BASELINE_SVC	/*= 83*/,
    HL_CODEC_PROFILE_H264_HIGH_SVC		/*= 86*/
}
HL_CODEC_PROFILE_T;

/** Result type for the encoding and decoding operations */
typedef enum HL_CODEC_RESULT_TYPE_E {
    HL_CODEC_RESULT_TYPE_NONE, /**< No useful result. */
    HL_CODEC_RESULT_TYPE_DATA = (0x01 << 0), /**< Result contains encoded or decoded frames. */
    HL_CODEC_RESULT_TYPE_HDR = (0x01 << 1), /**< Result contains headers (e.g. SPS or PPS for H.264). */
}
HL_CODEC_RESULT_TYPE_T;

typedef enum HL_PARSER_TYPE_E {
    HL_PARSER_TYPE_NONE,

    HL_PARSER_TYPE_H264,
    /* For backward compatibility */
    HL_PARSER_TYPE_H264_AVC = HL_PARSER_TYPE_H264,
    HL_PARSER_TYPE_H264_SVC = HL_PARSER_TYPE_H264,
    HL_PARSER_TYPE_H264_MVC = HL_PARSER_TYPE_H264,

#if HL_RELEASE_VERSION >= 0x002000000
    HL_PARSER_TYPE_H265,
    HL_PARSER_TYPE_MP4,
    HL_PARSER_TYPE_WEBM,
    HL_PARSER_TYPE_MKV,
#endif
}
HL_PARSER_TYPE_T;

typedef enum HL_MEDIA_TYPE_E {
    HL_MEDIA_TYPE_NONE,

    HL_MEDIA_TYPE_AUDIO,
    HL_MEDIA_TYPE_VIDEO,
}
HL_MEDIA_TYPE_T;

/** Video motion estimation type. */
typedef enum HL_VIDEO_ME_TYPE_E {
    HL_VIDEO_ME_TYPE_INTEGER = (0x01 << 0),
    HL_VIDEO_ME_TYPE_HALF = (0x01 << 1),
    HL_VIDEO_ME_TYPE_QUATER = (0x01 << 2),
    HL_VIDEO_ME_TYPE_ALL = (HL_VIDEO_ME_TYPE_HALF | HL_VIDEO_ME_TYPE_HALF | HL_VIDEO_ME_TYPE_QUATER)
} HL_VIDEO_ME_TYPE_T;

/** Inter predition search type for the encoder */
typedef enum HL_VIDEO_INTER_PRED_SEARCH_TYPE_E {
    /** Full search type. Try all 9 modes and compute the cost using SAD (or any other enabled method).
    This method gives good compression ratio at speed expense (slooow). Not recommended for embedded devices.
    */
    HL_VIDEO_INTER_PRED_SEARCH_TYPE_FS,
    /** Check "Fast Three Step Intra Prediction Algorithm for 4x4 blocks in H.264" paper for moinformation about this method.
    This method gives good speed at compression ratio expense. Recommended for embedded devices.
    */
    HL_VIDEO_INTER_PRED_SEARCH_TYPE_FAST_THREE_STEP,

    /** Diamond search type.
    */
    HL_VIDEO_INTER_PRED_SEARCH_TYPE_DIAMOND,

    /** UMHexagonS (Unsymmetrical-cross Multi-Hexagon-grid Search) as defined in JVT-F017 (see pdf in documentation folder) for
    interger pel search and a CBFPS (Center biased Fractional Pel Search) for fractional pel search.
    */
    HL_VIDEO_INTER_PRED_SEARCH_TYPE_UMHEXAGONS,
} HL_VIDEO_INTER_PRED_SEARCH_TYPE_T;

/** Method to use to compute distorsion. */
typedef enum HL_VIDEO_DISTORTION_MESURE_TYPE_E {
    /** Let the encoder choose the best type. */
    HL_VIDEO_DISTORTION_MESURE_TYPE_AUTO,
    /** Sum of Absolute Difference. <a href="http://en.wikipedia.org/wiki/Sum_of_absolute_differences">http://en.wikipedia.org/wiki/Sum_of_absolute_differences</a>. */
    HL_VIDEO_DISTORTION_MESURE_TYPE_SAD,
    /** Sum of Squared Distortion. */
    HL_VIDEO_DISTORTION_MESURE_TYPE_SSD,
}
HL_VIDEO_DISTORTION_MESURE_TYPE_T;

/** Partition types to check when performing motion estimation */
typedef enum HL_VIDEO_ME_PART_TYPE_E {
    HL_VIDEO_ME_PART_TYPE_NONE = 0x00, /**< None. You must not use this value. */
    HL_VIDEO_ME_PART_TYPE_16X16 = (0x01 << 0), /**< 16x16 partition type. */
    HL_VIDEO_ME_PART_TYPE_16X8 = (0x01 << 1), /**< 16x8 partition type. You can also define subparts (for the 8x8) types when this type is enabled. */
    HL_VIDEO_ME_PART_TYPE_8X16 = (0x01 << 2), /**< 8x16 partition type. You can also define subparts (for the 8x8) types when this type is enabled. */
    HL_VIDEO_ME_PART_TYPE_8X8 = (0x01 << 3), /**< 8x8 partition type. You can also define subparts (for the 8x8) types when this type is enabled. */
    HL_VIDEO_ME_PART_TYPE_ALL = 0xFF /**< All partition types. */
}
HL_VIDEO_ME_PART_TYPE_T;

/** Sub-Partition (for each 8x8 part) types to check when performing motion estimation */
typedef enum HL_VIDEO_ME_SUBPART_TYPE_E {
    HL_VIDEO_ME_SUBPART_TYPE_NONE = 0x00, /**< None. */
    HL_VIDEO_ME_SUBPART_TYPE_4X4 = (0x01 << 0), /**< 4X4 sub-partition type. */
    HL_VIDEO_ME_SUBPART_TYPE_8X4 = (0x01 << 1), /**< 8X4 sub-partition type. */
    HL_VIDEO_ME_SUBPART_TYPE_4X8 = (0x01 << 2), /**< 4X8 sub-partition type. */
    HL_VIDEO_ME_SUBPART_TYPE_8X8 = (0x01 << 3), /**< 8X8 sub-partition type. */
    HL_VIDEO_ME_SUBPART_TYPE_ALL = 0xFF /**< All sub-partition types. */
}
HL_VIDEO_ME_SUBPART_TYPE_T;

/** Video chroma type. */
typedef enum HL_VIDEO_CHROMA_E {
    HL_VIDEO_CHROMA_NONE,

    HL_VIDEO_CHROMA_YUV420, /**< 4:2:0 Progressive */
}
HL_VIDEO_CHROMA_T;

/** Video encoding types. Could be defined before encoding every video frame to decide the output type. */
typedef enum HL_VIDEO_ENCODING_TYPE_E {
    /**Let the encoder choose the best mode. The mode will depend on the video granularity, scene and GOP size. This is the default value. */
    HL_VIDEO_ENCODING_TYPE_AUTO,

    /** Encodes an INTRA frame (without any dependency on the previous frames) */
    HL_VIDEO_ENCODING_TYPE_INTRA,
    /** Encodes an INTER frame using dependencies on the previous frames */
    HL_VIDEO_ENCODING_TYPE_INTER,
    /** Encode the frame with any loss. You should not use this option unless you know what you're doing */
    HL_VIDEO_ENCODING_TYPE_LOSSLESS,
    /** Write the video samples "AS IS". For H.264 AVC/SVC the frame type will be I_PCM.  */
    HL_VIDEO_ENCODING_TYPE_RAW
} HL_VIDEO_ENCODING_TYPE_T;

/** Video motion rank. */
typedef enum HL_VIDEO_MOTION_RANK_E {
    HL_VIDEO_MOTION_RANK_LOW = 1, /**< Low motion rank. */
    HL_VIDEO_MOTION_RANK_MEDIUM = 2, /**< Medium motion rank. */
    HL_VIDEO_MOTION_RANK_HIGH = 4, /**< High motion rank. */
}
HL_VIDEO_MOTION_RANK_T;

/** Rate Control level. */
typedef enum HL_RC_LEVEL_E {
    /** GOP level RC type. Default RC type for video media type. */
    HL_RC_LEVEL_GOP = (0x01 << 0),
    /** Picture level RC type. */
    HL_RC_LEVEL_PICTURE = (0x01 << 1),
    /** MB level RC type. */
    HL_RC_LEVEL_MB = (0x01 << 2),
    /** AU level RC type. */
    HL_RC_LEVEL_AU = (0x01 << 3),
    /** ALL levels RC type. */
    HL_RC_LEVEL_ALL = 0xFF
} HL_RC_LEVEL_T;

// must be sync'ed with "DEBUG_LEVEL_XXX" in "hl_debug.h"
typedef enum HL_DEBUG_LEVEL_E {
    HL_DEBUG_LEVEL_TALKATIVE	= 5,
    HL_DEBUG_LEVEL_INFO			= 4,
    HL_DEBUG_LEVEL_WARN			= 3,
    HL_DEBUG_LEVEL_ERROR		= 2,
    DEBUG_LEVEL_FATAL			= 1,
}
HL_DEBUG_LEVEL_T;

typedef enum HL_OPTION_TYPE_E {
    HL_OPTION_TYPE_NONE,

    HL_OPTION_TYPE_INT64,
    HL_OPTION_TYPE_PCHAR,
    HL_OPTION_TYPE_POBJ
}
HL_OPTION_TYPE_T;

typedef enum HL_DIAMOND_PATTERN_TYPE_E {
    // Large Diamond Search Pattern (SDSP)
    HL_DIAMOND_PATTERN_TYPE_LARGE,
    // Small Diamond Search Pattern (SDSP)
    HL_DIAMOND_PATTERN_TYPE_SMALL,
}
HL_DIAMOND_PATTERN_TYPE_T;

typedef enum HL_DIAMOND_PT_TYPE_E {
    HL_DIAMOND_PT_TYPE_CENTER = 0,
    HL_DIAMOND_PT_TYPE_LEFT = 1,
    HL_DIAMOND_PT_TYPE_DIAG_TOP_LEFT = 2,
    HL_DIAMOND_PT_TYPE_TOP = 3,
    HL_DIAMOND_PT_TYPE_DIAG_TOP_RIGHT = 4,
    HL_DIAMOND_PT_TYPE_RIGHT = 5,
    HL_DIAMOND_PT_TYPE_DIAG_BOTTOM_RIGHT = 6,
    HL_DIAMOND_PT_TYPE_BOTTOM = 7,
    HL_DIAMOND_PT_TYPE_DIAG_BOTTOM_LEFT = 8,

    HL_DIAMOND_PT_TYPE_MAX_COUNT
}
HL_DIAMOND_PT_TYPE_T;

#if HL_HAVE_X86_INTRIN
typedef __m128i hl_int128_t;
typedef union {
    __m128i i;
    __m128 f;
} __m128x;
#else
HL_ALIGN(HL_ALIGN_V) typedef union hl_int128_u {
    int8_t              i8[16];
    int16_t             i16[8];
    int32_t             i32[4];
    int64_t             i64[2];
    uint8_t				u8[16];
    uint16_t			u16[8];
    uint32_t			u32[4];
    uint64_t			u64[2];
}
hl_int128_t;
#endif /* HL_HAVE_X86_INTRIN */
#define HL_INT128_I32(p_self, idx) (((int32_t*)(p_self))[(idx)])
#define HL_INT128_I16(p_self, idx) (((int16_t*)(p_self))[(idx)])

// Order (left,right,top,bottom) MUST not change
typedef struct hl_rect_xs {
    int32_t left;
    int32_t right;
    int32_t top;
    int32_t bottom;
}
hl_rect_xt;

typedef struct hl_rational_s {
    int32_t num;
    int32_t den;
}
hl_rational_t;

// Used to define layers (Quality, FrameRate and Spatial) for a SVC (Scalable Video Coding) encoder
typedef struct hl_layer_def_xs {
    hl_size_t u_index;
    uint32_t u_width;
    uint32_t u_height;
    int32_t i_qp;
    int32_t i_fps;
}
hl_layer_def_xt;

typedef struct hl_diamond_pt_xs {
    enum HL_DIAMOND_PT_TYPE_E e_type;
    hl_bool_t b_skip;
    double d_cost;
    int32_t i_dist;
    int32_t i_rate;
    int32_t i_x;
    int32_t i_y;
}
hl_diamond_pt_xt;

typedef struct hl_diamond_search_xs {
    enum HL_DIAMOND_PATTERN_TYPE_E e_pattern;

    struct hl_diamond_pt_xs pts[HL_DIAMOND_PT_TYPE_MAX_COUNT];
    double tmp_cost[HL_DIAMOND_PT_TYPE_MAX_COUNT];

    struct hl_diamond_pt_xs min_pt;

    struct hl_rect_xs clip;
}
hl_diamond_search_xt;

HL_END_DECLS

#endif /* _HARTALLO_TYPES_H_ */
