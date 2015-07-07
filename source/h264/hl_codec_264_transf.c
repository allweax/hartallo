#include "hartallo/h264/hl_codec_264_transf.h"
#include "hartallo/h264/hl_codec_264_quant.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_utils.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_macros.h"
#include "hartallo/h264/hl_codec_264_pict.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

#if HL_HAVE_X86_INTRIN
#include "hartallo/h264/intrinsics/x86/hl_codec_x86_264_transf_intrin.h"
#endif /* HL_HAVE_X86_INTRIN */

struct hl_codec_264_s;
struct hl_codec_264_mb_s;

static void hl_codec_264_transf_inverse_residual4x4_cpp(int32_t bitDepth, int32_t d[4][4], int32_t r[4][4]);
static void hl_codec_264_transf_scale_luma_dc_coeff_intra16x16_cpp(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    int32_t qP,
    int32_t BitDepth,
    HL_IN_ALIGNED(16) const int32_t c[4][4],
    HL_OUT_ALIGNED(16) int32_t dcY[4][4]);
static void hl_codec_264_transf_frw_residual4x4_cpp(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16)int32_t out4x4[4][4]);
static void hl_codec_264_transf_frw_hadamard4x4_dc_luma_cpp(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);

void (*hl_codec_264_transf_inverse_residual4x4)(int32_t bitDepth, int32_t d[4][4], int32_t r[4][4]) = hl_codec_264_transf_inverse_residual4x4_cpp;
void (*hl_codec_264_transf_scale_luma_dc_coeff_intra16x16)(const struct hl_codec_264_s* p_codec, const struct hl_codec_264_mb_s* p_mb,  int32_t qP, int32_t BitDepth, HL_IN_ALIGNED(16) const int32_t c[4][4],  HL_OUT_ALIGNED(16) int32_t dcY[4][4]) = hl_codec_264_transf_scale_luma_dc_coeff_intra16x16_cpp;
void (*hl_codec_264_transf_frw_residual4x4)(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]) = hl_codec_264_transf_frw_residual4x4_cpp;
void (*hl_codec_264_transf_frw_hadamard4x4_dc_luma)(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]) = hl_codec_264_transf_frw_hadamard4x4_dc_luma_cpp;

HL_ERROR_T hl_codec_264_transf_init_funcs()
{
    HL_DEBUG_INFO("Initializing transform functions");

    hl_codec_264_transf_inverse_residual4x4 = hl_codec_264_transf_inverse_residual4x4_cpp;
    hl_codec_264_transf_scale_luma_dc_coeff_intra16x16 = hl_codec_264_transf_scale_luma_dc_coeff_intra16x16_cpp;
    hl_codec_264_transf_frw_residual4x4 = hl_codec_264_transf_frw_residual4x4_cpp;
    hl_codec_264_transf_frw_hadamard4x4_dc_luma = hl_codec_264_transf_frw_hadamard4x4_dc_luma_cpp;

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        hl_codec_264_transf_inverse_residual4x4 = hl_codec_x86_264_transf_inverse_residual4x4_intrin_sse2;
        hl_codec_264_transf_scale_luma_dc_coeff_intra16x16 = hl_codec_x86_264_transf_scale_luma_dc_coeff_intra16x16_intrin_sse2;
        hl_codec_264_transf_frw_residual4x4 = hl_codec_264_transf_frw_residual4x4_intrin_sse2;
        hl_codec_264_transf_frw_hadamard4x4_dc_luma = hl_codec_264_transf_frw_hadamard4x4_dc_luma_intrin_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSSE3)) {

    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        hl_codec_264_transf_scale_luma_dc_coeff_intra16x16 = hl_codec_x86_264_transf_scale_luma_dc_coeff_intra16x16_intrin_sse41;
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        extern void hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2(int32_t bitDepth, HL_ALIGNED(16) int32_t d[4][4], HL_ALIGNED(16) int32_t r[4][4]);
        extern void hl_codec_264_transf_frw_residual4x4_asm_sse2(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16)int32_t out4x4[4][4]);
        extern hl_codec_264_transf_frw_hadamard4x4_dc_luma_asm_sse2(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);

        hl_codec_264_transf_inverse_residual4x4 = hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2;
        hl_codec_264_transf_frw_residual4x4 = hl_codec_264_transf_frw_residual4x4_asm_sse2;
        hl_codec_264_transf_frw_hadamard4x4_dc_luma = hl_codec_264_transf_frw_hadamard4x4_dc_luma_asm_sse2;
    }

#endif /* HL_HAVE_X86_ASM */

    return HL_ERROR_SUCCESS;
}


