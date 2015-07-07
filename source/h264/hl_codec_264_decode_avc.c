#include "hartallo/h264/hl_codec_264_decode_avc.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_pred_intra.h"
#include "hartallo/h264/hl_codec_264_pred_inter.h"
#include "hartallo/h264/hl_codec_264_deblock.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/hl_asynctask.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

HL_ASYNC_CALL_INDIRECT
static HL_ERROR_T _hl_codec_264_pred_inter_decode_async(
    const hl_asynctoken_param_xt* pc_params)
{
    int32_t i_core_id, CurrMbAddr;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_mb_t* pc_mb;
    hl_codec_264_layer_t* pc_layer;

    hl_codec_264_t* p_codec = HL_ASYNCTASK_GET_PARAM(pc_params[0].pc_param_ptr, hl_codec_264_t*);
    int32_t i_mb_start = HL_ASYNCTASK_GET_PARAM(pc_params[1].pc_param_ptr, int32_t);
    int32_t i_mb_end = HL_ASYNCTASK_GET_PARAM(pc_params[2].pc_param_ptr, int32_t);

    i_core_id = hl_thread_get_core_id();
    pc_layer = p_codec->layers.pc_active;

    // HL_DEBUG_INFO("_hl_codec_264_pred_inter_decode_async(core_id=%d, i_mb_start=%d, i_mb_end=%d)", i_core_id, i_mb_start, i_mb_end);

    for (CurrMbAddr = i_mb_start; CurrMbAddr < i_mb_end; ++CurrMbAddr) {
        if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
            HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
            continue;
        }
        if (!HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mb)) {
            // AVC-Inter decoding without computing MVs and Refs because already done by the caller
            err = hl_codec_264_pred_inter_decode_without_mvs_and_refs_computation(p_codec, pc_mb);
            if (err) {
                return err;
            }
        }
    }
    return err;
}

