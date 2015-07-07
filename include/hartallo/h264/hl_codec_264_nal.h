#ifndef _HARTALLO_CODEC_264_NAL_H_
#define _HARTALLO_CODEC_264_NAL_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_defs.h"
#include "hartallo/hl_object.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;

typedef struct hl_codec_264_nal_s {
    HL_DECLARE_OBJECT;

    unsigned u_forbidden_zero_bit; // 1bit
    unsigned u_ref_idc; // 2bits
    HL_CODEC_264_NAL_TYPE_T e_type; // 5bits
}
hl_codec_264_nal_t;

#define HL_CODEC_264_NAL(self) ((hl_codec_264_nal_t*)(self))

#define HL_DECLARE_264_NAL hl_codec_264_nal_t __base_codec_264_nal__

HL_ERROR_T hl_codec_264_nal_init(hl_codec_264_nal_t* self, HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc);
HL_ERROR_T hl_codec_264_nal_decode(HL_CODEC_264_NAL_TYPE_T e_type, unsigned u_ref_idc, struct hl_codec_264_s* p_codec, hl_codec_264_nal_t** pp_nal);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_NAL_H_ */
