#include "hartallo/h264/hl_codec_264_interpol.h"
#include "hartallo/h264/hl_codec_264_tables.h"
#include "hartallo/h264/hl_codec_264.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_debug.h"

#if HL_HAVE_X86_INTRIN
#include "hartallo/h264/intrinsics/x86/hl_codec_x86_264_interpol_intrin.h"
#endif /* HL_HAVE_X86_INTRIN */

/*** Interpolation (Half/Quater-Pel) functions in header file ***/

struct hl_codec_264_s;
struct hl_codec_264_pict_s;
struct hl_codec_264_mb_s;
struct hl_codec_264_mv_xs;

extern const hl_object_def_t *hl_codec_264_interpol_indices_def_t;
extern HL_ERROR_T hl_codec_264_interpol_chroma_cpp(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t mbPartIdx, int32_t subMbPartIdx, const struct hl_codec_264_mv_xs* mvCLX, const struct hl_codec_264_mv_xs* mvLX, const struct hl_codec_264_pict_s* refPicLXCb, const struct hl_codec_264_pict_s* refPicLXCr, HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16], HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);
extern void hl_codec_264_interpol_chroma_cat1_u8_cpp(struct hl_codec_264_s* p_codec, const hl_pixel_t* cSCb_u8, const hl_pixel_t* cSCr_u8, int32_t xFracC, int32_t yFracC, int32_t _xIntC, int32_t _yIntC, int32_t partWidthC, int32_t partHeightC, int32_t i_pic_height, int32_t i_pic_width, HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16], HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]);

void (*hl_codec_264_interpol_chroma_cat1_u8)(struct hl_codec_264_s* p_codec, const hl_pixel_t* cSCb_u8, const hl_pixel_t* cSCr_u8, int32_t xFracC, int32_t yFracC, int32_t _xIntC, int32_t _yIntC, int32_t partWidthC, int32_t partHeightC, int32_t i_pic_height, int32_t i_pic_width, HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16], HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]) = hl_codec_264_interpol_chroma_cat1_u8_cpp;
void (*hl_codec_264_interpol_load_samples4x4_u8)(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/) = hl_codec_264_interpol_load_samples4x4_u8_cpp;
void (*hl_codec_264_interpol_load_samples4x4_chroma_u8)(const hl_pixel_t* cSCb_u8, HL_ALIGNED(16) uint8_t retCb_u8[4/*A=0,B=1,C=2,D=3*/][16], const hl_pixel_t* cSCr_u8, HL_ALIGNED(16) uint8_t retCr_u8[4/*A=0,B=1,C=2,D=3*/][16], int32_t i_x0, int32_t i_y0, int32_t i_height, int32_t i_width) = hl_codec_264_interpol_load_samples4x4_chroma_u8_cpp;