// 8.5.1 Specification of transform decoding process for 4x4 luma residual blocks
// This specification applies when transform_size_8x8_flag is equal to 0
// Also reconstruct the luma samples of the block
HL_ERROR_T hl_codec_264_transf_decode_luma4x4(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    const int32_t* luma4x4BlkIdxList,
    hl_size_t luma4x4BlkIdxListCount,
    HL_IN_ALIGNED(16) const int32_t* predL,
    int32_t predLStride,
    hl_bool_t predLIs4x4SamplesOnly)
{
    int32_t xO, yO, luma4x4BlkIdx;
    hl_size_t j;
    hl_int32_4x4_t *c, *r, *u;
    const int32_t* _predL;
    hl_memory_blocks_t* pc_mem_blocks;

    if (predLIs4x4SamplesOnly && (luma4x4BlkIdxListCount > 1 || predLStride > 4)) {
        HL_DEBUG_ERROR("invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

    // map() memory blocks
    hl_memory_blocks_map_4x4zeros(pc_mem_blocks, &c);
    hl_memory_blocks_map_4x4zeros(pc_mem_blocks, &r);
    hl_memory_blocks_map_4x4zeros(pc_mem_blocks, &u);

    for (j = 0; j < luma4x4BlkIdxListCount; ++j) {
        luma4x4BlkIdx = luma4x4BlkIdxList[j];

        if (predLIs4x4SamplesOnly) {
            _predL = predL;
        }
        else {
            // 6.4.3 Inverse 4x4 luma block scanning process
            xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
            yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

            _predL = predL + (yO * predLStride) + xO;
        }

        // Do not use "p_mb->CodedBlockPatternLuma or p_mb->TotalCoeffsLuma" because of the Encoder (e.g. RDO module) which could call the function with wrong values
        //if(!(p_mb->CodedBlockPatternLuma4x4 & hlMath_Pow2Val[luma4x4BlkIdx])){
        if (!(p_mb->CodedBlockPatternLuma4x4 & (1 << luma4x4BlkIdx))) { // Check whether we have residual on this 4x4 block
            hl_memory_copy4x4((int32_t*)u, 4, _predL, predLStride);
        }
        else {
            // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
            InverseScan4x4(p_mb->LumaLevel[luma4x4BlkIdx], (*c));
            // 8.5.12 Scaling and transformation process for residual 4x4 blocks
            hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (const int32_t(*)[4])(*c), (*r), HL_TRUE, HL_FALSE, -1);

            if (p_mb->TransformBypassModeFlag == 1 && p_mb->MbPartPredMode[0] == HL_CODEC_264_MB_MODE_INTRA_4X4
                    && (p_mb->Intra4x4PredMode[luma4x4BlkIdx] == Intra_4x4_Vertical || p_mb->Intra4x4PredMode[luma4x4BlkIdx] == Intra_4x4_Horizontal)) {
                // 8.5.15 Intra residual transform-bypass decoding process
                hl_codec_264_transf_bypass_intra_residual(4, 4, (int32_t)p_mb->Intra4x4PredMode[luma4x4BlkIdx], (*r));
            }
            // Add predicted samples to residual and clip the result
            hl_math_addclip_4x4(_predL, predLStride, (const int32_t*)(*r), 4, p_codec->PixelMaxValueY, (int32_t*)(*u), 4);
        } // if(AllZeros)

        // 8.5.14 Picture construction process prior to deblocking filter process
        hl_codec_264_pict_reconstruct_luma4x4(p_codec, p_mb, (const int32_t*)(*u), 4, luma4x4BlkIdx);
    }

    // unmap() memory blocks
    hl_memory_blocks_unmap(pc_mem_blocks, c);
    hl_memory_blocks_unmap(pc_mem_blocks, r);
    hl_memory_blocks_unmap(pc_mem_blocks, u);

    return HL_ERROR_SUCCESS;
}

// 8.5.4 Specification of transform decoding process for chroma samples
// Also reconstruct the chroma samples of the block
HL_ERROR_T hl_codec_264_transf_decode_chroma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_IN_ALIGNED(16) const int32_t predC[16][16],
    int32_t iCbCr)
{
    if (p_codec->sps.pc_active->ChromaArrayType == 1 || p_codec->sps.pc_active->ChromaArrayType == 2) {
        hl_int32_4x2_t* dcC; // must be zeros
        hl_int32_4x4_t* c;
        hl_bool_t isAllZeroChromaDCLevel, isAllZeroChromaACLevel;
        hl_memory_blocks_t* pc_mem_blocks;

        static const int32_t dcCij[8][2] = { {0,0},{0,1},{1,0},{1,1},{2,0},{2,1},{3,0},{3,1} };//Figure 8-7

        pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

        isAllZeroChromaDCLevel = (p_mb->CodedBlockPatternChromaDC4x4[iCbCr] == 0);
        isAllZeroChromaACLevel = (p_mb->CodedBlockPatternChromaAC4x4[iCbCr] == 0);

        // Do not use "p_mb->CodedBlockPatternChroma" because of the Encoder (e.g. RDO module) which could call the function with wrong values
        // No DC coeffs -> No AC coeffs
        if (isAllZeroChromaDCLevel && isAllZeroChromaACLevel) {
            return hl_codec_264_pict_reconstruct_chroma(p_codec, p_mb, predC, 0, iCbCr==0);
        }

        // map() memory blocks
        hl_memory_blocks_map_4x4zeros(pc_mem_blocks, &dcC); // must be zeros
        hl_memory_blocks_map(pc_mem_blocks, &c);

        /*** Chroma DC ***/
        // Do not use "p_mb->CodedBlockPatternChroma" because of the Encoder (e.g. RDO module) which could call the function with wrong values
        if (!isAllZeroChromaDCLevel) {
            int32_t bitDepth, qP;
            if (p_codec->sps.pc_active->ChromaArrayType == 1) {
                (*c)[0][0] = p_mb->ChromaDCLevel[iCbCr][0];
                (*c)[0][1] = p_mb->ChromaDCLevel[iCbCr][1];
                (*c)[1][0] = p_mb->ChromaDCLevel[iCbCr][2];
                (*c)[1][1] = p_mb->ChromaDCLevel[iCbCr][3];
            }
            else { // ChromaArrayType==2
                (*c)[0][0] = p_mb->ChromaDCLevel[iCbCr][0];
                (*c)[0][1] = p_mb->ChromaDCLevel[iCbCr][2];
                (*c)[1][0] = p_mb->ChromaDCLevel[iCbCr][1];
                (*c)[1][1] = p_mb->ChromaDCLevel[iCbCr][5];
                (*c)[2][0] = p_mb->ChromaDCLevel[iCbCr][3];
                (*c)[2][1] = p_mb->ChromaDCLevel[iCbCr][6];
                (*c)[3][0] = p_mb->ChromaDCLevel[iCbCr][4];
                (*c)[3][1] = p_mb->ChromaDCLevel[iCbCr][7];
            }
            // 8.5.11 Scaling and transformation process for chroma DC transform coefficients
            bitDepth = p_codec->sps.pc_active->BitDepthC;
            qP = p_mb->QPprimeC[iCbCr];
            hl_codec_264_transf_scale_chroma_dc_coeff(p_codec, p_mb, (*c), bitDepth, qP, iCbCr, (*dcC));
        }// if(AllZero)

        /*** Chroma AC ***/

        {
            hl_int32_4x4_t* r;
            hl_int32_16x16_t* rMb; // must be initialized with zeros
            hl_int32_16x16_t* u;
            hl_int32_16_t* chromaList;
            int32_t numChroma4x4Blks ,chroma4x4BlkIdx, xO, yO, byPass;
            uint32_t  i, j;

            // map() memory blocks
            hl_memory_blocks_map(pc_mem_blocks, &r);
            hl_memory_blocks_map_16x16zeros(pc_mem_blocks, &rMb); // must be initialized with zeros
            hl_memory_blocks_map(pc_mem_blocks, &u);
            hl_memory_blocks_map(pc_mem_blocks, &chromaList);

            numChroma4x4Blks = (p_codec->sps.pc_active->MbWidthC >>2) * (p_codec->sps.pc_active->MbHeightC >>2);
            byPass = p_mb->TransformBypassModeFlag==1 &&
                     (p_mb->MbPartPredMode[0]==HL_CODEC_264_MB_MODE_INTRA_4X4 || p_mb->MbPartPredMode[0]==HL_CODEC_264_MB_MODE_INTRA_8X8 || p_mb->MbPartPredMode[0]==HL_CODEC_264_MB_MODE_INTRA_16X16) &&
                     (p_mb->intra_chroma_pred_mode == 1 || p_mb->intra_chroma_pred_mode==2);

            for (chroma4x4BlkIdx = 0; chroma4x4BlkIdx < numChroma4x4Blks; ++chroma4x4BlkIdx) {
                (*chromaList)[0] = (*dcC)[dcCij[chroma4x4BlkIdx][0]][dcCij[chroma4x4BlkIdx][1]];

                if (((*chromaList)[0] || (p_mb->CodedBlockPatternChromaAC4x4[iCbCr] & (1 << chroma4x4BlkIdx)))) {
                    static const int32_t __count = 15 * sizeof(int32_t);
                    hl_memory_copy(&(*chromaList)[1], &p_mb->ChromaACLevel[iCbCr][chroma4x4BlkIdx][0], __count);

                    InverseScan4x4((*chromaList), (*c));
                    hl_codec_264_transf_scale_residual4x4(p_codec,p_mb, (*c), (*r), HL_FALSE,HL_FALSE, iCbCr);

                    xO = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][0];// (8-309)
                    yO = InverseRasterScan16_4x4[chroma4x4BlkIdx][8][1];// (8-310)
                    hl_memory_copy4x4(&(*rMb)[yO][xO], 16, &(*r)[0][0], 4);
                }
            }

            if(byPass) {
                hl_codec_264_transf_bypass_intra_residual(p_codec->sps.pc_active->MbWidthC,p_codec->sps.pc_active->MbHeightC,(2-p_mb->intra_chroma_pred_mode),(int32_t (*)[4])rMb);
            }

            // Add predicted samples with residual and clip the result
            if (p_codec->sps.pc_active->MbWidthC == 8 && p_codec->sps.pc_active->MbHeightC == 8) { // YUV420
                hl_math_addclip_8x8((int32_t*)predC, 16, (const int32_t*)(*rMb), 16, p_codec->PixelMaxValueC, (int32_t*)(*u), 16);
            }
            else if (p_codec->sps.pc_active->MbWidthC == 16 && p_codec->sps.pc_active->MbHeightC == 16) { // YUV444
                hl_math_addclip_16x16((int32_t*)predC, 16, (const int32_t*)(*rMb), 16, p_codec->PixelMaxValueC, (int32_t*)(*u), 16);
            }
            else {
                for ( i= 0; i<p_codec->sps.pc_active->MbHeightC; i+=4) {
                    for (j = 0; j<p_codec->sps.pc_active->MbWidthC; j+=4) {
                        // FIXME: use "hl_math_addclip_8x8" or "hl_math_addclip_16x16"
                        hl_math_addclip_4x4(&predC[i][j], 16, (const int32_t*)&(*rMb)[i][j], 16, p_codec->PixelMaxValueC, (int32_t*)&(*u)[i][j], 16);
                    }
                }
            }

            // 8.5.14 Picture construction process prior to deblocking filter process
            hl_codec_264_pict_reconstruct_chroma(p_codec, p_mb, (*u), 0, iCbCr==0);

            // unmap() memory arrays
            hl_memory_blocks_unmap(pc_mem_blocks, r);
            hl_memory_blocks_unmap(pc_mem_blocks, rMb);
            hl_memory_blocks_unmap(pc_mem_blocks, u);
            hl_memory_blocks_unmap(pc_mem_blocks, chromaList);
        }

        // unmap() memory arrays
        hl_memory_blocks_unmap(pc_mem_blocks, dcC);
        hl_memory_blocks_unmap(pc_mem_blocks, c);
    }
    else if (p_codec->sps.pc_active->ChromaArrayType == 3) {
        // subclause 8.5.5
        HL_DEBUG_ERROR("Not implemented yet");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    return HL_ERROR_SUCCESS;
}

