#include "hartallo/h264/hl_codec_264_fmo.h"
#include "hartallo/h264/hl_codec_264_slice.h"
#include "hartallo/h264/hl_codec_264_pps.h"
#include "hartallo/h264/hl_codec_264_sps.h"
#include "hartallo/h264/hl_codec_264_layer.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

// 8.2.2 Decoding process for macroblock to slice group map
// Inputs to this process are the active picture parameter set and the slice header of the slice to be decoded.
// Output of this process is a macroblock to slice group map MbToSliceGroupMap.
// This process is invoked at the start of every slice.
// NOTE – The output of this process is equal for all slices of a picture.
HL_ERROR_T hl_codec_264_fmo_read_mb_to_slice_group_map(hl_codec_264_t* p_codec)
{
    if (p_codec->pps.pc_active->num_slice_groups_minus1 == 0) {
        // If num_slice_groups_minus1 is equal to 0, the map unit to slice group map is generated for all i ranging from 0 to
        // PicSizeInMapUnits - 1, inclusive, as specified by
        // mapUnitToSliceGroupMap[ i ] = 0
        // => we don't need to execute this code when num_slice_groups_minus1==0 as MbToSliceGroupMap has been allocated using calloc()
    }
    else {
        int32_t* mapUnitToSliceGroupMap; // FIXME: move "mapUnitToSliceGroupMap" into p_codec to avoid too frequent alloc()
        if (!(mapUnitToSliceGroupMap = hl_memory_calloc(p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMbs, sizeof(int32_t)))) {
            HL_DEBUG_ERROR("Failed to allocate memory");
            return HL_ERROR_OUTOFMEMMORY;
        }
        switch (p_codec->pps.pc_active->slice_group_map_type) {
        case 0: { // 8.2.2.1 Specification for interleaved slice group map type
            // (8-17)
            uint32_t i = 0, iGroup, j;
            do
                for(iGroup = 0;
                        iGroup <= p_codec->pps.pc_active->num_slice_groups_minus1 && i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits;
                        i += p_codec->pps.pc_active->run_length_minus1[iGroup++] + 1) {
                    for(j = 0;
                            j <= p_codec->pps.pc_active->run_length_minus1[iGroup] && i + j < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits;
                            j++) {
                        mapUnitToSliceGroupMap[ i + j ] = iGroup;
                    }
                }
            while(i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits);

            break;
        };

        case 1: { // 8.2.2.2 Specification for dispersed slice group map type
            // (8-18)
            uint32_t i;
            for(i = 0;
                    i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits;
                    i++) {
                mapUnitToSliceGroupMap[ i ] = ( ( i % p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs ) +
                                                ( ( ( i / p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs ) * ( p_codec->pps.pc_active->num_slice_groups_minus1 + 1 ) ) / 2 ) )
                                              % ( p_codec->pps.pc_active->num_slice_groups_minus1 + 1);
            }

            break;
        };

        case 2: { // 8.2.2.3 Specification for foreground with left-over slice group map type
            // (8-19)
            int32_t yTopLeft, xTopLeft, yBottomRight, xBottomRight, y, x,iGroup;
            uint32_t i;
            for(i = 0; i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits; i++) {
                mapUnitToSliceGroupMap[ i ] = p_codec->pps.pc_active->num_slice_groups_minus1;
            }

            for(iGroup = p_codec->pps.pc_active->num_slice_groups_minus1 - 1; iGroup >= 0; iGroup--) {
                yTopLeft = p_codec->pps.pc_active->top_left[ iGroup ] / p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs;
                xTopLeft = p_codec->pps.pc_active->top_left[ iGroup ] % p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs;
                yBottomRight = p_codec->pps.pc_active->bottom_right[ iGroup ] / p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs;
                xBottomRight = p_codec->pps.pc_active->bottom_right[ iGroup ] % p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs;
                for(y = yTopLeft; y <= yBottomRight; y++) {
                    for(x = xTopLeft; x <= xBottomRight; x++) {
                        mapUnitToSliceGroupMap[ y * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs + x ] = iGroup;
                    }
                }
            }

            break;
        };

        case 3: { // 8.2.2.4 Specification for box-out slice group map types
            // (8-20)
            int32_t i, x, y, k, xDir, yDir;
            int32_t leftBound, bottomBound, rightBound, topBound;
            int32_t mapUnitVacant;
            for (i = 0; i < (int32_t)p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits; i++) {
                mapUnitToSliceGroupMap[ i ] = 1;
            }
            x = ( p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs - p_codec->pps.pc_active->slice_group_change_direction_flag ) / 2;
            y = ( p_codec->layers.pc_active->pc_slice_hdr->PicHeightInMapUnits - p_codec->pps.pc_active->slice_group_change_direction_flag ) / 2;
            leftBound = x, topBound = y;
            rightBound = x, bottomBound = y;
            xDir = p_codec->pps.pc_active->slice_group_change_direction_flag - 1, yDir = p_codec->pps.pc_active->slice_group_change_direction_flag;
            for (k = 0; k < p_codec->layers.pc_active->pc_slice_hdr->MapUnitsInSliceGroup0; k += mapUnitVacant) {
                mapUnitVacant = ( mapUnitToSliceGroupMap[ y * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs + x ] == 1 );
                if (mapUnitVacant) {
                    mapUnitToSliceGroupMap[ y * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs + x ] = 0;
                }
                if (xDir == -1 && x == leftBound) {
                    leftBound = HL_MATH_MAX(leftBound - 1, 0);
                    x = leftBound;
                    xDir = 0, yDir = (p_codec->pps.pc_active->slice_group_change_direction_flag << 1) - 1;
                }
                else if (xDir == 1 && x == rightBound) {
                    rightBound = HL_MATH_MIN( rightBound + 1, (int32_t)p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs - 1 );
                    x = rightBound;
                    xDir = 0, yDir = 1 - (p_codec->pps.pc_active->slice_group_change_direction_flag << 1);
                }
                else if (yDir == -1 && y == topBound) {
                    topBound = HL_MATH_MAX( topBound - 1, 0 );
                    y = topBound;
                    xDir = 1 - 2 * p_codec->pps.pc_active->slice_group_change_direction_flag, yDir = 0;
                }
                else if (yDir == 1 && y == bottomBound) {
                    bottomBound = HL_MATH_MIN( bottomBound + 1, (int32_t)p_codec->layers.pc_active->pc_slice_hdr->PicHeightInMapUnits - 1 );
                    y = bottomBound;
                    xDir = (p_codec->pps.pc_active->slice_group_change_direction_flag << 1) - 1, yDir = 0;
                }
                else {
                    x = x + xDir, y = y + yDir;
                }
            }

            break;
        };

        case 4: { // 8.2.2.5 Specification for raster scan slice group map types
            uint32_t i;
            // (8-14)
            int32_t sizeOfUpperLeftGroup = (p_codec->pps.pc_active->slice_group_change_direction_flag ?
                                            (p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits - p_codec->layers.pc_active->pc_slice_hdr->MapUnitsInSliceGroup0) : p_codec->layers.pc_active->pc_slice_hdr->MapUnitsInSliceGroup0);

            // (8-21)
            for (i = 0; i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits; i++)
                if ((int32_t)i < sizeOfUpperLeftGroup) {
                    mapUnitToSliceGroupMap[ i ] = p_codec->pps.pc_active->slice_group_change_direction_flag;
                }
                else {
                    mapUnitToSliceGroupMap[ i ] = 1 - p_codec->pps.pc_active->slice_group_change_direction_flag;
                }

            break;
        };

        case 5: {
            // 8.2.2.6 Specification for wipe slice group map types
            int32_t k = 0;
            uint32_t i, j;
            // (8-14)
            int32_t sizeOfUpperLeftGroup = (p_codec->pps.pc_active->slice_group_change_direction_flag ?
                                            (p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits - p_codec->layers.pc_active->pc_slice_hdr->MapUnitsInSliceGroup0) : p_codec->layers.pc_active->pc_slice_hdr->MapUnitsInSliceGroup0);
            // (8-22)
            for (j = 0; j < p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs; j++)
                for (i = 0; i < p_codec->layers.pc_active->pc_slice_hdr->PicHeightInMapUnits; i++)
                    if (k++ < sizeOfUpperLeftGroup) {
                        mapUnitToSliceGroupMap[ i * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs + j ] = p_codec->pps.pc_active->slice_group_change_direction_flag;
                    }
                    else {
                        mapUnitToSliceGroupMap[ i * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs + j ] = 1 - p_codec->pps.pc_active->slice_group_change_direction_flag;
                    }
            break;
        };

        case 6: { // 8.2.2.7 Specification for explicit slice group map type
            // mapUnitToSliceGroupMap[ i ] = slice_group_id[ i ] (8-23)
            // for all i ranging from 0 to PicSizeInMapUnits - 1, inclusive
            uint32_t i;
            for (i = 0; i<p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMapUnits; i++) {
                mapUnitToSliceGroupMap[ i ] = p_codec->pps.pc_active->slice_group_id[i];
            }
            break;
        };

        default: {
            HL_SAFE_FREE(mapUnitToSliceGroupMap);
            HL_DEBUG_ERROR("%d not valid for 'slice_group_map_typ'", p_codec->pps.pc_active->slice_group_map_type);
            return HL_ERROR_INVALID_BITSTREAM;
        }
        }//end of switch

        // 8.2.2.8 Specification for conversion of map unit to slice group map to macroblock to slice group map
        if (p_codec->sps.pc_active->frame_mbs_only_flag || p_codec->layers.pc_active->pc_slice_hdr->field_pic_flag) {
            uint32_t i;
            for (i = 0; i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMbs; i++) {
                p_codec->layers.pc_active->pc_slice_hdr->MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i];
            }
        }
        else if (p_codec->layers.pc_active->pc_slice_hdr->MbaffFrameFlag) {
            uint32_t i;
            for (i = 0; i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMbs; i++) {
                p_codec->layers.pc_active->pc_slice_hdr->MbToSliceGroupMap[i] = mapUnitToSliceGroupMap[i>>1];
            }
        }
        else {
            uint32_t i;
            for (i = 0; i < p_codec->layers.pc_active->pc_slice_hdr->PicSizeInMbs; i++) {
                p_codec->layers.pc_active->pc_slice_hdr->MbToSliceGroupMap[ i ] = mapUnitToSliceGroupMap[(i / (p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs << 1)) * p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs + ( i % p_codec->layers.pc_active->pc_slice_hdr->PicWidthInMbs ) ];
            }
        }

        HL_SAFE_FREE(mapUnitToSliceGroupMap);
    }// end of else

    return HL_ERROR_SUCCESS;
}