HL_ERROR_T hl_codec_264_decode_avc(hl_codec_264_t* p_codec)
{
    hl_codec_264_mb_t* pc_mb;
    hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    int32_t CurrMbAddr, WorthCheckingLoopFilterFlag = 0;
    hl_size_t u;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    // Count number of slices requiring deblocking
    for (u = 0; u < pc_layer->u_list_slices_count; ++u) {
        if (WorthCheckingLoopFilterFlag = (pc_layer->p_list_slices[u]->p_header->disable_deblocking_filter_idc != 1)) {
            break;
        }
    }

    if (p_codec->threads.u_list_tasks_count > 1) {
        int32_t i_mb_intra_to_decode_count, i_mb_inter_to_decode_count;

        // Count Intra MBs
        i_mb_intra_to_decode_count = 0;
        for (CurrMbAddr = 0; CurrMbAddr < pc_layer->i_mb_read_count; ++CurrMbAddr) {
            if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
                HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
                continue;
            }
            if (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mb)) {
                ++i_mb_intra_to_decode_count;
            }
        }

        i_mb_inter_to_decode_count = (pc_layer->i_mb_read_count - i_mb_intra_to_decode_count);

        // Decode Inter (Multi-threaded?)
        if (i_mb_inter_to_decode_count > 0) {
            int32_t i_min_mb_per_thread;

            i_min_mb_per_thread = i_mb_inter_to_decode_count / (int32_t)p_codec->threads.u_list_tasks_count; // FIXME: clip using user-defined config
            if (i_mb_inter_to_decode_count <= i_min_mb_per_thread) {
                // do it on a single (current) thread - because doesn't worth using more than one thread
                for (CurrMbAddr = 0; CurrMbAddr < pc_layer->i_mb_read_count; ++CurrMbAddr) {
                    if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
                        HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
                        continue;
                    }
                    if (!HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mb)) {
                        // 8.4 Inter prediction process
                        err = hl_codec_264_pred_inter_decode_with_mvs_and_refs_computation(p_codec, pc_mb);
                        if (err) {
                            goto bail;
                        }
                        ++pc_layer->i_mb_decode_count;
                    }
                }
            }
            else {
                // do it on more than #1 thread
                hl_bool_t b_last;
                int32_t i_mb_start, i_mb_end, i_i, i_core_id, i_cores_count, i_mb_block_per_thread;
                hl_asynctoken_id_t i_token_id;
                int32_t mbPartsCount, mbPartIdx, subMbPartIdx, subPartsCount;

                // FIXME: move these variables to the layer context
                int32_t async_mb_start[HL_CODEC_264_SLICES_MAX_COUNT];
                int32_t async_mb_end[HL_CODEC_264_SLICES_MAX_COUNT];

                //!\ Computing motion vector components and reference indices cannot be do asynchronously
                for (CurrMbAddr = 0; CurrMbAddr < pc_layer->i_mb_read_count; ++CurrMbAddr) {
                    if ((pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr]) && !HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mb)) {
                        mbPartsCount = (pc_mb->e_type == HL_CODEC_264_MB_TYPE_B_SKIP || pc_mb->e_type == HL_CODEC_264_MB_TYPE_B_DIRECT_16X16) ? 4 : pc_mb->NumMbPart;
                        for (mbPartIdx = 0; mbPartIdx<mbPartsCount; ++mbPartIdx) {
                            subPartsCount = pc_mb->NumSubMbPart[mbPartIdx];//FIXME: is it right?
                            subMbPartIdx = 0;
                            do {
                                // 8.4.1 Derivation process for motion vector components and reference indices
                                err = hl_codec_264_utils_derivation_process_for_movect_comps_and_ref_indices(p_codec, pc_mb, mbPartIdx, subMbPartIdx,
                                        &pc_mb->mvL0[mbPartIdx][subMbPartIdx], &pc_mb->mvL1[mbPartIdx][subMbPartIdx], &pc_mb->mvCL0[mbPartIdx][subMbPartIdx], &pc_mb->mvCL1[mbPartIdx][subMbPartIdx],
                                        &pc_mb->refIdxL0[mbPartIdx], &pc_mb->refIdxL1[mbPartIdx],
                                        &pc_mb->predFlagL0[mbPartIdx], &pc_mb->predFlagL1[mbPartIdx],
                                        &pc_mb->subMvCnt);

                                pc_mb->MvL0[mbPartIdx][subMbPartIdx] = pc_mb->mvL0[mbPartIdx][subMbPartIdx]; //(8-162)
                                pc_mb->MvL1[mbPartIdx][subMbPartIdx] = pc_mb->mvL1[mbPartIdx][subMbPartIdx]; //(8-163)
                                pc_mb->MvCL0[mbPartIdx][subMbPartIdx] = pc_mb->mvCL0[mbPartIdx][subMbPartIdx];
                                pc_mb->MvCL1[mbPartIdx][subMbPartIdx] = pc_mb->mvCL1[mbPartIdx][subMbPartIdx];
                                pc_mb->RefIdxL0[mbPartIdx] = pc_mb->refIdxL0[mbPartIdx]; //(8-164)
                                pc_mb->RefIdxL1[mbPartIdx] = pc_mb->refIdxL1[mbPartIdx]; //(8-165)
                                pc_mb->PredFlagL0[mbPartIdx] = pc_mb->predFlagL0[mbPartIdx]; //(8-166)
                                pc_mb->PredFlagL1[mbPartIdx] = pc_mb->predFlagL1[mbPartIdx]; //(8-167)
                            }
                            while (++subMbPartIdx < subPartsCount);
                        }
                    }
                }

                // Run asynchronous tasks
                i_mb_block_per_thread = pc_layer->i_mb_read_count / (int32_t)p_codec->threads.u_list_tasks_count;
                i_cores_count = hl_cpu_get_cores_count();
                i_core_id = (hl_thread_get_core_id() + 1) % p_codec->threads.u_list_tasks_count;
                i_token_id = 0;
                i_mb_start = 0;
                i_i = 0;
                do {
                    b_last = ((i_i + 1) == p_codec->threads.u_list_tasks_count);
                    i_mb_end = (b_last ? pc_layer->i_mb_read_count : (i_mb_start + i_mb_block_per_thread));
                    async_mb_start[i_i] = i_mb_start;
                    async_mb_end[i_i] = i_mb_end;

                    err = hl_asynctask_execute(p_codec->threads.pp_list_tasks[i_core_id], i_token_id, _hl_codec_264_pred_inter_decode_async,
                                               HL_ASYNCTASK_SET_PARAM_VAL(p_codec),
                                               HL_ASYNCTASK_SET_PARAM_VAL(async_mb_start[i_i]),
                                               HL_ASYNCTASK_SET_PARAM_VAL(async_mb_end[i_i]),
                                               HL_ASYNCTASK_SET_PARAM_NULL());
#if 0 // FIXME
                    hl_asynctask_wait_2(p_codec->threads.pp_list_tasks[i_core_id], i_token_id);
#endif
                    if (err) {
                        return err;
                    }
                    ++i_i;
                    ++i_token_id;
                    i_core_id = (i_core_id + 1) % p_codec->threads.u_list_tasks_count; // FIXME
                    i_mb_start += (i_mb_end - i_mb_start);
                }
                while(!b_last);

                // Wait for all tasks
                for (i_core_id = 0; i_core_id < i_cores_count; ++i_core_id) {
                    for (i_i = 0; i_i < i_token_id; ++i_i) {
                        hl_asynctask_wait_2(p_codec->threads.pp_list_tasks[i_core_id], i_i);
                    }
                }
                pc_layer->i_mb_decode_count = i_mb_inter_to_decode_count;
            }
        }

        // Decode Intra (Single-threaded)
        if (i_mb_intra_to_decode_count > 0) {
            for (CurrMbAddr = 0; CurrMbAddr < pc_layer->i_mb_read_count; ++CurrMbAddr) {
                if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
                    HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
                    continue;
                }
                if (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mb)) {
                    if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb)) {
                        // alredy decoded in slice_decode()
                    }
                    else {
                        // 8.3 Intra prediction process
                        err = hl_codec_264_pred_intra_decode(p_codec, pc_mb);
                        if (err) {
                            goto bail;
                        }
                    }
                    ++pc_layer->i_mb_decode_count;
                }
            }
        }
    }
    else {
        for (CurrMbAddr = 0; CurrMbAddr < pc_layer->i_mb_read_count; ++CurrMbAddr) {
            if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
                HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
                continue;
            }
            // Intra Prediction
            if (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mb)) {
                if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mb)) {
                    // alredy decoded in slice_decode()
                }
                else {
                    // 8.3 Intra prediction process
                    err = hl_codec_264_pred_intra_decode(p_codec, pc_mb);
                    if (err) {
                        goto bail;
                    }
                }
            }
            else { // B or P
                // 8.4 Inter prediction process
                err = hl_codec_264_pred_inter_decode_with_mvs_and_refs_computation(p_codec, pc_mb);
                if (err) {
                    goto bail;
                }
            }
            ++pc_layer->i_mb_decode_count;
        }
    }

    /* Loop Filter */
    if (WorthCheckingLoopFilterFlag && pc_layer->i_mb_decode_count >= (int32_t)pc_layer->pc_slice_hdr->PicSizeInMbs) {
        err = hl_codec_264_deblock_avc(p_codec);
        if (err) {
            return err;
        }
    }

bail:

#if 0
    if (pc_layer->i_pict_decode_count == 1) {
        for (CurrMbAddr = 0; CurrMbAddr < pc_layer->i_mb_decode_count; ++CurrMbAddr) {
            hl_codec_264_mb_print_samples(pc_layer->pp_list_macroblocks[CurrMbAddr], pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_slice_hdr->PicWidthInSamplesL, 0/*Y*/);
            //hl_codec_264_mb_print_md5(pc_layer->pp_list_macroblocks[CurrMbAddr], pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->pc_slice_hdr->PicWidthInSamplesL, 0/*Y*/);
        }
        //hl_codec_264_mb_print_samples(pc_layer->pp_list_macroblocks[0], pc_layer->pc_fs_curr->p_pict->pc_data_u, pc_layer->pc_slice_hdr->PicWidthInSamplesC, 1/*U*/);
    }
#endif
    return err;
}
