#ifndef _HARTALLO_CODEC_264_TABLES_H_
#define _HARTALLO_CODEC_264_TABLES_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/h264/hl_codec_264_defs.h"

// Guard against adding this header in the source file
#define HARTALLO_CODEC_264_TABLES_H

HL_BEGIN_DECLS

static const int32_t INTRA_LUMA_NEIGHBOURING_SAMPLES4X4_X[13] = {-1, -1, -1, -1, -1, +0, +1, +2, +3, +4, +5, +6, +7};
static const int32_t INTRA_LUMA_NEIGHBOURING_SAMPLES4X4_Y[13] = {-1, +0, +1, +2, +3, -1, -1, -1, -1, -1, -1, -1, -1};
static const int32_t INTRA_LUMA_NEIGHBOURING_SAMPLES16X16_X[33] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
static const int32_t INTRA_LUMA_NEIGHBOURING_SAMPLES16X16_Y[33] = {
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
static const int32_t INTRA_CHROMA_NEIGHBOURING_SAMPLES_X[17] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,
    0, 1, 2, 3, 4, 5, 6, 7
};
static const int32_t INTRA_CHROMA_NEIGHBOURING_SAMPLES_Y[17] = {
    -1, 0, 1, 2, 3, 4, 5, 6, 7,
    -1,-1,-1,-1,-1,-1,-1,-1,
};

static const int8_t HL_CODEC_264_START_CODE_PREFIX[3] = { 0x00, 0x00, 0x01 };

// JVT-O079 2.1.3.1.2.2 Early termination
static const double HL_CODEC_264_EARLY_TERMINATION_APLHA1[8/*Mode=1...7*/] = {0.00/*placeholder*/, 0.01, 0.01,0.01, 0.02, 0.03, 0.03, 0.04 };
static const double HL_CODEC_264_EARLY_TERMINATION_APLHA2[8/*Mode=1...7*/] = {0.00/*placeholder*/, 0.06, 0.07, 0.07, 0.08, 0.12, 0.11, 0.15 };

// (8-319)
static const int32_t HL_CODEC_264_SCALING_MATRIX_V[6][3] = {
    {10, 16, 13},
    {11, 18, 14},
    {13, 20, 16},
    {14, 23, 18},
    {16, 25, 20},
    {18, 29, 23},
};

// Table 6-2 – Specification of input and output assignments for subclauses 6.4.10.1 to 6.4.10.7
// HL_H264_xD_yD[A=0/B=1/C=2/D=3][xD=0/yD=1]
static const int32_t xD_yD[4][2] = {
    //N=A
    {-1, 0},
    //N=B
    {0, -1},
    //N=C
    {/*FIXME:predPartWidth*/-1, -1},
    //N=D
    {-1, -1}
};

static const int32_t dcYij[16][2] = {// Figure 8-6
    {0,0},{0,1},{1,0},{1,1},
    {0,2},{0,3},{1,2},{1,3},
    {2,0},{2,1},{3,0},{3,1},
    {2,2},{2,3},{3,2},{3,3},
};

// 6.4.3 Inverse 4x4 luma block scanning process
static const int32_t Inverse4x4LumaBlockScanOrder[16] = {
    0,	1,	4,	5,
    2,	3,	6,	7,
    8,	9,	12,	13,
    10,	11,	14,	15
};
// 6.4.3 Inverse 4x4 luma block scanning process
static const int32_t Inverse4x4LumaBlockScanOrderXY[16/*luma4x4BlkIdx*/][2/*x,y*/] = {
    {0, 0}, {4, 0}, {0, 4}, {4, 4},
    {8, 0}, {12, 0}, {8, 4}, {12, 4},
    {0, 8}, {4, 8}, {0, 12}, {4, 12},
    {8, 8}, {12, 8}, {8, 12}, {12, 12}
};
static const int32_t Raster4x4LumaBlockScanOrderXY[16/*luma4x4BlkIdx*/][2/*x,y*/] = {
    {0, 0}, {4, 0}, {8, 0}, {12, 0},
    {0, 4}, {4, 4}, {8, 4}, {12, 4},
    {0, 8}, {4, 8}, {8, 8}, {12, 8},
    {0, 12}, {4, 12}, {8, 12}, {12, 12}
};

// Table 7-3 – Specification of default scaling lists Default_4x4_Intra and Default_4x4_Inter
static const int32_t Scaling_Default_4x4_Intra[16] = { 6, 13, 13, 20, 20, 20, 28, 28, 28, 28, 32, 32, 32, 37, 37, 42 };
static const int32_t Scaling_Default_4x4_Inter[16] = { 10, 14, 14, 20, 20, 20, 24, 24, 24, 24, 27, 27, 27, 30, 30, 34 };
// Table 7-4 – Specification of default scaling lists Default_8x8_Intra and Default_8x8_Inter
static const int32_t Scaling_Default_8x8_Intra[64] = {
    6, 10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
    23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
    27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
    31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42
};
static const int32_t Scaling_Default_8x8_Inter[64] = {
    9, 13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
    21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
    24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
    27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35,
};

// 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists
// Table 8-13 – Specification of mapping of idx to cij for zig-zag and field scan
static const int32_t ZigZag4x4BlockScanYX[16][2] = {
    {0,0}, {0,1}, {1,0}, {2,0},
    {1,1}, {0,2}, {0,3}, {1,2},
    {2,1}, {3,0}, {3,1}, {2,2},
    {1,3}, {2,3}, {3,2}, {3,3}
};
static const int32_t ZigZag4x4FieldScanYX[16][2] = {
    {0,0}, {1,0}, {0,1}, {2,0},
    {3,0}, {1,1}, {2,1}, {3,1},
    {0,2}, {1,2}, {2,2}, {3,2},
    {0,3}, {1,3}, {2,3}, {3,3}
};
static const int32_t ZigZag2x2BlockScanYX[4][2] = {
    {0,0}, {0,1},
    {1,0}, {1,1},
};

// Table 8-15 – Specification of QPC as a function of qPI
static const int32_t qPI2QPC[52/*qPI*/] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39
};

// Multiplication factor (MF) used for 4x4 forward qantization
extern /*static*/ const HL_ALIGN(HL_ALIGN_V) int32_t HL_CODEC_264_QUANT_MF[6/*QP%6*/][4][4];

// "F"actor (for rounding) used in 4x4 forward qantization
extern /*static*/ const HL_ALIGN(HL_ALIGN_V) int32_t HL_CODEC_264_QUANT_F[2/*INTRA?*/][52/*QP*/];
// "qBits" used in 4x4 forward qantization
extern /*static*/ const HL_ALIGN(HL_ALIGN_V) int32_t HL_CODEC_264_QUANT_QBITS[52/*QP*/];

// Table A-1 – Level limits
static const int32_t MaxMBPS[HL_CODEC_264_LEVEL_MAX_COUNT] = { 1485, 1485, 3000, 6000, 11880, 11880, 19800, 20250, 40500, 108000, 216000, 245760, 245760, 522240, 589824, 983040 };
static const int32_t MaxFS[HL_CODEC_264_LEVEL_MAX_COUNT] =  { 99, 99, 396, 396, 396, 396, 792, 1620, 1620, 3600, 5120, 8192, 8192, 8704, 22080, 36864 };
static const int32_t MaxDpbMbs[HL_CODEC_264_LEVEL_MAX_COUNT] = { 396, 396, 900, 2376, 2376, 2376, 4752, 8100, 8100, 18000, 20480, 32768, 32768, 34816, 110400, 184320 };
static const int32_t MaxBR[HL_CODEC_264_LEVEL_MAX_COUNT] = { 64, 128, 192, 384, 768, 2000, 4000, 4000, 10000, 14000, 20000, 20000, 50000, 50000, 135000, 240000 };
static const int32_t MaxCPB[HL_CODEC_264_LEVEL_MAX_COUNT] = { 175, 350, 500, 1000, 2000, 2000, 4000, 4000, 10000, 14000, 20000, 25000, 62500, 62500, 135000, 240000 };
static const int32_t MinCR[HL_CODEC_264_LEVEL_MAX_COUNT] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 2, 2, 2, 2 };

// From "enum HL_CODEC_264_LEVEL_E" to zero-based index
// level_idc = (level.prefix * 10) + level.suffix. For example: 3.1 = 31. Exception for 1b = 9.
// usage: int32_t maxDpbMbs = MaxDpbMbs[HL_CODEC_264_LEVEL_TO_ZERO_BASED_INDEX[level_idc]];
static const int32_t HL_CODEC_264_LEVEL_TO_ZERO_BASED_INDEX[255] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 3, 4, 4, 4, 4, 4,
    4, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 9, 10, 10, 10, 10, 10,
    10, 10, 10, 11, 12, 13, 13, 13, 13, 13, 13, 13, 13, 14, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16
};


// lookup table for "InverseRasterScan()"
// InverseRasterScan(index, 4, 4, pitch, y_dim)
// "pitch" must be equal or less than "16"
static const int32_t InverseRasterScan16_4x4[16/*index*/][16/*pitch*/][2/*y_dim*/] = {
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 4},{0, 4},{0, 4},{0, 4},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 8},{0, 8},{0, 8},{0, 8},{0, 4},{0, 4},{0, 4},{0, 4},{8, 0},{8, 0},{8, 0},{8, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 12},{0, 12},{0, 12},{0, 12},{4, 4},{4, 4},{4, 4},{4, 4},{0, 4},{0, 4},{0, 4},{0, 4}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 16},{0, 16},{0, 16},{0, 16},{0, 8},{0, 8},{0, 8},{0, 8},{4, 4},{4, 4},{4, 4},{4, 4}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 20},{0, 20},{0, 20},{0, 20},{4, 8},{4, 8},{4, 8},{4, 8},{8, 4},{8, 4},{8, 4},{8, 4}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 24},{0, 24},{0, 24},{0, 24},{0, 12},{0, 12},{0, 12},{0, 12},{0, 8},{0, 8},{0, 8},{0, 8}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 28},{0, 28},{0, 28},{0, 28},{4, 12},{4, 12},{4, 12},{4, 12},{4, 8},{4, 8},{4, 8},{4, 8}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 32},{0, 32},{0, 32},{0, 32},{0, 16},{0, 16},{0, 16},{0, 16},{8, 8},{8, 8},{8, 8},{8, 8}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 36},{0, 36},{0, 36},{0, 36},{4, 16},{4, 16},{4, 16},{4, 16},{0, 12},{0, 12},{0, 12},{0, 12}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 40},{0, 40},{0, 40},{0, 40},{0, 20},{0, 20},{0, 20},{0, 20},{4, 12},{4, 12},{4, 12},{4, 12}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 44},{0, 44},{0, 44},{0, 44},{4, 20},{4, 20},{4, 20},{4, 20},{8, 12},{8, 12},{8, 12},{8, 12}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 48},{0, 48},{0, 48},{0, 48},{0, 24},{0, 24},{0, 24},{0, 24},{0, 16},{0, 16},{0, 16},{0, 16}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 52},{0, 52},{0, 52},{0, 52},{4, 24},{4, 24},{4, 24},{4, 24},{4, 16},{4, 16},{4, 16},{4, 16}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 56},{0, 56},{0, 56},{0, 56},{0, 28},{0, 28},{0, 28},{0, 28},{8, 16},{8, 16},{8, 16},{8, 16}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 60},{0, 60},{0, 60},{0, 60},{4, 28},{4, 28},{4, 28},{4, 28},{0, 20},{0, 20},{0, 20},{0, 20}},
};
// lookup table for "InverseRasterScan()"
// InverseRasterScan(index, 2, 2, pitch, y_dim)
// "pitch" must be equal or less than "16"
static const int32_t InverseRasterScan16_2x2[16/*index*/][16/*pitch*/][2/*y_dim*/] = {
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0},{2, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 2},{0, 2},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0},{4, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 2},{2, 2},{0, 2},{0, 2},{6, 0},{6, 0},{6, 0},{6, 0},{6, 0},{6, 0},{6, 0},{6, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 4},{0, 4},{2, 2},{2, 2},{0, 2},{0, 2},{8, 0},{8, 0},{8, 0},{8, 0},{8, 0},{8, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 4},{2, 4},{4, 2},{4, 2},{2, 2},{2, 2},{0, 2},{0, 2},{10, 0},{10, 0},{10, 0},{10, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 6},{0, 6},{0, 4},{0, 4},{4, 2},{4, 2},{2, 2},{2, 2},{0, 2},{0, 2},{12, 0},{12, 0}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 6},{2, 6},{2, 4},{2, 4},{6, 2},{6, 2},{4, 2},{4, 2},{2, 2},{2, 2},{0, 2},{0, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 8},{0, 8},{4, 4},{4, 4},{0, 4},{0, 4},{6, 2},{6, 2},{4, 2},{4, 2},{2, 2},{2, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 8},{2, 8},{0, 6},{0, 6},{2, 4},{2, 4},{8, 2},{8, 2},{6, 2},{6, 2},{4, 2},{4, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 10},{0, 10},{2, 6},{2, 6},{4, 4},{4, 4},{0, 4},{0, 4},{8, 2},{8, 2},{6, 2},{6, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 10},{2, 10},{4, 6},{4, 6},{6, 4},{6, 4},{2, 4},{2, 4},{10, 2},{10, 2},{8, 2},{8, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 12},{0, 12},{0, 8},{0, 8},{0, 6},{0, 6},{4, 4},{4, 4},{0, 4},{0, 4},{10, 2},{10, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 12},{2, 12},{2, 8},{2, 8},{2, 6},{2, 6},{6, 4},{6, 4},{2, 4},{2, 4},{12, 2},{12, 2}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{0, 14},{0, 14},{4, 8},{4, 8},{4, 6},{4, 6},{8, 4},{8, 4},{4, 4},{4, 4},{0, 4},{0, 4}},
    {{0, 0},{0, 0},{0, 0},{0, 0},{2, 14},{2, 14},{0, 10},{0, 10},{6, 6},{6, 6},{0, 6},{0, 6},{6, 4},{6, 4},{2, 4},{2, 4}}
};

// lookup table for "hl264_derivation_process_for_4x4_luma_block_indices"
static const int32_t LumaBlockIndices4x4_YX[16/*yP*/][16/*xP*/] = {
    {0, 0, 0, 0, 1, 1, 1, 1, 4, 4, 4, 4, 5, 5, 5, 5},
    {0, 0, 0, 0, 1, 1, 1, 1, 4, 4, 4, 4, 5, 5, 5, 5},
    {0, 0, 0, 0, 1, 1, 1, 1, 4, 4, 4, 4, 5, 5, 5, 5},
    {0, 0, 0, 0, 1, 1, 1, 1, 4, 4, 4, 4, 5, 5, 5, 5},
    {2, 2, 2, 2, 3, 3, 3, 3, 6, 6, 6, 6, 7, 7, 7, 7},
    {2, 2, 2, 2, 3, 3, 3, 3, 6, 6, 6, 6, 7, 7, 7, 7},
    {2, 2, 2, 2, 3, 3, 3, 3, 6, 6, 6, 6, 7, 7, 7, 7},
    {2, 2, 2, 2, 3, 3, 3, 3, 6, 6, 6, 6, 7, 7, 7, 7},
    {8, 8, 8, 8, 9, 9, 9, 9, 12, 12, 12, 12, 13, 13, 13, 13},
    {8, 8, 8, 8, 9, 9, 9, 9, 12, 12, 12, 12, 13, 13, 13, 13},
    {8, 8, 8, 8, 9, 9, 9, 9, 12, 12, 12, 12, 13, 13, 13, 13},
    {8, 8, 8, 8, 9, 9, 9, 9, 12, 12, 12, 12, 13, 13, 13, 13},
    {10, 10, 10, 10, 11, 11, 11, 11, 14, 14, 14, 14, 15, 15, 15, 15},
    {10, 10, 10, 10, 11, 11, 11, 11, 14, 14, 14, 14, 15, 15, 15, 15},
    {10, 10, 10, 10, 11, 11, 11, 11, 14, 14, 14, 14, 15, 15, 15, 15},
    {10, 10, 10, 10, 11, 11, 11, 11, 14, 14, 14, 14, 15, 15, 15, 15}
};

// lookup table for "6.4.12.2"
static const int32_t ChromaBlockIndices4x4_YX[16/*yP*/][16/*xP*/] = {
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3},
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3},
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3},
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3},
    {2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5},
    {2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5},
    {2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5},
    {2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5},
    {4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7},
    {4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7},
    {4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7},
    {4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7},
    {6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9},
    {6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9},
    {6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9},
    {6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9},
};

// lookup table for "6.4.12.3"
static const int32_t LumaBlockIndices8x8_YX[16/*yP*/][16/*xP*/] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
};

/* Table 7-11 – Macroblock types for I slices ("mb_type") */
// cbpc = CodedBlockPatternChroma, cbpl = CodedBlockPatternLuma
static const int32_t HL_CODEC_264_MB_I_TABLE[2/*transform_size_8x8_flag*/][26/*mb_type*/][5/*type,mode,imode16x16,cbpc,cbpl*/] = {
    // transform_size_8x8_flag==0
    {
        {HL_CODEC_264_MB_TYPE_I_NXN,		 HL_CODEC_264_MB_MODE_INTRA_4X4,	HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_PCM		   , HL_CODEC_264_MB_MODE_NA,		   HL_CODEC_264_NA,  HL_CODEC_264_NA, HL_CODEC_264_NA},
    },
    // transform_size_8x8_flag==1
    {
        {HL_CODEC_264_MB_TYPE_I_NXN, HL_CODEC_264_MB_MODE_INTRA_8X8,			HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_0_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			0,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_1_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			1,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_2_0, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			2,			0},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_0_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			0,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_1_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			1,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_0_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	0,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_1_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	1,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_2_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	2,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_16X16_3_2_1, HL_CODEC_264_MB_MODE_INTRA_16X16,	3,			2,			15},
        {HL_CODEC_264_MB_TYPE_I_PCM		   , HL_CODEC_264_MB_MODE_NA,		   HL_CODEC_264_NA,  HL_CODEC_264_NA, HL_CODEC_264_NA},
    }
};

