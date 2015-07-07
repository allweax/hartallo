#ifndef _HARTALLO_CODEC_264_FMO_H_
#define _HARTALLO_CODEC_264_FMO_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;

HL_ERROR_T hl_codec_264_fmo_read_mb_to_slice_group_map(struct hl_codec_264_s* p_codec);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_FMO_H_ */
