#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_nal.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_fmo.h"
#include "hartallo/h264/hl_codec_264_reflist.h"
#include "hartallo/h264/hl_codec_264_rbsp.h"
#include "hartallo/h264/hl_codec_264_rc.h"
#include "hartallo/h264/hl_codec_264_encode.h"
#include "hartallo/hl_option.h"
#include "hartallo/hl_asynctask.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_frame.h"
#include "hartallo/hl_debug.h"
#include "hartallo/hl_md5.h" // FIXME: remove
#include "hartallo/hl_string.h" // FIXME: remove

static HL_ERROR_T _hl_codec_264_start_threads(hl_codec_264_t* p_self);
static HL_ERROR_T _hl_codec_264_stop_threads(hl_codec_264_t* p_self);

HL_ERROR_T hl_codec_264_init_functions()
{
#if HL_HAVE_X86_INTRIN || HL_HAVE_X86_ASM
    extern HL_ERROR_T hl_x86_globals_init();
#endif
    extern HL_ERROR_T hl_codec_264_interpol_init_functions();
    extern HL_ERROR_T hl_codec_264_transf_init_funcs();
    extern HL_ERROR_T hl_codec_264_quant_init_funcs();
    extern HL_ERROR_T hl_codec_264_deblock_init_funcs();

    HL_ERROR_T err;

#if HL_HAVE_X86_INTRIN || HL_HAVE_X86_ASM
    err = hl_x86_globals_init();
    if (err) {
        return err;
    }
#endif

    err = hl_codec_264_interpol_init_functions();
    if (err) {
        return err;
    }
    err = hl_codec_264_transf_init_funcs();
    if (err) {
        return err;
    }
    err = hl_codec_264_quant_init_funcs();
    if (err) {
        return err;
    }
    err = hl_codec_264_deblock_init_funcs();
    if (err) {
        return err;
    }

    return err;
}

static HL_ERROR_T _hl_codec_264_set_option(hl_codec_t* _self, const hl_option_t* option)
{
    /* hl_codec_264_t* self = (hl_codec_264_t*)_self; */


    HL_DEBUG_ERROR("Not implemented");
    return HL_ERROR_NOT_IMPLEMENTED;
}