// 8.5.2 Specification of transform decoding process for luma samples of Intra_16x16 macroblock prediction mode
// Also reconstruct the luma samples of the block
HL_ERROR_T hl_codec_264_transf_decode_intra16x16_luma(
    hl_codec_264_t* p_codec,
    hl_codec_264_mb_t* p_mb,
    HL_IN_ALIGNED(16) const int32_t predL[16][16])
{
    // Neither DC coeffs nor No AC coeffs
    // CodedBlockPatternLuma4x4 -> Intra16x16DCLevel
    if (!p_mb->CodedBlockPatternLuma4x4) {
        // memcpy(u, predL, 1024);
        // 8.5.14 Picture construction process prior to deblocking filter process
        return hl_codec_264_pict_reconstruct_luma16x16(p_codec, p_mb, predL/*predL==u*/);
    }
    else {
        hl_int32_4x4_t* c;
        hl_int32_4x4_t* dcY; // must be zeros
        hl_int32_16x16_t* rMb; // must be zeros
        hl_int32_16x16_t* u;
        hl_int32_4x4_t* r;
        hl_memory_blocks_t* pc_mem_blocks;

        int32_t luma4x4BlkIdx, xO, yO/*, i*/;
        hl_int32_16_t* lumaList;

        pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

        // map() blocks
        hl_memory_blocks_map(pc_mem_blocks, &c);
        hl_memory_blocks_map_4x4zeros(pc_mem_blocks, &dcY); // must be zeros
        hl_memory_blocks_map_16x16zeros(pc_mem_blocks, &rMb); // must be zeros
        hl_memory_blocks_map(pc_mem_blocks, &u);
        hl_memory_blocks_map(pc_mem_blocks, &r);
        hl_memory_blocks_map(pc_mem_blocks, &lumaList);

        // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
        InverseScan4x4(p_mb->Intra16x16DCLevel, (*c));
        // 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
        hl_codec_264_transf_scale_luma_dc_coeff_intra16x16(p_codec, p_mb, p_mb->QPyprime, p_codec->sps.pc_active->BitDepthY, (*c), (*dcY));

        for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; ++luma4x4BlkIdx) {
            static const int32_t __count = 15 * sizeof(int32_t);
            (*lumaList)[0] = (*dcY)[dcYij[luma4x4BlkIdx][0]][dcYij[luma4x4BlkIdx][1]];
            memcpy(&(*lumaList)[1], &p_mb->Intra16x16ACLevel[luma4x4BlkIdx][0], __count); // FIXME: use hl_memory_copy()
            // 6.4.3 Inverse 4x4 luma block scanning process
            xO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][0];
            yO = Inverse4x4LumaBlockScanOrderXY[luma4x4BlkIdx][1];

            if (!hl_math_allzero16(lumaList)) { // Intra16x16ACLevel 4x4 CBP never saved
                // 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
                InverseScan4x4((*lumaList), (*c));
                // 8.5.12 Scaling and transformation process for residual 4x4 blocks
                hl_codec_264_transf_scale_residual4x4(p_codec, p_mb, (*c), (*r), HL_TRUE, HL_TRUE, -1);
                // Copy 4x4 residual block
                hl_memory_copy4x4(&(*rMb)[yO][xO], 16, (const int32_t *)(*r), 4);
            }//if (!AllZero)
        }//for (each blk4x4)

        if (p_mb->TransformBypassModeFlag == 1 && (p_mb->Intra16x16PredMode==0 || p_mb->Intra16x16PredMode==1)) {
            // 8.5.15 Intra residual transform-bypass decoding process
            hl_codec_264_transf_bypass_intra_residual(16, 16, (int32_t)p_mb->Intra16x16PredMode, (int32_t (*)[4])rMb);
        }

        // Add predicted data to the residual and clip the result
        hl_math_addclip_16x16((int32_t*)predL, 16, (const int32_t*)(*rMb), 16, p_codec->PixelMaxValueY, (int32_t*)(*u), 16);

        // unmap() memory blocks
        hl_memory_blocks_unmap(pc_mem_blocks, c);
        hl_memory_blocks_unmap(pc_mem_blocks, dcY);
        hl_memory_blocks_unmap(pc_mem_blocks, rMb);
        hl_memory_blocks_unmap(pc_mem_blocks, u);
        hl_memory_blocks_unmap(pc_mem_blocks, r);
        hl_memory_blocks_unmap(pc_mem_blocks, lumaList);

        // 8.5.14 Picture construction process prior to deblocking filter process
        return hl_codec_264_pict_reconstruct_luma16x16(p_codec, p_mb, (*u));
    }
}

