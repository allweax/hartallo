#include "hartallo/h264/hl_codec_264_encode.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_fmo.h"
#include "hartallo/h264/hl_codec_264_reflist.h"
#include "hartallo/h264/hl_codec_264_rbsp.h"
#include "hartallo/hl_md5.h" // FIXME: remove
#include "hartallo/hl_string.h" // FIXME: remove
#include "hartallo/hl_frame.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_asynctask.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_debug.h"

static const uint32_t __u_idx_fake = 0;
static const int32_t __i_mb_start_fake = 0;
static const int32_t __i_mb_end_fake = 1;
static const int32_t __i_coreid_invalid = -1;

HL_ASYNC_CALL_DIRECT
static HL_ERROR_T _hl_codec_264_encode_slice(
    hl_codec_264_t* p_self,
    uint32_t u_idx,
    int32_t i_mb_start, int32_t i_mb_end,
    hl_bool_t b_encoding_changed, hl_bool_t b_sps_changed, hl_bool_t b_pps_changed,
    int32_t i_core_id);
HL_ASYNC_CALL_INDIRECT
static HL_ERROR_T _hl_codec_264_encode_slice_async(
    const hl_asynctoken_param_xt* pc_params);

static HL_ERROR_T _hl_codec_264_encode_slice_data_create(hl_codec_264_encode_slice_data_t** pp_esd, hl_size_t u_rbsp_bytes_size)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    extern const hl_object_def_t *hl_codec_264_encode_slice_data_def_t;
    if (!pp_esd) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_esd = hl_object_create(hl_codec_264_encode_slice_data_def_t);
    if (!*pp_esd) {
        return HL_ERROR_OUTOFMEMMORY;
    }

    err = hl_codec_264_bits_create(&(*pp_esd)->rdo.pobj_bits, (*pp_esd)->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
    if (err) {
        goto bail;
    }

    if (!((*pp_esd)->p_rbsp_bytes = hl_memory_malloc((u_rbsp_bytes_size + HL_BUFFER_PADDING_SIZE)))) {
        HL_DEBUG_ERROR("Failed to allocate %u bytes", u_rbsp_bytes_size);
        err = HL_ERROR_OUTOFMEMMORY;
        goto bail;
    }
    (*pp_esd)->u_rbsp_bytes_size = u_rbsp_bytes_size;
    err = hl_codec_264_bits_create(&(*pp_esd)->pobj_bits, (*pp_esd)->p_rbsp_bytes, (*pp_esd)->u_rbsp_bytes_size);
    if (err) {
        goto bail;
    }

    (*pp_esd)->i_qp = -1;

bail:
    if (err) {
        HL_OBJECT_SAFE_FREE(*pp_esd);
    }
    return err;
}

HL_ASYNC_CALL_INDIRECT
static HL_ERROR_T _hl_codec_264_encode_slice_async(
    const hl_asynctoken_param_xt* pc_params)
{
    int32_t i_core_id;

    hl_codec_264_t* p_codec = HL_ASYNCTASK_GET_PARAM(pc_params[0].pc_param_ptr, hl_codec_264_t*);
    uint32_t u_idx = HL_ASYNCTASK_GET_PARAM(pc_params[1].pc_param_ptr, uint32_t);
    int32_t i_mb_start = HL_ASYNCTASK_GET_PARAM(pc_params[2].pc_param_ptr, int32_t);
    int32_t i_mb_end = HL_ASYNCTASK_GET_PARAM(pc_params[3].pc_param_ptr, int32_t);
    hl_bool_t b_encoding_changed = HL_ASYNCTASK_GET_PARAM(pc_params[4].pc_param_ptr, hl_bool_t);
    hl_bool_t b_sps_changed = HL_ASYNCTASK_GET_PARAM(pc_params[5].pc_param_ptr, hl_bool_t);
    hl_bool_t b_pps_changed = HL_ASYNCTASK_GET_PARAM(pc_params[6].pc_param_ptr, hl_bool_t);

    i_core_id = hl_thread_get_core_id();

    // HL_DEBUG_INFO("_hl_codec_264_encode_slice_async(core_id=%d, u_idx=%u, i_mb_start=%d, i_mb_end=%d)", i_core_id, u_idx, i_mb_start, i_mb_end);

    return _hl_codec_264_encode_slice(p_codec,
                                      u_idx,
                                      i_mb_start, i_mb_end,
                                      b_encoding_changed, b_sps_changed, b_pps_changed,
                                      i_core_id);
}

