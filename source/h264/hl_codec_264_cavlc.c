/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo (From Android source code)
 * Copyright (C) 2012-2013 Mamadou DIOP
 *
 * This file is part of Hartallo Project <http://code.google.com/p/hartallo/>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#include "hartallo/h264/hl_codec_264_cavlc.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/h264/hl_codec_264_bits.h"
#include "hartallo/h264/hl_codec_264_defs.h"
#include "hartallo/hl_bits.h"
#include "hartallo/hl_debug.h"

#define HL_NO_CLZ 1 // FIXME: Bourne crashing and it's slow

/*
	Maximum value for levelCode is calculated by reverse enginering '7.3.5.3.2 Residual block CAVLC syntax' as follows:

	1.	Max values for 'level_prefix' is equal to (11 + bitDepth) (http://lists.mpegif.org/pipermail/mp4-tech/2008-June/008124.html).
		In our case (Base, Main and High profiles), bitDepth = 8 which means that the max value for Max(level_prefix)=19
	2.	'suffixLength' values is from 0-6 (each one define a VLC table). Max(suffixLength)=6
	3.  'level_suffix' is the value defined by 'suffixLength's bits which means that Max(level_suffix)=1<<(Max(suffixLength)+1)= 1<<(6+1) = 1<<7 = 128
	3.	Max(levelCode) = HL_MATH_MIN(15, Max(level_prefix)) << Max(suffixLength) = (15 << 6) = 960;
	4.	if(suffixLength > 0 || level_prefix >= 14) then levelCode += level_suffix; -> Max(levelCode)=Prev(Max(levelCode))+Max(level_suffix)=960+128=1088
	6.	if(level_prefix >= 15 && suffixLength == 0) then levelCode += 15; -> Max(levelCode)=Prev(Max(levelCode))+15=1088+15=1103
	7.	if(level_prefix >= 16) then levelCode += (1 << (level_prefix - 3)) - 4096; -> Max(levelCode)=Prev(Max(levelCode)) + (1 << (Max(level_prefix) - 3)) - 4096
			= Max(levelCode)=1103+ (1 << (19 - 3)) - 4096=1103 + (1 << 16) - 4096 = 1103+65536-4096=62543
	8.	if(coeff_index == TrailingOnes && TrailingOnes < 3) then levelCode += 2; -> Max(levelCode)=Prev(Max(levelCode))+2 = 62543+2 = 62545
	*/
#define HL_CAVLC_MAX_LEVEL_CODE			62545
#define MAX_BITDEPTH					8
//	A.2.1 Baseline profile, A.2.2 Main profile, A.2.3 Extended profile,
//	The syntax element level_prefix shall not be greater than 15 (when present)
#if 0 // using level_prefix >= 16 will cause overlap
#	define HL_CAVLC_MAX_LEVEL_PREFIX				11 + MAX_BITDEPTH
#else
#	define HL_CAVLC_MAX_LEVEL_PREFIX				15
#endif
#define HL_CAVLC_MAX_LEVEL_SUFFIX_LENGTH			6 // [0-6] VLC tables
#define HL_CAVLC_MAX_TRAILING_ONES				3
#define HL_CAVLC_MAX_COEFF_NUM					16

hl_codec_264_cavlc_level_xt (*hl_codec_264_cavlc_levels)[HL_CAVLC_MAX_LEVEL_SUFFIX_LENGTH + 1][HL_CAVLC_MAX_LEVEL_CODE + 1] = HL_NULL;

