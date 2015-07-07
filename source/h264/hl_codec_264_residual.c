#include "hartallo/h264/hl_codec_264_residual.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_cavlc.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_defs.h"
#include "hartallo/h264/hl_codec_264_encode.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

static HL_ERROR_T _hl_codec_264_residual_read_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t (*i16x16DClevel)[16],
    int32_t (*i16x16AClevel)[16][16],
    int32_t (*level)[16][16],
    int32_t (*level8x8)[8][64],
    int32_t startIdx,
    int32_t endIdx);
static HL_ERROR_T _hl_codec_264_residual_read_block_cavlc(
    hl_codec_264_residual_inv_xt* p_inv,
    struct hl_codec_264_s* p_codec,
    struct hl_codec_264_mb_s* p_mb,
    int32_t *coeffLevels, // table with at least "maxNumCoeff" values
    int32_t startIdx,
    int32_t endIdx,
    uint32_t maxNumCoeff);
static HL_ERROR_T _hl_codec_264_residual_write_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t i16x16DClevel[16],
    int32_t i16x16AClevel[16][16],
    int32_t level[16][16],
    int32_t startIdx,
    int32_t endIdx);

// 7.3.5.3 Residual data syntax
// residual( startIdx, endIdx )
HL_ERROR_T hl_codec_264_residual_read(hl_codec_264_t* p_codec, hl_codec_264_mb_t* p_mb, int32_t startIdx, int32_t endIdx)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t _startIdx, _endIdx;
    const hl_codec_264_nal_sps_t* pc_sps = p_codec->sps.pc_active;
    hl_codec_264_residual_read_block_f read_block_f =  p_codec->pps.pc_active->entropy_coding_mode_flag
            ? HL_NULL /*FIXME:CABAC hl264Residual_read_block_f_CABAC */
            : _hl_codec_264_residual_read_block_cavlc;

    /* Decode Luma Blocks */

    // residual_luma()
    err = _hl_codec_264_residual_read_luma(
              p_codec,
              p_mb,
              &p_mb->Intra16x16DCLevel,
              &p_mb->Intra16x16ACLevel,
              &p_mb->LumaLevel,
              &p_mb->LumaLevel8x8,
              startIdx,
              endIdx);
    if (err) {
        return err;
    }

    /* Decode Chroma DC and AC Blocks */

    if (pc_sps->ChromaArrayType == 1 || pc_sps->ChromaArrayType == 2) {
        int32_t NumC8x8 = 4 >> (pc_sps->SubWidthC_TrailingZeros + pc_sps->SubHeightC_TrailingZeros);
        int32_t maxNumCoeff = (NumC8x8 << 2);
        int32_t iCbCr, i8x8;
        _endIdx = maxNumCoeff - 1;
        if ((p_mb->CodedBlockPatternChroma & 3) && startIdx == 0) {/* chroma DC residual present */
            hl_codec_264_residual_inv_xt invType = {0};
            invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL;
            // iCbCr = 0
            invType.i_iCbCr = 0;
            err = read_block_f(&invType, p_codec, p_mb, p_mb->ChromaDCLevel[invType.i_iCbCr], 0, _endIdx, maxNumCoeff);
            // iCbCr = 1
            invType.i_iCbCr = 1;
            err = read_block_f(&invType, p_codec, p_mb, p_mb->ChromaDCLevel[invType.i_iCbCr], 0, _endIdx, maxNumCoeff);
        }
        else {
            int32_t i;
            for (i = 0; i< NumC8x8; i+= 4) {
                p_mb->ChromaDCLevel[0][i] = p_mb->ChromaDCLevel[1][i] = 0;
                p_mb->ChromaDCLevel[0][i + 1] = p_mb->ChromaDCLevel[1][i + 1] = 0;
                p_mb->ChromaDCLevel[0][i + 2] = p_mb->ChromaDCLevel[1][i + 2] = 0;
                p_mb->ChromaDCLevel[0][i + 3] = p_mb->ChromaDCLevel[1][i + 3] = 0;
            }
        }

        if (p_mb->CodedBlockPatternChroma & 2) {/* Chroma AC residual present */
            hl_codec_264_residual_inv_xt invType = {0};
            int32_t i8x8x4;
            _startIdx = HL_MATH_MAX(0, startIdx - 1);
            _endIdx = endIdx - 1;
            invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL;
            for (iCbCr = 0; iCbCr < 2; ++iCbCr) {
                invType.i_iCbCr = iCbCr;
                for (i8x8 = 0; i8x8 < NumC8x8; ++i8x8) {
                    i8x8x4 = (i8x8 << 2);
                    invType.i_cbr4x4BlkIdx = 0;
                    err = read_block_f(&invType, p_codec, p_mb, p_mb->ChromaACLevel[iCbCr][i8x8x4], _startIdx, _endIdx, 15);
                    invType.i_cbr4x4BlkIdx = 1;
                    err = read_block_f(&invType, p_codec, p_mb, p_mb->ChromaACLevel[iCbCr][i8x8x4 + 1], _startIdx, _endIdx, 15);
                    invType.i_cbr4x4BlkIdx = 2;
                    err = read_block_f(&invType, p_codec, p_mb, p_mb->ChromaACLevel[iCbCr][i8x8x4 + 2], _startIdx, _endIdx, 15);
                    invType.i_cbr4x4BlkIdx = 3;
                    err = read_block_f(&invType, p_codec, p_mb, p_mb->ChromaACLevel[iCbCr][i8x8x4 + 3], _startIdx, _endIdx, 15);
                }
            }
        }
        else {
#if 1
            p_mb->CodedBlockPatternChromaAC4x4[0] = p_mb->CodedBlockPatternChromaAC4x4[1] = 0;
            memset(p_mb->ChromaACLevel, 0, sizeof(p_mb->ChromaACLevel)); // FIXME: remove
#else
            int32_t i4x4, i8x8x4, i;
            for (iCbCr = 0; iCbCr < 2; ++iCbCr) {
                for (i8x8 = 0; i8x8 < NumC8x8; ++i8x8) {
                    i8x8x4 = (i8x8 << 2);
                    for (i4x4 = 0; i4x4 < 4; ++i4x4) {
                        i = i8x8x4 + i4x4;
                        p_mb->ChromaACLevel[iCbCr][i][0] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][1] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][2] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][3] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][4] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][5] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][6] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][7] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][8] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][9] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][10] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][11] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][12] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][13] = 0;
                        p_mb->ChromaACLevel[iCbCr][i][14] = 0;
                    }
                }
            }
