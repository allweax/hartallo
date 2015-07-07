#include "hartallo/h264/hl_codec_264_deblock.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_asynctask.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

#if HL_HAVE_X86_INTRIN
#include "hartallo/h264/intrinsics/x86/hl_codec_x86_264_deblock_intrin.h"
#endif /* HL_HAVE_X86_INTRIN */

extern void (*hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8)(const uint8_t *pc_luma_samples, uint32_t u_luma_stride, int16_t bS[2][2], uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8)(uint8_t *p_luma_samples, uint32_t u_luma_stride, int16_t filterSamplesFlag[16], const uint8_t pf[3/*p0,p1,p2,p3*/][16], uint8_t const qf[3/*q0,q1,q2,q3*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8)(const uint8_t *pc_luma_samples, uint32_t u_luma_stride, uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8)(uint8_t *pc_luma_samples, uint32_t u_luma_stride, const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*qf0,qf1,qf2*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8)(const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride, uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8)(uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride, const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*qf0,qf1,qf2*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8)(const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride, int16_t bS[4], uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8)(uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride, int16_t filterSamplesFlag[16], const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*q0,q1,q2,q3*/][16]);
extern void (*hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8)(uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1, int16_t bS[2], int16_t alpha, int16_t beta, HL_OUT int16_t filterSamplesFlag[8]);
extern void (*hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8)(uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1, int16_t bS[4], int16_t alpha, int16_t beta, HL_OUT int16_t filterSamplesFlag[8]);
extern void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, int16_t bS[2], int16_t indexA, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);
extern void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, int16_t bS[4], int16_t indexA, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);
extern void (*hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8)(const uint8_t p0, const uint8_t p1, const uint8_t p2, const uint8_t p3, const uint8_t q0, const uint8_t q1, const uint8_t q2, const uint8_t q3, int32_t chromaStyleFilteringFlag, int16_t indexA, int16_t alpha, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);
extern void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3, int16_t alpha, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);
extern void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);

static const int16_t kMinNumSamplesToFilterUsingSIMD = 3;
static const int32_t __A = 0;
static const int32_t __B = 1;
static const int32_t iCbCr_0 = 0;
static const int32_t iCbCr_1 = 1;
static const int32_t Ek_0[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const int32_t Ek_1[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static const int32_t Ek_4[16] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
static const int32_t Ek_8[16] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
static const int32_t Ek_12[16] = {12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
static const int32_t Ek_0_to_15[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// Table 8-16 - Derivation of offset dependent threshold variables alpha' and beta' from indexA and indexB
const static int32_t HL_CODEC_264_DEBLOCK_ALPHA_TABLE[52]= {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 5, 6, 7, 8, 9, 10, 12, 13,
    15, 17,  20, 22, 25, 28, 32, 36, 40, 45,  50, 56, 63, 71, 80, 90, 101, 113, 127, 144, 162, 182, 203, 226, 255, 255
};
const static int32_t HL_CODEC_264_DEBLOCK_BETA_TABLE[52]= {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 3,  3, 3, 3, 4, 4, 4,
    6, 6,   7, 7, 8, 8, 9, 9, 10, 10,  11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18
};

// Table 8-17 – Value of variable t'c0 as a function of indexA and bS
// EXTERN because used in ASM and INTRIN code. Must use "int32_t" values to keep ASM code valid.
// FIXME: uncomment the const
HARTALLO_EXPORT /*const*/ int32_t HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[52][5]  = {
    {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, {0, 0, 0, 1, 1}, {0, 0, 0, 1, 1}, {0, 0, 0, 1, 1}, {0, 0, 0, 1, 1}, {0, 0, 1, 1, 1}, {0, 0, 1, 1, 1}, {0, 1, 1, 1, 1},
    {0, 1, 1, 1, 1}, {0, 1, 1, 1, 1}, {0, 1, 1, 1, 1}, {0, 1, 1, 2, 2}, {0, 1, 1, 2, 2}, {0, 1, 1, 2, 2}, {0, 1, 1, 2, 2}, {0, 1, 2, 3, 3},
    {0, 1, 2, 3, 3}, {0, 2, 2, 3, 3}, {0, 2, 2, 4, 4}, {0, 2, 3, 4, 4}, {0, 2, 3, 4, 4}, {0, 3, 3, 5, 5}, {0, 3, 4, 6, 6}, {0, 3, 4, 6, 6},
    {0, 4, 5, 7, 7}, {0, 4, 5, 8, 8}, {0, 4, 6, 9, 9}, {0, 5, 7, 10, 10}, {0, 6, 8, 11, 11}, {0, 6, 8, 13, 13}, {0, 7, 10, 14, 14}, {0, 8, 11, 16, 16},
    {0, 9, 12, 18, 18}, {0, 10, 13, 20, 20}, {0, 11, 15, 23, 23}, {0, 13, 17, 25, 25}
};

static const int32_t __lumaIdx = 0;
static const int32_t __chromaIdx = 1;

#define HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS(filterSamplesFlag) \
       ((filterSamplesFlag)[0] + (filterSamplesFlag)[1] + (filterSamplesFlag)[2] + (filterSamplesFlag)[3] + (filterSamplesFlag)[4] + (filterSamplesFlag)[5] + (filterSamplesFlag)[6] + (filterSamplesFlag)[7])

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_mb_init_flags(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
);
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_mb_init_flags(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
);

HL_ASYNC_CALL_DIRECT
HL_SHOULD_INLINE
static HL_ERROR_T hl_codec_264_deblock_avc_mb_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
);
HL_ASYNC_CALL_DIRECT
HL_SHOULD_INLINE
static HL_ERROR_T hl_codec_264_deblock_avc_mb_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
);

HL_ASYNC_CALL_INDIRECT
static HL_ERROR_T _hl_codec_264_deblock_avc_mb_async(
    const hl_asynctoken_param_xt* pc_params
);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_baseline_mb_luma_u8(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb);
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_baseline_mb_chroma_u8(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_FilterBlockEdges(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_codec_264_mb_t *pc_mbP, const hl_codec_264_mb_t *pc_mbQ,
    const int32_t* xEk, const int32_t* yEk,
    int32_t chromaEdgeFlag,
    int32_t iCbCr,
    int32_t verticalEdgeFlag,
    int32_t fieldModeInFrameFilteringFlag
);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_FilterHzOrVtBlockEdges(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_codec_264_mb_t* pc_mbP, const hl_codec_264_mb_t* pc_mbQ,
    const int32_t p[4], const int32_t q[4],
    int32_t pf[3], int32_t qf[3],
    int32_t iCbCr,
    int32_t chromaEdgeFlag, int32_t verticalEdgeFlag,
    hl_bool_t is_mb_edge,
    int32_t mbPx, int32_t mbPy, int32_t mbQx, int32_t mbQy);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_get_luma_bS(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_codec_264_mb_t* p_mbP, const hl_codec_264_mb_t* p_mbQ,
    int32_t p0, int32_t q0,
    int32_t verticalEdgeFlag,
    hl_bool_t is_mb_edge,
    int32_t mbPx, int32_t mbPy, int32_t mbQx, int32_t mbQy,
    int32_t *bS);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_get_threshold(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t p0, int32_t q0, int32_t p1, int32_t q1,
    int32_t chromaEdgeFlag,
    int32_t bS,
    int32_t filterOffsetA, int32_t filterOffsetB,
    int32_t qPp, int32_t qPq,
    int32_t* filterSamplesFlag,
    int32_t* indexA,
    int32_t* alpha, int32_t* beta);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_filter_bs_lt4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const int32_t* p, const int32_t* q,
    int32_t chromaEdgeFlag, int32_t chromaStyleFilteringFlag,
    int32_t bS, int32_t beta, int32_t indexA,
    int32_t* pf, int32_t* qf);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_filter_bs_eq4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const int32_t* p, const int32_t* q,
    int32_t chromaEdgeFlag, int32_t chromaStyleFilteringFlag,
    int32_t alpha, int32_t beta,
    int32_t* pf, int32_t* qf);

// G.8.7.1 Deblocking filter process for Intra_Base prediction
HL_ERROR_T hl_codec_264_deblock_intra_base_svc(hl_codec_264_t* p_codec, int32_t currDQId)
{
    if (p_codec->layers.pc_active->pc_slice_hdr->ext.svc.disable_inter_layer_deblocking_filter_idc != 1) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    // FIXME: Not implemented
    return HL_ERROR_SUCCESS;
}

// G.8.7.2 Deblocking filter process for target representations
HL_ERROR_T hl_codec_264_deblock_target_reps_svc(hl_codec_264_t* p_codec, int32_t currDQId)
{
    // FIXME: Not implemented
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_deblock_avc(hl_codec_264_t* p_codec)
{
    hl_codec_264_mb_t* pc_mb;
    hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;
    hl_size_t CurrMbAddr, u_min_mb_per_thread;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    u_min_mb_per_thread = pc_layer->u_list_macroblocks_count; // FIXME: clip using user-defined config

    if (p_codec->threads.u_list_tasks_count < 2 || u_min_mb_per_thread > pc_layer->u_list_macroblocks_count) { // FIXME
        // Single-threaded
        for (CurrMbAddr = 0; CurrMbAddr < pc_layer->u_list_macroblocks_count; ++CurrMbAddr) {
            if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
                HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
                continue;
            }
            if (pc_layer->p_list_slices[pc_mb->u_slice_idx]->p_header->disable_deblocking_filter_idc != 1) { // FIXME: must check for each slice
                err = hl_codec_264_deblock_avc_mb(p_codec, pc_mb);
                if (err) {
                    goto bail;
                }
            }
#if 0
            hl_codec_264_mb_print_samples(pc_layer->pp_list_macroblocks[0], pc_layer->pc_fs_curr->p_pict->pc_data_y, pc_layer->p_list_slices[0]->p_header->PicWidthInSamplesL, 0/*Y*/);
#endif
        }
    }
    else {
        // Multi-threaded
        HL_ERROR_T (*f_hl_codec_264_deblock_avc_mb_luma)(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
        HL_ERROR_T (*f_hl_codec_264_deblock_avc_mb_chroma)(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);
        void (*f_hl_codec_264_deblock_avc_mb_init_flags)(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb);

        hl_size_t u;
        hl_bool_t is_baseline = HL_TRUE;
        static int32_t __MbStartZero = 0;
        static hl_asynctoken_id_t __ToKenId_Luma = 5;
        static hl_asynctoken_id_t __ToKenId_Chroma = 6;
        int32_t i_core_id_luma, i_core_id_chroma, i_mb_end;

        i_mb_end = (int32_t)pc_layer->u_list_macroblocks_count;

        // Check whether all slices are baseline
        for (u = 0; u < pc_layer->u_list_slices_count; ++u) {
            if(!(is_baseline = hl_codec_264_is_baseline(pc_layer->p_list_slices[u]->p_header->pc_pps->pc_sps))) {
                break;
            }
        }

        // Set function pointers
        if (is_baseline && sizeof(hl_pixel_t) == 1) {
            f_hl_codec_264_deblock_avc_mb_luma = hl_codec_264_deblock_avc_baseline_mb_luma_u8;
            f_hl_codec_264_deblock_avc_mb_chroma = hl_codec_264_deblock_avc_baseline_mb_chroma_u8;
            f_hl_codec_264_deblock_avc_mb_init_flags = hl_codec_264_deblock_avc_baseline_mb_init_flags;
        }
        else {
            f_hl_codec_264_deblock_avc_mb_luma = hl_codec_264_deblock_avc_mb_luma;
            f_hl_codec_264_deblock_avc_mb_chroma = hl_codec_264_deblock_avc_mb_chroma;
            f_hl_codec_264_deblock_avc_mb_init_flags = hl_codec_264_deblock_avc_mb_init_flags;
        }

        // Init flags
        for (CurrMbAddr = 0; CurrMbAddr < pc_layer->u_list_macroblocks_count; ++CurrMbAddr) {
            if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
                HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
                continue;
            }
            f_hl_codec_264_deblock_avc_mb_init_flags(p_codec, pc_mb);
        }

        // Run LoopFilter on Chroma and Luma using #1 thread for each component
        i_core_id_luma = (hl_thread_get_core_id() + 1) % p_codec->threads.u_list_tasks_count;
        err = hl_asynctask_execute(p_codec->threads.pp_list_tasks[i_core_id_luma], __ToKenId_Luma, _hl_codec_264_deblock_avc_mb_async,
                                   HL_ASYNCTASK_SET_PARAM_VAL(p_codec),
                                   HL_ASYNCTASK_SET_PARAM_VAL(__MbStartZero),
                                   HL_ASYNCTASK_SET_PARAM_VAL(i_mb_end),
                                   HL_ASYNCTASK_SET_PARAM_VAL(f_hl_codec_264_deblock_avc_mb_luma),
                                   HL_ASYNCTASK_SET_PARAM_NULL());
        i_core_id_chroma = (i_core_id_luma + 1) % p_codec->threads.u_list_tasks_count;
        err = hl_asynctask_execute(p_codec->threads.pp_list_tasks[i_core_id_chroma], __ToKenId_Chroma, _hl_codec_264_deblock_avc_mb_async,
                                   HL_ASYNCTASK_SET_PARAM_VAL(p_codec),
                                   HL_ASYNCTASK_SET_PARAM_VAL(__MbStartZero),
                                   HL_ASYNCTASK_SET_PARAM_VAL(i_mb_end),
                                   HL_ASYNCTASK_SET_PARAM_VAL(f_hl_codec_264_deblock_avc_mb_chroma),
                                   HL_ASYNCTASK_SET_PARAM_NULL());

        hl_asynctask_wait_2(p_codec->threads.pp_list_tasks[i_core_id_chroma], __ToKenId_Chroma);
        hl_asynctask_wait_2(p_codec->threads.pp_list_tasks[i_core_id_luma], __ToKenId_Luma);
    }

bail:
    return err;
}

HL_ASYNC_CALL_DIRECT
HL_SHOULD_INLINE
static HL_ERROR_T hl_codec_264_deblock_avc_mb_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
)
{
    const int32_t* xEk, *yEk;
    int32_t chromaEdgeFlag, verticalEdgeFlag, fieldModeInFrameFilteringFlag;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_mb_t *pc_mbQ, *pc_mbP;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;

    pc_mbQ = pc_mbP = p_mb;

    if (p_mb->deblock.filterLeftMbEdgeFlag == 1) {
        //a. left vertical luma edge
        pc_mbP = pc_layer->pp_list_macroblocks[p_mb->u_addr - 1];
        chromaEdgeFlag = 0;
        verticalEdgeFlag = 1;
        fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
        xEk = Ek_0;
        yEk = Ek_0_to_15;
        err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
    }

    //b. internal vertical luma edges
    if (p_mb->deblock.filterInternalEdgesFlag == 1) {
        pc_mbP = pc_mbQ;
        chromaEdgeFlag = 0;
        verticalEdgeFlag = 1;
        fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
        if (p_mb->transform_size_8x8_flag == 0) {
            //i.
            xEk = Ek_4;
            yEk = Ek_0_to_15;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }

        //ii.
        xEk = Ek_8;
        yEk = Ek_0_to_15;
        err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);

        if (p_mb->transform_size_8x8_flag == 0) {
            //iii.
            xEk = Ek_12;
            yEk = Ek_0_to_15;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }
    }

    //c. top horizontal luma edge
    if (p_mb->deblock.filterTopMbEdgeFlag == 1) {
        const hl_codec_264_mb_t* mb = pc_layer->pp_list_macroblocks[p_mb->u_addr - 2 * pc_slice_header->PicWidthInMbs + 1];
        if ((pc_slice_header->MbaffFrameFlag == 1) &&
                ((p_mb->u_addr & 1) == 0) &&
                (p_mb->u_addr >= (int32_t)(pc_slice_header->PicWidthInMbs << 1)) &&
                (!p_mb->mb_field_decoding_flag) && // FIXME: frame MB
                (mb && mb->mb_field_decoding_flag)) { // FIXME: field MB
            // i.
            pc_mbP = pc_layer->pp_list_macroblocks[p_mb->u_addr - pc_slice_header->PicWidthInMbs];
            chromaEdgeFlag = 0;
            verticalEdgeFlag = 0;
            fieldModeInFrameFilteringFlag = 1;
            xEk = Ek_0_to_15;
            yEk = Ek_0;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            // ii.
            xEk = Ek_0_to_15;
            yEk = Ek_1;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }
        else {
            pc_mbP = pc_layer->pp_list_macroblocks[p_mb->u_addr - pc_slice_header->PicWidthInMbs];
            chromaEdgeFlag = 0;
            verticalEdgeFlag = 0;
            fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
            xEk = Ek_0_to_15;
            yEk = Ek_0;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }
    }

    // d. internal horizontal luma edges
    if (p_mb->deblock.filterInternalEdgesFlag == 1) {
        pc_mbP = pc_mbQ;
        chromaEdgeFlag = 0;
        verticalEdgeFlag = 0;
        fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
        if (p_mb->transform_size_8x8_flag == 0) {
            //i.
            xEk = Ek_0_to_15;
            yEk = Ek_4;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }
        //ii.
        xEk = Ek_0_to_15;
        yEk = Ek_8;
        err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        if (p_mb->transform_size_8x8_flag == 0) {
            //iii.
            xEk = Ek_0_to_15;
            yEk = Ek_12;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, 0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }
    }

    return err;
}

