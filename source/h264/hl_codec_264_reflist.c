#include "hartallo/h264/hl_codec_264_reflist.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_dpb.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_list.h"
#include "hartallo/hl_debug.h"

//FIXME: is qsort() fast? -> replace and check

typedef int (*RefListComparator_f)(const void*, const void*);
static int _RefListSort_8_2_4_2_1(const void* pic1, const void* pic2);
static int _RefListSort_8_2_4_2_2_ByFrameNumWrap(const void* pic1, const void* pic2);
static int _RefListSort_8_2_4_2_2_ByLongTermFrameIdx(const void* pic1, const void* pic2);
static HL_ERROR_T _RefListProcessModification(hl_codec_264_t* p_codec, hl_bool_t isBSlice);

// 8.2.4 Decoding process for reference picture lists construction
// Subclause 8.2.4.1 is invoked to specify
// – the assignment of variables FrameNum, FrameNumWrap, and PicNum to each of the short-term reference pictures,
// and
// – the assignment of variable LongTermPicNum to each of the long-term reference pictures.
HL_ERROR_T hl_codec_264_reflist_init(hl_codec_264_t* p_codec)
{
    int32_t i,j,count;
    hl_bool_t hasAtLeastOneUsedRef = HL_FALSE;
    hl_bool_t isCodedFrameSlice;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    isCodedFrameSlice = (p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag == 0);

    // For I slice => clear RefList0 and RefList1
    if (IsSliceHeaderI(p_codec->layers.pc_active->pc_slice_hdr)) { //FIXME:
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->RefPicList0, p_codec->pc_poc->RefPicList0Count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->RefPicList1, p_codec->pc_poc->RefPicList1Count);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->refFrameList0ShortTerm, p_codec->pc_poc->refFrameList0ShortTermCount);
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->refFrameList0LongTerm, p_codec->pc_poc->refFrameList0LongTermCount);
        //? return HL_ERROR_SUCCESS;
    }

    // Compute FrameNum and FrameNumWrap as per (8-27) pseudo code
    for (i = 0; i < (int32_t)p_codec->pc_dpb->u_list_fs_count; ++i) {
        // It's short term and is used as reference picture
        if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
            if (p_codec->pc_dpb->p_list_fs[i]->FrameNum > (int32_t)p_codec->layers.pc_active->pc_slice_hdr->frame_num) {
                p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap = p_codec->pc_dpb->p_list_fs[i]->FrameNum - p_codec->pc_poc->MaxFrameNum;
            }
            else {
                p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap = p_codec->pc_dpb->p_list_fs[i]->FrameNum;
            }
        }
    }

    // Compute PicNum and LongTermPicNum as per (8-28) to (8-33)
    for (i = 0; i < (int32_t)p_codec->pc_dpb->u_list_fs_count; ++i) {
        // short term (Bottom or Top)
        if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
            if (p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag == 0) {
                p_codec->pc_dpb->p_list_fs[i]->PicNum = p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap;
            }
            else {
                // FIXME: party check => is it right?
                if (p_codec->layers.pc_active->pc_slice_hdr->bottom_field_flag && HL_CODEC_264_REF_TYPE_IS_BOTTOM_USED(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
                    p_codec->pc_dpb->p_list_fs[i]->PicNum = 2 * p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap + 1;
                }
                else {
                    p_codec->pc_dpb->p_list_fs[i]->PicNum = 2 * p_codec->pc_dpb->p_list_fs[i]->FrameNumWrap;
                }
            }
        }
        // long term (Bottom or Top)
        else if (HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
            if (p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag == 0) {
                p_codec->pc_dpb->p_list_fs[i]->LongTermPicNum = p_codec->pc_dpb->p_list_fs[i]->LongTermFrameIdx;
            }
            else {
                // FIXME: party check => is it right?
                if (p_codec->layers.pc_active->pc_slice_hdr->bottom_field_flag && HL_CODEC_264_REF_TYPE_IS_BOTTOM_USED(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
                    p_codec->pc_dpb->p_list_fs[i]->LongTermPicNum = 2 * p_codec->pc_dpb->p_list_fs[i]->LongTermFrameIdx + 1;
                }
                else {
                    p_codec->pc_dpb->p_list_fs[i]->LongTermPicNum = 2 * p_codec->pc_dpb->p_list_fs[i]->LongTermFrameIdx;
                }
            }
        }

        if (!hasAtLeastOneUsedRef && HL_CODEC_264_REF_TYPE_IS_USED(p_codec->pc_dpb->p_list_fs[i]->RefType) && !p_codec->pc_dpb->p_list_fs[i]->NonExisting) {
            hasAtLeastOneUsedRef = HL_TRUE;
        }
    }

    if (hasAtLeastOneUsedRef) {
        hl_bool_t isBSlice = HL_FALSE;
        // P or SP slices
        if (IsSliceHeaderP(p_codec->layers.pc_active->pc_slice_hdr) || IsSliceHeaderSP(p_codec->layers.pc_active->pc_slice_hdr)) {
            if (isCodedFrameSlice) {
                HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->RefPicList0, p_codec->pc_poc->RefPicList0Count);
                // 8.2.4.2.1 Initialisation process for the reference picture list for P and SP slices in frames
                // This initialisation process is invoked when decoding a P or SP slice in a coded frame.
                for (i = 0; i < (int32_t)p_codec->pc_dpb->u_list_fs_count; ++i) {
                    if (!p_codec->pc_dpb->p_list_fs[i]->NonExisting && HL_CODEC_264_REF_TYPE_IS_USED(p_codec->pc_dpb->p_list_fs[i]->RefType)) {
                        if (p_codec->pc_poc->RefPicList0Count+1 < HL_CODEC_264_REFPICT_LIST0_MAX_COUNT) {
                            p_codec->pc_poc->RefPicList0[p_codec->pc_poc->RefPicList0Count++] = (hl_codec_264_dpb_fs_t*)hl_object_ref(HL_OBJECT(p_codec->pc_dpb->p_list_fs[i]));
                        }
                    }
                }
                if (p_codec->pc_poc->RefPicList0Count > 1) {
                    qsort(&p_codec->pc_poc->RefPicList0,
                          p_codec->pc_poc->RefPicList0Count,
                          sizeof(struct hl264Pic_s*),
                          _RefListSort_8_2_4_2_1);
                }
            }
            else {
                // 8.2.4.2.2 Initialisation process for the reference picture list for P and SP slices in fields
                // This initialisation process is invoked when decoding a P or SP slice in a coded field.
                HL_DEBUG_ERROR("Not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
#if 0
                HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->refFrameList0ShortTerm);
                HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->refFrameList0LongTerm);

                // A. First fill "refFrameList0ShortTerm" and "refFrameList0LongTerm"
                for(i = 0; i < p_codec->pc_poc->RefPicList0Count; ++i) {
                    if(p_codec->pc_poc->RefPicList0[i]->RefType & HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM) {
                        if(p_codec->pc_poc->refFrameList0ShortTermCount+1 < HL_CODEC_264_REFPICT_LIST0_MAX_COUNT) {
                            p_codec->pc_poc->refFrameList0ShortTerm[p_codec->pc_poc->refFrameList0ShortTermCount++] = (hl_codec_264_dpb_fs_t*)hl_object_ref(HL_OBJECT(p_codec->pc_poc->RefPicList0[i]));
                        }
                    }
                    else if(p_codec->pc_poc->RefPicList0[i]->RefType & HL_CODEC_264_REF_TYPE_UsedForLongTerm) {
                        if(p_codec->pc_poc->refFrameList0LongTermCount+1 < HL_CODEC_264_REFPICT_LIST0_MAX_COUNT) {
                            p_codec->pc_poc->refFrameList0LongTerm[p_codec->pc_poc->refFrameList0LongTermCount++] = (hl_codec_264_dpb_fs_t*)hl_object_ref(HL_OBJECT(p_codec->pc_poc->RefPicList0[i]));
                        }
                    }
                }
                // B. Sort "refFrameList0ShortTerm" by FrameNumWrap (Highest -> lowest)
                if(p_codec->pc_poc->refFrameList0ShortTermCount > 1)
                    qsort(p_codec->pc_poc->refFrameList0ShortTerm,
                          p_codec->pc_poc->refFrameList0ShortTermCount,
                          sizeof(hl_codec_264_dpb_fs_t*),
                          _RefListSort_8_2_4_2_2_ByFrameNumWrap);
                // C. Sort "refFrameList0LongTerm" by LongTermFrameIdx (Lowest -> highest)
                if(p_codec->pc_poc->refFrameList0LongTermCount > 1)
                    qsort(p_codec->pc_poc->refFrameList0LongTerm,
                          p_codec->pc_poc->refFrameList0LongTermCount,
                          sizeof(hl_codec_264_dpb_fs_t*),
                          _RefListSort_8_2_4_2_2_ByLongTermFrameIdx);
#endif
            }
        }

        // B slices
        else if (IsSliceHeaderB(p_codec->layers.pc_active->pc_slice_hdr)) { // B slices not supported for Base Profile
            isBSlice = HL_TRUE;
            if (isCodedFrameSlice) {
                // 8.2.4.2.3 Initialisation process for reference picture lists for B slices in frames
                // This initialisation process is invoked when decoding a B slice in a coded frame.
                HL_DEBUG_ERROR("Not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
            }
            else {
                // 8.2.4.2.4 Initialisation process for reference picture lists for B slices in fields
                // This initialisation process is invoked when decoding a B slice in a coded field.
                HL_DEBUG_ERROR("Not implemented yet");
                return HL_ERROR_NOT_IMPLEMENTED;
            }
        }

        // 8.2.4.3 Modification process for reference picture lists
        if (p_codec->layers.pc_active->pc_slice_hdr->ref_pic_list_modification_flag_l0 == 1) {
            err = _RefListProcessModification(p_codec, isBSlice);
        }
    }


    // When the number of entries in the initial RefPicList0 or RefPicList1 produced as specified in subclauses 8.2.4.2.1
    // through 8.2.4.2.5 is greater than num_ref_idx_l0_active_minus1 + 1 or num_ref_idx_l1_active_minus1 + 1,
    // respectively, the extra entries past position num_ref_idx_l0_active_minus1 or num_ref_idx_l1_active_minus1 are
    // discarded from the initial reference picture list.
    count = (int32_t)p_codec->pc_poc->RefPicList0Count - (p_codec->layers.pc_active->pc_slice_hdr->num_ref_idx_l0_active_minus1 + 1);
    if (count > 0) {
        for (i = (int32_t)p_codec->pc_poc->RefPicList0Count - 1,j=0; j<count; --i,++j) {
            HL_OBJECT_SAFE_FREE(p_codec->pc_poc->RefPicList0[i]);
        }
        p_codec->pc_poc->RefPicList0Count -= count;
    }
    count = (int32_t)p_codec->pc_poc->RefPicList1Count - (p_codec->layers.pc_active->pc_slice_hdr->num_ref_idx_l1_active_minus1 + 1);
    if (count > 0) {
        for (i = (int32_t)p_codec->pc_poc->RefPicList1Count - 1,j=0; j<count; ++j) {
            HL_OBJECT_SAFE_FREE(p_codec->pc_poc->RefPicList1[i]);
        }
        p_codec->pc_poc->RefPicList1Count -= count;
    }

    return err;
}

