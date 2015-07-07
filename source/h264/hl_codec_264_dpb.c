#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_interpol.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

#include <limits.h> /* INT_MAX */

extern const hl_object_def_t *hl_codec_264_dpb_def_t;
extern const hl_object_def_t *hl_codec_264_dpb_fs_def_t;

static hl_bool_t _hl_codec_264_dpb_sps_changed(const hl_codec_264_nal_sps_t* pc_sps1, const hl_codec_264_nal_sps_t* pc_sps2);


HL_ERROR_T hl_codec_264_dpb_create(hl_codec_264_dpb_t** pp_dpb)
{
    if (!pp_dpb) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_dpb = hl_object_create(hl_codec_264_dpb_def_t);
    if (!*pp_dpb) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _hl_codec_264_dpb_fs_create(const hl_codec_264_t* pc_codec, hl_codec_264_dpb_fs_t** pp_fs)
{
    if (!pp_fs) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_fs = hl_object_create(hl_codec_264_dpb_fs_def_t);
    if (!*pp_fs) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_fs)->pc_dpb = pc_codec->pc_dpb;
    return HL_ERROR_SUCCESS;
}

// Allocate memory (must only be called when SPS change)
HL_ERROR_T hl_codec_264_dpb_init(hl_codec_264_dpb_t* p_dpb, const hl_codec_264_nal_sps_t* pc_sps)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_interpol_indices_t* p_interpol_indices;
    if (!p_dpb || !pc_sps) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    // A.3.1 Level limits common to the Baseline, Constrained Baseline, Main, and Extended profiles
    //e) PicWidthInMbs * FrameHeightInMbs <= MaxFS, where MaxFS is specified in Table A-1
    //f) PicWidthInMbs <= Sqrt( MaxFS * 8 )
    //g) FrameHeightInMbs <= Sqrt( MaxFS * 8 )
    //h) max_dec_frame_buffering <= MaxDpbFrames, where MaxDpbFrames is equal to Min( MaxDpbMbs / ( PicWidthInMbs * FrameHeightInMbs ), 16 ) and MaxDpbMbs is given in Table A-1.
    if (_hl_codec_264_dpb_sps_changed(p_dpb->p_sps, pc_sps)) {
        hl_size_t u_fs_memory_size_y, u_fs_memory_size_c, u_fs_memory_size, u_overall_memory_size, i;
        // reset fs
        for (i = 0; i < p_dpb->u_list_fs_count; ++i) {
            if (p_dpb->p_list_fs[i]) {
                p_dpb->p_list_fs[i]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                if (p_dpb->p_list_fs[i]->p_pict) {
                    p_dpb->p_list_fs[i]->p_pict->pc_data_y = HL_NULL;
                    p_dpb->p_list_fs[i]->p_pict->pc_data_u = HL_NULL;
                    p_dpb->p_list_fs[i]->p_pict->pc_data_v = HL_NULL;
                    p_dpb->p_list_fs[i]->p_pict->uWidthL = 0;
                    p_dpb->p_list_fs[i]->p_pict->uWidthC = 0;
                    p_dpb->p_list_fs[i]->p_pict->uHeightL = 0;
                    p_dpb->p_list_fs[i]->p_pict->uHeightC = 0;
                }
            }
        }

        p_dpb->u_buff_size_inuse = 0;
        p_dpb->u_buff_size_overall = 0;
        p_dpb->u_list_fs_count_max = MaxDpbMbs[HL_CODEC_264_LEVEL_TO_ZERO_BASED_INDEX[pc_sps->level_idc]] / (pc_sps->uPicWidthInMbs * pc_sps->uFrameHeightInMbs);
        if (p_dpb->u_list_fs_count_max > 16) {
            p_dpb->u_list_fs_count_max = 16;
        }
        p_dpb->u_list_fs_count_max += 1; // current picture being decoded <> reference frames
        u_fs_memory_size_y = (pc_sps->uPicSizeInMapUnits << 8); // size in pixel
        u_fs_memory_size_c = (pc_sps->uPicSizeInMapUnits * pc_sps->MbWidthC * pc_sps->MbHeightC); // size in pixel
        u_fs_memory_size = (u_fs_memory_size_y/* Y */ + (u_fs_memory_size_c << 1) /* U,V */); // size in pixel
        u_overall_memory_size = (p_dpb->u_list_fs_count_max * u_fs_memory_size);
        // alloc memory
        p_dpb->p_buff = (hl_pixel_t*)hl_memory_realloc(p_dpb->p_buff, (u_overall_memory_size * sizeof(hl_pixel_t)));
        if (!p_dpb->p_buff) {
            err = HL_ERROR_OUTOFMEMMORY;
            goto bail;
        }
        p_dpb->u_buff_size_overall = u_overall_memory_size;
        p_dpb->u_buff_size_fs_y = u_fs_memory_size_y;
        p_dpb->u_buff_size_fs_c = u_fs_memory_size_c;
        p_dpb->u_buff_size_fs_ycc = u_fs_memory_size;

        // Interpolation
        {
            // Both MBAFF (0,1) values are needed as it could be changed at macroblock level (SLICE->MbaffFrameFlag or MB->mb_field_decoding_flag) unless baseline.
            int32_t i = 0;
            HL_LIST_STATIC_CLEAR_OBJECTS(p_dpb->p_list_interpol_indices, p_dpb->u_list_list_interpol_indices_count);
            for (i = 0; i < 2; ++i) {
                err = hl_codec_264_interpol_indices_create(&p_interpol_indices, (pc_sps->uPicWidthInMbs << 4), (pc_sps->uPicHeightInMapUnit << 4), i ? HL_TRUE : HL_FALSE/*MBAFF*/);
                if (err) {
                    goto bail;
                }
                HL_LIST_STATIC_ADD_OBJECT(
                    p_dpb->p_list_interpol_indices,
                    p_dpb->u_list_list_interpol_indices_count,
                    2,
                    p_interpol_indices,
                    err);
                if (err) {
                    goto bail;
                }
                if (hl_codec_264_is_baseline(pc_sps)) {
                    break;
                }
            }
        }

        // Update SPS now that all initializations succeeded
        hl_object_update((hl_object_t**)&p_dpb->p_sps, HL_OBJECT(pc_sps));
    }