// Table 7-14 – Macroblock type values 0 to 22 for B slices ("mb_type")
static const enum HL_CODEC_264_MB_TYPE_E HL_CODEC_264_MB_TYPES_B[23 /*"mb_type" when slice type is B*/] = {
            HL_CODEC_264_MB_TYPE_B_DIRECT_16X16,
            HL_CODEC_264_MB_TYPE_B_L0_16X16,
            HL_CODEC_264_MB_TYPE_B_L1_16X16,
            HL_CODEC_264_MB_TYPE_B_BI_16X16,
            HL_CODEC_264_MB_TYPE_B_L0_L0_16X8,
            HL_CODEC_264_MB_TYPE_B_L0_L0_8X16,
            HL_CODEC_264_MB_TYPE_B_L1_L1_16X8,
            HL_CODEC_264_MB_TYPE_B_L1_L1_8X16,
            HL_CODEC_264_MB_TYPE_B_L0_L1_16X8,
            HL_CODEC_264_MB_TYPE_B_L0_L1_8X16,
            HL_CODEC_264_MB_TYPE_B_L1_L0_16X8,
            HL_CODEC_264_MB_TYPE_B_L1_L0_8X16,
            HL_CODEC_264_MB_TYPE_B_L0_BI_16X8,
            HL_CODEC_264_MB_TYPE_B_L0_BI_8X16,
            HL_CODEC_264_MB_TYPE_B_L1_BI_16X8,
            HL_CODEC_264_MB_TYPE_B_L1_BI_8X16,
            HL_CODEC_264_MB_TYPE_B_BI_L0_16X8,
            HL_CODEC_264_MB_TYPE_B_BI_L0_8X16,
            HL_CODEC_264_MB_TYPE_B_BI_L1_16X8,
            HL_CODEC_264_MB_TYPE_B_BI_L1_8X16,
            HL_CODEC_264_MB_TYPE_B_BI_BI_16X8,
            HL_CODEC_264_MB_TYPE_B_BI_BI_8X16,
            HL_CODEC_264_MB_TYPE_B_8X8,
        };

// Table 7-14 – Macroblock type values 0 to 22 for B slices
static const enum HL_CODEC_264_MB_MODE_E HL_CODEC_264_MB_MODES_B[23 /*"mb_type" when slice type is B*/][2 /*MbPartPredMode(0,1)*/] = {
{ HL_CODEC_264_NA, HL_CODEC_264_MB_MODE_DIRECT },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_NA },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_NA },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_NA },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_MB_MODE_PRED_L1 },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_MB_MODE_PRED_L1 },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_PRED_L1 },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_PRED_L1 },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_BIPRED },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_BIPRED },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_MB_MODE_BIPRED },
{ HL_CODEC_264_MB_MODE_PRED_L1, HL_CODEC_264_MB_MODE_BIPRED },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_MB_MODE_PRED_L1 },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_MB_MODE_PRED_L1 },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_MB_MODE_BIPRED },
{ HL_CODEC_264_MB_MODE_BIPRED, HL_CODEC_264_MB_MODE_BIPRED },
{ HL_CODEC_264_NA, HL_CODEC_264_NA },
// inferred B_Skip Direct na
        };

// PartNum for both B and SB slices
// Usage: if (B_SLICE) { int partNum = HL_CODEC_264_MB_PART_NUM_B[mb_type]; }
static const int32_t HL_CODEC_264_MB_PART_NUM_B[23/*mb_type*/] = {
    1,// B_Direct_16x16,
    1,// B_L0_16x16,
    1,// B_L1_16x16,
    1,// B_Bi_16x16,
    2,// B_L0_L0_16x8,
    2,// B_L0_L0_8x16,
    2,// B_L1_L1_16x8,
    2,// B_L1_L1_8x16,
    2,// B_L0_L1_16x8,
    2,// B_L0_L1_8x16,
    2,// B_L1_L0_16x8,
    2,// B_L1_L0_8x16,
    2,// B_L0_Bi_16x8,
    2,// B_L0_Bi_8x16,
    2,// B_L1_Bi_16x8,
    2,// B_L1_Bi_8x16,
    2,// B_Bi_L0_16x8,
    2,// B_Bi_L0_8x16,
    2,// B_Bi_L1_16x8,
    2,// B_Bi_L1_8x16,
    2,// B_Bi_Bi_16x8,
    2,// B_Bi_Bi_8x16,
    4,// B_8x8,
};
// PartSize for both B and SB slices
// Usage: if (B_SLICE) { int partWidth = HL_CODEC_264_MB_PART_SIZE_B[mb_type][0]; int partHeight = HL_CODEC_264_MB_PART_SIZE_B[mb_type][1]; }
static const int32_t HL_CODEC_264_MB_PART_SIZE_B[23/*mb_type*/][2/*w=0,h=1*/] = {
    {16,16},// B_Direct_16x16,
    {16,16},// B_L0_16x16,
    {16,16},// B_L1_16x16,
    {16,16},// B_Bi_16x16,
    {16,8},// B_L0_L0_16x8,
    {8,16},// B_L0_L0_8x16,
    {16,8},// B_L1_L1_16x8,
    {8,16},// B_L1_L1_8x16,
    {16,8},// B_L0_L1_16x8,
    {8,16},// B_L0_L1_8x16,
    {16,8},// B_L1_L0_16x8,
    {8,16},// B_L1_L0_8x16,
    {16,8},// B_L0_Bi_16x8,
    {8,16},// B_L0_Bi_8x16,
    {16,8},// B_L1_Bi_16x8,
    {8,16},// B_L1_Bi_8x16,
    {16,8},// B_Bi_L0_16x8,
    {8,16},// B_Bi_L0_8x16,
    {16,8},// B_Bi_L1_16x8,
    {8,16},// B_Bi_L1_8x16,
    {16,8},// B_Bi_Bi_16x8,
    {8,16},// B_Bi_Bi_8x16,
    {8,8},// B_8x8,
};

// Table 7-13 – Macroblock type values 0 to 4 for P and SP slices
static const enum HL_CODEC_264_MB_MODE_E HL_CODEC_264_MB_MODES_P[5 /*"mb_type" when slice type is P*/][2 /*MbPartPredMode(0,1)*/] = {
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_NA },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_MB_MODE_PRED_L0, HL_CODEC_264_MB_MODE_PRED_L0 },
{ HL_CODEC_264_NA, HL_CODEC_264_NA },
{ HL_CODEC_264_NA, HL_CODEC_264_NA },
// inferred P_Skip Pred_L0 na
        };

static const int32_t HL_CODEC_264_MB_PART_NUM_P[5/*mb_type*/] = {
    1,// P_L0_16x16
    2,// P_L0_L0_16x8
    2,// P_L0_L0_8x16
    4,// P_8x8
    4,// P_8x8ref0
};

static const int32_t HL_CODEC_264_MB_PART_SIZE_P[5/*mb_type*/][2/*w=0,h=1*/] = {
    {16,16},// P_L0_16x16
    {16,8},// P_L0_L0_16x8
    {8,16},// P_L0_L0_8x16
    {8,8},// P_8x8
    {8,8},// P_8x8ref0
};

// Table 7-17 – Sub-macroblock types in P macroblocks
static const enum HL_CODEC_264_SUBMB_TYPE_E HL_CODEC_264_SUBMB_PRED_TYPE_P[4/*sub_mb_type*/] = {
            HL_CODEC_264_SUBMB_TYPE_P_L0_8X8,
            HL_CODEC_264_SUBMB_TYPE_P_L0_8X4,
            HL_CODEC_264_SUBMB_TYPE_P_L0_4X8,
            HL_CODEC_264_SUBMB_TYPE_P_L0_4X4,
        };
// Table 7-17 – Sub-macroblock types in P macroblocks
static const enum HL_CODEC_264_SUBMB_MODE_E HL_CODEC_264_SUBMB_PRED_MODE_P[4/*sub_mb_type*/] = {
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
        };

// Table 7-17 – Sub-macroblock types in P macroblocks
static const int32_t HL_CODEC_264_SUBMB_PART_NUM_P[4/*sub_mb_type*/] = {
    1,// P_L0_8x8,
    2,// P_L0_8x4,
    2,// P_L0_4x8,
    4,// P_L0_4x4
};

// Table 7-17 – Sub-macroblock types in P macroblocks
static const int32_t HL_CODEC_264_SUBMB_PART_SIZE_P[4/*sub_mb_type*/][2/*w=0,h=1*/]  = {
    {8,8},// P_L0_8x8
    {8,4},// P_L0_8x4
    {4,8},// P_L0_4x8
    {4,4},// P_L0_4x4
};

// Table 7-18 – Sub-macroblock types in B macroblocks
static const enum HL_CODEC_264_SUBMB_TYPE_E HL_CODEC_264_SUBMB_PRED_TYPE_B[13/*sub_mb_type*/] = {
            HL_CODEC_264_SUBMB_TYPE_B_DIRECT_8X8,
            HL_CODEC_264_SUBMB_TYPE_B_L0_8X8,
            HL_CODEC_264_SUBMB_TYPE_B_L1_8X8,
            HL_CODEC_264_SUBMB_TYPE_B_BI_8X8,
            HL_CODEC_264_SUBMB_TYPE_B_L0_8X4,
            HL_CODEC_264_SUBMB_TYPE_B_L0_4X8,
            HL_CODEC_264_SUBMB_TYPE_B_L1_8X4,
            HL_CODEC_264_SUBMB_TYPE_B_L1_4X8,
            HL_CODEC_264_SUBMB_TYPE_B_BI_8X4,
            HL_CODEC_264_SUBMB_TYPE_B_BI_4X8,
            HL_CODEC_264_SUBMB_TYPE_B_L0_4X4,
            HL_CODEC_264_SUBMB_TYPE_B_L1_4X4,
            HL_CODEC_264_SUBMB_TYPE_B_BI_4X4
        };

// Table 7-18 – Sub-macroblock types in B macroblocks
static const enum HL_CODEC_264_SUBMB_MODE_E HL_CODEC_264_SUBMB_PRED_MODE_B[13/*sub_mb_type*/] = {
            HL_CODEC_264_SUBMB_MODE_DIRECT,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L1,
            HL_CODEC_264_SUBMB_MODE_BIPRED,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L1,
            HL_CODEC_264_SUBMB_MODE_PRED_L1,
            HL_CODEC_264_SUBMB_MODE_BIPRED,
            HL_CODEC_264_SUBMB_MODE_BIPRED,
            HL_CODEC_264_SUBMB_MODE_PRED_L0,
            HL_CODEC_264_SUBMB_MODE_PRED_L1,
            HL_CODEC_264_SUBMB_MODE_BIPRED,
        };

// Table 7-18 – Sub-macroblock types in B macroblocks
static const int32_t HL_CODEC_264_SUBMB_PART_NUM_B[13/*sub_mb_type*/] = {
    4,// B_Direct_8x8,
    1,// B_L0_8x8,
    1,// B_L1_8x8,
    1,// B_Bi_8x8,
    2,// B_L0_8x4,
    2,// B_L0_4x8,
    2,// B_L1_8x4,
    2,// B_L1_4x8,
    2,// B_Bi_8x4,
    2,// B_Bi_4x8,
    4,// B_L0_4x4,
    4,// B_L1_4x4,
    4,// B_Bi_4x4
};

// Table 7-18 – Sub-macroblock types in B macroblocks
static const int32_t HL_CODEC_264_SUBMB_PART_SIZE_B[13/*sub_mb_type*/][2/*w=0,h=1*/]  = {
    {4,4},// B_Direct_8x8,
    {8,8},// B_L0_8x8,
    {8,8},// B_L1_8x8,
    {8,8},// B_Bi_8x8,
    {8,4},// B_L0_8x4,
    {4,8},// B_L0_4x8,
    {8,4},// B_L1_8x4,
    {4,8},// B_L1_4x8,
    {8,4},// B_Bi_8x4,
    {4,8},// B_Bi_4x8,
    {4,4},// B_L0_4x4,
    {4,4},// B_L1_4x4,
    {4,4},// B_Bi_4x4
};

// Table G-7 – Macroblock type predictors mbTypeILPred
static const enum HL_CODEC_264_MB_TYPE_E HL_CODEC_264_SVC_MB_TYPE_ILPRED_EB[4/*partitionSize*/] [4/*partPredModeA*/][4/*partPredModeB*/] = {
{ {HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA}, {HL_CODEC_264_MB_TYPE_B_L0_16X16, HL_CODEC_264_MB_TYPE_B_L0_16X16, HL_CODEC_264_MB_TYPE_B_L0_16X16, HL_CODEC_264_MB_TYPE_B_L0_16X16}, {HL_CODEC_264_MB_TYPE_B_L1_16X16, HL_CODEC_264_MB_TYPE_B_L1_16X16, HL_CODEC_264_MB_TYPE_B_L1_16X16, HL_CODEC_264_MB_TYPE_B_L1_16X16}, {HL_CODEC_264_MB_TYPE_B_BI_16X16, HL_CODEC_264_MB_TYPE_B_BI_16X16, HL_CODEC_264_MB_TYPE_B_BI_16X16, HL_CODEC_264_MB_TYPE_B_BI_16X16} },
{ {HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA}, {HL_CODEC_264_NA, HL_CODEC_264_MB_TYPE_B_L0_L0_16X8, HL_CODEC_264_MB_TYPE_B_L0_L1_16X8, HL_CODEC_264_MB_TYPE_B_L0_BI_16X8}, {HL_CODEC_264_NA, HL_CODEC_264_MB_TYPE_B_L1_L0_16X8, HL_CODEC_264_MB_TYPE_B_L1_L1_16X8, HL_CODEC_264_MB_TYPE_B_L1_BI_16X8}, {HL_CODEC_264_NA, HL_CODEC_264_MB_TYPE_B_BI_L0_16X8, HL_CODEC_264_MB_TYPE_B_BI_L1_16X8, HL_CODEC_264_MB_TYPE_B_BI_BI_16X8} },
{ {HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA}, {HL_CODEC_264_NA, HL_CODEC_264_MB_TYPE_B_L0_L0_8X16, HL_CODEC_264_MB_TYPE_B_L0_L1_8X16, HL_CODEC_264_MB_TYPE_B_L0_BI_8X16}, {HL_CODEC_264_NA, HL_CODEC_264_MB_TYPE_B_L1_L0_8X16, HL_CODEC_264_MB_TYPE_B_L1_L1_8X16, HL_CODEC_264_MB_TYPE_B_L1_BI_8X16}, {HL_CODEC_264_NA, HL_CODEC_264_MB_TYPE_B_BI_L0_8X16, HL_CODEC_264_MB_TYPE_B_BI_L1_8X16, HL_CODEC_264_MB_TYPE_B_BI_BI_8X16} },
{ {HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8}, {HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8}, {HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8}, {HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8, HL_CODEC_264_MB_TYPE_B_8X8} }
        };
static const int32_t HL_CODEC_264_SVC_MB_TYPE_ILPRED_EB_STANDARD[4/*partitionSize*/] [4/*partPredModeA*/][4/*partPredModeB*/] = {
    { {HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA}, {B_L0_16x16, B_L0_16x16, B_L0_16x16, B_L0_16x16}, {B_L1_16x16, B_L1_16x16, B_L1_16x16, B_L1_16x16}, {B_Bi_16x16, B_Bi_16x16, B_Bi_16x16, B_Bi_16x16} },
    { {HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA}, {HL_CODEC_264_NA, B_L0_L0_16x8, B_L0_L1_16x8, B_L0_Bi_16x8}, {HL_CODEC_264_NA, B_L1_L0_16x8, B_L1_L1_16x8, B_L1_Bi_16x8}, {HL_CODEC_264_NA, B_Bi_L0_16x8, B_Bi_L1_16x8, B_Bi_Bi_16x8} },
    { {HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA, HL_CODEC_264_NA}, {HL_CODEC_264_NA, B_L0_L0_8x16, B_L0_L1_8x16, B_L0_Bi_8x16}, {HL_CODEC_264_NA, B_L1_L0_8x16, B_L1_L1_8x16, B_L1_Bi_8x16}, {HL_CODEC_264_NA, B_Bi_L0_8x16, B_Bi_L1_8x16, B_Bi_Bi_8x16} },
    { {B_8x8, B_8x8, B_8x8, B_8x8}, {B_8x8, B_8x8, B_8x8, B_8x8}, {B_8x8, B_8x8, B_8x8, B_8x8}, {B_8x8, B_8x8, B_8x8, B_8x8} }
};
static const enum HL_CODEC_264_MB_TYPE_E HL_CODEC_264_SVC_MB_TYPE_ILPRED_EP[4/*partitionSize*/] = {
            HL_CODEC_264_MB_TYPE_P_L0_16X16,
            HL_CODEC_264_MB_TYPE_P_L0_L0_16X8,
            HL_CODEC_264_MB_TYPE_P_L0_L0_8X16,
            HL_CODEC_264_MB_TYPE_P_8X8
        };
