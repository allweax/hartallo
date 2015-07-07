#include "hartallo/h264/hl_codec_264_quant.h"
#include "hartallo/h264/hl_codec_264_mb.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

#if HL_HAVE_X86_INTRIN
#include "hartallo/h264/intrinsics/x86/hl_codec_x86_264_quant_intrin.h"
#endif

static void hl_codec_264_quant_frw4x4_scale_ac_cpp(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);
static void hl_codec_264_quant_frw4x4_scale_dc_luma_cpp(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);
static void hl_codec_264_quant_frw2x2_scale_dc_chroma_cpp(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[2][2], HL_ALIGNED(16) int32_t out2x2[2][2]);

void (*hl_codec_264_quant_scale_residual4x4)(const struct hl_codec_264_s* pc_codec, const struct hl_codec_264_mb_s* pc_mb, int32_t bitDepth, int32_t qP, const int32_t c[4][4], hl_bool_t luma, hl_bool_t Intra16x16, int32_t CbCrIdx, /* out */int32_t d[4][4]) = hl_codec_264_quant_scale_residual4x4_cpp;
void (*hl_codec_264_quant_frw4x4_scale_ac)(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]) = hl_codec_264_quant_frw4x4_scale_ac_cpp;
void (*hl_codec_264_quant_frw4x4_scale_dc_luma)(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]) = hl_codec_264_quant_frw4x4_scale_dc_luma_cpp;
void (*hl_codec_264_quant_frw2x2_scale_dc_chroma)(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[2][2], HL_ALIGNED(16) int32_t out2x2[2][2]) = hl_codec_264_quant_frw2x2_scale_dc_chroma_cpp;

HL_ERROR_T hl_codec_264_quant_init_funcs()
{
    HL_DEBUG_INFO("Initializing quantization functions");

    hl_codec_264_quant_scale_residual4x4 = hl_codec_264_quant_scale_residual4x4_cpp;
    hl_codec_264_quant_frw4x4_scale_ac = hl_codec_264_quant_frw4x4_scale_ac_cpp;
    hl_codec_264_quant_frw4x4_scale_dc_luma = hl_codec_264_quant_frw4x4_scale_dc_luma_cpp;
    hl_codec_264_quant_frw2x2_scale_dc_chroma = hl_codec_264_quant_frw2x2_scale_dc_chroma_cpp;

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        hl_codec_264_quant_frw4x4_scale_ac = hl_codec_264_quant_frw4x4_scale_ac_intin_sse2;
        hl_codec_264_quant_frw4x4_scale_dc_luma = hl_codec_264_quant_frw4x4_scale_dc_luma_intin_sse2;
        hl_codec_264_quant_frw2x2_scale_dc_chroma = hl_codec_264_quant_frw2x2_scale_dc_chroma_intin_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSSE3)) {

    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        hl_codec_264_quant_scale_residual4x4 = hl_codec_x86_264_quant_scale_residual4x4_intin_sse41;
        hl_codec_264_quant_frw4x4_scale_ac = hl_codec_264_quant_frw4x4_scale_ac_intin_sse41;
        hl_codec_264_quant_frw4x4_scale_dc_luma = hl_codec_264_quant_frw4x4_scale_dc_luma_intin_sse41;
        hl_codec_264_quant_frw2x2_scale_dc_chroma = hl_codec_264_quant_frw2x2_scale_dc_chroma_intin_sse41;
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
    }
    if (hl_cpu_flags_test(kCpuFlagSSSE3)) {
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
		extern void hl_codec_264_quant_frw4x4_scale_ac_asm_sse41(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4]);

        hl_codec_264_quant_frw4x4_scale_ac = hl_codec_264_quant_frw4x4_scale_ac_asm_sse41;
    }
#endif /* HL_HAVE_X86_ASM */

    return HL_ERROR_SUCCESS;
}

// 8.5.12.1 Scaling process for residual 4x4 blocks
void hl_codec_264_quant_scale_residual4x4_cpp(
    const struct hl_codec_264_s* pc_codec,
    const struct hl_codec_264_mb_s* pc_mb,
    int32_t bitDepth,
    int32_t qP,
    const int32_t c[4][4],
    hl_bool_t luma,
    hl_bool_t Intra16x16,
    int32_t CbCrIdx,
    /* out */int32_t d[4][4])
{
    int32_t i;
    int32_t qP_div6 = qP/6;
    int32_t qP_mod6 = qP % 6;
    int32_t mbIsInterFlag = HL_CODEC_264_MB_TYPE_IS_INTER(pc_mb) ? 1 : 0;
    int32_t iYCbCr = pc_codec->sps.pc_active->separate_colour_plane_flag ? pc_codec->layers.pc_active->pc_slice_hdr->colour_plane_id : (luma ? 0 : CbCrIdx + 1); // Luma=0,Cb=1,Cr=2

    (bitDepth);
    (i);

    if (qP >= 24) {
        int32_t qP_shift = (qP_div6-4);
        for (i=0; i<4; ++i) {
            d[i][0] = (c[i][0] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][0]) << qP_shift;
            d[i][1] = (c[i][1] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][1]) << qP_shift;
            d[i][2] = (c[i][2] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][2]) << qP_shift;
            d[i][3] = (c[i][3] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][3]) << qP_shift;
        }
    }
    else {
        int32_t qP_offset = (1 << (3 - qP_div6)); // 2^(3-qp/6)
        int32_t qP_shift = (4 - qP_div6); //(4 - qP/6)
        for (i=0; i<4; ++i) {
            d[i][0] = (c[i][0] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][0] + qP_offset) >> qP_shift;
            d[i][1] = (c[i][1] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][1] + qP_offset) >> qP_shift;
            d[i][2] = (c[i][2] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][2] + qP_offset) >> qP_shift;
            d[i][3] = (c[i][3] * pc_codec->pps.pc_active->LevelScale4x4[mbIsInterFlag][iYCbCr][qP_mod6][i][3] + qP_offset) >> qP_shift;
        }
    }

    if ((luma && Intra16x16) || !luma) {
        d[0][0] = c[0][0];
    }
}


