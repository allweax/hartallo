#include "hartallo/hl_codec.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_option.h"
#include "hartallo/hl_debug.h"

static const hl_codec_plugin_def_t* __hl_codec_plugins[HL_CODEC_MAX_PLUGINS] = {0};

HL_ERROR_T hl_codec_result_create(struct hl_codec_result_s** pp_result)
{
    extern const hl_object_def_t *hl_codec_result_def_t;
    if (!pp_result) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_result = hl_object_create(hl_codec_result_def_t);
    if (!*pp_result) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_create(const struct hl_codec_plugin_def_s* plugin, hl_codec_t** pp_codec)
{
    if (!plugin || !pp_codec) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_codec = hl_object_create(plugin->objdef);
    if (!*pp_codec) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_codec)->plugin = plugin;
    (*pp_codec)->threads_count = hl_cpu_get_cores_count();
    if (plugin->media == HL_MEDIA_TYPE_VIDEO) {
        (*pp_codec)->gop_size = HL_DEFAULT_VIDEO_GOP_SIZE;
        (*pp_codec)->max_ref_frame = HL_DEFAULT_VIDEO_MAX_REF_FRAMES;
        (*pp_codec)->fps.num = 1;
        (*pp_codec)->fps.den = HL_DEFAULT_VIDEO_FPS;
        (*pp_codec)->qp = HL_DEFAULT_VIDEO_QP;
        (*pp_codec)->rc_level = HL_DEFAULT_VIDEO_RC_LEVEL;
        (*pp_codec)->rc_bitrate = HL_DEFAULT_VIDEO_RC_BITRATE;
        (*pp_codec)->rc_bitrate_min = HL_DEFAULT_VIDEO_RC_BITRATE_MIN;
        (*pp_codec)->rc_bitrate_max = HL_DEFAULT_VIDEO_RC_BITRATE_MAX;
        (*pp_codec)->rc_qp_min = HL_DEFAULT_VIDEO_RC_QP_MIN;
        (*pp_codec)->rc_qp_max = HL_DEFAULT_VIDEO_RC_QP_MAX;
        (*pp_codec)->rc_basicunit = HL_DEFAULT_VIDEO_RC_BASICUNIT;
        (*pp_codec)->me_range = HL_DEFAULT_VIDEO_ME_RANGE;
        (*pp_codec)->me_type = HL_DEFAULT_VIDEO_ME_TYPE;
        (*pp_codec)->me_part_types = HL_DEFAULT_VIDEO_ME_PART_TYPE;
        (*pp_codec)->me_subpart_types = HL_DEFAULT_VIDEO_ME_SUBPART_TYPE;
        (*pp_codec)->me_early_term_flag = HL_DEFAULT_VIDEO_ME_EARLY_TERM_FLAG;
        (*pp_codec)->deblock_flag = HL_DEFAULT_VIDEO_DEBLOCK_BASE_FLAG;
        (*pp_codec)->deblock_inter_layer_flag = HL_DEFAULT_VIDEO_DEBLOCK_INTER_LAYER_FLAG;
        (*pp_codec)->inter_pred_search_type = HL_DEFAULT_VIDEO_INTER_PRED_SEARCH_TYPE;
        (*pp_codec)->distortion_mesure_type = HL_DEFAULT_VIDEO_DISTORTION_MESURE_TYPE;
        (*pp_codec)->dqid_min = HL_DEFAULT_VIDEO_DQID_MIN;
        (*pp_codec)->dqid_max = HL_DEFAULT_VIDEO_DQID_MAX;
    }

    return HL_ERROR_SUCCESS;
}

#define hl_codec_set_option(self, val, type) \
{ \
	if (!self || !self->plugin || !self->plugin->set_option) { \
		HL_DEBUG_ERROR("Invalid parameter"); \
        return HL_ERROR_INVALID_PARAMETER; \
	} \
	else { \
		hl_option_t* p_option = HL_NULL; \
		HL_ERROR_T err = hl_option_create_##type(val, &p_option); \
		if (err == HL_ERROR_SUCCESS) { \
			err = self->plugin->set_option(self, p_option); \
		} \
		HL_OBJECT_SAFE_FREE(p_option); \
		return err; \
	} \
}

HL_ERROR_T hl_codec_set_option_int64(hl_codec_t* self, int64_t val)
{
    hl_codec_set_option(self, val, i64);
}

HL_ERROR_T hl_codec_set_option_pchar(hl_codec_t* self, const char* val)
{
    hl_codec_set_option(self, val, pchar);
}

HL_ERROR_T hl_codec_set_option_pobj(hl_codec_t* self, hl_object_t* val)
{
    hl_codec_set_option(self, val, pobj);
}