// 8.5.12 Scaling and transformation process for residual 4x4 blocks
void hl_codec_264_transf_scale_residual4x4(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    HL_IN const int32_t c[4][4],
    HL_OUT int32_t r[4][4],
    hl_bool_t luma,
    hl_bool_t Intra16x16,
    int32_t iCbCr
)
{
    int32_t bitDepth, qP, i;
    int32_t sMbFlag = 0;//Must be 0 for baseline

    bitDepth = luma ? p_codec->sps.pc_active->BitDepthY : p_codec->sps.pc_active->BitDepthC;
    if(p_mb->e_type == HL_CODEC_264_MB_TYPE_SI || (HL_CODEC_264_MB_TYPE_IS_INTER(p_mb) && IsSliceHeaderSP(p_codec->layers.pc_active->pc_slice_hdr))) {
        HL_DEBUG_ERROR("Not checked code (must not be called for baseline)");
        sMbFlag = 1;
    }
    if(luma) {
        qP = !sMbFlag ? p_mb->QPyprime : p_mb->QSy;
    }
    else {
        qP = !sMbFlag ? p_mb->QPprimeC[iCbCr] : p_mb->QSc[iCbCr];
    }

    if (p_mb->TransformBypassModeFlag == 1) { // TransformBypassModeFlag must be 0 for baseline
        HL_DEBUG_ERROR("Not checked code (must not be called for baseline)");
        for (i=0; i<4; ++i) {
            r[i][0] = c[i][0];
            r[i][1] = c[i][1];
            r[i][2] = c[i][2];
            r[i][3] = c[i][3];
        }
    }
    else {
        // 8.5.12.1 Scaling process for residual 4x4 blocks
        HL_ALIGN(HL_ALIGN_V) int32_t d[4][4];
        hl_codec_264_quant_scale_residual4x4(p_codec, p_mb, bitDepth, qP, c, luma, Intra16x16, iCbCr, d);
        // 8.5.12.2 Transformation process for residual 4x4 blocks
        hl_codec_264_transf_inverse_residual4x4(bitDepth, d, r); // FIXME
    }
}

// 8.5.12.2 Transformation process for residual 4x4 blocks
static void hl_codec_264_transf_inverse_residual4x4_cpp(int32_t bitDepth, int32_t d[4][4], int32_t r[4][4])
{
    int32_t e[4][4], f[4][4], g[4][4], h[4][4];
    int32_t i,j;

    for (i=0; i<4; ++i) {
        e[i][0] = d[i][0]+d[i][2];
        e[i][1] = d[i][0]-d[i][2];
        e[i][2] = (d[i][1]>>1)-d[i][3];
        e[i][3] = d[i][1]+(d[i][3]>>1);
    }
    for (i=0; i<4; ++i) {
        f[i][0]=e[i][0]+e[i][3];
        f[i][1]=e[i][1]+e[i][2];
        f[i][2]=e[i][1]-e[i][2];
        f[i][3]=e[i][0]-e[i][3];
    }
    for (j=0; j<4; ++j) {
        g[0][j]=f[0][j]+f[2][j];
        g[1][j]=f[0][j]-f[2][j];
        g[2][j]=(f[1][j]>>1)-f[3][j];
        g[3][j]=f[1][j]+(f[3][j]>>1);
    }
    for (j=0; j<4; ++j) {
        h[0][j]=g[0][j]+g[3][j];
        h[1][j]=g[1][j]+g[2][j];
        h[2][j]=g[1][j]-g[2][j];
        h[3][j]=g[0][j]-g[3][j];
    }

    for (i=0; i<4; ++i) {
        r[i][0]=(h[i][0]+32/*2^5*/)>>6;
        r[i][1]=(h[i][1]+32/*2^5*/)>>6;
        r[i][2]=(h[i][2]+32/*2^5*/)>>6;
        r[i][3]=(h[i][3]+32/*2^5*/)>>6;
    }
}


// 8.5.15 Intra residual transform-bypass decoding process
void hl_codec_264_transf_bypass_intra_residual(
    int32_t nW,
    int32_t nH,
    int32_t horPredFlag,
    HL_IN_OUT int32_t r[4][4]/*4x4 or more*/)
{
    int32_t f[16][16],i,j,k;//FIXME: is f needed?

    for (i=0; i<nH; ++i) {
        for (j=0; j<nW; ++j) {
            f[i][j]=r[j][j];//(8-417)
        }
    }
    //FIXME: MULTS
    if (horPredFlag == 0) {
        for (i=0; i<nH; ++i) {
            for (j=0; j<nW; ++j) {
                r[i][j]=0;
                for(k=0; k<=i; ++k) {
                    r[i][j]+=f[k][j];//(8-418)
                }
            }
        }
    }
    else {
        for (i=0; i<nH; ++i) {
            for (j=0; j<nW; ++j) {
                r[i][j]=0;
                for (k=0; k<=j; ++k) {
                    r[i][j]=f[i][k];//(8-419)
                }
            }
        }
    }
}