HL_ASYNC_CALL_DIRECT
HL_SHOULD_INLINE
static HL_ERROR_T hl_codec_264_deblock_avc_mb_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
)
{
    const int32_t* xEk, *yEk;
    int32_t chromaEdgeFlag, verticalEdgeFlag, fieldModeInFrameFilteringFlag;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_mb_t *pc_mbQ, *pc_mbP;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    pc_mbQ = pc_mbP = p_mb;

    // e. both chroma components
    if (pc_sps->ChromaArrayType != 0) {
        //i. vertical chroma edge
        if (p_mb->deblock.filterLeftMbEdgeFlag == 1) {
            pc_mbP = pc_layer->pp_list_macroblocks[p_mb->u_addr - 1];
            chromaEdgeFlag = 1;
            verticalEdgeFlag = 1;
            fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
            xEk = Ek_0;
            yEk = Ek_0_to_15;
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                    chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
        }

        // ii. internal vertical chroma edge
        if (p_mb->deblock.filterInternalEdgesFlag == 1) {
            pc_mbP = pc_mbQ;
            chromaEdgeFlag = 1;
            verticalEdgeFlag = 1;
            fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
            if (pc_sps->ChromaArrayType != 3 || p_mb->transform_size_8x8_flag == 0) {
                //(1)
                xEk = Ek_4;
                yEk = Ek_0_to_15;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
            if (pc_sps->ChromaArrayType == 3) {
                //(2)
                xEk = Ek_8;
                yEk = Ek_0_to_15;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
            if (pc_sps->ChromaArrayType == 3 && p_mb->transform_size_8x8_flag == 0) {
                //(3)
                xEk = Ek_12;
                yEk = Ek_0_to_15;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
        }

        // iii. top horizontal chroma edge
        if (p_mb->deblock.filterTopMbEdgeFlag == 1) {
            const hl_codec_264_mb_t* mb = pc_layer->pp_list_macroblocks[p_mb->u_addr - 2 * pc_slice_header->PicWidthInMbs + 1];
            if ((pc_slice_header->MbaffFrameFlag == 1) &&
                    ((p_mb->u_addr & 1) == 0) &&
                    (p_mb->u_addr >= (int32_t)(pc_slice_header->PicWidthInMbs << 1)) &&
                    (!p_mb->mb_field_decoding_flag) && // FIXME: frame MB
                    (mb && mb->mb_field_decoding_flag)) { // FIXME: field MB
                pc_mbP = pc_layer->pp_list_macroblocks[p_mb->u_addr - pc_slice_header->PicWidthInMbs];
                chromaEdgeFlag = 1;
                verticalEdgeFlag = 0;
                fieldModeInFrameFilteringFlag = 1;
                // (1)
                xEk = Ek_0_to_15;
                yEk = Ek_0;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                // (2)
                xEk = Ek_0_to_15;
                yEk = Ek_1;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
            else {
                pc_mbP = pc_layer->pp_list_macroblocks[p_mb->u_addr - pc_slice_header->PicWidthInMbs];
                chromaEdgeFlag = 1;
                verticalEdgeFlag = 0;
                fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
                xEk = Ek_0_to_15;
                yEk = Ek_0;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
        }

        // iv. internal horizontal chroma edge
        if (p_mb->deblock.filterInternalEdgesFlag == 1) {
            pc_mbP = pc_mbQ;
            chromaEdgeFlag = 1;
            verticalEdgeFlag = 0;
            fieldModeInFrameFilteringFlag = p_mb->deblock.fieldMbInFrameFlag;
            if (pc_sps->ChromaArrayType != 3 || p_mb->transform_size_8x8_flag == 0) {
                // (1)
                xEk = Ek_0_to_15;
                yEk = Ek_4;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
            if (pc_sps->ChromaArrayType != 1) {
                // (2)
                xEk = Ek_0_to_15;
                yEk = Ek_8;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
            if (pc_sps->ChromaArrayType == 2) {
                // (3)
                xEk = Ek_0_to_15;
                yEk = Ek_12;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
            if (pc_sps->ChromaArrayType == 3 && p_mb->transform_size_8x8_flag == 0) {
                // (3)
                xEk = Ek_0_to_15;
                yEk = Ek_12;
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_0, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
                err = hl_codec_264_deblock_avc_FilterBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ, xEk, yEk,
                        chromaEdgeFlag, iCbCr_1, verticalEdgeFlag, fieldModeInFrameFilteringFlag);
            }
        }
    }

    return err;
}

// 8.7 Deblocking filter process
HL_ERROR_T hl_codec_264_deblock_avc_mb(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
)
{
    HL_ERROR_T err;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    if (1 && hl_codec_264_is_baseline(pc_sps)) { // FIXME
        hl_codec_264_deblock_avc_baseline_mb_init_flags(p_codec, p_mb);

        err = hl_codec_264_deblock_avc_baseline_mb_luma_u8(p_codec, p_mb);
        if (err) {
            return err;
        }
        if (pc_sps->ChromaArrayType != 0) {
            err = hl_codec_264_deblock_avc_baseline_mb_chroma_u8(p_codec, p_mb);
            if (err) {
                return err;
            }
        }
    }
    else {
        hl_codec_264_deblock_avc_mb_init_flags(p_codec, p_mb);

        err = hl_codec_264_deblock_avc_mb_luma(p_codec, p_mb);
        if (err) {
            return err;
        }
        if (pc_sps->ChromaArrayType != 0) {
            err = hl_codec_264_deblock_avc_mb_chroma(p_codec, p_mb);
            if (err) {
                return err;
            }
        }
    }
    return err;
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_mb_init_flags(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
)
{
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;

    p_mb->deblock.fieldMbInFrameFlag = 0;

    if (pc_slice_header->disable_deblocking_filter_idc == 1) {
        p_mb->deblock.filterInternalEdgesFlag = 0;
        p_mb->deblock.filterLeftMbEdgeFlag = 0;
        p_mb->deblock.filterTopMbEdgeFlag = 0;
    }
    else {
        // INTER16x16 without residual -> bS=0
        p_mb->deblock.filterInternalEdgesFlag = ((HL_CODEC_264_MB_TYPE_IS_P_16X16(p_mb) || HL_CODEC_264_MB_TYPE_IS_SKIP(p_mb)) && !p_mb->CodedBlockPatternLuma) ? 0 : 1;
        if (pc_slice_header->disable_deblocking_filter_idc == 2) {
            // when disable_deblocking_filter_idc is equal to 2, macroblocks in different slices are considered not available during specified
            // steps of the operation of the deblocking filter process.
            // "b_avail_X" also checks that MBs are in the same slice
            p_mb->deblock.filterLeftMbEdgeFlag = p_mb->neighbours.b_avail_A ? 1 : 0;
            p_mb->deblock.filterTopMbEdgeFlag = p_mb->neighbours.b_avail_B ? 1 : 0;
        }
        else {
            p_mb->deblock.filterLeftMbEdgeFlag = p_mb->u_x ? 1 : 0;
            p_mb->deblock.filterTopMbEdgeFlag = p_mb->u_y ? 1 : 0;
        }
    }
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_mb_init_flags(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb
)
{
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    int32_t mbAddrA, mbAddrB;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;

    if (pc_slice_header->disable_deblocking_filter_idc == 1) {
        p_mb->deblock.filterInternalEdgesFlag = 0;
        p_mb->deblock.filterLeftMbEdgeFlag = 0;
        p_mb->deblock.filterTopMbEdgeFlag = 0;
    }
    else {
        int32_t xW, yW;
        static const hl_bool_t isLuma_Yes = HL_TRUE;

        // 6.4.10.1 Derivation process for neighbouring macroblocks
        hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, xD_yD[__A][0], xD_yD[__A][1], &mbAddrA, &xW, &yW, isLuma_Yes);
        hl_codec_264_utils_derivation_process_for_neighbouring_locations(p_codec, p_mb, xD_yD[__B][0], xD_yD[__B][1], &mbAddrB, &xW, &yW, isLuma_Yes);

        p_mb->deblock.fieldMbInFrameFlag = (pc_slice_header->MbaffFrameFlag == 1 && p_mb->mb_field_decoding_flag == 1) ? 1 : 0;
        p_mb->deblock.filterInternalEdgesFlag = 1;

        if ((pc_slice_header->MbaffFrameFlag == 0 && (p_mb->u_addr % pc_slice_header->PicWidthInMbs) == 0) ||
                (pc_slice_header->MbaffFrameFlag == 1 && ((p_mb->u_addr >> 1) % pc_slice_header->PicWidthInMbs) == 0) ||
                (pc_slice_header->disable_deblocking_filter_idc == 2 && HL_MATH_IS_NEGATIVE_INT32(mbAddrA))) {
            p_mb->deblock.filterLeftMbEdgeFlag = 0;
        }
        else {
            p_mb->deblock.filterLeftMbEdgeFlag = 1;
        }

        if ((pc_slice_header->MbaffFrameFlag == 0 && (p_mb->u_addr < (int32_t)pc_slice_header->PicWidthInMbs)) ||
                (pc_slice_header->MbaffFrameFlag == 1 && ((p_mb->u_addr >> 1) < (int32_t)pc_slice_header->PicWidthInMbs) && p_mb->mb_field_decoding_flag) ||
                (pc_slice_header->MbaffFrameFlag == 1 && ((p_mb->u_addr >> 1) < (int32_t)pc_slice_header->PicWidthInMbs) && !p_mb->mb_field_decoding_flag && (p_mb->u_addr % 2) == 0) ||
                (pc_slice_header->disable_deblocking_filter_idc == 2 && HL_MATH_IS_NEGATIVE_INT32(mbAddrB))
           ) {
            p_mb->deblock.filterTopMbEdgeFlag = 0;
        }
        else {
            p_mb->deblock.filterTopMbEdgeFlag = 1;
        }
    }
}

HL_ASYNC_CALL_INDIRECT
static HL_ERROR_T _hl_codec_264_deblock_avc_mb_async(
    const hl_asynctoken_param_xt* pc_params)
{
    int32_t CurrMbAddr;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_mb_t* pc_mb;
    hl_codec_264_layer_t* pc_layer;
    HL_ERROR_T(*f_hl_codec_264_deblock_avc_mb)(hl_codec_264_t* p_codec, hl_codec_264_mb_t* pc_mb);

    hl_codec_264_t* p_codec = HL_ASYNCTASK_GET_PARAM(pc_params[0].pc_param_ptr, hl_codec_264_t*);
    int32_t i_mb_start = HL_ASYNCTASK_GET_PARAM(pc_params[1].pc_param_ptr, int32_t);
    int32_t i_mb_end = HL_ASYNCTASK_GET_PARAM(pc_params[2].pc_param_ptr, int32_t);
    f_hl_codec_264_deblock_avc_mb = HL_ASYNCTASK_GET_PARAM(pc_params[3].pc_param_ptr, void*);

    pc_layer = p_codec->layers.pc_active;

    // HL_DEBUG_INFO("_hl_codec_264_deblock_avc_mb_async(core_id=%d, i_mb_start=%d, i_mb_end=%d)", hl_thread_get_core_id(), i_mb_start, i_mb_end);

    for (CurrMbAddr = i_mb_start; CurrMbAddr < i_mb_end; ++CurrMbAddr) {
        if (!(pc_mb = pc_layer->pp_list_macroblocks[CurrMbAddr])) {
            HL_DEBUG_ERROR("Failed to find macroblock at index %d", CurrMbAddr);
            continue;
        }
        err = f_hl_codec_264_deblock_avc_mb(p_codec, pc_mb);
        if (err) {
            return err;
        }
    }
    return err;
}



// 8.7.1 Filtering process for block edges
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_FilterBlockEdges(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_codec_264_mb_t *pc_mbP, const hl_codec_264_mb_t *pc_mbQ,
    const int32_t* xEk, const int32_t* yEk,
    int32_t chromaEdgeFlag,
    int32_t iCbCr,
    int32_t verticalEdgeFlag,
    int32_t fieldModeInFrameFilteringFlag)
{
    int32_t nE, dy, xP, yP, k, s_width, index, mbPx, mbPy, mbQx, mbQy;
    int32_t p[4], q[4], pf[3], qf[3];
    hl_pixel_t* s_data;
    hl_bool_t is_mb_edge;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_nal_sps_t* pc_sps;
    HL_ERROR_T err;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    if (chromaEdgeFlag == 0) {
        nE = 16;
        if (pc_sps->separate_colour_plane_flag == 0) {
            s_data = pc_layer->pc_fs_curr->p_pict->pc_data_y;
            s_width = pc_layer->pc_fs_curr->p_pict->uWidthL;
        }
        else {
            s_data = (pc_slice_header->colour_plane_id == 0)
                     ? pc_layer->pc_fs_curr->p_pict->pc_data_y
                     : (pc_slice_header->colour_plane_id == 1 ? pc_layer->pc_fs_curr->p_pict->pc_data_u : pc_layer->pc_fs_curr->p_pict->pc_data_v);
            s_width = (pc_slice_header->colour_plane_id == 0) ? pc_layer->pc_fs_curr->p_pict->uWidthL : pc_layer->pc_fs_curr->p_pict->uWidthC;
            // TODO:
            // s_data = p_codec->currPic->frame->data_ptr[pc_slice_header->colour_plane_id];
            // s_width = p_codec->currPic->frame->data_width[pc_slice_header->colour_plane_id];
        }
    }
    else {
        nE = (verticalEdgeFlag == 1) ? pc_sps->MbHeightC : pc_sps->MbWidthC;
        if (iCbCr == 0) {
            s_data = pc_layer->pc_fs_curr->p_pict->pc_data_u/*TODO: p_codec->currPic->frame->data_ptr[1]*/;
            s_width = pc_layer->pc_fs_curr->p_pict->uWidthC /*TODO: p_codec->currPic->frame->data_width[1]*/;
        }
        else {
            s_data = pc_layer->pc_fs_curr->p_pict->pc_data_v /*TODO: p_codec->currPic->frame->data_ptr[2]*/;
            s_width = pc_layer->pc_fs_curr->p_pict->uWidthC /*TODO: p_codec->currPic->frame->data_width[2]*/;
        }
    }

    dy = (1 + fieldModeInFrameFilteringFlag);

    if (chromaEdgeFlag == 0) {
        xP = p_mb->xL;
        yP = p_mb->yL;
    }
    else {
        xP = p_mb->xC;
        yP = p_mb->yC;
    }

    if (verticalEdgeFlag == 1) {
        for (k=0; k<nE; ++k) {
            is_mb_edge = (xEk[k] == 0 || xEk[k] == (nE - 1)); // FIXME: remove "xEk[k] == (nE - 1)"

            mbQx = xEk[k];
            mbQy = dy * yEk[k];
            mbPx = xEk[k] - 1;
            if (mbPx < 0) {
                mbPx += nE;
            }
            mbPy = dy * yEk[k];

            // (8-450)
            index = (xP + xEk[k]) + ((yP + (dy * yEk[k])) * s_width);
            q[0] = s_data[index];
            q[1] = s_data[index + 1];
            q[2] = s_data[index + 2];
            q[3] = s_data[index + 3];

            // (8-451)
            index = (xP + (xEk[k] - 1)) + ((yP + (dy * yEk[k])) * s_width);
            p[0] = s_data[index];
            p[1] = s_data[index - 1];
            p[2] = s_data[index - 2];
            p[3] = s_data[index - 3];

            // 8.7.2 Filtering process for a set of samples across a horizontal or vertical block edge
            err = hl_codec_264_deblock_avc_FilterHzOrVtBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ,
                    p, q,
                    pf, qf,
                    iCbCr,
                    chromaEdgeFlag, verticalEdgeFlag,
                    is_mb_edge,
                    mbPx, mbPy, mbQx, mbQy);

            // (8-454)
            s_data[(xP + xEk[k] + 0) + ((yP + dy * yEk[k]) * s_width)] = qf[0];
            s_data[(xP + xEk[k] + 1) + ((yP + dy * yEk[k]) * s_width)] = qf[1];
            s_data[(xP + xEk[k] + 2) + ((yP + dy * yEk[k]) * s_width)] = qf[2];
            // (8-455)
            s_data[(xP + xEk[k] - 0 - 1) + ((yP + dy * yEk[k]) * s_width)] = pf[0];
            s_data[(xP + xEk[k] - 1 - 1) + ((yP + dy * yEk[k]) * s_width)] = pf[1];
            s_data[(xP + xEk[k] - 2 - 1) + ((yP + dy * yEk[k]) * s_width)] = pf[2];
        }
    }
    else {
        for (k=0; k<nE; ++k) {
            //is_mb_edge = (yEk[k] == 0 || yEk[k] == (nE - 1));
            is_mb_edge = (/*xEk[k] == 0 || xEk[k] == (nE - 1) ||*/ yEk[k] == 0 || yEk[k] == (nE - 1));

            mbQx = xEk[k];
            mbQy = (dy * yEk[k] - (yEk[k] & 1));
            if (mbQy < 0) {
                mbQy += nE;
            }
            mbPx = xEk[k];
            mbPy = (dy * (yEk[k] - 1) - (yEk[k] & 1));
            if (mbPy < 0) {
                mbPy += nE;
            }

            // (8-452)
            q[0] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 0) - (yEk[k] & 1)) * s_width)];
            q[1] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 1) - (yEk[k] & 1)) * s_width)];
            q[2] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 2) - (yEk[k] & 1)) * s_width)];
            q[3] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 3) - (yEk[k] & 1)) * s_width)];

            // (8-453)
            p[0] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 0 - 1) - (yEk[k] & 1)) * s_width)];
            p[1] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 1 - 1) - (yEk[k] & 1)) * s_width)];
            p[2] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 2 - 1) - (yEk[k] & 1)) * s_width)];
            p[3] = s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 3 - 1) - (yEk[k] & 1)) * s_width)];

            // 8.7.2 Filtering process for a set of samples across a horizontal or vertical block edge
            err = hl_codec_264_deblock_avc_FilterHzOrVtBlockEdges(p_codec, p_mb, pc_mbP, pc_mbQ,
                    p, q,
                    pf, qf,
                    iCbCr,
                    chromaEdgeFlag, verticalEdgeFlag,
                    is_mb_edge,
                    mbPx, mbPy, mbQx, mbQy);

            // (8-456)
            s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 0) - (yEk[k] & 1)) * s_width)] = qf[0];
            s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 1) - (yEk[k] & 1)) * s_width)] = qf[1];
            s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] + 2) - (yEk[k] & 1)) * s_width)] = qf[2];
            // (8-457)
            s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 0 - 1) - (yEk[k] & 1)) * s_width)] = pf[0];
            s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 1 - 1) - (yEk[k] & 1)) * s_width)] = pf[1];
            s_data[(xP + xEk[k]) + ((yP + dy * (yEk[k] - 2 - 1) - (yEk[k] & 1)) * s_width)] = pf[2];
        }
    }

    return err;
}

// 8.7.2 Filtering process for a set of samples across a horizontal or vertical block edge
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_FilterHzOrVtBlockEdges(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_codec_264_mb_t* pc_mbP, const hl_codec_264_mb_t* pc_mbQ,
    const int32_t p[4], const int32_t q[4],
    int32_t pf[3], int32_t qf[3],
    int32_t iCbCr,
    int32_t chromaEdgeFlag, int32_t verticalEdgeFlag,
    hl_bool_t is_mb_edge,
    int32_t mbPx, int32_t mbPy, int32_t mbQx, int32_t mbQy)
{
    HL_ERROR_T err;
    int32_t bS, filterOffsetA, filterOffsetB, qPp, qPq;
    int32_t filterSamplesFlag, indexA, alpha, beta;
    int32_t chromaStyleFilteringFlag;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t *pc_slice_headerP, *pc_slice_headerQ;
    const hl_codec_264_nal_sps_t *pc_spsP, *pc_spsQ;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_headerP = pc_layer->p_list_slices[pc_mbP->u_slice_idx]->p_header;
    pc_slice_headerQ = pc_layer->p_list_slices[pc_mbQ->u_slice_idx]->p_header;
    pc_spsP = pc_slice_headerP->pc_pps->pc_sps;
    pc_spsQ = pc_slice_headerQ->pc_pps->pc_sps;

    if (pc_slice_headerP->MbaffFrameFlag == 1 || pc_slice_headerQ->MbaffFrameFlag == 1) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // Let filterOffsetA and filterOffsetB be the values of FilterOffsetA and FilterOffsetB as specified in subclause 7.4.3 for
    // the slice that contains the macroblock containing sample q0.
    filterOffsetA = pc_slice_headerQ->FilterOffsetA;
    filterOffsetB = pc_slice_headerQ->FilterOffsetB;

    // 8.7.2.1 Derivation process for the luma content dependent boundary filtering strength (bS)
    if (chromaEdgeFlag == 0) {
        err = hl_codec_264_deblock_avc_get_luma_bS(p_codec, p_mb, pc_mbP, pc_mbQ, p[0], q[0],
                verticalEdgeFlag, is_mb_edge, mbPx, mbPy, mbQx, mbQy, &bS);
        qPp = HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbP) ? 0 : pc_mbP->QPy;
        qPq = HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbQ) ? 0 : pc_mbQ->QPy;
    }
    else {
        // Otherwise (chromaEdgeFlag is equal to 1), the bS used for filtering a set of samples of a horizontal or vertical
        // chroma edge is set equal to the value of bS for filtering the set of samples of a horizontal or vertical luma edge,
        // respectively, that contains the luma sample at location ( SubWidthC * x, SubHeightC * y ) inside the luma array of
        // the same field, where ( x, y ) is the location of the chroma sample q0 inside the chroma array for that field.
        static const int32_t QPyZero = 0;
        err = hl_codec_264_deblock_avc_get_luma_bS(p_codec, p_mb, pc_mbP, pc_mbQ, p[0], q[0],
                verticalEdgeFlag, is_mb_edge, (mbPx * pc_spsQ->SubWidthC), (mbPy * pc_spsQ->SubHeightC), (mbQx * pc_spsQ->SubWidthC), (mbQy * pc_spsQ->SubHeightC), &bS);
        if (HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbP)) {
            int32_t qPI = HL_MATH_CLIP3(-pc_spsP->QpBdOffsetC, 51, QPyZero + pc_mbP->qPOffset[iCbCr]);
            qPp = qPI2QPC[qPI];
        }
        else {
            qPp = pc_mbP->QPc[iCbCr];
        }
        if (HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbQ)) {
            int32_t qPI = HL_MATH_CLIP3(-pc_spsQ->QpBdOffsetC, 51, QPyZero + pc_mbQ->qPOffset[iCbCr]);
            qPq = qPI2QPC[qPI];
        }
        else {
            qPq = pc_mbQ->QPc[iCbCr];
        }
    }

    // 8.7.2.2 Derivation process for the thresholds for each block edge
    err = hl_codec_264_deblock_avc_get_threshold(p_codec, p_mb,
            p[0], q[0], p[1], q[1],
            chromaEdgeFlag,
            bS,
            filterOffsetA, filterOffsetB,
            qPp, qPq,
            &filterSamplesFlag,
            &indexA,
            &alpha, &beta);
    if (err) {
        return err;
    }

    // FIXME:
    //filterSamplesFlag = bS < 4 ? 1 : filterSamplesFlag;

    chromaStyleFilteringFlag = chromaEdgeFlag && (pc_spsQ->ChromaArrayType != 3);// (8-458)

    if (filterSamplesFlag == 1) {
        if (bS < 4) {
            // 8.7.2.3 Filtering process for edges with bS less than 4
            err = hl_codec_264_deblock_avc_filter_bs_lt4(p_codec, p_mb,
                    p, q,
                    chromaEdgeFlag, chromaStyleFilteringFlag,
                    bS, beta, indexA,
                    pf, qf);
            if (err) {
                return err;
            }
        }
        else {
            // 8.7.2.4 Filtering process for edges for bS equal to 4
            err = hl_codec_264_deblock_avc_filter_bs_eq4(p_codec, p_mb,
                    p, q,
                    chromaEdgeFlag, chromaStyleFilteringFlag,
                    alpha, beta,
                    pf, qf);
        }
    }
    else {
        // (8-459)
        pf[0] = p[0];
        pf[1] = p[1];
        pf[2] = p[2];
        // (8-460)
        qf[0] = q[0];
        qf[1] = q[1];
        qf[2] = q[2];
    }

    return err;
}