HL_ASYNC_CALL_DIRECT
static HL_ERROR_T _hl_codec_264_encode_slice(
    hl_codec_264_t* p_self,
    uint32_t u_idx,
    int32_t i_mb_start, int32_t i_mb_end,
    hl_bool_t b_encoding_changed, hl_bool_t b_sps_changed, hl_bool_t b_pps_changed,
    int32_t i_core_id)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer;
    hl_codec_264_slice_t *pc_slice;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_pps_t* pc_pps;
    hl_codec_264_encode_slice_data_t *pc_esd;
    int32_t i_mb_count, i_tmp;
    hl_bool_t b_slice_qp_delta_changed, b_fake_slice, b_multithreaded;
    hl_size_t u_svc_nal_prefix_length;

    if (u_idx >= HL_CODEC_264_SLICES_MAX_COUNT) {
        HL_DEBUG_ERROR("u_idx(%u) out of bound(0-%u)", u_idx, (HL_CODEC_264_SLICES_MAX_COUNT - 1));
        return HL_ERROR_OUTOFBOUND;
    }
    if ((i_mb_count = (i_mb_end - i_mb_start)) <= 0) {
        HL_DEBUG_ERROR("[i_mb_start(%d) - i_mb_end(%d)] invalid", i_mb_start, i_mb_end);
        return HL_ERROR_OUTOFBOUND;
    }

    u_svc_nal_prefix_length = 0;

    pc_layer = p_self->layers.pc_active;
    pc_pps = p_self->pps.pc_active;
    pc_sps = pc_pps->pc_sps;
    b_slice_qp_delta_changed = HL_FALSE;
    b_multithreaded = p_self->threads.u_list_tasks_count >= 2;
    b_fake_slice = (u_idx == __u_idx_fake && b_multithreaded); // fake slices used for multi-threading only

    // create slice object
    pc_slice = pc_layer->p_list_slices[u_idx];
    if (!pc_slice) {
        hl_codec_264_slice_t *p_slice;
        err = hl_codec_264_slice_create(&p_slice);
        if (err) {
            return err;
        }
        pc_slice = p_slice;
        pc_slice->u_idx = u_idx;
        HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
            pc_layer->p_list_slices,
            pc_layer->u_list_slices_count,
            HL_CODEC_264_SLICES_MAX_COUNT,
            p_slice,
            p_slice->u_idx, err);

        if (err) {
            HL_OBJECT_SAFE_FREE(p_slice);
            return err;
        }
    }

    // Create slice header
    if (!pc_slice->p_header) {
        // Slice type will be update later depending on the "encoding type"
        err = hl_codec_264_nal_slice_header_create(p_self->nal_current.i_nal_ref_idc, HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION, &pc_slice->p_header);
        if (err) {
            return err;
        }
        if (pc_layer->DQId > 0) {
            pc_slice->p_header->ext.svc.scan_idx_start = 0;
            pc_slice->p_header->ext.svc.scan_idx_end = 15;
            pc_slice->p_header->ext.svc.adaptive_base_mode_flag = 1;
            pc_slice->p_header->ext.svc.adaptive_motion_prediction_flag = 1;
            pc_slice->p_header->ext.svc.adaptive_residual_prediction_flag = 1;
        }
    }

    // Set common values
    pc_slice->p_header->pc_pps = pc_pps;
    pc_slice->p_header->pic_parameter_set_id = pc_pps->pic_parameter_set_id;
    if (pc_sps->frame_mbs_only_flag) {
        pc_slice->p_header->field_pic_flag = 0;
    }

    // Create the ESD (Encode Slice Data)
    pc_esd = pc_layer->encoder.p_list_esd[u_idx];
    if (!pc_esd) {
        hl_size_t u_rbsp_bytes_size = (i_mb_count << 8) + HL_CODEC_264_SLICEHDR_MAX_BYTES_COUNT;
        hl_codec_264_encode_slice_data_t *p_esd;
        err = _hl_codec_264_encode_slice_data_create(&p_esd, u_rbsp_bytes_size);
        if (err) {
            return err;
        }
        pc_esd = p_esd;
        HL_LIST_STATIC_ADD_OBJECT_AT_IDX(
            pc_layer->encoder.p_list_esd,
            pc_layer->encoder.u_p_list_esd_count,
            HL_CODEC_264_SLICES_MAX_COUNT,
            p_esd,
            u_idx, err);

        if (err) {
            HL_OBJECT_SAFE_FREE(p_esd);
            return err;
        }
    }
    else {
        hl_codec_264_bits_reset(pc_esd->rdo.pobj_bits, pc_esd->rdo.bits_buff, HL_CODEC_264_RDO_BUFFER_MAX_SIZE);
        hl_codec_264_bits_reset(pc_esd->pobj_bits, pc_esd->p_rbsp_bytes, pc_esd->u_rbsp_bytes_size);
    }
    pc_esd->pc_slice = pc_slice;
    pc_esd->i_mb_start = i_mb_start;
    pc_esd->i_mb_end = i_mb_end;
    pc_esd->pc_mem_blocks = p_self->pobj_mem_blocks;
    if (i_core_id != __i_coreid_invalid) {
        // multi-threaded
        if (i_core_id >= (int32_t)p_self->threads.u_list_mem_blocks_count) {
            HL_DEBUG_WARN("i_core_id=%d out of bound", i_core_id);
        }
        else {
            pc_esd->pc_mem_blocks = p_self->threads.pp_list_mem_blocks[i_core_id];
        }
    }

    //** Slice header **//
    switch (p_self->encoder.encoding_curr) {
    case HL_VIDEO_ENCODING_TYPE_RAW: {
        pc_layer->i_pict_encode_count = 0; // reset
        HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc = p_self->nal_current.i_nal_ref_idc;
        HL_CODEC_264_NAL(pc_slice->p_header)->e_type = HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE;
        pc_slice->p_header->first_mb_in_slice = i_mb_start;
        pc_slice->p_header->slice_type = HL_CODEC_264_SLICE_TYPE_I;
        pc_slice->p_header->frame_num = pc_layer->i_pict_encode_count;
        pc_slice->p_header->num_ref_idx_active_override_flag = 0;
        pc_slice->p_header->pic_order_cnt_lsb = 0;
        pc_slice->p_header->idr_pic_id = pc_layer->encoder.i_idr_pic_id;
        pc_slice->p_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = 0;
        pc_slice->p_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = 0;
        break;
    }
    case HL_VIDEO_ENCODING_TYPE_INTRA: {
        HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc = p_self->nal_current.i_nal_ref_idc;
        HL_CODEC_264_NAL(pc_slice->p_header)->e_type =
            pc_layer->DQId > 0 ? HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION : HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE;
        pc_slice->p_header->first_mb_in_slice = i_mb_start;
        pc_slice->p_header->slice_type = HL_CODEC_264_SLICE_TYPE_I;
        pc_slice->p_header->frame_num = pc_layer->i_pict_encode_count;
        pc_slice->p_header->num_ref_idx_active_override_flag = 0;
        pc_slice->p_header->pic_order_cnt_lsb = 0;
        pc_slice->p_header->idr_pic_id = pc_layer->encoder.i_idr_pic_id;
        pc_slice->p_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = 0;
        pc_slice->p_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = 0;
        i_tmp = (p_self->encoder.rc.b_enabled ? p_self->encoder.rc.qp : p_self->encoder.i_qp) - 26 - pc_slice->p_header->pc_pps->pic_init_qp_minus26;
        b_slice_qp_delta_changed = (pc_slice->p_header->slice_qp_delta != i_tmp);
        pc_slice->p_header->slice_qp_delta = i_tmp;
        break;
    }
    case HL_VIDEO_ENCODING_TYPE_INTER: {
        HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc = p_self->nal_current.i_nal_ref_idc;
        HL_CODEC_264_NAL(pc_slice->p_header)->e_type =
            pc_layer->DQId > 0 ? HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION : HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_A_NON_IDR_PICTURE;
        pc_slice->p_header->first_mb_in_slice = i_mb_start;
        pc_slice->p_header->slice_type = HL_CODEC_264_SLICE_TYPE_P;
        pc_slice->p_header->frame_num = pc_layer->i_pict_encode_count;
        pc_slice->p_header->num_ref_idx_active_override_flag = 1;
        pc_slice->p_header->pic_order_cnt_lsb = 0;
        pc_slice->p_header->idr_pic_id = pc_layer->encoder.i_idr_pic_id;
        pc_slice->p_header->xs_dec_ref_base_pic_marking.no_output_of_prior_pics_flag = 0;
        pc_slice->p_header->xs_dec_ref_base_pic_marking.long_term_reference_flag = 0;
        i_tmp = (p_self->encoder.rc.b_enabled ? p_self->encoder.rc.qp : p_self->encoder.i_qp) - 26 - pc_slice->p_header->pc_pps->pic_init_qp_minus26;
        b_slice_qp_delta_changed = (pc_slice->p_header->slice_qp_delta != i_tmp);
        pc_slice->p_header->slice_qp_delta = i_tmp;
        break;
    }
    }

    /* Current NAL (for SVC only) for the slices */
    if (p_self->encoder.b_svc_enabled) {
        p_self->nal_current.e_nal_type = (pc_layer->DQId == 0) ? HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT : HL_CODEC_264_NAL_TYPE_CODED_SLICE_EXTENSION;
        p_self->nal_current.ext.svc.idr_flag = (p_self->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_INTRA || p_self->encoder.encoding_curr == HL_VIDEO_ENCODING_TYPE_RAW);
        p_self->nal_current.ext.svc.priority_id = 0;
        p_self->nal_current.ext.svc.no_inter_layer_pred_flag = (pc_layer->DQId == 0) ? 1 : 0;
        p_self->nal_current.ext.svc.dependency_id = (pc_layer->DQId >> 4);
        p_self->nal_current.ext.svc.quality_id = pc_layer->DQId - (p_self->nal_current.ext.svc.dependency_id << 4); // TODO: Quality scalability not supported yet
        p_self->nal_current.ext.svc.temporal_id = 0; // TODO: Temporal scalability not supported yet
        p_self->nal_current.ext.svc.use_ref_base_pic_flag = 0;
        p_self->nal_current.ext.svc.discardable_flag = 0;
        p_self->nal_current.ext.svc.output_flag = 1;
        p_self->nal_current.ext.svc.reserved_three_2bits = 3;
        p_self->nal_current.ext.svc.DQId = (p_self->nal_current.ext.svc.dependency_id << 4) + p_self->nal_current.ext.svc.quality_id; // (G-61)

        if (!b_fake_slice) {
            // Write NALU header. For SLICE_Ext, will be done in encode function
            /*if (p_self->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT)*/ {
                // forbidden_zero_bit f(1)
                hl_codec_264_bits_write_f1(pc_esd->pobj_bits, 0);
                // ref_idc u(2)
                hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.i_nal_ref_idc, 2);
                // nal_unit_tye u(5)
                hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.e_nal_type, 5);
            }
            // nal_unit_header_svc_extension( ) /* specified in Annex G */
            // svc_extension_flag u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, 1);
            // idr_flag All u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_self->nal_current.ext.svc.idr_flag);
            // priority_id All u(6)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.ext.svc.priority_id, 6);
            // no_inter_layer_pred_flag All u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_self->nal_current.ext.svc.no_inter_layer_pred_flag);
            // dependency_id All u(3)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.ext.svc.dependency_id, 3);
            // quality_id All u(4)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.ext.svc.quality_id, 4);
            // temporal_id All u(3)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.ext.svc.temporal_id, 3);
            // use_ref_base_pic_flag All u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_self->nal_current.ext.svc.use_ref_base_pic_flag);
            // discardable_flag All u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_self->nal_current.ext.svc.discardable_flag);
            // output_flag All u(1)
            hl_codec_264_bits_write_u1(pc_esd->pobj_bits, p_self->nal_current.ext.svc.output_flag);
            // reserved_three_2bits All u(2)
            hl_codec_264_bits_write_u(pc_esd->pobj_bits, p_self->nal_current.ext.svc.reserved_three_2bits, 2);

            if (p_self->nal_current.e_nal_type == HL_CODEC_264_NAL_TYPE_PREFIX_NAL_UNIT) {
                // Add the SCP (Start Code Prefix) between the "PREFIX_NALU" and the "AVC_SLICE_HEADER".
                if ((pc_esd->pobj_bits->pc_end - pc_esd->pobj_bits->pc_current) <= 4) {
                    HL_DEBUG_ERROR("Too short");
                    return HL_ERROR_TOOSHORT;
                }

                // G.7.3.2.12.1 Prefix NAL unit SVC syntax
                // prefix_nal_unit_svc( )
                if (p_self->nal_current.i_nal_ref_idc) {
                    if ((p_self->nal_current.ext.svc.use_ref_base_pic_flag || p_self->nal_current.prefix.store_ref_base_pic_flag)
                            && p_self->nal_current.ext.svc.idr_flag) {
                        HL_DEBUG_ERROR("Not implemented yet");
                        return HL_ERROR_NOT_IMPLEMENTED;
                    }
                    else {
                        // store_ref_base_pic_flag = 0
                        // additional_prefix_nal_unit_extension_flag = 0
#if 0
                        hl_codec_264_bits_write_u(pc_esd->pobj_bits, 0, 2);
                        err = hl_codec_264_rbsp_avc_trailing_bits_write(pc_esd->pobj_bits);
                        if (err) {
                            return err;
                        }
#else
                        *(pc_esd->pobj_bits->pc_current++) = 32;
#endif
                    }
                }

                pc_esd->pobj_bits->pc_current[0] = HL_CODEC_264_START_CODE_PREFIX[0];
                pc_esd->pobj_bits->pc_current[1] = HL_CODEC_264_START_CODE_PREFIX[1];
                pc_esd->pobj_bits->pc_current[2] = HL_CODEC_264_START_CODE_PREFIX[2];
                pc_esd->pobj_bits->pc_current += 3;
                u_svc_nal_prefix_length = (pc_esd->pobj_bits->pc_current - pc_esd->pobj_bits->pc_start);
            }
        }
    }

    // Update current NAL with the default slice header
    p_self->nal_current.e_nal_type = HL_CODEC_264_NAL(pc_slice->p_header)->e_type;
    p_self->nal_current.i_nal_ref_idc = HL_CODEC_264_NAL(pc_slice->p_header)->u_ref_idc;
    p_self->nal_current.svc_extension_flag = (pc_layer->DQId > 0);

    // Encode slice header
    err = hl_codec_264_nal_slice_header_encode(p_self, u_idx, (b_encoding_changed || b_sps_changed || b_pps_changed || b_slice_qp_delta_changed), !b_fake_slice);
    if (err) {
        return err;
    }

    // Init current Layer representation now that the slice header is correctly encoded
    if (b_fake_slice || !b_multithreaded) {
        // Init current Layer representation now that the slice header is correctly encoded
        err = hl_codec_264_layer_init(pc_layer, pc_slice, p_self);
        if (err) {
            return err;
        }

        // Init variables depending on SPS
        if (b_sps_changed || b_pps_changed) {
            p_self->sps.pc_active = pc_slice->p_header->pc_pps->pc_sps;
            // Init DPB
            err = hl_codec_264_dpb_init(p_self->pc_dpb, p_self->sps.pc_active);
            if (err) {
                return err;
            }
            // Allocate Macroblocks
            if (pc_layer->u_list_macroblocks_count < p_self->sps.pc_active->uPicSizeInMapUnits) {
                HL_LIST_STATIC_SAFE_FREE_OBJECTS(pc_layer->pp_list_macroblocks, pc_layer->u_list_macroblocks_count); // Must not realloc as the list contains objects
                if (!(pc_layer->pp_list_macroblocks = hl_memory_calloc(p_self->sps.pc_active->uPicSizeInMapUnits, sizeof(hl_codec_264_mb_t*)))) {
                    pc_layer->u_list_macroblocks_count = 0;
                    return HL_ERROR_OUTOFMEMMORY;
                }
                pc_layer->u_list_macroblocks_count = p_self->sps.pc_active->uPicSizeInMapUnits;
            }
            // FIXME: is "MaxPicNum" needed?
            p_self->pc_poc->MaxFrameNum = p_self->pc_poc->MaxPicNum = p_self->sps.pc_active->uMaxFrameNum;
        }

        if (pc_layer->b_new_pict) {
            // map current FS memory to the right place in the DPB
            err = hl_codec_264_dpb_map_current(p_self, p_self->pc_dpb);
            if (err) {
                return err;
            }
        }

        // 8.2.1 Decoding process for picture order count
        // Must be called even if not new pict (e.g. for BOT, TOP)
        err = hl_codec_264_poc_decode(p_self->pc_poc, p_self);
        if (err) {
            return err;
        }

        // 8.2.2 Decoding process for macroblock to slice group map
        err = hl_codec_264_fmo_read_mb_to_slice_group_map(p_self);
        if (err) {
            return err;
        }

        // 8.2.4 Decoding process for reference picture lists construction
        err = hl_codec_264_reflist_init(p_self);
        if (err) {
            return err;
        }
    }

    // Encode slice data and escape
    if (!b_fake_slice) {
        err = hl_codec_264_nal_slice_data_encode(p_self, pc_esd);
        if (err) {
            return err;
        }
        pc_esd->u_rbsp_bytes_num = (pc_esd->pobj_bits->pc_current - pc_esd->pobj_bits->pc_start);
        err = hl_codec_264_rbsp_avc_escape(&pc_esd->p_rbsp_bytes[u_svc_nal_prefix_length], (pc_esd->u_rbsp_bytes_num - u_svc_nal_prefix_length), (pc_esd->u_rbsp_bytes_size - u_svc_nal_prefix_length));
    }

    return err;
}

