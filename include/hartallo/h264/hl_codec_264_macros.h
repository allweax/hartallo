#ifndef _HARTALLO_CODEC_264_MACROS_H_
#define _HARTALLO_CODEC_264_MACROS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_math.h"

HL_BEGIN_DECLS

// 5.7 Mathematical functions
// you should use "hl264_InverseRasterScan16_4x4" or "hl264_InverseRasterScan16_2x2" if possible
#define InverseRasterScan(index, width, height, pitch, y_dim)\
	((y_dim)==0 ? ((index)%((pitch)/(width)))*(width) : ((index)/((pitch)/(width)))*(height))

// Both "width" and "height" are power of 2 and less than "256"
#define InverseRasterScan_Pow2(index, width, height, pitch, y_dim)\
	((y_dim)==0 ? ((index)%((pitch)>>hlMath_Pow2TrailingZeros[(width)])) << hlMath_Pow2TrailingZeros[(width)] : ((index)/((pitch)>>hlMath_Pow2TrailingZeros[(width)]))<<hlMath_Pow2TrailingZeros[(height)])

// "pitch", "width" and "height" are power of 2 and less than "256"
#define InverseRasterScan_Pow2Full(index, width, height, pitch, y_dim)\
	((y_dim)==0 ? (HL_MATH_MOD_POW2_INT32((index), ((pitch)>>hlMath_Pow2TrailingZeros[(width)]))) << hlMath_Pow2TrailingZeros[(width)] \
		: ((index)>>hlMath_Pow2TrailingZeros[((pitch)>>hlMath_Pow2TrailingZeros[(width)])])<<hlMath_Pow2TrailingZeros[(height)])

// (7-1)
#define IdrPicFlag(e_nal_type)	((e_nal_type) == HL_CODEC_264_NAL_TYPE_CODED_SLICE_OF_AN_IDR_PICTURE)

// pseudo-code (8-1)
#define PicOrderCnt( \
		/*hl_codec_264_dpb_fs_t**/ _p_fs_, \
		/*hl_codec_264_poc_t**/ _p_poc_, \
		/*int32_t***/ _poc_ \
	){ \
	if (((_p_fs_)->RefType & HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM) == HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM || ((_p_fs_)->RefType & HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM) == HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM){ \
		*(_poc_) = HL_MATH_MIN((_p_poc_)->TopFieldOrderCnt, (_p_poc_)->BottomFieldOrderCnt); \
	} \
	else if (((_p_fs_)->RefType & HL_CODEC_264_REF_TYPE_TOP_USED) == HL_CODEC_264_REF_TYPE_TOP_USED) { \
		*(_poc_) = (_p_poc_)->TopFieldOrderCnt; \
	} \
	else if (((_p_fs_)->RefType & HL_CODEC_264_REF_TYPE_BOTTOM_USED) == HL_CODEC_264_REF_TYPE_BOTTOM_USED) { \
		*(_poc_) = (_p_poc_)->BottomFieldOrderCnt; \
	} \
	else { \
		*(_poc_) = 0; \
	} \
}

// pseudo-code (8-2)
#define DiffPicOrderCnt( \
	/*hl_codec_264_dpb_fs_t**/ _p_fsA_, \
	/*hl_codec_264_dpb_fs_t**/ _p_fsB_, \
	/*hl_codec_264_poc_t**/ _p_poc_, \
	/*int32_t***/ _poc_ \
){ \
	int32_t _pocA_, _pocB_; \
	PicOrderCnt((_p_fsA_), (_p_poc_), &(_pocA_)); \
	PicOrderCnt((_p_fsB_), (_p_poc_), &(_pocB_)); \
	*(_poc_) = (_pocA_) - (_pocB_); \
}

// (8-16)
// u_slice_group_id = p_slice_header->MbToSliceGroupMap[CurrMbAddr];
#define NextMbAddress(u_mb_addr_curr, u_slice_group_id, p_slice_header) \
	(u_mb_addr_curr) += 1; \
	while ((u_mb_addr_curr) < p_slice_header->PicSizeInMbs && \
	p_slice_header->MbToSliceGroupMap[(u_mb_addr_curr)] != (u_slice_group_id)) \
	++(u_mb_addr_curr);

// (8-318)
#define normAdjust4x4(_m_, _i_, _j_) \
	((((_i_)&1)==0 && ((_j_)&1)==0) ? HL_CODEC_264_SCALING_MATRIX_V[(_m_)][0] : ( (((_i_)&1)==1 && ((_j_)&1)==1) ? HL_CODEC_264_SCALING_MATRIX_V[(_m_)][1] : HL_CODEC_264_SCALING_MATRIX_V[(_m_)][2] ))

// 8.4.2.2.1 Luma sample interpolation process
#define Tap6Filter(E, F, G, H, I, J) ((E) - 5*((F) + (I)) + 20*((G) + (H)) + (J))

// Gets MbAddr from (X,Y)
#define HL_CODEC_264_GET_MB_ADDR(X, Y, PicWidthInMbs) (((Y) * PicWidthInMbs) + (X))

