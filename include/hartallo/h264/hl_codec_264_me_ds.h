#ifndef _HARTALLO_CODEC_264_ME_DS_H_
#define _HARTALLO_CODEC_264_ME_DS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

HL_ERROR_T hl_codec_264_me_ds_mb_find_best_cost(
    HL_IN struct hl_codec_264_mb_s* p_mb,
    HL_IN struct hl_codec_264_s* p_codec,
    HL_IN const struct hl_codec_264_me_part_xs *pc_part
);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_ME_DS_H_ */