void (*hl_codec_264_interpol_luma00_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL) = hl_codec_264_interpol_luma00_horiz4_cpp;
void (*hl_codec_264_interpol_luma01_vert4)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma01_vert4_cpp;
void (*hl_codec_264_interpol_luma02_vert4)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL,HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma02_vert4_cpp;
void (*hl_codec_264_interpol_luma03_vert4)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma03_vert4_cpp;
void (*hl_codec_264_interpol_luma10_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma10_horiz4_cpp;
void (*hl_codec_264_interpol_luma11_diag4)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma11_diag4_cpp;
void (*hl_codec_264_interpol_luma12_vert4)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma12_vert4_cpp;
void (*hl_codec_264_interpol_luma13_diag4)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma13_diag4_cpp;
void (*hl_codec_264_interpol_luma20_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma20_horiz4_cpp;
void (*hl_codec_264_interpol_luma21_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma21_diag4_cpp;
void (*hl_codec_264_interpol_luma22_vert4)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma22_vert4_cpp;
void (*hl_codec_264_interpol_luma23_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma23_diag4_cpp;
void (*hl_codec_264_interpol_luma30_horiz4)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma30_horiz4_cpp;
void (*hl_codec_264_interpol_luma31_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma31_diag4_cpp;
void (*hl_codec_264_interpol_luma32_vert4)(const uint32_t* pc_indices_vert[7], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma32_vert4_cpp;
void (*hl_codec_264_interpol_luma33_diag4)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]) = hl_codec_264_interpol_luma33_diag4_cpp;

void (*hl_codec_264_interpol_luma00_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma00_horiz4x4_u8_cpp;
void (*hl_codec_264_interpol_luma01_vert4x4_u8)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma01_vert4x4_u8_cpp;
void (*hl_codec_264_interpol_luma02_vert4x4_u8)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma02_vert4x4_u8_cpp;
void (*hl_codec_264_interpol_luma03_vert4x4_u8)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma03_vert4x4_u8_cpp;
void (*hl_codec_264_interpol_luma10_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma10_horiz4x4_u8_cpp;
void (*hl_codec_264_interpol_luma11_diag4x4_u8)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma11_diag4x4_u8_cpp;
void (*hl_codec_264_interpol_luma12_vert4x4_u8)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma12_vert4x4_u8_cpp;
void (*hl_codec_264_interpol_luma13_diag4x4_u8)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma13_diag4x4_u8_cpp;
void (*hl_codec_264_interpol_luma20_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma20_horiz4x4_u8_cpp;
void (*hl_codec_264_interpol_luma21_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma21_diag4x4_u8_cpp;
void (*hl_codec_264_interpol_luma22_vert4x4_u8)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma22_vert4x4_u8_cpp;
void (*hl_codec_264_interpol_luma23_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma23_diag4x4_u8_cpp;
void (*hl_codec_264_interpol_luma30_horiz4x4_u8)(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma30_horiz4x4_u8_cpp;
void (*hl_codec_264_interpol_luma31_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma31_diag4x4_u8_cpp;
void (*hl_codec_264_interpol_luma32_vert4x4_u8)(const uint32_t* pc_indices_vert[7], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma32_vert4x4_u8_cpp;
void (*hl_codec_264_interpol_luma33_diag4x4_u8)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x1) = hl_codec_264_interpol_luma33_diag4x4_u8_cpp;

HL_ERROR_T (*hl_codec_264_interpol_chroma)(struct hl_codec_264_s* p_codec, struct hl_codec_264_mb_s* p_mb, int32_t mbPartIdx, int32_t subMbPartIdx, const struct hl_codec_264_mv_xs* mvCLX, const struct hl_codec_264_mv_xs* mvLX, const struct hl_codec_264_pict_s* refPicLXCb, const struct hl_codec_264_pict_s* refPicLXCr, HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16], HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16]) = hl_codec_264_interpol_chroma_cpp;

#undef IDX_A
#undef IDX_B
#undef IDX_C
#undef IDX_D
#define IDX_A 0
#define IDX_B 1
#define IDX_C 2
#define IDX_D 3

HL_ERROR_T hl_codec_264_interpol_indices_create(hl_codec_264_interpol_indices_t** pp_obj, uint32_t u_width, uint32_t u_height, hl_bool_t b_mbaff)
{
    int32_t i_width_padded, i_height_padded, i_indice, i_width_minus1 = u_width -1, i_height_minus1 = u_height - 1;
    int32_t x, y, i_width_plus_pad, i_height_plus_pad;
    int32_t __max_pad_size = 32; // must be at least MaxMbWidth/Height + 1 and 32 is perfect for AVX.
    uint32_t *p_indices;
    if (!pp_obj || !u_width || !u_height) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    if (b_mbaff) {
        u_height >>= 1;
    }

    i_width_padded = u_width + (__max_pad_size << 1);
    i_width_padded += (32 - (i_width_padded & 31)); // make multiple of 32
    __max_pad_size = (i_width_padded - u_width) >> 1;
    i_height_padded = u_height + (__max_pad_size << 1);

    i_width_plus_pad = u_width + __max_pad_size;
    i_height_plus_pad = u_height + __max_pad_size;
    p_indices = hl_memory_malloc(i_width_padded * i_height_padded * sizeof(int32_t));
    if (!p_indices) {
        return HL_ERROR_OUTOFMEMMORY;
    }

    *pp_obj = hl_object_create(hl_codec_264_interpol_indices_def_t);
    if (!*pp_obj) {
        HL_SAFE_FREE(p_indices);
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_obj)->p_indices_unpadded = p_indices;
#if 1
#define L_INTERPOL_SAMPLE_XZL(xDZL) HL_MATH_CLIP3(0,i_width_minus1,(xDZL))
#define L_INTERPOL_SAMPLE_YZL(yDZL) HL_MATH_CLIP3(0,i_height_minus1,(yDZL))
#define L_INTERPOL_SAMPLE_INDICE(xDZL,yDZL) L_INTERPOL_SAMPLE_XZL((xDZL))+(L_INTERPOL_SAMPLE_YZL((yDZL))*u_width)
    for (y = -__max_pad_size, i_indice = 0; y < i_height_plus_pad; ++y) {
        for (x = -__max_pad_size; x < i_width_plus_pad; x += 16) {
            p_indices[0] = L_INTERPOL_SAMPLE_INDICE(x, y);
            p_indices[1] = L_INTERPOL_SAMPLE_INDICE(x+1, y);
            p_indices[2] = L_INTERPOL_SAMPLE_INDICE(x+2, y);
            p_indices[3] = L_INTERPOL_SAMPLE_INDICE(x+3, y);
            p_indices[4] = L_INTERPOL_SAMPLE_INDICE(x+4, y);
            p_indices[5] = L_INTERPOL_SAMPLE_INDICE(x+5, y);
            p_indices[6] = L_INTERPOL_SAMPLE_INDICE(x+6, y);
            p_indices[7] = L_INTERPOL_SAMPLE_INDICE(x+7, y);
            p_indices[8] = L_INTERPOL_SAMPLE_INDICE(x+8, y);
            p_indices[9] = L_INTERPOL_SAMPLE_INDICE(x+9, y);
            p_indices[10] = L_INTERPOL_SAMPLE_INDICE(x+10, y);
            p_indices[11] = L_INTERPOL_SAMPLE_INDICE(x+11, y);
            p_indices[12] = L_INTERPOL_SAMPLE_INDICE(x+12, y);
            p_indices[13] = L_INTERPOL_SAMPLE_INDICE(x+13, y);
            p_indices[14] = L_INTERPOL_SAMPLE_INDICE(x+14, y);
            p_indices[15] = L_INTERPOL_SAMPLE_INDICE(x+15, y);
            p_indices += 16;
        }
    }
#else
    // left top square
    p_indices = (*pp_obj)->p_indices_unpadded;
    i_indice = 0;
    for (y = 0; y < __max_pad_size + 1; ++y) {
        for (x = 0; x < __max_pad_size; ++x) {
            p_indices[x] = 0;
        }
        p_indices += i_width_padded;
    }
    // right top square
    p_indices = ((*pp_obj)->p_indices_unpadded + i_width_padded - __max_pad_size - 1);
    i_indice = u_width - 1;
    for (y = 0; y < __max_pad_size + 1; ++y) {
        for (x = 0; x < __max_pad_size; ++x) {
            p_indices[x] = i_indice;
        }
        p_indices += i_width_padded;
    }
    // left bottom square
    p_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * (u_height + __max_pad_size - 1)));
    i_indice = (u_width * (u_height - 2));
    for (y = 0; y < __max_pad_size; ++y) {
        for (x = 0; x < __max_pad_size; ++x) {
            p_indices[x] = i_indice;
        }
        p_indices += i_width_padded;
    }
    // right bottom square
    p_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * (u_height + __max_pad_size - 1)) + u_width + i_width_padded);
    i_indice = (u_width * u_height) - 1;
    for (y = 0; y < __max_pad_size; ++y) {
        for (x = 0; x < __max_pad_size; ++x) {
            p_indices[x] = i_indice;
        }
        p_indices += i_width_padded;
    }
    // top band
    p_indices = ((*pp_obj)->p_indices_unpadded + __max_pad_size);
    i_indice = 0;
    for (y = 0; y < __max_pad_size + 1; ++y) {
        for (x = 0; x < u_width; ++x) {
            p_indices[x] = x;
        }
        p_indices += i_width_padded;
    }
    // left bands
    p_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * __max_pad_size));
    i_indice = 0;
    for (y = 0; y < u_height + 1; ++y) {
        for (x = 0; x < __max_pad_size; ++x) {
            p_indices[x] = i_indice;
        }
        p_indices += i_width_padded;
        i_indice += u_width;
    }
    // right band
    p_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * __max_pad_size) + __max_pad_size + u_width);
    i_indice = u_width - 1;
    for (y = 0; y < u_height + 1; ++y) {
        for (x = 0; x < __max_pad_size; ++x) {
            p_indices[x] = i_indice;
        }
        p_indices += i_width_padded;
        i_indice += u_width;
    }
    // bottom band
    p_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * (u_height + __max_pad_size - 1)) + __max_pad_size);
    i_indice = (u_width * (u_height - 1));
    for (y = 0; y < __max_pad_size + 1; ++y) {
        for (x = 0; x < u_width; ++x) {
            p_indices[x] = i_indice + x;
        }
        p_indices += i_width_padded;
    }

    // center rectangle
    p_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * __max_pad_size) + __max_pad_size);
    i_indice = 0;
    for (y = 0; y < u_height; ++y) {
        for (x = 0; x < u_width; ++x) {
            p_indices[x] = i_indice++;
        }
        p_indices += i_width_padded;
    }