// 8.5.10 Scaling and transformation process for DC transform coefficients for Intra_16x16 macroblock type
static void hl_codec_264_transf_scale_luma_dc_coeff_intra16x16_cpp(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    int32_t qP,
    int32_t BitDepth,
    HL_IN_ALIGNED(16) const int32_t c[4][4],
    HL_OUT_ALIGNED(16) int32_t dcY[4][4])
{
    if (p_mb->TransformBypassModeFlag == 1) {
        int32_t i;
        for (i = 0; i<4; ++i) {
            dcY[i][0] = c[i][0];
            dcY[i][1] = c[i][1];
            dcY[i][2] = c[i][2];
            dcY[i][3] = c[i][3];
        }
    }
    else {
        hl_memory_blocks_t* pc_mem_blocks;
        HL_ALIGNED(16) hl_int32_4x4_t *d, *f;
        int32_t i;
        // (8-324) => FIXME: perfect for SIMD (inverse transform DC luma)
        //f = m.c.m = d.m with d = m.c
#if 0
        static HL_ALIGN(HL_ALIGN_V) const int32_t m[4][4] = { {1, 1, 1, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -1, 1, -1} };
#endif
        static const int32_t kIsInterFlag0 = 0;
        static const int32_t kiYCbCr0 = 0; // luma=0, Cb=1, Cr=2

        pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

        // map() memory
        hl_memory_blocks_map(pc_mem_blocks, &d);
        hl_memory_blocks_map(pc_mem_blocks, &f);

        // d = MUL(m, c)
#if 0
        hl_math_mul4x4(m, c, (*d));
#else
        (*d)[0][0] = c[0][0] + c[1][0] + c[2][0] + c[3][0];
        (*d)[0][1] = c[0][1] + c[1][1] + c[2][1] + c[3][1];
        (*d)[0][2] = c[0][2] + c[1][2] + c[2][2] + c[3][2];
        (*d)[0][3] = c[0][3] + c[1][3] + c[2][3] + c[3][3];

        (*d)[1][0] = c[0][0] + c[1][0] - c[2][0] - c[3][0];
        (*d)[1][1] = c[0][1] + c[1][1] - c[2][1] - c[3][1];
        (*d)[1][2] = c[0][2] + c[1][2] - c[2][2] - c[3][2];
        (*d)[1][3] = c[0][3] + c[1][3] - c[2][3] - c[3][3];

        (*d)[2][0] = c[0][0] - c[1][0] - c[2][0] + c[3][0];
        (*d)[2][1] = c[0][1] - c[1][1] - c[2][1] + c[3][1];
        (*d)[2][2] = c[0][2] - c[1][2] - c[2][2] + c[3][2];
        (*d)[2][3] = c[0][3] - c[1][3] - c[2][3] + c[3][3];

        (*d)[3][0] = c[0][0] - c[1][0] + c[2][0] - c[3][0];
        (*d)[3][1] = c[0][1] - c[1][1] + c[2][1] - c[3][1];
        (*d)[3][2] = c[0][2] - c[1][2] + c[2][2] - c[3][2];
        (*d)[3][3] = c[0][3] - c[1][3] + c[2][3] - c[3][3];
#endif
        // f = MUL(d, m)
#if 0
        hl_math_mul4x4(d, m, (*f));
#else
        (*f)[0][0] = (*d)[0][0] + (*d)[0][1] + (*d)[0][2] + (*d)[0][3];
        (*f)[1][0] = (*d)[1][0] + (*d)[1][1] + (*d)[1][2] + (*d)[1][3];
        (*f)[2][0] = (*d)[2][0] + (*d)[2][1] + (*d)[2][2] + (*d)[2][3];
        (*f)[3][0] = (*d)[3][0] + (*d)[3][1] + (*d)[3][2] + (*d)[3][3];

        (*f)[0][1] = (*d)[0][0] + (*d)[0][1] - (*d)[0][2] - (*d)[0][3];
        (*f)[1][1] = (*d)[1][0] + (*d)[1][1] - (*d)[1][2] - (*d)[1][3];
        (*f)[2][1] = (*d)[2][0] + (*d)[2][1] - (*d)[2][2] - (*d)[2][3];
        (*f)[3][1] = (*d)[3][0] + (*d)[3][1] - (*d)[3][2] - (*d)[3][3];

        (*f)[0][2] = (*d)[0][0] - (*d)[0][1] - (*d)[0][2] + (*d)[0][3];
        (*f)[1][2] = (*d)[1][0] - (*d)[1][1] - (*d)[1][2] + (*d)[1][3];
        (*f)[2][2] = (*d)[2][0] - (*d)[2][1] - (*d)[2][2] + (*d)[2][3];
        (*f)[3][2] = (*d)[3][0] - (*d)[3][1] - (*d)[3][2] + (*d)[3][3];

        (*f)[0][3] = (*d)[0][0] - (*d)[0][1] + (*d)[0][2] - (*d)[0][3];
        (*f)[1][3] = (*d)[1][0] - (*d)[1][1] + (*d)[1][2] - (*d)[1][3];
        (*f)[2][3] = (*d)[2][0] - (*d)[2][1] + (*d)[2][2] - (*d)[2][3];
        (*f)[3][3] = (*d)[3][0] - (*d)[3][1] + (*d)[3][2] - (*d)[3][3];
#endif

        if (p_mb->QPy >= 36) {
            int32_t scale = p_codec->pps.pc_active->LevelScale4x4[kIsInterFlag0][kiYCbCr0][p_mb->QPy%6][0][0];
            int32_t qP_shift = qP/6-6;
            for (i=0; i<4; ++i) {
                dcY[i][0] = ((*f)[i][0] *scale) << qP_shift;
                dcY[i][1] = ((*f)[i][1] *scale) << qP_shift;
                dcY[i][2] = ((*f)[i][2] *scale) << qP_shift;
                dcY[i][3] = ((*f)[i][3] *scale) << qP_shift;
            }
        }
        else {
            int32_t scale = p_codec->pps.pc_active->LevelScale4x4[kIsInterFlag0][kiYCbCr0][p_mb->QPy%6][0][0];
            int32_t qP_plus = (1 << (5-qP/6));
            int32_t qP_shift = (6 - qP/6);
            for (i = 0; i<4; ++i) {
                dcY[i][0] = ((*f)[i][0] * scale + qP_plus) >> qP_shift;
                dcY[i][1] = ((*f)[i][1] * scale + qP_plus) >> qP_shift;
                dcY[i][2] = ((*f)[i][2] * scale + qP_plus) >> qP_shift;
                dcY[i][3] = ((*f)[i][3] * scale + qP_plus) >> qP_shift;
            }
        }

        // unmap() memory
        hl_memory_blocks_unmap(pc_mem_blocks, d);
        hl_memory_blocks_unmap(pc_mem_blocks, f);
    }
}

