#ifndef _HARTALLO_CODEC_264_PRED_INTRA_H_
#define _HARTALLO_CODEC_264_PRED_INTRA_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
enum HL_CODEC_264_I16x16_MODE_E;
enum HL_CODEC_264_I4x4_MODE_E;
enum HL_CODEC_264_INTRA_CHROMA_MODE_E;


HL_ERROR_T hl_codec_264_pred_intra_decode(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb);
void hl_codec_264_pred_intra_set_intra4x4_pred_mode(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb);


HL_ERROR_T hl_codec_264_pred_intra_get_neighbouring_samples_16x16L(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, const hl_pixel_t *p_SL, int32_t p[33]);
HL_ERROR_T hl_codec_264_pred_intra_get_neighbouring_samples_4x4L(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t luma4x4BlkIdx, int32_t* xO, int32_t* yO, const hl_pixel_t *p_SL, int32_t p[13]);
hl_size_t hl_codec_264_pred_intra_get_neighbouring_samples_C(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, const hl_pixel_t *p_SU, const hl_pixel_t *p_SV, int32_t pCb[33], int32_t pCr[33]);
void hl_codec_264_pred_intra_perform_prediction_4x4L(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t pred4x4L[4][4], const int32_t p[13], enum HL_CODEC_264_I4x4_MODE_E i4x4PredMode);
void hl_codec_264_pred_intra_perform_prediction_16x16L(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t pred16x16L[16][16], const int32_t p[33], enum HL_CODEC_264_I16x16_MODE_E i16x16PredMode);
void hl_codec_264_pred_intra_perform_prediction_chroma(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t predCb[16][16], int32_t predCr[16][16], const int32_t pCb[33], const int32_t pCr[33], enum HL_CODEC_264_INTRA_CHROMA_MODE_E iChomaPredMode);
void hl_codec_264_pred_intra_perform_prediction_modes_4x4(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_PRED_INTRA_H_ */