// TODO: add ASM version
// Post-Scaling + Quantization (excluding 16x16-Intra)
static void hl_codec_264_quant_frw4x4_scale_ac_cpp(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t in4x4[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    // |Zij] = (|Wij|.MF+f) >> qbits
    // sign(Zij) = sign(Wij)
    // qbits = 15 + floor(QP/6)
    // f = (2^qbits)/3 for intra and (2^qbits)/6 for Inter block
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index=QP%6;
    int32_t i,Z;

    for (i=0; i<4; ++i) {
        Z = (HL_MATH_ABS_INT32(in4x4[i][0]) * HL_CODEC_264_QUANT_MF[mf_index][i][0] + f) >> qBits;
        out4x4[i][0] = Z*HL_MATH_SIGN(in4x4[i][0]);
        Z = (HL_MATH_ABS_INT32(in4x4[i][1]) * HL_CODEC_264_QUANT_MF[mf_index][i][1] + f) >> qBits;
        out4x4[i][1] = Z*HL_MATH_SIGN(in4x4[i][1]);
        Z = (HL_MATH_ABS_INT32(in4x4[i][2]) * HL_CODEC_264_QUANT_MF[mf_index][i][2] + f) >> qBits;
        out4x4[i][2] = Z*HL_MATH_SIGN(in4x4[i][2]);
        Z = (HL_MATH_ABS_INT32(in4x4[i][3]) * HL_CODEC_264_QUANT_MF[mf_index][i][3] + f) >> qBits;
        out4x4[i][3] = Z*HL_MATH_SIGN(in4x4[i][3]);
    }
}

// TODO: add ASM version
// Post-Scaling + Quantization
static void hl_codec_264_quant_frw4x4_scale_dc_luma_cpp(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[4][4], HL_ALIGNED(16) int32_t out4x4[4][4])
{
    //|Zij]=(|Yij|.MF00+2f)>>(qbits+1)
    //sign(Zij)= Sign(Yij)

    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t qBits_plus1 = qBits+1;
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index = QP%6;
    const int32_t mf_00 = HL_CODEC_264_QUANT_MF[mf_index][0][0];
    const int32_t _2f = f<<1;
    int32_t i, Z;

    for (i=0; i<4; ++i) {
        Z = (HL_MATH_ABS_INT32(dcC[i][0])*mf_00+_2f)>>qBits_plus1;
        out4x4[i][0] = Z*HL_MATH_SIGN(dcC[i][0]);
        Z = (HL_MATH_ABS_INT32(dcC[i][1])*mf_00+_2f)>>qBits_plus1;
        out4x4[i][1] = Z*HL_MATH_SIGN(dcC[i][1]);
        Z = (HL_MATH_ABS_INT32(dcC[i][2])*mf_00+_2f)>>qBits_plus1;
        out4x4[i][2] = Z*HL_MATH_SIGN(dcC[i][2]);
        Z = (HL_MATH_ABS_INT32(dcC[i][3])*mf_00+_2f)>>qBits_plus1;
        out4x4[i][3] = Z*HL_MATH_SIGN(dcC[i][3]);
    }
}

// TODO: add ASM version
// Post-Scaling + Quantization
static void hl_codec_264_quant_frw2x2_scale_dc_chroma_cpp(int32_t QP, hl_bool_t isIntraBlk, HL_ALIGNED(16) const int32_t dcC[2][2], HL_ALIGNED(16) int32_t out2x2[2][2])
{
    //|Zij]=(|Yij|.MF00+2f)>>(qbits+1)
    //sign(Zij)= Sign(Yij)

    //FIXME: use SIMD
    const int32_t qBits = HL_CODEC_264_QUANT_QBITS[QP];
    const int32_t qBits_plus1=qBits+1;
    const int32_t f = HL_CODEC_264_QUANT_F[isIntraBlk ? 1 : 0][QP];
    const int32_t mf_index=QP%6;
    const int32_t mf_00=HL_CODEC_264_QUANT_MF[mf_index][0][0];
    const int32_t _2f=f<<1;
    int32_t Z;

    Z=(HL_MATH_ABS_INT32(dcC[0][0])*mf_00+_2f)>>qBits_plus1;
    out2x2[0][0] = Z*HL_MATH_SIGN(dcC[0][0]);
    Z=(HL_MATH_ABS_INT32(dcC[0][1])*mf_00+_2f)>>qBits_plus1;
    out2x2[0][1] = Z*HL_MATH_SIGN(dcC[0][1]);
    Z=(HL_MATH_ABS_INT32(dcC[1][0])*mf_00+_2f)>>qBits_plus1;
    out2x2[1][0] = Z*HL_MATH_SIGN(dcC[1][0]);
    Z=(HL_MATH_ABS_INT32(dcC[1][1])*mf_00+_2f)>>qBits_plus1;
    out2x2[1][1] = Z*HL_MATH_SIGN(dcC[1][1]);
}