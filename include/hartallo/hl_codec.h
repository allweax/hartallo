#ifndef _HARTALLO_CODEC_H_
#define _HARTALLO_CODEC_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"

HL_BEGIN_DECLS

#if !defined(HL_CODEC_MAX_PLUGINS)
#	define HL_CODEC_MAX_PLUGINS			0xFF
#endif

struct hl_option_s;

typedef struct hl_codec_s {
    HL_DECLARE_OBJECT;

    /** Number of threads to use for encoding / decoding. Should be at most the number of CPU cores (see @ref hl_cpu_get_cores_count()).
    You should let us use the right value. */
    int32_t threads_count;

    /** Defines the codec profile. Default value: "BaseLine SVC".
    * encoding: set by you
    * decoding: set by us
    */
    HL_CODEC_PROFILE_T profile;

    /** Used by H.264 only. Zero to let the engine choose a right value.
    * encoding: set by you
    * decoding: set by us
    * Supported values: 9 ("1b"), 10("1.0"), 11("1.1"), 12("1.2"), 13("1.3"), 20("2.0"), 21("2.1"), 22("2.2"), 30("3.0"), 31("3.1"), 32("3.2"), 40("4.0"), 41("4.1"), 42("4.2"), 50("5.0") and 51("5.1").
    */
    int32_t level;

    /** The current video width. */
    hl_size_t width;
    /** The current video height. */
    hl_size_t height;

    /** Defines the frame rate. Required when the RC (Rate Control) module is enabled. For example, for 15fps: fps.num=1, fps.denum=15.
    * encoding: set by you
    * decoding: useless
    */
    struct hl_rational_s fps;

    /** The number of P frames for every I frame */
    int32_t gop_size;
    /** Maximum number of frames to use formotion estimation. Must be > 1 */
    int32_t max_ref_frame;

    /** Default quantization Parameter value. Useless in VBR mode. For H.264 AVC/AVC must be in [0-51]. This value is ignored when @b "rc_bitrate" is defined. */
    int32_t qp;

    /** Defines the RC(Rate Control) level.
    * encoding: set by you
    * decoding: useless
    */
    enum HL_RC_LEVEL_E rc_level;
    /** Defines the target bitrate (in kbps). Use a value <#0 to disable the rate control module and lower CPU usage. Default=#0. <br />
    * For VBR (Variable BitRate), use @b "rc_bitrate_min" < @b "rc_bitrate_max". <br />
    * For CBR (Constant BitRate), use @b "rc_bitrate" == @b "rc_bitrate_min" == @b "rc_bitrate_max". <br />
    * encoding: set by you
    * decoding: useless
    */
    int64_t rc_bitrate;
    /** Defines the minimum bitrate (in kbps). Use a value <#0 to ignore. This value is useful to guarantee a minimum quality. Default=#0.
    * encoding: set by you
    * decoding: useless
    */
    int64_t rc_bitrate_min;
    /** Defines the maximum bitrate (in kbps). Use a value <#0 to ignore. Default=#0.
    * encoding: set by you
    * decoding: useless
    */
    int64_t rc_bitrate_max;
    /** Min Quantization Parameter value. Useless in CBR mode. For H.264 AVC/AVC must be in [0-51]. */
    int32_t rc_qp_min;
    /** Max Quantization Parameter value. Useless in CBR mode. For H.264 AVC/AVC must be in [0-51]. */
    int32_t rc_qp_max;

    /* Number of elements (MB for H.264) per base unit. "<=0" means picture. */
    int32_t rc_basicunit;

    /** Motion estimation search range (integer-pel range). For H.264, this value should be in [1-64]. */
    int32_t me_range;
    /** Motion estimation type to perform for motion search */
    HL_VIDEO_ME_TYPE_T me_type;
    /** Partition types to check when performing motion estimation. */
    HL_VIDEO_ME_PART_TYPE_T me_part_types;
    /** Sub-Partition types to check when performing motion estimation. */
    HL_VIDEO_ME_SUBPART_TYPE_T me_subpart_types;
    /** Whether to enable Early termination for Motion Estimation to speedup the process. Check JVT-O079 for more information.
    * encoding: set by you
    * decoding: set by us
    */
    int32_t me_early_term_flag;

    /** Whether to enable deblocking filter on the base layer.
    * encoding: set by you
    * decoding: set by us
    */
    int32_t deblock_flag;

    /** Whether to enable deblocking filter on the enhacement layers.
    * encoding: set by you
    * decoding: set by us
    */
    int32_t deblock_inter_layer_flag;

    /** Headers bytes. For H.264, this will contain the active SPS and PPS bytes separated by a start code prefix (0x000001).
    * encoding: set by us
    * decoding: set by us
    */
    const uint8_t* hdr_bytes;
    hl_size_t hdr_bytes_count;

    /** The inter pred search type to use.
    * encoding: set by you
    * decoding: ignored
    */
    HL_VIDEO_INTER_PRED_SEARCH_TYPE_T inter_pred_search_type;

    /** The method to use to compute image distorsion.
    * encoding: set by you
    * decoding: ignored
    */
    HL_VIDEO_DISTORTION_MESURE_TYPE_T  distortion_mesure_type;

    /** List of active layers for the encoder. Onlu valid for SVC (Scalable Video Coding) codec (e.g. H.264 SVC).
    * encoding: set by you using @ref hl_codec_add_layer(). Use @ref hl_codec_clear_layers() to clear all layers (e.g. to switch from H.264 SVC to AVC).
    * decoding: ignored
    */
    struct hl_layer_def_xs layers[HL_ENCODER_MAX_LAYERS];
    hl_size_t layers_active_count; /**< Private variable must not be changed. */

    /** Minimum SVC layer to decode. Any layer with lower identifier (DQId) will be skipped. Use @ref HL_SVC_H264_MAKE_DQID to build this value.
    * encoding: ignored
    * decoding: set by you
    */
    int32_t dqid_min;
    /** Maximum SVC layer to decode. Any layer with higher identifier (DQId) will be skipped. Use @ref HL_SVC_H264_MAKE_DQID to build this value.
    * encoding: ignored
    * decoding: set by you
    */
    int32_t dqid_max;

    const struct hl_codec_plugin_def_s* plugin;
}
hl_codec_t;

