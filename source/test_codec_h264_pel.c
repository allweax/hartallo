#include <hartallo/hl_memory.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_math.h>
#include <hartallo/hl_bits.h>
#include <assert.h>

typedef uint8_t hl_pixel8_t;

#define HL_TEST_CODEC_H264_PEL_IMAGE_PATH					"C:\\Projects\\toulde\\videos\\AUD_MW_E\\AUD_MW_E.yuv"
#define HL_TEST_CODEC_H264_PEL_IMAGE_WIDTH					176 // entire image width
#define HL_TEST_CODEC_H264_PEL_IMAGE_HEIGHT					144 // entire image height

#define HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_START_X			0
#define HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_START_Y			0
#define HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_WIDTH				16 // partial image width to interpolate
#define HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_HEIGHT			16 // partial image height to interpolate
#define HL_TEST_CODEC_H264_PEL_IMAGE_HPEL_WIDTH				((HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_WIDTH << 1)-0)
#define HL_TEST_CODEC_H264_PEL_IMAGE_HPEL_HEIGHT			((HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_HEIGHT << 1)-0)
#define HL_TEST_CODEC_H264_PEL_MBAFF						HL_FALSE


//***********************************************
// hl_test_codec_h264_luma_build_indices
//***********************************************
static HL_ERROR_T hl_test_codec_h264_luma_build_indices(uint32_t u_width, uint32_t u_height, hl_bool_t b_mbaff, int32_t** pp_indices_unpadded, const int32_t** ppc_indices, uint32_t* pu_stride)
{
    int32_t i_width_padded, i_height_padded, i_indice, i_width_minus1 = u_width -1, i_height_minus1 = u_height - 1;
    int32_t x, y, i_width_plus_pad, i_height_plus_pad;
    int32_t __max_pad_size = 32; // must be at least MaxMbWidth/Height + 1 and 32 is perfect for AVX.
    uint32_t *p_indices;

    if (b_mbaff) {
        u_height >>= 1;
    }

    i_width_padded = u_width + (__max_pad_size << 1);
    i_width_padded += (16 - (i_width_padded & 15)); // make multiple of 16
    __max_pad_size = (i_width_padded - u_width) >> 1;
    i_height_padded = u_height + (__max_pad_size << 1);

    i_width_plus_pad = u_width + __max_pad_size;
    i_height_plus_pad = u_height + __max_pad_size;
    p_indices = hl_memory_malloc(i_width_padded * i_height_padded * sizeof(int32_t));
    if (!p_indices) {
        return HL_ERROR_OUTOFMEMMORY;
    }

    *pp_indices_unpadded = p_indices;

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

    // skip pads
    *ppc_indices = ((*pp_indices_unpadded) + (i_width_padded * __max_pad_size) + __max_pad_size);
    *pu_stride = i_width_padded;

    return HL_ERROR_SUCCESS;
}

//***********************************************
// hl_test_codec_h264_luma_hpel
//***********************************************
static HL_ERROR_T hl_test_codec_h264_luma_hpel(
    HL_IN const int32_t* pc_indices_start, int32_t i_indices_stride,
    HL_IN const hl_pixel8_t* pc_luma_ipel, // input
    HL_OUT hl_pixel8_t* p_luma_hpel, // output
    HL_IN uint32_t u_width, uint32_t u_height // size of the rectangle to interpolate (in i-pel coordinates)
)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    uint32_t u_width_hpel = (u_width << 1) - 0;
    uint32_t u_height_hpel = (u_height << 1) - 0;
    uint32_t u_height_hpel_mul_p2 = (u_height_hpel << 1);
    int32_t i_indices_stride_mul_m2 = -(i_indices_stride << 1);
    int32_t i_indices_stride_mul_m1 = i_indices_stride_mul_m2 + i_indices_stride;
    int32_t i_indices_stride_mul_p0 = 0;
    int32_t i_indices_stride_mul_p1 = i_indices_stride;
    int32_t i_indices_stride_mul_p2 = (i_indices_stride << 2);
    int32_t i_indices_stride_mul_p3 = i_indices_stride_mul_p2 + i_indices_stride;
    uint32_t i_i, j_i, i_h, j_h;
    hl_pixel8_t* _p_luma_hpel;
    const int32_t* _pc_indices_start;
    const int32_t* _pc_indices_horiz;
    const int32_t* _pc_indices_vert;