// 8.7.2.1 Derivation process for the luma content dependent boundary filtering strength
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_get_luma_bS(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const hl_codec_264_mb_t* pc_mbP, const hl_codec_264_mb_t* pc_mbQ,
    int32_t p0, int32_t q0,
    int32_t verticalEdgeFlag,
    hl_bool_t is_mb_edge,
    int32_t mbPx, int32_t mbPy, int32_t mbQx, int32_t mbQy,
    int32_t *bS)
{
    int32_t mixedModeEdgeFlag;
    int32_t qLuma4x4BlkIdx, pLuma4x4BlkIdx;
    int32_t cond = 0;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t *pc_slice_header, *pc_slice_headerP, *pc_slice_headerQ;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_slice_headerP = pc_layer->p_list_slices[pc_mbP->u_slice_idx]->p_header;
    pc_slice_headerQ = pc_layer->p_list_slices[pc_mbQ->u_slice_idx]->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    *bS = 0;

    pLuma4x4BlkIdx = LumaBlockIndices4x4_YX[mbPy][mbPx];
    qLuma4x4BlkIdx = LumaBlockIndices4x4_YX[mbQy][mbQx];

    // FIXME:
    /*if (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbQ) || HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbP)) {
        *bS = is_mb_edge ? 4 : 3;
        return HL_ERROR_SUCCESS;
    }
    else {
    	*bS = 3;
    	return 0;
    }*/

    if (pc_slice_header->MbaffFrameFlag == 1) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }
    else {
        mixedModeEdgeFlag = 0;
    }

    /*====== COND0 ======*/
    if (is_mb_edge) {
        cond =
            !pc_mbP->mb_field_decoding_flag
            && !pc_mbQ->mb_field_decoding_flag
            && (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbQ) || HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbP));
        if (!cond) {
            cond =
                !pc_mbP->mb_field_decoding_flag
                && !pc_mbQ->mb_field_decoding_flag
                && (IsSliceHeaderSP(pc_slice_header) || IsSliceHeaderSI(pc_slice_header)); // TODO: Must be MbQ/P slices. Anyways SI/SP not supported for baseline
            if (!cond) {
                cond =
                    (pc_slice_header->MbaffFrameFlag || pc_slice_header->field_pic_flag)
                    && verticalEdgeFlag
                    && (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbQ) || HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbP));
                if (!cond) {
                    cond =
                        (pc_slice_header->MbaffFrameFlag || pc_slice_header->field_pic_flag)
                        && verticalEdgeFlag
                        && (IsSliceHeaderSP(pc_slice_header) || IsSliceHeaderSI(pc_slice_header)); // TODO: Must be MbQ/P slices. Anyways SI/SP not supported for baseline
                }
            }
        }
        if (cond) {
            *bS = 4;
        }
    }
    /*====== COND1 ======*/
    if (!cond) {
        cond =
            (!mixedModeEdgeFlag && (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbQ) || HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbP)))
            || (!mixedModeEdgeFlag && (IsSliceHeaderSP(pc_slice_header) || IsSliceHeaderSI(pc_slice_header)))  // TODO: Must be MbQ/P slices. Anyways SI/SP not supported for baseline
            || (mixedModeEdgeFlag && !verticalEdgeFlag && (HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbQ) || HL_CODEC_264_MB_TYPE_IS_INTRA(pc_mbP)))
            || (mixedModeEdgeFlag && !verticalEdgeFlag && (IsSliceHeaderSP(pc_slice_header) || IsSliceHeaderSI(pc_slice_header))); // TODO: Must be MbQ/P slices. Anyways SI/SP not supported for baseline

        if (cond) {
            *bS = 3;
        }
    }
    /*====== COND2 ======*/
    if (!cond) {
        // TODO: First condition skipped because "transform_size_8x8_flag=1" not supported yet
        cond =
            (!pc_mbP->transform_size_8x8_flag && (pc_mbP->CodedBlockPatternLuma4x4 & (1 << pLuma4x4BlkIdx)))
            || (!pc_mbQ->transform_size_8x8_flag && (pc_mbQ->CodedBlockPatternLuma4x4 & (1 << qLuma4x4BlkIdx)));
        if (cond) {
            *bS = 2;
        }
    }
    /*====== COND3 ======*/
    if (!cond) {
        if (mixedModeEdgeFlag) {
            *bS = 1;
        }
        else {
            int32_t mbPartIdxP, mbPartIdxQ, subMbPartIdxP, subMbPartIdxQ, numVectsP, numVectsQ;
#if 0
            int32_t xP, yP, xQ, yQ;
            xP = Raster4x4LumaBlockScanOrderXY[pLuma4x4BlkIdx][0];
            yP = Raster4x4LumaBlockScanOrderXY[pLuma4x4BlkIdx][1];
            xQ = Raster4x4LumaBlockScanOrderXY[qLuma4x4BlkIdx][0];
            yQ = Raster4x4LumaBlockScanOrderXY[qLuma4x4BlkIdx][1];
#endif

            hl_codec_264_mb_get_sub_partition_indices(pc_mbP, mbPx, mbPy, &mbPartIdxP, &subMbPartIdxP);
            hl_codec_264_mb_get_sub_partition_indices(pc_mbQ, mbQx, mbQy, &mbPartIdxQ, &subMbPartIdxQ);

            numVectsP = pc_mbP->predFlagL0[mbPartIdxP] + pc_mbP->predFlagL1[mbPartIdxP];
            numVectsQ = pc_mbQ->predFlagL0[mbPartIdxQ] + pc_mbQ->predFlagL1[mbPartIdxQ];

            cond =
                (pc_mbP->ref_idx_l0[mbPartIdxP] != pc_mbQ->ref_idx_l0[mbPartIdxQ])
                || (numVectsP != numVectsQ);

            if (!cond && (numVectsP > 0 && numVectsQ > 0)) {
                int32_t tmp = pc_mbP->mvL0[mbPartIdxP][subMbPartIdxP].x - pc_mbQ->mvL0[mbPartIdxQ][subMbPartIdxQ].x;
                if (tmp < 0) {
                    tmp = -tmp;    // abs(tmp)
                }
                if (!(cond = (tmp >= 4))) {
                    tmp = pc_mbP->mvL0[mbPartIdxP][subMbPartIdxP].y - pc_mbQ->mvL0[mbPartIdxQ][subMbPartIdxQ].y; // TODO: use predFlagX to know which mv to check
                    if (tmp < 0) {
                        tmp = -tmp;
                    }

                    if (!(cond = (tmp >= 4))) {
                        // TODO: "mixedModeEdgeFlag is equal to 0 and two motion vectors and two different reference pictures are used to..."
                        // Never true because prediction usin L1 not supported for baseline
                        // "numVectsP/Q" must be equal to #2

                        // TODO: "mixedModeEdgeFlag is equal to 0 and two motion vectors for the same reference picture are used to predict..."
                        // Never true because prediction usin L1 not supported for baseline
                        // "numVectsP/Q" must be equal to #2
                    }
                }
            }

            if (cond) {
                *bS = 1;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

// 8.7.2.2 Derivation process for the thresholds for each block edge
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_get_threshold(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t p0, int32_t q0, int32_t p1, int32_t q1,
    int32_t chromaEdgeFlag,
    int32_t bS,
    int32_t filterOffsetA, int32_t filterOffsetB,
    int32_t qPp, int32_t qPq,
    int32_t* filterSamplesFlag,
    int32_t* indexA,
    int32_t* alpha, int32_t* beta)
{
    int32_t qPav,_indexA,_indexB, _alpha, _beta;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    qPav = (qPp + qPq + 1) >> 1;// (8-461)
    _indexA = HL_MATH_CLIP3(0, 51, qPav + filterOffsetA);// (8-462)
    _indexB = HL_MATH_CLIP3(0, 51, qPav + filterOffsetB);// (8-463)

    _alpha = HL_CODEC_264_DEBLOCK_ALPHA_TABLE[_indexA];
    _beta = HL_CODEC_264_DEBLOCK_BETA_TABLE[_indexB];

    if (chromaEdgeFlag == 0) {
        *alpha = _alpha * (1 << (pc_sps->BitDepthY - 8));// (8-464)
        *beta = _beta * (1 << (pc_sps->BitDepthY - 8));// (8-465)
    }
    else {
        *alpha = _alpha * (1 << (pc_sps->BitDepthC - 8));// (8-466)
        *beta = _beta * (1 << (pc_sps->BitDepthC - 8));// (8-467)
    }

    *indexA = _indexA;
    *filterSamplesFlag = (bS != 0 && HL_MATH_ABS_INT32(p0 - q0) < *alpha && HL_MATH_ABS_INT32(p1 - p0) < *beta && HL_MATH_ABS_INT32(q1 - q0) < *beta );// (8-468)

    return HL_ERROR_SUCCESS;
}

// 8.7.2.3 Filtering process for edges with bS less than 4
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_filter_bs_lt4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const int32_t* p, const int32_t* q,
    int32_t chromaEdgeFlag, int32_t chromaStyleFilteringFlag,
    int32_t bS, int32_t beta, int32_t indexA,
    int32_t* pf, int32_t* qf)
{

    int32_t _tc0,tc0,tc;
    int32_t ap,aq;
    int32_t delta;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;
    const hl_codec_264_nal_sps_t* pc_sps;

    pc_layer = p_codec->layers.pc_active;
    pc_slice_header = pc_layer->p_list_slices[p_mb->u_slice_idx]->p_header;
    pc_sps = pc_slice_header->pc_pps->pc_sps;

    _tc0 = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS];

    if (chromaEdgeFlag == 0) {
        tc0 = _tc0 * (1 << (pc_sps->BitDepthY - 8));// (8-469)
    }
    else {
        tc0 = _tc0 * (1 << (pc_sps->BitDepthC - 8));// (8-470)
    }

    ap = HL_MATH_ABS_INT32(p[2] - p[0]);// (8-471)
    aq = HL_MATH_ABS_INT32(q[2] - q[0]);// (8-472)

    if (chromaStyleFilteringFlag == 0) {
        tc = tc0 + ((ap < beta) ? 1 : 0) + ((aq < beta) ? 1 : 0);// (8-473)
    }
    else {
        tc = tc0 + 1;
    }

    delta = HL_MATH_CLIP3(-tc, tc, ((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3));// (8-475)

    //(8-476) (8-477)
    if (chromaEdgeFlag == 0) {
        pf[0] = HL_MATH_CLIP1Y((p[0] + delta), pc_sps->BitDepthY);
        qf[0] = HL_MATH_CLIP1Y((q[0] - delta), pc_sps->BitDepthY);
    }
    else {
        pf[0] = HL_MATH_CLIP1C((p[0] + delta), pc_sps->BitDepthC);
        qf[0] = HL_MATH_CLIP1C((q[0] - delta), pc_sps->BitDepthC);
    }

    if (chromaStyleFilteringFlag == 0 && ap < beta) {
        pf[1] = p[1] + HL_MATH_CLIP3(-tc0, tc0, (p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1);// (8-478)
    }
    else {
        pf[1] = p[1];// (8-479)
    }
    if (chromaStyleFilteringFlag == 0 && aq < beta) {
        qf[1] = q[1] + HL_MATH_CLIP3(-tc0, tc0, (q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1);// (8-480)
    }
    else {
        qf[1] = q[1];//(8-481)
    }

    pf[2] = p[2];//(8-482)
    qf[2] = q[2];//(8-483)

    return HL_ERROR_SUCCESS;
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_filter_bs_eq4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const int32_t* p, const int32_t* q,
    int32_t chromaEdgeFlag, int32_t chromaStyleFilteringFlag,
    int32_t alpha, int32_t beta,
    int32_t* pf, int32_t* qf)
{
    int32_t ap, aq;

    ap = HL_MATH_ABS_INT32(p[2] - p[0]);// (8-471)
    aq = HL_MATH_ABS_INT32(q[2] - q[0]);// (8-472)

    if (chromaStyleFilteringFlag == 0 && (ap < beta && HL_MATH_ABS_INT32(p[0] - q[0]) < ((alpha >> 2) + 2))) {
        pf[0] = (p[2] + 2*p[1] + 2*p[0] + 2*q[0] + q[1] + 4) >> 3;// (8-485)
        pf[1] = (p[2] + p[1] + p[0] + q[0] + 2) >> 2;// (8-486)
        pf[2] = (2*p[3] + 3*p[2] + p[1] + p[0] + q[0] + 4) >> 3;// (8-487)
    }
    else {
        pf[0] = (2*p[1] + p[0] + q[1] + 2) >> 2;// (8-488)
        pf[1] = p[1];// (8-489)
        pf[2] = p[2];// (8-490)
    }


    if (chromaStyleFilteringFlag == 0 && (aq < beta && HL_MATH_ABS_INT32(p[0] - q[0]) < ((alpha >> 2) + 2))) {
        qf[0] = (p[1] + 2*p[0] + 2*q[0] + 2*q[1] + q[2] + 4) >> 3;// (8-492)
        qf[1] = (p[0] + q[0] + q[1] + q[2] + 2 ) >> 2;// (8-493)
        qf[2] = (2*q[3] + 3*q[2] + q[1] + q[0] + p[0] + 4) >> 3;// (8-494)
    }
    else {
        qf[0] = (2*q[1] + q[0] + p[1] + 2) >> 2; //(8-495)
        qf[1] = q[1]; //(8-496)
        qf[2] = q[2]; //(8-497)
    }

    return HL_ERROR_SUCCESS;
}

// TODO: add INTRIN and ASM versions
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8_cpp(
    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    int16_t bS[2/*8x8 index*/][2/*4x4 index*/],
    uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    uint32_t u_luma_stride_mul4 = (u_luma_stride << 2);
    if (*((int32_t*)bS[0])) {
        if (bS[0][0]) {
            p[3][0] = pc_luma_samples[0], p[2][0] = pc_luma_samples[1], p[1][0] = pc_luma_samples[2], p[0][0] = pc_luma_samples[3];
            q[0][0] = pc_luma_samples[4], q[1][0] = pc_luma_samples[5], q[2][0] = pc_luma_samples[6], q[3][0] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][1] = pc_luma_samples[0], p[2][1] = pc_luma_samples[1], p[1][1] = pc_luma_samples[2], p[0][1] = pc_luma_samples[3];
            q[0][1] = pc_luma_samples[4], q[1][1] = pc_luma_samples[5], q[2][1] = pc_luma_samples[6], q[3][1] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][2] = pc_luma_samples[0], p[2][2] = pc_luma_samples[1], p[1][2] = pc_luma_samples[2], p[0][2] = pc_luma_samples[3];
            q[0][2] = pc_luma_samples[4], q[1][2] = pc_luma_samples[5], q[2][2] = pc_luma_samples[6], q[3][2] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][3] = pc_luma_samples[0], p[2][3] = pc_luma_samples[1], p[1][3] = pc_luma_samples[2], p[0][3] = pc_luma_samples[3];
            q[0][3] = pc_luma_samples[4], q[1][3] = pc_luma_samples[5], q[2][3] = pc_luma_samples[6], q[3][3] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
        }
        else {
            pc_luma_samples += u_luma_stride_mul4;
        }
        if (bS[0][1]) {
            p[3][4] = pc_luma_samples[0], p[2][4] = pc_luma_samples[1], p[1][4] = pc_luma_samples[2], p[0][4] = pc_luma_samples[3];
            q[0][4] = pc_luma_samples[4], q[1][4] = pc_luma_samples[5], q[2][4] = pc_luma_samples[6], q[3][4] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][5] = pc_luma_samples[0], p[2][5] = pc_luma_samples[1], p[1][5] = pc_luma_samples[2], p[0][5] = pc_luma_samples[3];
            q[0][5] = pc_luma_samples[4], q[1][5] = pc_luma_samples[5], q[2][5] = pc_luma_samples[6], q[3][5] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][6] = pc_luma_samples[0], p[2][6] = pc_luma_samples[1], p[1][6] = pc_luma_samples[2], p[0][6] = pc_luma_samples[3];
            q[0][6] = pc_luma_samples[4], q[1][6] = pc_luma_samples[5], q[2][6] = pc_luma_samples[6], q[3][6] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][7] = pc_luma_samples[0], p[2][7] = pc_luma_samples[1], p[1][7] = pc_luma_samples[2], p[0][7] = pc_luma_samples[3];
            q[0][7] = pc_luma_samples[4], q[1][7] = pc_luma_samples[5], q[2][7] = pc_luma_samples[6], q[3][7] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
        }
        else {
            pc_luma_samples += u_luma_stride_mul4;
        }
    }
    else {
        pc_luma_samples += (u_luma_stride_mul4 << 1);
    }

    if (*((int32_t*)bS[1])) {
        if (bS[1][0]) {
            p[3][8] = pc_luma_samples[0], p[2][8] = pc_luma_samples[1], p[1][8] = pc_luma_samples[2], p[0][8] = pc_luma_samples[3];
            q[0][8] = pc_luma_samples[4], q[1][8] = pc_luma_samples[5], q[2][8] = pc_luma_samples[6], q[3][8] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][9] = pc_luma_samples[0], p[2][9] = pc_luma_samples[1], p[1][9] = pc_luma_samples[2], p[0][9] = pc_luma_samples[3];
            q[0][9] = pc_luma_samples[4], q[1][9] = pc_luma_samples[5], q[2][9] = pc_luma_samples[6], q[3][9] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][10] = pc_luma_samples[0], p[2][10] = pc_luma_samples[1], p[1][10] = pc_luma_samples[2], p[0][10] = pc_luma_samples[3];
            q[0][10] = pc_luma_samples[4], q[1][10] = pc_luma_samples[5], q[2][10] = pc_luma_samples[6], q[3][10] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][11] = pc_luma_samples[0], p[2][11] = pc_luma_samples[1], p[1][11] = pc_luma_samples[2], p[0][11] = pc_luma_samples[3];
            q[0][11] = pc_luma_samples[4], q[1][11] = pc_luma_samples[5], q[2][11] = pc_luma_samples[6], q[3][11] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
        }
        else {
            pc_luma_samples += u_luma_stride_mul4;
        }
        if (bS[1][1]) {
            p[3][12] = pc_luma_samples[0], p[2][12] = pc_luma_samples[1], p[1][12] = pc_luma_samples[2], p[0][12] = pc_luma_samples[3];
            q[0][12] = pc_luma_samples[4], q[1][12] = pc_luma_samples[5], q[2][12] = pc_luma_samples[6], q[3][12] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][13] = pc_luma_samples[0], p[2][13] = pc_luma_samples[1], p[1][13] = pc_luma_samples[2], p[0][13] = pc_luma_samples[3];
            q[0][13] = pc_luma_samples[4], q[1][13] = pc_luma_samples[5], q[2][13] = pc_luma_samples[6], q[3][13] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][14] = pc_luma_samples[0], p[2][14] = pc_luma_samples[1], p[1][14] = pc_luma_samples[2], p[0][14] = pc_luma_samples[3];
            q[0][14] = pc_luma_samples[4], q[1][14] = pc_luma_samples[5], q[2][14] = pc_luma_samples[6], q[3][14] = pc_luma_samples[7];
            pc_luma_samples += u_luma_stride;
            p[3][15] = pc_luma_samples[0], p[2][15] = pc_luma_samples[1], p[1][15] = pc_luma_samples[2], p[0][15] = pc_luma_samples[3];
            q[0][15] = pc_luma_samples[4], q[1][15] = pc_luma_samples[5], q[2][15] = pc_luma_samples[6], q[3][15] = pc_luma_samples[7];
        }
    }
}