// FIXME: INTRIN and ASM
// 8.5.11 Scaling and transformation process for chroma DC transform coefficients
void hl_codec_264_transf_scale_chroma_dc_coeff(
    const hl_codec_264_t* p_codec,
    const hl_codec_264_mb_t* p_mb,
    const int32_t c[4][4],
    int32_t bitDepth,
    int32_t qP,
    int32_t iCbCr,
    HL_OUT int32_t dcC[4][2])
{
    int32_t i,j,f[4][4]= {0};

    if(p_mb->TransformBypassModeFlag == 1) {
        int32_t w=(p_codec->sps.pc_active->MbWidthC>>2);
        int32_t h=(p_codec->sps.pc_active->MbHeightC>>2);
        for(i=0; i<w; ++i) {
            for(j=0; j<h; ++j) {
                dcC[i][j]=c[i][j];
            }
        }
    }
    else {
        static const int32_t f2x2[2][2] = {{1,1}, {1,-1}};
        static const int32_t f4x4[4][4] = {{1,1,1,1}, {1,1,-1,-1}, {1,-1,-1,1}, {1,-1,1-1}};
        // 8.5.11.1 Transformation process for chroma DC transform coefficients
        if (p_codec->sps.pc_active->ChromaArrayType == 1) {
#if 1
            int32_t tmp[2][2];
            // (f2x2) MUL (c)
            tmp[0][0] = c[0][0] + c[1][0];
            tmp[0][1] = c[0][1] + c[1][1];
            tmp[1][0] = c[0][0] - c[1][0];
            tmp[1][1] = c[0][1] - c[1][1];
            // f = (tmp) MUL (f2x2)
            f[0][0] = tmp[0][0] + tmp[0][1];
            f[0][1] = tmp[0][0] - tmp[0][1];
            f[1][0] = tmp[1][0] + tmp[1][1];
            f[1][1] = tmp[1][0] - tmp[1][1];
#else
            // FIXME: use adds and subs -> remove the muls
            //int32_t tmp[2][2];
            // (8-328)
            // tmp = (f2x2) MUL (c)
            hl_math_mul2x2(f2x2, c, tmp);
            // f = (tmp) MUL (f2x2)
            hl_math_mul2x2(tmp, f2x2, f);
#endif
        }
        else { //ChromaArrayType==2
            // FIXME: (8-329) very good for SIMD
            int32_t tmp[4][4]= {0};
            //FIXME:hl_math_mul4x4x4x2_C
            for (i=0; i<4; ++i) {
                for (j=0; j<2; ++j) {
                    tmp[i][j]+= f4x4[i][0]*c[0][j];
                    tmp[i][j]+= f4x4[i][1]*c[1][j];
                    tmp[i][j]+= f4x4[i][2]*c[2][j];
                    tmp[i][j]+= f4x4[i][3]*c[3][j];
                }
            }
            //FIXME:hlMath_MUL4x2x2x2_C
            for (i=0; i<4; ++i) {
                f[i][0]+= tmp[i][0]*f2x2[0][0];
                f[i][0]+= tmp[i][1]*f2x2[1][0];
                f[i][1]+= tmp[i][0]*f2x2[0][1];
                f[i][1]+= tmp[i][1]*f2x2[1][1];
            }
        }

        // 8.5.11.2 Scaling process for chroma DC transform coefficients
        if (p_codec->sps.pc_active->ChromaArrayType == 1) {
            int32_t qP_mod6 = qP%6;
            int32_t qP_div6 = qP/6;
            int32_t scale = p_codec->pps.pc_active->LevelScale4x4[0][iCbCr+1][qP_mod6][0][0];
            dcC[0][0]=((f[0][0]*scale)<<qP_div6)>>5;
            dcC[0][1]=((f[0][1]*scale)<<qP_div6)>>5;
            dcC[1][0]=((f[1][0]*scale)<<qP_div6)>>5;
            dcC[1][1]=((f[1][1]*scale)<<qP_div6)>>5;
        }
        else { //ChromaArrayType == 2
            int32_t qPdc = qP+3;
            int32_t qPdc_mod6 = qPdc%6;
            if (qPdc>=36) {
                int32_t shift = qPdc/6-6;
                int32_t scale = p_codec->pps.pc_active->LevelScale4x4[0][iCbCr+1][qPdc_mod6][0][0];
                for(i=0; i<4; ++i) {
                    dcC[i][0]=(f[i][0]*scale)<<shift;
                    dcC[i][1]=(f[i][1]*scale)<<shift;
                }
            }
            else {
                int32_t mul = p_codec->pps.pc_active->LevelScale4x4[0][iCbCr+1][qPdc_mod6][0][0] + (1 << (5-qPdc/6));
                int32_t shift=(6-qPdc/6);
                for (i=0; i<4; ++i) {
                    dcC[i][0]=(f[i][0]*mul)>>shift;
                    dcC[i][1]=(f[i][1]*mul)>>shift;
                }
            }
        }
    }
}