// 8.2.4.2.1 Initialisation process for the reference picture list for P and SP slices in frames
// ShortTerms first then longTerms
// ShortTerms are ordered from highest PicNum to lowest PicNum
// LongTerms are ordered from Lowest LongTermPicNum to highest LongTermPicNum
static int _RefListSort_8_2_4_2_1(const void* _pic1, const void* _pic2)
{
    hl_codec_264_dpb_fs_t* pic1 = *((hl_codec_264_dpb_fs_t**)_pic1);
    hl_codec_264_dpb_fs_t* pic2 = *((hl_codec_264_dpb_fs_t**)_pic2);
    hl_bool_t isPic1ShortTerm = HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(pic1->RefType);
    hl_bool_t isPic2ShortTerm = HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(pic2->RefType);

    if (isPic1ShortTerm && isPic2ShortTerm) {
        return -(pic1->PicNum - pic2->PicNum);
    }
    if (!isPic1ShortTerm && !isPic2ShortTerm) {
        return (pic1->LongTermPicNum - pic2->LongTermPicNum);
    }
    return isPic1ShortTerm ? -1 : +1;
}

// 8.2.4.2.2 Initialisation process for the reference picture list for P and SP slices in fields
// From highest FrameNumWrap to lowest
static int _RefListSort_8_2_4_2_2_ByFrameNumWrap(const void* _pic1, const void* _pic2)
{
    hl_codec_264_dpb_fs_t* pic1 = *((hl_codec_264_dpb_fs_t**)_pic1);
    hl_codec_264_dpb_fs_t* pic2 = *((hl_codec_264_dpb_fs_t**)_pic2);
    return -(pic1->FrameNumWrap - pic2->FrameNumWrap);
}