HL_ERROR_T hl_codec_264_cavlc_InitEncodingTable()
{
    static hl_codec_264_cavlc_level_xt _hl_codec_264_cavlc_levels[HL_CAVLC_MAX_LEVEL_SUFFIX_LENGTH + 1][HL_CAVLC_MAX_LEVEL_CODE + 1];
    if (!hl_codec_264_cavlc_levels) {
        int32_t suffixLength,level_suffix,level_prefix,levelCode,levelSuffixSize;

        hl_codec_264_cavlc_levels = &_hl_codec_264_cavlc_levels;

        for (level_prefix=0; level_prefix<=HL_CAVLC_MAX_LEVEL_PREFIX; ++level_prefix) {
            for (suffixLength=0; suffixLength<=HL_CAVLC_MAX_LEVEL_SUFFIX_LENGTH; ++suffixLength) {
                levelSuffixSize = suffixLength;
                if (level_prefix == 14 && suffixLength == 0) {
                    levelSuffixSize = 4;
                }
                else if (level_prefix >= 15) {
                    levelSuffixSize = level_prefix - 3;
                }
                for (level_suffix=0; level_suffix<=(1<<levelSuffixSize); ++level_suffix) {
                    levelCode = (HL_MATH_MIN(15, level_prefix) << suffixLength);

                    if(suffixLength > 0 || level_prefix >= 14) {
                        levelCode += level_suffix;
                    }
                    if(level_prefix >= 15 && suffixLength == 0) {
                        levelCode += 15;
                    }
                    if(level_prefix >= 16) {
                        levelCode += (1 << (level_prefix - 3)) - 4096;
                    }

                    // IMPORTANT: it's up to the caller to substract '2' from the levelCode value
                    //if(coeff_index == 1/ coeff_index == TrailingOnes && TrailingOnes < 3 /){
                    //	levelCode += 2;
                    //}

                    _hl_codec_264_cavlc_levels[suffixLength][levelCode].level_prefix = level_prefix;
                    _hl_codec_264_cavlc_levels[suffixLength][levelCode].level_suffix = level_suffix;
                    _hl_codec_264_cavlc_levels[suffixLength][levelCode].levelSuffixSize = levelSuffixSize;
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

#if !HL_NO_CLZ

typedef struct tagVLCNumCoeffTrail {
    int trailing;
    int total_coeff;
    int length;
} VLCNumCoeffTrail;

typedef struct tagShiftOffset {
    int shift;
    int offset;
} ShiftOffset;

const VLCNumCoeffTrail NumCoeffTrailOnes[3][67] = {
    {   {0, 0, 1}, {1, 1, 2}, {2, 2, 3}, {1, 2, 6}, {0, 1, 6}, {3, 3, 5}, {3, 3, 5}, {3, 5, 7},
        {2, 3, 7}, {3, 4, 6}, {3, 4, 6}, {3, 6, 8}, {2, 4, 8}, {1, 3, 8}, {0, 2, 8}, {3, 7, 9},
        {2, 5, 9}, {1, 4, 9}, {0, 3, 9}, {3, 8, 10}, {2, 6, 10}, {1, 5, 10}, {0, 4, 10}, {3, 9, 11},
        {2, 7, 11}, {1, 6, 11}, {0, 5, 11}, {0, 8, 13}, {2, 9, 13}, {1, 8, 13}, {0, 7, 13}, {3, 10, 13},
        {2, 8, 13}, {1, 7, 13}, {0, 6, 13}, {3, 12, 14}, {2, 11, 14}, {1, 10, 14}, {0, 10, 14}, {3, 11, 14},
        {2, 10, 14}, {1, 9, 14}, {0, 9, 14}, {3, 14, 15}, {2, 13, 15}, {1, 12, 15}, {0, 12, 15}, {3, 13, 15},
        {2, 12, 15}, {1, 11, 15}, {0, 11, 15}, {3, 16, 16}, {2, 15, 16}, {1, 15, 16}, {0, 14, 16}, {3, 15, 16},
        {2, 14, 16}, {1, 14, 16}, {0, 13, 16}, {0, 16, 16}, {2, 16, 16}, {1, 16, 16}, {0, 15, 16}, {1, 13, 15},
        { -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}
    },

    {   {1, 1, 2}, {0, 0, 2}, {3, 4, 4}, {3, 3, 4}, {2, 2, 3}, {2, 2, 3}, {3, 6, 6}, {2, 3, 6},
        {1, 3, 6}, {0, 1, 6}, {3, 5, 5}, {3, 5, 5}, {1, 2, 5}, {1, 2, 5}, {3, 7, 6}, {2, 4, 6},
        {1, 4, 6}, {0, 2, 6}, {3, 8, 7}, {2, 5, 7}, {1, 5, 7}, {0, 3, 7}, {0, 5, 8}, {2, 6, 8},
        {1, 6, 8}, {0, 4, 8}, {3, 9, 9}, {2, 7, 9}, {1, 7, 9}, {0, 6, 9}, {3, 11, 11}, {2, 9, 11},
        {1, 9, 11}, {0, 8, 11}, {3, 10, 11}, {2, 8, 11}, {1, 8, 11}, {0, 7, 11}, {0, 11, 12}, {2, 11, 12},
        {1, 11, 12}, {0, 10, 12}, {3, 12, 12}, {2, 10, 12}, {1, 10, 12}, {0, 9, 12}, {3, 14, 13}, {2, 13, 13},
        {1, 13, 13}, {0, 13, 13}, {3, 13, 13}, {2, 12, 13}, {1, 12, 13}, {0, 12, 13}, {1, 15, 14}, {0, 15, 14},
        {2, 15, 14}, {1, 14, 14}, {2, 14, 13}, {2, 14, 13}, {0, 14, 13}, {0, 14, 13}, {3, 16, 14}, {2, 16, 14},
        {1, 16, 14}, {0, 16, 14}, {3, 15, 13}
    },

    {   {3, 7, 4}, {3, 6, 4}, {3, 5, 4}, {3, 4, 4}, {3, 3, 4}, {2, 2, 4}, {1, 1, 4}, {0, 0, 4},
        {1, 5, 5}, {2, 5, 5}, {1, 4, 5}, {2, 4, 5}, {1, 3, 5}, {3, 8, 5}, {2, 3, 5}, {1, 2, 5},
        {0, 3, 6}, {2, 7, 6}, {1, 7, 6}, {0, 2, 6}, {3, 9, 6}, {2, 6, 6}, {1, 6, 6}, {0, 1, 6},
        {0, 7, 7}, {0, 6, 7}, {2, 9, 7}, {0, 5, 7}, {3, 10, 7}, {2, 8, 7}, {1, 8, 7}, {0, 4, 7},
        {3, 12, 8}, {2, 11, 8}, {1, 10, 8}, {0, 9, 8}, {3, 11, 8}, {2, 10, 8}, {1, 9, 8}, {0, 8, 8},
        {0, 12, 9}, {2, 13, 9}, {1, 12, 9}, {0, 11, 9}, {3, 13, 9}, {2, 12, 9}, {1, 11, 9}, {0, 10, 9},
        {1, 15, 10}, {0, 14, 10}, {3, 14, 10}, {2, 14, 10}, {1, 14, 10}, {0, 13, 10}, {1, 13, 9}, {1, 13, 9},
        {1, 16, 10}, {0, 15, 10}, {3, 15, 10}, {2, 15, 10}, {3, 16, 10}, {2, 16, 10}, {0, 16, 10}, { -1, -1, -1},
        { -1, -1, -1}, { -1, -1, -1}, { -1, -1, -1}
    }
};


const ShiftOffset NumCoeffTrailOnes_indx[3][15] = {
    {   {15, -1}, {14, 0}, {13, 1}, {10, -1}, {9, 3}, {8, 7}, {7, 11}, {6, 15},
        {5, 19}, {3, 19}, {2, 27}, {1, 35}, {0, 43}, {0, 55}, {1, 62}
    },

    {   {14, -2}, {12, -2}, {10, -2}, {10, 10}, {9, 14}, {8, 18}, {7, 22}, {5, 22},
        {4, 30}, {3, 38}, {2, 46}, {2, 58}, {3, 65}, {16, 0}, {16, 0}
    },

    {   {12, -8}, {11, 0}, {10, 8}, {9, 16}, {8, 24}, {7, 32}, {6, 40}, {6, 52},
        {6, 58}, {6, 61}, {16, 0}, {16, 0}, {16, 0}, {16, 0}, {16, 0}
    }
};

const static int nC_table[8] = {0, 0, 1, 1, 2, 2, 2, 2};

#endif


HL_ERROR_T hl_codec_264_cavlc_ReadTotalCoeffTrailingOnes(const hl_codec_264_t* pc_codec, uint32_t *TrailingOnes, uint32_t *TotalCoeff, int32_t nC)
{
#if HL_NO_CLZ
    const static uint8_t TotCofNTrail1[75][3] = {{0, 0, 16}/*error */, {0, 0, 16}/*error */, {1, 13, 15}, {1, 13, 15}, {0, 16, 16}, {2, 16, 16}, {1, 16, 16}, {0, 15, 16},
        {3, 16, 16}, {2, 15, 16}, {1, 15, 16}, {0, 14, 16}, {3, 15, 16}, {2, 14, 16}, {1, 14, 16}, {0, 13, 16},
        {3, 14, 15}, {2, 13, 15}, {1, 12, 15}, {0, 12, 15}, {3, 13, 15}, {2, 12, 15}, {1, 11, 15}, {0, 11, 15},
        {3, 12, 14}, {2, 11, 14}, {1, 10, 14}, {0, 10, 14}, {3, 11, 14}, {2, 10, 14}, {1, 9, 14}, {0, 9, 14},
        {0, 8, 13}, {2, 9, 13}, {1, 8, 13}, {0, 7, 13}, {3, 10, 13}, {2, 8, 13}, {1, 7, 13}, {0, 6, 13},
        {3, 9, 11}, {2, 7, 11}, {1, 6, 11}, {0, 5, 11}, {3, 8, 10},
        {2, 6, 10}, {1, 5, 10}, {0, 4, 10}, {3, 7, 9}, {2, 5, 9}, {1, 4, 9}, {0, 3, 9}, {3, 6, 8},
        {2, 4, 8}, {1, 3, 8}, {0, 2, 8}, {3, 5, 7}, {2, 3, 7}, {3, 4, 6}, {3, 4, 6}, {1, 2, 6},
        {1, 2, 6}, {0, 1, 6}, {0, 1, 6}, {3, 3, 5}, {3, 3, 5}, {3, 3, 5}, {3, 3, 5}, {2, 2, 3},
        {1, 1, 2}, {1, 1, 2}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}
    };

    const static uint8_t TotCofNTrail2[84][3] = {{0, 0, 14 /* error */}, {0, 0, 14/*error */}, {3, 15, 13}, {3, 15, 13}, {3, 16, 14}, {2, 16, 14}, {1, 16, 14}, {0, 16, 14},
        {1, 15, 14}, {0, 15, 14}, {2, 15, 14}, {1, 14, 14}, {2, 14, 13}, {2, 14, 13}, {0, 14, 13}, {0, 14, 13},
        {3, 14, 13}, {2, 13, 13}, {1, 13, 13}, {0, 13, 13}, {3, 13, 13}, {2, 12, 13}, {1, 12, 13}, {0, 12, 13},
        {0, 11, 12}, {2, 11, 12}, {1, 11, 12}, {0, 10, 12}, {3, 12, 12}, {2, 10, 12}, {1, 10, 12}, {0, 9, 12},
        {3, 11, 11}, {2, 9, 11}, {1, 9, 11}, {0, 8, 11}, {3, 10, 11}, {2, 8, 11}, {1, 8, 11}, {0, 7, 11},
        {3, 9, 9}, {2, 7, 9}, {1, 7, 9}, {0, 6, 9}, {0, 5, 8}, {0, 5, 8}, {2, 6, 8}, {2, 6, 8},
        {1, 6, 8}, {1, 6, 8}, {0, 4, 8}, {0, 4, 8}, {3, 8, 7}, {2, 5, 7}, {1, 5, 7}, {0, 3, 7},
        {3, 7, 6}, {3, 7, 6}, {2, 4, 6}, {2, 4, 6}, {1, 4, 6}, {1, 4, 6}, {0, 2, 6}, {0, 2, 6},
        {3, 6, 6}, {2, 3, 6}, {1, 3, 6}, {0, 1, 6}, {3, 5, 5}, {3, 5, 5}, {1, 2, 5}, {1, 2, 5},
        {3, 4, 4}, {3, 3, 4}, {2, 2, 3}, {2, 2, 3}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2}, {1, 1, 2},
        {0, 0, 2}, {0, 0, 2}, {0, 0, 2}, {0, 0, 2}
    };

    const static uint8_t TotCofNTrail3[64][3] = {{0, 0, 10/*error*/}, {0, 16, 10}, {3, 16, 10}, {2, 16, 10}, {1, 16, 10}, {0, 15, 10}, {3, 15, 10},
        {2, 15, 10}, {1, 15, 10}, {0, 14, 10}, {3, 14, 10}, {2, 14, 10}, {1, 14, 10}, {0, 13, 10}, {1, 13, 9},
        {1, 13, 9}, {0, 12, 9}, {2, 13, 9}, {1, 12, 9}, {0, 11, 9}, {3, 13, 9}, {2, 12, 9}, {1, 11, 9},
        {0, 10, 9}, {3, 12, 8}, {2, 11, 8}, {1, 10, 8}, {0, 9, 8}, {3, 11, 8}, {2, 10, 8}, {1, 9, 8},
        {0, 8, 8}, {0, 7, 7}, {0, 6, 7}, {2, 9, 7}, {0, 5, 7}, {3, 10, 7}, {2, 8, 7}, {1, 8, 7},
        {0, 4, 7}, {0, 3, 6}, {2, 7, 6}, {1, 7, 6}, {0, 2, 6}, {3, 9, 6}, {2, 6, 6}, {1, 6, 6},
        {0, 1, 6}, {1, 5, 5}, {2, 5, 5}, {1, 4, 5}, {2, 4, 5}, {1, 3, 5}, {3, 8, 5}, {2, 3, 5},
        {1, 2, 5}, {3, 7, 4}, {3, 6, 4}, {3, 5, 4}, {3, 4, 4}, {3, 3, 4}, {2, 2, 4}, {1, 1, 4},
        {0, 0, 4}
    };
#endif
    uint32_t code;

#if HL_NO_CLZ
    uint8_t *pcode;

    if (nC < 2) {
        code = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 16);

        if (code >= 8192) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>13)+65+2][0]);
        }
        else if (code >= 2048) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>9)+50+2][0]);
        }
        else if (code >= 1024) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>8)+46+2][0]);
        }
        else if (code >= 512) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>7)+42+2][0]);
        }
        else if (code >= 256) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>6)+38+2][0]);
        }
        else if (code >= 128) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>5)+34+2][0]);
        }
        else if (code >= 64) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>3)+22+2][0]);
        }
        else if (code >= 32) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>2)+14+2][0]);
        }
        else if (code >= 16) {
            pcode = (uint8_t*) & (TotCofNTrail1[(code>>1)+6+2][0]);
        }
        else {
            pcode = (uint8_t*) & (TotCofNTrail1[(code-2)+2][0]);
        }

        *TrailingOnes = pcode[0];
        *TotalCoeff = pcode[1];

        hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[2]);
    }
    else if (nC < 4) {
        code = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 14);

        if (code >= 4096) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>10)+66+2][0]);
        }
        else if (code >= 2048) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>8)+54+2][0]);
        }
        else if (code >= 512) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>7)+46+2][0]);
        }
        else if (code >= 128) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>5)+34+2][0]);
        }
        else if (code >= 64) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>3)+22+2][0]);
        }
        else if (code >= 32) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>2)+14+2][0]);
        }
        else if (code >= 16) {
            pcode = (uint8_t*) & (TotCofNTrail2[(code>>1)+6+2][0]);
        }
        else {
            pcode = (uint8_t*) & (TotCofNTrail2[code-2+2][0]);
        }
        *TrailingOnes = pcode[0];
        *TotalCoeff = pcode[1];

        hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[2]);
    }
    else if (nC < 8) {
        code = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 10);

        if (code >= 512) {
            pcode = (uint8_t*) & (TotCofNTrail3[(code>>6)+47+1][0]);
        }
        else if (code >= 256) {
            pcode = (uint8_t*) & (TotCofNTrail3[(code>>5)+39+1][0]);
        }
        else if (code >= 128) {
            pcode = (uint8_t*) & (TotCofNTrail3[(code>>4)+31+1][0]);
        }
        else if (code >= 64) {
            pcode = (uint8_t*) & (TotCofNTrail3[(code>>3)+23+1][0]);
        }
        else if (code >= 32) {
            pcode = (uint8_t*) & (TotCofNTrail3[(code>>2)+15+1][0]);
        }
        else if (code >= 16) {
            pcode = (uint8_t*) & (TotCofNTrail3[(code>>1)+7+1][0]);
        }
        else {
            pcode = (uint8_t*) & (TotCofNTrail3[code-1+1][0]);
        }
        *TrailingOnes = pcode[0];
        *TotalCoeff = pcode[1];

        hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[2]);
    }
    else {
        /* read 6 bit FLC */
        code = hl_codec_264_bits_read_u(pc_codec->pobj_bits, 6);


        *TrailingOnes = code & 3;
        *TotalCoeff = (code >> 2) + 1;

        if (*TotalCoeff > 16) {
            *TotalCoeff = 16;  // _ERROR
        }

        if (code == 3) {
            *TrailingOnes = 0;
            (*TotalCoeff)--;
        }
    }