#endif
        }
    }
    else if (pc_sps->ChromaArrayType == 3) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return err;
}

static HL_ERROR_T _hl_codec_264_residual_read_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t (*i16x16DClevel)[16],
    int32_t (*i16x16AClevel)[16][16],
    int32_t (*level)[16][16],
    int32_t (*level8x8)[8][64],
    int32_t startIdx,
    int32_t endIdx)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t i8x8, i8x8x4, i4x4, i, _startIdx, _endIdx;
    hl_codec_264_residual_read_block_f read_block_f =  p_codec->pps.pc_active->entropy_coding_mode_flag
            ? HL_NULL /*FIXME:CABAC hl264Residual_read_block_f_CABAC */
            : _hl_codec_264_residual_read_block_cavlc;

    // FIXME: use one "invType"

    if (startIdx == 0 && p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_16X16) {
        // residual_block(i16x16DClevel, 0, 15, 16)
        hl_codec_264_residual_inv_xt invType = {0};
        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL;
        err = read_block_f(&invType, p_codec, p_mb, (*i16x16DClevel), 0, 15, 16);
        if (err) {
            return err;
        }
    }
    for (i8x8 = 0; i8x8 < 4; ++i8x8) {
        if(!p_mb->transform_size_8x8_flag || !p_codec->pps.pc_active->entropy_coding_mode_flag) {
            hl_codec_264_residual_inv_xt invType = {0};
            i8x8x4 = (i8x8 << 2);
            _startIdx = HL_MATH_MAX(0, startIdx - 1);
            _endIdx = endIdx - 1;
            for (i4x4 = 0; i4x4 < 4; ++i4x4) {
                if (p_mb->CodedBlockPatternLuma & (1 << i8x8)) {
                    if (p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_16X16) {
                        // residual_block( i16x16AClevel[i8x8*4+ i4x4], Max( 0, startIdx - 1 ), endIdx - 1, 15)
                        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL;
                        invType.i_luma4x4BlkIdx = i8x8x4 + i4x4;
                        err = read_block_f(&invType, p_codec, p_mb, (*i16x16AClevel)[invType.i_luma4x4BlkIdx], _startIdx, _endIdx, 15);
                        if (err) {
                            return err;
                        }
                    }
                    else {
                        // residual_block( level[ i8x8 * 4 + i4x4 ], startIdx, endIdx, 16)
                        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;
                        invType.i_luma4x4BlkIdx = i8x8x4 + i4x4;
                        //HL_DEBUG_INFO("Bytes=%d BitsCount=%d\n", (p_codec->pobj_bits->current-p_codec->pobj_bits->start), p_codec->pobj_bits->bits_count);
                        err = read_block_f(&invType, p_codec, p_mb, (*level)[invType.i_luma4x4BlkIdx], startIdx, endIdx, 16);
                        if (err) {
                            return err;
                        }
                    }
                }
                else if (p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_16X16) {
                    i = i8x8x4 + i4x4;
                    (*i16x16AClevel)[i][0] = 0;
                    (*i16x16AClevel)[i][1] = 0;
                    (*i16x16AClevel)[i][2] = 0;
                    (*i16x16AClevel)[i][3] = 0;
                    (*i16x16AClevel)[i][4] = 0;
                    (*i16x16AClevel)[i][5] = 0;
                    (*i16x16AClevel)[i][6] = 0;
                    (*i16x16AClevel)[i][7] = 0;
                    (*i16x16AClevel)[i][8] = 0;
                    (*i16x16AClevel)[i][9] = 0;
                    (*i16x16AClevel)[i][10] = 0;
                    (*i16x16AClevel)[i][11] = 0;
                    (*i16x16AClevel)[i][12] = 0;
                    (*i16x16AClevel)[i][13] = 0;
                    (*i16x16AClevel)[i][14] = 0;
                }
                else {
                    i = i8x8x4 + i4x4;
                    (*level)[i][0] = 0;
                    (*level)[i][1] = 0;
                    (*level)[i][2] = 0;
                    (*level)[i][3] = 0;
                    (*level)[i][4] = 0;
                    (*level)[i][5] = 0;
                    (*level)[i][6] = 0;
                    (*level)[i][7] = 0;
                    (*level)[i][8] = 0;
                    (*level)[i][9] = 0;
                    (*level)[i][10] = 0;
                    (*level)[i][11] = 0;
                    (*level)[i][12] = 0;
                    (*level)[i][13] = 0;
                    (*level)[i][14] = 0;
                    (*level)[i][15] = 0;
                }
                if (!p_codec->pps.pc_active->entropy_coding_mode_flag && p_mb->transform_size_8x8_flag) {
                    for (i = 0; i < 16; ++i) {
                        HL_DEBUG_ERROR("Unchecked code and unroll!");
                        (*level8x8)[i8x8][4 * i + i4x4] = (*level)[(i8x8 << 2) + i4x4][i];//FIXME: Overflow
                    }
                }
            }
        }
        else if (p_mb->CodedBlockPatternLuma & (1 << i8x8)) {
            hl_codec_264_residual_inv_xt invType = {0};
            invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;
            invType.i_luma4x4BlkIdx = i8x8;
            err = read_block_f(&invType, p_codec, p_mb, (*level8x8)[invType.i_luma4x4BlkIdx], (startIdx << 2), (endIdx << 2) + 3, 64);
            if (err) {
                return err;
            }
        }
        else {
            for (i = 0; i < 64; ++i) {
                HL_DEBUG_ERROR("Unchecked code and unroll!");
                (*level8x8)[i8x8][i] = 0;//FIXME: Overflow
            }
        }
    }

    return err;
}

