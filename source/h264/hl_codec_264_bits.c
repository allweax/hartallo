#include "hartallo/h264/hl_codec_264_bits.h"

extern const hl_object_def_t *hl_codec_264_bits_def_t;

HL_ERROR_T hl_codec_264_bits_create(hl_codec_264_bits_t** pp_bits, const void* pc_buff, hl_size_t u_buff_size)
{
    if (!pp_bits) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_bits = hl_object_create(hl_codec_264_bits_def_t);
    if (!*pp_bits) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    else {
        HL_ERROR_T err = hl_codec_264_bits_reset(*pp_bits, pc_buff, u_buff_size);
        if (err) {
            HL_OBJECT_SAFE_FREE(*pp_bits);
            return err;
        }

        return HL_ERROR_SUCCESS;
    }
}

HL_ERROR_T hl_codec_264_bits_create_2(hl_codec_264_bits_t** pp_bits)
{
    return hl_codec_264_bits_create(pp_bits, HL_NULL, 0);
}

static hl_object_t* hl_codec_264_bits_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_bits_t *p_bits = (hl_codec_264_bits_t*)self;
    if (p_bits) {

    }
    return self;
}
static hl_object_t* hl_codec_264_bits_dtor(hl_object_t * self)
{
    hl_codec_264_bits_t *p_bits = (hl_codec_264_bits_t*)self;
    if (p_bits) {

    }
    return self;
}
static int hl_codec_264_bits_cmp(const hl_object_t *_d1, const hl_object_t *_d2)
{
    return (int)((int*)_d1 - (int*)_d2);
}
static const hl_object_def_t hl_codec_264_bits_def_s = {
    sizeof(hl_codec_264_bits_t),
    hl_codec_264_bits_ctor,
    hl_codec_264_bits_dtor,
    hl_codec_264_bits_cmp,
};
const hl_object_def_t *hl_codec_264_bits_def_t = &hl_codec_264_bits_def_s;
