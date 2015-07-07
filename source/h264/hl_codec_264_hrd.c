#include "hartallo/h264/hl_codec_264_hrd.h"
#include "hartallo/hl_debug.h"

extern const hl_object_def_t *hl_codec_264_hrd_def_t;

HL_ERROR_T hl_codec_264_hrd_create(hl_codec_264_hrd_t** pp_hrd)
{
    if (!pp_hrd) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_hrd = hl_object_create(hl_codec_264_hrd_def_t);
    if (!*pp_hrd) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}


/*** OBJECT DEFINITION FOR "hl_codec_264_hrd_t" ***/
static hl_object_t* hl_codec_264_hrd_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_hrd_t *p_hrd = (hl_codec_264_hrd_t*)self;
    if (p_hrd) {

    }
    return self;
}
static hl_object_t* hl_codec_264_hrd_dtor(hl_object_t * self)
{
    hl_codec_264_hrd_t *p_hrd = (hl_codec_264_hrd_t*)self;
    if (p_hrd) {

    }
    return self;
}
static int hl_codec_264_hrd_cmp(const hl_object_t *_v1, const hl_object_t *_v2)
{
    return (int)((int*)_v1 - (int*)_v2);
}
static const hl_object_def_t hl_codec_264_hrd_def_s = {
    sizeof(hl_codec_264_hrd_t),
    hl_codec_264_hrd_ctor,
    hl_codec_264_hrd_dtor,
    hl_codec_264_hrd_cmp,
};
const hl_object_def_t *hl_codec_264_hrd_def_t = &hl_codec_264_hrd_def_s;