#else
    const VLCNumCoeffTrail *ptr;
    const ShiftOffset *ptr_indx;
    uint32_t temp, leading_zeros = 0;

    if (nC < 8) {
        code = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 16);
        leading_zeros = (uint32_t)hl_bits_clz16(code);

        temp = nC_table[nC];
        ptr_indx = &NumCoeffTrailOnes_indx[temp][leading_zeros];
        ptr = &NumCoeffTrailOnes[temp][(code >> ptr_indx->shift) + ptr_indx->offset];
        *TrailingOnes = ptr->trailing;
        *TotalCoeff = ptr->total_coeff;
        hl_codec_264_bits_discard(pc_codec->pobj_bits, ptr->length);
    }
    else {
        /* read 6 bit FLC */
        code = hl_codec_264_bits_read_u(pc_codec->pobj_bits, 6);

        *TrailingOnes = code & 3;
        *TotalCoeff = (code >> 2) + 1;

        if (*TotalCoeff > 16) {
            *TotalCoeff = 16;  // _ERROR
        }

        if (code == 3) {
            *TrailingOnes = 0;
            (*TotalCoeff)--;
        }
    }
#endif
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_cavlc_ReadTotalCoeffTrailingOnesChromaDC(const hl_codec_264_t* pc_codec, uint32_t *TrailingOnes, uint32_t *TotalCoeff, int32_t nC)
{
    const static uint8_t TotCofNTrail5[21][3] = {
        {3, 4, 7}, {3, 4, 7}, {2, 4, 8}, {1, 4, 8}, {2, 3, 7}, {2, 3, 7}, {1, 3, 7},
        {1, 3, 7}, {0, 4, 6}, {0, 3, 6}, {0, 2, 6}, {3, 3, 6}, {1, 2, 6}, {0, 1, 6},
        {2, 2, 3}, {0, 0, 2}, {0, 0, 2}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}
    };

    uint32_t code;
    uint8_t *pcode;

    if (nC != -1) { //-1: ChromaArrayType=1 and -2:ChromaArrayType=2
        HL_DEBUG_ERROR("Not implemented");
        return HL_ERROR_NOT_IMPLEMENTED;
    }

    code = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 8);

    if (code >= 32) {
        pcode = (uint8_t*) & (TotCofNTrail5[(code>>5) + 13][0]);
    }
    else if (code >= 8) {
        pcode = (uint8_t*) & (TotCofNTrail5[(code>>2) + 6][0]);
    }
    else {
        pcode = (uint8_t*) & (TotCofNTrail5[code][0]);
    }

    *TrailingOnes = pcode[0];
    *TotalCoeff = pcode[1];

    hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[2]);

    return HL_ERROR_SUCCESS;
}

