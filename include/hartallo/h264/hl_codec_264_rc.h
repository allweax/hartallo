#ifndef _HARTALLO_CODEC_264_RC_H_
#define _HARTALLO_CODEC_264_RC_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"
#include "hartallo/h264/hl_codec_264_tables.h"

HL_BEGIN_DECLS

HL_ERROR_T hl_codec_264_rc_init(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rc_deinit(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rc_start_gop(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rc_end_gop(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rc_start_frame(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rc_store_slice_header_bits( struct hl_codec_264_s* p_codec, int32_t len );
HL_ERROR_T hl_codec_264_rc_rc_handle_mb( struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t* p_qp );
HL_ERROR_T hl_codec_264_rc_rc_store_mad_and_bitscount( struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t i_mad, int32_t i_hdr_bitscount,  int32_t i_data_bitscount);
HL_ERROR_T hl_codec_264_rc_end_frame(struct hl_codec_264_s* p_codec, int32_t nBits);
HL_ERROR_T hl_codec_264_rc_pict_start(struct hl_codec_264_s* p_codec);
HL_ERROR_T hl_codec_264_rc_pict_end(struct hl_codec_264_s* p_codec);

typedef struct hl_codec_264_rc_pict_xs;
typedef struct hl_codec_264_rc_gop_s;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_RC_H_ */