static HL_ERROR_T _hl_codec_264_decode(hl_codec_t* _self, const void* nal_ptr, hl_size_t nal_size, hl_codec_result_t *p_result)
{
    hl_codec_264_t* self = (hl_codec_264_t*)_self;
    const uint8_t* pc_nal_ptr = (const uint8_t*)nal_ptr;
    register hl_size_t i;
    hl_size_t i_rbsp_bytes_num = nal_size;
    hl_size_t i_nalunit_header_bytes = 1;
    hl_size_t i_xrbsp_bytes;
    HL_ERROR_T err;
    hl_codec_264_nal_t* p_nal = HL_NULL;

    if (p_result) {
        p_result->type = HL_CODEC_RESULT_TYPE_NONE;
    }

    if(!self || !nal_ptr || nal_size < 2) {
        HL_DEBUG_ERROR("Invalid parameter");
        err = HL_ERROR_INVALID_PARAMETER;
        goto bail;
    }

    // Make sure we're not switching from 'encoding...' to 'decoding...'.
    if (self->input_type) {
        if (self->input_type != HL_CODEC_264_INPUT_TYPE_DECODE) {
            HL_DEBUG_ERROR("Invalid operation");
            return HL_ERROR_INVALID_OPERATION;
        }
    }
    else {
        self->input_type = HL_CODEC_264_INPUT_TYPE_DECODE;
    }

    pc_nal_ptr = (const uint8_t*)nal_ptr;
    self->decoder.i_rbsp_bytes_num = 0;

    // 7.3.1 NAL unit syntax
    if (pc_nal_ptr[0] & 0x80) {
        HL_DEBUG_ERROR("forbidden_zero_bit not equal to zero");
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }
    self->nal_current.e_nal_type = (HL_CODEC_264_NAL_TYPE_T)(pc_nal_ptr[0] & 0x1F);
    self->nal_current.i_nal_ref_idc = (pc_nal_ptr[0] >> 5) & 0x03;

    // 7.3.2.12 Prefix NAL unit RBSP syntax
    if (self->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT || self->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION) {
        // svc_extension_flag u(1)
        self->nal_current.svc_extension_flag = (pc_nal_ptr[1] >> 7);
        if (self->nal_current.svc_extension_flag) {
            // nal_unit_header_svc_extension( ) /* specified in Annex G */
            // idr_flag All u(1)
            self->nal_current.ext.svc.idr_flag = (pc_nal_ptr[1] & 0x40) ? 1 : 0;
            // priority_id All u(6)
            self->nal_current.ext.svc.priority_id = (pc_nal_ptr[1] & 0x3F);
            // no_inter_layer_pred_flag All u(1)
            self->nal_current.ext.svc.no_inter_layer_pred_flag = (pc_nal_ptr[2] & 0x80) ? 1 : 0;
            // dependency_id All u(3)
            self->nal_current.ext.svc.dependency_id = (pc_nal_ptr[2] >> 4) & 0x07;
            // quality_id All u(4)
            self->nal_current.ext.svc.quality_id = (pc_nal_ptr[2] & 0x0F);
            // temporal_id All u(3)
            self->nal_current.ext.svc.temporal_id = (pc_nal_ptr[3] >> 5) & 0x07;
            // use_ref_base_pic_flag All u(1)
            self->nal_current.ext.svc.use_ref_base_pic_flag = (pc_nal_ptr[3] & 0x10) ? 1 : 0;
            // discardable_flag All u(1)
            self->nal_current.ext.svc.discardable_flag = (pc_nal_ptr[3] & 0x08) ? 1 : 0;
            // output_flag All u(1)
            self->nal_current.ext.svc.output_flag = (pc_nal_ptr[3] & 0x04) ? 1 : 0;
            // reserved_three_2bits All u(2)
            self->nal_current.ext.svc.reserved_three_2bits = (pc_nal_ptr[3] & 0x03);
            if (self->nal_current.ext.svc.reserved_three_2bits != 3) {
                // reserved_three_2bits shall be equal to 3. Other values of reserved_three_2bits may be specified in the future by
                // ITU-T | ISO/IEC. Decoders shall ignore the value of reserved_three_2bits.
                HL_DEBUG_WARN("reserved_three_2bits not equal to zero");
                err = HL_ERROR_INVALID_BITSTREAM;
                goto bail;
            }
            self->nal_current.ext.svc.DQId = (self->nal_current.ext.svc.dependency_id << 4) + self->nal_current.ext.svc.quality_id; // (G-61)

            i_nalunit_header_bytes = 4; /* 1+=3 */

            if (nal_size > i_nalunit_header_bytes) {
                // G.7.3.2.12.1 Prefix NAL unit SVC syntax
                // prefix_nal_unit_svc( )
                if (self->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT) {
                    if (self->nal_current.i_nal_ref_idc) {
                        err = hl_codec_264_bits_reset(self->pobj_bits, &pc_nal_ptr[i_nalunit_header_bytes], (nal_size - i_nalunit_header_bytes));
                        if (err) {
                            return err;
                        }
                        self->nal_current.prefix.store_ref_base_pic_flag = hl_codec_264_bits_read_u1(self->pobj_bits);
                        if ((self->nal_current.ext.svc.use_ref_base_pic_flag || self->nal_current.prefix.store_ref_base_pic_flag)
                                && self->nal_current.ext.svc.idr_flag) {
                            // G.7.3.3.5 Decoded reference base picture marking syntax
                            // dec_ref_base_pic_marking( )
                            err = hl_codec_264_rbsp_svc_dec_ref_base_pic_marking_read(self->pobj_bits, &self->nal_current.prefix.xs_dec_ref_base_pic_marking);
                            if (err) {
                                return err;
                            }
                        }
                        i_nalunit_header_bytes += (self->pobj_bits->pc_current - self->pobj_bits->pc_start); // useless, anyways no other function will be called
                    }
                }
            }
        }
        else {
            // nal_unit_header_mvc_extension( ) /* specified in Annex H */
            HL_DEBUG_ERROR("MVC extension not supported");
            err = HL_ERROR_INVALID_BITSTREAM;
            goto bail;
        }
    }

    // RBSP
    i_xrbsp_bytes = (i_rbsp_bytes_num + HL_BUFFER_PADDING_SIZE);
    if (self->decoder.i_rbsp_bytes_size < i_xrbsp_bytes) {
        if(!(self->decoder.p_rbsp_bytes = (uint8_t*)hl_memory_realloc(self->decoder.p_rbsp_bytes, i_xrbsp_bytes))) {
            HL_DEBUG_ERROR("Failed to allocate new buffer");
            self->decoder.i_rbsp_bytes_size = 0;
            err = HL_ERROR_OUTOFMEMMORY;
            goto bail;
        }
        self->decoder.i_rbsp_bytes_size = i_xrbsp_bytes;
    }

    // Set padding bytes to zeroes
    memset(self->decoder.p_rbsp_bytes + (i_rbsp_bytes_num - 1), 0, HL_BUFFER_PADDING_SIZE);

    for (i = i_nalunit_header_bytes; i<i_rbsp_bytes_num; ++i) {
        if((i+2) < i_rbsp_bytes_num && (pc_nal_ptr[i] == 0x00 && pc_nal_ptr[i + 1] == 0x00 && pc_nal_ptr[i + 2] == 0x03)) {
            self->decoder.p_rbsp_bytes[self->decoder.i_rbsp_bytes_num++] = pc_nal_ptr[i];
            self->decoder.p_rbsp_bytes[self->decoder.i_rbsp_bytes_num++] = pc_nal_ptr[i+1];
            i += 2;
            // emulation_prevention_three_byte...
        }
        else {
            self->decoder.p_rbsp_bytes[self->decoder.i_rbsp_bytes_num++] = pc_nal_ptr[i];
        }
    }

    // Start threads
    if (!self->b_started) {
        err = _hl_codec_264_start_threads(self);
        if (err) {
            goto bail;
        }
    }

    // Create memory blocks
    if (!self->pobj_mem_blocks) {
        err = hl_memory_blocks_create(&self->pobj_mem_blocks);
        if (err) {
            return err;
        }
    }

    // Create or reset bitstream reader
    if (!self->pobj_bits) {
        err = hl_codec_264_bits_create(&self->pobj_bits, self->decoder.p_rbsp_bytes, self->decoder.i_rbsp_bytes_num);
        if (err) {
            return err;
        }
    }
    else {
        err = hl_codec_264_bits_reset(self->pobj_bits, self->decoder.p_rbsp_bytes, self->decoder.i_rbsp_bytes_num);
        if (err) {
            goto bail;
        }
    }

    // Decode NAL Unit
    switch (self->nal_current.e_nal_type) {
    case HL_CODEC_264_NAL_TYPE_ACCESS_UNIT_DELIMITER: {
        goto bail;
    }

    /* Prefix (SVC) */
    case HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT: {
        // Already decoded (see above)
        break;
    }
    /* non-VCL NALs*/
    /* No picture decoding */
    case HL_CODEC_264_NAL_TYPE_PPS:
    case HL_CODEC_264_NAL_TYPE_SPS:
    case HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET: {
        err = hl_codec_264_nal_decode(self->nal_current.e_nal_type, self->nal_current.i_nal_ref_idc, self, &p_nal);
        if (err) {
            goto bail;
        }

        switch(p_nal->e_type) {
        case HL_CODEC_264_NAL_TYPE_SPS:
        case HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET: {
            hl_codec_264_nal_sps_t* p_sps = (hl_codec_264_nal_sps_t*)p_nal;
            HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
                self->sps.p_list,
                self->sps.u_p_list_count,
                HL_CODEC_264_SPS_MAX_COUNT,
                p_sps,
                p_sps->seq_parameter_set_id, err);

            if (err) {
                goto bail;
            }
            p_nal = HL_NULL; // stolen
            break;
        }
        case HL_CODEC_264_NAL_TYPE_PPS: {
            hl_codec_264_nal_pps_t* p_pps = (hl_codec_264_nal_pps_t*)p_nal;
            HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
                self->pps.p_list,
                self->pps.u_p_list_count,
                HL_CODEC_264_PPS_MAX_COUNT,
                p_pps,
                p_pps->pic_parameter_set_id, err);

            if (err) {
                goto bail;
            }
            p_nal = HL_NULL; // stolen
            break;
        }
        }// switch-2
        break;
    }

    /* VCL NALs*/
    /* Picture decoding take place here! */
    case HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE:
    case HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_A_NON_IDR_PICTURE:
    case HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION: {
        // Output will be a frame for AVC and frame"S" for SVC
        hl_codec_264_layer_t* pc_layer;
        err = hl_codec_264_nal_decode(self->nal_current.e_nal_type, self->nal_current.i_nal_ref_idc, self, &p_nal);
        pc_layer = self->layers.pc_active;
        if (err) {
            pc_layer->i_mb_decode_count = 0;
            pc_layer->i_mb_read_count = 0;
            goto bail;
        }

        /* FIXME: dump to file */
        if (0) {
            if (pc_layer->b_got_frame) {
                static FILE* p_file = HL_NULL;
                static const int32_t __max_frames = 800;
                static const int32_t __write2file = 1;
                static const int32_t __print_md5 =  1;
                // static int count = 0;
                if (!p_file && pc_layer->i_pict_decode_count < __max_frames) {
                    p_file = fopen("./reconstructed.yuv", "wb+");
                }
                if (/*pc_layer->DQId == 16 && pc_layer->i_pict_decode_count >= 0*/__print_md5) {
                    hl_md5string_t md5_result;
                    hl_md5context_t md5_ctx;
                    hl_md5digest_t md5_digest;
                    hl_md5compute(pc_layer->pc_fs_curr->p_pict->pc_data_y, /*((2 * 16) + (0 * 16 * 176))*/pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y, &md5_result);
                    HL_DEBUG_INFO("Y[DQID=%d, Pict=%d] MD5=%s", pc_layer->DQId, pc_layer->i_pict_decode_count, md5_result);
                    hl_md5compute(pc_layer->pc_fs_curr->p_pict->pc_data_u, /*((0 * 8) + (1 * 8 * 88))*/pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, &md5_result);
                    HL_DEBUG_INFO("U[DQID=%d, Pict=%d] MD5=%s", pc_layer->DQId, pc_layer->i_pict_decode_count, md5_result);
                    hl_md5compute(pc_layer->pc_fs_curr->p_pict->pc_data_v, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, &md5_result);
                    HL_DEBUG_INFO("V[DQID=%d, Pict=%d] MD5=%s", pc_layer->DQId, pc_layer->i_pict_decode_count, md5_result);
                    hl_md5init(&md5_ctx);
                    hl_md5update(&md5_ctx, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y);
                    hl_md5update(&md5_ctx, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c);
                    hl_md5update(&md5_ctx, pc_layer->pc_fs_curr->p_pict->pc_data_v, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c);
                    hl_md5final(md5_digest, &md5_ctx);
                    hl_str_from_hex(md5_digest, HL_MD5_DIGEST_SIZE, md5_result);
                    HL_DEBUG_INFO("YUV[DQID=%d, Pict=%d] MD5=%s\n", pc_layer->DQId, pc_layer->i_pict_decode_count, md5_result);
                }
                if (p_file && __write2file) {
                    /*
                    hl_size_t i;
                    for (i = 0; i < self->pc_fs_curr->pc_dpb->u_buff_size_fs_y; ++i) {
                        fwrite(&self->pc_fs_curr->p_pict->pc_data_y[i], 1, 1, p_file);
                    }
                    for (i = 0; i < self->pc_fs_curr->pc_dpb->u_buff_size_fs_c; ++i) {
                        fwrite(&self->pc_fs_curr->p_pict->pc_data_u[i], 1, 1, p_file);
                    }
                    for (i = 0; i < self->pc_fs_curr->pc_dpb->u_buff_size_fs_c; ++i) {
                        fwrite(&self->pc_fs_curr->p_pict->pc_data_v[i], 1, 1, p_file);
                    }*/
                    fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_y, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y, p_file);
                    fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_u, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, p_file);
                    fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_v, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, p_file);
                    if (pc_layer->i_pict_decode_count == __max_frames) {
                        fclose(p_file);
                        p_file = HL_NULL;
                    }
                }
            }
        }


        if (pc_layer->b_got_frame) {
            // Reset values
            pc_layer->i_mb_decode_count = 0;
            pc_layer->i_mb_read_count = 0;
            pc_layer->b_got_frame = HL_FALSE;

            // Forward decoded data to the end-user
            if (!pc_layer->SVCExtFlag  || (self->pc_base->dqid_min < 0 || pc_layer->DQId >= self->pc_base->dqid_min) && (self->pc_base->dqid_max < 0 || pc_layer->DQId <= self->pc_base->dqid_max)) {
                p_result->type = HL_CODEC_RESULT_TYPE_DATA;
                p_result->data_ptr = (const uint8_t*)pc_layer->pc_fs_curr->p_pict->pc_data_y;
                p_result->data_size = (pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y + (pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c << 1)); // YUV420
                p_result->width = pc_layer->pc_fs_curr->p_pict->uWidthL;
                p_result->height = pc_layer->pc_fs_curr->p_pict->uHeightL;
                p_result->dqid = pc_layer->DQId;
            }
        }
        break;
    }
    default: {
        HL_DEBUG_ERROR("Parsing NAL type = %d not implemented yet", (int32_t)self->nal_current.e_nal_type);
        err = HL_ERROR_NOT_IMPLEMENTED;
        goto bail;
    }
    }// switch-1