#endif
    // skip pads
    (*pp_obj)->pc_indices = ((*pp_obj)->p_indices_unpadded + (i_width_padded * __max_pad_size) + __max_pad_size);
    (*pp_obj)->u_stride = i_width_padded;
    (*pp_obj)->u_width = u_width;
    (*pp_obj)->u_height = u_height;

    return HL_ERROR_SUCCESS;
}

void hl_codec_264_interpol_load_samples4x4_u8_cpp(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/)
{
    if ((pc_indices[3] - pc_indices[0]) == 3) {
        *((uint32_t*)&ret_u8[0]) = *((uint32_t*)&cSL_u8[pc_indices[0]]);
        pc_indices+=i_indices_stride;
        *((uint32_t*)&ret_u8[4]) = *((uint32_t*)&cSL_u8[pc_indices[0]]);
        pc_indices+=i_indices_stride;
        *((uint32_t*)&ret_u8[8]) = *((uint32_t*)&cSL_u8[pc_indices[0]]);
        pc_indices+=i_indices_stride;
        *((uint32_t*)&ret_u8[12]) = *((uint32_t*)&cSL_u8[pc_indices[0]]);
    }
    else {
        *((uint32_t*)&ret_u8[0]) = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        pc_indices+=i_indices_stride;
        *((uint32_t*)&ret_u8[4]) = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        pc_indices+=i_indices_stride;
        *((uint32_t*)&ret_u8[8]) = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
        pc_indices+=i_indices_stride;
        *((uint32_t*)&ret_u8[12]) = cSL_u8[pc_indices[0]] + (cSL_u8[pc_indices[1]] << 8) + (cSL_u8[pc_indices[2]] << 16) + (cSL_u8[pc_indices[3]] << 24);
    }
}