// IMPORTANT: "hl264Mb_IsNotAvail()" and "hl264Mb_IsAvail" must be used carefully
// "MB_IDX" should be computed using derivation process from "utils"
// Should not be used to check availability for A, B, C and D -> use "(MB)->Neighbours.mbAddrA" if "6.4.7"
// FIXME: not thread-safe, because of "pc_slice_curr"
#define HL_CODEC_264_MB_IS_NOT_AVAIL(U_MB_IDX, P_CODEC, P_CURR_SLICE_HDR) \
	(HL_MATH_IS_NEGATIVE_INT32(U_MB_IDX) || (!(P_CODEC)->layers.pc_active->pp_list_macroblocks[(U_MB_IDX)] || (P_CODEC)->layers.pc_active->pp_list_macroblocks[(U_MB_IDX)]->l_slice_id != (P_CURR_SLICE_HDR)->l_id))
#define HL_CODEC_264_MB_IS_AVAIL(U_MB_IDX, P_CODEC) (!HL_CODEC_264_MB_IS_NOT_AVAIL((MB_IDX), (P_CODEC)))

#define HL_CODEC_264_REF_TYPE_IS_USED(__refType__) \
	((__refType__) & HL_CODEC_264_REF_TYPE_USED)
#define HL_CODEC_264_REF_TYPE_IS_UNUSED(__refType__) \
	((__refType__) == HL_CODEC_264_REF_TYPE_UNUSED)
#define HL_CODEC_264_REF_TYPE_IS_USED_FOR_LONG_TERM(__refType__) \
	((__refType__) & HL_CODEC_264_REF_TYPE_USED_FOR_LONG_TERM)
#define HL_CODEC_264_REF_TYPE_IS_USED_FOR_SHORT_TERM(__refType__) \
	((__refType__) & HL_CODEC_264_REF_TYPE_USED_FOR_SHORT_TERM)
#define HL_CODEC_264_REF_TYPE_IS_USED_FOR_BASE_REF(__refType__) \
	((__refType__) & HL_CODEC_264_REF_TYPE_USED_FOR_SVC_BASE_REF)
#define HL_CODEC_264_REF_TYPE_IS_BOTTOM_USED(__refType__) \
	((__refType__) & HL_CODEC_264_REF_TYPE_BOTTOM_USED)
#define HL_CODEC_264_REF_TYPE_IS_TOP_USED(__refType__) \
	((__refType__) & HL_CODEC_264_REF_TYPE_TOP_USED)


#define IsSliceHeaderI(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_I)
#define IsSliceHeaderEI(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_EI)
#define IsSliceHeaderSI(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_SI)
#define IsSliceHeaderP(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_P)
#define IsSliceHeaderEP(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_EP)
#define IsSliceHeaderSP(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_SP)
#define IsSliceHeaderB(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_B)
#define IsSliceHeaderEB(p_header) ((p_header)->SliceTypeModulo5 == HL_CODEC_264_SLICE_TYPE_EB)

#define hl_codec_264_intra_luma_neighbouring_samples4x4_i(_x_,_y_) ((_x_) == -1 ? (_y_)+1 : (_x_)+5)
#define hl_codec_264_intra_luma_neighbouring_samples4x4_p(_p_,_x_,_y_)(_p_)[hl_codec_264_intra_luma_neighbouring_samples4x4_i((_x_),(_y_))]

#define hl_codec_264_intra_luma_neighbouring_samples16x16_i(_x_,_y_) ((_x_) == -1 ? (_y_)+1 : (_x_)+17)
#define hl_codec_264_intra_luma_neighbouring_samples16x16_p(_p_,_x_,_y_)(_p_)[hl_codec_264_intra_luma_neighbouring_samples16x16_i((_x_),(_y_))]

//FIXME: Not correct if _MbWidthC_#_MbHeigthC_
/*#define hl_codec_264_intra_chroma_neighbouring_samples_i(_MbWidthC_,_x_,_y_) ((_x_) == -1 ? (_y_)+1 : (_x_)+(_MbWidthC_)+1)
#define hl_codec_264_intra_chroma_neighbouring_samples_p(_p_,_MbWidthC_,_x_,_y_)(_p_)[hl_codec_264_intra_chroma_neighbouring_samples_i((_MbWidthC_),(_x_),(_y_))]
#define hl_codec_264_intra_chroma_neighbouring_samples_pCr(_p_,_MbWidthC_,_x_,_y_)	hl_codec_264_intra_chroma_neighbouring_samples_p(_p_,_MbWidthC_,_x_,_y_)
#define hl_codec_264_intra_chroma_neighbouring_samples_pCb(_p_,_MbWidthC_,_x_,_y_)	hl_codec_264_intra_chroma_neighbouring_samples_p(_p_,_MbWidthC_,_x_,_y_)*/
#define hl_codec_264_intra_chroma_neighbouring_samples_i(_MbWidthC_,_x_,_y_) ((_x_) == -1 ? (_y_)+1 : (_x_)+(_MbWidthC_)+1)
#define hl_codec_264_intra_chroma_neighbouring_samples_p(_p_,_MbWidthC_,_x_,_y_)(_p_)[hl_codec_264_intra_chroma_neighbouring_samples_i((_MbWidthC_),(_x_),(_y_))]

#define hl_codec_264_is_baseline(pc_sps) \
	((pc_sps)->profile_idc == HL_CODEC_264_PROFILE_BASELINE || (pc_sps)->profile_idc == HL_CODEC_264_PROFILE_BASELINE_SVC)

#define hl_codec_264_get_mem_blocks(p_self) \
	(((p_self)->threads.u_list_tasks_count > 1) ? p_codec->threads.pp_list_mem_blocks[(hl_thread_get_core_id() % (p_self)->threads.u_list_tasks_count)] : (p_self)->pobj_mem_blocks)

HL_END_DECLS


#endif /* _HARTALLO_CODEC_264_MACROS_H_ */