bail:
    HL_OBJECT_SAFE_FREE(p_nal);
    return err;
}

static HL_ERROR_T _hl_codec_264_encode(hl_codec_t* _p_self, const hl_frame_t* _pc_frame, hl_codec_result_t *p_result)
{
    hl_codec_264_t* p_self = (hl_codec_264_t*)_p_self;
    const hl_frame_video_t* pc_frame = (const hl_frame_video_t*)_pc_frame;
    hl_codec_264_layer_t* pc_layer;
    hl_codec_264_slice_t *pc_slice;
    hl_codec_264_nal_sps_t* pc_sps;
    hl_codec_264_nal_pps_t* pc_pps;
    hl_codec_264_encode_slice_data_t* pc_esd;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_bool_t b_sps_changed , b_pps_changed, b_encoding_changed, b_slice_qp_delta_changed, b_first_time, b_last_layer;
    hl_size_t i, u_svc_nal_prefix_length = 0;
    int32_t i_tmp;
    const hl_layer_def_xt* pc_layer_def = HL_NULL; // active SVC layer. Valid for SVC stream only
    static const hl_size_t __scp_size = sizeof(HL_CODEC_264_START_CODE_PREFIX);

    HL_DEBUG_INFO("Encode frame");

    if (!p_self || !_pc_frame || !p_result) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // Make sure video frame is valid
    if (_pc_frame->type != HL_MEDIA_TYPE_VIDEO ||
            !pc_frame->data_ptr[0] || !pc_frame->data_ptr[1] || !pc_frame->data_ptr[2] ||
            !pc_frame->data_width[0] || (pc_frame->data_width[0] & 15) ||
            !pc_frame->data_width[1] || (pc_frame->data_width[1] & 3) ||
            !pc_frame->data_width[2] || (pc_frame->data_width[2] & 3) ||
            !pc_frame->data_height[0] || (pc_frame->data_height[0] & 15) ||
            !pc_frame->data_height[1] || (pc_frame->data_height[1] & 3) ||
            !pc_frame->data_height[2] || (pc_frame->data_height[2] & 3)
       ) {
        HL_DEBUG_ERROR("Invalid video frame");
        return HL_ERROR_INVALID_FORMAT;
    }

    b_sps_changed = HL_FALSE;
    b_pps_changed = HL_FALSE;
    b_encoding_changed = HL_FALSE;
    b_slice_qp_delta_changed = HL_FALSE;
    b_last_layer = HL_FALSE;

    // Make sure we're not switching from 'decoding...' to 'encoding...'.
    if (p_self->input_type) {
        if (p_self->input_type != HL_CODEC_264_INPUT_TYPE_ENCODE) {
            HL_DEBUG_ERROR("Invalid operation");
            return HL_ERROR_INVALID_OPERATION;
        }
        b_first_time = HL_FALSE;
    }
    else {
        p_self->input_type = HL_CODEC_264_INPUT_TYPE_ENCODE;
        b_first_time = HL_TRUE;
        if (p_self->pc_base->layers_active_count > 1) {
            HL_DEBUG_INFO("Initializing H.264 SVC encoder with %d spatial layers", p_self->pc_base->layers_active_count);
            p_self->encoder.b_svc_enabled = HL_TRUE;
        }
        else {
            HL_DEBUG_INFO("Initializing H.264 AVC encoder");
        }
        p_self->encoder.i_qp = p_self->pc_base->qp;
        p_self->encoder.rc.i_bitrate = p_self->pc_base->rc_bitrate;
        p_self->encoder.rc.i_fps = p_self->pc_base->fps.den / p_self->pc_base->fps.num;
    }

    if (p_self->encoder.b_svc_enabled) {
        // Find the active layer definition
        for (i = 0; i < p_self->pc_base->layers_active_count; ++i) {
            if (p_self->pc_base->layers[i].u_width == pc_frame->data_width[0] && p_self->pc_base->layers[i].u_height == pc_frame->data_height[0]) {
                pc_layer_def = &p_self->pc_base->layers[i];
                break;
            }
        }
        if (!pc_layer_def) {
            HL_DEBUG_ERROR("Failed to find layer definition with size = (%ux%u). Did you forget to call hl_codec_add_layer()?", pc_frame->data_width[0], pc_frame->data_height[0]);
            return HL_ERROR_NOT_FOUND;
        }
    }
    b_last_layer = (!pc_layer_def || pc_layer_def->u_index == (p_self->pc_base->layers_active_count - 1));

    // Create RDO bits
    if (!p_self->encoder.rdo.pobj_bits) {
        err = hl_codec_264_bits_create_2(&p_self->encoder.rdo.pobj_bits);
        if (err) {
            return err;
        }
    }

    // Create memory blocks
    if (!p_self->pobj_mem_blocks) {
        err = hl_memory_blocks_create(&p_self->pobj_mem_blocks);
        if (err) {
            return err;
        }
    }

    // Start threads
    if (!p_self->b_started) {
        err = _hl_codec_264_start_threads(p_self);
        if (err) {
            return err;
        }
    }

    p_self->encoder.pc_frame = pc_frame;
    p_result->type = HL_CODEC_RESULT_TYPE_NONE; // no result
    p_self->nal_current.i_nal_ref_idc = 1; // always used as reference
    // p_self->nal_current.svc_extension_flag = p_self->encoder.b_svc_enabled ? 1 : 0;

    if (b_first_time || !p_self->encoder.p_rbsp_bytes) {
        hl_size_t u_xrbsp_bytes, u_max_width, u_max_height;
        if (p_self->encoder.b_svc_enabled) {
            // SVC encoding
            u_max_width = p_self->pc_base->layers[p_self->pc_base->layers_active_count - 1].u_width;
            u_max_height = p_self->pc_base->layers[p_self->pc_base->layers_active_count - 1].u_height;
        }
        else {
            // AVC encoding
            u_max_width = pc_frame->data_width[0];
            u_max_height = pc_frame->data_height[0];
        }

        // RBSP
        u_xrbsp_bytes = ((u_max_width * u_max_height * 3) >> 1) + HL_CODEC_264_SLICEHDR_MAX_BYTES_COUNT; // I_PCM + SliceHdr + emulation_bytes
        if (p_self->encoder.i_rbsp_bytes_size < u_xrbsp_bytes) {
            if(!(p_self->encoder.p_rbsp_bytes = (uint8_t*)hl_memory_realloc(p_self->encoder.p_rbsp_bytes, (u_xrbsp_bytes + HL_BUFFER_PADDING_SIZE)))) {
                HL_DEBUG_ERROR("Failed to allocate new buffer");
                p_self->encoder.i_rbsp_bytes_size = 0;
                err = HL_ERROR_OUTOFMEMMORY;
                goto bail;
            }
            p_self->encoder.i_rbsp_bytes_size = u_xrbsp_bytes;
            memset(p_self->encoder.p_rbsp_bytes + (u_xrbsp_bytes - 1), 0, HL_BUFFER_PADDING_SIZE); // Set padding bytes to zeroes
        }
    }

    // Create bitstream reader
    if (!p_self->pobj_bits) {
        err = hl_codec_264_bits_create(&p_self->pobj_bits, p_self->encoder.p_rbsp_bytes, p_self->encoder.i_rbsp_bytes_size);
        if (err) {
            return err;
        }
    }

    // Compute "currDQId"
    if (p_self->encoder.b_svc_enabled) {
        p_self->layers.currDQId = (int32_t)(pc_layer_def->u_index/*dependency_id*/ << 4) + 0/*quality_id*/;
    }
    else {
        p_self->layers.currDQId = 0;
    }

    // Create current Layer representation
    pc_layer = p_self->layers.p_list[p_self->layers.currDQId];
    if (!pc_layer) {
        err = hl_codec_264_layer_create(&pc_layer, p_self->layers.currDQId);
        if (err) {
            goto bail;
        }
        p_self->layers.p_list[p_self->layers.currDQId] = pc_layer;
    }
    pc_layer->b_new_pict = HL_TRUE; // Always new picture
    /*if (pc_layer->DQId) { // FIXME: SVC layer not multithreaded
    	pc_layer->encoder.i_slice_count = 1;
    }
    else {*/
    pc_layer->encoder.i_slice_count = p_self->threads.u_list_tasks_count > 0 ? (int32_t)p_self->threads.u_list_tasks_count : 1; // FIXME: must be based on "thread_count" and "frame_size"
    //}
    p_self->layers.pc_active = pc_layer;
    p_self->pc_dpb = pc_layer->pobj_dpb;
    p_self->pc_poc = pc_layer->pobj_poc;

    // ==Sequence Parameter Set (SPS)== //
    p_self->encoder.i_sps_active_id = (pc_layer->DQId >> 4);
    pc_sps = p_self->sps.p_list[p_self->encoder.i_sps_active_id];
    if (!pc_sps) {
        hl_codec_264_nal_sps_t* p_sps = HL_NULL;
        // Create the SPS
        err = hl_codec_264_nal_sps_create(p_self->nal_current.i_nal_ref_idc, pc_layer->DQId > 0 ? HL_CODEC_264_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET : HL_CODEC_264_NAL_TYPE_SPS, &p_sps);
        if (err) {
            return err;
        }
        p_sps->seq_parameter_set_id = p_self->encoder.i_sps_active_id;
        pc_sps = p_sps;

        // Add the SPS to the list
        HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
            p_self->sps.p_list,
            p_self->sps.u_p_list_count,
            HL_CODEC_264_SPS_MAX_COUNT,
            p_sps,
            p_sps->seq_parameter_set_id, err);

        if (err) {
            HL_OBJECT_SAFE_FREE(p_sps);
            return err;
        }
        b_sps_changed = HL_TRUE;
    }
    p_self->sps.pc_active = pc_sps;

    // ==Picture Parameter Set (PPS)== //
    p_self->encoder.i_pps_active_id = (pc_layer->DQId >> 4);
    pc_pps = p_self->pps.p_list[p_self->encoder.i_pps_active_id];
    if (!pc_pps) {
        hl_codec_264_nal_pps_t* p_pps = HL_NULL;
        // Create the PPS
        err = hl_codec_264_nal_pps_create(p_self->nal_current.i_nal_ref_idc, HL_CODEC_264_NAL_TYPE_PPS, &p_pps);
        if (err) {
            return err;
        }
        p_pps->pic_parameter_set_id = p_self->encoder.i_pps_active_id;
        p_pps->seq_parameter_set_id = p_self->encoder.i_sps_active_id;
        pc_pps = p_pps;

        // Add the PPS to the list
        HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
            p_self->pps.p_list,
            p_self->pps.u_p_list_count,
            HL_CODEC_264_PPS_MAX_COUNT,
            p_pps,
            p_pps->pic_parameter_set_id, err);

        if (err) {
            HL_OBJECT_SAFE_FREE(p_pps);
            return err;
        }
        b_pps_changed = HL_TRUE;
    }
    p_self->pps.pc_active = pc_pps;

    // Write SPS and PPS bytes
    if (b_sps_changed || b_pps_changed) {
        uint8_t sps_bytes[HL_CODEC_264_SPS_MAX_BYTES_COUNT], pps_bytes[HL_CODEC_264_PPS_MAX_BYTES_COUNT];
        hl_size_t sps_bytes_count, pps_bytes_count, u;
# define WRITE_SCP(p_bits) \
			(p_bits)->pc_current[0] = HL_CODEC_264_START_CODE_PREFIX[0], \
			(p_bits)->pc_current[1] = HL_CODEC_264_START_CODE_PREFIX[1], \
			(p_bits)->pc_current[2] = HL_CODEC_264_START_CODE_PREFIX[2], \
			(p_bits)->pc_current+=3; \
 
        // Encode all SPSs (all layers) and align on 8-bytes boundary
        err = hl_codec_264_bits_reset(p_self->pobj_bits, sps_bytes, HL_CODEC_264_SPS_MAX_BYTES_COUNT);
        if (err) {
            return err;
        }

        for (u = 0; u < p_self->sps.u_p_list_count; ++u) {
            WRITE_SCP(p_self->pobj_bits);
            err = hl_codec_264_nal_sps_encode(p_self->sps.p_list[u], p_self);
            if (err) {
                return err;
            }
        }
        sps_bytes_count = (p_self->pobj_bits->pc_current - p_self->pobj_bits->pc_start);

        // Encode all PPSs (all layers) and align on 8-bytes boundary
        err = hl_codec_264_bits_reset(p_self->pobj_bits, pps_bytes, HL_CODEC_264_PPS_MAX_BYTES_COUNT);
        if (err) {
            return err;
        }
        for (u = 0; u < p_self->pps.u_p_list_count; ++u) {
            WRITE_SCP(p_self->pobj_bits);
            err = hl_codec_264_nal_pps_encode(p_self->pps.p_list[u], p_self->pps.p_list[u]->seq_parameter_set_id, p_self);
            if (err) {
                return err;
            }
        }
        pps_bytes_count = (p_self->pobj_bits->pc_current - p_self->pobj_bits->pc_start);

        p_self->encoder.u_hdr_bytes_size = (sps_bytes_count + pps_bytes_count);
        if (!(p_self->encoder.p_hdr_bytes = hl_memory_realloc(p_self->encoder.p_hdr_bytes, p_self->encoder.u_hdr_bytes_size))) {
            p_self->encoder.u_hdr_bytes_size = 0;
            return HL_ERROR_OUTOFMEMMORY;
        }
        memcpy(p_self->encoder.p_hdr_bytes, sps_bytes, sps_bytes_count);
        memcpy(&p_self->encoder.p_hdr_bytes[sps_bytes_count], pps_bytes, pps_bytes_count);

        // Forward the pointer to the base codec to allowend-user to have access to it
        p_self->pc_base->hdr_bytes = p_self->encoder.p_hdr_bytes;
        p_self->pc_base->hdr_bytes_count = p_self->encoder.u_hdr_bytes_size;
        p_result->type |= HL_CODEC_RESULT_TYPE_HDR; // signal header data have been updated
    }

    // Reset bytes
    if (!pc_layer_def || pc_layer_def->u_index == 0) {
        // SVC disabled or encoding base layer
        p_self->encoder.i_rbsp_bytes_start = 0;
    }
    // else append bytes
    if (p_self->encoder.i_rbsp_bytes_start >= p_self->encoder.i_rbsp_bytes_size) {
        HL_DEBUG_ERROR("Buffer too short");
        return HL_ERROR_TOOSHORT;
    }
    err = hl_codec_264_bits_reset(p_self->pobj_bits, &p_self->encoder.p_rbsp_bytes[p_self->encoder.i_rbsp_bytes_start], (p_self->encoder.i_rbsp_bytes_size - p_self->encoder.i_rbsp_bytes_start));
    if (err) {
        return err;
    }

    // Override encoding type
    p_self->encoder.encoding_curr = pc_frame->encoding;
    if (p_self->encoder.gop_left <= 0 && pc_frame->encoding != HL_VIDEO_ENCODING_TYPE_RAW) {
        p_self->encoder.encoding_curr = HL_VIDEO_ENCODING_TYPE_INTRA;
        if (b_last_layer) {
            p_self->encoder.gop_left = _p_self->gop_size;
        }
    }
    else if (pc_frame->encoding == HL_VIDEO_ENCODING_TYPE_AUTO) {
        p_self->encoder.encoding_curr = HL_VIDEO_ENCODING_TYPE_INTER;
    }
    else if (pc_frame->encoding == HL_VIDEO_ENCODING_TYPE_INTRA) {
        // Caller is trying to force a keyframe
        p_self->encoder.gop_left = _p_self->gop_size; // reset "gop_left"
    }
    b_encoding_changed = (p_self->encoder.encoding_prev != p_self->encoder.encoding_curr);

    // Init RC (Rate Control) module
    // FIXME: per
    if (p_self->encoder.gop_left == _p_self->gop_size) {
        p_self->encoder.rc.b_enabled = (p_self->pc_base->rc_bitrate > 0);
        if (p_self->encoder.rc.b_enabled) {
            if (!p_self->encoder.rc.b_initialized) {
                err = hl_codec_264_rc_init(p_self);
                if (err) {
                    return err;
                }
            }
            err = hl_codec_264_rc_start_gop(p_self);
            if (err) {
                return err;
            }
        }
    }

    // RC update QP
    // Slice header must be defined
    if (p_self->encoder.rc.b_enabled) {
        err = hl_codec_264_rc_start_frame(p_self);
        if (err) {
            return err;
        }
    }

    // Encode (and escape) the frame (multi-threaded function)
    err = hl_codec_264_encode_frame(
              p_self,
              pc_frame,
              b_encoding_changed, b_sps_changed, b_pps_changed);
    if (err) {
        return err;
    }

    // Write slices
    i_tmp = (p_self->threads.u_list_tasks_count >= 2) ? 1/* FakeSliceTakePosZero */ : 0/*SingleThreadedMeansNoFakeSliceAtPosZero*/;
    for (; i_tmp <= pc_layer->encoder.i_slice_count; ++i_tmp) {
        pc_esd = pc_layer->encoder.p_list_esd[i_tmp];
        if (pc_esd) {
            i = (pc_esd->pobj_bits->pc_current - pc_esd->pobj_bits->pc_start);
            if (i) {
                if ((p_self->pobj_bits->pc_current + (i + __scp_size)) >= p_self->pobj_bits->pc_end) {
                    HL_DEBUG_ERROR("Buffer too short");
                    return HL_ERROR_TOOSHORT;
                }
                if (pc_esd->i_mb_start) { // up to the caller to add the first SCP
                    p_self->pobj_bits->pc_current[0] = HL_CODEC_264_START_CODE_PREFIX[0];
                    p_self->pobj_bits->pc_current[1] = HL_CODEC_264_START_CODE_PREFIX[1];
                    p_self->pobj_bits->pc_current[2] = HL_CODEC_264_START_CODE_PREFIX[2];
                    p_self->pobj_bits->pc_current += 3;
                }
                hl_memory_copy(p_self->pobj_bits->pc_current, pc_esd->pobj_bits->pc_start, i);
                p_self->pobj_bits->pc_current += i;
            }
        }
    }

    // Default slice = fake slice
    pc_slice = pc_layer->encoder.p_list_esd[0/* __u_idx_fake */]->pc_slice;

    ////== Slice (/!\for now we only support a single slice per frame) ==//
    //// Set current slice idx
    //if (pc_layer->i_mb_write_count == 0) { // first time to try to read macroblocks
    //    pc_layer->u_list_slices_idx = 0; // reset to allow overriding
    //}
    //else {
    //    pc_layer->u_list_slices_idx = (pc_layer->u_list_slices_idx + 1) % HL_CODEC_264_SLICES_MAX_COUNT;
    //}

    //// create slice object
    //pc_slice = pc_layer->p_list_slices[pc_layer->u_list_slices_idx];
    //if (!pc_slice) {
    //    hl_codec_264_slice_t *p_slice;
    //    err = hl_codec_264_slice_create(&p_slice);
    //    if (err) {
    //        return err;
    //    }
    //    pc_slice = p_slice;
    //    pc_slice->u_idx = pc_layer->u_list_slices_idx;
    //    HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
    //        pc_layer->p_list_slices,
    //        pc_layer->u_list_slices_count,
    //        HL_CODEC_264_SLICES_MAX_COUNT,
    //        p_slice,
    //        p_slice->u_idx, err);

    //    if (err) {
    //        HL_OBJECT_SAFE_FREE(p_slice);
    //        return err;
    //    }
    //}
    //// set current slice
    //pc_layer->pc_slice_curr = pc_slice;

    //// Create slice header
    //if (!pc_slice->p_header) {
    //    // Slice type will be update later depending on the "encoding type"
    //    err = hl_codec_264_nal_slice_header_create(p_self->nal_current.i_nal_ref_idc, HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION, &pc_slice->p_header);
    //    if (err) {
    //        return err;
    //    }
    //    if (pc_layer->DQId > 0) {
    //        pc_slice->p_header->ext.svc.scan_idx_start = 0;
    //        pc_slice->p_header->ext.svc.scan_idx_end = 15;
    //        pc_slice->p_header->ext.svc.adaptive_base_mode_flag = 1;
    //        pc_slice->p_header->ext.svc.adaptive_motion_prediction_flag = 1;
    //        pc_slice->p_header->ext.svc.adaptive_residual_prediction_flag = 1;
    //    }
    //}

    //// Set common values
    //pc_slice->p_header->pc_pps = pc_pps;
    //pc_slice->p_header->pic_parameter_set_id = pc_pps->pic_parameter_set_id;
    //if (pc_sps->frame_mbs_only_flag) {
    //    pc_slice->p_header->field_pic_flag = 0;
    //}

    ////** Slice header **//
    //switch (p_self->encoder.encoding_curr) {
    //case HL_VIDEO_ENCODING_TYPE_RAW: {
    //    pc_layer->i_pict_encode_count = 0; // reset
    //    HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc = 0;
    //    HL_CODEC_264_NAL(pc_slice->p_header)->e_type = HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE;
    //    pc_slice->p_header->first_mb_in_slice = 0;
    //    pc_slice->p_header->slice_type = HL_CODEC_264_SLICE_TYPE_I;
    //    pc_slice->p_header->frame_num = pc_layer->i_pict_encode_count;
    //    pc_slice->p_header->num_ref_idx_active_override_flag = 0;
    //    pc_slice->p_header->pic_order_cnt_lsb = 0;
    //    pc_slice->p_header->idr_pic_id = pc_layer->encoder.i_idr_pic_id;
    //    pc_slice->p_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = 0;
    //    pc_slice->p_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = 0;
    //    break;
    //}
    //case HL_VIDEO_ENCODING_TYPE_INTRA: {
    //    HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc = 1;
    //    HL_CODEC_264_NAL(pc_slice->p_header)->e_type =
    //        pc_layer->DQId > 0 ? HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION : HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE;
    //    pc_slice->p_header->first_mb_in_slice = 0;
    //    pc_slice->p_header->slice_type = HL_CODEC_264_SLICE_TYPE_I;
    //    pc_slice->p_header->frame_num = pc_layer->i_pict_encode_count;
    //    pc_slice->p_header->num_ref_idx_active_override_flag = 0;
    //    pc_slice->p_header->pic_order_cnt_lsb = 0;
    //    pc_slice->p_header->idr_pic_id = pc_layer->encoder.i_idr_pic_id;
    //    pc_slice->p_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = 0;
    //    pc_slice->p_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = 0;
    //    i_tmp = (p_self->encoder.rc.b_enabled ? p_self->encoder.rc.qp : p_self->encoder.i_qp) - 26 - pc_slice->p_header->pc_pps->pic_init_qp_minus26;
    //    b_slice_qp_delta_changed = (pc_slice->p_header->slice_qp_delta != i_tmp);
    //    pc_slice->p_header->slice_qp_delta = i_tmp;
    //    break;
    //}
    //case HL_VIDEO_ENCODING_TYPE_INTER: {
    //    HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc = 1;
    //    HL_CODEC_264_NAL(pc_slice->p_header)->e_type =
    //        pc_layer->DQId > 0 ? HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION : HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_A_NON_IDR_PICTURE;
    //    pc_slice->p_header->first_mb_in_slice = 0;
    //    pc_slice->p_header->slice_type = HL_CODEC_264_SLICE_TYPE_P;
    //    pc_slice->p_header->frame_num = pc_layer->i_pict_encode_count;
    //    pc_slice->p_header->num_ref_idx_active_override_flag = 1;
    //    pc_slice->p_header->pic_order_cnt_lsb = 0;
    //    pc_slice->p_header->idr_pic_id = pc_layer->encoder.i_idr_pic_id;
    //    pc_slice->p_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = 0;
    //    pc_slice->p_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = 0;
    //    i_tmp = (p_self->encoder.rc.b_enabled ? p_self->encoder.rc.qp : p_self->encoder.i_qp) - 26 - pc_slice->p_header->pc_pps->pic_init_qp_minus26;
    //    b_slice_qp_delta_changed = (pc_slice->p_header->slice_qp_delta != i_tmp);
    //    pc_slice->p_header->slice_qp_delta = i_tmp;
    //    break;
    //}
    //}

    // Update current NAL with the default slice header
    //p_self->nal_current.e_nal_type = HL_CODEC_264_NAL(pc_slice->p_header)->e_type;
    //p_self->nal_current.i_nal_ref_idc = HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc;
    //p_self->nal_current.svc_extension_flag = (pc_layer->DQId > 0);

    //// Encode slice header
    //err = hl_codec_264_nal_slice_header_encode(pc_slice->p_header, p_self, (b_encoding_changed || b_sps_changed || b_pps_changed || b_slice_qp_delta_changed));
    //if (err) {
    //    return err;
    //}

    //// Init current Layer representation now that the slice header is correctly encoded
    //err = hl_codec_264_layer_init(pc_layer, pc_slice, p_self);
    //if (err) {
    //    return err;
    //}

    //// Init variables depending on SPS
    //if (b_sps_changed || b_pps_changed) {
    //    p_self->sps.pc_active = pc_slice->p_header->pc_pps->pc_sps;
    //    // Init DPB
    //    err = hl_codec_264_dpb_init(p_self->pc_dpb, p_self->sps.pc_active);
    //    if (err) {
    //        return err;
    //    }
    //    // Allocate Macroblocks
    //    if (pc_layer->u_list_macroblocks_count < p_self->sps.pc_active->uPicSizeInMapUnits) {
    //        HL_LIST_STATIC_SAFE_FREE_OBJECTS(pc_layer->pp_list_macroblocks, pc_layer->u_list_macroblocks_count); // Must not realloc as the list contains objects
    //        if (!(pc_layer->pp_list_macroblocks = hl_memory_calloc(p_self->sps.pc_active->uPicSizeInMapUnits, sizeof(hl_codec_264_mb_t*)))) {
    //            pc_layer->u_list_macroblocks_count = 0;
    //            return HL_ERROR_OUTOFMEMMORY;
    //        }
    //        pc_layer->u_list_macroblocks_count = p_self->sps.pc_active->uPicSizeInMapUnits;
    //    }
    //    // FIXME: is "MaxPicNum" needed?
    //    p_self->pc_poc->MaxFrameNum = p_self->pc_poc->MaxPicNum = p_self->sps.pc_active->uMaxFrameNum;
    //}

    //if (pc_layer->b_new_pict) {
    //    // map current FS memory to the right place in the DPB
    //    err = hl_codec_264_dpb_map_current(p_self, p_self->pc_dpb);
    //    if (err) {
    //        return err;
    //    }
    //}

    //// 8.2.1 Decoding process for picture order count
    //// Must be called even if not new pict (e.g. for BOT, TOP)
    //err = hl_codec_264_poc_decode(p_self->pc_poc, p_self);
    //if (err) {
    //    return err;
    //}

    //// 8.2.2 Decoding process for macroblock to slice group map
    //err = hl_codec_264_fmo_read_mb_to_slice_group_map(p_self);
    //if (err) {
    //    return err;
    //}

    //// 8.2.4 Decoding process for reference picture lists construction
    //err = hl_codec_264_reflist_init(p_self);
    //if (err) {
    //    return err;
    //}

    //// Encode slice data
    //err = hl_codec_264_nal_slice_data_encode(pc_slice, p_self);
    //if (err) {
    //    return err;
    //}




    // Escape any SCP
    p_self->encoder.i_rbsp_bytes_num = (p_self->pobj_bits->pc_current - p_self->pobj_bits->pc_start);
    //for (i = (p_self->encoder.i_rbsp_bytes_start + u_svc_nal_prefix_length), zeroBytesCount = 0; i<p_self->encoder.i_rbsp_bytes_num; ++i) {
    //    if (zeroBytesCount == 2) {
    //        if (p_self->encoder.p_rbsp_bytes[i]==0x01 || (p_self->encoder.p_rbsp_bytes[i]==0x00 && i+1 < p_self->encoder.i_rbsp_bytes_num && p_self->encoder.p_rbsp_bytes[i]==0x01)) {
    //            // TODO: not fully tested code
    //            HL_DEBUG_INFO("zeroBytesCount == 2");
    //            if ((p_self->encoder.i_rbsp_bytes_num + 1) >= p_self->encoder.i_rbsp_bytes_size) {
    //                HL_DEBUG_ERROR("Memory too short"); // Must never happen
    //                return HL_ERROR_TOOSHORT;
    //            }
    //            hl_memory_copy(&p_self->encoder.p_rbsp_bytes[i + 1], &p_self->encoder.p_rbsp_bytes[i], (p_self->encoder.i_rbsp_bytes_num - i + 1));
    //            p_self->encoder.i_rbsp_bytes_num++;
    //            p_self->encoder.p_rbsp_bytes[i++] = 0x03; //emulation_prevention_three_byte
    //        }
    //        zeroBytesCount = 0;
    //    }

    //    zeroBytesCount = p_self->encoder.p_rbsp_bytes[i] ? 0 : (zeroBytesCount + 1);
    //}
    p_self->encoder.i_rbsp_bytes_start += p_self->encoder.i_rbsp_bytes_num; // move index to the next position where to write data

    // Update some values now that the encoding succeed
    if (b_last_layer) {
        p_self->encoder.encoding_prev = p_self->encoder.encoding_curr;
    }

    // 8.2.5 Decoded reference picture marking process
    err = hl_codec_264_dpb_add_decoded(p_self);
    if (err) {
        return err;
    }

    // Transfer encoded frames to the result
    if (b_last_layer) { // write encoded data only if SVC is disabled or we encoded the last layer
        p_result->type |= HL_CODEC_RESULT_TYPE_DATA;
        p_result->data_ptr = p_self->encoder.p_rbsp_bytes;
        p_result->data_size = p_self->encoder.i_rbsp_bytes_start;
        --p_self->encoder.gop_left;
        p_self->encoder.i_rbsp_bytes_start = 0;
    }
    else {
        // Write SCP (Start Code Prefix) between this frame and the next one
        if ((p_self->pobj_bits->pc_end - p_self->pobj_bits->pc_current) <= 3) {
            HL_DEBUG_ERROR("Too short");
            return HL_ERROR_TOOSHORT;
        }
        p_self->pobj_bits->pc_current[0] = HL_CODEC_264_START_CODE_PREFIX[0];
        p_self->pobj_bits->pc_current[1] = HL_CODEC_264_START_CODE_PREFIX[1];
        p_self->pobj_bits->pc_current[2] = HL_CODEC_264_START_CODE_PREFIX[2];
        p_self->pobj_bits->pc_current += 3;
        p_self->encoder.i_rbsp_bytes_start += 3;
    }

    // Rate Control
    if (p_self->encoder.rc.b_enabled) {
        // FIXME: not correct for SVC (layers are accumulated)
        int32_t nBits = (int32_t)(((p_self->pobj_bits->pc_current - p_self->pobj_bits->pc_start) << 3) + (7 - p_self->pobj_bits->i_bits_count));
        err = hl_codec_264_rc_end_frame(p_self, nBits);
        if (err) {
            return err;
        }
        if (p_self->encoder.gop_left <= 0) {
            err = hl_codec_264_rc_end_gop(p_self);
            if (err) {
                return err;
            }
        }
    }