HL_ERROR_T hl_codec_264_encode_frame(
    hl_codec_264_t* p_self,
    const hl_frame_video_t* pc_frame,
    hl_bool_t b_encoding_changed, hl_bool_t b_sps_changed, hl_bool_t b_pps_changed)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t* pc_layer;
    hl_codec_264_encode_slice_data_t *pc_esd_fake;
    hl_bool_t b_multi_threaded;

    pc_layer = p_self->layers.pc_active;

    b_multi_threaded = (p_self->threads.u_list_tasks_count >= 2);

    pc_layer->i_mb_encode_count = 0;

    // encode fake slice
    // always called to initialize FMO, POC, REFS....
    err = _hl_codec_264_encode_slice(
              p_self,
              __u_idx_fake,
              b_multi_threaded ? __i_mb_start_fake : 0,
              b_multi_threaded ? __i_mb_end_fake : (int32_t)p_self->sps.pc_active->uPicSizeInMapUnits,
              b_encoding_changed, b_sps_changed, b_pps_changed, __i_coreid_invalid);
    if (err) {
        return err;
    }
    pc_esd_fake = pc_layer->encoder.p_list_esd[__u_idx_fake];

    if (b_multi_threaded) {
        int32_t i_core_id, i_i, i_min_mb_per_slice, i_mb_start, i_mb_end, i_cores_count;
        hl_bool_t b_last;
        hl_asynctoken_id_t i_token_id;
        static const uint32_t __u_slice_idx[HL_CODEC_264_SLICES_MAX_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

        i_cores_count = hl_cpu_get_cores_count();
        i_i = 0;
        i_core_id = 0; // FIXME
        i_token_id = 0;
        i_mb_start = i_mb_end = 0;
        i_min_mb_per_slice = ((int32_t)pc_layer->u_list_macroblocks_count / pc_layer->encoder.i_slice_count);
        do {
            b_last = ((i_i + 1) == pc_layer->encoder.i_slice_count);
            i_mb_end = (b_last ? (int32_t)pc_layer->u_list_macroblocks_count : (i_mb_start + i_min_mb_per_slice));
            pc_layer->encoder.async_mb_start[i_i] = i_mb_start;
            pc_layer->encoder.async_mb_end[i_i] = i_mb_end;
            err = hl_asynctask_execute(p_self->threads.pp_list_tasks[i_core_id], i_token_id, _hl_codec_264_encode_slice_async,
                                       HL_ASYNCTASK_SET_PARAM_VAL(p_self),
                                       HL_ASYNCTASK_SET_PARAM_VAL(__u_slice_idx[i_i]),
                                       HL_ASYNCTASK_SET_PARAM_VAL(pc_layer->encoder.async_mb_start[i_i]),
                                       HL_ASYNCTASK_SET_PARAM_VAL(pc_layer->encoder.async_mb_end[i_i]),
                                       HL_ASYNCTASK_SET_PARAM_VAL(b_encoding_changed),
                                       HL_ASYNCTASK_SET_PARAM_VAL(b_sps_changed),
                                       HL_ASYNCTASK_SET_PARAM_VAL(b_pps_changed),
                                       HL_ASYNCTASK_SET_PARAM_NULL());
#if 0 // FIXME
            hl_asynctask_wait_2(p_self->threads.pp_list_tasks[i_core_id], i_token_id);
#endif
            if (err) {
                return err;
            }

            ++i_i;
            ++i_token_id;
            i_core_id = (i_core_id + 1) % i_cores_count; // FIXME
            i_mb_start += (i_mb_end - i_mb_start);
        }
        while (!b_last);

        // Wait for all tasks
        for (i_core_id = 0; i_core_id < i_cores_count; ++i_core_id) {
            for (i_i = 0; i_i < i_token_id; ++i_i) {
                hl_asynctask_wait_2(p_self->threads.pp_list_tasks[i_core_id], i_i);
            }
        }
    }

    ++pc_layer->i_pict_encode_count;
    if (IdrPicFlag(HL_CODEC_264_NAL(pc_esd_fake->pc_slice->p_header)->e_type)) {
        ++pc_layer->encoder.i_idr_pic_id;
    }

    // FIXME:
    if(0) {
        char name[1024] = {0};
        FILE* p_file;
        static FILE* p_file_rec = HL_NULL;
        static int const n_file_max_frame = 100;

        hl_md5string_t md5_result;
        hl_md5context_t md5_ctx;
        hl_md5digest_t md5_digest;

        if (!p_file_rec && pc_layer->i_pict_decode_count < n_file_max_frame) {
            p_file_rec = fopen("./reconstructed.yuv", "wb+");
        }

        sprintf(name, "./svc_DQId[%d]_Frame[%d].yuv", pc_layer->DQId, pc_layer->i_pict_encode_count);
        p_file = fopen(name, "wb+");

        hl_md5compute(pc_layer->pc_fs_curr->p_pict->pc_data_y, /*((0 * 16) + (1 * 16 * 352))*/pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y, &md5_result);
        HL_DEBUG_INFO("Y MD5=%s", md5_result);
        hl_md5compute(pc_layer->pc_fs_curr->p_pict->pc_data_u, /*((0 * 8) + (7 * 8 * 176))*/pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, &md5_result);
        HL_DEBUG_INFO("U MD5=%s", md5_result);
        hl_md5compute(pc_layer->pc_fs_curr->p_pict->pc_data_v, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, &md5_result);
        HL_DEBUG_INFO("V MD5=%s", md5_result);
        hl_md5init(&md5_ctx);
        hl_md5update(&md5_ctx, pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y);
        hl_md5update(&md5_ctx, pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c);
        hl_md5update(&md5_ctx, pc_layer->pc_fs_curr->p_pict->pc_data_v, pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c);
        hl_md5final(md5_digest, &md5_ctx);
        hl_str_from_hex(md5_digest, HL_MD5_DIGEST_SIZE, md5_result);
        HL_DEBUG_INFO("YUV MD5=%s", md5_result);

        fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_y, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y, p_file);
        fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_u, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, p_file);
        fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_v, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, p_file);
        fclose(p_file);

        fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_y, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_y, p_file_rec);
        fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_u, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, p_file_rec);
        fwrite(pc_layer->pc_fs_curr->p_pict->pc_data_v, sizeof(hl_pixel_t), pc_layer->pc_fs_curr->pc_dpb->u_buff_size_fs_c, p_file_rec);
        if (pc_layer->i_pict_decode_count == n_file_max_frame) {
            fclose(p_file_rec);
            p_file_rec = HL_NULL;
        }
    }

    return err;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_encode_slice_data_t" ***/