bail:
    return err;
}

// Map memory to the current picture being encoded/decode (must only be called for new pictures).
// This function sets the current FS.
HL_ERROR_T hl_codec_264_dpb_map_current(hl_codec_264_t* p_codec, hl_codec_264_dpb_t* p_dpb)
{
    hl_size_t i;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_layer_t *pc_layer = pc_layer = p_codec->layers.pc_active;

    for (i = 0; i < p_dpb->u_list_fs_count_max; ++i) {
        if (!p_dpb->p_list_fs[i] || HL_CODEC_264_REF_TYPE_IS_UNUSED(p_dpb->p_list_fs[i]->RefType)) {
            if (!p_dpb->p_list_fs[i]) {
                hl_codec_264_dpb_fs_t* p_fs;
                err = _hl_codec_264_dpb_fs_create(p_codec, &p_fs);
                if (err) {
                    return err;
                }
                HL_LIST_STATIC_ADD_OBJECT_AT_IDX(p_dpb->p_list_fs, p_dpb->u_list_fs_count, p_dpb->u_list_fs_count_max, p_fs, i, err);
                if (err) {
                    return err;
                }
            }
            pc_layer->pc_fs_curr = p_dpb->p_list_fs[i];
            if (!pc_layer->pc_fs_curr->p_pict) {
                err = hl_codec_264_pict_create(&pc_layer->pc_fs_curr->p_pict);
                if (err) {
                    return err;
                }
            }
            if(!pc_layer->pc_fs_curr->p_pict->pc_data_y) {
                pc_layer->pc_fs_curr->p_pict->pc_data_y = (p_dpb->p_buff + (i * p_dpb->u_buff_size_fs_ycc));
                pc_layer->pc_fs_curr->p_pict->pc_data_u = (pc_layer->pc_fs_curr->p_pict->pc_data_y + p_dpb->u_buff_size_fs_y);
                pc_layer->pc_fs_curr->p_pict->pc_data_v = (pc_layer->pc_fs_curr->p_pict->pc_data_u + p_dpb->u_buff_size_fs_c);
            }
            if (!pc_layer->pc_fs_curr->p_pict->uWidthL) {
                pc_layer->pc_fs_curr->p_pict->uWidthL = pc_layer->pc_slice_hdr->PicWidthInSamplesL;
                pc_layer->pc_fs_curr->p_pict->uHeightL = pc_layer->pc_slice_hdr->PicHeightInSamplesL;
                pc_layer->pc_fs_curr->p_pict->uWidthC = pc_layer->pc_slice_hdr->PicWidthInSamplesC;
                pc_layer->pc_fs_curr->p_pict->uHeightC = pc_layer->pc_slice_hdr->PicHeightInSamplesC;
            }
            pc_layer->pc_fs_curr->FrameNum = pc_layer->pc_slice_hdr->frame_num;
            break;
        }
    }
    if (i == p_dpb->u_list_fs_count_max) {
        HL_DEBUG_ERROR("Failed to find free memory where to output decoded picture");
        return HL_ERROR_NOT_FOUND;
    }
    return err;
}