// TODO: add INTRIN and ASM versions
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8_cpp(
    uint8_t *p_luma_samples, uint32_t u_luma_stride,
    int16_t filterSamplesFlag[16],
    const uint8_t pf[3/*p0,p1,p2,p3*/][16], uint8_t const qf[3/*q0,q1,q2,q3*/][16])
{
    uint32_t u_luma_stride_mul4 = (u_luma_stride << 2);

    // "*((uint64_t*)&filterSamplesFlag[X])" is used to skip 4x4 blocks. If line #N in a 4x4 block is skipped then the probablity to have the same for the other #3 lines is very high.

    if (*((uint64_t*)&filterSamplesFlag[0])) {
        if (filterSamplesFlag[0]) {
            /*p_luma_samples[0] = pf[3][0],*/ p_luma_samples[1] = pf[2][0], p_luma_samples[2] = pf[1][0], p_luma_samples[3] = pf[0][0];
            p_luma_samples[4] = qf[0][0], p_luma_samples[5] = qf[1][0], p_luma_samples[6] = qf[2][0]/*, p_luma_samples[7] = qf[3][0]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[1]) {
            /*p_luma_samples[0] = pf[3][1],*/ p_luma_samples[1] = pf[2][1], p_luma_samples[2] = pf[1][1], p_luma_samples[3] = pf[0][1];
            p_luma_samples[4] = qf[0][1], p_luma_samples[5] = qf[1][1], p_luma_samples[6] = qf[2][1] /*,p_luma_samples[7] = qf[3][1]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[2]) {
            /*p_luma_samples[0] = pf[3][2],*/ p_luma_samples[1] = pf[2][2], p_luma_samples[2] = pf[1][2], p_luma_samples[3] = pf[0][2];
            p_luma_samples[4] = qf[0][2], p_luma_samples[5] = qf[1][2], p_luma_samples[6] = qf[2][2] /*,p_luma_samples[7] = qf[3][2]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[3]) {
            /*p_luma_samples[0] = pf[3][3],*/ p_luma_samples[1] = pf[2][3], p_luma_samples[2] = pf[1][3], p_luma_samples[3] = pf[0][3];
            p_luma_samples[4] = qf[0][3], p_luma_samples[5] = qf[1][3], p_luma_samples[6] = qf[2][3] /*,p_luma_samples[7] = qf[3][3]*/;
        }
        p_luma_samples += u_luma_stride;
    }
    else {
        p_luma_samples += u_luma_stride_mul4;
    }

    if (*((uint64_t*)&filterSamplesFlag[4])) {
        if (filterSamplesFlag[4]) {
            /*p_luma_samples[0] = pf[3][4],*/ p_luma_samples[1] = pf[2][4], p_luma_samples[2] = pf[1][4], p_luma_samples[3] = pf[0][4];
            p_luma_samples[4] = qf[0][4], p_luma_samples[5] = qf[1][4], p_luma_samples[6] = qf[2][4] /*,p_luma_samples[7] = qf[3][4]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[5]) {
            /*p_luma_samples[0] = pf[3][5],*/ p_luma_samples[1] = pf[2][5], p_luma_samples[2] = pf[1][5], p_luma_samples[3] = pf[0][5];
            p_luma_samples[4] = qf[0][5], p_luma_samples[5] = qf[1][5], p_luma_samples[6] = qf[2][5] /*,p_luma_samples[7] = qf[3][5]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[6]) {
            /*p_luma_samples[0] = pf[3][6],*/ p_luma_samples[1] = pf[2][6], p_luma_samples[2] = pf[1][6], p_luma_samples[3] = pf[0][6];
            p_luma_samples[4] = qf[0][6], p_luma_samples[5] = qf[1][6], p_luma_samples[6] = qf[2][6] /*,p_luma_samples[7] = qf[3][6]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[7]) {
            /*p_luma_samples[0] = pf[3][7],*/ p_luma_samples[1] = pf[2][7], p_luma_samples[2] = pf[1][7], p_luma_samples[3] = pf[0][7];
            p_luma_samples[4] = qf[0][7], p_luma_samples[5] = qf[1][7], p_luma_samples[6] = qf[2][7] /*,p_luma_samples[7] = qf[3][7]*/;
        }
        p_luma_samples += u_luma_stride;
    }
    else {
        p_luma_samples += u_luma_stride_mul4;
    }

    if (*((uint64_t*)&filterSamplesFlag[8])) {
        if (filterSamplesFlag[8]) {
            /*p_luma_samples[0] = pf[3][8],*/ p_luma_samples[1] = pf[2][8], p_luma_samples[2] = pf[1][8], p_luma_samples[3] = pf[0][8];
            p_luma_samples[4] = qf[0][8], p_luma_samples[5] = qf[1][8], p_luma_samples[6] = qf[2][8] /*,p_luma_samples[7] = qf[3][8]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[9]) {
            /*p_luma_samples[0] = pf[3][9],*/ p_luma_samples[1] = pf[2][9], p_luma_samples[2] = pf[1][9], p_luma_samples[3] = pf[0][9];
            p_luma_samples[4] = qf[0][9], p_luma_samples[5] = qf[1][9], p_luma_samples[6] = qf[2][9] /*,p_luma_samples[7] = qf[3][9]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[10]) {
            /*p_luma_samples[0] = pf[3][10],*/ p_luma_samples[1] = pf[2][10], p_luma_samples[2] = pf[1][10], p_luma_samples[3] = pf[0][10];
            p_luma_samples[4] = qf[0][10], p_luma_samples[5] = qf[1][10], p_luma_samples[6] = qf[2][10] /*,p_luma_samples[7] = qf[3][10]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[11]) {
            /*p_luma_samples[0] = pf[3][11],*/ p_luma_samples[1] = pf[2][11], p_luma_samples[2] = pf[1][11], p_luma_samples[3] = pf[0][11];
            p_luma_samples[4] = qf[0][11], p_luma_samples[5] = qf[1][11], p_luma_samples[6] = qf[2][11] /*,p_luma_samples[7] = qf[3][11]*/;
        }
        p_luma_samples += u_luma_stride;
    }
    else {
        p_luma_samples += u_luma_stride_mul4;
    }

    if (*((uint64_t*)&filterSamplesFlag[12])) {
        if (filterSamplesFlag[12]) {
            /*p_luma_samples[0] = pf[3][12],*/ p_luma_samples[1] = pf[2][12], p_luma_samples[2] = pf[1][12], p_luma_samples[3] = pf[0][12];
            p_luma_samples[4] = qf[0][12], p_luma_samples[5] = qf[1][12], p_luma_samples[6] = qf[2][12] /*,p_luma_samples[7] = qf[3][12]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[13]) {
            /*p_luma_samples[0] = pf[3][13],*/ p_luma_samples[1] = pf[2][13], p_luma_samples[2] = pf[1][13], p_luma_samples[3] = pf[0][13];
            p_luma_samples[4] = qf[0][13], p_luma_samples[5] = qf[1][13], p_luma_samples[6] = qf[2][13] /*,p_luma_samples[7] = qf[3][13]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[14]) {
            /*p_luma_samples[0] = pf[3][14],*/ p_luma_samples[1] = pf[2][14], p_luma_samples[2] = pf[1][14], p_luma_samples[3] = pf[0][14];
            p_luma_samples[4] = qf[0][14], p_luma_samples[5] = qf[1][14], p_luma_samples[6] = qf[2][14] /*,p_luma_samples[7] = qf[3][14]*/;
        }
        p_luma_samples += u_luma_stride;
        if (filterSamplesFlag[15]) {
            /*p_luma_samples[0] = pf[3][15],*/ p_luma_samples[1] = pf[2][15], p_luma_samples[2] = pf[1][15], p_luma_samples[3] = pf[0][15];
            p_luma_samples[4] = qf[0][15], p_luma_samples[5] = qf[1][15], p_luma_samples[6] = qf[2][15] /*,p_luma_samples[7] = qf[3][15]*/;
        }
    }
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8_cpp(
    const uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16])
{
#define MOVE128(dst, src) \
	*((uint64_t*)&(dst)[0])=*((uint64_t*)&(src)[0]), \
	*((uint64_t*)&(dst)[8])=*((uint64_t*)&(src)[8])

    MOVE128(p[3], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(p[2], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(p[1], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(p[0], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(q[0], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(q[1], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(q[2], pc_luma_samples);
    pc_luma_samples += u_luma_stride;
    MOVE128(q[3], pc_luma_samples);
#undef MOVE128
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_cpp(
    uint8_t *pc_luma_samples, uint32_t u_luma_stride,
    const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*qf0,qf1,qf2*/][16])
{
#define MOVE128(dst, src) \
	*((uint64_t*)&(dst)[0])=*((uint64_t*)&(src)[0]), \
	*((uint64_t*)&(dst)[8])=*((uint64_t*)&(src)[8])

    pc_luma_samples += u_luma_stride; // skip pf/qf[3] and take p/qf[0,2,2]
    MOVE128(pc_luma_samples, pf[2]);
    pc_luma_samples += u_luma_stride;
    MOVE128(pc_luma_samples, pf[1]);
    pc_luma_samples += u_luma_stride;
    MOVE128(pc_luma_samples, pf[0]);
    pc_luma_samples += u_luma_stride;
    MOVE128(pc_luma_samples, qf[0]);
    pc_luma_samples += u_luma_stride;
    MOVE128(pc_luma_samples, qf[1]);
    pc_luma_samples += u_luma_stride;
    MOVE128(pc_luma_samples, qf[2]);
#undef MOVE128
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_cpp(
    const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16])
{
#define MOVE64(dst, src) \
	*((uint64_t*)&(dst)[0])=*((uint64_t*)&(src)[0])

    MOVE64(&p[3][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&p[2][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&p[1][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&p[0][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&q[0][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&q[1][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&q[2][0], pc_cb_samples);
    pc_cb_samples += u_chroma_stride;
    MOVE64(&q[3][0], pc_cb_samples);

    MOVE64(&p[3][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&p[2][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&p[1][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&p[0][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&q[0][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&q[1][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&q[2][8], pc_cr_samples);
    pc_cr_samples += u_chroma_stride;
    MOVE64(&q[3][8], pc_cr_samples);

#undef MOVE64
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_cpp(
    uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*qf0,qf1,qf2*/][16])
{
#define MOVE64(dst, src) \
	*((uint64_t*)&(dst)[0])=*((uint64_t*)&(src)[0])

    pc_cb_samples += u_chroma_stride; // skip p/q[3]
    MOVE64(pc_cb_samples, &pf[2][0]);
    pc_cb_samples += u_chroma_stride;
    MOVE64(pc_cb_samples, &pf[1][0]);
    pc_cb_samples += u_chroma_stride;
    MOVE64(pc_cb_samples, &pf[0][0]);
    pc_cb_samples += u_chroma_stride;
    MOVE64(pc_cb_samples, &qf[0][0]);
    pc_cb_samples += u_chroma_stride;
    MOVE64(pc_cb_samples, &qf[1][0]);
    pc_cb_samples += u_chroma_stride;
    MOVE64(pc_cb_samples, &qf[2][0]);

    pc_cr_samples += u_chroma_stride; // skip p/q[3]
    MOVE64(pc_cr_samples, &pf[2][8]);
    pc_cr_samples += u_chroma_stride;
    MOVE64(pc_cr_samples, &pf[1][8]);
    pc_cr_samples += u_chroma_stride;
    MOVE64(pc_cr_samples, &pf[0][8]);
    pc_cr_samples += u_chroma_stride;
    MOVE64(pc_cr_samples, &qf[0][8]);
    pc_cr_samples += u_chroma_stride;
    MOVE64(pc_cr_samples, &qf[1][8]);
    pc_cr_samples += u_chroma_stride;
    MOVE64(pc_cr_samples, &qf[2][8]);

#undef MOVE64
}

// TODO: add INTRIN and ASM versions
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8_cpp(
    const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    int16_t bS[4/*2x2 index*/],
    uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16])
{
    if (*((int32_t*)&bS[0])) {
        p[3][0] = pc_cb_samples[0], p[2][0] = pc_cb_samples[1], p[1][0] = pc_cb_samples[2], p[0][0] = pc_cb_samples[3];
        q[0][0] = pc_cb_samples[4], q[1][0] = pc_cb_samples[5], q[2][0] = pc_cb_samples[6], q[3][0] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;
        p[3][1] = pc_cb_samples[0], p[2][1] = pc_cb_samples[1], p[1][1] = pc_cb_samples[2], p[0][1] = pc_cb_samples[3];
        q[0][1] = pc_cb_samples[4], q[1][1] = pc_cb_samples[5], q[2][1] = pc_cb_samples[6], q[3][1] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;
        p[3][2] = pc_cb_samples[0], p[2][2] = pc_cb_samples[1], p[1][2] = pc_cb_samples[2], p[0][2] = pc_cb_samples[3];
        q[0][2] = pc_cb_samples[4], q[1][2] = pc_cb_samples[5], q[2][2] = pc_cb_samples[6], q[3][2] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;
        p[3][3] = pc_cb_samples[0], p[2][3] = pc_cb_samples[1], p[1][3] = pc_cb_samples[2], p[0][3] = pc_cb_samples[3];
        q[0][3] = pc_cb_samples[4], q[1][3] = pc_cb_samples[5], q[2][3] = pc_cb_samples[6], q[3][3] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;

        p[3][8] = pc_cr_samples[0], p[2][8] = pc_cr_samples[1], p[1][8] = pc_cr_samples[2], p[0][8] = pc_cr_samples[3];
        q[0][8] = pc_cr_samples[4], q[1][8] = pc_cr_samples[5], q[2][8] = pc_cr_samples[6], q[3][8] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
        p[3][9] = pc_cr_samples[0], p[2][9] = pc_cr_samples[1], p[1][9] = pc_cr_samples[2], p[0][9] = pc_cr_samples[3];
        q[0][9] = pc_cr_samples[4], q[1][9] = pc_cr_samples[5], q[2][9] = pc_cr_samples[6], q[3][9] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
        p[3][10] = pc_cr_samples[0], p[2][10] = pc_cr_samples[1], p[1][10] = pc_cr_samples[2], p[0][10] = pc_cr_samples[3];
        q[0][10] = pc_cr_samples[4], q[1][10] = pc_cr_samples[5], q[2][10] = pc_cr_samples[6], q[3][10] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
        p[3][11] = pc_cr_samples[0], p[2][11] = pc_cr_samples[1], p[1][11] = pc_cr_samples[2], p[0][11] = pc_cr_samples[3];
        q[0][11] = pc_cr_samples[4], q[1][11] = pc_cr_samples[5], q[2][11] = pc_cr_samples[6], q[3][11] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
    }
    else {
        pc_cb_samples += (u_chroma_stride << 2);
        pc_cr_samples += (u_chroma_stride << 2);
    }

    if (*((int32_t*)&bS[2])) {
        p[3][4] = pc_cb_samples[0], p[2][4] = pc_cb_samples[1], p[1][4] = pc_cb_samples[2], p[0][4] = pc_cb_samples[3];
        q[0][4] = pc_cb_samples[4], q[1][4] = pc_cb_samples[5], q[2][4] = pc_cb_samples[6], q[3][4] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;
        p[3][5] = pc_cb_samples[0], p[2][5] = pc_cb_samples[1], p[1][5] = pc_cb_samples[2], p[0][5] = pc_cb_samples[3];
        q[0][5] = pc_cb_samples[4], q[1][5] = pc_cb_samples[5], q[2][5] = pc_cb_samples[6], q[3][5] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;
        p[3][6] = pc_cb_samples[0], p[2][6] = pc_cb_samples[1], p[1][6] = pc_cb_samples[2], p[0][6] = pc_cb_samples[3];
        q[0][6] = pc_cb_samples[4], q[1][6] = pc_cb_samples[5], q[2][6] = pc_cb_samples[6], q[3][6] = pc_cb_samples[7];
        pc_cb_samples += u_chroma_stride;
        p[3][7] = pc_cb_samples[0], p[2][7] = pc_cb_samples[1], p[1][7] = pc_cb_samples[2], p[0][7] = pc_cb_samples[3];
        q[0][7] = pc_cb_samples[4], q[1][7] = pc_cb_samples[5], q[2][7] = pc_cb_samples[6], q[3][7] = pc_cb_samples[7];

        p[3][12] = pc_cr_samples[0], p[2][12] = pc_cr_samples[1], p[1][12] = pc_cr_samples[2], p[0][12] = pc_cr_samples[3];
        q[0][12] = pc_cr_samples[4], q[1][12] = pc_cr_samples[5], q[2][12] = pc_cr_samples[6], q[3][12] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
        p[3][13] = pc_cr_samples[0], p[2][13] = pc_cr_samples[1], p[1][13] = pc_cr_samples[2], p[0][13] = pc_cr_samples[3];
        q[0][13] = pc_cr_samples[4], q[1][13] = pc_cr_samples[5], q[2][13] = pc_cr_samples[6], q[3][13] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
        p[3][14] = pc_cr_samples[0], p[2][14] = pc_cr_samples[1], p[1][14] = pc_cr_samples[2], p[0][14] = pc_cr_samples[3];
        q[0][14] = pc_cr_samples[4], q[1][14] = pc_cr_samples[5], q[2][14] = pc_cr_samples[6], q[3][14] = pc_cr_samples[7];
        pc_cr_samples += u_chroma_stride;
        p[3][15] = pc_cr_samples[0], p[2][15] = pc_cr_samples[1], p[1][15] = pc_cr_samples[2], p[0][15] = pc_cr_samples[3];
        q[0][15] = pc_cr_samples[4], q[1][15] = pc_cr_samples[5], q[2][15] = pc_cr_samples[6], q[3][15] = pc_cr_samples[7];
    }
}

// TODO: add INTRIN and ASM versions
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8_cpp(
    uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride,
    int16_t filterSamplesFlag[16],
    const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*q0,q1,q2,q3*/][16])
{
    const uint32_t u_chroma_stride_mul4 = (u_chroma_stride << 2);

    // "*((uint64_t*)&filterSamplesFlag[X])" is used to skip 4x4 blocks. If line #N in a 4x4 block is skipped then the probablity to have the same for the other #3 lines is very high.

    if (*((uint64_t*)&filterSamplesFlag[0])) {
        /*pc_cb_samples[0] = pf[3][0],*/ pc_cb_samples[1] = pf[2][0], pc_cb_samples[2] = pf[1][0], pc_cb_samples[3] = pf[0][0];
        pc_cb_samples[4] = qf[0][0], pc_cb_samples[5] = qf[1][0], pc_cb_samples[6] = qf[2][0]/*, pc_cb_samples[7] = qf[3][0]*/;
        pc_cb_samples += u_chroma_stride;
        /* pc_cb_samples[0] = pf[3][1],*/ pc_cb_samples[1] = pf[2][1], pc_cb_samples[2] = pf[1][1], pc_cb_samples[3] = pf[0][1];
        pc_cb_samples[4] = qf[0][1], pc_cb_samples[5] = qf[1][1], pc_cb_samples[6] = qf[2][1]/*, pc_cb_samples[7] = qf[3][1]*/;
        pc_cb_samples += u_chroma_stride;
        /*pc_cb_samples[0] = pf[3][2],*/ pc_cb_samples[1] = pf[2][2], pc_cb_samples[2] = pf[1][2], pc_cb_samples[3] = pf[0][2];
        pc_cb_samples[4] = qf[0][2], pc_cb_samples[5] = qf[1][2], pc_cb_samples[6] = qf[2][2]/*, pc_cb_samples[7] = qf[3][2]*/;
        pc_cb_samples += u_chroma_stride;
        /*pc_cb_samples[0] = pf[3][3],*/ pc_cb_samples[1] = pf[2][3], pc_cb_samples[2] = pf[1][3], pc_cb_samples[3] = pf[0][3];
        pc_cb_samples[4] = qf[0][3], pc_cb_samples[5] = qf[1][3], pc_cb_samples[6] = qf[2][3]/*, pc_cb_samples[7] = qf[3][3]*/;
        pc_cb_samples += u_chroma_stride;
    }
    else {
        pc_cb_samples += u_chroma_stride_mul4;
    }
    if (*((uint64_t*)&filterSamplesFlag[4])) {
        /*pc_cb_samples[0] = pf[3][4],*/ pc_cb_samples[1] = pf[2][4], pc_cb_samples[2] = pf[1][4], pc_cb_samples[3] = pf[0][4];
        pc_cb_samples[4] = qf[0][4], pc_cb_samples[5] = qf[1][4], pc_cb_samples[6] = qf[2][4]/*, pc_cb_samples[7] = qf[3][4]*/;
        pc_cb_samples += u_chroma_stride;
        /*pc_cb_samples[0] = pf[3][5],*/ pc_cb_samples[1] = pf[2][5], pc_cb_samples[2] = pf[1][5], pc_cb_samples[3] = pf[0][5];
        pc_cb_samples[4] = qf[0][5], pc_cb_samples[5] = qf[1][5], pc_cb_samples[6] = qf[2][5]/*, pc_cb_samples[7] = qf[3][5]*/;
        pc_cb_samples += u_chroma_stride;
        /*pc_cb_samples[0] = pf[3][6],*/ pc_cb_samples[1] = pf[2][6], pc_cb_samples[2] = pf[1][6], pc_cb_samples[3] = pf[0][6];
        pc_cb_samples[4] = qf[0][6], pc_cb_samples[5] = qf[1][6], pc_cb_samples[6] = qf[2][6]/*, pc_cb_samples[7] = qf[3][6]*/;
        pc_cb_samples += u_chroma_stride;
        /*pc_cb_samples[0] = pf[3][7],*/ pc_cb_samples[1] = pf[2][7], pc_cb_samples[2] = pf[1][7], pc_cb_samples[3] = pf[0][7];
        pc_cb_samples[4] = qf[0][7], pc_cb_samples[5] = qf[1][7], pc_cb_samples[6] = qf[2][7]/*, pc_cb_samples[7] = qf[3][7]*/;
    }

    if (*((uint64_t*)&filterSamplesFlag[8])) {
        /*pc_cr_samples[0] = pf[3][8],*/ pc_cr_samples[1] = pf[2][8], pc_cr_samples[2] = pf[1][8], pc_cr_samples[3] = pf[0][8];
        pc_cr_samples[4] = qf[0][8], pc_cr_samples[5] = qf[1][8], pc_cr_samples[6] = qf[2][8]/*, pc_cr_samples[7] = qf[3][8]*/;
        pc_cr_samples += u_chroma_stride;
        /*pc_cr_samples[0] = pf[3][9],*/ pc_cr_samples[1] = pf[2][9], pc_cr_samples[2] = pf[1][9], pc_cr_samples[3] = pf[0][9];
        pc_cr_samples[4] = qf[0][9], pc_cr_samples[5] = qf[1][9], pc_cr_samples[6] = qf[2][9]/*, pc_cr_samples[7] = qf[3][9]*/;
        pc_cr_samples += u_chroma_stride;
        /*pc_cr_samples[0] = pf[3][10],*/ pc_cr_samples[1] = pf[2][10], pc_cr_samples[2] = pf[1][10], pc_cr_samples[3] = pf[0][10];
        pc_cr_samples[4] = qf[0][10], pc_cr_samples[5] = qf[1][10], pc_cr_samples[6] = qf[2][10]/*, pc_cr_samples[7] = qf[3][10]*/;
        pc_cr_samples += u_chroma_stride;
        /*pc_cr_samples[0] = pf[3][11],*/ pc_cr_samples[1] = pf[2][11], pc_cr_samples[2] = pf[1][11], pc_cr_samples[3] = pf[0][11];
        pc_cr_samples[4] = qf[0][11], pc_cr_samples[5] = qf[1][11], pc_cr_samples[6] = qf[2][11]/*, pc_cr_samples[7] = qf[3][11]*/;
        pc_cr_samples += u_chroma_stride;
    }
    else {
        pc_cr_samples += u_chroma_stride_mul4;
    }
    if (*((uint64_t*)&filterSamplesFlag[12])) {
        /*pc_cr_samples[0] = pf[3][12],*/ pc_cr_samples[1] = pf[2][12], pc_cr_samples[2] = pf[1][12], pc_cr_samples[3] = pf[0][12];
        pc_cr_samples[4] = qf[0][12], pc_cr_samples[5] = qf[1][12], pc_cr_samples[6] = qf[2][12]/*, pc_cr_samples[7] = qf[3][12]*/;
        pc_cr_samples += u_chroma_stride;
        /*pc_cr_samples[0] = pf[3][13],*/ pc_cr_samples[1] = pf[2][13], pc_cr_samples[2] = pf[1][13], pc_cr_samples[3] = pf[0][13];
        pc_cr_samples[4] = qf[0][13], pc_cr_samples[5] = qf[1][13], pc_cr_samples[6] = qf[2][13]/*, pc_cr_samples[7] = qf[3][13]*/;
        pc_cr_samples += u_chroma_stride;
        /*pc_cr_samples[0] = pf[3][14],*/ pc_cr_samples[1] = pf[2][14], pc_cr_samples[2] = pf[1][14], pc_cr_samples[3] = pf[0][14];
        pc_cr_samples[4] = qf[0][14], pc_cr_samples[5] = qf[1][14], pc_cr_samples[6] = qf[2][14]/*, pc_cr_samples[7] = qf[3][14]*/;
        pc_cr_samples += u_chroma_stride;
        /*pc_cr_samples[0] = pf[3][15],*/ pc_cr_samples[1] = pf[2][15], pc_cr_samples[2] = pf[1][15], pc_cr_samples[3] = pf[0][15];
        pc_cr_samples[4] = qf[0][15], pc_cr_samples[5] = qf[1][15], pc_cr_samples[6] = qf[2][15]/*, pc_cr_samples[7] = qf[3][15]*/;
    }
}

// 8.7.2.1 Derivation process for the luma content dependent boundary filtering strength
#define hl_codec_264_deblock_avc_baseline_get_bs_luma4lines( \
    /*hl_codec_264_t**/ p_codec,  \
    /*const hl_codec_264_mb_t**/ pc_mbP, /*const hl_codec_264_mb_t**/ pc_mbQ, \
    /*int32_t*/ mbPx, /*int32_t*/ mbPy, /*int32_t*/ mbQx, /*int32_t*/ mbQy, \
    /*hl_bool_t*/ b_is_mb_edge, \
    /*int16_t **/bS) \
{ \
    if (HL_CODEC_264_MB_TYPE_IS_INTRA((pc_mbQ)) || HL_CODEC_264_MB_TYPE_IS_INTRA((pc_mbP))) { \
        *(bS) = b_is_mb_edge ? 4 : 3; \
    } \
    else { \
        int32_t qLuma4x4BlkIdx, pLuma4x4BlkIdx; \
		 \
        pLuma4x4BlkIdx = LumaBlockIndices4x4_YX[(mbPy)][(mbPx)]; \
        qLuma4x4BlkIdx = LumaBlockIndices4x4_YX[(mbQy)][(mbQx)]; \
		 \
        if (((pc_mbP)->CodedBlockPatternLuma4x4 & (1 << pLuma4x4BlkIdx)) || ((pc_mbQ)->CodedBlockPatternLuma4x4 & (1 << qLuma4x4BlkIdx))) { \
            *(bS) = 2; \
        } \
        else { \
            int32_t mbPartIdxP, mbPartIdxQ, subMbPartIdxP, subMbPartIdxQ; \
			 \
            hl_codec_264_mb_get_sub_partition_indices((pc_mbP), (mbPx), (mbPy), &mbPartIdxP, &subMbPartIdxP); \
            hl_codec_264_mb_get_sub_partition_indices((pc_mbQ), (mbQx), (mbQy), &mbPartIdxQ, &subMbPartIdxQ); \
			 \
            if ((((pc_mbP)->ref_idx_l0[mbPartIdxP] != (pc_mbQ)->ref_idx_l0[mbPartIdxQ]) || ((pc_mbP)->predFlagL0[mbPartIdxP] != (pc_mbQ)->predFlagL0[mbPartIdxQ]))) { \
                *(bS) = 1; \
            } \
            else if (pc_mbP->predFlagL0[mbPartIdxP] && pc_mbQ->predFlagL0[mbPartIdxQ]) { \
                int32_t tmp = (pc_mbP)->mvL0[mbPartIdxP][subMbPartIdxP].x - (pc_mbQ)->mvL0[mbPartIdxQ][subMbPartIdxQ].x; \
                tmp = HL_MATH_ABS_INT32(tmp); \
                if (tmp < 4) { \
                    tmp = (pc_mbP)->mvL0[mbPartIdxP][subMbPartIdxP].y - (pc_mbQ)->mvL0[mbPartIdxQ][subMbPartIdxQ].y; \
                    tmp = HL_MATH_ABS_INT32(tmp); \
                    if ((tmp >= 4)) { \
                        *(bS) = 1; \
                    } \
					else { \
						*(bS) = 0; \
					} \
                } \
                else { \
                    *(bS) = 1; \
                } \
            } \
			else { \
				*(bS) = 0; \
			} \
        } \
    } \
}

#define hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8( \
	qPp, qPq, filterOffsetA, filterOffsetB, indexA, alpha, beta) \
	{  \
		int16_t qPav = ((qPp) + (qPq) + 1) >> 1; /* (8-461) */ \
		int16_t indexB = HL_MATH_CLIP3(0, 51, qPav + (filterOffsetB)); /* (8-463) */ \
		*(indexA) = HL_MATH_CLIP3(0, 51, qPav + (filterOffsetA)); /* (8-462) */ \
		*(alpha) = HL_CODEC_264_DEBLOCK_ALPHA_TABLE[*(indexA)]; \
		*(beta) = HL_CODEC_264_DEBLOCK_BETA_TABLE[indexB]; \
	}

// 8.7.2.2 Derivation process for the thresholds for each block edge
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8_cpp(
    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
    int16_t bS[2],
    int16_t alpha, int16_t beta,
    HL_OUT int16_t filterSamplesFlag[8])
{
    // (8-468)
    filterSamplesFlag[0] = (bS[0] != 0 && HL_MATH_ABS_INT32(p0[0] - q0[0]) < alpha && HL_MATH_ABS_INT32(p1[0] - p0[0]) < beta && HL_MATH_ABS_INT32(q1[0] - q0[0]) < beta);
    filterSamplesFlag[1] = (bS[0] != 0 && HL_MATH_ABS_INT32(p0[1] - q0[1]) < alpha && HL_MATH_ABS_INT32(p1[1] - p0[1]) < beta && HL_MATH_ABS_INT32(q1[1] - q0[1]) < beta);
    filterSamplesFlag[2] = (bS[0] != 0 && HL_MATH_ABS_INT32(p0[2] - q0[2]) < alpha && HL_MATH_ABS_INT32(p1[2] - p0[2]) < beta && HL_MATH_ABS_INT32(q1[2] - q0[2]) < beta);
    filterSamplesFlag[3] = (bS[0] != 0 && HL_MATH_ABS_INT32(p0[3] - q0[3]) < alpha && HL_MATH_ABS_INT32(p1[3] - p0[3]) < beta && HL_MATH_ABS_INT32(q1[3] - q0[3]) < beta);
    filterSamplesFlag[4] = (bS[1] != 0 && HL_MATH_ABS_INT32(p0[4] - q0[4]) < alpha && HL_MATH_ABS_INT32(p1[4] - p0[4]) < beta && HL_MATH_ABS_INT32(q1[4] - q0[4]) < beta);
    filterSamplesFlag[5] = (bS[1] != 0 && HL_MATH_ABS_INT32(p0[5] - q0[5]) < alpha && HL_MATH_ABS_INT32(p1[5] - p0[5]) < beta && HL_MATH_ABS_INT32(q1[5] - q0[5]) < beta);
    filterSamplesFlag[6] = (bS[1] != 0 && HL_MATH_ABS_INT32(p0[6] - q0[6]) < alpha && HL_MATH_ABS_INT32(p1[6] - p0[6]) < beta && HL_MATH_ABS_INT32(q1[6] - q0[6]) < beta);
    filterSamplesFlag[7] = (bS[1] != 0 && HL_MATH_ABS_INT32(p0[7] - q0[7]) < alpha && HL_MATH_ABS_INT32(p1[7] - p0[7]) < beta && HL_MATH_ABS_INT32(q1[7] - q0[7]) < beta);
}

// 8.7.2.2 Derivation process for the thresholds for each block edge
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_cpp(
    uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1,
    int16_t bS[4],
    int16_t alpha, int16_t beta,
    HL_OUT int16_t filterSamplesFlag[8])
{
    // (8-468)
    filterSamplesFlag[0] = (bS[0] != 0 && HL_MATH_ABS_INT32(p0[0] - q0[0]) < alpha && HL_MATH_ABS_INT32(p1[0] - p0[0]) < beta && HL_MATH_ABS_INT32(q1[0] - q0[0]) < beta);
    filterSamplesFlag[1] = (bS[0] != 0 && HL_MATH_ABS_INT32(p0[1] - q0[1]) < alpha && HL_MATH_ABS_INT32(p1[1] - p0[1]) < beta && HL_MATH_ABS_INT32(q1[1] - q0[1]) < beta);
    filterSamplesFlag[2] = (bS[1] != 0 && HL_MATH_ABS_INT32(p0[2] - q0[2]) < alpha && HL_MATH_ABS_INT32(p1[2] - p0[2]) < beta && HL_MATH_ABS_INT32(q1[2] - q0[2]) < beta);
    filterSamplesFlag[3] = (bS[1] != 0 && HL_MATH_ABS_INT32(p0[3] - q0[3]) < alpha && HL_MATH_ABS_INT32(p1[3] - p0[3]) < beta && HL_MATH_ABS_INT32(q1[3] - q0[3]) < beta);
    filterSamplesFlag[4] = (bS[2] != 0 && HL_MATH_ABS_INT32(p0[4] - q0[4]) < alpha && HL_MATH_ABS_INT32(p1[4] - p0[4]) < beta && HL_MATH_ABS_INT32(q1[4] - q0[4]) < beta);
    filterSamplesFlag[5] = (bS[2] != 0 && HL_MATH_ABS_INT32(p0[5] - q0[5]) < alpha && HL_MATH_ABS_INT32(p1[5] - p0[5]) < beta && HL_MATH_ABS_INT32(q1[5] - q0[5]) < beta);
    filterSamplesFlag[6] = (bS[3] != 0 && HL_MATH_ABS_INT32(p0[6] - q0[6]) < alpha && HL_MATH_ABS_INT32(p1[6] - p0[6]) < beta && HL_MATH_ABS_INT32(q1[6] - q0[6]) < beta);
    filterSamplesFlag[7] = (bS[3] != 0 && HL_MATH_ABS_INT32(p0[7] - q0[7]) < alpha && HL_MATH_ABS_INT32(p1[7] - p0[7]) < beta && HL_MATH_ABS_INT32(q1[7] - q0[7]) < beta);
}

// 8.7.2.3 Filtering process for edges with bS less than 4
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter1samples_bs_lt4_u8(
    const uint8_t p0, const uint8_t p1, const uint8_t p2,
    const uint8_t q0, const uint8_t q1, const uint8_t q2,
    int32_t chromaStyleFilteringFlag,
    int16_t bS,
    int16_t indexA,
    int16_t beta,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    int16_t tc0, tc, ap, aq, delta;

    tc0 = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS];
    ap = HL_MATH_ABS_INT32(p2 - p0); // (8-471)
    aq = HL_MATH_ABS_INT32(q2 - q0); // (8-472)

    if (chromaStyleFilteringFlag == 0) {
        tc = tc0 + ((ap < beta) ? 1 : 0) + ((aq < beta) ? 1 : 0); // (8-473)
    }
    else {
        tc = tc0 + 1;
    }

    delta = HL_MATH_CLIP3(-tc, tc, ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3)); // (8-475)

    //(8-476) (8-477)
    *pf0 = HL_MATH_CLIP3(0, 255, (p0 + delta));
    *qf0 = HL_MATH_CLIP3(0, 255, (q0 - delta));

    if (chromaStyleFilteringFlag == 0 && ap < beta) {
        *pf1 = p1 + HL_MATH_CLIP3(-tc0, tc0, (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1); // (8-478)
    }
    else {
        *pf1 = p1; // (8-479)
    }
    if (chromaStyleFilteringFlag == 0 && aq < beta) {
        *qf1 = q1 + HL_MATH_CLIP3(-tc0, tc0, (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1); // (8-480)
    }
    else {
        *qf1 = q1; //(8-481)
    }

    *pf2 = p2; //(8-482)
    *qf2 = q2; //(8-483)
}

// 8.7.2.3 Filtering process for edges with bS less than 4 (luma)
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_cpp(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2,
    int16_t bS[2],
    int16_t indexA,
    int16_t beta,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    int16_t tc0[8];
    int16_t tc[8];
    int16_t ap[8];
    int16_t aq[8];
    int16_t delta[8];

    tc0[0] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]];
    tc0[1] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]];
    tc0[2] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]];
    tc0[3] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]];
    tc0[4] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]];
    tc0[5] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]];
    tc0[6] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]];
    tc0[7] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]];


    // (8-469) and (8-470) useless because "BitDepthY" and "BitDepthY" are equal to #8

    // (8-471)
    ap[0] = HL_MATH_ABS_INT32(p2[0] - p0[0]);
    ap[1] = HL_MATH_ABS_INT32(p2[1] - p0[1]);
    ap[2] = HL_MATH_ABS_INT32(p2[2] - p0[2]);
    ap[3] = HL_MATH_ABS_INT32(p2[3] - p0[3]);
    ap[4] = HL_MATH_ABS_INT32(p2[4] - p0[4]);
    ap[5] = HL_MATH_ABS_INT32(p2[5] - p0[5]);
    ap[6] = HL_MATH_ABS_INT32(p2[6] - p0[6]);
    ap[7] = HL_MATH_ABS_INT32(p2[7] - p0[7]);

    // (8-472)
    aq[0] = HL_MATH_ABS_INT32(q2[0] - q0[0]);
    aq[1] = HL_MATH_ABS_INT32(q2[1] - q0[1]);
    aq[2] = HL_MATH_ABS_INT32(q2[2] - q0[2]);
    aq[3] = HL_MATH_ABS_INT32(q2[3] - q0[3]);
    aq[4] = HL_MATH_ABS_INT32(q2[4] - q0[4]);
    aq[5] = HL_MATH_ABS_INT32(q2[5] - q0[5]);
    aq[6] = HL_MATH_ABS_INT32(q2[6] - q0[6]);
    aq[7] = HL_MATH_ABS_INT32(q2[7] - q0[7]);

    // (8-473)
    tc[0] = tc0[0] + ((ap[0] < beta) ? 1 : 0) + ((aq[0] < beta) ? 1 : 0);
    tc[1] = tc0[1] + ((ap[1] < beta) ? 1 : 0) + ((aq[1] < beta) ? 1 : 0);
    tc[2] = tc0[2] + ((ap[2] < beta) ? 1 : 0) + ((aq[2] < beta) ? 1 : 0);
    tc[3] = tc0[3] + ((ap[3] < beta) ? 1 : 0) + ((aq[3] < beta) ? 1 : 0);
    tc[4] = tc0[4] + ((ap[4] < beta) ? 1 : 0) + ((aq[4] < beta) ? 1 : 0);
    tc[5] = tc0[5] + ((ap[5] < beta) ? 1 : 0) + ((aq[5] < beta) ? 1 : 0);
    tc[6] = tc0[6] + ((ap[6] < beta) ? 1 : 0) + ((aq[6] < beta) ? 1 : 0);
    tc[7] = tc0[7] + ((ap[7] < beta) ? 1 : 0) + ((aq[7] < beta) ? 1 : 0);

    // (8-475)
    delta[0] = HL_MATH_CLIP3(-tc[0], tc[0], ((((q0[0] - p0[0]) << 2) + (p1[0] - q1[0]) + 4) >> 3));
    delta[1] = HL_MATH_CLIP3(-tc[1], tc[1], ((((q0[1] - p0[1]) << 2) + (p1[1] - q1[1]) + 4) >> 3));
    delta[2] = HL_MATH_CLIP3(-tc[2], tc[2], ((((q0[2] - p0[2]) << 2) + (p1[2] - q1[2]) + 4) >> 3));
    delta[3] = HL_MATH_CLIP3(-tc[3], tc[3], ((((q0[3] - p0[3]) << 2) + (p1[3] - q1[3]) + 4) >> 3));
    delta[4] = HL_MATH_CLIP3(-tc[4], tc[4], ((((q0[4] - p0[4]) << 2) + (p1[4] - q1[4]) + 4) >> 3));
    delta[5] = HL_MATH_CLIP3(-tc[5], tc[5], ((((q0[5] - p0[5]) << 2) + (p1[5] - q1[5]) + 4) >> 3));
    delta[6] = HL_MATH_CLIP3(-tc[6], tc[6], ((((q0[6] - p0[6]) << 2) + (p1[6] - q1[6]) + 4) >> 3));
    delta[7] = HL_MATH_CLIP3(-tc[7], tc[7], ((((q0[7] - p0[7]) << 2) + (p1[7] - q1[7]) + 4) >> 3));

    // (8-476) (8-477)
    pf0[0] = HL_MATH_CLIP3(0, 255, (p0[0] + delta[0]));
    pf0[1] = HL_MATH_CLIP3(0, 255, (p0[1] + delta[1]));
    pf0[2] = HL_MATH_CLIP3(0, 255, (p0[2] + delta[2]));
    pf0[3] = HL_MATH_CLIP3(0, 255, (p0[3] + delta[3]));
    pf0[4] = HL_MATH_CLIP3(0, 255, (p0[4] + delta[4]));
    pf0[5] = HL_MATH_CLIP3(0, 255, (p0[5] + delta[5]));
    pf0[6] = HL_MATH_CLIP3(0, 255, (p0[6] + delta[6]));
    pf0[7] = HL_MATH_CLIP3(0, 255, (p0[7] + delta[7]));

    qf0[0] = HL_MATH_CLIP3(0, 255, (q0[0] - delta[0]));
    qf0[1] = HL_MATH_CLIP3(0, 255, (q0[1] - delta[1]));
    qf0[2] = HL_MATH_CLIP3(0, 255, (q0[2] - delta[2]));
    qf0[3] = HL_MATH_CLIP3(0, 255, (q0[3] - delta[3]));
    qf0[4] = HL_MATH_CLIP3(0, 255, (q0[4] - delta[4]));
    qf0[5] = HL_MATH_CLIP3(0, 255, (q0[5] - delta[5]));
    qf0[6] = HL_MATH_CLIP3(0, 255, (q0[6] - delta[6]));
    qf0[7] = HL_MATH_CLIP3(0, 255, (q0[7] - delta[7]));

    // (8-478) - SIMD => AVG(p0, q0)
    if (ap[0] < beta) {
        pf1[0] = p1[0] + HL_MATH_CLIP3(-tc0[0], tc0[0], (p2[0] + ((p0[0] + q0[0] + 1) >> 1) - (p1[0] << 1)) >> 1);
    }
    else {
        pf1[0] = p1[0];
    }
    if (ap[1] < beta) {
        pf1[1] = p1[1] + HL_MATH_CLIP3(-tc0[1], tc0[1], (p2[1] + ((p0[1] + q0[1] + 1) >> 1) - (p1[1] << 1)) >> 1);
    }
    else {
        pf1[1] = p1[1];
    }
    if (ap[2] < beta) {
        pf1[2] = p1[2] + HL_MATH_CLIP3(-tc0[2], tc0[2], (p2[2] + ((p0[2] + q0[2] + 1) >> 1) - (p1[2] << 1)) >> 1);
    }
    else {
        pf1[2] = p1[2];
    }
    if (ap[3] < beta) {
        pf1[3] = p1[3] + HL_MATH_CLIP3(-tc0[3], tc0[3], (p2[3] + ((p0[3] + q0[3] + 1) >> 1) - (p1[3] << 1)) >> 1);
    }
    else {
        pf1[3] = p1[3];
    }
    if (ap[4] < beta) {
        pf1[4] = p1[4] + HL_MATH_CLIP3(-tc0[4], tc0[4], (p2[4] + ((p0[4] + q0[4] + 1) >> 1) - (p1[4] << 1)) >> 1);
    }
    else {
        pf1[4] = p1[4];
    }
    if (ap[5] < beta) {
        pf1[5] = p1[5] + HL_MATH_CLIP3(-tc0[5], tc0[5], (p2[5] + ((p0[5] + q0[5] + 1) >> 1) - (p1[5] << 1)) >> 1);
    }
    else {
        pf1[5] = p1[5];
    }
    if (ap[6] < beta) {
        pf1[6] = p1[6] + HL_MATH_CLIP3(-tc0[6], tc0[6], (p2[6] + ((p0[6] + q0[6] + 1) >> 1) - (p1[6] << 1)) >> 1);
    }
    else {
        pf1[6] = p1[6];
    }
    if (ap[7] < beta) {
        pf1[7] = p1[7] + HL_MATH_CLIP3(-tc0[7], tc0[7], (p2[7] + ((p0[7] + q0[7] + 1) >> 1) - (p1[7] << 1)) >> 1);
    }
    else {
        pf1[7] = p1[7];
    }

    // (8-480) - SIMD => AVG(p0, q0)
    if (aq[0] < beta) {
        qf1[0] = q1[0] + HL_MATH_CLIP3(-tc0[0], tc0[0], (q2[0] + ((p0[0] + q0[0] + 1) >> 1) - (q1[0] << 1)) >> 1);
    }
    else {
        qf1[0] = q1[0];
    }
    if (aq[1] < beta) {
        qf1[1] = q1[1] + HL_MATH_CLIP3(-tc0[1], tc0[1], (q2[1] + ((p0[1] + q0[1] + 1) >> 1) - (q1[1] << 1)) >> 1);
    }
    else {
        qf1[1] = q1[1];
    }
    if (aq[2] < beta) {
        qf1[2] = q1[2] + HL_MATH_CLIP3(-tc0[2], tc0[2], (q2[2] + ((p0[2] + q0[2] + 1) >> 1) - (q1[2] << 1)) >> 1);
    }
    else {
        qf1[2] = q1[2];
    }
    if (aq[3] < beta) {
        qf1[3] = q1[3] + HL_MATH_CLIP3(-tc0[3], tc0[3], (q2[3] + ((p0[3] + q0[3] + 1) >> 1) - (q1[3] << 1)) >> 1);
    }
    else {
        qf1[3] = q1[3];
    }
    if (aq[4] < beta) {
        qf1[4] = q1[4] + HL_MATH_CLIP3(-tc0[4], tc0[4], (q2[4] + ((p0[4] + q0[4] + 1) >> 1) - (q1[4] << 1)) >> 1);
    }
    else {
        qf1[4] = q1[4];
    }
    if (aq[5] < beta) {
        qf1[5] = q1[5] + HL_MATH_CLIP3(-tc0[5], tc0[5], (q2[5] + ((p0[5] + q0[5] + 1) >> 1) - (q1[5] << 1)) >> 1);
    }
    else {
        qf1[5] = q1[5];
    }
    if (aq[6] < beta) {
        qf1[6] = q1[6] + HL_MATH_CLIP3(-tc0[6], tc0[6], (q2[6] + ((p0[6] + q0[6] + 1) >> 1) - (q1[6] << 1)) >> 1);
    }
    else {
        qf1[6] = q1[6];
    }
    if (aq[7] < beta) {
        qf1[7] = q1[7] + HL_MATH_CLIP3(-tc0[7], tc0[7], (q2[7] + ((p0[7] + q0[7] + 1) >> 1) - (q1[7] << 1)) >> 1);
    }
    else {
        qf1[7] = q1[7];
    }


    // (8-482)
    pf2[0] = p2[0];
    pf2[1] = p2[1];
    pf2[2] = p2[2];
    pf2[3] = p2[3];
    pf2[4] = p2[4];
    pf2[5] = p2[5];
    pf2[6] = p2[6];
    pf2[7] = p2[7];

    // (8-483)
    qf2[0] = q2[0];
    qf2[1] = q2[1];
    qf2[2] = q2[2];
    qf2[3] = q2[3];
    qf2[4] = q2[4];
    qf2[5] = q2[5];
    qf2[6] = q2[6];
    qf2[7] = q2[7];
}