static const int32_t HL_CODEC_264_SVC_MB_TYPE_ILPRED_EP_STANDARD[4/*partitionSize*/] = {
    P_L0_16x16,
    P_L0_L0_16x8,
    P_L0_L0_8x16,
    P_8x8
};

// Table G-8 – Sub-macroblock type predictors subMbTypeILPred[ mbPartIdx ]
static const enum HL_CODEC_264_SUBMB_TYPE_E HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB[4/*subPartitionSize*/] [4/*partPredMode*/] = {
{HL_CODEC_264_NA, HL_CODEC_264_SUBMB_TYPE_B_L0_8X8, HL_CODEC_264_SUBMB_TYPE_B_L1_8X8, HL_CODEC_264_SUBMB_TYPE_B_BI_8X8},
{HL_CODEC_264_NA, HL_CODEC_264_SUBMB_TYPE_B_L0_8X4, HL_CODEC_264_SUBMB_TYPE_B_L1_8X4, HL_CODEC_264_SUBMB_TYPE_B_BI_8X4},
{HL_CODEC_264_NA, HL_CODEC_264_SUBMB_TYPE_B_L0_4X8, HL_CODEC_264_SUBMB_TYPE_B_L1_4X8, HL_CODEC_264_SUBMB_TYPE_B_BI_4X8},
{HL_CODEC_264_NA, HL_CODEC_264_SUBMB_TYPE_B_L0_4X4, HL_CODEC_264_SUBMB_TYPE_B_L1_4X4, HL_CODEC_264_SUBMB_TYPE_B_BI_4X4}
        };
static const int32_t HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EB_STANDARD[4/*subPartitionSize*/] [4/*partPredMode*/] = {
    {HL_CODEC_264_NA, B_L0_8x8, B_L1_8x8, B_Bi_8x8},
    {HL_CODEC_264_NA, B_L0_8x4, B_L1_8x4, B_Bi_8x4},
    {HL_CODEC_264_NA, B_L0_4x8, B_L1_4x8, B_Bi_4x8},
    {HL_CODEC_264_NA, B_L0_4x4, B_L1_4x4, B_Bi_4x4}
};
static const enum HL_CODEC_264_SUBMB_TYPE_E HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP[4/*subPartitionSize*/]= {
            HL_CODEC_264_SUBMB_TYPE_P_L0_8X8,
            HL_CODEC_264_SUBMB_TYPE_P_L0_8X4,
            HL_CODEC_264_SUBMB_TYPE_P_L0_4X8,
            HL_CODEC_264_SUBMB_TYPE_P_L0_4X4,
        };
static const int32_t HL_CODEC_264_SVC_SUBMB_TYPE_ILPRED_EP_STANDARD[4/*subPartitionSize*/]= {
    P_L0_8x8,
    P_L0_8x4,
    P_L0_4x8,
    P_L0_4x4
};

// Table G-9 – 16-phase luma interpolation filter for resampling in Intra_Base prediction
// eF[ p, x ] = HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[p][x]
static const int32_t HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[16/*p=0..15*/][4/*x=0..3*/] = {
    {0, 32, 0, 0},
    {-1, 32, 2, -1},
    {-2, 31, 4, -1},
    {-3, 30, 6, -1},
    {-3, 28, 8, -1},
    {-4, 26, 11, -1},
    {-4, 24, 14, -2},
    {-3, 22, 16, -3},
    {-3, 19, 19, -3},
    {-3, 16, 22, -3},
    {-2, 14, 24, -4},
    {-1, 11, 26, -4},
    {-1, 8, 28, -3},
    {-1, 6, 30, -3},
    {-1, 4, 31, -2},
    {-1, 2, 32, -1}
};

// From JVSM
// eF[ p, x ] = HL_CODEC_264_SVC_16_PHASE_LUMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[p][x]
static const int32_t HL_CODEC_264_SVC_16_PHASE_CHROMA_INTERPOL_FILTER_RESAMPL_INTRA_BASE[16/*p=0..15*/][2/*x=0..1*/] = {
    { 32,  0 },
    { 30,  2 },
    { 28,  4 },
    { 26,  6 },
    { 24,  8 },
    { 22, 10 },
    { 20, 12 },
    { 18, 14 },
    { 16, 16 },
    { 14, 18 },
    { 12, 20 },
    { 10, 22 },
    {  8, 24 },
    {  6, 26 },
    {  4, 28 },
    {  2, 30 }
};

// Table 9-42 – Specification of ctxBlockCat for the different blocks
static const int32_t maxNumCoeff_ctxBlockCat_except3[14 /* HL_CODEC_264_RESISUAL_INV_TYPE_T == ctxBlockCat*/] =
{ 16, 15, 16, 16/* Must be computed */, 15, 64, 16, 15, 16, 64, 16, 15, 16, 64 };

static const int32_t __xFracC_per_yFracC[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 2, 4, 6, 8, 10, 12, 14}, {0, 3, 6, 9, 12, 15, 18, 21},
    {0, 4, 8, 12, 16, 20, 24, 28}, {0, 5, 10, 15, 20, 25, 30, 35}, {0, 6, 12, 18, 24, 30, 36, 42}, {0, 7, 14, 21, 28, 35, 42, 49}
};
static const int32_t __8minus_xFracC[8][8] = {
    {8, 7, 6, 5, 4, 3, 2, 1}, {8, 7, 6, 5, 4, 3, 2, 1}, {8, 7, 6, 5, 4, 3, 2, 1}, {8, 7, 6, 5, 4, 3, 2, 1},
    {8, 7, 6, 5, 4, 3, 2, 1}, {8, 7, 6, 5, 4, 3, 2, 1}, {8, 7, 6, 5, 4, 3, 2, 1}, {8, 7, 6, 5, 4, 3, 2, 1}
};
static const int32_t __8minus_yFracC[8][8] = {
    {8, 8, 8, 8, 8, 8, 8, 8}, {7, 7, 7, 7, 7, 7, 7, 7}, {6, 6, 6, 6, 6, 6, 6, 6}, {5, 5, 5, 5, 5, 5, 5, 5},
    {4, 4, 4, 4, 4, 4, 4, 4}, {3, 3, 3, 3, 3, 3, 3, 3}, {2, 2, 2, 2, 2, 2, 2, 2}, {1, 1, 1, 1, 1, 1, 1, 1}
};
static const int32_t __8minus_xFracC_per_8minus_yFracC[8][8] = {
    {64, 56, 48, 40, 32, 24, 16, 8}, {56, 49, 42, 35, 28, 21, 14, 7}, {48, 42, 36, 30, 24, 18, 12, 6}, {40, 35, 30, 25, 20, 15, 10, 5},
    {32, 28, 24, 20, 16, 12, 8, 4}, {24, 21, 18, 15, 12, 9, 6, 3}, {16, 14, 12, 10, 8, 6, 4, 2}, {8, 7, 6, 5, 4, 3, 2, 1}
};
static const int32_t __8minus_xFracC_per_yFracC[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0}, {8, 7, 6, 5, 4, 3, 2, 1}, {16, 14, 12, 10, 8, 6, 4, 2}, {24, 21, 18, 15, 12, 9, 6, 3},
    {32, 28, 24, 20, 16, 12, 8, 4}, {40, 35, 30, 25, 20, 15, 10, 5}, {48, 42, 36, 30, 24, 18, 12, 6}, {56, 49, 42, 35, 28, 21, 14, 7}
};
static const int32_t __8minus_yFracC_per_xFracC[8][8] = {
    {0, 8, 16, 24, 32, 40, 48, 56}, {0, 7, 14, 21, 28, 35, 42, 49}, {0, 6, 12, 18, 24, 30, 36, 42}, {0, 5, 10, 15, 20, 25, 30, 35},
    {0, 4, 8, 12, 16, 20, 24, 28}, {0, 3, 6, 9, 12, 15, 18, 21}, {0, 2, 4, 6, 8, 10, 12, 14}, {0, 1, 2, 3, 4, 5, 6, 7}
};