// TODO: add INTRIN and ASM versions
void hl_codec_264_interpol_load_samples4x4_chroma_u8_cpp(
    const hl_pixel_t* cSCb_u8, HL_ALIGNED(16) uint8_t retCb_u8[4/*A=0,B=1,C=2,D=3*/][16],
    const hl_pixel_t* cSCr_u8, HL_ALIGNED(16) uint8_t retCr_u8[4/*A=0,B=1,C=2,D=3*/][16],
    int32_t i_x0, int32_t i_y0, int32_t i_height, int32_t i_width)
{
#define SET_PIXEL(Pos, Col) \
	retCb_u8[IDX_A][(Pos)] = _cSCb_u8[(Col)],					retCr_u8[IDX_A][(Pos)] = _cSCr_u8[(Col)]; \
	retCb_u8[IDX_B][(Pos)] = _cSCb_u8[1 + (Col)],				retCr_u8[IDX_B][(Pos)] = _cSCr_u8[1 + (Col)]; \
	retCb_u8[IDX_C][(Pos)] = _cSCb_u8[i_width + (Col)],			retCr_u8[IDX_C][(Pos)] = _cSCr_u8[i_width + (Col)]; \
	retCb_u8[IDX_D][(Pos)] = _cSCb_u8[i_width + 1 + (Col)],		retCr_u8[IDX_D][(Pos)] = _cSCr_u8[i_width + 1 + (Col)]

    int32_t i_idx;
    const hl_pixel_t *_cSCb_u8, *_cSCr_u8;
    // int32_t i_idx0 = (i_y0 * i_stride) + i_x0;
    if (i_x0 >= 0 && i_y0 >= 0 && (i_x0 + 4) < i_width && (i_y0 + 4) < i_height) {
        /* No clip() needed [90% of the time] */
        i_idx = (i_y0 * i_width) + i_x0;
        // mov to start
        cSCb_u8 += i_idx, cSCr_u8 += i_idx;

        //== (row #0) ==//
        _cSCb_u8 = cSCb_u8, _cSCr_u8 = cSCr_u8;
        SET_PIXEL(0, 0);
        SET_PIXEL(1, 1);
        SET_PIXEL(2, 2);
        SET_PIXEL(3, 3);
        //== (row #1) ==//
        _cSCb_u8 += i_width, _cSCr_u8 += i_width;
        SET_PIXEL(4, 0);
        SET_PIXEL(5, 1);
        SET_PIXEL(6, 2);
        SET_PIXEL(7, 3);
        //== (row #2) ==//
        _cSCb_u8 += i_width, _cSCr_u8 += i_width;
        SET_PIXEL(8, 0);
        SET_PIXEL(9, 1);
        SET_PIXEL(10, 2);
        SET_PIXEL(11, 3);
        //== (row #3) ==//
        _cSCb_u8 += i_width, _cSCr_u8 += i_width;
        SET_PIXEL(12, 0);
        SET_PIXEL(13, 1);
        SET_PIXEL(14, 2);
        SET_PIXEL(15, 3);
    }
    else {
        int32_t i, j, y, i_x, i_y, i_height_minus1, i_width_minus1;
        int32_t x_pad[4/*A=0,B=1,C=2,D=3*/] = { 0, 1, 0, 1 };
        int32_t y_pad[4/*A=0,B=1,C=2,D=3*/] = { 0, 0, 1, 1 };
        uint8_t *_retCb_u8, *_retCr_u8;
        i_height_minus1  = i_height - 1;
        i_width_minus1 = i_width - 1;
        for (i = 0; i < 4; ++i) { // A, B, C, D
            _retCb_u8 = retCb_u8[i];
            _retCr_u8 = retCr_u8[i];
            for (j = 0, y = 0; y < 4; ++y) {
                i_y = HL_MATH_CLIP3(0, i_height_minus1, (i_y0 + y + y_pad[i]));
#if 0
                for (x = 0; x < 4; ++x)
#endif
                {
                    i_x = HL_MATH_CLIP3(0, i_width_minus1, (i_x0 + 0 + x_pad[i]));
                    i_idx = (i_y * i_width) + i_x;
                    _cSCb_u8 = (cSCb_u8 + i_idx), _cSCr_u8 = (cSCr_u8 + i_idx);
                    _retCb_u8[j] = *_cSCb_u8, _retCr_u8[j++] = *_cSCr_u8;

                    i_x = HL_MATH_CLIP3(0, i_width_minus1, (i_x0 + 1 + x_pad[i]));
                    i_idx = (i_y * i_width) + i_x;
                    _cSCb_u8 = (cSCb_u8 + i_idx), _cSCr_u8 = (cSCr_u8 + i_idx);
                    _retCb_u8[j] = *_cSCb_u8, _retCr_u8[j++] = *_cSCr_u8;

                    i_x = HL_MATH_CLIP3(0, i_width_minus1, (i_x0 + 2 + x_pad[i]));
                    i_idx = (i_y * i_width) + i_x;
                    _cSCb_u8 = (cSCb_u8 + i_idx), _cSCr_u8 = (cSCr_u8 + i_idx);
                    _retCb_u8[j] = *_cSCb_u8, _retCr_u8[j++] = *_cSCr_u8;

                    i_x = HL_MATH_CLIP3(0, i_width_minus1, (i_x0 + 3 + x_pad[i]));
                    i_idx = (i_y * i_width) + i_x;
                    _cSCb_u8 = (cSCb_u8 + i_idx), _cSCr_u8 = (cSCr_u8 + i_idx);
                    _retCb_u8[j] = *_cSCb_u8, _retCr_u8[j++] = *_cSCr_u8;
                }
            }
        }
    }
}