// 9.2.2.1 Parsing process for level_prefix
HL_ERROR_T hl_codec_264_cavlc_ReadLevelPrefix(const hl_codec_264_t* pc_codec, int32_t *levelPrefix)
{
    // (9-4)
    uint32_t tmp;
    int32_t leadingZeroBits;
    tmp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 16);

    leadingZeroBits = (int32_t)hl_bits_clz16(tmp);

    hl_codec_264_bits_discard(pc_codec->pobj_bits, leadingZeroBits + 1);
    *levelPrefix = leadingZeroBits;

    return HL_ERROR_SUCCESS;
}

// Table 9-7 – total_zeros tables for 4x4 blocks with tzVlcIndex 1 to 7
// Table 9-8 – total_zeros tables for 4x4 blocks with tzVlcIndex 8 to 15
HL_ERROR_T hl_codec_264_cavlc_ReadTotalZeros(const hl_codec_264_t* pc_codec, int32_t *TotalZeros, int32_t TotalCoeff)
{
    const static uint8_t TotZero1[28][2] = {{15, 9}, {14, 9}, {13, 9}, {12, 8},
        {12, 8}, {11, 8}, {11, 8}, {10, 7}, {9, 7}, {8, 6}, {8, 6}, {7, 6}, {7, 6}, {6, 5}, {6, 5},
        {6, 5}, {6, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {4, 4}, {3, 4},
        {2, 3}, {2, 3}, {1, 3}, {1, 3}, {0, 1}
    };

    const static uint8_t TotZero2n3[2][18][2] = {{{14, 6}, {13, 6}, {12, 6}, {11, 6},
            {10, 5}, {10, 5}, {9, 5}, {9, 5}, {8, 4}, {7, 4}, {6, 4}, {5, 4}, {4, 3}, {4, 3},
            {3, 3}, {2, 3}, {1, 3}, {0, 3}
        },

        /*const static uint8 TotZero3[18][2]=*/{{13, 6}, {11, 6}, {12, 5}, {12, 5}, {10, 5},
            {10, 5}, {9, 5}, {9, 5}, {8, 4}, {5, 4}, {4, 4}, {0, 4}, {7, 3}, {7, 3}, {6, 3}, {3, 3},
            {2, 3}, {1, 3}
        }
    };

    const static uint8_t TotZero4[17][2] = {{12, 5}, {11, 5}, {10, 5}, {0, 5}, {9, 4},
        {9, 4}, {7, 4}, {7, 4}, {3, 4}, {3, 4}, {2, 4}, {2, 4}, {8, 3}, {6, 3}, {5, 3}, {4, 3}, {1, 3}
    };

    const static uint8_t TotZero5[13][2] = {{11, 5}, {9, 5}, {10, 4}, {8, 4}, {2, 4},
        {1, 4}, {0, 4}, {7, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}
    };

    const static uint8_t TotZero6to10[5][15][2] = {{{10, 6}, {0, 6}, {1, 5}, {1, 5}, {8, 4},
            {8, 4}, {8, 4}, {8, 4}, {9, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 3}
        },

        /*const static uint8 TotZero7[15][2]=*/{{9, 6}, {0, 6}, {1, 5}, {1, 5}, {7, 4},
            {7, 4}, {7, 4}, {7, 4}, {8, 3}, {6, 3}, {4, 3}, {3, 3}, {2, 3}, {5, 2}, {5, 2}
        },

        /*const static uint8 TotZero8[15][2]=*/{{8, 6}, {0, 6}, {2, 5}, {2, 5}, {1, 4},
            {1, 4}, {1, 4}, {1, 4}, {7, 3}, {6, 3}, {3, 3}, {5, 2}, {5, 2}, {4, 2}, {4, 2}
        },

        /*const static uint8 TotZero9[15][2]=*/{{1, 6}, {0, 6}, {7, 5}, {7, 5}, {2, 4},
            {2, 4}, {2, 4}, {2, 4}, {5, 3}, {6, 2}, {6, 2}, {4, 2}, {4, 2}, {3, 2}, {3, 2}
        },

        /*const static uint8 TotZero10[11][2]=*/{{1, 5}, {0, 5}, {6, 4}, {6, 4}, {2, 3},
            {2, 3}, {2, 3}, {2, 3}, {5, 2}, {4, 2}, {3, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
        }
    };

    const static uint8_t TotZero11[7][2] = {{0, 4}, {1, 4}, {2, 3}, {2, 3}, {3, 3}, {5, 3}, {4, 1}};

    const static uint8_t TotZero12to15[4][5][2] = {
        {{3, 1}, {2, 2}, {4, 3}, {1, 4}, {0, 4}},
        {{2, 1}, {3, 2}, {1, 3}, {0, 3}, {0, 0}},
        {{2, 1}, {1, 2}, {0, 2}, {0, 0}, {0, 0}},
        {{1, 1}, {0, 1}, {0, 0}, {0, 0}, {0, 0}}
    };

    uint32_t temp, mask;
    int32_t indx;
    uint8_t *pcode;

    if (TotalCoeff == 1) {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 9);

        if (temp >= 256) {
            pcode = (uint8_t*) & (TotZero1[27][0]);
        }
        else if (temp >= 64) {
            pcode = (uint8_t*) & (TotZero1[(temp>>5)+19][0]);
        }
        else if (temp >= 8) {
            pcode = (uint8_t*) & (TotZero1[(temp>>2)+5][0]);
        }
        else {
            pcode = (uint8_t*) & (TotZero1[temp-1][0]);
        }
    }
    else if (TotalCoeff == 2 || TotalCoeff == 3) {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 6);

        if (temp >= 32) {
            pcode = (uint8_t*) & (TotZero2n3[TotalCoeff-2][(temp>>3)+10][0]);
        }
        else if (temp >= 8) {
            pcode = (uint8_t*) & (TotZero2n3[TotalCoeff-2][(temp>>2)+6][0]);
        }
        else {
            pcode = (uint8_t*) & (TotZero2n3[TotalCoeff-2][temp][0]);
        }
    }
    else if (TotalCoeff == 4) {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 5);

        if (temp >= 12) {
            pcode = (uint8_t*) & (TotZero4[(temp>>2)+9][0]);
        }
        else {
            pcode = (uint8_t*) & (TotZero4[temp][0]);
        }
    }
    else if (TotalCoeff == 5) {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 5);

        if (temp >= 16) {
            pcode = (uint8_t*) & (TotZero5[(temp>>2)+5][0]);
        }
        else if (temp >= 2) {
            pcode = (uint8_t*) & (TotZero5[(temp>>1)+1][0]);
        }
        else {
            pcode = (uint8_t*) & (TotZero5[temp][0]);
        }
    }
    else if (TotalCoeff >= 6 && TotalCoeff <= 10) {
        if (TotalCoeff == 10) {
            temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 5);
        }
        else {
            temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 6);
        }

        if (temp >= 8) {
            pcode = (uint8_t*) & (TotZero6to10[TotalCoeff-6][(temp>>3)+7][0]);
        }
        else {
            pcode = (uint8_t*) & (TotZero6to10[TotalCoeff-6][temp][0]);
        }
    }
    else if (TotalCoeff == 11) {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 4);

        if (temp >= 8) {
            pcode = (uint8_t*) & (TotZero11[6][0]);
        }
        else if (temp >= 4) {
            pcode = (uint8_t*) & (TotZero11[(temp>>1)+2][0]);
        }
        else {
            pcode = (uint8_t*) & (TotZero11[temp][0]);
        }
    }
    else {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, (16 - TotalCoeff));
        mask = (1 << (15 - TotalCoeff));
        indx = 0;
        while ((temp&mask) == 0 && indx < (16 - TotalCoeff)) { /* search location of 1 bit */
            mask >>= 1;
            indx++;
        }

        pcode = (uint8_t*) & (TotZero12to15[TotalCoeff-12][indx]);
    }

    *TotalZeros = pcode[0];
    hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[1]);

    return HL_ERROR_SUCCESS;
}

