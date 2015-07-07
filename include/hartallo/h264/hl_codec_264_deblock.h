#ifndef _HARTALLO_CODEC_264_DEBLOCK_H_
#define _HARTALLO_CODEC_264_DEBLOCK_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

HL_ERROR_T hl_codec_264_deblock_init_funcs();

HL_ERROR_T hl_codec_264_deblock_avc(struct hl_codec_264_s* p_codec);
// 8.7 Deblocking filter process
HL_ERROR_T hl_codec_264_deblock_avc_mb(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb);
// G.8.7.1 Deblocking filter process for Intra_Base prediction
HL_ERROR_T hl_codec_264_deblock_intra_base_svc(struct hl_codec_264_s* p_codec, int32_t currDQId);
// G.8.7.2 Deblocking filter process for target representations
HL_ERROR_T hl_codec_264_deblock_target_reps_svc(struct hl_codec_264_s* p_codec, int32_t currDQId);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_DEBLOCK_H_ */