// TODO: add ASM version
void hl_codec_264_interpol_chroma_cat1_u8_cpp(
    struct hl_codec_264_s* p_codec,
    const hl_pixel_t* cSCb_u8, const hl_pixel_t* cSCr_u8,
    int32_t xFracC, int32_t yFracC,
    int32_t _xIntC, int32_t _yIntC, int32_t partWidthC, int32_t partHeightC, int32_t i_pic_height, int32_t i_pic_width,
    HL_OUT_ALIGNED(16) int32_t predPartLXCb[16][16],
    HL_OUT_ALIGNED(16) int32_t predPartLXCr[16][16])
{
    int32_t yC, xC, i, yIntC, xIntC, x, y;

    uint8_t (*retCb_u8)[4/*A=0,B=1,C=2,D=3*/][16];
    uint8_t (*retCr_u8)[4/*A=0,B=1,C=2,D=3*/][16];

    int32_t _xFracC_per_yFracC = __xFracC_per_yFracC[yFracC][xFracC];
    int32_t _8minus_xFracC = __8minus_xFracC[yFracC][xFracC];
    int32_t _8minus_yFracC = __8minus_yFracC[yFracC][xFracC];
    int32_t _8minus_xFracC_per_8minus_yFracC = __8minus_xFracC_per_8minus_yFracC[yFracC][xFracC];
    int32_t _8minus_xFracC_per_yFracC = __8minus_xFracC_per_yFracC[yFracC][xFracC];
    int32_t _8minus_yFracC_per_xFracC = __8minus_yFracC_per_xFracC[yFracC][xFracC];

    hl_memory_blocks_t *pc_mem_blocks = hl_codec_264_get_mem_blocks(p_codec);

    // map() memory
    hl_memory_blocks_map(pc_mem_blocks, &retCb_u8);
    hl_memory_blocks_map(pc_mem_blocks, &retCr_u8);

    for (yC = 0; yC < partHeightC; yC+=4) {
        yIntC = _yIntC + yC; // (8-230)
        for (xC = 0; xC < partWidthC; xC+=4) {
            xIntC = _xIntC + xC; // (8-229)
            hl_codec_264_interpol_load_samples4x4_chroma_u8(cSCb_u8, (*retCb_u8), cSCr_u8, (*retCr_u8), xIntC, yIntC, i_pic_height, i_pic_width);

            for (i = 0, y = 0; y < 4; ++y) {
                for (x = 0; x < 4; ++x) { // FIXME: unroll loop
                    // 8.4.2.2.2 Chroma sample interpolation process
                    predPartLXCb[yC + y][xC + x] = (_8minus_xFracC_per_8minus_yFracC * (*retCb_u8)[IDX_A][i] + _8minus_yFracC_per_xFracC * (*retCb_u8)[IDX_B][i] +
                                                    _8minus_xFracC_per_yFracC * (*retCb_u8)[IDX_C][i] + _xFracC_per_yFracC * (*retCb_u8)[IDX_D][i] + 32) >> 6;
                    predPartLXCr[yC + y][xC + x] = (_8minus_xFracC_per_8minus_yFracC * (*retCr_u8)[IDX_A][i] + _8minus_yFracC_per_xFracC * (*retCr_u8)[IDX_B][i] +
                                                    _8minus_xFracC_per_yFracC * (*retCr_u8)[IDX_C][i] + _xFracC_per_yFracC * (*retCr_u8)[IDX_D][i] + 32) >> 6;
                    ++i;
                }
            }
        }
    }

    // unmap() memory
    hl_memory_blocks_unmap(pc_mem_blocks, retCb_u8);
    hl_memory_blocks_unmap(pc_mem_blocks, retCr_u8);
}

// FIXME: called in ASM and INTRIN code -> To be optimized
void hl_codec_264_interpol_load_samples4_u32(const uint32_t* pc_indices, const hl_pixel_t* cSL_u8, uint32_t* ret_u32/*[4]*/)
{
    ret_u32[0] = cSL_u8[pc_indices[0]];
    ret_u32[1] = cSL_u8[pc_indices[1]];
    ret_u32[2] = cSL_u8[pc_indices[2]];
    ret_u32[3] = cSL_u8[pc_indices[3]];
}