// Table 9-9 – total_zeros tables for chroma DC 2x2 and 2x4 blocks
// (a) Chroma DC 2x2 block (4:2:0 chroma sampling)
// FIXME: (b) Chroma DC 2x4 block (4:2:2 chroma sampling)
HL_ERROR_T hl_codec_264_cavlc_ReadTotalZerosChromaDC(const hl_codec_264_t* pc_codec, int32_t *TotalZeros, int32_t TotalCoeff)
{
    const static uint8_t TotZeroChrom1to3[3][8][2] = {
        {{3, 3}, {2, 3}, {1, 2}, {1, 2}, {0, 1}, {0, 1}, {0, 1}, {0, 1}},
        {{2, 2}, {2, 2}, {1, 2}, {1, 2}, {0, 1}, {0, 1}, {0, 1}, {0, 1}},
        {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}},
    };


    uint32_t temp;
    uint8_t *pcode;

    temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 3);
    pcode = (uint8_t*) & (TotZeroChrom1to3[TotalCoeff-1][temp]);

    *TotalZeros = pcode[0];

    hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[1]);

    return HL_ERROR_SUCCESS;
}

// Table 9-10 – Tables for run_before
HL_ERROR_T hl_codec_264_cavlc_ReadRunBefore(const hl_codec_264_t* pc_codec, int32_t *RunBefore, int32_t zerosLeft)
{
    const static int32_t codlen[6] = {1, 2, 2, 3, 3, 3}; /* num bits to read */
    const static uint8_t RunBeforeTab[6][8][2] = {{{1, 1}, {0, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        /*const static int RunBefore2[4][2]=*/{{2, 2}, {1, 2}, {0, 1}, {0, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        /*const static int RunBefore3[4][2]=*/{{3, 2}, {2, 2}, {1, 2}, {0, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        /*const static int RunBefore4[7][2]=*/{{4, 3}, {3, 3}, {2, 2}, {2, 2}, {1, 2}, {1, 2}, {0, 2}, {0, 2}},
        /*const static int RunBefore5[7][2]=*/{{5, 3}, {4, 3}, {3, 3}, {2, 3}, {1, 2}, {1, 2}, {0, 2}, {0, 2}},
        /*const static int RunBefore6[7][2]=*/{{1, 3}, {2, 3}, {4, 3}, {3, 3}, {6, 3}, {5, 3}, {0, 2}, {0, 2}}
    };

    uint32_t temp;
    uint8_t *pcode;
    int32_t indx;
    //printf("zerosLeft=%d ", zerosLeft);
    if (zerosLeft <= 6) {
        temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, codlen[zerosLeft-1]);

        pcode = (uint8_t*) & (RunBeforeTab[zerosLeft-1][temp][0]);

        *RunBefore = pcode[0];

        hl_codec_264_bits_discard(pc_codec->pobj_bits, pcode[1]);
    }
    else {
        temp = hl_codec_264_bits_read_u(pc_codec->pobj_bits, 3);
        if (temp) {
            *RunBefore = 7 - temp;
        }
        else {
            temp = hl_codec_264_bits_show_u(pc_codec->pobj_bits, 9);
            temp <<= 7;
            indx = (int32_t)hl_bits_clz16(temp);

            *RunBefore = 7 + indx;
            hl_codec_264_bits_discard(pc_codec->pobj_bits, indx + 1);
        }
    }

    return HL_ERROR_SUCCESS;
}


HL_ERROR_T hl_codec_264_cavlc_WriteTotalCoeffTrailingOnes(hl_codec_264_bits_t *bits, int32_t TrailingOnes, int32_t TotalCoeff, int32_t nC)
{
    const static uint8_t totCoeffTrailOne[3][4][17][2] = {
        {
            // 0702
            {{1, 1}, {6, 5}, {8, 7}, {9, 7}, {10, 7}, {11, 7}, {13, 15}, {13, 11}, {13, 8}, {14, 15}, {14, 11}, {15, 15}, {15, 11}, {16, 15}, {16, 11}, {16, 7}, {16, 4}},
            {{0, 0}, {2, 1}, {6, 4}, {8, 6}, {9, 6}, {10, 6}, {11, 6}, {13, 14}, {13, 10}, {14, 14}, {14, 10}, {15, 14}, {15, 10}, {15, 1}, {16, 14}, {16, 10}, {16, 6}},
            {{0, 0}, {0, 0}, {3, 1}, {7, 5}, {8, 5}, {9, 5}, {10, 5}, {11, 5}, {13, 13}, {13, 9}, {14, 13}, {14, 9}, {15, 13}, {15, 9}, {16, 13}, {16, 9}, {16, 5}},
            {{0, 0}, {0, 0}, {0, 0}, {5, 3}, {6, 3}, {7, 4}, {8, 4}, {9, 4}, {10, 4}, {11, 4}, {13, 12}, {14, 12}, {14, 8}, {15, 12}, {15, 8}, {16, 12}, {16, 8}},
        },
        {
            {{2, 3}, {6, 11}, {6, 7}, {7, 7}, {8, 7}, {8, 4}, {9, 7}, {11, 15}, {11, 11}, {12, 15}, {12, 11}, {12, 8}, {13, 15}, {13, 11}, {13, 7}, {14, 9}, {14, 7}},
            {{0, 0}, {2, 2}, {5, 7}, {6, 10}, {6, 6}, {7, 6}, {8, 6}, {9, 6}, {11, 14}, {11, 10}, {12, 14}, {12, 10}, {13, 14}, {13, 10}, {14, 11}, {14, 8}, {14, 6}},
            {{0, 0}, {0, 0}, {3, 3}, {6, 9}, {6, 5}, {7, 5}, {8, 5}, {9, 5}, {11, 13}, {11, 9}, {12, 13}, {12, 9}, {13, 13}, {13, 9}, {13, 6}, {14, 10}, {14, 5}},
            {{0, 0}, {0, 0}, {0, 0}, {4, 5}, {4, 4}, {5, 6}, {6, 8}, {6, 4}, {7, 4}, {9, 4}, {11, 12}, {11, 8}, {12, 12}, {13, 12}, {13, 8}, {13, 1}, {14, 4}},
        },
        {
            {{4, 15}, {6, 15}, {6, 11}, {6, 8}, {7, 15}, {7, 11}, {7, 9}, {7, 8}, {8, 15}, {8, 11}, {9, 15}, {9, 11}, {9, 8}, {10, 13}, {10, 9}, {10, 5}, {10, 1}},
            {{0, 0}, {4, 14}, {5, 15}, {5, 12}, {5, 10}, {5, 8}, {6, 14}, {6, 10}, {7, 14}, {8, 14}, {8, 10}, {9, 14}, {9, 10}, {9, 7}, {10, 12}, {10, 8}, {10, 4}},
            {{0, 0}, {0, 0}, {4, 13}, {5, 14}, {5, 11}, {5, 9}, {6, 13}, {6, 9}, {7, 13}, {7, 10}, {8, 13}, {8, 9}, {9, 13}, {9, 9}, {10, 11}, {10, 7}, {10, 3}},
            {{0, 0}, {0, 0}, {0, 0}, {4, 12}, {4, 11}, {4, 10}, {4, 9}, {4, 8}, {5, 13}, {6, 12}, {7, 12}, {8, 12}, {8, 8}, {9, 12}, {10, 10}, {10, 6}, {10, 2}}
        }
    };


    uint32_t code, len;
    int32_t vlcnum;

    if (nC >= 8) {
        if (TotalCoeff) {
            code = ((TotalCoeff - 1) << 2) | (TrailingOnes);
        }
        else {
            code = 3;
        }
        hl_codec_264_bits_write_u(bits, code, 6);
    }
    else {
        if (nC < 2) {
            vlcnum = 0;
        }
        else if (nC < 4) {
            vlcnum = 1;
        }
        else {
            vlcnum = 2;
        }

        len = totCoeffTrailOne[vlcnum][TrailingOnes][TotalCoeff][0];
        code = totCoeffTrailOne[vlcnum][TrailingOnes][TotalCoeff][1];
        hl_codec_264_bits_write_u(bits, code, len);
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_codec_264_cavlc_WriteTotalCoeffTrailingOnesChromaDC(hl_codec_264_bits_t *bits, int32_t TrailingOnes, int32_t TotalCoeff)
{
    const static uint8_t totCoeffTrailOneChrom[4][5][2] = {
        { {2, 1}, {6, 7}, {6, 4}, {6, 3}, {6, 2}},
        { {0, 0}, {1, 1}, {6, 6}, {7, 3}, {8, 3}},
        { {0, 0}, {0, 0}, {3, 1}, {7, 2}, {8, 2}},
        { {0, 0}, {0, 0}, {0, 0}, {6, 5}, {7, 0}},
    };

    uint32_t code, len;

    len = totCoeffTrailOneChrom[TrailingOnes][TotalCoeff][0];
    code = totCoeffTrailOneChrom[TrailingOnes][TotalCoeff][1];
    hl_codec_264_bits_write_u(bits, code, len);

    return HL_ERROR_SUCCESS;
}

/* see Table 9-7 and 9-8 */
HL_ERROR_T hl_codec_264_cavlc_WriteTotalZeros(hl_codec_264_bits_t *bits, int32_t total_zeros, int32_t TotalCoeff)
{
    const static uint8_t lenTotalZeros[15][16] = {
        { 1, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9},
        { 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 6, 6},
        { 4, 3, 3, 3, 4, 4, 3, 3, 4, 5, 5, 6, 5, 6},
        { 5, 3, 4, 4, 3, 3, 3, 4, 3, 4, 5, 5, 5},
        { 4, 4, 4, 3, 3, 3, 3, 3, 4, 5, 4, 5},
        { 6, 5, 3, 3, 3, 3, 3, 3, 4, 3, 6},
        { 6, 5, 3, 3, 3, 2, 3, 4, 3, 6},
        { 6, 4, 5, 3, 2, 2, 3, 3, 6},
        { 6, 6, 4, 2, 2, 3, 2, 5},
        { 5, 5, 3, 2, 2, 2, 4},
        { 4, 4, 3, 3, 1, 3},
        { 4, 4, 2, 1, 3},
        { 3, 3, 1, 2},
        { 2, 2, 1},
        { 1, 1},
    };

    const static uint8_t codTotalZeros[15][16] = {
        {1, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1},
        {7, 6, 5, 4, 3, 5, 4, 3, 2, 3, 2, 3, 2, 1, 0},
        {5, 7, 6, 5, 4, 3, 4, 3, 2, 3, 2, 1, 1, 0},
        {3, 7, 5, 4, 6, 5, 4, 3, 3, 2, 2, 1, 0},
        {5, 4, 3, 7, 6, 5, 4, 3, 2, 1, 1, 0},
        {1, 1, 7, 6, 5, 4, 3, 2, 1, 1, 0},
        {1, 1, 5, 4, 3, 3, 2, 1, 1, 0},
        {1, 1, 1, 3, 3, 2, 2, 1, 0},
        {1, 0, 1, 3, 2, 1, 1, 1, },
        {1, 0, 1, 3, 2, 1, 1, },
        {0, 1, 1, 2, 1, 3},
        {0, 1, 1, 1, 1},
        {0, 1, 1, 1},
        {0, 1, 1},
        {0, 1},
    };
    uint32_t len, code;

    len = lenTotalZeros[TotalCoeff-1][total_zeros];
    code = codTotalZeros[TotalCoeff-1][total_zeros];

    hl_codec_264_bits_write_u(bits, code, len);

    return HL_ERROR_SUCCESS;
}

/* see Table 9-9 */
HL_ERROR_T hl_codec_264_cavlc_WriteTotalZerosChromaDC(hl_codec_264_bits_t *bits, int total_zeros, int TotalCoeff)
{
    const static uint8_t lenTotalZerosChromaDC[3][4] = {
        { 1, 2, 3, 3, },
        { 1, 2, 2, 0, },
        { 1, 1, 0, 0, },
    };

    const static uint8_t codTotalZerosChromaDC[3][4] = {
        { 1, 1, 1, 0, },
        { 1, 1, 0, 0, },
        { 1, 0, 0, 0, },
    };

    uint32_t len, code;

    len = lenTotalZerosChromaDC[TotalCoeff-1][total_zeros];
    code = codTotalZerosChromaDC[TotalCoeff-1][total_zeros];

    hl_codec_264_bits_write_u(bits, code, len);

    return HL_ERROR_SUCCESS;
}

/* see Table 9-10 */
HL_ERROR_T hl_codec_264_cavlc_WriteRunBefore(hl_codec_264_bits_t *bits, int32_t run_before, int32_t zerosLeft)
{
    const static uint8_t lenRunBefore[7][16] = {
        {1, 1},
        {1, 2, 2},
        {2, 2, 2, 2},
        {2, 2, 2, 3, 3},
        {2, 2, 3, 3, 3, 3},
        {2, 3, 3, 3, 3, 3, 3},
        {3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    };

    const static uint8_t codRunBefore[7][16] = {
        {1, 0},
        {1, 1, 0},
        {3, 2, 1, 0},
        {3, 2, 1, 1, 0},
        {3, 2, 3, 2, 1, 0},
        {3, 0, 1, 3, 2, 5, 4},
        {7, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    uint32_t len, code;

    if (zerosLeft <= 6) {
        len = lenRunBefore[zerosLeft-1][run_before];
        code = codRunBefore[zerosLeft-1][run_before];
    }
    else {
        len = lenRunBefore[6][run_before];
        code = codRunBefore[6][run_before];
    }

    hl_codec_264_bits_write_u(bits, code, len);


    return HL_ERROR_SUCCESS;
}