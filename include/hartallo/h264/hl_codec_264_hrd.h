#ifndef _HARTALLO_CODEC_264_HRD_H_
#define _HARTALLO_CODEC_264_HRD_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"

HL_BEGIN_DECLS

struct hl_codec_264_hrd_s;

HL_ERROR_T hl_codec_264_hrd_create(struct hl_codec_264_hrd_s** pp_hrd);

typedef struct hl_codec_264_hrd_s {
    HL_DECLARE_OBJECT;

    uint32_t cpb_cnt_minus1; // range of 0 to 31
    unsigned bit_rate_scale:4;
    uint32_t bit_rate_value_minus1[32/*[0-cpb_cnt_minus1]*/];
    uint32_t cpb_size_scale;
    uint32_t cpb_size_value_minus1[32/*[0-cpb_cnt_minus1]*/];
    uint8_t cbr_flag[32/*[0-cpb_cnt_minus1]*/];
    unsigned initial_cpb_removal_delay_length_minus1:5;
    unsigned cpb_removal_delay_length_minus1:5;
    unsigned dpb_output_delay_length_minus1:5;
    unsigned time_offset_length:5;
}
hl_codec_264_hrd_t;

HL_END_DECLS


#endif /* #ifndef _HARTALLO_CODEC_264_HRD_H_ */
