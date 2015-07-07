#ifndef _HARTALLO_CODEC_264_DECODE_SVC_H_
#define _HARTALLO_CODEC_264_DECODE_SVC_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

// G.8.1.3 Layer representation decoding processes
HL_ERROR_T hl_codec_264_decode_svc(struct hl_codec_264_s* p_codec);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_DECODE_SVC_H_ */