static HL_ERROR_T _hl_codec_264_residual_read_block_cavlc(
    hl_codec_264_residual_inv_xt* p_inv,
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t *coeffLevels, // table with at least "maxNumCoeff" values
    int32_t startIdx,
    int32_t endIdx,
    uint32_t maxNumCoeff)
{
    uint32_t i;
    int32_t nC, nA, nB;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_memory_blocks_t* pc_mem_blocks;
    hl_codec_264_layer_t* pc_layer = p_codec->layers.pc_active;

    int32_t ii, jj;
    int32_t coeffNum;
    int32_t blkA, blkB;
    int32_t mbAddrA, mbAddrB;
    hl_bool_t availableFlagA, availableFlagB;
    uint32_t TrailingOnes, TotalCoeff;
    hl_int32_16_t *level, *run;
    hl_codec_264_neighbouring_luma_block4x4_xt* neighbouringLumaBlock4x4 = p_mb->neighbouringLumaBlock4x4;
    hl_codec_264_neighbouring_chroma_block4x4_xt* neighbouringChromaBlock4x4 = p_mb->neighbouringChromaBlock4x4;
    int32_t zerosLeft;
    int32_t run_before;

    pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

    // map() memory
    hl_memory_blocks_map(pc_mem_blocks, &level);
    hl_memory_blocks_map(pc_mem_blocks, &run);

    // set coeffs to zeros
    // FIXME: use hl_memory_set(0)
    for (i = 0; i < maxNumCoeff; ++i) {
        coeffLevels[i] = 0;
    }

    // 9.2.1 Parsing process for total number of transform coefficient levels and trailing ones
    if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
        nC = (p_codec->sps.pc_active->ChromaArrayType == 1) ? -1 : -2;
    }
    else {
        const hl_codec_264_mb_t *pc_mbA, *pc_mbB;
        if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) {
            p_inv->i_luma4x4BlkIdx = 0;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_DCLEVEL) {
            p_inv->i_cbr4x4BlkIdx = 0;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_DCLEVEL) {
            p_inv->i_cbr4x4BlkIdx = 0;
        }
        // Generate blkA and blkB
        if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
            // subclause 6.4.10.4 is invoked with luma4x4BlkIdx as the input,
            //	and the output is assigned to mbAddrA, mbAddrB, luma4x4BlkIdxA, and luma4x4BlkIdxB. The 4x4
            //	luma block specified by mbAddrA\luma4x4BlkIdxA is assigned to blkA, and the 4x4 luma block
            //	specified by mbAddrB\luma4x4BlkIdxB is assigned to blkB.
            mbAddrA = neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_addr_A;
            blkA = neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_blk_idx_A;
            mbAddrB = neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_addr_B;
            blkB = neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_blk_idx_B;
        }
        else if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_LEVEL)) {
            // subclause 6.4.10.6 is invoked with
            // cb4x4BlkIdx as the input, and the output is assigned to mbAddrA, mbAddrB, cb4x4BlkIdxA, and
            // cb4x4BlkIdxB. The 4x4 Cb block specified by mbAddrA\cb4x4BlkIdxA is assigned to blkA, and the
            // 4x4 Cb block specified by mbAddrB\cb4x4BlkIdxB is assigned to blkB.
            // 6.4.10.6 Derivation process for neighbouring 4x4 chroma blocks for ChromaArrayType equal to 3
            HL_DEBUG_ERROR("Not implemented");
            err = HL_ERROR_NOT_IMPLEMENTED;
            goto bail;
        }
        else if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_LEVEL)) {
            // subclause 6.4.10.6 is invoked with
            // cr4x4BlkIdx as the input, and the output is assigned to mbAddrA, mbAddrB, cr4x4BlkIdxA, and
            // cr4x4BlkIdxB. The 4x4 Cr block specified by mbAddrA\cr4x4BlkIdxA is assigned to blkA, and the
            // 4x4 Cr block specified by mbAddrB\cr4x4BlkIdxB is assigned to blkB.
            HL_DEBUG_ERROR("Not implemented");
            err = HL_ERROR_NOT_IMPLEMENTED;
            goto bail;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) {
            // subclause 6.4.10.5 is invoked with chroma4x4BlkIdx as input, and the output is assigned to mbAddrA,
            // mbAddrB, chroma4x4BlkIdxA, and chroma4x4BlkIdxB. The 4x4 chroma block specified by
            // mbAddrA\iCbCr\chroma4x4BlkIdxA is assigned to blkA, and the 4x4 chroma block specified by
            // mbAddrB\iCbCr\chroma4x4BlkIdxB is assigned to blkB.
            mbAddrA = neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_addr_A;
            blkA = neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_blk_idx_A;
            mbAddrB = neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_addr_B;
            blkB = neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_blk_idx_B;
        }

        availableFlagA = HL_MATH_IS_POSITIVE_INT32(mbAddrA);
        availableFlagB = HL_MATH_IS_POSITIVE_INT32(mbAddrB);

        pc_mbA = availableFlagA ? pc_layer->pp_list_macroblocks[mbAddrA] : HL_NULL;
        pc_mbB = availableFlagB ? pc_layer->pp_list_macroblocks[mbAddrB] : HL_NULL;

        // mbAddrN is not avail if:
        //  -p_mb is coded using Intra mb pred mode
        //  -constrained_intra_pred_flag is equal to 1
        //  -FIXME: (Not sure about the test) mbAddrN is coded using an Inter macroblock prediction mode
        //  -slice data partitioning is in use (nal_unit_type is in the range of 2 to 4, inclusive)
        if ((availableFlagA || availableFlagB) && HL_CODEC_264_MB_TYPE_IS_INTRA(p_mb) && p_codec->pps.pc_active->constrained_intra_pred_flag == 1 && (2 <= p_codec->nal_current.e_nal_type && p_codec->nal_current.e_nal_type <= 4)) {
            if (pc_mbA && HL_CODEC_264_MB_TYPE_IS_INTER(pc_mbA)) {
                availableFlagA = HL_FALSE;
            }
            if (pc_mbB && HL_CODEC_264_MB_TYPE_IS_INTER(pc_mbB)) {
                availableFlagB = HL_FALSE;
            }
        }

        // derivating nA
        if (availableFlagA) {
            if ((HL_CODEC_264_MB_TYPE_IS_P_SKIP(pc_mbA) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(pc_mbA)) ||
                    (!HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbA) && hl_codec_264_utils_is_all_neighbouringblocks_zero(p_inv, p_codec, mbAddrA, blkA))) {
                nA = 0;
            }
            else if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbA)) {
                nA = 16;
            }
            else {
                if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
                    nA = pc_mbA->TotalCoeffsLuma[blkA];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) {
                    nA = pc_mbA->TotalCoeffsChromaACCbCr[p_inv->i_iCbCr][blkA];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
                    nA = pc_mbA->TotalCoeffsChromaDCCbCr[p_inv->i_iCbCr][blkA];
                }
            }
        }

        // derivating nB
        if (availableFlagB) {
            if ((HL_CODEC_264_MB_TYPE_IS_P_SKIP(pc_mbB) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(pc_mbB)) ||
                    (!HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbB) && hl_codec_264_utils_is_all_neighbouringblocks_zero(p_inv, p_codec, mbAddrB, blkB))) {
                nB = 0;
            }
            else if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbB)) {
                nB = 16;
            }
            else {
                if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
                    nB = pc_mbB->TotalCoeffsLuma[blkB];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) {
                    nB = pc_mbB->TotalCoeffsChromaACCbCr[p_inv->i_iCbCr][blkB];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
                    nB = pc_mbB->TotalCoeffsChromaDCCbCr[p_inv->i_iCbCr][blkB];
                }
            }
        }

        // derivating nC
        if (availableFlagA == 1 && availableFlagB == 1) {
            nC = (nA + nB + 1) >> 1;
        }
        else if (availableFlagA == 1 && availableFlagB == 0) {
            nC = nA;
        }
        else if (availableFlagA == 0 && availableFlagB == 1) {
            nC = nB;
        }
        else {
            nC = 0;
        }
    }

    // derivating "TrailingOnes" and "TotalCoeff"
    if (nC >= 0) {
        err = hl_codec_264_cavlc_ReadTotalCoeffTrailingOnes(p_codec, &TrailingOnes, &TotalCoeff, nC);
    }
    else {
        err = hl_codec_264_cavlc_ReadTotalCoeffTrailingOnesChromaDC(p_codec, &TrailingOnes, &TotalCoeff, nC);
    }

    // save "TotalCoeffs" for later use
    if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
        if ((p_mb->TotalCoeffsLuma[p_inv->i_luma4x4BlkIdx] = TotalCoeff)) {
            p_mb->CodedBlockPatternLuma4x4 |= (1 << p_inv->i_luma4x4BlkIdx);
        }
    }
    else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
        if ((p_mb->TotalCoeffsChromaDCCbCr[p_inv->i_iCbCr][p_inv->i_cbr4x4BlkIdx] = TotalCoeff)) {
            p_mb->CodedBlockPatternChromaDC4x4[p_inv->i_iCbCr] |= (1 << p_inv->i_cbr4x4BlkIdx);
        }
    }
    else {
        if ((p_mb->TotalCoeffsChromaACCbCr[p_inv->i_iCbCr][p_inv->i_cbr4x4BlkIdx] = TotalCoeff)) {
            p_mb->CodedBlockPatternChromaAC4x4[p_inv->i_iCbCr] |= (1 << p_inv->i_cbr4x4BlkIdx);
        }
    }

    //
    // EndOf coeff_token
    //

    if (TotalCoeff > 0) {
        uint32_t suffixLength, levelSuffixSize;
        uint32_t trailing_ones_sign_flag;
        if (TotalCoeff > 10 && TrailingOnes < 3) {
            suffixLength = 1;
        }
        else {
            suffixLength = 0;
        }

        // trailing_ones_sign_flag
        if (TrailingOnes) {
            trailing_ones_sign_flag = (hl_codec_264_bits_read_u(p_codec->pobj_bits, TrailingOnes) << 1);
            for (i = 0; i < TrailingOnes; ++i) {
                (*level)[i] = 1 - ((trailing_ones_sign_flag >> (TrailingOnes - i - 1)) & 0x02);
            }
        }

        for (i = TrailingOnes; i < TotalCoeff; ++i) {
            // level_prefix ce(v)
            int32_t level_prefix, levelCode, level_suffix;
            err = hl_codec_264_cavlc_ReadLevelPrefix(p_codec, &level_prefix);
            levelCode = (HL_MATH_MIN(15, level_prefix) << suffixLength);

            if (suffixLength > 0 || level_prefix >= 14) {
                if (level_prefix == 14 && suffixLength == 0) {
                    levelSuffixSize = 4;
                }
                else if (level_prefix >= 15) {
                    levelSuffixSize = level_prefix - 3;
                }
                else {
                    levelSuffixSize = suffixLength;
                }
                // level_suffix u(v)
                level_suffix = hl_codec_264_bits_read_u(p_codec->pobj_bits, levelSuffixSize);
                levelCode += level_suffix;
            }


            if (level_prefix >= 15 && suffixLength == 0) {
                levelCode += 15;
            }
            if (level_prefix >= 16) {
                levelCode += (1 << (level_prefix - 3)) - 4096;
            }
            if (i == TrailingOnes && TrailingOnes < 3) {
                levelCode += 2;
            }
            if ((levelCode & 1) == 0) {
                (*level)[i] = (levelCode + 2) >> 1;
            }
            else {
                (*level)[i] = (-levelCode - 1) >> 1;
            }
            if (suffixLength == 0) {
                suffixLength = 1;
            }
            if (HL_MATH_ABS_INT32((*level)[i]) > (3 << (suffixLength - 1)) && suffixLength < 6) {
                suffixLength ++;
            }
        }

        if ((int32_t)TotalCoeff < endIdx - startIdx + 1) {
            // total_zeros ce(v)
            if (nC>=0) {
                err = hl_codec_264_cavlc_ReadTotalZeros(p_codec, &zerosLeft, TotalCoeff);
            }
            else {
                err = hl_codec_264_cavlc_ReadTotalZerosChromaDC(p_codec, &zerosLeft, TotalCoeff);
            }
        }
        else {
            zerosLeft = 0;
        }

        for (i = 0; i < TotalCoeff - 1; i++) {
            if (zerosLeft > 0) {
                // run_before ce(v)
                err = hl_codec_264_cavlc_ReadRunBefore(p_codec, &run_before, zerosLeft);
                (*run)[i] = run_before;
            }
            else {
                (*run)[i] = 0;
            }
            zerosLeft = zerosLeft - (*run)[i];
        }
        (*run)[TotalCoeff - 1] = zerosLeft;
        coeffNum = -1;
        for (ii = TotalCoeff - 1; ii >= 0; ii--) {
            coeffNum += (*run)[ii] + 1;
            jj = startIdx + coeffNum;
            coeffLevels[jj] = (*level)[ii];
        }
    }

