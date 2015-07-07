#include "hartallo/h264/hl_codec_264_vui.h"
#include "hartallo/hl_debug.h"

extern const hl_object_def_t *hl_codec_264_vui_def_t;
extern const hl_object_def_t *hl_codec_264_vui_svc_def_t;

HL_ERROR_T hl_codec_264_vui_create(hl_codec_264_vui_t** pp_vui)
{
    if (!pp_vui) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_vui = hl_object_create(hl_codec_264_vui_def_t);
    if (!*pp_vui) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_vui_svc_create(hl_codec_264_vui_svc_t** pp_vui)
{
    if (!pp_vui) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_vui = hl_object_create(hl_codec_264_vui_svc_def_t);
    if (!*pp_vui) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}


/*** OBJECT DEFINITION FOR "hl_codec_264_vui_t" ***/
static hl_object_t* hl_codec_264_vui_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_vui_t *p_vui = (hl_codec_264_vui_t*)self;
    if (p_vui) {

    }
    return self;
}
static hl_object_t* hl_codec_264_vui_dtor(hl_object_t * self)
{
    hl_codec_264_vui_t *p_vui = (hl_codec_264_vui_t*)self;
    if (p_vui) {
        HL_OBJECT_SAFE_FREE(p_vui->p_hrd_nal);
        HL_OBJECT_SAFE_FREE(p_vui->p_hrd_vcl);
    }
    return self;
}
static int hl_codec_264_vui_cmp(const hl_object_t *_v1, const hl_object_t *_v2)
{
    return (int)((int*)_v1 - (int*)_v2);
}
static const hl_object_def_t hl_codec_264_vui_def_s = {
    sizeof(hl_codec_264_vui_t),
    hl_codec_264_vui_ctor,
    hl_codec_264_vui_dtor,
    hl_codec_264_vui_cmp,
};
const hl_object_def_t *hl_codec_264_vui_def_t = &hl_codec_264_vui_def_s;

/*** OBJECT DEFINITION FOR "hl_codec_264_vui_svc_t" ***/
static hl_object_t* hl_codec_264_vui_svc_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_vui_svc_t *p_vui_svc = (hl_codec_264_vui_svc_t*)self;
    if (p_vui_svc) {

    }
    return self;
}
static hl_object_t* hl_codec_264_vui_svc_dtor(hl_object_t * self)
{
    hl_codec_264_vui_svc_t *p_vui_svc = (hl_codec_264_vui_svc_t*)self;
    if (p_vui_svc) {
        hl_size_t i;
        for (i = 0; i < HL_CODEC_264_VUI_EXT_NUM_ENTRIES_MAX_COUNT; ++i) {
            HL_OBJECT_SAFE_FREE(p_vui_svc->hrds_nal[i]);
            HL_OBJECT_SAFE_FREE(p_vui_svc->hrds_vcl[i]);
        }
    }
    return self;
}
static int hl_codec_264_vui_svc_cmp(const hl_object_t *_v1, const hl_object_t *_v2)
{
    return (int)((int*)_v1 - (int*)_v2);
}
static const hl_object_def_t hl_codec_264_vui_svc_def_s = {
    sizeof(hl_codec_264_vui_svc_t),
    hl_codec_264_vui_svc_ctor,
    hl_codec_264_vui_svc_dtor,
    hl_codec_264_vui_svc_cmp,
};
const hl_object_def_t *hl_codec_264_vui_svc_def_t = &hl_codec_264_vui_svc_def_s;