#undef Tap6Filter
#define Tap6Filter(E,F,G,H,I,J) (((E)-5*(F)+20*(G)+20*(H)-5*(I)+(J))>>5)
#undef BiLinearFilter
#define BiLinearFilter(A,B) (((A)+(B))>>1)

    HL_DEBUG_INFO("[TEST H264 PEL] Cheking Luma Half-Pel...");

    /* I-Pel positioning + horizontal 6-tap filter + vertical 6-tap filter on I-Pels */
    _p_luma_hpel = p_luma_hpel;
    _pc_indices_start = pc_indices_start;
    _pc_indices_vert = _pc_indices_start; // FIXME : vert not used
    for (j_h = 0, j_i = 0; j_h < u_height_hpel; j_h+=2, j_i+=1) {
        for (i_h = 0, i_i = 0; i_h < u_width_hpel; i_h+=2, i_i+=1) {
            _pc_indices_horiz = (_pc_indices_start + i_i);
            /* Place I-Pel samples in the H-Pel destination (G) */
            _p_luma_hpel[i_h] = pc_luma_ipel[*_pc_indices_horiz];
            /* Horizontal 6-Tap filter on next pixel (b) */
            _p_luma_hpel[i_h + 1] = Tap6Filter(
                                        pc_luma_ipel[_pc_indices_horiz[-2]],
                                        pc_luma_ipel[_pc_indices_horiz[-1]],
                                        pc_luma_ipel[_pc_indices_horiz[0]],
                                        pc_luma_ipel[_pc_indices_horiz[1]],
                                        pc_luma_ipel[_pc_indices_horiz[2]],
                                        pc_luma_ipel[_pc_indices_horiz[3]]);
            /* Vertical 6-Tap filter on the next line (h) */
            _p_luma_hpel[i_h + u_width_hpel] = Tap6Filter(
                                                   pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_m2]],
                                                   pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_m1]],
                                                   pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p0]],
                                                   pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p1]],
                                                   pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p2]],
                                                   pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p3]]);
        }
        // last/closing sample on the next horizontal line (ff) - Vertical 6-Tap filter
        _pc_indices_horiz += i_indices_stride; // go to next line
        _p_luma_hpel[u_width_hpel + u_height_hpel - 1] = Tap6Filter(
                    pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_m2]],
                    pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_m1]],
                    pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p0]],
                    pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p1]],
                    pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p2]],
                    pc_luma_ipel[_pc_indices_horiz[i_indices_stride_mul_p3]]);

        _pc_indices_start += i_indices_stride;
        _p_luma_hpel += u_height_hpel_mul_p2;
    }

    /* 6-tap filter on h-pels generated from the above steps */
    // Samples to process are interleaved (0:done, 1:todo, 2:done, 3:todo, 4:done, 5:todo...)
    _p_luma_hpel = (p_luma_hpel + u_height_hpel); // start at row-1
    // H-Pel on other samples from  position 5 to w-5 (they don't need to be clipped)
    for (j_h = 1; j_h < u_height_hpel; j_h+=2) {
        // performs 6-tap filter on the sample at position 1 and 3(0 already done at step-1)
        _p_luma_hpel[1] = Tap6Filter(
                              _p_luma_hpel[0],
                              _p_luma_hpel[0],
                              _p_luma_hpel[0],
                              _p_luma_hpel[2],
                              _p_luma_hpel[4],
                              _p_luma_hpel[6]);
        _p_luma_hpel[3] = Tap6Filter(
                              _p_luma_hpel[0],
                              _p_luma_hpel[0],
                              _p_luma_hpel[2],
                              _p_luma_hpel[4],
                              _p_luma_hpel[6],
                              _p_luma_hpel[8]);
        // performs 6-tap filter on the sample at position w-2 and w-4
        _p_luma_hpel[u_width_hpel-3] = Tap6Filter(
                                           _p_luma_hpel[u_width_hpel-6],
                                           _p_luma_hpel[u_width_hpel-4],
                                           _p_luma_hpel[u_width_hpel-2],
                                           _p_luma_hpel[u_width_hpel-1],
                                           _p_luma_hpel[u_width_hpel-1],
                                           _p_luma_hpel[u_width_hpel-1]);
        _p_luma_hpel[u_width_hpel-5] = Tap6Filter(
                                           _p_luma_hpel[u_width_hpel-10],
                                           _p_luma_hpel[u_width_hpel-8],
                                           _p_luma_hpel[u_width_hpel-6],
                                           _p_luma_hpel[u_width_hpel-4],
                                           _p_luma_hpel[u_width_hpel-2],
                                           _p_luma_hpel[u_width_hpel-1]);

        for (i_h = 4, i_i = 1; i_h < u_width_hpel - 4; i_h+=2, i_i+=1) {
            /* Horizontal 6-Tap filter on previous the hpels (j) */
            _p_luma_hpel[i_h + 1] = Tap6Filter(
                                        _p_luma_hpel[i_h - 4],
                                        _p_luma_hpel[i_h - 2],
                                        _p_luma_hpel[i_h],
                                        _p_luma_hpel[i_h + 2],
                                        _p_luma_hpel[i_h + 4],
                                        _p_luma_hpel[i_h + 6]);
        }
        _p_luma_hpel += u_height_hpel_mul_p2;
    }

    return err;
}