//
// Cabac context global tables: from 9-13 to 9-33
// SI and I
//
static const int32_t HL_CODEC_264_CABAC_TABLE_I[HL_CODEC_264_CABAC_TABLE_SIZE/*ctxIdx*/][2/*m=0,n=1*/] = {
    // Table 9-12 - Values of variables m and n for ctxIdx from 0 to 10
    { 20,-15 },{ 2,54 },{ 3,74 },{ 20,-15 },{ 2,54 },{ 3,74 },{ -28,127 },{ -23,104 },
    { -6,53 },{ -1,54 },{ 7,51 },

    // Table 9-13 - Values of variables m and n for ctxIdx from 11 to 23
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

    // Table 9-14 - Values of variables m and n for ctxIdx from 24 to 39
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

    // Table 9-15 - Values of variables m and n for ctxIdx from 40 to 53
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

    // Table 9-16 - Values of variables m and n for ctxIdx from 54 to 59,
    { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 }, { 0,0 },{ 0,0 },

    // Table 9-17 - Values of variables m and n for ctxIdx from 60 to 69
    { 0,41 },{ 0,63 },{ 0,63 },{ 0,63 }, { -9,83 },{ 4,86 },{ 0,97 },{ -7,72 },
    { 13,41 },{ 3,62 },

    // Table 9-18 - Values of variables m and n for ctxIdx from 70 to 104
    { 0,11 },{ 1,55 },{ 0,69 },{ -17,127 },{ -13,102 },{ 0,82 },
    { -7,74 },{ -21,107 },{ -27,127 },{ -31,127 },{ -24,127 },{ -18,95 },
    { -27,127 },{ -21,114 },{ -30,127 },{ -17,123 },{ -12,115 },{ -16,122 },
    { -11,115 },{ -12,63 },{ -2,68 },{ -15,84 },{ -13,104 },{ -3,70 },
    { -8,93 },{ -10,90 },{ -30,127 },{ -1,74 },{ -6,97 },{ -7,91 },
    { -20,127 },{ -4,56 },{ -5,82 },{ -7,76 },{ -22,125 },

    // Table 9-19 - Values of variables m and n for ctxIdx from 105 to 165
    { -7,93 },{ -11,87 },{ -3,77 },{ -5,71 },{ -4,63 },{ -4,68 },{ -12,84 },
    { -7,62 },{ -7,65 },{ 8,61 },{ 5,56 },{ -2,66 },{ 1,64 },
    { 0,61 },{ -2,78 },{ 1,50 },{ 7,52 },{ 10,35 },{ 0,44 },
    { 11,38 },{ 1,45 },{ 0,46 },{ 5,44 },{ 31,17 },{ 1,51 },
    { 7,50 },{ 28,19 },{ 16,33 },{ 14,62 },{ -13,108 },{ -15,100 },
    { -13,101 },{ -13,91 },{ -12,94 },{ -10,88 },{ -16,84 },{ -10,86 },
    { -7,83 },{ -13,87 },{ -19,94 },{ 1,70 },{ 0,72 },{ -5,74 },
    { 18,59 },{ -8,102 },{ -15,100 },{ 0,95 },{ -4,75 },{ 2,72 },
    { -11,75 },{ -3,71 },{ 15,46 },{ -13,69 },{ 0,62 },{ 0,65 },
    { 21,37 },{ -15,72 },{ 9,57 },{ 16,54 },{ 0,62 },{ 12,72 },

    // Table 9-20 - Values of variables m and n for ctxIdx from 166 to 226
    { 24,0 },{ 15,9 },{ 8,25 },{ 13,18 },{ 15,9 },{ 13,19 },
    { 10,37 },{ 12,18 },{ 6,29 },{ 20,33 },{ 15,30 },{ 4,45 },
    { 1,58 },{ 0,62 },{ 7,61 },{ 12,38 },{ 11,45 },{ 15,39 },
    { 11,42 },{ 13,44 },{ 16,45 },{ 12,41 },{ 10,49 },{ 30,34 },
    { 18,42 },{ 10,55 },{ 17,51 },{ 17,46 },{ 0,89 },{ 26,-19 },
    { 22,-17 },{ 26,-17 },{ 30,-25 },{ 28,-20 },{ 33,-23 },{ 37,-27 },
    { 33,-23 },{ 40,-28 },{ 38,-17 },{ 33,-11 },{ 40,-15 },{ 41,-6 },
    { 38,1 },{ 41,17 },{ 30,-6 },{ 27,3 },{ 26,22 },{ 37,-16 },
    { 35,-4 },{ 38,-8 },{ 38,-3 },{ 37,3 },{ 38,5 },{ 42,0 },
    { 35,16 },{ 39,22 },{ 14,48 },{ 27,37 },{ 21,60 },{ 12,68 },{ 2,97 },

    // Table 9-21 - Values of variables m and n for ctxIdx from 227 to 275
    { -3,71 },{ -6,42 },{ -5,50 },{ -3,54 },{ -2,62 },
    { 0,58 },{ 1,63 },{ -2,72 },{ -1,74 },{ -9,91 },{ -5,67 },
    { -5,27 },{ -3,39 },{ -2,44 },{ 0,46 },{ -16,64 },{ -8,68 },
    { -10,78 },{ -6,77 },{ -10,86 },{ -12,92 },{ -15,55 },{ -10,60 },
    { -6,62 },{ -4,65 },{ -12,73 },{ -8,76 },{ -7,80 },{ -9,88 },
    { -17,110 },{ -11,97 },{ -20,84 },{ -11,79 },{ -6,73 },{ -4,74 },
    { -13,86 },{ -13,96 },{ -11,97 },{ -19,117 },{ -8,78 },{ -5,33 },
    { -4,48 },{ -2,53 },{ -3,62 },{ -13,71 },{ -10,79 },{ -12,86 },
    { -13,90 },{ -14,97 },

    { HL_CODEC_264_NA, HL_CODEC_264_NA },//276 end_of_slice_flag

    // Table 9-22 - Values of variables m and n for ctxIdx from 277 to 337
    { -6,93 },{ -6,84 },{ -8,79 },{ 0,66 },
    { -1,71 },{ 0,62 },{ -2,60 },{ -2,59 },{ -5,75 },{ -3,62 },
    { -4,58 },{ -9,66 },{ -1,79 },{ 0,71 },{ 3,68 },{ 10,44 },
    { -7,62 },{ 15,36 },{ 14,40 },{ 16,27 },{ 12,29 },{ 1,44 },
    { 20,36 },{ 18,32 },{ 5,42 },{ 1,48 },{ 10,62 },{ 17,46 },
    { 9,64 },{ -12,104 },{ -11,97 },{ 31,21 },{ 31,31 },{ 25,50 },
    { -16,96 },{ -7,88 },{ -8,85 },{ -7,85 },{ -9,85 },{ -13,88 },
    { 4,66 },{ -3,77 },{ -3,76 },{ -6,76 },{ 10,58 },{ -1,76 },
    { -1,83 },{ -7,99 },{ -14,95 },{ 2,95 },{ 0,76 },{ -5,74 },
    { 0,70 },{ -11,75 },{ 1,68 },{ 0,65 },{ -14,73 },{ 3,62 },
    { 4,62 },{ -1,68 },{ -13,75 },{ 11,55 },{ 5,64 },{ 12,70 },

    // Table 9-23 - Values of variables m and n for ctxIdx from 338 to 398
    // and 399 to 401 => Table 9-16
    { 15,6 },{ 6,19 },{ 7,16 },{ 12,14 },{ 18,13 },{ 13,11 },{ 13,15 },
    { 15,16 },{ 12,23 },{ 13,23 },{ 15,20 },{ 14,26 },{ 14,44 },{ 17,40 },
    { 17,47 },{ 24,17 },{ 21,21 },{ 25,22 },{ 31,27 },{ 22,29 },{ 19,35 },
    { 14,50 },{ 10,57 },{ 7,63 },{ -2,77 },{ -4,82 },{ -3,94 },
    { 9,69 },{ -12,109 },{ 36,-35 },{ 36,-34 },{ 32,-26 },{ 37,-30 },
    { 44,-32 },{ 34,-18 },{ 34,-15 },{ 40,-15 },{ 33,-7 },{ 35,-5 },
    { 33,0 },{ 38,2 },{ 33,13 },{ 23,35 },{ 13,58 },{ 29,-3 },
    { 26,0 },{ 22,30 },{ 31,-7 },{ 35,-15 },{ 34,-3 },{ 34,3 },
    { 36,-1 },{ 34,5 },{ 32,11 },{ 35,5 },{ 34,12 },{ 39,11 },
    { 30,29 },{ 34,26 },{ 29,39 },{ 19,66 },

    // 399 - 401
    { HL_CODEC_264_NA, HL_CODEC_264_NA },{ HL_CODEC_264_NA, HL_CODEC_264_NA },{ HL_CODEC_264_NA, HL_CODEC_264_NA },

    // Table 9-24 - Values of variables m and n for ctxIdx from 402 to 459
    { -17,120 },{ -20,112 },{ -18,114 },{ -11,85 },{ -15,92 },{ -14,89 },
    { -26,71 },{ -15,81 },{ -14,80 },{ 0,68 },{ -14,70 },{ -24,56 },{ -23,68 },
    { -24,50 },{ -11,74 },{ 23,-13 },{ 26,-13 },{ 40,-15 },{ 49,-14 },{ 44,3 },
    { 45,6 },{ 44,34 },{ 33,54 },{ 19,82 },{ -3,75 },{ -1,23 },
    { 1,34 },{ 1,43 },{ 0,54 },{ -2,55 },{ 0,61 },{ 1,64 },
    { 0,68 },{ -9,92 },{ -14,106 },{ -13,97 },{ -15,90 },{ -12,90 },
    { -18,88 },{ -10,73 },{ -9,79 },{ -14,86 },{ -10,73 },{ -10,70 },
    { -10,69 },{ -5,66 },{ -9,64 },{ -5,58 },{ 2,59 },{ 21,-10 },
    { 24,-11 },{ 28,-8 },{ 28,-1 },{ 29,3 },{ 29,9 },{ 35,20 },
    { 29,36 },{ 14,67 },

    // Table 9-25 - Values of variables m and n for ctxIdx from 460 to 483
    { -17,123 },{ -12,115 },{ -16,122 },{ -11,115 },
    { -12,63 },{ -2,68 },{ -15,84 },{ -13,104 },{ -3,70 },{ -8,93 },
    { -10,90 },{ -30,127 },{ -17,123 },{ -12,115 },{ -16,122 },{ -11,115 },
    { -12,63 },{ -2,68 },{ -15,84 },{ -13,104 },{ -3,70 },{ -8,93 },
    { -10,90 },{ -30,127 },

    // Table 9-26 - Values of variables m and n for ctxIdx from 484 to 571
    { -7,93 },{ -11,87 },{ -3,77 },{ -5,71 },{ -4,63 },{ -4,68 },{ -12,84 },
    { -7,62 },{ -7,65 },{ 8,61 },{ 5,56 },{ -2,66 },{ 1,64 },{ 0,61 },{ -2,78 },
    { 1,50 },{ 7,52 },{ 10,35 },{ 0,44 },{ 11,38 },{ 1,45 },{ 0,46 },
    { 5,44 },{ 31,17 },{ 1,51 },{ 7,50 },{ 28,19 },{ 16,33 },
    { 14,62 },{ -13,108 },{ -15,100 },{ -13,101 },{ -13,91 },{ -12,94 },
    { -10,88 },{ -16,84 },{ -10,86 },{ -7,83 },{ -13,87 },{ -19,94 },
    { 1,70 },{ 0,72 },{ -5,74 },{ 18,59 },{ -7,93 },{ -11,87 },
    { -3,77 },{ -5,71 },{ -4,63 },{ -4,68 },{ -12,84 },{ -7,62 },
    { -7,65 },{ 8,61 },{ 5,56 },{ -2,66 },{ 1,64 },{ -2,78 },
    { 1,50 },{ 7,52 },{ 10,35 },{ 0,44 },{ 11,38 },{ 1,45 },
    { 0,46 },{ 5,44 },{ 31,17 },{ 1,51 },{ 7,50 },{ 28,19 },
    { 16,33 },{ 14,62 },{ -13,108 },{ -15,100 },{ -13,101 },{ -13,91 },
    { -12,94 },{ -10,88 },{ -16,84 },{ -10,86 },{ -7,83 },{ -13,87 },
    { -19,94 },{ 1,70 },{ 0,72 },{ -5,74 },{ 18,59 },

    // Table 9-27 - Values of variables m and n for ctxIdx from 572 to 659
    { 24,0 },{ 15,9 },{ 8,25 },{ 13,18 },{ 15,9 },{ 13,19 },{ 10,37 },
    { 12,18 },{ 6,29 },{ 20,33 },{ 15,30 },{ 4,45 },{ 1,58 },
    { 0,62 },{ 7,61 },{ 12,38 },{ 11,45 },{ 15,39 },{ 11,42 },
    { 13,44 },{ 16,45 },{ 12,41 },{ 10,49 },{ 30,34 },{ 18,42 },
    { 10,55 },{ 17,51 },{ 17,46 },{ 0,89 },{ 26,-19 },{ 22,-17 },
    { 26,-17 },{ 30,-25 },{ 28,-20 },{ 33,-23 },{ 37,-27 },{ 33,-23 },
    { 40,-28 },{ 38,-17 },{ 33,-11 },{ 40,-15 },{ 41,-6 },{ 38,1 },
    { 41,17 },{ 24,0 },{ 15,9 },{ 8,25 },{ 13,18 },{ 15,9 },
    { 13,19 },{ 10,37 },{ 12,18 },{ 6,29 },{ 20,33 },{ 15,30 },
    { 4,45 },{ 1,58 },{ 0,62 },{ 7,61 },{ 12,38 },{ 11,45 },
    { 15,39 },{ 11,42 },{ 13,44 },{ 16,45 },{ 12,41 },{ 10,49 },
    { 30,34 },{ 18,42 },{ 0,61 },{ 10,55 },{ 17,51 },{ 17,46 },
    { 0,89 },{ 26,-19 },{ 22,-17 },{ 26,-17 },{ 30,-25 },{ 28,-20 },
    { 33,-23 },{ 37,-27 },{ 33,-23 },{ 40,-28 },{ 38,-17 },{ 33,-11 },
    { 40,-15 },{ 41,-6 },{ 38,1 },{ 41,17 },

    // Table 9-28 - Values of variables m and n for ctxIdx from 660 to 717
    { -17,120 },{ -20,112 },{ -18,114 },{ -11,85 },{ -15,92 },{ -14,89 },
    { -26,71 },{ -15,81 },{ -14,80 },{ 0,68 },{ -14,70 },{ -24,56 },{ -23,68 },
    { -24,50 },{ -11,74 },{ -14,106 },{ -13,97 },{ -15,90 },{ -12,90 },{ -18,88 },
    { -10,73 },{ -9,79 },{ -14,86 },{ -10,73 },{ -10,70 },{ -10,69 },
    { -5,66 },{ -9,64 },{ -5,58 },{ 2,59 },{ 23,-13 },{ 26,-13 },
    { 40,-15 },{ 49,-14 },{ 44,3 },{ 45,6 },{ 44,34 },{ 33,54 },
    { 19,82 },{ 21,-10 },{ 24,-11 },{ 28,-8 },{ 28,-1 },{ 29,3 },
    { 29,9 },{ 35,20 },{ 29,36 },{ 14,67 },{ -3,75 },{ -1,23 },
    { 1,34 },{ 1,43 },{ 0,54 },{ -2,55 },{ 0,61 },{ 1,64 },{ 0,68 },{ -9,92 },

    // Table 9-29 - Values of variables m and n for ctxIdx from 718 to 775
    { -17,120 },{ -20,112 },{ -18,114 },{ -11,85 },
    { -15,92 },{ -14,89 },{ -26,71 },{ -15,81 },{ -14,80 },{ 0,68 },
    { -14,70 },{ -24,56 },{ -23,68 },{ -24,50 },{ -11,74 },{ -14,106 },
    { -13,97 },{ -15,90 },{ -12,90 },{ -18,88 },{ -10,73 },{ -9,79 },
    { -14,86 },{ -10,73 },{ -10,70 },{ -10,69 },{ -5,66 },{ -9,64 },
    { -5,58 },{ 2,59 },{ 23,-13 },{ 26,-13 },{ 40,-15 },{ 49,-14 },
    { 44,3 },{ 45,6 },{ 44,34 },{ 33,54 },{ 19,82 },{ 21,-10 },
    { 24,-11 },{ 28,-8 },{ 28,-1 },{ 29,3 },{ 29,9 },{ 35,20 },
    { 29,36 },{ 14,67 },{ -3,75 },{ -1,23 },{ 1,34 },{ 1,43 },
    { 0,54 },{ -2,55 },{ 0,61 },{ 1,64 },{ 0,68 },{ -9,92 },

    // Table 9-30 - Values of variables m and n for ctxIdx from 776 to 863
    { -6,93 },{ -6,84 },{ -8,79 },{ 0,66 },{ -1,71 },{ 0,62 },
    { -2,60 },{ -2,59 },{ -5,75 },{ -3,62 },{ -4,58 },{ -9,66 },
    { -1,79 },{ 0,71 },{ 3,68 },{ 10,44 },{ -7,62 },{ 15,36 },
    { 14,40 },{ 16,27 },{ 12,29 },{ 1,44 },{ 20,36 },{ 18,32 },
    { 5,42 },{ 1,48 },{ 10,62 },{ 17,46 },{ 9,64 },{ -12,104 },
    { -11,97 },{ -16,96 },{ -7,88 },{ -8,85 },{ -7,85 },{ -9,85 },
    { -13,88 },{ 4,66 },{ -3,77 },{ -3,76 },{ -6,76 },{ 10,58 },
    { -1,76 },{ -1,83 },{ -6,93 },{ -6,84 },{ -8,79 },{ 0,66 },
    { -1,71 },{ 0,62 },{ -2,60 },{ -2,59 },{ -5,75 },{ -3,62 },
    { -4,58 },{ -9,66 },{ -1,79 },{ 0,71 },{ 3,68 },{ 10,44 },
    { -7,62 },{ 15,36 },{ 14,40 },{ 16,27 },{ 12,29 },{ 1,44 },
    { 20,36 },{ 18,32 },{ 5,42 },{ 1,48 },{ 10,62 },{ 17,46 },
    { 9,64 },{ -12,104 },{ -11,97 },{ -16,96 },{ -7,88 },{ -8,85 },
    { -7,85 },{ -9,85 },{ -13,88 },{ 4,66 },{ -3,77 },{ -3,76 },
    { -6,76 },{ 10,58 },{ -1,76 },{ -1,83 },

    // Table 9-31 - Values of variables m and n for ctxIdx from 864 to 951
    { 15,6 },{ 6,19 },{ 7,16 },{ 12,14 },{ 18,13 },{ 13,11 },{ 13,15 },
    { 15,16 },{ 12,23 },{ 13,23 },{ 15,20 },{ 14,26 },{ 14,44 },{ 17,40 },
    { 17,47 },{ 24,17 },{ 21,21 },{ 25,22 },{ 31,27 },{ 22,29 },
    { 19,35 },{ 14,50 },{ 10,57 },{ 7,63 },{ -2,77 },{ -4,82 },
    { -3,94 },{ 9,69 },{ -12,109 },{ 36,-35 },{ 36,-34 },{ 32,-26 },
    { 37,-30 },{ 44,-32 },{ 34,-18 },{ 34,-15 },{ 40,-15 },{ 33,-7 },
    { 35,-5 },{ 33,0 },{ 38,2 },{ 33,13 },{ 23,35 },{ 13,58 },
    { 15,6 },{ 6,19 },{ 7,16 },{ 12,14 },{ 18,13 },{ 13,11 },
    { 13,15 },{ 15,16 },{ 12,23 },{ 13,23 },{ 15,20 },{ 14,26 },
    { 14,44 },{ 17,40 },{ 17,47 },{ 24,17 },{ 21,21 },{ 25,22 },
    { 31,27 },{ 22,29 },{ 19,35 },{ 14,50 },{ 10,57 },{ 7,63 },
    { -2,77 },{ -4,82 },{ -3,94 },{ 9,69 },{ -12,109 },{ 36,-35 },
    { 36,-34 },{ 32,-26 },{ 37,-30 },{ 44,-32 },{ 34,-18 },{ 34,-15 },
    { 40,-15 },{ 33,-7 },{ 35,-5 },{ 33,0 },{ 38,2 },{ 33,13 },{ 23,35 },{ 13,58 },

    // Table 9-32 - Values of variables m and n for ctxIdx from 952 to 1011
    { -3,71 },{ -6,42 },{ -5,50 },{ -3,54 },
    { -2,62 },{ 0,58 },{ 1,63 },{ -2,72 },{ -1,74 },{ -9,91 },
    { -5,67 },{ -5,27 },{ -3,39 },{ -2,44 },{ 0,46 },{ -16,64 },
    { -8,68 },{ -10,78 },{ -6,77 },{ -10,86 },{ -12,92 },{ -15,55 },
    { -10,60 },{ -6,62 },{ -4,65 },{ -12,73 },{ -8,76 },{ -7,80 },
    { -9,88 },{ -17,110 },{ -3,71 },{ -6,42 },{ -5,50 },{ -3,54 },
    { -2,62 },{ 0,58 },{ 1,63 },{ -2,72 },{ -1,74 },{ -9,91 },{ -5,67 },
    { -5,27 },{ -3,39 },{ -2,44 },{ 0,46 },{ -16,64 },{ -8,68 },{ -10,78 },
    { -6,77 },{ -10,86 },{ -12,92 },{ -15,55 },{ -10,60 },{ -6,62 },{ -4,65 },
    { -12,73 },{ -8,76 },{ -7,80 },{ -9,88 },{ -17,110 },

    // Table 9-33 - Values of variables m and n for ctxIdx from 1012 to 1023
    { -3,70 },{ -8,93 },{ -10,90 },{ -30,127 },
    { -3,70 },{ -8,93 },{ -10,90 },{ -30,127 },{ -3,70 },{ -8,93 },
    { -10,90 },{ -30,127 },

};

