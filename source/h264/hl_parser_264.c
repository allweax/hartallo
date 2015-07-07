#include "hartallo/h264/hl_parser_264.h"
#include "hartallo/hl_option.h"
#include "hartallo/hl_debug.h"

static HL_ERROR_T _hl_parser_264_set_option(hl_parser_t* self, const hl_option_t* option)
{
    // Parameters validity alread tested by the caller ( hl_parser_set_option(...) ).

    HL_DEBUG_ERROR("Not implemented");
    return HL_ERROR_NOT_IMPLEMENTED;
}

HL_ERROR_T _hl_parser_264_find_bounds(hl_parser_t* self, const void* pc_indata, hl_size_t size_indata, hl_size_t *p_start, hl_size_t *p_end)
{
    // Parameters validity alread tested by the caller ( hl_parser_find_bounds(...) ).

    const uint8_t *_pc_indata = (const uint8_t*)pc_indata;
    register hl_size_t i;

    // find start
    // start code prefix for byte alignment = four-byte sequence	00000001
    // once aligned the start code prefix = three-byte sequence		000001
    for (i = 0; i<(size_indata - 3); i++) {
        if (_pc_indata[i] == 0x00 && _pc_indata[i+1] == 0x00 && _pc_indata[i+2] == 0x01) {
            i += 3;
            goto find_end;
        }
    }

    HL_DEBUG_ERROR("Failed to find the start of the NAL Unit");
    return HL_ERROR_NOT_FOUND;


find_end:
    *p_start = i;
    // find the end of the NAL
    for (; i< (size_indata - 3); i++) {
        if (_pc_indata[i] == 0x00 && _pc_indata[i+1] == 0x00 && _pc_indata[i+2] == 0x01) {
            *p_end = (_pc_indata[i - 1] == 0 ? (i - 2) : (i - 1));
            return HL_ERROR_SUCCESS;
        }
    }
    *p_end = size_indata;
    return HL_ERROR_SUCCESS;
}

static hl_object_t* hl_parser_264_ctor(hl_object_t * self, va_list * app)
{
    hl_parser_264_t *p_parser_264 = (hl_parser_264_t*)self;
    if (p_parser_264) {
    }
    return self;
}

static hl_object_t* hl_parser_264_dtor(hl_object_t * self)
{
    hl_parser_264_t *p_parser_264 = (hl_parser_264_t*)self;
    if (p_parser_264) {

    }
    return self;
}

static int hl_parser_264_cmp(const hl_object_t *_c1, const hl_object_t *_c2)
{
    return (int)((int*)_c1 - (int*)_c2);
}

static const hl_object_def_t hl_parser_264_def_s = {
    sizeof(hl_parser_264_t),
    hl_parser_264_ctor,
    hl_parser_264_dtor,
    hl_parser_264_cmp,
};
const hl_parser_plugin_def_t hl_parser_264_plugin_def_s = {
    &hl_parser_264_def_s,

    HL_PARSER_TYPE_H264,
    HL_MEDIA_TYPE_VIDEO,
    "H.264 AVC/SVC/MVC",

    _hl_parser_264_set_option,
    _hl_parser_264_find_bounds,
};
const hl_parser_plugin_def_t *hl_parser_264_plugin_def_t = &hl_parser_264_plugin_def_s;