// 8.2.5 Decoded reference picture marking process
// This process is invoked for decoded pictures when nal_ref_idc is not equal to 0.
HL_ERROR_T hl_codec_264_dpb_add_decoded(hl_codec_264_t* p_codec)
{
    int32_t i;
    enum HL_CODEC_264_REF_TYPE_E svcRefBaseType = HL_CODEC_264_REF_TYPE_UNUSED;
    hl_codec_264_layer_t *pc_layer = p_codec->layers.pc_active;

    if (p_codec->nal_current.svc_extension_flag) {
        // When store_ref_base_pic_flag is equal to 1, a second representation of the
        // decoded picture also referred to as reference base picture is marked as "used for short-term reference" or "used for
        // long-term reference" and additionally marked as "reference base picture".
        if (p_codec->nal_current.prefix.store_ref_base_pic_flag) {
            svcRefBaseType = HL_CODEC_264_REF_TYPE_USED_FOR_SVC_BASE_REF;
        }
    }

    // 8.2.5.1 Sequence of operations for decoded reference picture marking process
    if (IdrPicFlag(p_codec->nal_current.e_nal_type)) {
        // reset all FSs
        for (i = 0; i < (int32_t)p_codec->pc_dpb->u_list_fs_count; ++i) {
            if (p_codec->pc_dpb->p_list_fs[i] && p_codec->pc_dpb->p_list_fs[i] != pc_layer->pc_fs_curr) {
                p_codec->pc_dpb->p_list_fs[i]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                if (p_codec->pc_dpb->p_list_fs[i]->p_pict) {
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->pc_data_y = HL_NULL;
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->pc_data_u = HL_NULL;
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->pc_data_v = HL_NULL;
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->uWidthL = 0;
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->uWidthC = 0;
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->uHeightL = 0;
                    p_codec->pc_dpb->p_list_fs[i]->p_pict->uHeightC = 0;
                }
            }
        }

        if (p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.long_term_reference_flag == 0) {
            pc_layer->pc_fs_curr->RefType = (HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM | svcRefBaseType);
            pc_layer->pc_fs_curr->LongTermFrameIdx = -1;
        }
        else {
            pc_layer->pc_fs_curr->RefType = (HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM | svcRefBaseType);
            pc_layer->pc_fs_curr->LongTermFrameIdx = 0;
        }
    }
    else {
        if (p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.adaptive_ref_pic_marking_mode_flag == 0) {
            // 8.2.5.3 Sliding window decoded reference picture marking process
            // FIXME: if(p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag)....
            // else
            while ((p_codec->pc_dpb->numShortTerm + p_codec->pc_dpb->numLongTerm) >= HL_MATH_MAX(p_codec->sps.pc_active->max_num_ref_frames, 1)) {
                int32_t idx = -1;
                int32_t minFrameNumWrap = INT_MAX;
                for (i=0; i<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++i) {
                    if ((HL_CODEC_264_REF_TYPE_IS_USED(p_codec->pc_dpb->p_list_fs[i]->RefType) && !HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[i]->RefType)) && p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap < minFrameNumWrap) {
                        minFrameNumWrap = p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap;
                        idx = i;
                    }
                }

                if (HL_MATH_IS_NEGATIVE_INT32(idx)) {
                    HL_DEBUG_ERROR("idx<0");
                    break;
                }

                if(HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(p_codec->pc_dpb->p_list_fs[idx]->RefType)) {
                    --p_codec->pc_dpb->numShortTerm;
                }
                else if(HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[idx]->RefType)) {
                    --p_codec->pc_dpb->numLongTerm;
                }

                p_codec->pc_dpb->p_list_fs[idx]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
            }

            // Add the picture (as used as short term)
            pc_layer->pc_fs_curr->RefType = (HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM | svcRefBaseType);
            pc_layer->pc_fs_curr->LongTermFrameIdx = -1;
        }
        else {
            uint32_t *long_term_pic_num, *long_term_frame_idx;
            //– If field_pic_flag is equal to 0, memory_management_control_operation commands are applied to the frames or
            //	complementary reference field pairs specified.
            // – Otherwise (field_pic_flag is equal to 1), memory_management_control_operation commands are applied to the
            //	individual reference fields specified.
            if (p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag) {
                // The below code is only for frames
                HL_DEBUG_ERROR("Not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
            }

            // Add the picture (as used as short term)
            pc_layer->pc_fs_curr->RefType = (HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM | svcRefBaseType);
            pc_layer->pc_fs_curr->LongTermFrameIdx = -1;

            long_term_pic_num = p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.long_term_pic_num;
            long_term_frame_idx = p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.long_term_frame_idx;

            for (i=0; i < HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT && p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.memory_management_control_operation[i] != 0; ++i) {
                switch (p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.memory_management_control_operation[i]) {
                    // 8.2.5.4.1 Marking process of a short-term reference picture as "unused for reference"
                case 1: {
                    int32_t picNumX, ii;
                    picNumX = p_codec->pc_poc->CurrPicNum - (p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.difference_of_pic_nums_minus1[i] + 1);//(8-39)
                    for (ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(p_codec->pc_dpb->p_list_fs[ii]->RefType) && p_codec->pc_dpb->p_list_fs[ii]->PicNum == picNumX) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                            break;
                        }
                    }
                    break;
                }

                // 8.2.5.4.2 Marking process of a long-term reference picture as "unused for reference"
                case 2: {
                    int32_t ii;
                    for (ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[ii]->RefType) && p_codec->pc_dpb->p_list_fs[ii]->LongTermPicNum == long_term_pic_num[i]) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                            break;
                        }
                    }
                    break;
                }

                // 8.2.5.4.3 Assignment process of a LongTermFrameIdx to a short-term reference picture
                case 3: {
                    int32_t picNumX, ii;
                    picNumX = p_codec->pc_poc->CurrPicNum - (p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.difference_of_pic_nums_minus1[i] + 1);//(8-39)

                    for(ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if(HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[ii]->RefType) && p_codec->pc_dpb->p_list_fs[ii]->LongTermFrameIdx == long_term_frame_idx[i]) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                            break;
                        }
                    }
                    for(ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if(HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(p_codec->pc_dpb->p_list_fs[ii]->RefType) && p_codec->pc_dpb->p_list_fs[ii]->PicNum == picNumX) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = (HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM | svcRefBaseType);
                            p_codec->pc_dpb->p_list_fs[ii]->LongTermFrameIdx = long_term_frame_idx[i];
                            break;
                        }
                    }
                    break;
                }

                //8.2.5.4.4 Decoding process for MaxLongTermFrameIdx
                case 4: {
                    int32_t ii;
                    for (ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if(HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[ii]->RefType) && p_codec->pc_dpb->p_list_fs[ii]->LongTermFrameIdx > (int32_t)p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.max_long_term_frame_idx_plus1[i]-1) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                            ii=-1;
                        }
                    }
                    if (p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.max_long_term_frame_idx_plus1[i]==0) {
                        p_codec->pc_poc->MaxLongTermFrameIdx=-1;
                    }
                    else {
                        p_codec->pc_poc->MaxLongTermFrameIdx=p_codec->layers.pc_active->pc_slice_hdr->xs_dec_ref_base_pic_marking.max_long_term_frame_idx_plus1[i]-1;
                    }

                    break;
                }

                // 8.2.5.4.5 Marking process of all reference pictures as "unused for reference" and setting MaxLongTermFrameIdx to "no long-term frame indices"
                case 5: {
                    int32_t ii;
                    HL_DEBUG_ERROR("prevMemMgrCtrlOp probably contains invalid value"); // FIXME: because 'prevMemMgrCtrlOp' never affected
                    p_codec->pc_poc->MaxLongTermFrameIdx = -1;
                    for (ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if (p_codec->pc_dpb->p_list_fs[ii] != pc_layer->pc_fs_curr) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                            ii=-1;
                        }
                    }
                    break;
                }

                // 8.2.5.4.6 Process for assigning a long-term frame index to the current picture
                case 6: {
                    int32_t ii;

                    for(ii=0; ii<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++ii) {
                        if(p_codec->pc_dpb->p_list_fs[ii]->LongTermFrameIdx == long_term_frame_idx[i]) {
                            p_codec->pc_dpb->p_list_fs[ii]->RefType = HL_CODEC_264_REF_TYPE_UNUSED;
                            break;
                        }
                    }
                    pc_layer->pc_fs_curr->RefType = (HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM | svcRefBaseType);
                    pc_layer->pc_fs_curr->LongTermFrameIdx = long_term_frame_idx[i];
                    break;
                }
                }//swicth
            }//for
        }
    }

    //compute 'numShortTerm' and 'numLongTerm'
    p_codec->pc_dpb->numShortTerm = p_codec->pc_dpb->numLongTerm = 0;
    for (i=0; i<(int32_t)p_codec->pc_dpb->u_list_fs_count; ++i) {
        if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
            p_codec->pc_dpb->numShortTerm++;
        }
        else if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
            p_codec->pc_dpb->numLongTerm++;
        }
    }

    // Compute PicOrderCnt
    PicOrderCnt(pc_layer->pc_fs_curr, p_codec->pc_poc, &pc_layer->pc_fs_curr->PicOrderCnt);

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_dpb_fs_create(hl_codec_264_dpb_fs_t** pp_dpb_fs)
{
    if (!pp_dpb_fs) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_dpb_fs = hl_object_create(hl_codec_264_dpb_fs_def_t);
    if (!*pp_dpb_fs) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    return HL_ERROR_SUCCESS;
}