bail:
    return err;
}

static HL_ERROR_T _hl_codec_264_start_threads(hl_codec_264_t* p_self)
{
    hl_size_t i;
    int32_t i_cores_count;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if (p_self->b_started) {
        return HL_ERROR_SUCCESS;
    }

    i_cores_count = hl_cpu_get_cores_count();

    // Clip the number of threads
    if (p_self->pc_base->threads_count <= 0) {
        p_self->pc_base->threads_count = 1;
    }
    else if (p_self->pc_base->threads_count > i_cores_count) {
        // p_self->pc_base->threads_count = i_cores_count; // FIXME: clip on (MAX_SLICE_NUM - 1/*FaceSlice*/)
    }

    HL_DEBUG_INFO("Number of threads = %d", p_self->pc_base->threads_count);

    if (p_self->pc_base->threads_count > 1) {
        // memory blocks
        HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_self->threads.pp_list_mem_blocks, p_self->threads.u_list_mem_blocks_count);
        if (!(p_self->threads.pp_list_mem_blocks = hl_memory_calloc(p_self->pc_base->threads_count, sizeof(struct hl_memory_blocks_s*)))) {
            p_self->threads.pp_list_mem_blocks = 0;
            return HL_ERROR_OUTOFMEMMORY;
        }
        p_self->threads.u_list_mem_blocks_count = p_self->pc_base->threads_count;

        // threads
        HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_self->threads.pp_list_tasks, p_self->threads.u_list_tasks_count);
        if (!(p_self->threads.pp_list_tasks = hl_memory_calloc(p_self->pc_base->threads_count, sizeof(struct hl_asynctask_s*)))) {
            p_self->threads.pp_list_tasks = 0;
            return HL_ERROR_OUTOFMEMMORY;
        }
        p_self->threads.u_list_tasks_count = p_self->pc_base->threads_count;
    }

    for (i = 0; i < p_self->threads.u_list_tasks_count; ++i) {
        // memory blocks
        err = hl_memory_blocks_create(&p_self->threads.pp_list_mem_blocks[i]);
        if (err) {
            return err;
        }

        // threads
        err = hl_asynctask_create(&p_self->threads.pp_list_tasks[i]);
        if (err) {
            return err;
        }
        err = hl_asynctask_set_affinity(p_self->threads.pp_list_tasks[i], (int32_t)i % i_cores_count);
        if (err) {
            return err;
        }
        err = hl_asynctask_start(p_self->threads.pp_list_tasks[i]);
        if (err) {
            return err;
        }
    }

    p_self->b_started = HL_TRUE;
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _hl_codec_264_stop_threads(hl_codec_264_t* p_self)
{
    if (!p_self->b_started) {
        return HL_ERROR_SUCCESS;
    }

    // stop() and free() threads
    HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_self->threads.pp_list_tasks, p_self->threads.u_list_tasks_count);

    p_self->b_started = HL_FALSE;
    return HL_ERROR_SUCCESS;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_t" ***/