//***********************************************
// hl_test_codec_h264_luma_qpel
//***********************************************
static HL_ERROR_T hl_test_codec_h264_luma_qpel()
{
    HL_DEBUG_INFO("[TEST H264 PEL] Cheking Luma Quater-Pel...");

    return HL_ERROR_SUCCESS;
}

HL_ALWAYS_INLINE static void hl_test_codec_h264_luma_load_samples1x16_u8(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel8_t* cSL,
    hl_pixel8_t* ret,
    int32_t i_ret_stride
)
{
    int32_t i;
    (i_indices_stride);
    (i_ret_stride);
    for (i = 0;  i < 16; ++i) {
        ret[i] = cSL[pc_indices[i]];
    }
}

HL_ALWAYS_INLINE static void hl_test_codec_h264_luma_load_samples4x4_u8(
    const uint32_t* pc_indices,
    int32_t i_indices_stride,
    const hl_pixel8_t* cSL,
    hl_pixel8_t* ret,
    int32_t i_ret_stride
)
{
    int32_t i;
    for (i = 0; i < 4; ++i) {
        ret[0] = cSL[pc_indices[0]];
        ret[1] = cSL[pc_indices[1]];
        ret[2] = cSL[pc_indices[2]];
        ret[3] = cSL[pc_indices[3]];
        ret += i_ret_stride;
        pc_indices += i_indices_stride;
    }
}