HL_ERROR_T hl_codec_add_layer(hl_codec_t* self, uint32_t u_width, uint32_t u_height, int32_t i_qp, int32_t i_fps)
{
    hl_layer_def_xt* p_layer_def;
    if (!self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (self->layers_active_count >= HL_ENCODER_MAX_LAYERS) {
        HL_DEBUG_ERROR("%d already added", self->layers_active_count);
        return HL_ERROR_OUTOFCAPACITY;
    }
    if (self->layers_active_count > 0) {
        /* Make sure layers are in increasing order */
        p_layer_def = &self->layers[self->layers_active_count - 1];
        if (p_layer_def->u_width >= u_width || p_layer_def->u_height >= u_height) { // "=" because only "Spatial" scalability is supported on the encoder for now
            HL_DEBUG_ERROR("Invalid parameter. Layers must be in increasing order");
            return HL_ERROR_INVALID_PARAMETER;
        }
        /* Make sure the ratio is power of 2 (required for H.264 SVC Baseline) */
        if (self->plugin->type == HL_CODEC_TYPE_H264_SVC) {
            uint32_t u_w_ratio = (u_width / p_layer_def->u_width);
            uint32_t u_h_ratio = (u_height / p_layer_def->u_height);
            if ((u_w_ratio & (u_w_ratio - 1)) != 0 || (u_h_ratio & (u_h_ratio - 1)) != 0) {
                HL_DEBUG_ERROR("Invalid parameter. Invalid image ratio(%ux%u)", u_w_ratio, u_h_ratio);
                return HL_ERROR_INVALID_PARAMETER;
            }
        }
    }
    p_layer_def = &self->layers[self->layers_active_count++];
    p_layer_def->u_index = (self->layers_active_count - 1);
    p_layer_def->u_width = u_width;
    p_layer_def->u_height = u_height;
    p_layer_def->i_qp = i_qp;
    p_layer_def->i_fps = i_fps;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_clear_layers(hl_codec_t* self)
{
    if (!self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    self->layers_active_count = 0;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_decode(hl_codec_t* self, const void* pc_data, hl_size_t u_data_size, hl_codec_result_t* p_result)
{
    if (!self || !self->plugin || !self->plugin->decode) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    return self->plugin->decode(self, pc_data, u_data_size, p_result);
}

HL_ERROR_T hl_codec_encode(hl_codec_t* self, const struct hl_frame_s* pc_frame, hl_codec_result_t* p_result)
{
    if (!self || !self->plugin || !self->plugin->decode) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    return self->plugin->encode(self, pc_frame, p_result);
}

HL_ERROR_T hl_codec_plugin_register(const hl_codec_plugin_def_t* plugin)
{
    hl_size_t i;
    if (!plugin) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    /* add or replace the plugin */
    for (i = 0; i<HL_CODEC_MAX_PLUGINS; i++) {
        if(!__hl_codec_plugins[i] || (__hl_codec_plugins[i] == plugin)) {
            __hl_codec_plugins[i] = plugin;
            HL_DEBUG_INFO("Registered codec: %s", plugin->description);
            return HL_ERROR_SUCCESS;
        }
    }

    HL_DEBUG_ERROR("There are already %d plugins.", HL_CODEC_MAX_PLUGINS);
    return HL_ERROR_OUTOFBOUND;
}

HL_ERROR_T hl_codec_plugin_unregister(const hl_codec_plugin_def_t* plugin)
{
    hl_size_t i;
    hl_bool_t found = hl_false;
    if (!plugin) {
        HL_DEBUG_ERROR("Invalid Parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    /* find the plugin to unregister */
    for (i = 0; i<HL_CODEC_MAX_PLUGINS && __hl_codec_plugins[i]; i++) {
        if(__hl_codec_plugins[i] == plugin) {
            __hl_codec_plugins[i] = hl_null;
            found = hl_true;
            break;
        }
    }

    /* compact */
    if (found) {
        for(; i<(HL_CODEC_MAX_PLUGINS - 1); i++) {
            if(__hl_codec_plugins[i+1]) {
                __hl_codec_plugins[i] = __hl_codec_plugins[i+1];
            }
            else {
                break;
            }
        }
        __hl_codec_plugins[i] = hl_null;
    }
    return (found ? HL_ERROR_SUCCESS : HL_ERROR_NOT_FOUND);
}

HL_ERROR_T hl_codec_plugin_find(HL_CODEC_TYPE_T type, const hl_codec_plugin_def_t** ppc_plugin)
{
    hl_size_t i;
    if (!ppc_plugin) {
        HL_DEBUG_ERROR("Invalid Parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    for (i = 0; i<HL_CODEC_MAX_PLUGINS && __hl_codec_plugins[i]; i++) {
        if	(__hl_codec_plugins[i]->type == type) {
            *ppc_plugin = __hl_codec_plugins[i];
            return HL_ERROR_SUCCESS;
        }
    }
    return HL_ERROR_NOT_FOUND;
}

HL_ERROR_T hl_codec_guess_best_bitrate(enum HL_VIDEO_MOTION_RANK_E e_mr, uint32_t u_width, uint32_t u_height, const struct hl_rational_s* p_fps, int64_t* p_bitrate)
{
    if (!p_bitrate || !p_fps || !p_fps->den) {
        HL_DEBUG_ERROR("Invalid Parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *p_bitrate = ((int64_t)(u_width * u_height * (p_fps->den/p_fps->num) * ((int32_t)e_mr) * 0.07) /*>> 10*/);
    return HL_ERROR_SUCCESS;
}

static hl_object_t* hl_codec_result_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_result_t *result = (hl_codec_result_t*)self;
    if (result) {

    }
    return self;
}
static hl_object_t* hl_codec_result_dtor(hl_object_t * self)
{
    hl_codec_result_t *result = (hl_codec_result_t*)self;
    if (result) {

    }
    return self;
}
static int hl_codec_result_cmp(const hl_object_t *_r1, const hl_object_t *_r2)
{
    return (int)((int*)_r1 - (int*)_r2);
}
static const hl_object_def_t hl_codec_result_def_s = {
    sizeof(hl_codec_result_t),
    hl_codec_result_ctor,
    hl_codec_result_dtor,
    hl_codec_result_cmp,
};
const hl_object_def_t *hl_codec_result_def_t = &hl_codec_result_def_s;
