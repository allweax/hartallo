#ifndef _HARTALLO_CODEC_264_REFLIST_H_
#define _HARTALLO_CODEC_264_REFLIST_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;

HL_ERROR_T hl_codec_264_reflist_init(struct hl_codec_264_s* p_codec);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_REFLIST_H_ */
