#ifndef _HARTALLO_CODEC_264_H_
#define _HARTALLO_CODEC_264_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_codec.h"
#include "hartallo/h264/hl_codec_264_defs.h"

HL_BEGIN_DECLS

typedef struct hl_codec_264_s {
    HL_DECLARE_CODEC;

    struct hl_codec_s* pc_base;

    struct {
        enum HL_CODEC_264_NAL_TYPE_E e_nal_type;
        uint32_t i_nal_ref_idc;
        uint32_t svc_extension_flag;
        struct {
            uint32_t store_ref_base_pic_flag; // u(1)
            struct hl_codec_264_svc_dec_ref_base_pic_marking_xs xs_dec_ref_base_pic_marking; // dec_ref_base_pic_marking( )
        } prefix;
        union {
            struct {
                uint32_t idr_flag;// All u(1)
                uint32_t priority_id;// All u(6)
                uint32_t no_inter_layer_pred_flag;// All u(1)
                uint32_t dependency_id;// All u(3)
                uint32_t quality_id;// All u(4)
                uint32_t temporal_id;// All u(3)
                uint32_t use_ref_base_pic_flag;// All u(1)
                uint32_t discardable_flag;// All u(1)
                uint32_t output_flag;// All u(1)
                uint32_t reserved_three_2bits;// All u(2)

                int32_t DQId;
            } svc;
            struct {
                uint32_t NOT_IMPLEMENTED;
            } mvc;
        } ext;
    } nal_current;

    struct hl_codec_264_dpb_s* pc_dpb; // FIXME: remove
    struct hl_codec_264_poc_s* pc_poc; // FIXME: remove

    struct hl_codec_264_bits_s* pobj_bits;

    struct hl_memory_blocks_s* pobj_mem_blocks;

    struct {
        struct hl_codec_264_layer_s* pc_active; // "current" layer being decoded/encoded
        // const struct hl_codec_264_layer_s* pc_ref; // "reference" layer representation
        struct hl_codec_264_layer_s* p_list[HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT];

        int32_t currDQId;
        int32_t maxDQId;
        int32_t DQIdMin;
        int32_t DQIdMax;
        int32_t DependencyIdMax;
        int32_t dqIdList[HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT];
    } layers;

    struct {
        const struct hl_codec_264_nal_sps_s* pc_active;
        struct hl_codec_264_nal_sps_s* p_list[HL_CODEC_264_SPS_MAX_COUNT];
        hl_size_t u_p_list_count;
    } sps;

    struct {
        const struct hl_codec_264_nal_pps_s* pc_active;
        struct hl_codec_264_nal_pps_s* p_list[HL_CODEC_264_PPS_MAX_COUNT];
        hl_size_t u_p_list_count;
    } pps;

    struct {
        uint8_t* p_rbsp_bytes;
        hl_size_t i_rbsp_bytes_size;
        hl_size_t i_rbsp_bytes_num;
    } decoder;

    struct {
        hl_bool_t b_svc_enabled; // whether "SVC" option is enabled on this codec
        uint8_t* p_rbsp_bytes;
        hl_size_t i_rbsp_bytes_size;
        hl_size_t i_rbsp_bytes_num;
        hl_size_t i_rbsp_bytes_start; // Where to start writing bytes - FIXME: remove
        int32_t gop_left;
        int32_t i_qp;
        int32_t i_sps_active_id;
        int32_t i_pps_active_id;
        const struct hl_frame_video_s* pc_frame; // current frame being encoded

        int32_t (*fn_distortion_compute_4x4_u8)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride); // Function used to compute distortion

        uint8_t* p_hdr_bytes; // Active SPS and PPS bytes separated by a start code prefix (0x000001)
        hl_size_t u_hdr_bytes_size;

        HL_VIDEO_ENCODING_TYPE_T encoding_prev;
        HL_VIDEO_ENCODING_TYPE_T encoding_curr; // could be different than the value in the frame (e.g. "AUTO" vs "INTRA")

        struct {
            double d_lambda_mode; // Lagrange multiplier used to compute the cost in the RDO module (cost = D + (lambda * R)).
            double d_lambda_motion;
            struct hl_codec_264_bits_s* pobj_bits;
            uint8_t bits_buff[HL_CODEC_264_RDO_BUFFER_MAX_SIZE];
        } rdo; // FIXME: remove and replace by "slice_data".rdo

        struct {
            // FIXME: most of the next fields are not used by JM RC
            hl_bool_t b_enabled; // Whether RC module is enabled.
            hl_bool_t b_initialized; // Whether RC module is initialized.
            int64_t i_bitrate;
            int32_t i_fps;
            int32_t i_gop_idx; // GOP index.
            int64_t i_pict_mad; // Sum of Absolute MADs
            int32_t i_pict_bits_hdr; // Number of bits used to code the headers for this picture. Elements: Slice headers, MB structure (type, MVs...). It includes all the information except to the residual information.
            int32_t i_pict_bits_data; // Number of bits used to code the data for this current picture.
            int32_t i_np; // Number of P-Frames in the GOP.
            int32_t i_qp_p_avg_sum; // the sum of average QPs for all P-frames in the ith GOP.
            int32_t i_qp_p_avg_sum_acc; // tmp value for accumulation. Used to void half pictures.

            // FIXME: remove  all the next block or add #if HARTALLO_RC
            struct hl_codec_264_rc_gop_s* pc_gop_active;
            struct hl_codec_264_rc_gop_s* p_list_gops[HL_CODEC_264_RC_GOP_WINDOW_SIZE_MAX];
            hl_size_t u_p_list_gops_count;
            hl_size_t u_p_list_gops_idx;


            // JM
            int64_t bit_rate;
            double frame_rate;
            int qp;
            int bitdepth_luma_qp_scale; // FIXME: not updated
            int curr_frm_idx; // FIXME: not updated
            int no_frames; // FIXME: not update (may be =gop_size)
            int idr_period;
            int intra_period;
            int BasicUnit;
            int RDPictureDecision;
            int number;
            int SeinitialQP;
            int NumberofCodedMacroBlocks; // FIXME: not updated
            hl_size_t size;

            int (*updateQP)(struct hl_codec_264_s* p_codec, enum HL_CODEC_264_SLICE_TYPE_E SliceTypeModulo5, struct rc_quadratic *p_quad, struct rc_generic *p_gen, int topfield); // FIXME: not updated

            struct rc_generic   *p_rc_gen; // free()d using hl_codec_264_rc_deinit()
            struct rc_generic   *p_rc_gen_init, *p_rc_gen_best; // free()d using hl_codec_264_rc_deinit()
            struct rc_quadratic *p_rc_quad; // free()d using hl_codec_264_rc_deinit()
            struct rc_quadratic *p_rc_quad_init, *p_rc_quad_best; // free()d using hl_codec_264_rc_deinit()

        } rc;

        union {
            struct {
                unsigned NO_VAL:1;
            } DiamonSearch;
            struct {
                unsigned NO_VAL:1;
            } UMHexagonS;
        } me; // FIXME: remove. Use Patterns for Diamond search
    } encoder;

    struct {
        struct hl_asynctask_s** pp_list_tasks;
        hl_size_t u_list_tasks_count; // number of tasks

        struct hl_memory_blocks_s** pp_list_mem_blocks;
        hl_size_t u_list_mem_blocks_count; // number of mem_blocks (one or each thread/core)
    } threads;

    struct {
        HL_ALIGN(HL_ALIGN_V) uint8_t p[2/*luma=0,chroma=1*/][4][16];
        HL_ALIGN(HL_ALIGN_V) uint8_t q[2/*luma=0,chroma=1*/][4][16];
        HL_ALIGN(HL_ALIGN_V) uint8_t pf[2/*luma=0,chroma=1*/][3][16];
        HL_ALIGN(HL_ALIGN_V) uint8_t qf[2/*luma=0,chroma=1*/][3][16];
        HL_ALIGN(HL_ALIGN_V) int16_t filterSamplesFlag[2/*luma=0,chroma=1*/][2/*8x8 block index*/][8];
    } deblock;

    hl_bool_t b_started; // whether async threads started
    int32_t CurrPicNum; // part of the standard
    HL_CODEC_264_INPUT_TYPE_T input_type;

    hl_bool_t b_4pixels_aligned; // whether copying 4 pixels keep the alignment valid

    // FIXME: remove
    HL_ALIGN(HL_ALIGN_V) int32_t PixelMaxValueY[4]; // ((1 << BitDepthY) - 1)
    HL_ALIGN(HL_ALIGN_V) int32_t PixelMaxValueC[4]; // ((1 << BitDepthC) - 1)
}
hl_codec_264_t;

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_H_ */