typedef struct hl_codec_result_s {
    HL_DECLARE_OBJECT;

    /** The result type. Valid only if the operation (encode() or decode()) succeed. */
    enum HL_CODEC_RESULT_TYPE_E type;

    /** Pointer to the output data (e.g. encoded or decoded samples). Valid only if @b type is @ref HL_CODEC_RESULT_TYPE_FRAMES */
    const uint8_t* data_ptr;
    /** @b data_ptr size in bytes */
    hl_size_t data_size;
    /** @b width */
    hl_size_t width;
    /** @b height */
    hl_size_t height;
    /** @b dqid */
    int32_t dqid;
}
hl_codec_result_t;

#define HL_DECLARE_CODEC hl_codec_t __base_codec__

typedef struct hl_codec_plugin_def_s {
    const hl_object_def_t* objdef;

    enum HL_CODEC_TYPE_E type;
    enum HL_MEDIA_TYPE_E media;
    const char* description;

    HL_ERROR_T (*set_option) (struct hl_codec_s* , const struct hl_option_s*);
    HL_ERROR_T (*decode) (struct hl_codec_s* , const void* pc_data, hl_size_t u_data_size, struct hl_codec_result_s* p_result);
    HL_ERROR_T (*encode) (struct hl_codec_s* , const struct hl_frame_s* pc_frame, struct hl_codec_result_s* p_result);
}
hl_codec_plugin_def_t;

HARTALLO_API HL_ERROR_T hl_codec_result_create(struct hl_codec_result_s** pp_result);

HARTALLO_API HL_ERROR_T hl_codec_create(const struct hl_codec_plugin_def_s* plugin, hl_codec_t** pp_codec);
HARTALLO_API HL_ERROR_T hl_codec_set_option_int64(struct hl_codec_s* self, int64_t val);
HARTALLO_API HL_ERROR_T hl_codec_set_option_pchar(struct hl_codec_s* self, const char* val);
HARTALLO_API HL_ERROR_T hl_codec_set_option_pobj(struct hl_codec_s* self, hl_object_t* val);
HARTALLO_API HL_ERROR_T hl_codec_add_layer(struct hl_codec_s* self, uint32_t u_width, uint32_t u_height, int32_t i_qp, int32_t i_fps);
HARTALLO_API HL_ERROR_T hl_codec_clear_layers(struct hl_codec_s* self);
HARTALLO_API HL_ERROR_T hl_codec_decode(struct hl_codec_s* self, const void* pc_data, hl_size_t data_size, struct hl_codec_result_s* p_result);
HARTALLO_API HL_ERROR_T hl_codec_encode(struct hl_codec_s* self, const struct hl_frame_s* pc_frame, struct hl_codec_result_s* p_result);
HARTALLO_API HL_ERROR_T hl_codec_get_result_const(const struct hl_codec_s* self, const struct hl_codec_result_s** ppc_result);


HARTALLO_API HL_ERROR_T hl_codec_plugin_register(const hl_codec_plugin_def_t* plugin);
HARTALLO_API HL_ERROR_T hl_codec_plugin_unregister(const hl_codec_plugin_def_t* plugin);
HARTALLO_API HL_ERROR_T hl_codec_plugin_find(HL_CODEC_TYPE_T type, const hl_codec_plugin_def_t** ppc_plugin);

HARTALLO_API HL_ERROR_T hl_codec_guess_best_bitrate(enum HL_VIDEO_MOTION_RANK_E e_mr, uint32_t u_width, uint32_t u_height, const struct hl_rational_s* p_fps, int64_t* p_bitrate);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_H_ */