static hl_object_t* hl_codec_264_encode_slice_data_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_encode_slice_data_t *p_esd = (hl_codec_264_encode_slice_data_t*)self;
    if (p_esd) {

    }
    return self;
}
static hl_object_t* hl_codec_264_encode_slice_data_dtor(hl_object_t * self)
{
    hl_codec_264_encode_slice_data_t *p_esd = (hl_codec_264_encode_slice_data_t*)self;
    if (p_esd) {
        // rdo
        HL_OBJECT_SAFE_FREE(p_esd->rdo.pobj_bits);

        // common
        HL_OBJECT_SAFE_FREE(p_esd->pobj_bits);
        HL_SAFE_FREE(p_esd->p_rbsp_bytes);
    }
    return self;
}
static int hl_codec_264_encode_slice_data_cmp(const hl_object_t *_esd1, const hl_object_t *_esd2)
{
    return (int)((int*)_esd1 - (int*)_esd2);
}
static const hl_object_def_t hl_codec_264_encode_slice_data_def_s = {
    sizeof(hl_codec_264_encode_slice_data_t),
    hl_codec_264_encode_slice_data_ctor,
    hl_codec_264_encode_slice_data_dtor,
    hl_codec_264_encode_slice_data_cmp,
};
const hl_object_def_t *hl_codec_264_encode_slice_data_def_t = &hl_codec_264_encode_slice_data_def_s;
