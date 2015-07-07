#ifndef _HARTALLO_CODEC_264_CAVLC_H_
#define _HARTALLO_CODEC_264_CAVLC_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

struct hl_codec_264_s;
struct hl_codec_264_bits_s;

extern struct hl_codec_264_cavlc_level_xs (*hl_codec_264_cavlc_levels)[7 /* HL_CAVLC_MAX_LEVEL_SUFFIX_LENGTH + 1 */][62546 /* HL_CAVLC_MAX_LEVEL_CODE + 1 */];

HL_ERROR_T hl_codec_264_cavlc_InitEncodingTable();
HL_ERROR_T hl_codec_264_cavlc_ReadTotalCoeffTrailingOnes(const struct hl_codec_264_s* pc_codec, uint32_t *TrailingOnes, uint32_t *TotalCoeff, int32_t nC);
HL_ERROR_T hl_codec_264_cavlc_ReadTotalCoeffTrailingOnesChromaDC(const struct hl_codec_264_s* pc_codec, uint32_t *TrailingOnes, uint32_t *TotalCoeff, int32_t nC);
HL_ERROR_T hl_codec_264_cavlc_ReadLevelPrefix(const struct hl_codec_264_s* pc_codec, int32_t *levelPrefix);
HL_ERROR_T hl_codec_264_cavlc_ReadTotalZeros(const struct hl_codec_264_s* pc_codec, int32_t *TotalZeros, int32_t TotalCoeff);
HL_ERROR_T hl_codec_264_cavlc_ReadTotalZerosChromaDC(const struct hl_codec_264_s* pc_codec, int32_t *TotalZeros, int32_t TotalCoeff);
HL_ERROR_T hl_codec_264_cavlc_ReadRunBefore(const struct hl_codec_264_s* pc_codec, int32_t *RunBefore, int32_t zerosLeft);
HL_ERROR_T hl_codec_264_cavlc_WriteTotalCoeffTrailingOnes(struct hl_codec_264_bits_s* p_bits, int32_t TrailingOnes, int32_t TotalCoeff, int32_t nC);
HL_ERROR_T hl_codec_264_cavlc_WriteTotalCoeffTrailingOnesChromaDC(struct hl_codec_264_bits_s* p_bits, int32_t TrailingOnes, int32_t TotalCoeff);
HL_ERROR_T hl_codec_264_cavlc_WriteTotalZeros(struct hl_codec_264_bits_s* p_bits, int32_t total_zeros, int32_t TotalCoeff);
HL_ERROR_T hl_codec_264_cavlc_WriteTotalZerosChromaDC(struct hl_codec_264_bits_s* p_bits, int total_zeros, int TotalCoeff);
HL_ERROR_T hl_codec_264_cavlc_WriteRunBefore(struct hl_codec_264_bits_s* p_bits, int32_t run_before, int32_t zerosLeft);

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_CAVLC_H_ */