static hl_bool_t _hl_codec_264_dpb_sps_changed(const hl_codec_264_nal_sps_t* pc_sps1, const hl_codec_264_nal_sps_t* pc_sps2)
{
    if (!pc_sps1 || !pc_sps2) {
        return HL_TRUE;
    }
    if (pc_sps1 == pc_sps2) {
        return HL_FALSE;
    }

    return
        pc_sps1->seq_parameter_set_id != pc_sps2->seq_parameter_set_id
        || pc_sps1->level_idc != pc_sps2->level_idc
        || pc_sps1->uPicWidthInMbs != pc_sps2->uPicWidthInMbs
        || pc_sps1->uFrameHeightInMbs != pc_sps2->uFrameHeightInMbs
        || pc_sps1->uPicSizeInMapUnits != pc_sps2->uPicSizeInMapUnits // MBAFF also depends on it
        ;
}



/*** OBJECT DEFINITION FOR "hl_codec_264_dpb_t" ***/
static hl_object_t* hl_codec_264_dpb_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_dpb_t *p_dpb = (hl_codec_264_dpb_t*)self;
    if (p_dpb) {

    }
    return self;
}
static hl_object_t* hl_codec_264_dpb_dtor(hl_object_t * self)
{
    hl_codec_264_dpb_t *p_dpb = (hl_codec_264_dpb_t*)self;
    if (p_dpb) {
        HL_LIST_STATIC_CLEAR_OBJECTS(p_dpb->p_list_fs, p_dpb->u_list_fs_count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_dpb->p_list_interpol_indices, p_dpb->u_list_list_interpol_indices_count);
        HL_OBJECT_SAFE_FREE(p_dpb->p_buff);
        HL_OBJECT_SAFE_FREE(p_dpb->p_sps);
    }
    return self;
}
static int hl_codec_264_dpb_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_dpb_def_s = {
    sizeof(hl_codec_264_dpb_t),
    hl_codec_264_dpb_ctor,
    hl_codec_264_dpb_dtor,
    hl_codec_264_dpb_cmp,
};
const hl_object_def_t *hl_codec_264_dpb_def_t = &hl_codec_264_dpb_def_s;


/*** OBJECT DEFINITION FOR "hl_codec_264_dpb_fs_t" ***/
static hl_object_t* hl_codec_264_dpb_fs_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_dpb_fs_t *p_dpb_fs = (hl_codec_264_dpb_fs_t*)self;
    if (p_dpb_fs) {

    }
    return self;
}
static hl_object_t* hl_codec_264_dpb_fs_dtor(hl_object_t * self)
{
    hl_codec_264_dpb_fs_t *p_dpb_fs = (hl_codec_264_dpb_fs_t*)self;
    if (p_dpb_fs) {
        HL_OBJECT_SAFE_FREE(p_dpb_fs->p_pict);
    }
    return self;
}
static int hl_codec_264_dpb_fs_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_dpb_fs_def_s = {
    sizeof(hl_codec_264_dpb_fs_t),
    hl_codec_264_dpb_fs_ctor,
    hl_codec_264_dpb_fs_dtor,
    hl_codec_264_dpb_fs_cmp,
};
const hl_object_def_t *hl_codec_264_dpb_fs_def_t = &hl_codec_264_dpb_fs_def_s;