bail:
    // unmap() memory
    hl_memory_blocks_unmap(pc_mem_blocks, level);
    hl_memory_blocks_unmap(pc_mem_blocks, run);

    return err;
}

HL_ERROR_T hl_codec_264_residual_write_block_cavlc(
    hl_codec_264_residual_inv_xt* p_inv,
    const hl_codec_264_t* pc_codec,
    hl_codec_264_mb_t* p_mb,
    hl_codec_264_bits_t* p_bits,
    int32_t coeffLevel[16],
    int32_t startIdx,
    int32_t endIdx,
    int32_t maxNumCoef)
{
    int32_t j;
    int32_t nC, nA, nB;
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    static const int32_t suffixLengthThresholds[7/*suffixLength*/] = {0,3,6,12,24,48,1<<15};
    int32_t nz_coeff[16],coeff,highestCoeff=0,TotalCoeffs=0,TrailingOnes=0;
    hl_bool_t countTrailingOnes,countTotalZeros;
    int32_t suffixLength=0,total_zeros=0,k;
    int32_t run_before[16]= {0},zerosLeft;
    uint32_t trailing_ones_sign_flag;
    hl_codec_264_encode_slice_data_t* pc_esd;

    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;

    int32_t blkA, blkB;
    int32_t mbAddrA, mbAddrB;
    hl_bool_t availableFlagA, availableFlagB;

    pc_layer = pc_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    // 9.2.1 Parsing process for total number of transform coefficient levels and trailing ones
    if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
        nC = (pc_sps->ChromaArrayType == 1) ? -1 : -2;
    }
    else {
        const hl_codec_264_mb_t *pc_mbA, *pc_mbB;
        if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) {
            p_inv->i_luma4x4BlkIdx = 0;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_DCLEVEL) {
            p_inv->i_cbr4x4BlkIdx = 0;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_DCLEVEL) {
            p_inv->i_cbr4x4BlkIdx = 0;
        }
        // Generate blkA and blkB
        if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
            // subclause 6.4.10.4 is invoked with luma4x4BlkIdx as the input,
            //	and the output is assigned to mbAddrA, mbAddrB, luma4x4BlkIdxA, and luma4x4BlkIdxB. The 4x4
            //	luma block specified by mbAddrA\luma4x4BlkIdxA is assigned to blkA, and the 4x4 luma block
            //	specified by mbAddrB\luma4x4BlkIdxB is assigned to blkB.
            mbAddrA = p_mb->neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_addr_A;
            blkA = p_mb->neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_blk_idx_A;
            mbAddrB = p_mb->neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_addr_B;
            blkB = p_mb->neighbouringLumaBlock4x4[p_inv->i_luma4x4BlkIdx].i_blk_idx_B;
        }
        else if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CB_LEVEL)) {
            // subclause 6.4.10.6 is invoked with
            // cb4x4BlkIdx as the input, and the output is assigned to mbAddrA, mbAddrB, cb4x4BlkIdxA, and
            // cb4x4BlkIdxB. The 4x4 Cb block specified by mbAddrA\cb4x4BlkIdxA is assigned to blkA, and the
            // 4x4 Cb block specified by mbAddrB\cb4x4BlkIdxB is assigned to blkB.
            // 6.4.10.6 Derivation process for neighbouring 4x4 chroma blocks for ChromaArrayType equal to 3
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
        else if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CR_LEVEL)) {
            // subclause 6.4.10.6 is invoked with
            // cr4x4BlkIdx as the input, and the output is assigned to mbAddrA, mbAddrB, cr4x4BlkIdxA, and
            // cr4x4BlkIdxB. The 4x4 Cr block specified by mbAddrA\cr4x4BlkIdxA is assigned to blkA, and the
            // 4x4 Cr block specified by mbAddrB\cr4x4BlkIdxB is assigned to blkB.
            HL_DEBUG_ERROR("Not implemented");
            return HL_ERROR_NOT_IMPLEMENTED;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) {
            // subclause 6.4.10.5 is invoked with chroma4x4BlkIdx as input, and the output is assigned to mbAddrA,
            // mbAddrB, chroma4x4BlkIdxA, and chroma4x4BlkIdxB. The 4x4 chroma block specified by
            // mbAddrA\iCbCr\chroma4x4BlkIdxA is assigned to blkA, and the 4x4 chroma block specified by
            // mbAddrB\iCbCr\chroma4x4BlkIdxB is assigned to blkB.
            mbAddrA = p_mb->neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_addr_A;
            blkA = p_mb->neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_blk_idx_A;
            mbAddrB = p_mb->neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_addr_B;
            blkB = p_mb->neighbouringChromaBlock4x4[p_inv->i_cbr4x4BlkIdx].i_blk_idx_B;
        }

        availableFlagA = HL_MATH_IS_POSITIVE_INT32(mbAddrA);
        availableFlagB = HL_MATH_IS_POSITIVE_INT32(mbAddrB);

        pc_mbA = availableFlagA ? pc_layer->pp_list_macroblocks[mbAddrA] : HL_NULL;
        pc_mbB = availableFlagB ? pc_layer->pp_list_macroblocks[mbAddrB] : HL_NULL;

        // mbAddrN is not avail if:
        //  -CurrMb is coded using Intra mb pred mode
        //  -constrained_intra_pred_flag is equal to 1
        //  -FIXME: (Not sure about the test) mbAddrN is coded using an Inter macroblock prediction mode
        //  -slice data partitioning is in use (nal_unit_type is in the range of 2 to 4, inclusive)
        if ((availableFlagA || availableFlagB) && HL_CODEC_264_MB_TYPE_IS_INTRA(p_mb) && pc_pps->constrained_intra_pred_flag == 1 && (2 <= HL_CODEC_264_NAL(pc_slice_header)->e_type && HL_CODEC_264_NAL(pc_slice_header)->e_type <= 4)) {
            if (availableFlagA && HL_CODEC_264_MB_TYPE_IS_INTER(pc_mbA)) {
                availableFlagA = HL_FALSE;
            }
            if (availableFlagB && HL_CODEC_264_MB_TYPE_IS_INTER(pc_mbB)) {
                availableFlagB = HL_FALSE;
            }
        }

        // derivating nA
        if (availableFlagA) {
            if ((HL_CODEC_264_MB_TYPE_IS_P_SKIP(pc_mbA) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(pc_mbA)) ||
                    (!HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbA) && hl_codec_264_utils_is_all_neighbouringblocks_zero(p_inv, pc_codec, mbAddrA, blkA))) {
                nA = 0;
            }
            else if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbA)) {
                nA = 16;
            }
            else {
                if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
                    nA = pc_mbA->TotalCoeffsLuma[blkA];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) {
                    nA = pc_mbA->TotalCoeffsChromaACCbCr[p_inv->i_iCbCr][blkA];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
                    nA = pc_mbA->TotalCoeffsChromaDCCbCr[p_inv->i_iCbCr][blkA];
                }
            }
        }

        // derivating nB
        if (availableFlagB) {
            if ((HL_CODEC_264_MB_TYPE_IS_P_SKIP(pc_mbB) || HL_CODEC_264_MB_TYPE_IS_B_SKIP(pc_mbB)) ||
                    (!HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbB) && hl_codec_264_utils_is_all_neighbouringblocks_zero(p_inv, pc_codec, mbAddrB, blkB))) {
                nB = 0;
            }
            else if (HL_CODEC_264_MB_TYPE_IS_I_PCM(pc_mbB)) {
                nB = 16;
            }
            else {
                if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
                    nB = pc_mbB->TotalCoeffsLuma[blkB];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL) {
                    nB = pc_mbB->TotalCoeffsChromaACCbCr[p_inv->i_iCbCr][blkB];
                }
                else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
                    nB = pc_mbB->TotalCoeffsChromaDCCbCr[p_inv->i_iCbCr][blkB];
                }
            }
        }

        // derivating nC
        if (availableFlagA == 1 && availableFlagB == 1) {
            nC = (nA + nB + 1) >> 1;
        }
        else if (availableFlagA == 1 && availableFlagB == 0) {
            nC = nA;
        }
        else if (availableFlagA == 0 && availableFlagB == 1) {
            nC = nB;
        }
        else {
            nC = 0;
        }
    }

    // reorder + TotalCoeffs + TrailingOnes + Level + total_zeros
    countTrailingOnes = HL_TRUE;
    countTotalZeros = HL_FALSE;
    k=-1;
    for (j=0; j<maxNumCoef; ++j) {
        // reverse scan
        coeff = coeffLevel[maxNumCoef-1-j];
        if(coeff) {
            nz_coeff[TotalCoeffs++]=coeff;
            countTotalZeros=HL_TRUE;
            ++k;
            if (countTrailingOnes) {
                if (coeff==1 || coeff==-1) {
                    ++TrailingOnes;
                    countTrailingOnes=(TrailingOnes<3);
                }
                else {
                    countTrailingOnes=HL_FALSE;
                }
            }
        }
        else if (countTotalZeros) {
            ++run_before[k];
        }
        if (countTotalZeros && coeff==0) {
            ++total_zeros;
        }
    }


    // write coeff_token
    if (nC >= 0) {
        hl_codec_264_cavlc_WriteTotalCoeffTrailingOnes(p_bits, TrailingOnes, TotalCoeffs, nC);
    }
    else {
        hl_codec_264_cavlc_WriteTotalCoeffTrailingOnesChromaDC(p_bits, TrailingOnes, TotalCoeffs);
    }

    // save TotalCoeffs for later use
    /*if (!(p_inv->b_rdo))*/ { /* called for RDO to calculate bitrate? */
        if ((p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL) || (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL)) {
            p_mb->TotalCoeffsLuma[p_inv->i_luma4x4BlkIdx] = TotalCoeffs;
        }
        else if (p_inv->e_type == HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL) {
            p_mb->TotalCoeffsChromaDCCbCr[p_inv->i_iCbCr][p_inv->i_cbr4x4BlkIdx] = TotalCoeffs;
        }
        else {
            p_mb->TotalCoeffsChromaACCbCr[p_inv->i_iCbCr][p_inv->i_cbr4x4BlkIdx] = TotalCoeffs;
        }
    }

    if (TotalCoeffs > 0) {
        const hl_codec_264_cavlc_level_xt* level;
        int32_t suffixLength;
        int32_t levelCode;

        if (TotalCoeffs > 10 && TrailingOnes < 3) {
            suffixLength = 1;
        }
        else {
            suffixLength = 0;
        }
        for (j = 0; j < TotalCoeffs; ++j) {
            if (j < TrailingOnes) {
                trailing_ones_sign_flag = (1-nz_coeff[j])>>1;
                hl_codec_264_bits_write_u1(p_bits, trailing_ones_sign_flag);
            }
            else {
                if (HL_MATH_SIGN(nz_coeff[j]) == 1) {
                    // nz_coeff[i] = (levelCode + 2) >> 1;
                    levelCode = (nz_coeff[j] << 1) - 2;
                }
                else {
                    // nz_coeff[i] = (-levelCode - 1) >> 1;
                    levelCode = -(nz_coeff[j] << 1) - 1;
                }
                if ((j == TrailingOnes && TrailingOnes < 3) && levelCode >= 2) {
                    levelCode -= 0x2; //see CAVLC table generation
                }

                level = &(*hl_codec_264_cavlc_levels)[suffixLength][levelCode];

                // level_prefix <zeros 1>
                if (level->level_prefix > 0) {
                    hl_codec_264_bits_write_u(p_bits, 0, level->level_prefix);
                }
                hl_codec_264_bits_write_u1(p_bits, 1);

                // level_suffix
                if (level->levelSuffixSize) {
                    hl_codec_264_bits_write_u(p_bits, level->level_suffix, level->levelSuffixSize);
                }

                // change VLC table
                if (suffixLength == 0) {
                    suffixLength = 1;
                }

                if (HL_MATH_ABS_INT32(nz_coeff[j]) > suffixLengthThresholds[suffixLength]) {
                    ++suffixLength;
                }
            }
        }

        // total_zeros
        if ((int32_t)TotalCoeffs < endIdx - startIdx + 1) {
            if (nC>=0) {
                hl_codec_264_cavlc_WriteTotalZeros(p_bits, total_zeros, TotalCoeffs);
            }
            else {
                hl_codec_264_cavlc_WriteTotalZerosChromaDC(p_bits, total_zeros, TotalCoeffs);
            }
            zerosLeft=total_zeros;
        }
        else {
            zerosLeft = 0;
        }

        // run_before
        for (k = 0; (k < TotalCoeffs - 1) && (zerosLeft > 0); ++k) {
            hl_codec_264_cavlc_WriteRunBefore(p_bits, run_before[k], zerosLeft);
            zerosLeft -= run_before[k];
        }

        // JVT - O079 - 2.3 Elimination of single coefficients in inter macroblocks
        if (p_inv->b_rdo) {
            pc_esd->rdo.Single_ctr = 9;
            if (TotalCoeffs == 1) {
                int32_t nz_coeff_abs = HL_MATH_ABS(nz_coeff[0]);
                int32_t run = zerosLeft > 0 ? run_before[0] : 0;
                if(nz_coeff_abs == 1) {
                    if (run < 6) {
                        static const int32_t T[6] = { 3, 2, 2, 1, 1, 1 };
                        pc_esd->rdo.Single_ctr = T[run];
                    }
                    else {
                        pc_esd->rdo.Single_ctr = 0;
                    }
                }
            }
        }
    }

    return err;
}