static hl_object_t* hl_codec_264_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_t *p_codec_264 = (hl_codec_264_t*)self;
    if (p_codec_264) {
        // Common
        p_codec_264->pc_base = (struct hl_codec_s*)self;
        p_codec_264->b_4pixels_aligned = ((sizeof(hl_pixel_t) << 2) == HL_ALIGN_V);
        p_codec_264->PixelMaxValueY[0] = p_codec_264->PixelMaxValueY[1] = p_codec_264->PixelMaxValueY[2] = p_codec_264->PixelMaxValueY[3] = 255; // ((1 << BitDepthY) - 1)
        p_codec_264->PixelMaxValueC[0] = p_codec_264->PixelMaxValueC[1] = p_codec_264->PixelMaxValueC[2] = p_codec_264->PixelMaxValueC[3] = 255; // ((1 << BitDepthC) - 1)
    }
    return self;
}

static hl_object_t* hl_codec_264_dtor(hl_object_t * self)
{
    hl_codec_264_t *p_codec_264 = (hl_codec_264_t*)self;
    if (p_codec_264) {
        hl_size_t count;
        /* Stop threads */
        _hl_codec_264_stop_threads(p_codec_264);
        /* Common */
        HL_OBJECT_SAFE_FREE(p_codec_264->pobj_bits);
        HL_OBJECT_SAFE_FREE(p_codec_264->pobj_mem_blocks);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec_264->sps.p_list, p_codec_264->sps.u_p_list_count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec_264->pps.p_list, p_codec_264->pps.u_p_list_count);
        HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_codec_264->threads.pp_list_tasks, p_codec_264->threads.u_list_tasks_count);
        HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_codec_264->threads.pp_list_mem_blocks, p_codec_264->threads.u_list_mem_blocks_count);
        count = HL_CODEC_264_SVC_SCALABLE_LAYERS_MAX_COUNT;
        HL_LIST_STATIC_SAFE_FREE_OBJECTS(p_codec_264->layers.p_list, count);

        /* Decoder */
        HL_SAFE_FREE(p_codec_264->decoder.p_rbsp_bytes);

        /* Encoder */
        HL_SAFE_FREE(p_codec_264->encoder.p_rbsp_bytes);
        HL_SAFE_FREE(p_codec_264->encoder.p_hdr_bytes);
        HL_OBJECT_SAFE_FREE(p_codec_264->encoder.rdo.pobj_bits);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec_264->encoder.rc.p_list_gops, p_codec_264->encoder.rc.u_p_list_gops_count);
        hl_codec_264_rc_deinit(p_codec_264); // free RC resources
    }
    return self;
}

static int hl_codec_264_cmp(const hl_object_t *_c1, const hl_object_t *_c2)
{
    return (int)((int*)_c1 - (int*)_c2);
}

static const hl_object_def_t hl_codec_264_def_s = {
    sizeof(hl_codec_264_t),
    hl_codec_264_ctor,
    hl_codec_264_dtor,
    hl_codec_264_cmp,
    HL_TRUE, // align struct to make sure members will also be aligned
};
const hl_codec_plugin_def_t hl_codec_264_plugin_def_s = {
    &hl_codec_264_def_s,

    HL_CODEC_TYPE_H264,
    HL_MEDIA_TYPE_VIDEO,
    "H.264 AVC/SVC/MVC",

    _hl_codec_264_set_option,
    _hl_codec_264_decode,
    _hl_codec_264_encode,
};
const hl_codec_plugin_def_t *hl_codec_264_plugin_def_t = &hl_codec_264_plugin_def_s;