// 8.7.2.3 Filtering process for edges with bS less than 4 (chroma)
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_cpp(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2,
    int16_t bS[4],
    int16_t indexA,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    int16_t tc0[8];
    int16_t tc[8];
    int16_t delta[8];

    tc0[0] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]];
    tc0[1] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[0]];
    tc0[2] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]];
    tc0[3] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[1]];
    tc0[4] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[2]];
    tc0[5] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[2]];
    tc0[6] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[3]];
    tc0[7] = HL_CODEC_264_DEBLOCK_THRESHOLD_TABLE[indexA][bS[3]];

    // (8-469) and (8-470) useless because "BitDepthY" and "BitDepthY" are equal to #8

    // (8-473)
    tc[0] = tc0[0] + 1;
    tc[1] = tc0[1] + 1;
    tc[2] = tc0[2] + 1;
    tc[3] = tc0[3] + 1;
    tc[4] = tc0[4] + 1;
    tc[5] = tc0[5] + 1;
    tc[6] = tc0[6] + 1;
    tc[7] = tc0[7] + 1;

    // (8-475)
    delta[0] = HL_MATH_CLIP3(-tc[0], tc[0], ((((q0[0] - p0[0]) << 2) + (p1[0] - q1[0]) + 4) >> 3));
    delta[1] = HL_MATH_CLIP3(-tc[1], tc[1], ((((q0[1] - p0[1]) << 2) + (p1[1] - q1[1]) + 4) >> 3));
    delta[2] = HL_MATH_CLIP3(-tc[2], tc[2], ((((q0[2] - p0[2]) << 2) + (p1[2] - q1[2]) + 4) >> 3));
    delta[3] = HL_MATH_CLIP3(-tc[3], tc[3], ((((q0[3] - p0[3]) << 2) + (p1[3] - q1[3]) + 4) >> 3));
    delta[4] = HL_MATH_CLIP3(-tc[4], tc[4], ((((q0[4] - p0[4]) << 2) + (p1[4] - q1[4]) + 4) >> 3));
    delta[5] = HL_MATH_CLIP3(-tc[5], tc[5], ((((q0[5] - p0[5]) << 2) + (p1[5] - q1[5]) + 4) >> 3));
    delta[6] = HL_MATH_CLIP3(-tc[6], tc[6], ((((q0[6] - p0[6]) << 2) + (p1[6] - q1[6]) + 4) >> 3));
    delta[7] = HL_MATH_CLIP3(-tc[7], tc[7], ((((q0[7] - p0[7]) << 2) + (p1[7] - q1[7]) + 4) >> 3));

    // (8-476) (8-477)
    pf0[0] = HL_MATH_CLIP3(0, 255, (p0[0] + delta[0]));
    pf0[1] = HL_MATH_CLIP3(0, 255, (p0[1] + delta[1]));
    pf0[2] = HL_MATH_CLIP3(0, 255, (p0[2] + delta[2]));
    pf0[3] = HL_MATH_CLIP3(0, 255, (p0[3] + delta[3]));
    pf0[4] = HL_MATH_CLIP3(0, 255, (p0[4] + delta[4]));
    pf0[5] = HL_MATH_CLIP3(0, 255, (p0[5] + delta[5]));
    pf0[6] = HL_MATH_CLIP3(0, 255, (p0[6] + delta[6]));
    pf0[7] = HL_MATH_CLIP3(0, 255, (p0[7] + delta[7]));

    qf0[0] = HL_MATH_CLIP3(0, 255, (q0[0] - delta[0]));
    qf0[1] = HL_MATH_CLIP3(0, 255, (q0[1] - delta[1]));
    qf0[2] = HL_MATH_CLIP3(0, 255, (q0[2] - delta[2]));
    qf0[3] = HL_MATH_CLIP3(0, 255, (q0[3] - delta[3]));
    qf0[4] = HL_MATH_CLIP3(0, 255, (q0[4] - delta[4]));
    qf0[5] = HL_MATH_CLIP3(0, 255, (q0[5] - delta[5]));
    qf0[6] = HL_MATH_CLIP3(0, 255, (q0[6] - delta[6]));
    qf0[7] = HL_MATH_CLIP3(0, 255, (q0[7] - delta[7]));

    // (8-479)
    pf1[0] = p1[0];
    pf1[1] = p1[1];
    pf1[2] = p1[2];
    pf1[3] = p1[3];
    pf1[4] = p1[4];
    pf1[5] = p1[5];
    pf1[6] = p1[6];
    pf1[7] = p1[7];

    // (8-481)
    qf1[0] = q1[0];
    qf1[1] = q1[1];
    qf1[2] = q1[2];
    qf1[3] = q1[3];
    qf1[4] = q1[4];
    qf1[5] = q1[5];
    qf1[6] = q1[6];
    qf1[7] = q1[7];

    // (8-482)
    pf2[0] = p2[0];
    pf2[1] = p2[1];
    pf2[2] = p2[2];
    pf2[3] = p2[3];
    pf2[4] = p2[4];
    pf2[5] = p2[5];
    pf2[6] = p2[6];
    pf2[7] = p2[7];

    // (8-483)
    qf2[0] = q2[0];
    qf2[1] = q2[1];
    qf2[2] = q2[2];
    qf2[3] = q2[3];
    qf2[4] = q2[4];
    qf2[5] = q2[5];
    qf2[6] = q2[6];
    qf2[7] = q2[7];
}