// Forward integer transform for residual
// Forward 'core' transform: W = Cf.X.Cf_T
static void hl_codec_264_transf_frw_residual4x4_cpp(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16)int32_t out4x4[4][4])
{
#if 0
    static const HL_ALIGN(HL_ALIGN_V) int32_t Cf[4][4] = {{1,1,1,1},{2,1,-1,-2},{1,-1,-1,1},{1,-2,2,-1}};
    static const HL_ALIGN(HL_ALIGN_V) int32_t Cf_T[4][4] = {{1,2,1,1},{1,1,-1,-2},{1,-1,-1,2},{1,-2,1,-1}};
    int32_t HL_ALIGN(HL_ALIGN_V) tmp4x4[4][4];

    hl_math_mul4x4(Cf, in4x4, tmp4x4);
    hl_math_mul4x4(tmp4x4, Cf_T, out4x4);
#else
    int32_t tmp4x4[4][4];

    // tmp4x4 = MULT(Cf, in4x4)
    tmp4x4[0][0] = in4x4[0][0] + in4x4[1][0] + in4x4[2][0] + in4x4[3][0];
    tmp4x4[0][1] = in4x4[0][1] + in4x4[1][1] + in4x4[2][1] + in4x4[3][1];
    tmp4x4[0][2] = in4x4[0][2] + in4x4[1][2] + in4x4[2][2] + in4x4[3][2];
    tmp4x4[0][3] = in4x4[0][3] + in4x4[1][3] + in4x4[2][3] + in4x4[3][3];

    tmp4x4[1][0] = (in4x4[0][0]<<1) + in4x4[1][0] - in4x4[2][0] - (in4x4[3][0]<<1);
    tmp4x4[1][1] = (in4x4[0][1]<<1) + in4x4[1][1] - in4x4[2][1] - (in4x4[3][1]<<1);
    tmp4x4[1][2] = (in4x4[0][2]<<1) + in4x4[1][2] - in4x4[2][2] - (in4x4[3][2]<<1);
    tmp4x4[1][3] = (in4x4[0][3]<<1) + in4x4[1][3] - in4x4[2][3] - (in4x4[3][3]<<1);

    tmp4x4[2][0] = in4x4[0][0] - in4x4[1][0] - in4x4[2][0] + in4x4[3][0];
    tmp4x4[2][1] = in4x4[0][1] - in4x4[1][1] - in4x4[2][1] + in4x4[3][1];
    tmp4x4[2][2] = in4x4[0][2] - in4x4[1][2] - in4x4[2][2] + in4x4[3][2];
    tmp4x4[2][3] = in4x4[0][3] - in4x4[1][3] - in4x4[2][3] + in4x4[3][3];

    tmp4x4[3][0] = in4x4[0][0] - (in4x4[1][0]<<1) + (in4x4[2][0]<<1) - in4x4[3][0];
    tmp4x4[3][1] = in4x4[0][1] - (in4x4[1][1]<<1) + (in4x4[2][1]<<1) - in4x4[3][1];
    tmp4x4[3][2] = in4x4[0][2] - (in4x4[1][2]<<1) + (in4x4[2][2]<<1) - in4x4[3][2];
    tmp4x4[3][3] = in4x4[0][3] - (in4x4[1][3]<<1) + (in4x4[2][3]<<1) - in4x4[3][3];

    // out4x4 = MULT(tmp4x4, Cf_T)
    out4x4[0][0] = tmp4x4[0][0] + tmp4x4[0][1] + tmp4x4[0][2] + tmp4x4[0][3];
    out4x4[1][0] = tmp4x4[1][0] + tmp4x4[1][1] + tmp4x4[1][2] + tmp4x4[1][3];
    out4x4[2][0] = tmp4x4[2][0] + tmp4x4[2][1] + tmp4x4[2][2] + tmp4x4[2][3];
    out4x4[3][0] = tmp4x4[3][0] + tmp4x4[3][1] + tmp4x4[3][2] + tmp4x4[3][3];

    out4x4[0][1] = (tmp4x4[0][0]<<1) + tmp4x4[0][1] - tmp4x4[0][2] - (tmp4x4[0][3]<<1);
    out4x4[1][1] = (tmp4x4[1][0]<<1) + tmp4x4[1][1] - tmp4x4[1][2] - (tmp4x4[1][3]<<1);
    out4x4[2][1] = (tmp4x4[2][0]<<1) + tmp4x4[2][1] - tmp4x4[2][2] - (tmp4x4[2][3]<<1);
    out4x4[3][1] = (tmp4x4[3][0]<<1) + tmp4x4[3][1] - tmp4x4[3][2] - (tmp4x4[3][3]<<1);

    out4x4[0][2] = tmp4x4[0][0] - tmp4x4[0][1] - tmp4x4[0][2] + tmp4x4[0][3];
    out4x4[1][2] = tmp4x4[1][0] - tmp4x4[1][1] - tmp4x4[1][2] + tmp4x4[1][3];
    out4x4[2][2] = tmp4x4[2][0] - tmp4x4[2][1] - tmp4x4[2][2] + tmp4x4[2][3];
    out4x4[3][2] = tmp4x4[3][0] - tmp4x4[3][1] - tmp4x4[3][2] + tmp4x4[3][3];

    out4x4[0][3] = tmp4x4[0][0] - (tmp4x4[0][1]<<1) + (tmp4x4[0][2]<<1) - tmp4x4[0][3];
    out4x4[1][3] = tmp4x4[1][0] - (tmp4x4[1][1]<<1) + (tmp4x4[1][2]<<1) - tmp4x4[1][3];
    out4x4[2][3] = tmp4x4[2][0] - (tmp4x4[2][1]<<1) + (tmp4x4[2][2]<<1) - tmp4x4[2][3];
    out4x4[3][3] = tmp4x4[3][0] - (tmp4x4[3][1]<<1) + (tmp4x4[3][2]<<1) - tmp4x4[3][3];
#endif
}