//FIXME: Interpol must work on sse2
HL_ERROR_T hl_codec_264_interpol_init_functions()
{
    HL_DEBUG_INFO("Initializing interpolation functions");

    hl_codec_264_interpol_load_samples4x4_u8 = hl_codec_264_interpol_load_samples4x4_u8_cpp;
    hl_codec_264_interpol_load_samples4x4_chroma_u8 = hl_codec_264_interpol_load_samples4x4_chroma_u8_cpp;

    hl_codec_264_interpol_luma00_horiz4 = hl_codec_264_interpol_luma00_horiz4_cpp;
    hl_codec_264_interpol_luma01_vert4 = hl_codec_264_interpol_luma01_vert4_cpp;
    hl_codec_264_interpol_luma02_vert4 = hl_codec_264_interpol_luma02_vert4_cpp;
    hl_codec_264_interpol_luma03_vert4 = hl_codec_264_interpol_luma03_vert4_cpp;
    hl_codec_264_interpol_luma10_horiz4 = hl_codec_264_interpol_luma10_horiz4_cpp;
    hl_codec_264_interpol_luma11_diag4 = hl_codec_264_interpol_luma11_diag4_cpp;
    hl_codec_264_interpol_luma12_vert4 = hl_codec_264_interpol_luma12_vert4_cpp;
    hl_codec_264_interpol_luma13_diag4 = hl_codec_264_interpol_luma13_diag4_cpp;
    hl_codec_264_interpol_luma20_horiz4 = hl_codec_264_interpol_luma20_horiz4_cpp;
    hl_codec_264_interpol_luma21_diag4 = hl_codec_264_interpol_luma21_diag4_cpp;
    hl_codec_264_interpol_luma22_vert4 = hl_codec_264_interpol_luma22_vert4_cpp;
    hl_codec_264_interpol_luma23_diag4 = hl_codec_264_interpol_luma23_diag4_cpp;
    hl_codec_264_interpol_luma30_horiz4 = hl_codec_264_interpol_luma30_horiz4_cpp;
    hl_codec_264_interpol_luma31_diag4 = hl_codec_264_interpol_luma31_diag4_cpp;
    hl_codec_264_interpol_luma32_vert4 = hl_codec_264_interpol_luma32_vert4_cpp;
    hl_codec_264_interpol_luma33_diag4 = hl_codec_264_interpol_luma33_diag4_cpp;

    hl_codec_264_interpol_luma00_horiz4x4_u8 = hl_codec_264_interpol_luma00_horiz4x4_u8_cpp;
    hl_codec_264_interpol_luma01_vert4x4_u8 = hl_codec_264_interpol_luma01_vert4x4_u8_cpp;
    hl_codec_264_interpol_luma02_vert4x4_u8 = hl_codec_264_interpol_luma02_vert4x4_u8_cpp;
    hl_codec_264_interpol_luma03_vert4x4_u8 = hl_codec_264_interpol_luma03_vert4x4_u8_cpp;
    hl_codec_264_interpol_luma10_horiz4x4_u8 = hl_codec_264_interpol_luma10_horiz4x4_u8_cpp;
    hl_codec_264_interpol_luma11_diag4x4_u8 = hl_codec_264_interpol_luma11_diag4x4_u8_cpp;
    hl_codec_264_interpol_luma12_vert4x4_u8 = hl_codec_264_interpol_luma12_vert4x4_u8_cpp;
    hl_codec_264_interpol_luma13_diag4x4_u8 = hl_codec_264_interpol_luma13_diag4x4_u8_cpp;
    hl_codec_264_interpol_luma20_horiz4x4_u8 = hl_codec_264_interpol_luma20_horiz4x4_u8_cpp;
    hl_codec_264_interpol_luma21_diag4x4_u8 = hl_codec_264_interpol_luma21_diag4x4_u8_cpp;
    hl_codec_264_interpol_luma22_vert4x4_u8 = hl_codec_264_interpol_luma22_vert4x4_u8_cpp;
    hl_codec_264_interpol_luma23_diag4x4_u8 = hl_codec_264_interpol_luma23_diag4x4_u8_cpp;
    hl_codec_264_interpol_luma30_horiz4x4_u8 = hl_codec_264_interpol_luma30_horiz4x4_u8_cpp;
    hl_codec_264_interpol_luma31_diag4x4_u8 = hl_codec_264_interpol_luma31_diag4x4_u8_cpp;
    hl_codec_264_interpol_luma32_vert4x4_u8 = hl_codec_264_interpol_luma32_vert4x4_u8_cpp;
    hl_codec_264_interpol_luma33_diag4x4_u8 = hl_codec_264_interpol_luma33_diag4x4_u8_cpp;

    hl_codec_264_interpol_chroma = hl_codec_264_interpol_chroma_cpp;
    hl_codec_264_interpol_chroma_cat1_u8 = hl_codec_264_interpol_chroma_cat1_u8_cpp;

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        hl_codec_264_interpol_load_samples4x4_u8 = hl_codec_x86_264_interpol_load_samples4x4_u8_intrin_sse2;
        hl_codec_264_interpol_chroma_cat1_u8 = hl_codec_x86_264_interpol_chroma_cat1_u8_intrin_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE3)) {
        hl_codec_264_interpol_luma00_horiz4x4_u8 = hl_codec_x86_264_interpol_luma00_horiz4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma01_vert4x4_u8 = hl_codec_x86_264_interpol_luma01_vert4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma02_vert4x4_u8 = hl_codec_x86_264_interpol_luma02_vert4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma03_vert4x4_u8 = hl_codec_x86_264_interpol_luma03_vert4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma10_horiz4x4_u8 = hl_codec_x86_264_interpol_luma10_horiz4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma11_diag4x4_u8 = hl_codec_x86_264_interpol_luma11_diag4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma12_vert4x4_u8 = hl_codec_x86_264_interpol_luma12_vert4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma13_diag4x4_u8 = hl_codec_x86_264_interpol_luma13_diag4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma20_horiz4x4_u8 = hl_codec_x86_264_interpol_luma20_horiz4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma21_diag4x4_u8 = hl_codec_x86_264_interpol_luma21_diag4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma22_vert4x4_u8 = hl_codec_x86_264_interpol_luma22_vert4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma23_diag4x4_u8 = hl_codec_x86_264_interpol_luma23_diag4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma30_horiz4x4_u8 = hl_codec_x86_264_interpol_luma30_horiz4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma31_diag4x4_u8 = hl_codec_x86_264_interpol_luma31_diag4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma32_vert4x4_u8 = hl_codec_x86_264_interpol_luma32_vert4x4_u8_intrin_sse3;
        hl_codec_264_interpol_luma33_diag4x4_u8 = hl_codec_x86_264_interpol_luma33_diag4x4_u8_intrin_sse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        hl_codec_264_interpol_load_samples4x4_u8 = hl_codec_x86_264_interpol_load_samples4x4_u8_intrin_sse41; // FIXME: SSE2 version faster

        hl_codec_264_interpol_luma00_horiz4 = hl_codec_x86_264_interpol_luma00_horiz4_intrin_sse41;
        hl_codec_264_interpol_luma01_vert4 = hl_codec_x86_264_interpol_luma01_vert4_intrin_sse41;
        hl_codec_264_interpol_luma02_vert4 = hl_codec_x86_264_interpol_luma02_vert4_intrin_sse41;
        hl_codec_264_interpol_luma03_vert4 = hl_codec_x86_264_interpol_luma03_vert4_intrin_sse41;
        hl_codec_264_interpol_luma10_horiz4 = hl_codec_x86_264_interpol_luma10_horiz4_intrin_sse41;
        hl_codec_264_interpol_luma11_diag4 = hl_codec_x86_264_interpol_luma11_diag4_intrin_sse41;
        hl_codec_264_interpol_luma12_vert4 = hl_codec_x86_264_interpol_luma12_vert4_intrin_sse41;
        hl_codec_264_interpol_luma13_diag4 = hl_codec_x86_264_interpol_luma13_diag4_intrin_sse41;
        hl_codec_264_interpol_luma20_horiz4 = hl_codec_x86_264_interpol_luma20_horiz4_intrin_sse41;
        hl_codec_264_interpol_luma21_diag4 = hl_codec_x86_264_interpol_luma21_diag4_intrin_sse41;
        hl_codec_264_interpol_luma22_vert4 = hl_codec_x86_264_interpol_luma22_vert4_intrin_sse41;
        hl_codec_264_interpol_luma23_diag4 = hl_codec_x86_264_interpol_luma23_diag4_intrin_sse41;
        hl_codec_264_interpol_luma30_horiz4 = hl_codec_x86_264_interpol_luma30_horiz4_intrin_sse41;
        hl_codec_264_interpol_luma31_diag4 = hl_codec_x86_264_interpol_luma31_diag4_intrin_sse41;
        hl_codec_264_interpol_luma32_vert4 = hl_codec_x86_264_interpol_luma32_vert4_intrin_sse41;
        hl_codec_264_interpol_luma33_diag4 = hl_codec_x86_264_interpol_luma33_diag4_intrin_sse41;
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE3)) {
        extern void hl_codec_x86_264_interpol_luma00_horiz4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma01_vert4x4_u8_asm_sse3(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma02_vert4x4_u8_asm_sse3(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma03_vert4x4_u8_asm_sse3(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma10_horiz4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma11_diag4x4_u8_asm_sse3(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma12_vert4x4_u8_asm_sse3(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma13_diag4x4_u8_asm_sse3(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma20_horiz4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma21_diag4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma22_vert4x4_u8_asm_sse3(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma23_diag4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma30_horiz4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma31_diag4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma32_vert4x4_u8_asm_sse3(const uint32_t* pc_indices_vert[7], int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);
        extern void hl_codec_x86_264_interpol_luma33_diag4x4_u8_asm_sse3(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* predPartLXL16x16);

        hl_codec_264_interpol_luma00_horiz4x4_u8 = hl_codec_x86_264_interpol_luma00_horiz4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma01_vert4x4_u8 = hl_codec_x86_264_interpol_luma01_vert4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma02_vert4x4_u8 = hl_codec_x86_264_interpol_luma02_vert4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma03_vert4x4_u8 = hl_codec_x86_264_interpol_luma03_vert4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma10_horiz4x4_u8 = hl_codec_x86_264_interpol_luma10_horiz4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma11_diag4x4_u8 = hl_codec_x86_264_interpol_luma11_diag4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma12_vert4x4_u8 = hl_codec_x86_264_interpol_luma12_vert4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma13_diag4x4_u8 = hl_codec_x86_264_interpol_luma13_diag4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma20_horiz4x4_u8 = hl_codec_x86_264_interpol_luma20_horiz4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma21_diag4x4_u8 = hl_codec_x86_264_interpol_luma21_diag4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma22_vert4x4_u8 = hl_codec_x86_264_interpol_luma22_vert4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma23_diag4x4_u8 = hl_codec_x86_264_interpol_luma23_diag4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma30_horiz4x4_u8 = hl_codec_x86_264_interpol_luma30_horiz4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma31_diag4x4_u8 = hl_codec_x86_264_interpol_luma31_diag4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma32_vert4x4_u8 = hl_codec_x86_264_interpol_luma32_vert4x4_u8_asm_sse3;
        hl_codec_264_interpol_luma33_diag4x4_u8 = hl_codec_x86_264_interpol_luma33_diag4x4_u8_asm_sse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        extern void hl_codec_x86_264_interpol_load_samples4x4_u8_asm_sse41(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL_u8, HL_ALIGNED(16) uint8_t* ret_u8/*[16]*/);
        extern void hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL);
        extern void hl_codec_x86_264_interpol_luma01_vert4_asm_sse41(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma02_vert4_asm_sse41(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL,HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma03_vert4_asm_sse41(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma11_diag4_asm_sse41(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma12_vert4_asm_sse41(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma13_diag4_asm_sse41(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma21_diag4_asm_sse41(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma22_vert4_asm_sse41(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma23_diag4_asm_sse41(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma31_diag4_asm_sse41(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma32_vert4_asm_sse41(const uint32_t* pc_indices_vert[7], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        extern void hl_codec_x86_264_interpol_luma33_diag4_asm_sse41(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);

        hl_codec_264_interpol_load_samples4x4_u8 = hl_codec_x86_264_interpol_load_samples4x4_u8_asm_sse41;

        hl_codec_264_interpol_luma00_horiz4 = hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41;
        hl_codec_264_interpol_luma01_vert4 = hl_codec_x86_264_interpol_luma01_vert4_asm_sse41;
        hl_codec_264_interpol_luma02_vert4 = hl_codec_x86_264_interpol_luma02_vert4_asm_sse41;
        hl_codec_264_interpol_luma03_vert4 = hl_codec_x86_264_interpol_luma03_vert4_asm_sse41;
        hl_codec_264_interpol_luma10_horiz4 = hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41;
        hl_codec_264_interpol_luma11_diag4 = hl_codec_x86_264_interpol_luma11_diag4_asm_sse41;
        hl_codec_264_interpol_luma12_vert4 = hl_codec_x86_264_interpol_luma12_vert4_asm_sse41;
        hl_codec_264_interpol_luma13_diag4 = hl_codec_x86_264_interpol_luma13_diag4_asm_sse41;
        hl_codec_264_interpol_luma20_horiz4 = hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41;
        hl_codec_264_interpol_luma21_diag4 = hl_codec_x86_264_interpol_luma21_diag4_asm_sse41;
        hl_codec_264_interpol_luma22_vert4 = hl_codec_x86_264_interpol_luma22_vert4_asm_sse41;
        hl_codec_264_interpol_luma23_diag4 = hl_codec_x86_264_interpol_luma23_diag4_asm_sse41;
        hl_codec_264_interpol_luma30_horiz4 = hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41;
        hl_codec_264_interpol_luma31_diag4 = hl_codec_x86_264_interpol_luma31_diag4_asm_sse41;
        hl_codec_264_interpol_luma32_vert4 = hl_codec_x86_264_interpol_luma32_vert4_asm_sse41;
        hl_codec_264_interpol_luma33_diag4 = hl_codec_x86_264_interpol_luma33_diag4_asm_sse41;
    }
#endif /* HL_HAVE_X86_ASM */

    return HL_ERROR_SUCCESS;
}

/*** OBJECT DEFINITION FOR "hl_codec_264_interpol_indices_t" ***/
static hl_object_t* hl_codec_264_interpol_indices_ctor(hl_object_t * self, va_list * app)
{
    hl_codec_264_interpol_indices_t *p_interpol_indices = (hl_codec_264_interpol_indices_t*)self;
    if (p_interpol_indices) {

    }
    return self;
}
static hl_object_t* hl_codec_264_interpol_indices_dtor(hl_object_t * self)
{
    hl_codec_264_interpol_indices_t *p_interpol_indices = (hl_codec_264_interpol_indices_t*)self;
    if (p_interpol_indices) {
        HL_SAFE_FREE(p_interpol_indices->p_indices_unpadded);
    }
    return self;
}
static int hl_codec_264_interpol_indices_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_codec_264_interpol_indices_def_s = {
    sizeof(hl_codec_264_interpol_indices_t),
    hl_codec_264_interpol_indices_ctor,
    hl_codec_264_interpol_indices_dtor,
    hl_codec_264_interpol_indices_cmp,
};
const hl_object_def_t *hl_codec_264_interpol_indices_def_t = &hl_codec_264_interpol_indices_def_s;