static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2,
    int32_t chromaStyleFilteringFlag,
    int16_t *bS, // 2 for luma and 4 for chroma
    int16_t indexA,
    int16_t beta,
    int16_t filterSamplesFlag[8],
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
#define MOVE64(dst, src) \
        *((uint64_t*)&(dst)[0])=*((uint64_t*)&(src)[0])
#define BYPASS_SAMPLE(_i) \
	pf0[(_i)] = p0[(_i)], pf1[(_i)] = p1[(_i)], pf2[(_i)] = p2[(_i)], qf0[(_i)] = q0[(_i)], qf1[(_i)] = q1[(_i)], qf2[(_i)] = q2[(_i)]
    int16_t i;
    int16_t numSamplesToFilter = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS(filterSamplesFlag);

    // (8-459) and (8-460)
    // TODO: optimize
    if (numSamplesToFilter == 0) {
        MOVE64(&pf0[0], p0);
        MOVE64(&pf1[0], p1);
        MOVE64(&pf2[0], p2);

        MOVE64(&qf0[0], q0);
        MOVE64(&qf1[0], q1);
        MOVE64(&qf2[0], q2);
        return;
    }

    if (numSamplesToFilter < kMinNumSamplesToFilterUsingSIMD) {
        // Doesn't worth doing the job using SIMD or cache optimization
        const int32_t bS_RQShift = chromaStyleFilteringFlag ? 1 : 2; // #4bS for chroma and #2 for luma
        for (i = 0; i < 8; ++i) {
            if (filterSamplesFlag[i] == 1) {
                hl_codec_264_deblock_avc_baseline_filter1samples_bs_lt4_u8(
                    p0[i], p1[i], p2[i], q0[i], q1[i], q2[i],
                    chromaStyleFilteringFlag, bS[i>>bS_RQShift], indexA, beta,
                    &pf0[i], &pf1[i], &pf2[i], &qf0[i], &qf1[i], &qf2[i]);
            }
            else {
                BYPASS_SAMPLE(i);
            }
        }
    }
    else {
        if (chromaStyleFilteringFlag) {
            hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8(
                p0, p1, p2, q0, q1, q2,
                bS, indexA,
                pf0, pf1, pf2, qf0, qf1, qf2);
        }
        else {
            hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8(
                p0, p1, p2, q0, q1, q2,
                bS, indexA, beta,
                pf0, pf1, pf2, qf0, qf1, qf2);
        }

        if (filterSamplesFlag[0] == 0) {
            BYPASS_SAMPLE(0);
        }
        if (filterSamplesFlag[1] == 0) {
            BYPASS_SAMPLE(1);
        }
        if (filterSamplesFlag[2] == 0) {
            BYPASS_SAMPLE(2);
        }
        if (filterSamplesFlag[3] == 0) {
            BYPASS_SAMPLE(3);
        }
        if (filterSamplesFlag[4] == 0) {
            BYPASS_SAMPLE(4);
        }
        if (filterSamplesFlag[5] == 0) {
            BYPASS_SAMPLE(5);
        }
        if (filterSamplesFlag[6] == 0) {
            BYPASS_SAMPLE(6);
        }
        if (filterSamplesFlag[7] == 0) {
            BYPASS_SAMPLE(7);
        }
    }