// 8.2.4.2.2 Initialisation process for the reference picture list for P and SP slices in fields
// From lowest LongTermFrameIdx to highest
static int _RefListSort_8_2_4_2_2_ByLongTermFrameIdx(const void* _pic1, const void* _pic2)
{
    hl_codec_264_dpb_fs_t* pic1 = *((hl_codec_264_dpb_fs_t**)_pic1);
    hl_codec_264_dpb_fs_t* pic2 = *((hl_codec_264_dpb_fs_t**)_pic2);
    return (pic1->LongTermFrameIdx - pic2->LongTermFrameIdx);
}

// 8.2.4.3 Modification process for reference picture lists
static HL_ERROR_T _RefListProcessModification(hl_codec_264_t* p_codec, hl_bool_t isBSlice)
{
    int32_t refIdxLX,nIdx,i,picNumLXPred,picNumLX,cIdx,RefPicListXCount;
    uint32_t *modification_of_pic_nums_idc_lX, *abs_diff_pic_num_minus1_lX, *long_term_pic_num_lX, num_ref_idx_lX_active_minus1,ucIdx;
    hl_codec_264_dpb_fs_t *tmpPic;
    HL_ERROR_T ret = HL_ERROR_SUCCESS;
    hl_codec_264_dpb_fs_t* tmpRefPicListX[HL_CODEC_264_REFPICT_LIST0_MAX_COUNT] = {0};//tmp list used to takle refCounting issue
    hl_codec_264_dpb_fs_t** RefPicListX;

    refIdxLX = 0;
    picNumLXPred = p_codec->pc_poc->CurrPicNum;

    if (isBSlice) {
        modification_of_pic_nums_idc_lX = p_codec->layers.pc_active->pc_slice_hdr->modification_of_pic_nums_idc_l1;
        abs_diff_pic_num_minus1_lX = p_codec->layers.pc_active->pc_slice_hdr->abs_diff_pic_num_minus1_l1;
        num_ref_idx_lX_active_minus1 = p_codec->layers.pc_active->pc_slice_hdr->num_ref_idx_l1_active_minus1;
        long_term_pic_num_lX = p_codec->layers.pc_active->pc_slice_hdr->long_term_pic_num_l1;
        RefPicListXCount = (int32_t)p_codec->pc_poc->RefPicList1Count;
        RefPicListX = p_codec->pc_poc->RefPicList1;
        for (cIdx=0; cIdx<RefPicListXCount; ++cIdx) {
            tmpRefPicListX[cIdx]=p_codec->pc_poc->RefPicList1[cIdx];
        }
    }
    else {
        modification_of_pic_nums_idc_lX = p_codec->layers.pc_active->pc_slice_hdr->modification_of_pic_nums_idc_l0;
        abs_diff_pic_num_minus1_lX = p_codec->layers.pc_active->pc_slice_hdr->abs_diff_pic_num_minus1_l0;
        num_ref_idx_lX_active_minus1 = p_codec->layers.pc_active->pc_slice_hdr->num_ref_idx_l0_active_minus1;
        long_term_pic_num_lX = p_codec->layers.pc_active->pc_slice_hdr->long_term_pic_num_l0;
        RefPicListXCount = (int32_t)p_codec->pc_poc->RefPicList0Count;
        RefPicListX=p_codec->pc_poc->RefPicList0;
        for (cIdx=0; cIdx<RefPicListXCount; ++cIdx) {
            tmpRefPicListX[cIdx]=p_codec->pc_poc->RefPicList0[cIdx];
        }
    }


    for (i=0; i<HL_CODEC_264_REF_PIC_MARKING_MAX_COUNT && modification_of_pic_nums_idc_lX[i] != 3; ++i) {
        if (modification_of_pic_nums_idc_lX[i]==0 || modification_of_pic_nums_idc_lX[i]==1) {
            // 8.2.4.3.1 Modification process of reference picture lists for short-term reference pictures
            int32_t picNumLXNoWrap;
            //(8-34)
            if (modification_of_pic_nums_idc_lX[i] == 0) {
                if (picNumLXPred - ((int32_t)abs_diff_pic_num_minus1_lX[i] + 1) < 0) {
                    picNumLXNoWrap = picNumLXPred - ((int32_t)abs_diff_pic_num_minus1_lX[i] + 1) + (int32_t)p_codec->pc_poc->MaxPicNum;
                }
                else {
                    picNumLXNoWrap = picNumLXPred - ((int32_t)abs_diff_pic_num_minus1_lX[i] + 1);
                }
            }
            //(8-35)
            else {
                if (picNumLXPred + ((int32_t)abs_diff_pic_num_minus1_lX[i] + 1) >= (int32_t)p_codec->pc_poc->MaxPicNum) {
                    picNumLXNoWrap = picNumLXPred + ((int32_t)abs_diff_pic_num_minus1_lX[i] + 1) - (int32_t)p_codec->pc_poc->MaxPicNum;
                }
                else {
                    picNumLXNoWrap = picNumLXPred + ((int32_t)abs_diff_pic_num_minus1_lX[i] + 1);
                }
            }

            picNumLXPred = picNumLXNoWrap;

            //(8-36)
            // picNumLX shall be equal to the PicNum of a reference picture that is marked as "used for short-term reference" and
            // shall not be equal to the PicNum of a short-term reference picture that is marked as "non-existing".
            if (picNumLXNoWrap > (int32_t)p_codec->pc_poc->CurrPicNum) {
                picNumLX = picNumLXNoWrap - (int32_t)p_codec->pc_poc->MaxPicNum;
            }
            else {
                picNumLX = picNumLXNoWrap;
            }

            // (8-37)

            // short-term reference picture with PicNum equal to picNumLX
            tmpPic = HL_NULL;
            for (ucIdx=0; ucIdx<num_ref_idx_lX_active_minus1 + 1; ++ucIdx) {
                if (RefPicListX[ucIdx] && HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(RefPicListX[ucIdx]->RefType) && RefPicListX[ucIdx]->PicNum == picNumLX) {
                    tmpPic = RefPicListX[ucIdx];
                    break;
                }
            }
            if (!tmpPic) {
                HL_DEBUG_ERROR("Failed to find short term pic with PicNum=%d", picNumLX);
                ret = HL_ERROR_INVALID_BITSTREAM;//FIXME
                goto bail;
            }

            for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--) {
                tmpRefPicListX[cIdx] = tmpRefPicListX[cIdx - 1];
            }
            tmpRefPicListX[refIdxLX++] = tmpPic;
            nIdx = refIdxLX;

            // NOTE 1 – A value of MaxPicNum can never be equal to picNumLX.
#define PicNumF(_pic_) (((_pic_) && ((_pic_)->RefType & HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM)) ? (_pic_)->PicNum : p_codec->pc_poc->MaxPicNum)
            for (cIdx = refIdxLX; cIdx <= (int32_t)num_ref_idx_lX_active_minus1 + 1; cIdx++) {
                if (PicNumF(tmpRefPicListX[cIdx]) != picNumLX) {
                    tmpRefPicListX[nIdx++] = tmpRefPicListX[cIdx];
                }
            }
        }
        else if (modification_of_pic_nums_idc_lX[i]==2) {
            // 8.2.4.3.2 Modification process of reference picture lists for long-term reference pictures

            // (8-38)

            // long-term reference picture with LongTermPicNum equal to long_term_pic_num
            tmpPic = HL_NULL;
            for (ucIdx=0; ucIdx<num_ref_idx_lX_active_minus1 + 1; ++ucIdx) {
                if (RefPicListX[ucIdx] && HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(RefPicListX[ucIdx]->RefType) && RefPicListX[ucIdx]->LongTermPicNum == long_term_pic_num_lX[i]) {
                    tmpPic = RefPicListX[ucIdx];
                    break;
                }
            }
            if (!tmpPic) {
                HL_DEBUG_ERROR("Failed to find long term pic with LongTermPicNum=%d", long_term_pic_num_lX[i]);
                ret = HL_ERROR_INVALID_BITSTREAM;//FIXME
                goto bail;
            }

            for (cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--) {
                tmpRefPicListX[cIdx] = tmpRefPicListX[cIdx - 1];
            }
            tmpRefPicListX[refIdxLX++] = tmpPic;
            nIdx = refIdxLX;

#define LongTermPicNumF(_pic_) ( ((_pic_) && ((_pic_)->RefType & HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM)) ? (_pic_)->LongTermPicNum : (2*(p_codec->pc_poc->MaxLongTermFrameIdx + 1)) )
            for (cIdx = refIdxLX; cIdx <= (int32_t)num_ref_idx_lX_active_minus1 + 1; cIdx++) {
                if (LongTermPicNumF(tmpRefPicListX[cIdx]) != long_term_pic_num_lX[i]) {
                    tmpRefPicListX[nIdx++] = tmpRefPicListX[cIdx];
                }
            }
        }
    }

    // check that there is no gap
    for (ucIdx=0; ucIdx<num_ref_idx_lX_active_minus1 + 1; ucIdx++) {
        if (!tmpRefPicListX[ucIdx]) {
            HL_DEBUG_WARN("There are gaps!");
            if (ucIdx == 0) {
                ret = HL_ERROR_INVALID_BITSTREAM;
                goto bail;
            }
            num_ref_idx_lX_active_minus1 = ucIdx-1;
            break;
        }
    }
    //Increment refCounts
    for (ucIdx=0; ucIdx<num_ref_idx_lX_active_minus1 + 1; ucIdx++) {
        tmpRefPicListX[ucIdx] = (hl_codec_264_dpb_fs_t*)hl_object_ref((hl_object_t*)tmpRefPicListX[ucIdx]);
    }
    // Assign
    if (isBSlice) {
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->RefPicList1, p_codec->pc_poc->RefPicList1Count);
        for (ucIdx=0; ucIdx<num_ref_idx_lX_active_minus1 + 1; ucIdx++) {
            p_codec->pc_poc->RefPicList1[ucIdx]=tmpRefPicListX[ucIdx];
        }
        p_codec->pc_poc->RefPicList1Count=num_ref_idx_lX_active_minus1 + 1;
    }
    else {
        HL_LIST_STATIC_CLEAR_OBJECTS(p_codec->pc_poc->RefPicList0, p_codec->pc_poc->RefPicList0Count);
        for (ucIdx=0; ucIdx<num_ref_idx_lX_active_minus1 + 1; ucIdx++) {
            p_codec->pc_poc->RefPicList0[ucIdx]=tmpRefPicListX[ucIdx];
        }
        p_codec->pc_poc->RefPicList0Count=num_ref_idx_lX_active_minus1 + 1;
    }

bail:
    return ret;
}