//
// Cabac context global tables: from 9-13 to 9-33
// SP and B
//
static const int32_t HL_CODEC_264_CABAC_TABLE_PB[3/*cabac_init_idc*/][HL_CODEC_264_CABAC_TABLE_SIZE/*ctxIdx*/][2/*m=0,n=1*/] = {
    //cabac_init_idc = 0
    {
        // Table 9-12 - Values of variables m and n for ctxIdx from 0 to 10
        { 20, -15 }, { 2, 54 }, { 3, 74 }, { 20, -15 },{ 2, 54 }, { 3, 74 },
        { -28, 127 }, { -23, 104 },{ -6, 53 }, { -1, 54 }, { 7, 51 },

        // Table 9-13 - Values of variables m and n for ctxIdx from 11 to 23
        { 23, 33 }, { 23, 2 }, { 21, 0 }, { 1, 9 },{ 0, 49 }, { -37, 118 }, { 5, 57 }, { -13, 78 },
        { -11, 65 }, { 1, 62 }, { 12, 49 }, { -4, 73 },{ 17, 50 },

        // Table 9-14 - Values of variables m and n for ctxIdx from 24 to 39
        { 18, 64 }, { 9, 43 }, { 29, 0 }, { 26, 67 },{ 16, 90 }, { 9, 104 },
        { -46, 127 }, { -20, 104 },{ 1, 67 }, { -13, 78 }, { -11, 65 }, { 1, 62 },
        { -6, 86 }, { -17, 95 }, { -6, 61 }, { 9, 45 },

        // Table 9-15 - Values of variables m and n for ctxIdx from 40 to 53
        { -3, 69 }, { -6, 81 }, { -11, 96 }, { 6, 55 },{ 7, 67 }, { -5, 86 }, { 2, 88 },
        { 0, 58 },{ -3, 76 }, { -10, 94 }, { 5, 54 }, { 4, 69 },{ -3, 81 }, { 0, 88 },

        // Table 9-16 - Values of variables m and n for ctxIdx from 54 to 59
        { -7, 67 }, { -5, 74 }, { -4, 74 }, { -5, 80 },{ -7, 72 }, { 1, 58 },

        // Table 9-17 - Values of variables m and n for ctxIdx from 60 to 69
        { 0, 41 }, { 0, 63 }, { 0, 63 }, { 0, 63 },{ -9, 83 }, { 4, 86 },
        { 0, 97 }, { -7, 72 },{ 13, 41 }, { 3, 62 },

        // Table 9-18 - Values of variables m and n for ctxIdx from 70 to 104
        { 0,45 },	{ -4,78 },	{ -3,96 },	{ -27,126 },	{ -28,98 },	{ -25,101 },
        { -23,67 },	{ -28,82 },	{ -20,94 },	{ -16,83 },	{ -22,110 },	{ -21,91 },
        { -18,102 },	{ -13,93 },	{ -29,127 },	{ -7,92 },	{ -5,89 },	{ -7,96 },
        { -13,108 },	{ -3,46 },	{ -1,65 },	{ -1,57 },	{ -9,93 },	{ -3,74 },
        { -9,92 },	{ -8,87 },	{ -23,126 },	{ 5,54 },	{ 6,60 },	{ 6,59 },
        { 6,69 },	{ -1,48 },	{ 0,68 },	{ -4,69 },	{ -8,88 },

        // Table 9-19 - Values of variables m and n for ctxIdx from 105 to 165
        { -2,85 }, { -6,78 },	{ -1,75 },	{ -7,77 },	{ 2,54 },	{ 5,50 },	{ -3,68 },
        { 1,50 },	{ 6,42 },	{ -4,81 },	{ 1,63 },	{ -4,70 },	{ 0,67 },
        { 2,57 },	{ -2,76 },	{ 11,35 },	{ 4,64 },	{ 1,61 },	{ 11,35 },
        { 18,25 },	{ 12,24 },	{ 13,29 },	{ 13,36 },	{ -10,93 },	{ -7,73 },
        { -2,73 },	{ 13,46 },	{ 9,49 },	{ -7,100 },	{ 9,53 },	{ 2,53 },
        { 5,53 },	{ -2,61 },	{ 0,56 },	{ 0,56 },	{ -13,63 },	{ -5,60 },
        { -1,62 },	{ 4,57 },	{ -6,69 },	{ 4,57 },	{ 14,39 },	{ 4,51 },
        { 13,68 },	{ 3,64 },	{ 1,61 },	{ 9,63 },	{ 7,50 },	{ 16,39 },
        { 5,44 },	{ 4,52 },	{ 11,48 },	{ -5,60 },	{ -1,59 },	{ 0,59 },
        { 22,33 },	{ 5,44 },	{ 14,43 },	{ -1,78 },	{ 0,60 },	{ 9,69 },

        // Table 9-20 - Values of variables m and n for ctxIdx from 166 to 226
        { 11,28 },	{ 2,40 },	{ 3,44 },	{ 0,49 },	{ 0,46 },	{ 2,44 },
        { 2,51 },	{ 0,47 },	{ 4,39 },	{ 2,62 },	{ 6,46 },	{ 0,54 },
        { 3,54 },	{ 2,58 },	{ 4,63 },	{ 6,51 },	{ 6,57 },	{ 7,53 },
        { 6,52 },	{ 6,55 },	{ 11,45 },	{ 14,36 },	{ 8,53 },	{ -1,82 },
        { 7,55 },	{ -3,78 },	{ 15,46 },	{ 22,31 },	{ -1,84 },	{ 25,7 },
        { 30,-7 },	{ 28,3 },	{ 28,4 },	{ 32,0 },	{ 34,-1 },	{ 30,6 },
        { 30,6 },	{ 32,9 },	{ 31,19 },	{ 26,27 },	{ 26,30 },	{ 37,20 },
        { 28,34 },	{ 17,70 },	{ 1,67 },	{ 5,59 },	{ 9,67 },	{ 16,30 },
        { 18,32 },	{ 18,35 },	{ 22,29 },	{ 24,31 },	{ 23,38 },	{ 18,43 },
        { 20,41 },	{ 11,63 },	{ 9,59 },	{ 9,64 },	{ -1,94 },	{ -2,89 },
        { -9,108 },

        // Table 9-21 - Values of variables m and n for ctxIdx from 227 to 275
        { -6,76 },	{ -2,44 },	{ 0,45 },	{ 0,52 },	{ -3,64 },
        { -2,59 },	{ -4,70 },	{ -4,75 },	{ -8,82 },	{ -17,102 },	{ -9,77 },
        { 3,24 },	{ 0,42 },	{ 0,48 },	{ 0,55 },	{ -6,59 },	{ -7,71 },
        { -12,83 },	{ -11,87 },	{ -30,119 },	{ 1,58 },	{ -3,29 },	{ -1,36 },
        { 1,38 },	{ 2,43 },{ -6,55 },	{ 0,58 },	{ 0,64 },	{ -3,74 },
        { -10,90 },	{ 0,70 },	{ -4,29 },	{ 5,31 },	{ 7,42 },	{ 1,59 },
        { -2,58 },	{ -3,72 },	{ -3,81 },	{ -11,97 },	{ 0,58 },	{ 8,5 },
        { 10,14 },	{ 14,18 },	{ 13,27 },	{ 2,40 },	{ 0,58 },	{ -3,70 },
        { -6,79 },	{ -8,85 },

        { HL_CODEC_264_NA, HL_CODEC_264_NA }, // 276 end_of_slice_flag

        // Table 9-22 - Values of variables m and n for ctxIdx from 277 to 337
        { -13,106 },	{ -16,106 },	{ -10,87 },	{ -21,114 },
        { -18,110 },	{ -14,98 },	{ -22,110 },	{ -21,106 },	{ -18,103 },	{ -21,107 },
        { -23,108 },	{ -26,112 },	{ -10,96 },	{ -12,95 },	{ -5,91 },	{ -9,93 },
        { -22,94 },	{ -5,86 },	{ 9,67 },	{ -4,80 },	{ -10,85 },	{ -1,70 },
        { 7,60 },	{ 9,58 },	{ 5,61 },	{ 12,50 },	{ 15,50 },	{ 18,49 },
        { 17,54 },	{ 10,41 },	{ 7,46 },	{ -1,51 },	{ 7,49 },	{ 8,52 },
        { 9,41 },	{ 6,47 },	{ 2,55 },	{ 13,41 },	{ 10,44 },	{ 6,50 },
        { 5,53 },	{ 13,49 },	{ 4,63 },	{ 6,64 },	{ -2,69 },	{ -2,59 },
        { 6,70 },	{ 10,44 },	{ 9,31 },	{ 12,43 },{ 3,53 },	{ 14,34 },
        { 10,38 },	{ -3,52 },	{ 13,40 },	{ 17,32 },	{ 7,44 },	{ 7,38 },
        { 13,50 },	{ 10,57 },	{ 26,43 },

        // Table 9-23 - Values of variables m and n for ctxIdx from 338 to 398
        { 14,11 },	{ 11,14 },	{ 9,11 },
        { 18,11 },	{ 21,9 },	{ 23,-2 },	{ 32,-15 },	{ 32,-15 },	{ 34,-21 },
        { 39,-23 },	{ 42,-33 },	{ 41,-31 },	{ 46,-28 },	{ 38,-12 },	{ 21,29 },
        { 45,-24 },	{ 53,-45 },	{ 48,-26 },	{ 65,-43 },	{ 43,-19 },	{ 39,-10 },
        { 30,9 },	{ 18,26 },	{ 20,27 },	{ 0,57 },	{ -14,82 },	{ -5,75 },
        { -19,97 },	{ -35,125 }, { 27,0 }, { 28,0 }, { 31,-4 },	{ 27,6 },
        { 34,8 },	{ 30,10 },	{ 24,22 },	{ 33,19 },	{ 22,32 },	{ 26,31 },
        { 21,41 },	{ 26,44 },	{ 23,47 },	{ 16,65 },	{ 14,71 },	{ 8,60 },
        { 6,63 },	{ 17,65 },	{ 21,24 },	{ 23,20 },	{ 26,23 },	{ 27,32 },
        { 28,23 },	{ 28,24 },	{ 23,40 },	{ 24,32 },	{ 28,29 },	{ 23,42 },
        { 19,57 },	{ 22,53 },	{ 22,61 },	{ 11,86 },

        { 12, 40 }, { 11, 51 }, { 14, 59 }, //  Table 9-16 (suite) - Values of variables m and n for ctxIdx from 399-401

        // Table 9-24 - Values of variables m and n for ctxIdx from 402 to 459
        { -4,79 },	{ -7,71 },
        { -5,69 },	{ -9,70 },	{ -8,66 },	{ -10,68 },	{ -19,73 },	{ -12,69 },
        { -16,70 },	{ -15,67 },	{ -20,62 },	{ -19,70 },	{ -16,66 },	{ -22,65 },
        { -20,63 },	{ 9,-2 },	{ 26,-9 },	{ 33,-9 },	{ 39,-7 },	{ 41,-2 },
        { 45,3 },	{ 49,9 },	{ 45,27 },	{ 36,59 },	{ -6,66 },	{ -7,35 },
        { -7,42 },	{ -8,45 },	{ -5,48 },	{ -12,56 },	{ -6,60 },	{ -5,62 },
        { -8,66 },	{ -8,76 },	{ -5,85 },	{ -6,81 },	{ -10,77 },	{ -7,81 },
        { -17,80 },	{ -18,73 },	{ -4,74 },	{ -10,83 },	{ -9,71 },	{ -9,67 },
        { -1,61 },	{ -8,66 },	{ -14,66 },	{ 0,59 },	{ 2,59 },	{ 21,-13 },
        { 33,-14 },	{ 39,-7 },	{ 46,-2 },	{ 51,2 },	{ 60,6 },	{ 61,17 },
        { 55,34 },	{ 42,62 },

        // Table 9-25 - Values of variables m and n for ctxIdx from 460 to 483
        { -7,92 },	{ -5,89 },	{ -7,96 },	{ -13,108 },
        { -3,46 },	{ -1,65 },	{ -1,57 },	{ -9,93 },	{ -3,74 },	{ -9,92 },
        { -8,87 },	{ -23,126 },	{ -7,92 },	{ -5,89 },	{ -7,96 },	{ -13,108 },
        { -3,46 },	{ -1,65 },	{ -1,57 },	{ -9,93 },	{ -3,74 },	{ -9,92 },
        { -8,87 },	{ -23,126 },

        // Table 9-26 - Values of variables m and n for ctxIdx from 484 to 571
        { -2,85 },	{ -6,78 },	{ -1,75 },	{ -7,77 },
        { 2,54 },	{ 5,50 },	{ -3,68 },	{ 1,50 },	{ 6,42 },	{ -4,81 },
        { 1,63 },	{ -4,70 },	{ 0,67 },	{ 2,57 },	{ -2,76 },	{ 11,35 },
        { 4,64 },	{ 1,61 },	{ 11,35 },	{ 18,25 },	{ 12,24 },	{ 13,29 },
        { 13,36 },	{ -10,93 },	{ -7,73 },	{ -2,73 },	{ 13,46 },	{ 9,49 },
        { -7,100 },	{ 9,53 },	{ 2,53 },	{ 5,53 },	{ -2,61 },	{ 0,56 },
        { 0,56 },	{ -13,63 },	{ -5,60 },	{ -1,62 },	{ 4,57 },	{ -6,69 },
        { 4,57 },	{ 14,39 },	{ 4,51 },	{ 13,68 },	{ -2,85 },	{ -6,78 },
        { -1,75 },	{ -7,77 },	{ 2,54 },	{ 5,50 },	{ -3,68 },	{ 1,50 },
        { 6,42 },	{ -4,81 },	{ 1,63 },	{ -4,70 },	{ 0,67 },	{ -2,76 },
        { 11,35 },	{ 4,64 },	{ 1,61 },	{ 11,35 },	{ 18,25 },	{ 12,24 },
        { 13,29 },	{ 13,36 },	{ -10,93 },	{ -7,73 },	{ -2,73 },	{ 13,46 },
        { 9,49 },	{ -7,100 },	{ 9,53 },	{ 2,53 },	{ 5,53 },	{ -2,61 },
        { 0,56 },	{ 0,56 },	{ -13,63 },	{ -5,60 },	{ -1,62 },	{ 4,57 },
        { -6,69 },	{ 4,57 },	{ 14,39 },	{ 4,51 },	{ 13,68 },

        // Table 9-27 - Values of variables m and n for ctxIdx from 572 to 659
        { 11,28 },
        { 2,40 },	{ 3,44 },	{ 0,49 },	{ 0,46 },	{ 2,44 },	{ 2,51 },
        { 0,47 },	{ 4,39 },	{ 2,62 },	{ 6,46 },	{ 0,54 },	{ 3,54 },
        { 2,58 },	{ 4,63 },	{ 6,51 },	{ 6,57 },	{ 7,53 },	{ 6,52 },
        { 6,55 },	{ 11,45 },	{ 14,36 },	{ 8,53 },	{ -1,82 },	{ 7,55 },
        { -3,78 },	{ 15,46 },	{ 22,31 },	{ -1,84 },	{ 25,7 },	{ 30,-7 },
        { 28,3 },	{ 28,4 },	{ 32,0 },	{ 34,-1 },	{ 30,6 },	{ 30,6 },
        { 32,9 },	{ 31,19 },	{ 26,27 },	{ 26,30 },	{ 37,20 },	{ 28,34 },
        { 17,70 },	{ 11,28 },	{ 2,40 },	{ 3,44 },	{ 0,49 },	{ 0,46 },
        { 2,44 },	{ 2,51 },	{ 0,47 },	{ 4,39 },	{ 2,62 },	{ 6,46 },
        { 0,54 },	{ 3,54 },	{ 2,58 },	{ 4,63 },	{ 6,51 },	{ 6,57 },
        { 7,53 },	{ 6,52 },	{ 6,55 },	{ 11,45 },	{ 14,36 },	{ 8,53 },
        { -1,82 },	{ 7,55 },	{ 2,57 },	{ -3,78 },	{ 15,46 },	{ 22,31 },
        { -1,84 },	{ 25,7 },	{ 30,-7 },  { 28,3 },	{ 28,4 },	{ 32,0 },
        { 34,-1 },	{ 30,6 },	{ 30,6 },	{ 32,9 },	{ 31,19 },	{ 26,27 },
        { 26,30 },	{ 37,20 },	{ 28,34 },	{ 17,70 },

        // Table 9-29 - Values of variables m and n for ctxIdx from 718 to 775
        { -4,79 },	{ -7,71 },
        { -5,69 },	{ -9,70 },	{ -8,66 },	{ -10,68 },	{ -19,73 },	{ -12,69 },
        { -16,70 },	{ -15,67 },	{ -20,62 },	{ -19,70 },	{ -16,66 },	{ -22,65 },
        { -20,63 },	{ -5,85 },	{ -6,81 },	{ -10,77 },	{ -7,81 },	{ -17,80 },
        { -18,73 },	{ -4,74 },	{ -10,83 },	{ -9,71 },	{ -9,67 },	{ -1,61 },
        { -8,66 },	{ -14,66 },	{ 0,59 },	{ 2,59 },	{ 9,-2 },	{ 26,-9 },
        { 33,-9 },	{ 39,-7 },	{ 41,-2 },	{ 45,3 },	{ 49,9 },	{ 45,27 },
        { 36,59 },	{ 21,-13 },	{ 33,-14 },	{ 39,-7 },	{ 46,-2 },	{ 51,2 },
        { 60,6 },	{ 61,17 },	{ 55,34 },	{ 42,62 },	{ -6,66 },	{ -7,35 },
        { -7,42 },	{ -8,45 },	{ -5,48 },	{ -12,56 },	{ -6,60 },	{ -5,62 },
        { -8,66 },	{ -8,76 },  { -4,79 },	{ -7,71 },	{ -5,69 },	{ -9,70 },
        { -8,66 },	{ -10,68 },	{ -19,73 },	{ -12,69 },	{ -16,70 },	{ -15,67 },
        { -20,62 },	{ -19,70 },	{ -16,66 },	{ -22,65 },	{ -20,63 },	{ -5,85 },
        { -6,81 },	{ -10,77 },	{ -7,81 },	{ -17,80 },	{ -18,73 },	{ -4,74 },
        { -10,83 },	{ -9,71 },	{ -9,67 },	{ -1,61 },	{ -8,66 },	{ -14,66 },
        { 0,59 },	{ 2,59 },	{ 9,-2 },	{ 26,-9 },	{ 33,-9 },	{ 39,-7 },
        { 41,-2 },	{ 45,3 },	{ 49,9 },	{ 45,27 },	{ 36,59 },	{ 21,-13 },
        { 33,-14 },	{ 39,-7 },	{ 46,-2 },	{ 51,2 },	{ 60,6 },	{ 61,17 },
        { 55,34 },	{ 42,62 },	{ -6,66 },	{ -7,35 },	{ -7,42 },	{ -8,45 },
        { -5,48 },	{ -12,56 },	{ -6,60 },	{ -5,62 },	{ -8,66 },	{ -8,76 },

        // Table 9-30 - Values of variables m and n for ctxIdx from 776 to 863
        { -13,106 },	{ -16,106 },	{ -10,87 },	{ -21,114 },	{ -18,110 },	{ -14,98 },
        { -22,110 },	{ -21,106 },	{ -18,103 },	{ -21,107 },	{ -23,108 },	{ -26,112 },
        { -10,96 },	{ -12,95 },	{ -5,91 },	{ -9,93 },	{ -22,94 },	{ -5,86 },
        { 9,67 },	{ -4,80 },	{ -10,85 },	{ -1,70 },	{ 7,60 },	{ 9,58 },
        { 5,61 },	{ 12,50 },	{ 15,50 },	{ 18,49 },	{ 17,54 },	{ 10,41 },
        { 7,46 },	{ -1,51 },	{ 7,49 },	{ 8,52 },	{ 9,41 },	{ 6,47 },
        { 2,55 },	{ 13,41 },	{ 10,44 },	{ 6,50 },	{ 5,53 },	{ 13,49 },
        { 4,63 },	{ 6,64 },	{ -13,106 },	{ -16,106 },	{ -10,87 },	{ -21,114 },
        { -18,110 },	{ -14,98 },	{ -22,110 },	{ -21,106 },	{ -18,103 },	{ -21,107 },
        { -23,108 },	{ -26,112 },	{ -10,96 },	{ -12,95 },	{ -5,91 },	{ -9,93 },
        { -22,94 },	{ -5,86 },	{ 9,67 },	{ -4,80 },	{ -10,85 },	{ -1,70 },
        { 7,60 },	{ 9,58 },	{ 5,61 },	{ 12,50 },	{ 15,50 },	{ 18,49 },
        { 17,54 },	{ 10,41 },	{ 7,46 },	{ -1,51 },	{ 7,49 },	{ 8,52 },
        { 9,41 },	{ 6,47 },	{ 2,55 },	{ 13,41 },	{ 10,44 },	{ 6,50 },
        { 5,53 },	{ 13,49 },	{ 4,63 },	{ 6,64 },

        // Table 9-31 - Values of variables m and n for ctxIdx from 864 to 951
        { 14,11 },	{ 11,14 },
        { 9,11 },	{ 18,11 },	{ 21,9 },	{ 23,-2 },	{ 32,-15 },	{ 32,-15 },
        { 34,-21 },	{ 39,-23 },	{ 42,-33 },	{ 41,-31 },	{ 46,-28 },	{ 38,-12 },
        { 21,29 },	{ 45,-24 },	{ 53,-45 },	{ 48,-26 },	{ 65,-43 },	{ 43,-19 },
        { 39,-10 },	{ 30,9 },	{ 18,26 },	{ 20,27 },	{ 0,57 },	{ -14,82 },
        { -5,75 },	{ -19,97 },	{ -35,125 },	{ 27,0 },	{ 28,0 },	{ 31,-4 },
        { 27,6 },	{ 34,8 },	{ 30,10 },	{ 24,22 },	{ 33,19 },	{ 22,32 },
        { 26,31 },	{ 21,41 },	{ 26,44 },	{ 23,47 },	{ 16,65 },	{ 14,71 },
        { 14,11 },	{ 11,14 },	{ 9,11 },	{ 18,11 },	{ 21,9 },	{ 23,-2 },
        { 32,-15 },	{ 32,-15 },	{ 34,-21 },	{ 39,-23 },	{ 42,-33 },	{ 41,-31 },
        { 46,-28 },	{ 38,-12 },	{ 21,29 },	{ 45,-24 },	{ 53,-45 },	{ 48,-26 },
        { 65,-43 },	{ 43,-19 },	{ 39,-10 },	{ 30,9 },	{ 18,26 },	{ 20,27 },
        { 0,57 },	{ -14,82 },	{ -5,75 },	{ -19,97 },	{ -35,125 },	{ 27,0 },
        { 28,0 },	{ 31,-4 },	{ 27,6 },	{ 34,8 },	{ 30,10 },	{ 24,22 },
        { 33,19 },	{ 22,32 },	{ 26,31 },	{ 21,41 },	{ 26,44 },	{ 23,47 },
        { 16,65 },	{ 14,71 },

        // Table 9-32 - Values of variables m and n for ctxIdx from 952 to 1011
        { -6,76 },	{ -2,44 },	{ 0,45 },	{ 0,52 },
        { -3,64 },	{ -2,59 },	{ -4,70 },	{ -4,75 },	{ -8,82 },	{ -17,102 },
        { -9,77 },	{ 3,24 },	{ 0,42 },	{ 0,48 },	{ 0,55 },	{ -6,59 },
        { -7,71 },	{ -12,83 },	{ -11,87 },	{ -30,119 },	{ 1,58 },	{ -3,29 },
        { -1,36 },	{ 1,38 },	{ 2,43 },	{ -6,55 },	{ 0,58 },	{ 0,64 },
        { -3,74 },	{ -10,90 },	{ -6,76 },	{ -2,44 },	{ 0,45 },	{ 0,52 },
        { -3,64 },	{ -2,59 },	{ -4,70 },	{ -4,75 },	{ -8,82 },	{ -17,102 },
        { -9,77 },	{ 3,24 },	{ 0,42 },	{ 0,48 },	{ 0,55 },	{ -6,59 },
        { -7,71 },	{ -12,83 },	{ -11,87 },	{ -30,119 },	{ 1,58 },	{ -3,29 },
        { -1,36 },	{ 1,38 },	{ 2,43 },	{ -6,55 },	{ 0,58 },	{ 0,64 },
        { -3,74 },	{ -10,90 },

        // Table 9-33 - Values of variables m and n for ctxIdx from 1012 to 1023
        { -3,74 },	{ -9,92 },	{ -8,87 },	{ -23,126 },
        { -3,74 },	{ -9,92 },	{ -8,87 },	{ -23,126 },	{ -3,74 },	{ -9,92 },
        { -8,87 },	{ -23,126 },
    },

    //cabac_init_idc = 1
    {
        // Table 9-12 - Values of variables m and n for ctxIdx from 0 to 10
        { 20, -15 }, { 2, 54 }, { 3, 74 }, { 20, -15 },{ 2, 54 }, { 3, 74 },
        { -28, 127 }, { -23, 104 },{ -6, 53 }, { -1, 54 }, { 7, 51 },

        // Table 9-13 - Values of variables m and n for ctxIdx from 11 to 23
        { 22, 25 }, { 34, 0 }, { 16, 0 }, { -2, 9 },{ 4, 41 }, { -29, 118 },
        { 2, 65 }, { -6, 71 },{ -13, 79 }, { 5, 52 }, { 9, 50 }, { -3, 70 },{ 10, 54 },

        // Table 9-14 - Values of variables m and n for ctxIdx from 24 to 39
        { 26, 34 }, { 19, 22 }, { 40, 0 }, { 57, 2 },{ 41, 36 }, { 26, 69 },
        { -45, 127 }, { -15, 101 },{ -4, 76 }, { -6, 71 },
        { -13, 79 }, { 5, 52 },{ 6, 69 }, { -13, 90 }, { 0, 52 }, { 8, 43 },

        // Table 9-15 - Values of variables m and n for ctxIdx from 40 to 53
        { -2, 69 },{ -5, 82 },{ -10, 96 },{ 2, 59 },{ 2, 75 },{ -3, 87 },
        { -3, 100 },{ 1, 56 },{ -3, 74 },{ -6, 85 },{ 0, 59 },{ -3, 81 },
        { -7, 86 },{ -5, 95 },

        // Table 9-16 - Values of variables m and n for ctxIdx from 54 to 59
        { -1, 66 },{ -1, 77 },{ 1, 70 },{ -2, 86 },{ -5, 72 },{ 0, 61 },

        // Table 9-17 - Values of variables m and n for ctxIdx from 60 to 69
        { 0, 41 }, { 0, 63 }, { 0, 63 },  { 0, 63 },{ -9, 83 },
        { 4, 86 }, { 0, 97 },  { -7, 72 },{ 13, 41 }, { 3, 62 },

        // Table 9-18 - Values of variables m and n for ctxIdx from 70 to 104
        { 13,15 },	{ 7,51 },	{ 2,80 },	{ -39,127 },	{ -18,91 },	{ -17,96 },
        { -26,81 },	{ -35,98 },	{ -24,102 },	{ -23,97 },	{ -27,119 },	{ -24,99 },
        { -21,110 },	{ -18,102 },	{ -36,127 },	{ 0,80 },	{ -5,89 },	{ -7,94 },
        { -4,92 },	{ 0,39 },	{ 0,65 },	{ -15,84 },	{ -35,127 },	{ -2,73 },
        { -12,104 },	{ -9,91 },	{ -31,127 },	{ 3,55 },	{ 7,56 },	{ 7,55 },
        { 8,61 },	{ -3,53 },	{ 0,68 },	{ -7,74 },	{ -9,88 },

        // Table 9-19 - Values of variables m and n for ctxIdx from 105 to 165
        { -13,103 },
        { -13,91 },	{ -9,89 },	{ -14,92 },	{ -8,76 },	{ -12,87 },	{ -23,110 },
        { -24,105 },	{ -10,78 },	{ -20,112 },	{ -17,99 },	{ -78,127 },	{ -70,127 },
        { -50,127 },	{ -46,127 },	{ -4,66 },	{ -5,78 },	{ -4,71 },	{ -8,72 },
        { 2,59 },	{ -1,55 },	{ -7,70 },	{ -6,75 },	{ -8,89 },	{ -34,119 },
        { -3,75 },	{ 32,20 },	{ 30,22 },	{ -44,127 },	{ 0,54 },	{ -5,61 },
        { 0,58 },	{ -1,60 },	{ -3,61 },	{ -8,67 },	{ -25,84 },	{ -14,74 },
        { -5,65 },	{ 5,52 },	{ 2,57 },	{ 0,61 },	{ -9,69 },	{ -11,70 },
        { 18,55 },	{ -4,71 },	{ 0,58 },	{ 7,61 },	{ 9,41 },	{ 18,25 },
        { 9,32 },	{ 5,43 },	{ 9,47 },	{ 0,44 },	{ 0,51 },	{ 2,46 },
        { 19,38 },	{ -4,66 },	{ 15,38 },	{ 12,42 },	{ 9,34 },	{ 0,89 },

        // Table 9-20 - Values of variables m and n for ctxIdx from 166 to 226
        { 4,45 },	{ 10,28 },	{ 10,31 },	{ 33,-11 },	{ 52,-43 },	{ 18,15 },
        { 28,0 },	{ 35,-22 },	{ 38,-25 },	{ 34,0 },	{ 39,-18 },	{ 32,-12 },
        { 102,-94 },	{ 0,0 },	{ 56,-15 },	{ 33,-4 },	{ 29,10 },	{ 37,-5 },
        { 51,-29 },	{ 39,-9 },	{ 52,-34 },	{ 69,-58 },	{ 67,-63 },	{ 44,-5 },
        { 32,7 },	{ 55,-29 },	{ 32,1 },	{ 0,0 },	{ 27,36 },	{ 33,-25 },
        { 34,-30 },	{ 36,-28 },	{ 38,-28 },	{ 38,-27 },	{ 34,-18 },	{ 35,-16 },
        { 34,-14 },	{ 32,-8 },	{ 37,-6 },	{ 35,0 },	{ 30,10 },	{ 28,18 },
        { 26,25 },	{ 29,41 },	{ 0,75 },	{ 2,72 },	{ 8,77 },	{ 14,35 },
        { 18,31 },	{ 17,35 },	{ 21,30 },	{ 17,45 },	{ 20,42 },	{ 18,45 },
        { 27,26 },	{ 16,54 },	{ 7,66 },	{ 16,56 },	{ 11,73 },	{ 10,67 },
        { -10,116 },

        // Table 9-21 - Values of variables m and n for ctxIdx from 227 to 275
        { -23,112 },	{ -15,71 },	{ -7,61 },	{ 0,53 },	{ -5,66 },
        { -11,77 },	{ -9,80 },	{ -9,84 },	{ -10,87 },	{ -34,127 },	{ -21,101 },
        { -3,39 },	{ -5,53 },	{ -7,61 },	{ -11,75 },	{ -15,77 },	{ -17,91 },
        { -25,107 },	{ -25,111 },	{ -28,122 },	{ -11,76 },	{ -10,44 },	{ -10,52 },
        { -10,57 },	{ -9,58 },	{ -16,72 },	{ -7,69 },	{ -4,69 },	{ -5,74 },
        { -9,86 },	{ 2,66 },	{ -9,34 },	{ 1,32 },	{ 11,31 },	{ 5,52 },
        { -2,55 },	{ -2,67 },	{ 0,73 },	{ -8,89 },	{ 3,52 },	{ 7,4 },
        { 10,8 },	{ 17,8 },	{ 16,19 },	{ 3,37 },	{ -1,61 },	{ -5,73 },
        { -1,70 },	{ -4,78 },

        { HL_CODEC_264_NA,HL_CODEC_264_NA },//276 end_of_slice_flag

        // Table 9-22 - Values of variables m and n for ctxIdx from 277 to 337
        { -21,126 },	{ -23,124 },	{ -20,110 },	{ -26,126 },
        { -25,124 },	{ -17,105 },	{ -27,121 },	{ -27,117 },	{ -17,102 },	{ -26,117 },
        { -27,116 },	{ -33,122 },	{ -10,95 },	{ -14,100 },	{ -8,95 },	{ -17,111 },
        { -28,114 },	{ -6,89 },	{ -2,80 },	{ -4,82 },	{ -9,85 },	{ -8,81 },
        { -1,72 },	{ 5,64 },	{ 1,67 },	{ 9,56 },	{ 0,69 },	{ 1,69 },
        { 7,69 },	{ -7,69 },	{ -6,67 },	{ -16,77 },	{ -2,64 },	{ 2,61 },
        { -6,67 },	{ -3,64 },	{ 2,57 },	{ -3,65 },	{ -3,66 },	{ 0,62 },
        { 9,51 },	{ -1,66 },	{ -2,71 },	{ -2,75 },	{ -1,70 },	{ -9,72 },
        { 14,60 },	{ 16,37 },	{ 0,47 },	{ 18,35 },	{ 11,37 },	{ 12,41 },
        { 10,41 },	{ 2,48 },	{ 12,41 },	{ 13,41 },	{ 0,59 },	{ 3,50 },
        { 19,40 },	{ 3,66 },	{ 18,50 },

        // Table 9-23 - Values of variables m and n for ctxIdx from 338 to 398
        { 19,-6 },	{ 18,-6 },	{ 14,0 },
        { 26,-12 },	{ 31,-16 },	{ 33,-25 },	{ 33,-22 },	{ 37,-28 },	{ 39,-30 },
        { 42,-30 },	{ 47,-42 },	{ 45,-36 },	{ 49,-34 },	{ 41,-17 },	{ 32,9 },
        { 69,-71 },	{ 63,-63 },	{ 66,-64 },	{ 77,-74 },	{ 54,-39 },	{ 52,-35 },
        { 41,-10 },	{ 36,0 },	{ 40,-1 },	{ 30,14 },	{ 28,26 },	{ 23,37 },
        { 12,55 },	{ 11,65 },	{ 37,-33 },	{ 39,-36 },	{ 40,-37 },	{ 38,-30 },
        { 46,-33 },	{ 42,-30 },	{ 40,-24 },	{ 49,-29 },	{ 38,-12 },	{ 40,-10 },
        { 38,-3 },	{ 46,-5 },	{ 31,20 },	{ 29,30 },	{ 25,44 },	{ 12,48 },
        { 11,49 },	{ 26,45 },	{ 22,22 },	{ 23,22 },	{ 27,21 },	{ 33,20 },
        { 26,28 },	{ 30,24 },	{ 27,34 },	{ 18,42 },	{ 25,39 },	{ 18,50 },
        { 12,70 },	{ 21,54 },	{ 14,71 },	{ 11,83 },

        { 25, 32 }, { 21, 49 }, { 21, 54 },//399-401

        // Table 9-24 - Values of variables m and n for ctxIdx from 402 to 459
        { -5,85 },	{ -6,81 },
        { -10,77 },	{ -7,81 },	{ -17,80 },	{ -18,73 },	{ -4,74 },	{ -10,83 },
        { -9,71 },	{ -9,67 },	{ -1,61 },	{ -8,66 },	{ -14,66 },	{ 0,59 },
        { 2,59 },	{ 17,-10 },	{ 32,-13 },	{ 42,-9 },	{ 49,-5 },	{ 53,0 },
        { 64,3 },	{ 68,10 },	{ 66,27 },	{ 47,57 },	{ -5,71 },	{ 0,24 },
        { -1,36 },	{ -2,42 },	{ -2,52 },	{ -9,57 },	{ -6,63 },	{ -4,65 },
        { -4,67 },	{ -7,82 },	{ -3,81 },	{ -3,76 },	{ -7,72 },	{ -6,78 },
        { -12,72 },	{ -14,68 },	{ -3,70 },	{ -6,76 },	{ -5,66 },	{ -5,62 },
        { 0,57 },	{ -4,61 },	{ -9,60 },	{ 1,54 },	{ 2,58 },	{ 17,-10 },
        { 32,-13 },	{ 42,-9 },	{ 49,-5 },	{ 53,0 },	{ 64,3 },	{ 68,10 },
        { 66,27 },	{ 47,57 },

        // Table 9-25 - Values of variables m and n for ctxIdx from 460 to 483
        { 0,80 },	{ -5,89 },	{ -7,94 },	{ -4,92 },
        { 0,39 },	{ 0,65 },	{ -15,84 },	{ -35,127 },	{ -2,73 },	{ -12,104 },
        { -9,91 },	{ -31,127 },	{ 0,80 },	{ -5,89 },	{ -7,94 },	{ -4,92 },
        { 0,39 },	{ 0,65 },	{ -15,84 },	{ -35,127 },	{ -2,73 },	{ -12,104 },
        { -9,91 },	{ -31,127 },

        // Table 9-26 - Values of variables m and n for ctxIdx from 484 to 571
        { -13,103 },	{ -13,91 },	{ -9,89 },	{ -14,92 },
        { -8,76 },	{ -12,87 },	{ -23,110 },	{ -24,105 },	{ -10,78 },	{ -20,112 },
        { -17,99 },	{ -78,127 },	{ -70,127 },	{ -50,127 },	{ -46,127 },	{ -4,66 },
        { -5,78 },	{ -4,71 },	{ -8,72 },	{ 2,59 },	{ -1,55 },	{ -7,70 },
        { -6,75 },	{ -8,89 },	{ -34,119 },	{ -3,75 },	{ 32,20 },	{ 30,22 },
        { -44,127 },	{ 0,54 },	{ -5,61 },	{ 0,58 },	{ -1,60 },	{ -3,61 },
        { -8,67 },	{ -25,84 },	{ -14,74 },	{ -5,65 },	{ 5,52 },	{ 2,57 },
        { 0,61 },	{ -9,69 },	{ -11,70 },	{ 18,55 },	{ -13,103 },	{ -13,91 },
        { -9,89 },	{ -14,92 },	{ -8,76 },	{ -12,87 },	{ -23,110 },	{ -24,105 },
        { -10,78 },	{ -20,112 },	{ -17,99 },	{ -78,127 },	{ -70,127 },{ -46,127 },
        { -4,66 },	{ -5,78 },	{ -4,71 },	{ -8,72 },	{ 2,59 },	{ -1,55 },
        { -7,70 },	{ -6,75 },	{ -8,89 },	{ -34,119 },	{ -3,75 },	{ 32,20 },
        { 30,22 },	{ -44,127 },	{ 0,54 },	{ -5,61 },	{ 0,58 },	{ -1,60 },
        { -3,61 },	{ -8,67 },	{ -25,84 },	{ -14,74 },	{ -5,65 },	{ 5,52 },
        { 2,57 },	{ 0,61 },	{ -9,69 },	{ -11,70 },	{ 18,55 },

        // Table 9-27 - Values of variables m and n for ctxIdx from 572 to 659
        { 4,45 },
        { 10,28 },	{ 10,31 },	{ 33,-11 },	{ 52,-43 },	{ 18,15 },	{ 28,0 },
        { 35,-22 },	{ 38,-25 },	{ 34,0 },	{ 39,-18 },	{ 32,-12 },	{ 102,-94 },
        { 0,0 },	{ 56,-15 },	{ 33,-4 },	{ 29,10 },	{ 37,-5 },	{ 51,-29 },
        { 39,-9 },	{ 52,-34 },	{ 69,-58 },	{ 67,-63 },	{ 44,-5 },	{ 32,7 },
        { 55,-29 },	{ 32,1 },	{ 0,0 },	{ 27,36 },	{ 33,-25 },	{ 34,-30 },
        { 36,-28 },	{ 38,-28 },	{ 38,-27 },	{ 34,-18 },	{ 35,-16 },	{ 34,-14 },
        { 32,-8 },	{ 37,-6 },	{ 35,0 },	{ 30,10 },	{ 28,18 },	{ 26,25 },
        { 29,41 },	{ 4,45 },	{ 10,28 },	{ 10,31 },	{ 33,-11 },	{ 52,-43 },
        { 18,15 },	{ 28,0 },	{ 35,-22 },	{ 38,-25 },	{ 34,0 },	{ 39,-18 },
        { 32,-12 },	{ 102,-94 },	{ 0,0 },	{ 56,-15 },	{ 33,-4 },	{ 29,10 },
        { 37,-5 },	{ 51,-29 },	{ 39,-9 },	{ 52,-34 },	{ 69,-58 },	{ 67,-63 },
        { 44,-5 },	{ 32,7 },	{ 55,-29 },	{ -50,127 },	{ 32,1 },	{ 0,0 },
        { 27,36 },	{ 33,-25 },	{ 34,-30 },	{ 36,-28 },	{ 38,-28 },	{ 38,-27 },
        { 34,-18 },	{ 35,-16 },	{ 34,-14 },	{ 32,-8 },	{ 37,-6 },	{ 35,0 },
        { 30,10 },	{ 28,18 },	{ 26,25 },	{ 29,41 },

        // Table 9-28 - Values of variables m and n for ctxIdx from 660 to 717
        { -5,85 },	{ -6,81 },
        { -10,77 },	{ -7,81 },	{ -17,80 },	{ -18,73 },	{ -4,74 },	{ -10,83 },
        { -9,71 },	{ -9,67 },	{ -1,61 },	{ -8,66 },	{ -14,66 },	{ 0,59 },
        { 2,59 },	{ -3,81 },	{ -3,76 },	{ -7,72 },	{ -6,78 },	{ -12,72 },
        { -14,68 },	{ -3,70 },	{ -6,76 },	{ -5,66 },	{ -5,62 },	{ 0,57 },
        { -4,61 },	{ -9,60 },	{ 1,54 },	{ 2,58 },	{ 17,-10 },	{ 32,-13 },
        { 42,-9 },	{ 49,-5 },	{ 53,0 },	{ 64,3 },	{ 68,10 },	{ 66,27 },
        { 47,57 },	{ 17,-10 },	{ 32,-13 },	{ 42,-9 },	{ 49,-5 },	{ 53,0 },
        { 64,3 },	{ 68,10 },	{ 66,27 },	{ 47,57 },	{ -5,71 },	{ 0,24 },
        { -1,36 },	{ -2,42 },	{ -2,52 },	{ -9,57 },	{ -6,63 },	{ -4,65 },
        { -4,67 },	{ -7,82 },

        // Table 9-29 - Values of variables m and n for ctxIdx from 718 to 775
        { -5,85 },	{ -6,81 },	{ -10,77 },	{ -7,81 },
        { -17,80 },	{ -18,73 },	{ -4,74 },	{ -10,83 },	{ -9,71 },	{ -9,67 },
        { -1,61 },	{ -8,66 },	{ -14,66 },	{ 0,59 },	{ 2,59 },	{ -3,81 },
        { -3,76 },	{ -7,72 },	{ -6,78 },	{ -12,72 },	{ -14,68 },	{ -3,70 },
        { -6,76 },	{ -5,66 },	{ -5,62 },	{ 0,57 },	{ -4,61 },	{ -9,60 },
        { 1,54 },	{ 2,58 },	{ 17,-10 },	{ 32,-13 },	{ 42,-9 },	{ 49,-5 },
        { 53,0 },	{ 64,3 },	{ 68,10 },	{ 66,27 },	{ 47,57 },	{ 17,-10 },
        { 32,-13 },	{ 42,-9 },	{ 49,-5 },	{ 53,0 },	{ 64,3 },	{ 68,10 },
        { 66,27 },	{ 47,57 },	{ -5,71 },	{ 0,24 },	{ -1,36 },	{ -2,42 },
        { -2,52 },	{ -9,57 },	{ -6,63 },	{ -4,65 },	{ -4,67 },	{ -7,82 },

        // Table 9-30 - Values of variables m and n for ctxIdx from 776 to 863
        { -21,126 },	{ -23,124 },	{ -20,110 },	{ -26,126 },	{ -25,124 },	{ -17,105 },
        { -27,121 },	{ -27,117 },	{ -17,102 },	{ -26,117 },	{ -27,116 },	{ -33,122 },
        { -10,95 },	{ -14,100 },	{ -8,95 },	{ -17,111 },	{ -28,114 },	{ -6,89 },
        { -2,80 },	{ -4,82 },	{ -9,85 },	{ -8,81 },	{ -1,72 },	{ 5,64 },
        { 1,67 },	{ 9,56 },	{ 0,69 },	{ 1,69 },	{ 7,69 },	{ -7,69 },
        { -6,67 },	{ -16,77 },	{ -2,64 },	{ 2,61 },	{ -6,67 },	{ -3,64 },
        { 2,57 },	{ -3,65 },	{ -3,66 },	{ 0,62 },	{ 9,51 },	{ -1,66 },
        { -2,71 },	{ -2,75 },	{ -21,126 },	{ -23,124 },	{ -20,110 },	{ -26,126 },
        { -25,124 },	{ -17,105 },	{ -27,121 },	{ -27,117 },	{ -17,102 },	{ -26,117 },
        { -27,116 },	{ -33,122 },	{ -10,95 },	{ -14,100 },	{ -8,95 },	{ -17,111 },
        { -28,114 },	{ -6,89 },	{ -2,80 },	{ -4,82 },	{ -9,85 },	{ -8,81 },
        { -1,72 },	{ 5,64 },	{ 1,67 },	{ 9,56 },	{ 0,69 },	{ 1,69 },
        { 7,69 },	{ -7,69 },	{ -6,67 },	{ -16,77 },	{ -2,64 },	{ 2,61 },
        { -6,67 },	{ -3,64 },	{ 2,57 },	{ -3,65 },	{ -3,66 },	{ 0,62 },
        { 9,51 },	{ -1,66 },	{ -2,71 }, { -2,75 },

        // Table 9-31 - Values of variables m and n for ctxIdx from 864 to 951
        { 19,-6 },	{ 18,-6 },
        { 14,0 },	{ 26,-12 },	{ 31,-16 },	{ 33,-25 },	{ 33,-22 },	{ 37,-28 },
        { 39,-30 },	{ 42,-30 },	{ 47,-42 },	{ 45,-36 },	{ 49,-34 },	{ 41,-17 },
        { 32,9 },	{ 69,-71 },	{ 63,-63 },	{ 66,-64 },	{ 77,-74 },	{ 54,-39 },
        { 52,-35 },	{ 41,-10 },	{ 36,0 },	{ 40,-1 },	{ 30,14 },	{ 28,26 },
        { 23,37 },	{ 12,55 },	{ 11,65 },	{ 37,-33 },	{ 39,-36 },	{ 40,-37 },
        { 38,-30 },	{ 46,-33 },	{ 42,-30 },	{ 40,-24 },	{ 49,-29 },	{ 38,-12 },
        { 40,-10 },	{ 38,-3 },	{ 46,-5 },	{ 31,20 },	{ 29,30 },	{ 25,44 },
        { 19,-6 },	{ 18,-6 },	{ 14,0 },	{ 26,-12 },	{ 31,-16 },	{ 33,-25 },
        { 33,-22 },	{ 37,-28 },	{ 39,-30 },	{ 42,-30 },	{ 47,-42 },	{ 45,-36 },
        { 49,-34 },	{ 41,-17 },	{ 32,9 },	{ 69,-71 },	{ 63,-63 },	{ 66,-64 },
        { 77,-74 },	{ 54,-39 },	{ 52,-35 },	{ 41,-10 },	{ 36,0 },	{ 40,-1 },
        { 30,14 },	{ 28,26 },	{ 23,37 },	{ 12,55 },	{ 11,65 },	{ 37,-33 },
        { 39,-36 },	{ 40,-37 },	{ 38,-30 },	{ 46,-33 },	{ 42,-30 },	{ 40,-24 },
        { 49,-29 },	{ 38,-12 },	{ 40,-10 },	{ 38,-3 },	{ 46,-5 },	{ 31,20 },
        { 29,30 },	{ 25,44 },

        // Table 9-32 - Values of variables m and n for ctxIdx from 952 to 1011
        { -23,112 },	{ -15,71 },	{ -7,61 },	{ 0,53 },
        { -5,66 },	{ -11,77 },	{ -9,80 },	{ -9,84 },	{ -10,87 },	{ -34,127 },
        { -21,101 },	{ -3,39 },	{ -5,53 },	{ -7,61 },	{ -11,75 },	{ -15,77 },
        { -17,91 },	{ -25,107 },	{ -25,111 },	{ -28,122 },	{ -11,76 },	{ -10,44 },
        { -10,52 },	{ -10,57 },	{ -9,58 },	{ -16,72 },	{ -7,69 },	{ -4,69 },
        { -5,74 },	{ -9,86 },	{ -23,112 },	{ -15,71 },	{ -7,61 },	{ 0,53 },
        { -5,66 },	{ -11,77 },	{ -9,80 },	{ -9,84 },	{ -10,87 },	{ -34,127 },
        { -21,101 },	{ -3,39 },	{ -5,53 },	{ -7,61 },	{ -11,75 },	{ -15,77 },
        { -17,91 },	{ -25,107 },	{ -25,111 },	{ -28,122 },	{ -11,76 },	{ -10,44 },
        { -10,52 },	{ -10,57 },	{ -9,58 },	{ -16,72 },	{ -7,69 },	{ -4,69 },
        { -5,74 },	{ -9,86 },

        // Table 9-33 - Values of variables m and n for ctxIdx from 1012 to 1023
        { -2,73 },	{ -12,104 },	{ -9,91 },	{ -31,127 },
        { -2,73 },	{ -12,104 },	{ -9,91 },	{ -31,127 },	{ -2,73 },	{ -12,104 },
        { -9,91 },	{ -31,127 },
    },

    //cabac_init_idc = 2
    {

        // Table 9-12 - Values of variables m and n for ctxIdx from 0 to 10
        { 20, -15 }, { 2, 54 }, { 3, 74 }, { 20, -15 },{ 2, 54 }, { 3, 74 },
        { -28, 127 }, { -23, 104 },{ -6, 53 }, { -1, 54 }, { 7, 51 },

        // Table 9-13 - Values of variables m and n for ctxIdx from 11 to 23
        { 29, 16 }, { 25, 0 }, { 14, 0 }, { -10, 51 },{ -3, 62 }, { -27, 99 },
        { 26, 16 }, { -4, 85 },{ -24, 102 }, { 5, 57 }, { 6, 57 }, { -17, 73 },{ 14, 57 },

        // Table 9-14 - Values of variables m and n for ctxIdx from 24 to 39
        { 20, 40 }, { 20, 10 }, { 29, 0 }, { 54, 0 },{ 37, 42 }, { 12, 97 },
        { -32, 127 }, { -22, 117 },{ -2, 74 }, { -4, 85 }, { -24, 102 }, { 5, 57 },
        { -6, 93 }, { -14, 88 }, { -6, 44 }, { 4, 55 },

        // Table 9-15 - Values of variables m and n for ctxIdx from 40 to 53
        { -11, 89 },{ -15, 103 },{ -21, 116 },{ 19, 57 },{ 20, 58 },{ 4, 84 },
        { 6, 96 },{ 1, 63 },{ -5, 85 },{ -13, 106 },{ 5, 63 },{ 6, 75 },{ -3, 90 },{ -1, 101 },

        // Table 9-16 - Values of variables m and n for ctxIdx from 54 to 59
        { 3, 55 },{ -4, 79 },{ -2, 75 },{ -12, 97 },{ -7, 50 },{ 1, 60 },

        // Table 9-17 - Values of variables m and n for ctxIdx from 60 to 69
        { 0, 41 }, { 0, 63 }, { 0, 63 },  { 0, 63 },{ -9, 83 },
        { 4, 86 }, { 0, 97 },  { -7, 72 },{ 13, 41 }, { 3, 62 },

        // Table 9-18 - Values of variables m and n for ctxIdx from 70 to 104
        { 7,34 },	{ -9,88 },	{ -20,127 },	{ -36,127 },	{ -17,91 },	{ -14,95 },
        { -25,84 },	{ -25,86 },	{ -12,89 },	{ -17,91 },	{ -31,127 },	{ -14,76 },
        { -18,103 },	{ -13,90 },	{ -37,127 },	{ 11,80 },	{ 5,76 },	{ 2,84 },
        { 5,78 },	{ -6,55 },	{ 4,61 },	{ -14,83 },	{ -37,127 },	{ -5,79 },
        { -11,104 },	{ -11,91 },	{ -30,127 },	{ 0,65 },	{ -2,79 },	{ 0,72 },
        { -4,92 },	{ -6,56 },	{ 3,68 },	{ -8,71 },	{ -13,98 },

        // Table 9-19 - Values of variables m and n for ctxIdx from 105 to 165
        { -4,86 },
        { -12,88 },	{ -5,82 },	{ -3,72 },	{ -4,67 },	{ -8,72 },	{ -16,89 },
        { -9,69 },	{ -1,59 },	{ 5,66 },	{ 4,57 },	{ -4,71 },	{ -2,71 },
        { 2,58 },	{ -1,74 },	{ -4,44 },	{ -1,69 },	{ 0,62 },	{ -7,51 },
        { -4,47 },	{ -6,42 },	{ -3,41 },	{ -6,53 },	{ 8,76 },	{ -9,78 },
        { -11,83 },	{ 9,52 },	{ 0,67 },	{ -5,90 },	{ 1,67 },	{ -15,72 },
        { -5,75 },	{ -8,80 },	{ -21,83 },	{ -21,64 },	{ -13,31 },	{ -25,64 },
        { -29,94 },	{ 9,75 },	{ 17,63 },	{ -8,74 },	{ -5,35 },	{ -2,27 },
        { 13,91 },	{ 3,65 },	{ -7,69 },	{ 8,77 },	{ -10,66 },	{ 3,62 },
        { -3,68 },	{ -20,81 },	{ 0,30 },	{ 1,7 },	{ -3,23 },	{ -21,74 },
        { 16,66 },	{ -23,124 },	{ 17,37 },	{ 44,-18 },	{ 50,-34 },	{ -22,127 },

        // Table 9-20 - Values of variables m and n for ctxIdx from 166 to 226
        { 4,39 },	{ 0,42 },	{ 7,34 },	{ 11,29 },	{ 8,31 },	{ 6,37 },
        { 7,42 },	{ 3,40 },	{ 8,33 },	{ 13,43 },	{ 13,36 },	{ 4,47 },
        { 3,55 },	{ 2,58 },	{ 6,60 },	{ 8,44 },	{ 11,44 },	{ 14,42 },
        { 7,48 },	{ 4,56 },	{ 4,52 },	{ 13,37 },	{ 9,49 },	{ 19,58 },
        { 10,48 },	{ 12,45 },	{ 0,69 },	{ 20,33 },	{ 8,63 },	{ 35,-18 },
        { 33,-25 },	{ 28,-3 },	{ 24,10 },	{ 27,0 },	{ 34,-14 },	{ 52,-44 },
        { 39,-24 },	{ 19,17 },	{ 31,25 },	{ 36,29 },	{ 24,33 },	{ 34,15 },
        { 30,20 },	{ 22,73 },	{ 20,34 },	{ 19,31 },	{ 27,44 },	{ 19,16 },
        { 15,36 },	{ 15,36 },	{ 21,28 },	{ 25,21 },	{ 30,20 },	{ 31,12 },
        { 27,16 },	{ 24,42 },	{ 0,93 },	{ 14,56 },	{ 15,57 },	{ 26,38 },
        { -24,127 },

        // Table 9-21 - Values of variables m and n for ctxIdx from 227 to 275
        { -24,115 },	{ -22,82 },	{ -9,62 },	{ 0,53 },	{ 0,59 },
        { -14,85 },	{ -13,89 },	{ -13,94 },	{ -11,92 },	{ -29,127 },	{ -21,100 },
        { -14,57 },	{ -12,67 },	{ -11,71 },	{ -10,77 },	{ -21,85 },	{ -16,88 },
        { -23,104 },	{ -15,98 },	{ -37,127 },	{ -10,82 },	{ -8,48 },	{ -8,61 },
        { -8,66 },	{ -7,70 },	{ -14,75 },	{ -10,79 },	{ -9,83 },	{ -12,92 },
        { -18,108 },	{ -4,79 },	{ -22,69 },	{ -16,75 },	{ -2,58 },	{ 1,58 },
        { -13,78 },	{ -9,83 },	{ -4,81 },	{ -13,99 },	{ -13,81 },	{ -6,38 },
        { -13,62 },	{ -6,58 },	{ -2,59 },	{ -16,73 },	{ -10,76 },	{ -13,86 },
        { -9,83 },	{ -10,87 },

        { HL_CODEC_264_NA,HL_CODEC_264_NA },//276 end_of_slice_flag

        // Table 9-22 - Values of variables m and n for ctxIdx from 277 to 337
        { -22,127 },	{ -25,127 },	{ -25,120 },	{ -27,127 },
        { -19,114 },	{ -23,117 },	{ -25,118 },	{ -26,117 },	{ -24,113 },	{ -28,118 },
        { -31,120 },	{ -37,124 },	{ -10,94 },	{ -15,102 },	{ -10,99 },	{ -13,106 },
        { -50,127 },	{ -5,92 },	{ 17,57 },	{ -5,86 },	{ -13,94 },	{ -12,91 },
        { -2,77 },	{ 0,71 },	{ -1,73 },	{ 4,64 },	{ -7,81 },	{ 5,64 },
        { 15,57 },	{ 1,67 },{ 0,68 },	{ -10,67 },	{ 1,68 },	{ 0,77 },
        { 2,64 },	{ 0,68 },	{ -5,78 },	{ 7,55 },	{ 5,59 },	{ 2,65 },
        { 14,54 },	{ 15,44 },	{ 5,60 },	{ 2,70 },	{ -2,76 },	{ -18,86 },
        { 12,70 },	{ 5,64 },	{ -12,70 },	{ 11,55 },	{ 5,56 },	{ 0,69 },
        { 2,65 },	{ -6,74 },	{ 5,54 },	{ 7,54 },	{ -6,76 },	{ -11,82 },
        { -2,77 },	{ -2,77 },	{ 25,42 },

        // Table 9-23 - Values of variables m and n for ctxIdx from 338 to 398
        { 17,-13 },	{ 16,-9 },	{ 17,-12 },
        { 27,-21 },	{ 37,-30 },	{ 41,-40 },	{ 42,-41 },	{ 48,-47 },	{ 39,-32 },
        { 46,-40 },	{ 52,-51 },	{ 46,-41 },	{ 52,-39 },	{ 43,-19 },	{ 32,11 },
        { 61,-55 },	{ 56,-46 },	{ 62,-50 },	{ 81,-67 },	{ 45,-20 },	{ 35,-2 },
        { 28,15 },	{ 34,1 },	{ 39,1 },	{ 30,17 },	{ 20,38 },	{ 18,45 },
        { 15,54 },	{ 0,79 },	{ 36,-16 },	{ 37,-14 },	{ 37,-17 },	{ 32,1 },
        { 34,15 },	{ 29,15 },	{ 24,25 },	{ 34,22 },	{ 31,16 },	{ 35,18 },
        { 31,28 },	{ 33,41 },	{ 36,28 },	{ 27,47 },	{ 21,62 },	{ 18,31 },
        { 19,26 },	{ 36,24 },	{ 24,23 },	{ 27,16 },	{ 24,30 },	{ 31,29 },
        { 22,41 },	{ 22,42 },	{ 16,60 },	{ 15,52 },	{ 14,60 },	{ 3,78 },
        { -16,123 },	{ 21,53 },	{ 22,56 },	{ 25,61 },

        { 21,  33 }, { 19, 50 }, { 17, 61 }, //399-401

        // Table 9-24 - Values of variables m and n for ctxIdx from 402 to 459
        { -3,78 },	{ -8,74 },
        { -9,72 },	{ -10,72 },	{ -18,75 },	{ -12,71 },	{ -11,63 },	{ -5,70 },
        { -17,75 },	{ -14,72 },	{ -16,67 },	{ -8,53 },	{ -14,59 },	{ -9,52 },
        { -11,68 },	{ 9,-2 },	{ 30,-10 },	{ 31,-4 },	{ 33,-1 },	{ 33,7 },
        { 31,12 },	{ 37,23 },	{ 31,38 },	{ 20,64 },	{ -9,71 },	{ -7,37 },
        { -8,44 },	{ -11,49 },	{ -10,56 },	{ -12,59 },	{ -8,63 },	{ -9,67 },
        { -6,68 },	{ -10,79 },	{ -3,78 },	{ -8,74 },	{ -9,72 },	{ -10,72 },
        { -18,75 },	{ -12,71 },	{ -11,63 },	{ -5,70 },	{ -17,75 },	{ -14,72 },
        { -16,67 },	{ -8,53 },	{ -14,59 },	{ -9,52 },	{ -11,68 },	{ 9,-2 },
        { 30,-10 },	{ 31,-4 },	{ 33,-1 },	{ 33,7 },	{ 31,12 },	{ 37,23 },
        { 31,38 },	{ 20,64 },

        // Table 9-25 - Values of variables m and n for ctxIdx from 460 to 483
        { 11,80 },	{ 5,76 },	{ 2,84 },	{ 5,78 },
        { -6,55 },	{ 4,61 },	{ -14,83 },	{ -37,127 },	{ -5,79 },	{ -11,104 },
        { -11,91 },	{ -30,127 },	{ 11,80 },	{ 5,76 },	{ 2,84 },	{ 5,78 },
        { -6,55 },	{ 4,61 },	{ -14,83 },	{ -37,127 },	{ -5,79 },	{ -11,104 },
        { -11,91 },	{ -30,127 },

        // Table 9-26 - Values of variables m and n for ctxIdx from 484 to 571
        { -4,86 },	{ -12,88 },	{ -5,82 },	{ -3,72 },
        { -4,67 },	{ -8,72 },	{ -16,89 },	{ -9,69 },	{ -1,59 },	{ 5,66 },
        { 4,57 },	{ -4,71 },	{ -2,71 },	{ 2,58 },	{ -1,74 },	{ -4,44 },
        { -1,69 },	{ 0,62 },	{ -7,51 },	{ -4,47 },	{ -6,42 },	{ -3,41 },
        { -6,53 },	{ 8,76 },	{ -9,78 },	{ -11,83 },	{ 9,52 },	{ 0,67 },
        { -5,90 },	{ 1,67 },	{ -15,72 },	{ -5,75 },	{ -8,80 },	{ -21,83 },
        { -21,64 },	{ -13,31 },	{ -25,64 },	{ -29,94 },	{ 9,75 },	{ 17,63 },
        { -8,74 },	{ -5,35 },	{ -2,27 },	{ 13,91 },	{ -4,86 },	{ -12,88 },
        { -5,82 },	{ -3,72 },	{ -4,67 },	{ -8,72 },	{ -16,89 },	{ -9,69 },
        { -1,59 },	{ 5,66 },	{ 4,57 },	{ -4,71 },	{ -2,71 },	{ -1,74 },
        { -4,44 },	{ -1,69 },	{ 0,62 },	{ -7,51 },	{ -4,47 },	{ -6,42 },
        { -3,41 },	{ -6,53 },	{ 8,76 },	{ -9,78 },	{ -11,83 },	{ 9,52 },
        { 0,67 },	{ -5,90 },	{ 1,67 },	{ -15,72 },	{ -5,75 },	{ -8,80 },
        { -21,83 },	{ -21,64 },	{ -13,31 },	{ -25,64 },	{ -29,94 },	{ 9,75 },
        { 17,63 },	{ -8,74 },	{ -5,35 },	{ -2,27 },	{ 13,91 },

        // Table 9-27 - Values of variables m and n for ctxIdx from 572 to 659
        { 4,39 },
        { 0,42 },	{ 7,34 },	{ 11,29 },	{ 8,31 },	{ 6,37 },	{ 7,42 },
        { 3,40 },	{ 8,33 },	{ 13,43 },	{ 13,36 },	{ 4,47 },	{ 3,55 },
        { 2,58 },	{ 6,60 },	{ 8,44 },	{ 11,44 },	{ 14,42 },	{ 7,48 },
        { 4,56 },	{ 4,52 },	{ 13,37 },	{ 9,49 },	{ 19,58 },	{ 10,48 },
        { 12,45 },	{ 0,69 },	{ 20,33 },	{ 8,63 },	{ 35,-18 },	{ 33,-25 },
        { 28,-3 },	{ 24,10 },	{ 27,0 },	{ 34,-14 },	{ 52,-44 },	{ 39,-24 },
        { 19,17 },	{ 31,25 },	{ 36,29 },	{ 24,33 },	{ 34,15 },	{ 30,20 },
        { 22,73 },	{ 4,39 },	{ 0,42 },	{ 7,34 },	{ 11,29 },	{ 8,31 },
        { 6,37 },	{ 7,42 },	{ 3,40 },	{ 8,33 },	{ 13,43 },	{ 13,36 },
        { 4,47 },	{ 3,55 },	{ 2,58 },	{ 6,60 },	{ 8,44 },	{ 11,44 },
        { 14,42 },	{ 7,48 },	{ 4,56 },	{ 4,52 },	{ 13,37 },	{ 9,49 },
        { 19,58 },	{ 10,48 },	{ 12,45 },	{ 2,58 },	{ 0,69 },	{ 20,33 },
        { 8,63 },	{ 35,-18 },	{ 33,-25 },	{ 28,-3 },	{ 24,10 },	{ 27,0 },
        { 34,-14 },	{ 52,-44 },	{ 39,-24 },	{ 19,17 },	{ 31,25 },	{ 36,29 },
        { 24,33 },	{ 34,15 },	{ 30,20 },	{ 22,73 },

        // Table 9-28 - Values of variables m and n for ctxIdx from 660 to 717
        { -3,78 },	{ -8,74 },
        { -9,72 },	{ -10,72 },	{ -18,75 },	{ -12,71 },	{ -11,63 },	{ -5,70 },
        { -17,75 },	{ -14,72 },	{ -16,67 },	{ -8,53 },	{ -14,59 },	{ -9,52 },
        { -11,68 },	{ -3,78 },	{ -8,74 },	{ -9,72 },	{ -10,72 },	{ -18,75 },
        { -12,71 },	{ -11,63 },	{ -5,70 },	{ -17,75 },	{ -14,72 },	{ -16,67 },
        { -8,53 },	{ -14,59 },	{ -9,52 },	{ -11,68 },	{ 9,-2 },	{ 30,-10 },
        { 31,-4 },	{ 33,-1 },	{ 33,7 },	{ 31,12 },	{ 37,23 },	{ 31,38 },
        { 20,64 },	{ 9,-2 },	{ 30,-10 },	{ 31,-4 },	{ 33,-1 },	{ 33,7 },
        { 31,12 },	{ 37,23 },	{ 31,38 },	{ 20,64 },	{ -9,71 },	{ -7,37 },
        { -8,44 },	{ -11,49 },	{ -10,56 },	{ -12,59 },	{ -8,63 },	{ -9,67 },
        { -6,68 },	{ -10,79 },

        // Table 9-29 - Values of variables m and n for ctxIdx from 718 to 775
        { -3,78 },	{ -8,74 },	{ -9,72 },	{ -10,72 },
        { -18,75 },	{ -12,71 },	{ -11,63 },	{ -5,70 },	{ -17,75 },	{ -14,72 },
        { -16,67 },	{ -8,53 },	{ -14,59 },	{ -9,52 },	{ -11,68 },	{ -3,78 },
        { -8,74 },	{ -9,72 },	{ -10,72 },	{ -18,75 },	{ -12,71 },	{ -11,63 },
        { -5,70 },	{ -17,75 },	{ -14,72 },	{ -16,67 },	{ -8,53 },	{ -14,59 },
        { -9,52 },	{ -11,68 },	{ 9,-2 },	{ 30,-10 },	{ 31,-4 },	{ 33,-1 },
        { 33,7 },	{ 31,12 },	{ 37,23 },	{ 31,38 },	{ 20,64 },	{ 9,-2 },
        { 30,-10 },	{ 31,-4 },	{ 33,-1 },	{ 33,7 },	{ 31,12 },	{ 37,23 },
        { 31,38 },	{ 20,64 },	{ -9,71 },	{ -7,37 },	{ -8,44 },	{ -11,49 },
        { -10,56 },	{ -12,59 },	{ -8,63 },	{ -9,67 },	{ -6,68 },	{ -10,79 },

        // Table 9-30 - Values of variables m and n for ctxIdx from 776 to 863
        { -22,127 },	{ -25,127 },	{ -25,120 },	{ -27,127 },	{ -19,114 },	{ -23,117 },
        { -25,118 },	{ -26,117 },	{ -24,113 },	{ -28,118 },	{ -31,120 },	{ -37,124 },
        { -10,94 },	{ -15,102 },	{ -10,99 },	{ -13,106 },	{ -50,127 },	{ -5,92 },
        { 17,57 },	{ -5,86 },	{ -13,94 },	{ -12,91 },	{ -2,77 },	{ 0,71 },
        { -1,73 },	{ 4,64 },	{ -7,81 },	{ 5,64 },	{ 15,57 },	{ 1,67 },
        { 0,68 },	{ -10,67 },	{ 1,68 },	{ 0,77 },	{ 2,64 },	{ 0,68 },
        { -5,78 },	{ 7,55 },	{ 5,59 },	{ 2,65 },	{ 14,54 },	{ 15,44 },
        { 5,60 },	{ 2,70 },	{ -22,127 },	{ -25,127 },	{ -25,120 },	{ -27,127 },
        { -19,114 },	{ -23,117 },	{ -25,118 },	{ -26,117 },	{ -24,113 },	{ -28,118 },
        { -31,120 },	{ -37,124 },	{ -10,94 },	{ -15,102 },	{ -10,99 },	{ -13,106 },
        { -50,127 },	{ -5,92 },	{ 17,57 },	{ -5,86 },	{ -13,94 },	{ -12,91 },
        { -2,77 },	{ 0,71 },	{ -1,73 },	{ 4,64 },	{ -7,81 },	{ 5,64 },
        { 15,57 },	{ 1,67 },	{ 0,68 },	{ -10,67 },	{ 1,68 },	{ 0,77 },
        { 2,64 },	{ 0,68 },	{ -5,78 },	{ 7,55 },	{ 5,59 },	{ 2,65 },
        { 14,54 },	{ 15,44 },	{ 5,60 },	{ 2,70 },

        // Table 9-31 - Values of variables m and n for ctxIdx from 864 to 951
        { 17,-13 },	{ 16,-9 },
        { 17,-12 },	{ 27,-21 },	{ 37,-30 },	{ 41,-40 },	{ 42,-41 },	{ 48,-47 },
        { 39,-32 },	{ 46,-40 },	{ 52,-51 },	{ 46,-41 },	{ 52,-39 },	{ 43,-19 },
        { 32,11 },	{ 61,-55 },	{ 56,-46 },	{ 62,-50 },	{ 81,-67 },	{ 45,-20 },
        { 35,-2 },	{ 28,15 },	{ 34,1 },	{ 39,1 },	{ 30,17 },	{ 20,38 },
        { 18,45 },	{ 15,54 },	{ 0,79 },	{ 36,-16 },	{ 37,-14 },	{ 37,-17 },
        { 32,1 },	{ 34,15 },	{ 29,15 },	{ 24,25 },	{ 34,22 },	{ 31,16 },
        { 35,18 },	{ 31,28 },	{ 33,41 },	{ 36,28 },	{ 27,47 },	{ 21,62 },
        { 17,-13 },	{ 16,-9 },	{ 17,-12 },	{ 27,-21 },	{ 37,-30 },	{ 41,-40 },
        { 42,-41 },	{ 48,-47 },	{ 39,-32 },	{ 46,-40 },	{ 52,-51 },	{ 46,-41 },
        { 52,-39 },	{ 43,-19 },	{ 32,11 },	{ 61,-55 },	{ 56,-46 },	{ 62,-50 },
        { 81,-67 },	{ 45,-20 },	{ 35,-2 },	{ 28,15 },	{ 34,1 },	{ 39,1 },
        { 30,17 },	{ 20,38 },	{ 18,45 },	{ 15,54 },	{ 0,79 },	{ 36,-16 },
        { 37,-14 },	{ 37,-17 },	{ 32,1 },	{ 34,15 },	{ 29,15 },	{ 24,25 },
        { 34,22 },	{ 31,16 },	{ 35,18 },	{ 31,28 },	{ 33,41 },	{ 36,28 },
        { 27,47 },	{ 21,62 },

        // Table 9-32 - Values of variables m and n for ctxIdx from 952 to 1011
        { -24,115 },	{ -22,82 },	{ -9,62 },	{ 0,53 },
        { 0,59 },	{ -14,85 },	{ -13,89 },	{ -13,94 },	{ -11,92 },	{ -29,127 },
        { -21,100 },	{ -14,57 },	{ -12,67 },	{ -11,71 },	{ -10,77 },	{ -21,85 },
        { -16,88 },	{ -23,104 },	{ -15,98 },	{ -37,127 },	{ -10,82 },	{ -8,48 },
        { -8,61 },	{ -8,66 },	{ -7,70 },	{ -14,75 },	{ -10,79 },	{ -9,83 },
        { -12,92 },	{ -18,108 },	{ -24,115 },	{ -22,82 },	{ -9,62 },	{ 0,53 },
        { 0,59 },	{ -14,85 },	{ -13,89 },	{ -13,94 },	{ -11,92 },	{ -29,127 },
        { -21,100 },	{ -14,57 },	{ -12,67 },	{ -11,71 },	{ -10,77 },	{ -21,85 },
        { -16,88 },	{ -23,104 },	{ -15,98 },	{ -37,127 },	{ -10,82 },	{ -8,48 },
        { -8,61 },	{ -8,66 },	{ -7,70 },	{ -14,75 },	{ -10,79 },	{ -9,83 },
        { -12,92 },	{ -18,108 },

        // Table 9-33 - Values of variables m and n for ctxIdx from 1012 to 1023
        { -5,79 },	{ -11,104 },	{ -11,91 },	{ -30,127 },
        { -5,79 },	{ -11,104 },	{ -11,91 },	{ -30,127 },	{ -5,79 },	{ -11,104 },
        { -11,91 },	{ -30,127 },
    }
};

HL_BEGIN_DECLS

#endif /* _HARTALLO_CODEC_264_TABLES_H_ */