#undef MOVE64
#undef BYPASS_SAMPLE
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
// TODO: add INTRIN and ASM versions
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8_cpp(
    const uint8_t p0, const uint8_t p1, const uint8_t p2, const uint8_t p3,
    const uint8_t q0, const uint8_t q1, const uint8_t q2, const uint8_t q3,
    int32_t chromaStyleFilteringFlag,
    int16_t indexA,
    int16_t alpha, int16_t beta,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    int16_t ap, aq;

    ap = HL_MATH_ABS_INT32(p2 - p0); // (8-471)
    aq = HL_MATH_ABS_INT32(q2 - q0); // (8-472)

    if (chromaStyleFilteringFlag == 0 && (ap < beta && HL_MATH_ABS_INT32(p0 - q0) < ((alpha >> 2) + 2))) {
        *pf0 = (p2 + (p1 << 1) + (p0 << 1) + (q0 << 1) + q1 + 4) >> 3; // (8-485)
        *pf1 = (p2 + p1 + p0 + q0 + 2) >> 2; // (8-486)
        *pf2 = ((p3 << 1) + (p2 << 1) + p2 + p1 + p0 + q0 + 4) >> 3; // (8-487)
    }
    else {
        *pf0 = ((p1 << 1) + p0 + q1 + 2) >> 2; // (8-488)
        *pf1 = p1; // (8-489)
        *pf2 = p2; // (8-490)
    }

    if (chromaStyleFilteringFlag == 0 && (aq < beta && HL_MATH_ABS_INT32(p0 - q0) < ((alpha >> 2) + 2))) {
        *qf0 = (p1 + (p0 << 1) + (q0 << 1) + (q1 << 1) + q2 + 4) >> 3; // (8-492)
        *qf1 = (p0 + q0 + q1 + q2 + 2 ) >> 2; // (8-493)
        *qf2 = ((q3 << 1) + (q2 << 1) + q2 + q1 + q0 + p0 + 4) >> 3; // (8-494)
    }
    else {
        *qf0 = ((q1 << 1) + q0 + p1 + 2) >> 2; //(8-495)
        *qf1 = q1; //(8-496)
        *qf2 = q2; //(8-497)
    }
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
// TODO: add ASM version
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_cpp(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3,
    int16_t alpha, int16_t beta,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    int16_t ap[8];
    int16_t aq[8];

    // (8-471)
    ap[0] = HL_MATH_ABS_INT32(p2[0] - p0[0]);
    ap[1] = HL_MATH_ABS_INT32(p2[1] - p0[1]);
    ap[2] = HL_MATH_ABS_INT32(p2[2] - p0[2]);
    ap[3] = HL_MATH_ABS_INT32(p2[3] - p0[3]);
    ap[4] = HL_MATH_ABS_INT32(p2[4] - p0[4]);
    ap[5] = HL_MATH_ABS_INT32(p2[5] - p0[5]);
    ap[6] = HL_MATH_ABS_INT32(p2[6] - p0[6]);
    ap[7] = HL_MATH_ABS_INT32(p2[7] - p0[7]);

    // (8-472)
    aq[0] = HL_MATH_ABS_INT32(q2[0] - q0[0]);
    aq[1] = HL_MATH_ABS_INT32(q2[1] - q0[1]);
    aq[2] = HL_MATH_ABS_INT32(q2[2] - q0[2]);
    aq[3] = HL_MATH_ABS_INT32(q2[3] - q0[3]);
    aq[4] = HL_MATH_ABS_INT32(q2[4] - q0[4]);
    aq[5] = HL_MATH_ABS_INT32(q2[5] - q0[5]);
    aq[6] = HL_MATH_ABS_INT32(q2[6] - q0[6]);
    aq[7] = HL_MATH_ABS_INT32(q2[7] - q0[7]);

    if ((ap[0] < beta && HL_MATH_ABS_INT32(p0[0] - q0[0]) < ((alpha >> 2) + 2))) {
        pf0[0] = (p2[0] + (p1[0] << 1) + (p0[0] << 1) + (q0[0] << 1) + q1[0] + 4) >> 3; // (8-485)
        pf1[0] = (p2[0] + p1[0] + p0[0] + q0[0] + 2) >> 2; // (8-486)
        pf2[0] = ((p3[0] << 1) + 3*p2[0] + p1[0] + p0[0] + q0[0] + 4) >> 3; // (8-487)
    }
    else {
        pf0[0] = ((p1[0] << 1) + p0[0] + q1[0] + 2) >> 2;// (8-488)
        pf1[0] = p1[0];// (8-489)
        pf2[0] = p2[0];// (8-490)
    }
    if ((ap[1] < beta && HL_MATH_ABS_INT32(p0[1] - q0[1]) < ((alpha >> 2) + 2))) {
        pf0[1] = (p2[1] + (p1[1] << 1) + (p0[1] << 1) + (q0[1] << 1) + q1[1] + 4) >> 3; // (8-485)
        pf1[1] = (p2[1] + p1[1] + p0[1] + q0[1] + 2) >> 2; // (8-486)
        pf2[1] = ((p3[1] << 1) + 3*p2[1] + p1[1] + p0[1] + q0[1] + 4) >> 3; // (8-487)
    }
    else {
        pf0[1] = ((p1[1] << 1) + p0[1] + q1[1] + 2) >> 2;// (8-488)
        pf1[1] = p1[1];// (8-489)
        pf2[1] = p2[1];// (8-490)
    }
    if ((ap[2] < beta && HL_MATH_ABS_INT32(p0[2] - q0[2]) < ((alpha >> 2) + 2))) {
        pf0[2] = (p2[2] + (p1[2] << 1) + (p0[2] << 1) + (q0[2] << 1) + q1[2] + 4) >> 3; // (8-485)
        pf1[2] = (p2[2] + p1[2] + p0[2] + q0[2] + 2) >> 2; // (8-486)
        pf2[2] = ((p3[2] << 1) + 3*p2[2] + p1[2] + p0[2] + q0[2] + 4) >> 3; // (8-487)
    }
    else {
        pf0[2] = ((p1[2] << 1) + p0[2] + q1[2] + 2) >> 2;// (8-488)
        pf1[2] = p1[2];// (8-489)
        pf2[2] = p2[2];// (8-490)
    }
    if ((ap[3] < beta && HL_MATH_ABS_INT32(p0[3] - q0[3]) < ((alpha >> 2) + 2))) {
        pf0[3] = (p2[3] + (p1[3] << 1) + (p0[3] << 1) + (q0[3] << 1) + q1[3] + 4) >> 3; // (8-485)
        pf1[3] = (p2[3] + p1[3] + p0[3] + q0[3] + 2) >> 2; // (8-486)
        pf2[3] = ((p3[3] << 1) + 3*p2[3] + p1[3] + p0[3] + q0[3] + 4) >> 3; // (8-487)
    }
    else {
        pf0[3] = ((p1[3] << 1) + p0[3] + q1[3] + 2) >> 2;// (8-488)
        pf1[3] = p1[3];// (8-489)
        pf2[3] = p2[3];// (8-490)
    }
    if ((ap[4] < beta && HL_MATH_ABS_INT32(p0[4] - q0[4]) < ((alpha >> 2) + 2))) {
        pf0[4] = (p2[4] + (p1[4] << 1) + (p0[4] << 1) + (q0[4] << 1) + q1[4] + 4) >> 3; // (8-485)
        pf1[4] = (p2[4] + p1[4] + p0[4] + q0[4] + 2) >> 2; // (8-486)
        pf2[4] = ((p3[4] << 1) + 3*p2[4] + p1[4] + p0[4] + q0[4] + 4) >> 3; // (8-487)
    }
    else {
        pf0[4] = ((p1[4] << 1) + p0[4] + q1[4] + 2) >> 2;// (8-488)
        pf1[4] = p1[4];// (8-489)
        pf2[4] = p2[4];// (8-490)
    }
    if ((ap[5] < beta && HL_MATH_ABS_INT32(p0[5] - q0[5]) < ((alpha >> 2) + 2))) {
        pf0[5] = (p2[5] + (p1[5] << 1) + (p0[5] << 1) + (q0[5] << 1) + q1[5] + 4) >> 3; // (8-485)
        pf1[5] = (p2[5] + p1[5] + p0[5] + q0[5] + 2) >> 2; // (8-486)
        pf2[5] = ((p3[5] << 1) + 3*p2[5] + p1[5] + p0[5] + q0[5] + 4) >> 3; // (8-487)
    }
    else {
        pf0[5] = ((p1[5] << 1) + p0[5] + q1[5] + 2) >> 2;// (8-488)
        pf1[5] = p1[5];// (8-489)
        pf2[5] = p2[5];// (8-490)
    }
    if ((ap[6] < beta && HL_MATH_ABS_INT32(p0[6] - q0[6]) < ((alpha >> 2) + 2))) {
        pf0[6] = (p2[6] + (p1[6] << 1) + (p0[6] << 1) + (q0[6] << 1) + q1[6] + 4) >> 3; // (8-485)
        pf1[6] = (p2[6] + p1[6] + p0[6] + q0[6] + 2) >> 2; // (8-486)
        pf2[6] = ((p3[6] << 1) + 3*p2[6] + p1[6] + p0[6] + q0[6] + 4) >> 3; // (8-487)
    }
    else {
        pf0[6] = ((p1[6] << 1) + p0[6] + q1[6] + 2) >> 2;// (8-488)
        pf1[6] = p1[6];// (8-489)
        pf2[6] = p2[6];// (8-490)
    }
    if ((ap[7] < beta && HL_MATH_ABS_INT32(p0[7] - q0[7]) < ((alpha >> 2) + 2))) {
        pf0[7] = (p2[7] + (p1[7] << 1) + (p0[7] << 1) + (q0[7] << 1) + q1[7] + 4) >> 3; // (8-485)
        pf1[7] = (p2[7] + p1[7] + p0[7] + q0[7] + 2) >> 2; // (8-486)
        pf2[7] = ((p3[7] << 1) + 3*p2[7] + p1[7] + p0[7] + q0[7] + 4) >> 3; // (8-487)
    }
    else {
        pf0[7] = ((p1[7] << 1) + p0[7] + q1[7] + 2) >> 2;// (8-488)
        pf1[7] = p1[7];// (8-489)
        pf2[7] = p2[7];// (8-490)
    }

    if ((aq[0] < beta && HL_MATH_ABS_INT32(p0[0] - q0[0]) < ((alpha >> 2) + 2))) {
        qf0[0] = (p1[0] + (p0[0] << 1) + (q0[0] << 1) + (q1[0] << 1) + q2[0] + 4) >> 3;// (8-492)
        qf1[0] = (p0[0] + q0[0] + q1[0] + q2[0] + 2 ) >> 2;// (8-493)
        qf2[0] = ((q3[0] << 1) + 3*q2[0] + q1[0] + q0[0] + p0[0] + 4) >> 3;// (8-494)
    }
    else {
        qf0[0] = ((q1[0] << 1) + q0[0] + p1[0] + 2) >> 2; //(8-495)
        qf1[0] = q1[0]; //(8-496)
        qf2[0] = q2[0]; //(8-497)
    }
    if ((aq[1] < beta && HL_MATH_ABS_INT32(p0[1] - q0[1]) < ((alpha >> 2) + 2))) {
        qf0[1] = (p1[1] + (p0[1] << 1) + (q0[1] << 1) + (q1[1] << 1) + q2[1] + 4) >> 3;// (8-492)
        qf1[1] = (p0[1] + q0[1] + q1[1] + q2[1] + 2 ) >> 2;// (8-493)
        qf2[1] = ((q3[1] << 1) + 3*q2[1] + q1[1] + q0[1] + p0[1] + 4) >> 3;// (8-494)
    }
    else {
        qf0[1] = ((q1[1] << 1) + q0[1] + p1[1] + 2) >> 2; //(8-495)
        qf1[1] = q1[1]; //(8-496)
        qf2[1] = q2[1]; //(8-497)
    }
    if ((aq[2] < beta && HL_MATH_ABS_INT32(p0[2] - q0[2]) < ((alpha >> 2) + 2))) {
        qf0[2] = (p1[2] + (p0[2] << 1) + (q0[2] << 1) + (q1[2] << 1) + q2[2] + 4) >> 3;// (8-492)
        qf1[2] = (p0[2] + q0[2] + q1[2] + q2[2] + 2 ) >> 2;// (8-493)
        qf2[2] = ((q3[2] << 1) + 3*q2[2] + q1[2] + q0[2] + p0[2] + 4) >> 3;// (8-494)
    }
    else {
        qf0[2] = ((q1[2] << 1) + q0[2] + p1[2] + 2) >> 2; //(8-495)
        qf1[2] = q1[2]; //(8-496)
        qf2[2] = q2[2]; //(8-497)
    }
    if ((aq[3] < beta && HL_MATH_ABS_INT32(p0[3] - q0[3]) < ((alpha >> 2) + 2))) {
        qf0[3] = (p1[3] + (p0[3] << 1) + (q0[3] << 1) + (q1[3] << 1) + q2[3] + 4) >> 3;// (8-492)
        qf1[3] = (p0[3] + q0[3] + q1[3] + q2[3] + 2 ) >> 2;// (8-493)
        qf2[3] = ((q3[3] << 1) + 3*q2[3] + q1[3] + q0[3] + p0[3] + 4) >> 3;// (8-494)
    }
    else {
        qf0[3] = ((q1[3] << 1) + q0[3] + p1[3] + 2) >> 2; //(8-495)
        qf1[3] = q1[3]; //(8-496)
        qf2[3] = q2[3]; //(8-497)
    }
    if ((aq[4] < beta && HL_MATH_ABS_INT32(p0[4] - q0[4]) < ((alpha >> 2) + 2))) {
        qf0[4] = (p1[4] + (p0[4] << 1) + (q0[4] << 1) + (q1[4] << 1) + q2[4] + 4) >> 3;// (8-492)
        qf1[4] = (p0[4] + q0[4] + q1[4] + q2[4] + 2 ) >> 2;// (8-493)
        qf2[4] = ((q3[4] << 1) + 3*q2[4] + q1[4] + q0[4] + p0[4] + 4) >> 3;// (8-494)
    }
    else {
        qf0[4] = ((q1[4] << 1) + q0[4] + p1[4] + 2) >> 2; //(8-495)
        qf1[4] = q1[4]; //(8-496)
        qf2[4] = q2[4]; //(8-497)
    }
    if ((aq[5] < beta && HL_MATH_ABS_INT32(p0[5] - q0[5]) < ((alpha >> 2) + 2))) {
        qf0[5] = (p1[5] + (p0[5] << 1) + (q0[5] << 1) + (q1[5] << 1) + q2[5] + 4) >> 3;// (8-492)
        qf1[5] = (p0[5] + q0[5] + q1[5] + q2[5] + 2 ) >> 2;// (8-493)
        qf2[5] = ((q3[5] << 1) + 3*q2[5] + q1[5] + q0[5] + p0[5] + 4) >> 3;// (8-494)
    }
    else {
        qf0[5] = ((q1[5] << 1) + q0[5] + p1[5] + 2) >> 2; //(8-495)
        qf1[5] = q1[5]; //(8-496)
        qf2[5] = q2[5]; //(8-497)
    }
    if ((aq[6] < beta && HL_MATH_ABS_INT32(p0[6] - q0[6]) < ((alpha >> 2) + 2))) {
        qf0[6] = (p1[6] + (p0[6] << 1) + (q0[6] << 1) + (q1[6] << 1) + q2[6] + 4) >> 3;// (8-492)
        qf1[6] = (p0[6] + q0[6] + q1[6] + q2[6] + 2 ) >> 2;// (8-493)
        qf2[6] = ((q3[6] << 1) + 3*q2[6] + q1[6] + q0[6] + p0[6] + 4) >> 3;// (8-494)
    }
    else {
        qf0[6] = ((q1[6] << 1) + q0[6] + p1[6] + 2) >> 2; //(8-495)
        qf1[6] = q1[6]; //(8-496)
        qf2[6] = q2[6]; //(8-497)
    }
    if ((aq[7] < beta && HL_MATH_ABS_INT32(p0[7] - q0[7]) < ((alpha >> 2) + 2))) {
        qf0[7] = (p1[7] + (p0[7] << 1) + (q0[7] << 1) + (q1[7] << 1) + q2[7] + 4) >> 3;// (8-492)
        qf1[7] = (p0[7] + q0[7] + q1[7] + q2[7] + 2 ) >> 2;// (8-493)
        qf2[7] = ((q3[7] << 1) + 3*q2[7] + q1[7] + q0[7] + p0[7] + 4) >> 3;// (8-494)
    }
    else {
        qf0[7] = ((q1[7] << 1) + q0[7] + p1[7] + 2) >> 2; //(8-495)
        qf1[7] = q1[7]; //(8-496)
        qf2[7] = q2[7]; //(8-497)
    }
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_cpp(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3,
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
    // (8-488)
    pf0[0] = ((p1[0] << 1) + p0[0] + q1[0] + 2) >> 2;
    pf0[1] = ((p1[1] << 1) + p0[1] + q1[1] + 2) >> 2;
    pf0[2] = ((p1[2] << 1) + p0[2] + q1[2] + 2) >> 2;
    pf0[3] = ((p1[3] << 1) + p0[3] + q1[3] + 2) >> 2;
    pf0[4] = ((p1[4] << 1) + p0[4] + q1[4] + 2) >> 2;
    pf0[5] = ((p1[5] << 1) + p0[5] + q1[5] + 2) >> 2;
    pf0[6] = ((p1[6] << 1) + p0[6] + q1[6] + 2) >> 2;
    pf0[7] = ((p1[7] << 1) + p0[7] + q1[7] + 2) >> 2;

    // (8-489)
    *((uint64_t*)pf1) = *((uint64_t*)p1);

    // (8-490)
    *((uint64_t*)pf2) = *((uint64_t*)p2);

    //(8-495)
    qf0[0] = ((q1[0] << 1) + q0[0] + p1[0] + 2) >> 2;
    qf0[1] = ((q1[1] << 1) + q0[1] + p1[1] + 2) >> 2;
    qf0[2] = ((q1[2] << 1) + q0[2] + p1[2] + 2) >> 2;
    qf0[3] = ((q1[3] << 1) + q0[3] + p1[3] + 2) >> 2;
    qf0[4] = ((q1[4] << 1) + q0[4] + p1[4] + 2) >> 2;
    qf0[5] = ((q1[5] << 1) + q0[5] + p1[5] + 2) >> 2;
    qf0[6] = ((q1[6] << 1) + q0[6] + p1[6] + 2) >> 2;
    qf0[7] = ((q1[7] << 1) + q0[7] + p1[7] + 2) >> 2;

    //(8-496)
    *((uint64_t*)qf1) = *((uint64_t*)q1);

    //(8-497)
    *((uint64_t*)qf2) = *((uint64_t*)q2);
}

// 8.7.2.4 Filtering process for edges for bS equal to 4
static HL_SHOULD_INLINE void hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
    const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3,
    const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3,
    int32_t chromaStyleFilteringFlag,
    int16_t indexA,
    int16_t alpha, int16_t beta,
    int16_t filterSamplesFlag[8],
    HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2,
    HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2
)
{
#define MOVE64(dst, src) \
        *((uint64_t*)&(dst)[0])=*((uint64_t*)&(src)[0])
#define BYPASS_SAMPLE(_i) \
	pf0[(_i)] = p0[(_i)], pf1[(_i)] = p1[(_i)], pf2[(_i)] = p2[(_i)], qf0[(_i)] = q0[(_i)], qf1[(_i)] = q1[(_i)], qf2[(_i)] = q2[(_i)]
    int16_t i;
    int16_t numSamplesToFilter = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS(filterSamplesFlag);

    // (8-459) and (8-460)
    // TODO: optimize
    if (numSamplesToFilter == 0) {
        MOVE64(&pf0[0], p0);
        MOVE64(&pf1[0], p1);
        MOVE64(&pf2[0], p2);

        MOVE64(&qf0[0], q0);
        MOVE64(&qf1[0], q1);
        MOVE64(&qf2[0], q2);
        return;
    }

    if (numSamplesToFilter < kMinNumSamplesToFilterUsingSIMD) {
        // Doesn't worth doing the job using SIMD or cache optimization
        for (i = 0; i < 8; ++i) {
            if (filterSamplesFlag[i] == 1) {
                hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8(
                    p0[i], p1[i], p2[i], p3[i], q0[i], q1[i], q2[i], q3[i],
                    chromaStyleFilteringFlag, indexA, alpha, beta,
                    &pf0[i], &pf1[i], &pf2[i], &qf0[i], &qf1[i], &qf2[i]);
            }
            else {
                BYPASS_SAMPLE(i);
            }
        }
    }
    else {
        if (chromaStyleFilteringFlag) {
            hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8(
                p0, p1, p2, p3, q0, q1, q2, q3,
                pf0, pf1, pf2, qf0, qf1, qf2);
        }
        else {
            hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8(
                p0, p1, p2, p3, q0, q1, q2, q3,
                alpha, beta, pf0, pf1, pf2, qf0, qf1, qf2);
        }
        if (filterSamplesFlag[0] == 0) {
            BYPASS_SAMPLE(0);
        }
        if (filterSamplesFlag[1] == 0) {
            BYPASS_SAMPLE(1);
        }
        if (filterSamplesFlag[2] == 0) {
            BYPASS_SAMPLE(2);
        }
        if (filterSamplesFlag[3] == 0) {
            BYPASS_SAMPLE(3);
        }
        if (filterSamplesFlag[4] == 0) {
            BYPASS_SAMPLE(4);
        }
        if (filterSamplesFlag[5] == 0) {
            BYPASS_SAMPLE(5);
        }
        if (filterSamplesFlag[6] == 0) {
            BYPASS_SAMPLE(6);
        }
        if (filterSamplesFlag[7] == 0) {
            BYPASS_SAMPLE(7);
        }
    }

#undef BYPASS_SAMPLE
#undef MOVE64
}

HL_ASYNC_CALL_DIRECT
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_baseline_mb_luma_u8(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb)
{
    int16_t indexA, alpha, beta, __bS[2/*8x8 index*/][2/*4x4 index*/];

    uint8_t *pc_luma_sample, *_pc_luma_sample;
    hl_codec_264_layer_t* pc_layer;
    uint32_t u_luma_stride;
    HL_ALIGNED(16) uint8_t (*p)[4][16], (*q)[4][16], (*pf)[3][16], (*qf)[3][16];
    HL_ALIGNED(16) int16_t (*filterSamplesFlag)[16], (*filterSamplesFlag0)[8], (*filterSamplesFlag1)[8],  numSamplesToFilter, numSamplesToFilter0, numSamplesToFilter1;
    int32_t mbPx, mbPy, mbQx, mbQy, filterOffsetA, filterOffsetB;
    int16_t qPp_luma, qPq_luma;
    const hl_codec_264_mb_t *pc_mbP, *pc_mbQ;
    static hl_bool_t __is_mb_edge_Yes = HL_TRUE;
    static hl_bool_t __is_mb_edge_No = HL_FALSE;
    static int32_t __chromaStyleFilteringFlag_Zero = 0;
    const hl_codec_264_nal_slice_header_t *pc_slice_headerQ;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    pc_layer = p_codec->layers.pc_active;
    pc_luma_sample = (uint8_t *)pc_layer->pc_fs_curr->p_pict->pc_data_y;
    u_luma_stride = pc_layer->pc_fs_curr->p_pict->uWidthL;

    pc_mbP = pc_mbQ = p_mb;
    pc_slice_headerQ = pc_layer->p_list_slices[pc_mbQ->u_slice_idx]->p_header;

    mbQx = pc_mbQ->xL;
    mbQy = pc_mbQ->yL;

    filterOffsetA = pc_slice_headerQ->FilterOffsetA;
    filterOffsetB = pc_slice_headerQ->FilterOffsetB;

    qPp_luma = qPq_luma = (int16_t)(HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbQ) ? 0 : pc_mbQ->QPy);

    p = &p_codec->deblock.p[__lumaIdx];
    pf = &p_codec->deblock.pf[__lumaIdx];
    q = &p_codec->deblock.q[__lumaIdx];
    qf = &p_codec->deblock.qf[__lumaIdx];
    filterSamplesFlag = (int16_t (*)[16])&p_codec->deblock.filterSamplesFlag[__lumaIdx];
    filterSamplesFlag0 = &p_codec->deblock.filterSamplesFlag[__lumaIdx][0];
    filterSamplesFlag1 = &p_codec->deblock.filterSamplesFlag[__lumaIdx][1];

    // a. left vertical luma edge
    if (p_mb->deblock.filterLeftMbEdgeFlag) {
        pc_mbP = pc_layer->pp_list_macroblocks[pc_mbQ->u_addr - 1];
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 0/*Py*/, 0/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 4/*Py*/, 0/*Qx*/, 4/*Qy*/, __is_mb_edge_Yes, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 8/*Py*/, 0/*Qx*/, 8/*Qy*/, __is_mb_edge_Yes, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 12/*Py*/, 0/*Qx*/, 12/*Qy*/, __is_mb_edge_Yes, &__bS[1][1]);
        if (*((uint64_t*)__bS)) {
            qPp_luma = (int16_t)(HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbP) ? 0 : pc_mbP->QPy);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qPp_luma, qPq_luma, filterOffsetA, filterOffsetB, &indexA, &alpha, &beta);
            mbPx = pc_mbP->xL + 12;
            mbPy = pc_mbP->yL;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8(
                _pc_luma_sample, u_luma_stride, __bS,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (numSamplesToFilter0) {
                    if (__bS[0][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                            __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                }
                if (numSamplesToFilter1) {
                    if (__bS[1][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                            __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*filterSamplesFlag), (*pf), (*qf));
            }
        }
    }

    // b. internal vertical luma edges
    if (p_mb->deblock.filterInternalEdgesFlag == 1) {
        pc_mbP = pc_mbQ;
        qPp_luma = qPq_luma;
        hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
            qPp_luma, qPq_luma, filterOffsetA, filterOffsetB, &indexA, &alpha, &beta);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 0/*Py*/, 4/*Qx*/, 0/*Qy*/, __is_mb_edge_No, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 4/*Py*/, 4/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 8/*Py*/, 4/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 12/*Py*/, 4/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            /* i. #1 */
            mbPx = pc_mbP->xL;
            mbPy = pc_mbP->yL;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8(
                _pc_luma_sample, u_luma_stride, __bS,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (numSamplesToFilter0) {
                    if (__bS[0][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                            __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                }
                if (numSamplesToFilter1) {
                    if (__bS[1][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                            __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*filterSamplesFlag), (*pf), (*qf));
            }
        }

        /* ii. #2 */
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 0/*Py*/, 8/*Qx*/, 0/*Qy*/, __is_mb_edge_No, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 4/*Py*/, 8/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 8/*Py*/, 8/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 12/*Py*/, 8/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            mbPx = pc_mbP->xL + 4;
            mbPy = pc_mbP->yL;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8(
                _pc_luma_sample, u_luma_stride, __bS,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (numSamplesToFilter0) {
                    if (__bS[0][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                            __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                }
                if (numSamplesToFilter1) {
                    if (__bS[1][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                            __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*filterSamplesFlag), (*pf), (*qf));
            }
        }

        /* iii. #3 */
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 0/*Py*/, 12/*Qx*/, 0/*Qy*/, __is_mb_edge_No, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 4/*Py*/, 12/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 8/*Py*/, 12/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 12/*Py*/, 12/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            mbPx = pc_mbP->xL + 8;
            mbPy = pc_mbP->yL;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8(
                _pc_luma_sample, u_luma_stride, __bS,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (numSamplesToFilter0) {
                    if (__bS[0][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                            __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                }
                if (numSamplesToFilter1) {
                    if (__bS[1][0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                            __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                            __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*filterSamplesFlag), (*pf), (*qf));
            }
        }
    }

    //c. top horizontal luma edge
    if (p_mb->deblock.filterTopMbEdgeFlag == 1) {
        pc_mbP = pc_layer->pp_list_macroblocks[pc_mbQ->u_addr - pc_slice_headerQ->PicWidthInMbs];
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 12/*Py*/, 0/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 12/*Py*/, 4/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 12/*Py*/, 8/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 12/*Py*/, 12/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            qPp_luma = (int16_t)(HL_CODEC_264_MB_TYPE_IS_PCM(pc_mbP) ? 0 : pc_mbP->QPy);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qPp_luma, qPq_luma, filterOffsetA, filterOffsetB, &indexA, &alpha, &beta);
            mbPx = pc_mbP->xL;
            mbPy = pc_mbP->yL + 12;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8(
                _pc_luma_sample, u_luma_stride,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (__bS[0][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                        __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                if (__bS[1][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                        __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*pf), (*qf));
            }
        }
    }

    // d. internal horizontal luma edges
    if (p_mb->deblock.filterInternalEdgesFlag == 1) {
        pc_mbP = pc_mbQ;
        qPp_luma = qPq_luma;
        hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
            qPp_luma, qPq_luma, filterOffsetA, filterOffsetB, &indexA, &alpha, &beta);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 0/*Py*/, 0/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 0/*Py*/, 4/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 0/*Py*/, 8/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 0/*Py*/, 12/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            /* i. #1 */
            mbPx = pc_mbP->xL;
            mbPy = pc_mbP->yL;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8(
                _pc_luma_sample, u_luma_stride,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (__bS[0][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                        __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                if (__bS[1][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                        __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*pf), (*qf));
            }
        }

        /* ii. #2 */
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 4/*Py*/, 0/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 4/*Py*/, 4/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 4/*Py*/, 8/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 4/*Py*/, 12/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            mbPx = pc_mbP->xL;
            mbPy = pc_mbP->yL + 4;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8(
                _pc_luma_sample, u_luma_stride,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (__bS[0][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                        __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                if (__bS[1][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                        __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*pf), (*qf));
            }
        }

        /* iii. #3 */
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 8/*Py*/, 0/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[0][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 8/*Py*/, 4/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[0][1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 8/*Py*/, 8/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[1][0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 8/*Py*/, 12/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[1][1]);

        if (*((uint64_t*)__bS)) {
            mbPx = pc_mbP->xL;
            mbPy = pc_mbP->yL + 8;
            _pc_luma_sample = pc_luma_sample + mbPx + (mbPy * u_luma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8(
                _pc_luma_sample, u_luma_stride,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS[0], alpha, beta, (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS[1], alpha, beta, (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (__bS[0][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                        __chromaStyleFilteringFlag_Zero, __bS[0], indexA, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                if (__bS[1][0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                        __chromaStyleFilteringFlag_Zero, __bS[1], indexA, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                        __chromaStyleFilteringFlag_Zero, indexA, alpha, beta,
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8(
                    (uint8_t*)_pc_luma_sample, u_luma_stride, (*pf), (*qf));
            }
        }
    }

    return err;
}

HL_ASYNC_CALL_DIRECT
static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_deblock_avc_baseline_mb_chroma_u8(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb)
{
    int16_t indexA[2], alpha[2], beta[2], __bS[4/*2x2 index*/];

    uint8_t *pc_cb_sample, *pc_cr_sample, *_pc_cb_sample, *_pc_cr_sample;
    hl_codec_264_layer_t* pc_layer;
    uint32_t u_chroma_stride;
    int32_t mbPx, mbPy, mbQx, mbQy, filterOffsetA, filterOffsetB, qPI;
    int16_t qpP_chroma[2], qpQ_chroma[2];
    HL_ALIGNED(16) uint8_t (*p)[4][16], (*q)[4][16], (*pf)[3][16], (*qf)[3][16];
    HL_ALIGNED(16) int16_t (*filterSamplesFlag)[16], (*filterSamplesFlag0)[8], (*filterSamplesFlag1)[8], numSamplesToFilter, numSamplesToFilter0, numSamplesToFilter1;
    const hl_codec_264_mb_t *pc_mbP, *pc_mbQ;
    static hl_bool_t __is_mb_edge_Yes = HL_TRUE;
    static hl_bool_t __is_mb_edge_No = HL_FALSE;
    static int32_t __chromaStyleFilteringFlag_One = 1;
    static const int32_t __QPyZero = 0;
    static const int32_t __iCbCr_Zero = 0;
    static const int32_t __iCbCr_One = 1;
    const hl_codec_264_nal_slice_header_t *pc_slice_headerQ;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    pc_layer = p_codec->layers.pc_active;
    pc_cb_sample = (uint8_t *)pc_layer->pc_fs_curr->p_pict->pc_data_u;
    pc_cr_sample = (uint8_t *)pc_layer->pc_fs_curr->p_pict->pc_data_v;
    u_chroma_stride = pc_layer->pc_fs_curr->p_pict->uWidthC;

    pc_mbP = pc_mbQ = p_mb;
    pc_slice_headerQ = pc_layer->p_list_slices[pc_mbQ->u_slice_idx]->p_header;

    mbQx = pc_mbQ->xL;
    mbQy = pc_mbQ->yL;

    filterOffsetA = pc_slice_headerQ->FilterOffsetA;
    filterOffsetB = pc_slice_headerQ->FilterOffsetB;

    p = &p_codec->deblock.p[__chromaIdx];
    pf = &p_codec->deblock.pf[__chromaIdx];
    q = &p_codec->deblock.q[__chromaIdx];
    qf = &p_codec->deblock.qf[__chromaIdx];
    filterSamplesFlag = (int16_t (*)[16])&p_codec->deblock.filterSamplesFlag[__chromaIdx];
    filterSamplesFlag0 = &p_codec->deblock.filterSamplesFlag[__chromaIdx][0];
    filterSamplesFlag1 = &p_codec->deblock.filterSamplesFlag[__chromaIdx][1];

#define _COMPUTE_QP(suffix, iCbCr) \
	if (HL_CODEC_264_MB_TYPE_IS_PCM(pc_mb##suffix)) {  \
		qPI = HL_MATH_CLIP3(-pc_layer->p_list_slices[pc_mb##suffix->u_slice_idx]->p_header->pc_pps->pc_sps->QpBdOffsetC, 51, __QPyZero + pc_mb##suffix->qPOffset[(iCbCr)]);  \
        qp##suffix##_chroma[(iCbCr)] = qPI2QPC[qPI];  \
    }  \
    else { \
		qp##suffix##_chroma[(iCbCr)] = pc_mb##suffix->QPc[(iCbCr)]; \
    }
#define COMPUTE_QP(suffix) _COMPUTE_QP(suffix, __iCbCr_Zero) _COMPUTE_QP(suffix, __iCbCr_One)

    COMPUTE_QP(Q);

    // i. vertical chroma edge
    if (p_mb->deblock.filterLeftMbEdgeFlag == 1) {
        pc_mbP = pc_layer->pp_list_macroblocks[pc_mbQ->u_addr - 1];
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 0/*Py*/, 0/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 4/*Py*/, 0/*Qx*/, 4/*Qy*/, __is_mb_edge_Yes, &__bS[1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 8/*Py*/, 0/*Qx*/, 8/*Qy*/, __is_mb_edge_Yes, &__bS[2]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 12/*Py*/, 0/*Qx*/, 12/*Qy*/, __is_mb_edge_Yes, &__bS[3]);

        if (*((uint64_t*)__bS)) {
            COMPUTE_QP(P);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpP_chroma[__iCbCr_Zero], qpQ_chroma[__iCbCr_Zero], filterOffsetA, filterOffsetB, &indexA[__iCbCr_Zero], &alpha[__iCbCr_Zero], &beta[__iCbCr_Zero]);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpP_chroma[__iCbCr_One], qpQ_chroma[__iCbCr_One], filterOffsetA, filterOffsetB, &indexA[__iCbCr_One], &alpha[__iCbCr_One], &beta[__iCbCr_One]);
            mbPx = pc_mbP->xC + 4;
            mbPy = pc_mbP->yC;
            _pc_cb_sample = pc_cb_sample + mbPx + (mbPy * u_chroma_stride);
            _pc_cr_sample = pc_cr_sample + mbPx + (mbPy * u_chroma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8(
                _pc_cb_sample, _pc_cr_sample, u_chroma_stride, __bS,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS, alpha[__iCbCr_Zero], beta[__iCbCr_Zero], (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS, alpha[__iCbCr_One], beta[__iCbCr_One], (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (numSamplesToFilter0) {
                    if (__bS[0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                            __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_Zero], beta[__iCbCr_Zero],
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                            __chromaStyleFilteringFlag_One, indexA[__iCbCr_Zero], alpha[__iCbCr_Zero], beta[__iCbCr_Zero],
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                }
                if (numSamplesToFilter1) {
                    if (__bS[0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                            __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_One], beta[__iCbCr_One],
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                            __chromaStyleFilteringFlag_One, indexA[__iCbCr_One], alpha[__iCbCr_One], beta[__iCbCr_One],
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8(
                    _pc_cb_sample, _pc_cr_sample, u_chroma_stride, (*filterSamplesFlag), (*pf), (*qf));
            }
        }
    }

    // b. internal vertical chroma edges
    if (p_mb->deblock.filterInternalEdgesFlag == 1) {
        pc_mbP = pc_mbQ;
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 0/*Py*/, 8/*Qx*/, 0/*Qy*/, __is_mb_edge_No, &__bS[0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 4/*Py*/, 8/*Qx*/, 4/*Qy*/, __is_mb_edge_No, &__bS[1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 8/*Py*/, 8/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[2]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 12/*Py*/, 8/*Qx*/, 12/*Qy*/, __is_mb_edge_No, &__bS[3]);

        if (*((uint64_t*)__bS)) {
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpQ_chroma[__iCbCr_Zero], qpQ_chroma[__iCbCr_Zero], filterOffsetA, filterOffsetB, &indexA[__iCbCr_Zero], &alpha[__iCbCr_Zero], &beta[__iCbCr_Zero]);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpQ_chroma[__iCbCr_One], qpQ_chroma[__iCbCr_One], filterOffsetA, filterOffsetB, &indexA[__iCbCr_One], &alpha[__iCbCr_One], &beta[__iCbCr_One]);

            /* i. #1 */
            mbPx = pc_mbP->xC;
            mbPy = pc_mbP->yC;
            _pc_cb_sample = pc_cb_sample + mbPx + (mbPy * u_chroma_stride);
            _pc_cr_sample = pc_cr_sample + mbPx + (mbPy * u_chroma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8(
                _pc_cb_sample, _pc_cr_sample, u_chroma_stride, __bS,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS, alpha[__iCbCr_Zero], beta[__iCbCr_Zero], (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS, alpha[__iCbCr_One], beta[__iCbCr_One], (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (numSamplesToFilter0) {
                    if (__bS[0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                            __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_Zero], beta[__iCbCr_Zero],
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                            __chromaStyleFilteringFlag_One, indexA[__iCbCr_Zero], alpha[__iCbCr_Zero], beta[__iCbCr_Zero],
                            (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                    }
                }
                if (numSamplesToFilter1) {
                    if (__bS[0] < 4) {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                            __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_One], beta[__iCbCr_One],
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                    else {
                        hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                            &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                            __chromaStyleFilteringFlag_One, indexA[__iCbCr_One], alpha[__iCbCr_One], beta[__iCbCr_One],
                            (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                    }
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8(
                    _pc_cb_sample, _pc_cr_sample, u_chroma_stride, (*filterSamplesFlag), (*pf), (*qf));
            }
        }
    }

    // iii. top horizontal chroma edge
    if (p_mb->deblock.filterTopMbEdgeFlag == 1) {
        pc_mbP = pc_layer->pp_list_macroblocks[pc_mbQ->u_addr - pc_layer->p_list_slices[pc_mbQ->u_slice_idx]->p_header->PicWidthInMbs];
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 12/*Py*/, 0/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 12/*Py*/, 4/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 12/*Py*/, 8/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[2]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 12/*Py*/, 12/*Qx*/, 0/*Qy*/, __is_mb_edge_Yes, &__bS[3]);

        if (*((uint64_t*)__bS)) {
            COMPUTE_QP(P);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpP_chroma[__iCbCr_Zero], qpQ_chroma[__iCbCr_Zero], filterOffsetA, filterOffsetB, &indexA[__iCbCr_Zero], &alpha[__iCbCr_Zero], &beta[__iCbCr_Zero]);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpP_chroma[__iCbCr_One], qpQ_chroma[__iCbCr_One], filterOffsetA, filterOffsetB, &indexA[__iCbCr_One], &alpha[__iCbCr_One], &beta[__iCbCr_One]);
            mbPx = pc_mbP->xC;
            mbPy = pc_mbP->yC + 4;
            _pc_cb_sample = pc_cb_sample + mbPx + (mbPy * u_chroma_stride);
            _pc_cr_sample = pc_cr_sample + mbPx + (mbPy * u_chroma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8(
                _pc_cb_sample, _pc_cr_sample, u_chroma_stride,
                (*p), (*q));


            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS, alpha[__iCbCr_Zero], beta[__iCbCr_Zero], (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS, alpha[__iCbCr_One], beta[__iCbCr_One], (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (__bS[0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                        __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_Zero], beta[__iCbCr_Zero],
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                        __chromaStyleFilteringFlag_One, indexA[__iCbCr_Zero], alpha[__iCbCr_Zero], beta[__iCbCr_Zero],
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                if (__bS[0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                        __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_One], beta[__iCbCr_One],
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                        __chromaStyleFilteringFlag_One, indexA[__iCbCr_One], alpha[__iCbCr_One], beta[__iCbCr_One],
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8(
                    _pc_cb_sample, _pc_cr_sample, u_chroma_stride, (*pf), (*qf));
            }
        }
    }

    // iv. internal horizontal chroma edge
    if (p_mb->deblock.filterInternalEdgesFlag == 1) {
        pc_mbP = pc_mbQ;
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 0/*Px*/, 4/*Py*/, 0/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[0]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 4/*Px*/, 4/*Py*/, 4/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[1]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 8/*Px*/, 4/*Py*/, 8/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[2]);
        hl_codec_264_deblock_avc_baseline_get_bs_luma4lines(
            p_codec, pc_mbP, pc_mbQ, 12/*Px*/, 4/*Py*/, 12/*Qx*/, 8/*Qy*/, __is_mb_edge_No, &__bS[3]);

        if (*((uint64_t*)__bS)) {
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpQ_chroma[__iCbCr_Zero], qpQ_chroma[__iCbCr_Zero], filterOffsetA, filterOffsetB, &indexA[__iCbCr_Zero], &alpha[__iCbCr_Zero], &beta[__iCbCr_Zero]);
            hl_codec_264_deblock_avc_baseline_get_indexA_alpha_and_beta_u8(
                qpQ_chroma[__iCbCr_One], qpQ_chroma[__iCbCr_One], filterOffsetA, filterOffsetB, &indexA[__iCbCr_One], &alpha[__iCbCr_One], &beta[__iCbCr_One]);
            mbPx = pc_mbP->xC;
            mbPy = pc_mbP->yC;
            _pc_cb_sample = pc_cb_sample + mbPx + (mbPy * u_chroma_stride);
            _pc_cr_sample = pc_cr_sample + mbPx + (mbPy * u_chroma_stride);
            hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8(
                _pc_cb_sample, _pc_cr_sample, u_chroma_stride,
                (*p), (*q));

            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][0], &(*q)[0][0], &(*p)[1][0], &(*q)[1][0], __bS, alpha[__iCbCr_Zero], beta[__iCbCr_Zero], (*filterSamplesFlag0));
            hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8(
                &(*p)[0][8], &(*q)[0][8], &(*p)[1][8], &(*q)[1][8], __bS, alpha[__iCbCr_One], beta[__iCbCr_One], (*filterSamplesFlag1));
            numSamplesToFilter = (numSamplesToFilter0 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag0))) + (numSamplesToFilter1 = HL_CODEC_264_DEBLOCK_COUNT_FILTER_SAMPLES_FLAGS((*filterSamplesFlag1)));

            if (numSamplesToFilter) {
                if (__bS[0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0],
                        __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_Zero], beta[__iCbCr_Zero],
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][0], &(*p)[1][0], &(*p)[2][0], &(*p)[3][0], &(*q)[0][0], &(*q)[1][0], &(*q)[2][0], &(*q)[3][0],
                        __chromaStyleFilteringFlag_One, indexA[__iCbCr_Zero], alpha[__iCbCr_Zero], beta[__iCbCr_Zero],
                        (*filterSamplesFlag0), &(*pf)[0][0], &(*pf)[1][0], &(*pf)[2][0], &(*qf)[0][0], &(*qf)[1][0], &(*qf)[2][0]);
                }
                if (__bS[0] < 4) {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_lt4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8],
                        __chromaStyleFilteringFlag_One, __bS, indexA[__iCbCr_One], beta[__iCbCr_One],
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }
                else {
                    hl_codec_264_deblock_avc_baseline_filter8samples0_bs_eq4_u8(
                        &(*p)[0][8], &(*p)[1][8], &(*p)[2][8], &(*p)[3][8], &(*q)[0][8], &(*q)[1][8], &(*q)[2][8], &(*q)[3][8],
                        __chromaStyleFilteringFlag_One, indexA[__iCbCr_One], alpha[__iCbCr_One], beta[__iCbCr_One],
                        (*filterSamplesFlag1), &(*pf)[0][8], &(*pf)[1][8], &(*pf)[2][8], &(*qf)[0][8], &(*qf)[1][8], &(*qf)[2][8]);
                }

                hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8(
                    _pc_cb_sample, _pc_cr_sample, u_chroma_stride, (*pf), (*qf));
            }
        }
    }

    return err;
}


void (*hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8)(const uint8_t *pc_luma_samples, uint32_t u_luma_stride, int16_t bS[2][2], uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]) = hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8)(uint8_t *p_luma_samples, uint32_t u_luma_stride, int16_t filterSamplesFlag[16], const uint8_t pf[3/*p0,p1,p2,p3*/][16], uint8_t const qf[3/*q0,q1,q2,q3*/][16]) = hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8)(const uint8_t *pc_luma_samples, uint32_t u_luma_stride, uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]) = hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8)(uint8_t *pc_luma_samples, uint32_t u_luma_stride, const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*qf0,qf1,qf2*/][16]) = hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8)(const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride, uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]) = hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8)(uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride, const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*qf0,qf1,qf2*/][16]) = hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8)(const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride, int16_t bS[4], uint8_t p[4/*p0,p1,p2,p3*/][16], uint8_t q[4/*q0,q1,q2,q3*/][16]) = hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8)(uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride, int16_t filterSamplesFlag[16], const uint8_t pf[3/*pf0,pf1,pf2*/][16], const uint8_t qf[3/*q0,q1,q2,q3*/][16]) = hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8)(uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1, int16_t bS[2], int16_t alpha, int16_t beta, HL_OUT int16_t filterSamplesFlag[8]) = hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8)(uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1, int16_t bS[4], int16_t alpha, int16_t beta, HL_OUT int16_t filterSamplesFlag[8]) = hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, int16_t bS[2], int16_t indexA, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2) = hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, int16_t bS[4], int16_t indexA, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2) = hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8)(const uint8_t p0, const uint8_t p1, const uint8_t p2, const uint8_t p3, const uint8_t q0, const uint8_t q1, const uint8_t q2, const uint8_t q3, int32_t chromaStyleFilteringFlag, int16_t indexA, int16_t alpha, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2) = hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3, int16_t alpha, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2) = hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_cpp;
void (*hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8)(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2) = hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_cpp;