HL_ERROR_T hl_test_codec_h264_pel()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t* p_indices_unpadded = HL_NULL;
    const int32_t* pc_indices;
    const int32_t* pc_indices_start;
    int32_t u_indices_stride;
    hl_pixel8_t* p_ipel_luma = HL_NULL; // padded with zero chroma pixels
    hl_pixel8_t* p_hpel_luma = HL_NULL; // padded with zero chroma pixels
    hl_pixel8_t* p_qpel_luma = HL_NULL; // padded with zero chroma pixels
    FILE *p_file_ipel = HL_NULL, *p_file_hpel = HL_NULL, *p_file_qpel = HL_NULL;
    hl_size_t u_ipel_size, u_hpel_size;

    // open file
    p_file_ipel = fopen(HL_TEST_CODEC_H264_PEL_IMAGE_PATH, "rb");
    if (!p_file_ipel) {
        HL_DEBUG_ERROR("[TEST H264 PEL] Failed to open file at %s", HL_TEST_CODEC_H264_PEL_IMAGE_PATH);
        err = HL_ERROR_NOT_FOUND;
        goto bail;
    }
    // allocate memory for the ipel data
    u_ipel_size = ((HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_WIDTH * HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_HEIGHT * 3) >> 1) * sizeof(hl_pixel8_t);
    p_ipel_luma = hl_memory_malloc(u_ipel_size);
    if (!p_ipel_luma) {
        goto bail;
    }
    if (u_ipel_size != fread(p_ipel_luma, 1, u_ipel_size, p_file_ipel)) {
        HL_DEBUG_ERROR("[TEST H264 PEL] No enough data in file at %s", HL_TEST_CODEC_H264_PEL_IMAGE_PATH);
        err = HL_ERROR_INVALID_BITSTREAM;
        goto bail;
    }

    // build indices for the entire image
    err = hl_test_codec_h264_luma_build_indices(
              HL_TEST_CODEC_H264_PEL_IMAGE_WIDTH,
              HL_TEST_CODEC_H264_PEL_IMAGE_HEIGHT,
              HL_TEST_CODEC_H264_PEL_MBAFF,
              &p_indices_unpadded,
              &pc_indices,
              &u_indices_stride);
    if (err) {
        goto bail;
    }

    // Luma Half-Pel
    u_hpel_size = (((HL_TEST_CODEC_H264_PEL_IMAGE_HPEL_WIDTH * HL_TEST_CODEC_H264_PEL_IMAGE_HPEL_HEIGHT * 3) >> 1) * sizeof(hl_pixel8_t));
    p_hpel_luma = hl_memory_calloc(u_hpel_size, 1);
    if (!p_hpel_luma) {
        goto bail;
    }
    pc_indices_start = pc_indices + (HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_START_Y * u_indices_stride) + HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_START_X;
    err = hl_test_codec_h264_luma_hpel(
              pc_indices_start, u_indices_stride, // start at position 0, 0
              p_ipel_luma,
              p_hpel_luma,
              HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_WIDTH, HL_TEST_CODEC_H264_PEL_IMAGE_IPEL_HEIGHT // size of rectangle to interpolation (in i-pel coordinates)
          );
    if (err) {
        goto bail;
    }
    p_file_hpel = fopen("./hpel.yuv", "wb+");
    if (!p_file_hpel) {
        HL_DEBUG_ERROR("[TEST H264 PEL] Failed to open file at %s", "./hpel.yuv");
        err = HL_ERROR_ACCESS_DENIED;
        goto bail;
    }
    if (u_hpel_size != fwrite(p_hpel_luma, 1, u_hpel_size, p_file_hpel)) {
        HL_DEBUG_ERROR("[TEST H264 PEL] Failed to write to file at %s", "./hpel.yuv");
        err = HL_ERROR_ACCESS_DENIED;
        goto bail;
    }

    // Luma Quater-Pel
    /*err = hl_test_codec_h264_luma_hpel();
    if (err) {
        goto bail;
    }*/

    if (!err) {
        HL_DEBUG_INFO("[TEST H264 PEL] OK");
    }

bail:
    HL_SAFE_FREE(p_indices_unpadded);
    HL_SAFE_FREE(p_ipel_luma);
    HL_SAFE_FREE(p_hpel_luma);
    HL_SAFE_FREE(p_qpel_luma);
    if (p_file_ipel) {
        fclose(p_file_ipel);
    }
    if (p_file_hpel) {
        fclose(p_file_hpel);
    }
    if (p_file_qpel) {
        fclose(p_file_qpel);
    }

    return err;
}