static HL_ERROR_T _hl_codec_264_residual_write_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t i16x16DClevel[16],
    int32_t i16x16AClevel[16][16],
    int32_t level[16][16],
    int32_t startIdx,
    int32_t endIdx)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t i8x8, i4x4;
    hl_codec_264_residual_write_block_f f_write_block;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    if (!pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // FIXME: create one "invType"

    if (startIdx == 0 && HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
        // residual_block(i16x16DClevel, 0, 15, 16)
        hl_codec_264_residual_inv_xt invType = {0};
        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_DCLEVEL;
        f_write_block(&invType, p_codec, p_mb, pc_esd->pobj_bits, i16x16DClevel, 0, 15, 16);
    }
    for (i8x8 = 0; i8x8 < 4; ++i8x8) {
        if (!p_mb->transform_size_8x8_flag || !pc_pps->entropy_coding_mode_flag) {
            for (i4x4 = 0; i4x4 < 4; ++i4x4) {
                if (p_mb->CodedBlockPatternLuma & (1 << i8x8)) {
                    hl_codec_264_residual_inv_xt invType = {0};
                    if (HL_CODEC_264_MB_MODE_IS_INTRA_16X16(p_mb, 0)) {
                        // residual_block( i16x16AClevel[i8x8*4+ i4x4], Max( 0, startIdx - 1 ), endIdx - 1, 15)
                        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_INTRA16X16_ACLEVEL;
                        invType.i_luma4x4BlkIdx = (i8x8 << 2) + i4x4;
                        err = f_write_block(&invType, p_codec, p_mb, pc_esd->pobj_bits,
                                            i16x16AClevel[invType.i_luma4x4BlkIdx], HL_MATH_MAX(0, startIdx - 1), endIdx - 1, 15);
                        if (err) {
                            return err;
                        }
                    }
                    else {
                        // residual_block( level[ i8x8 * 4 + i4x4 ], startIdx, endIdx, 16)
                        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_LUMA_LEVEL;
                        invType.i_luma4x4BlkIdx = (i8x8 << 2) + i4x4;
                        //HL_DEBUG_INFO("Bytes=%d BitsCount=%d\n", (pc_esd->pobj_bits->current-pc_esd->pobj_bits->start), pc_esd->pobj_bits->bits_count);
                        err = f_write_block(&invType, p_codec, p_mb, pc_esd->pobj_bits,
                                            level[invType.i_luma4x4BlkIdx], startIdx, endIdx, 16);
                        if (err) {
                            return err;
                        }
                    }
                }
            }
        }
        else if (p_mb->CodedBlockPatternLuma & (1 << i8x8)) {
            HL_DEBUG_ERROR("Level 8x8");
#if 0
            HL_CODEC_264_RESISUAL_INV_TYPE_xt invType = {0};
            invType.LumaLevel = HL_TRUE;
            invType.i_luma4x4BlkIdx = i8x8;
            err = f_write_block(&invType, p_codec, p_mb,
                                level8x8[invType.i_luma4x4BlkIdx], 4 * startIdx, 4 * endIdx + 3, 64);
#endif
        }
    }

    return err;
}