HL_ERROR_T hl_codec_264_deblock_init_funcs()
{
    HL_DEBUG_INFO("Initializing deblocking filter functions");

    hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8 = hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8 =  hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8 = hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8 = hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8 = hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8 = hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8 = hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8 = hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8 = hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8 = hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_cpp;

    hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8 = hl_codec_264_deblock_avc_baseline_load_pq_vert_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8 = hl_codec_264_deblock_avc_baseline_store_pfqf_vert_luma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8 = hl_codec_264_deblock_avc_baseline_load_pq_vert_chroma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8 = hl_codec_264_deblock_avc_baseline_store_pfqf_vert_chroma_u8_cpp;
    hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8 = hl_codec_264_deblock_avc_baseline_filter1samples_bs_eq4_u8_cpp;

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_intrin_sse2;
        hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_intrin_sse2;
        hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_intrin_sse2;
        hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_intrin_sse2;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_intrin_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE3)) {
        hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_intrin_sse3;
        hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_intrin_sse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSSE3)) {
        hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_intrin_ssse3;
        hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_intrin_ssse3;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_intrin_ssse3;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_intrin_ssse3;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_intrin_ssse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_intrin_sse4;
        hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_intrin_sse4;
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        extern void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse2(const uint8_t *pc_luma_samples, uint32_t u_luma_stride, HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16]);
        extern void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_asm_sse2(const uint8_t *pc_cb_samples, const uint8_t *pc_cr_samples, uint32_t u_chroma_stride, HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16]);
        extern void hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_asm_sse2(uint8_t *pc_luma_samples, uint32_t u_luma_stride, HL_ALIGNED(16) const uint8_t pf[3/*pf0,pf1,pf2*/][16], HL_ALIGNED(16) const uint8_t qf[3/*qf0,qf1,qf2*/][16]);
        extern void hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_asm_sse2(uint8_t *pc_cb_samples, uint8_t *pc_cr_samples, uint32_t u_chroma_stride, HL_ALIGNED(16) const uint8_t pf[3/*pf0,pf1,pf2*/][16], HL_ALIGNED(16) const uint8_t qf[3/*qf0,qf1,qf2*/][16]);
        extern void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_asm_sse2(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);

        hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse2;
        hl_codec_264_deblock_avc_baseline_load_pq_horiz_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_chroma_u8_asm_sse2;
        hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_luma_u8_asm_sse2;
        hl_codec_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_store_pfqf_horiz_chroma_u8_asm_sse2;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_chroma_u8_asm_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE3)) {
        extern void hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse3(const uint8_t *pc_luma_samples, uint32_t u_luma_stride, HL_ALIGNED(16) uint8_t p[4/*p0,p1,p2,p3*/][16], HL_ALIGNED(16) uint8_t q[4/*q0,q1,q2,q3*/][16]);
        hl_codec_264_deblock_avc_baseline_load_pq_horiz_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_load_pq_horiz_luma_u8_asm_sse3; // FIXME
    }
    if (hl_cpu_flags_test(kCpuFlagSSSE3)) {
        extern void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_asm_ssse3(uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1, int16_t bS[2], int16_t alpha, int16_t beta, HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8]);
        extern void hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_asm_ssse3(uint8_t *p0, uint8_t *q0, uint8_t *p1, uint8_t *q1, int16_t bS[4], int16_t alpha, int16_t beta, HL_ALIGNED(16) HL_OUT int16_t filterSamplesFlag[8]);
        extern void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_asm_ssse3(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, int16_t bS[2], int16_t indexA, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);
        extern void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_asm_ssse3(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, int16_t bS[4], int16_t indexA, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);
        extern void hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_asm_ssse3(const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3, const uint8_t *q0, const uint8_t *q1, const uint8_t *q2, const uint8_t *q3, int16_t alpha, int16_t beta, HL_OUT uint8_t *pf0, HL_OUT uint8_t *pf1, HL_OUT uint8_t *pf2, HL_OUT uint8_t *qf0, HL_OUT uint8_t *qf1, HL_OUT uint8_t *qf2);

        hl_codec_264_deblock_avc_baseline_get_threshold8samples_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_luma_u8_asm_ssse3;
        hl_codec_264_deblock_avc_baseline_get_threshold8samples_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_get_threshold8samples_chroma_u8_asm_ssse3;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_luma_u8_asm_ssse3;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_lt4_chroma_u8_asm_ssse3;
        hl_codec_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8 = hl_codec_x86_264_deblock_avc_baseline_filter8samples_bs_eq4_luma_u8_asm_ssse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
    }
#endif /* HL_HAVE_X86_ASM */

    return HL_ERROR_SUCCESS;
}