// 4x4 Luma DC coefficient Transform (Intra16x16 pred mode only)
// Yd=(Cf.Wd.Cf_T)/2
static void hl_codec_264_transf_frw_hadamard4x4_dc_luma_cpp(HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    // INFO: CF and CFT are equal since it's symetrict
#if 0
    static const HL_ALIGN(HL_ALIGN_V) int32_t Cf[4][4] = {{1,1,1,1},{1,1,-1,-1},{1,-1,-1,1},{1,-1,1,-1}};
    static const HL_ALIGN(HL_ALIGN_V) int32_t Cf_T[4][4] = {{1,1,1,1},{1,1,-1,-1},{1,-1,-1,1},{1,-1,1,-1}};
    int32_t HL_ALIGN(HL_ALIGN_V) tmp4x4[4][4], i;
    hl_math_mul4x4(Cf, in4x4, tmp4x4);
    hl_math_mul4x4(tmp4x4, Cf_T, out4x4);
    for (i=0; i<4; ++i) {
        out4x4[i][0]>>=1;
        out4x4[i][1]>>=1;
        out4x4[i][2]>>=1;
        out4x4[i][3]>>=1;
    }
#else
    int32_t tmp4x4[4][4];

    // tmp4x4 = MULT(Cf, in4x4)
    tmp4x4[0][0] = in4x4[0][0] + in4x4[1][0] + in4x4[2][0] + in4x4[3][0];
    tmp4x4[0][1] = in4x4[0][1] + in4x4[1][1] + in4x4[2][1] + in4x4[3][1];
    tmp4x4[0][2] = in4x4[0][2] + in4x4[1][2] + in4x4[2][2] + in4x4[3][2];
    tmp4x4[0][3] = in4x4[0][3] + in4x4[1][3] + in4x4[2][3] + in4x4[3][3];

    tmp4x4[1][0] = in4x4[0][0] + in4x4[1][0] - in4x4[2][0] - in4x4[3][0];
    tmp4x4[1][1] = in4x4[0][1] + in4x4[1][1] - in4x4[2][1] - in4x4[3][1];
    tmp4x4[1][2] = in4x4[0][2] + in4x4[1][2] - in4x4[2][2] - in4x4[3][2];
    tmp4x4[1][3] = in4x4[0][3] + in4x4[1][3] - in4x4[2][3] - in4x4[3][3];

    tmp4x4[2][0] = in4x4[0][0] - in4x4[1][0] - in4x4[2][0] + in4x4[3][0];
    tmp4x4[2][1] = in4x4[0][1] - in4x4[1][1] - in4x4[2][1] + in4x4[3][1];
    tmp4x4[2][2] = in4x4[0][2] - in4x4[1][2] - in4x4[2][2] + in4x4[3][2];
    tmp4x4[2][3] = in4x4[0][3] - in4x4[1][3] - in4x4[2][3] + in4x4[3][3];

    tmp4x4[3][0] = in4x4[0][0] - in4x4[1][0] + in4x4[2][0] - in4x4[3][0];
    tmp4x4[3][1] = in4x4[0][1] - in4x4[1][1] + in4x4[2][1] - in4x4[3][1];
    tmp4x4[3][2] = in4x4[0][2] - in4x4[1][2] + in4x4[2][2] - in4x4[3][2];
    tmp4x4[3][3] = in4x4[0][3] - in4x4[1][3] + in4x4[2][3] - in4x4[3][3];

    // out4x4 = MULT(tmp4x4, Cf_T)
    out4x4[0][0] = tmp4x4[0][0] + tmp4x4[0][1] + tmp4x4[0][2] + tmp4x4[0][3];
    out4x4[1][0] = tmp4x4[1][0] + tmp4x4[1][1] + tmp4x4[1][2] + tmp4x4[1][3];
    out4x4[2][0] = tmp4x4[2][0] + tmp4x4[2][1] + tmp4x4[2][2] + tmp4x4[2][3];
    out4x4[3][0] = tmp4x4[3][0] + tmp4x4[3][1] + tmp4x4[3][2] + tmp4x4[3][3];

    out4x4[0][1] = tmp4x4[0][0] + tmp4x4[0][1] - tmp4x4[0][2] - tmp4x4[0][3];
    out4x4[1][1] = tmp4x4[1][0] + tmp4x4[1][1] - tmp4x4[1][2] - tmp4x4[1][3];
    out4x4[2][1] = tmp4x4[2][0] + tmp4x4[2][1] - tmp4x4[2][2] - tmp4x4[2][3];
    out4x4[3][1] = tmp4x4[3][0] + tmp4x4[3][1] - tmp4x4[3][2] - tmp4x4[3][3];

    out4x4[0][2] = tmp4x4[0][0] - tmp4x4[0][1] - tmp4x4[0][2] + tmp4x4[0][3];
    out4x4[1][2] = tmp4x4[1][0] - tmp4x4[1][1] - tmp4x4[1][2] + tmp4x4[1][3];
    out4x4[2][2] = tmp4x4[2][0] - tmp4x4[2][1] - tmp4x4[2][2] + tmp4x4[2][3];
    out4x4[3][2] = tmp4x4[3][0] - tmp4x4[3][1] - tmp4x4[3][2] + tmp4x4[3][3];

    out4x4[0][3] = tmp4x4[0][0] - tmp4x4[0][1] + tmp4x4[0][2] - tmp4x4[0][3];
    out4x4[1][3] = tmp4x4[1][0] - tmp4x4[1][1] + tmp4x4[1][2] - tmp4x4[1][3];
    out4x4[2][3] = tmp4x4[2][0] - tmp4x4[2][1] + tmp4x4[2][2] - tmp4x4[2][3];
    out4x4[3][3] = tmp4x4[3][0] - tmp4x4[3][1] + tmp4x4[3][2] - tmp4x4[3][3];

    // out4x4[i][j] >>= 1;
    out4x4[0][0]>>=1, out4x4[0][1]>>=1, out4x4[0][2]>>=1, out4x4[0][3]>>=1;
    out4x4[1][0]>>=1, out4x4[1][1]>>=1, out4x4[1][2]>>=1, out4x4[1][3]>>=1;
    out4x4[2][0]>>=1, out4x4[2][1]>>=1, out4x4[2][2]>>=1, out4x4[2][3]>>=1;
    out4x4[3][0]>>=1, out4x4[3][1]>>=1, out4x4[3][2]>>=1, out4x4[3][3]>>=1;
#endif
}

// 2x2 Chroma DC coefficient
void hl_codec_264_transf_frw_hadamard2x2_dc_chroma(HL_ALIGNED(16) const int32_t in2x2[2][2], HL_ALIGNED(16) int32_t out2x2[2][2])
{
    // TODO: use SIMD (doesn't worth it)
    // INFO: CF and CFT are equal since it's symetrict
#if 0
    static HL_ALIGN(HL_ALIGN_V) int32_t Cf[2][2] = {{1,1},{1,-1}};
    static HL_ALIGN(HL_ALIGN_V) int32_t Cf_T[2][2] = {{1,1},{1,-1}};
    HL_ALIGN(HL_ALIGN_V) int32_t tmp2x2[2][2];

    hl_math_mul2x2(Cf, in2x2, tmp2x2);
    hl_math_mul2x2(tmp2x2, Cf_T, out2x2);
#else
    int32_t tmp2x2[2][2];

    // tmp2x2 = Cf * in2x2
    tmp2x2[0][0] = in2x2[0][0] + in2x2[1][0];
    tmp2x2[0][1] = in2x2[0][1] + in2x2[1][1];
    tmp2x2[1][0] = in2x2[0][0] - in2x2[1][0];
    tmp2x2[1][1] = in2x2[0][1] - in2x2[1][1];
    // out2x2 = tmp2x2 * Cf_T
    out2x2[0][0] = tmp2x2[0][0] + tmp2x2[0][1];
    out2x2[0][1] = tmp2x2[0][0] - tmp2x2[0][1];
    out2x2[1][0] = tmp2x2[1][0] + tmp2x2[1][1];
    out2x2[1][1] = tmp2x2[1][0] - tmp2x2[1][1];

#endif
}