HL_ERROR_T hl_codec_264_residual_write(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    int32_t startIdx,
    int32_t endIdx)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_codec_264_residual_write_block_f f_write_block;
    hl_codec_264_layer_t* pc_layer;
    const hl_codec_264_nal_pps_t* pc_pps;
    const hl_codec_264_nal_sps_t* pc_sps;
    hl_codec_264_encode_slice_data_t* pc_esd;
    const hl_codec_264_nal_slice_header_t* pc_slice_header;

    pc_layer = p_codec->layers.pc_active;
    pc_esd = pc_layer->encoder.p_list_esd[p_mb->u_slice_idx];
    pc_slice_header = pc_esd->pc_slice->p_header;
    pc_pps = pc_slice_header->pc_pps;
    pc_sps = pc_pps->pc_sps;

    if (!pc_pps->entropy_coding_mode_flag) {
        f_write_block = hl_codec_264_residual_write_block_cavlc;
    }
    else {
        HL_DEBUG_ERROR("CABAC not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    // 7.3.5.3.1 Residual luma syntax
    _hl_codec_264_residual_write_luma(p_codec,
                                      p_mb,
                                      p_mb->Intra16x16DCLevel,
                                      p_mb->Intra16x16ACLevel,
                                      p_mb->LumaLevel,
                                      startIdx,
                                      endIdx);

    // FIXME: use one "invType"

    if (pc_sps->ChromaArrayType == 1 || pc_sps->ChromaArrayType == 2) {
        int32_t NumC8x8 = 4 >> (pc_sps->SubWidthC_TrailingZeros + pc_sps->SubHeightC_TrailingZeros);
        int32_t iCbCr, i8x8, i4x4;
        static const HL_ALIGN(HL_ALIGN_V) int32_t __zero_coeffs[16] = { 0 };

        // "__zero_coeffs" is used because RDO module will not use fill "ChromaDCLevel" or "ChromaACLevel" with zeros

        if ((p_mb->CodedBlockPatternChroma & 3) && startIdx == 0) { /* chroma DC residual present */
            hl_codec_264_residual_inv_xt invType = {0};
            invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_DCLEVEL;
            // Chroma component Zero (Cb)
            invType.i_iCbCr = 0;
            err = f_write_block(&invType,
                                p_codec,
                                p_mb,
                                pc_esd->pobj_bits,
                                p_mb->CodedBlockPatternChromaDC4x4[0] ? p_mb->ChromaDCLevel[0] : __zero_coeffs,
                                0,
                                4 * NumC8x8 - 1,
                                4 * NumC8x8);
            if (err) {
                return err;
            }
            // Chroma component Zero (Cr)
            invType.i_iCbCr = 1;
            err = f_write_block(&invType,
                                p_codec,
                                p_mb,
                                pc_esd->pobj_bits,
                                p_mb->CodedBlockPatternChromaDC4x4[1] ? p_mb->ChromaDCLevel[1] : __zero_coeffs,
                                0,
                                4 * NumC8x8 - 1,
                                4 * NumC8x8);
            if (err) {
                return err;
            }
        }

        for (iCbCr = 0; iCbCr < 2; ++iCbCr) {
            for (i8x8 = 0; i8x8 < NumC8x8; ++i8x8) {
                for (i4x4 = 0; i4x4 < 4; ++i4x4) {
                    if (p_mb->CodedBlockPatternChroma & 2) { /* Chroma AC residual present */
                        hl_codec_264_residual_inv_xt invType = {0};
                        invType.e_type = HL_CODEC_264_RESISUAL_INV_TYPE_CHROMA_ACLEVEL;
                        invType.i_cbr4x4BlkIdx = i4x4;
                        invType.i_iCbCr = iCbCr;
                        err = f_write_block(&invType,
                                            p_codec,
                                            p_mb,
                                            pc_esd->pobj_bits,
                                            (p_mb->CodedBlockPatternChromaAC4x4[iCbCr] & (1 << i4x4)) ? p_mb->ChromaACLevel[iCbCr][i8x8*4+i4x4] : __zero_coeffs,
                                            HL_MATH_MAX(0, startIdx - 1),
                                            endIdx-1,
                                            15);
                        if (err) {
                            return err;
                        }
                    }
                }
            }
        }
    }
    else if (pc_sps->ChromaArrayType == 3) {
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return err;
}
