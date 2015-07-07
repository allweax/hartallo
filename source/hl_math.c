#include "hartallo/hl_math.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_debug.h"

#if HL_HAVE_X86_INTRIN
#include "hartallo/intrinsics/x86/hl_math_x86_intrin.h"
#endif /* HL_HAVE_X86_INTRIN */

hl_bool_t (*hl_math_allzero16)(HL_ALIGNED(16) const int32_t (*block)[16]) = hl_math_allzero16_cpp;
int32_t (*hl_math_nnz16)(HL_ALIGNED(16) const int32_t block[16]) = hl_math_nnz16_cpp;
int32_t (*hl_math_nz16)(HL_ALIGNED(16) const int32_t block[16]) = hl_math_nz16_cpp;
int32_t (*hl_math_mae4x4_u8)(HL_ALIGNED(16) const uint8_t block1[4][4], HL_ALIGNED(16) const uint8_t block2[4][4]) = hl_math_mae4x4_u8_cpp;
int32_t (*hl_math_mse4x4_u8)(HL_ALIGNED(16) const uint8_t block1[4][4], HL_ALIGNED(16) const uint8_t block2[4][4]) = hl_math_mse4x4_u8_cpp;
int32_t (*hl_math_sad4x4_u8)(const uint8_t* b1, int32_t b1_stride, const uint8_t* b2, int32_t b2_stride) = hl_math_sad4x4_u8_cpp;
int32_t (*hl_math_satd4x4_u8)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride) = hl_math_satd4x4_u8_cpp;
int32_t (*hl_math_ssd4x4_u8)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride) = hl_math_ssd4x4_u8_cpp;
int32_t (*hl_math_ssd4x4_u32)(HL_ALIGNED(16) const int32_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride) =  hl_math_ssd4x4_u32_cpp;
int32_t (*hl_math_ssd4x4_u8x32)(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride) = hl_math_ssd4x4_u8x32_cpp;
void (*hl_math_dot_product4x1)(HL_ALIGNED(16) const int32_t a[4], HL_ALIGNED(16) const int32_t b[4], int32_t* r) = hl_math_dot_product4x1_cpp;
void (*hl_math_mul4x4)(HL_ALIGNED(16) const int32_t a[4][4], HL_ALIGNED(16) const int32_t b[4][4], HL_ALIGNED(16) int32_t r[4][4]) = hl_math_mul4x4_cpp;
void (*hl_math_tap6filter4x1_u32)(const hl_int128_t* E, const hl_int128_t* F, const hl_int128_t* G, const hl_int128_t* H, const hl_int128_t* I, const hl_int128_t* J, hl_int128_t* RET) = hl_math_tap6filter4x1_u32_cpp;
void (*hl_math_tap6filter4x2_u16_partial)(const hl_int128_t* e, const hl_int128_t* f, const hl_int128_t* g, const hl_int128_t* h, const hl_int128_t* i, const hl_int128_t* j, hl_int128_t* ret_lo, hl_int128_t* ret_hi) = HL_NULL; // avail. for "INTRIN_" and "ASM_" only.
void (*hl_math_clip3_4x1)(const hl_int128_t* min, const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret) = hl_math_clip3_4x1_cpp;
void (*hl_math_clip2_4x1)(const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret) = hl_math_clip2_4x1_cpp;
void (*hl_math_clip1_4x1)(const hl_int128_t* val, const hl_int128_t* BitDepth, hl_int128_t* ret) = hl_math_clip1_4x1_cpp;
void (*hl_math_addclip_4x4)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride) = hl_math_addclip_4x4_cpp;
void (*hl_math_addclip_4x4_u8xi32)(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride, HL_ALIGNED(16) uint8_t* ret, int32_t ret_stride) = hl_math_addclip_4x4_u8xi32_cpp;
void (*hl_math_addclip_8x8)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride) = hl_math_addclip_8x8_cpp;
void (*hl_math_addclip_16x16)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride) = hl_math_addclip_16x16_cpp;
void (*hl_math_sub4x4_u8x32)(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride) = hl_math_sub4x4_u8x32_cpp;
void (*hl_math_sub4x4_u8x8)(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const uint8_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride) = hl_math_sub4x4_u8x8_cpp;
int32_t (*hl_math_homogeneousity8x8_u8)(const uint8_t* non_hz_vt_edge_start, int32_t m1_stride) = hl_math_homogeneousity8x8_u8_cpp;

HL_ERROR_T hl_math_init_funcs()
{
    HL_DEBUG_INFO("Initializing math functions");

    hl_math_allzero16 = hl_math_allzero16_cpp;
    hl_math_nnz16 = hl_math_nnz16_cpp;
    hl_math_nz16 = hl_math_nz16_cpp;
    hl_math_mae4x4_u8 = hl_math_mae4x4_u8_cpp;
    hl_math_mse4x4_u8 = hl_math_mse4x4_u8_cpp;
    hl_math_sad4x4_u8 = hl_math_sad4x4_u8_cpp;
    hl_math_satd4x4_u8 = hl_math_satd4x4_u8_cpp;
    hl_math_ssd4x4_u8 = hl_math_ssd4x4_u8_cpp;
    hl_math_ssd4x4_u32 = hl_math_ssd4x4_u32_cpp;
    hl_math_ssd4x4_u8x32 = hl_math_ssd4x4_u8x32_cpp;
    hl_math_dot_product4x1 = hl_math_dot_product4x1_cpp;
    hl_math_mul4x4 = hl_math_mul4x4_cpp;
    hl_math_tap6filter4x1_u32 = hl_math_tap6filter4x1_u32_cpp;
    hl_math_clip3_4x1 = hl_math_clip3_4x1_cpp;
    hl_math_clip2_4x1 = hl_math_clip2_4x1_cpp;
    hl_math_clip1_4x1 = hl_math_clip1_4x1_cpp;
    hl_math_addclip_4x4 = hl_math_addclip_4x4_cpp;
    hl_math_addclip_4x4_u8xi32 = hl_math_addclip_4x4_u8xi32_cpp;
    hl_math_addclip_8x8 = hl_math_addclip_8x8_cpp;
    hl_math_addclip_16x16 = hl_math_addclip_16x16_cpp;
    hl_math_sub4x4_u8x32 = hl_math_sub4x4_u8x32_cpp;
    hl_math_sub4x4_u8x8 = hl_math_sub4x4_u8x8_cpp;
    hl_math_homogeneousity8x8_u8 = hl_math_homogeneousity8x8_u8_cpp;

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        hl_math_tap6filter4x1_u32 = hl_math_tap6filter4x1_u32_intrin_sse2;
        hl_math_allzero16 = hl_math_allzero16_intrin_sse2;
        hl_math_sad4x4_u8 = hl_math_sad4x4_u8_intrin_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE3)) {
        hl_math_tap6filter4x2_u16_partial = hl_math_tap6filter4x2_u16_partial_intrin_sse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSSE3)) {
        hl_math_dot_product4x1 = hl_math_dot_product4x1_intrin_ssse3;
        hl_math_mul4x4 = hl_math_mul4x4_intrin_ssse3_or_41;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        hl_math_allzero16 = hl_math_allzero16_intrin_sse41;
        hl_math_dot_product4x1 = hl_math_dot_product4x1_intrin_sse41;
        hl_math_mul4x4 = hl_math_mul4x4_intrin_ssse3_or_41;
        hl_math_tap6filter4x1_u32 = hl_math_tap6filter4x1_u32_intrin_sse41;
        hl_math_clip3_4x1 = hl_math_clip3_4x1_intrin_sse41;
        hl_math_clip2_4x1 = hl_math_clip2_4x1_intrin_sse41;
        hl_math_clip1_4x1 = hl_math_clip1_4x1_intrin_sse41;
        hl_math_addclip_4x4 = hl_math_addclip_4x4_intrin_sse41;
		hl_math_addclip_4x4_u8xi32 = hl_math_addclip_4x4_u8xi32_intrin_sse41;
        hl_math_addclip_8x8 = hl_math_addclip_8x8_intrin_sse41;
        hl_math_addclip_16x16 = hl_math_addclip_16x16_intrin_sse41;
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        extern hl_bool_t hl_math_allzero16_asm_sse2(HL_ALIGNED(16) const int32_t (*block)[16]);
        extern int32_t hl_math_sad4x4_u8_asm_sse2(const uint8_t* b1, int32_t b1_stride, const uint8_t* b2, int32_t b2_stride);

        hl_math_allzero16 = hl_math_allzero16_asm_sse2;
        hl_math_sad4x4_u8 = hl_math_sad4x4_u8_asm_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE3)) {
        extern void hl_math_tap6filter4x2_u16_partial_asm_sse3( const hl_int128_t* e, const hl_int128_t* f, const hl_int128_t* g, const hl_int128_t* h, const hl_int128_t* i, const hl_int128_t* j, hl_int128_t* ret_lo, hl_int128_t* ret_hi);

        hl_math_tap6filter4x2_u16_partial = hl_math_tap6filter4x2_u16_partial_asm_sse3;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        extern hl_bool_t hl_math_allzero16_asm_sse41(HL_ALIGNED(16) const int32_t (*block)[16]);
        extern void hl_math_tap6filter4x1_u32_asm_sse41(const hl_int128_t* E, const hl_int128_t* F, const hl_int128_t* G, const hl_int128_t* H, const hl_int128_t* I, const hl_int128_t* J, hl_int128_t* RET);
        extern void hl_math_tap6filter4x2_u16_partial_asm_sse41( const hl_int128_t* e, const hl_int128_t* f, const hl_int128_t* g, const hl_int128_t* h, const hl_int128_t* i, const hl_int128_t* j, hl_int128_t* ret_lo, hl_int128_t* ret_hi);
        extern void hl_math_clip3_4x1_asm_sse41(const hl_int128_t* min, const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);
        extern void hl_math_clip2_4x1_asm_sse41(const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);
        extern void hl_math_clip1_4x1_asm_sse41(const hl_int128_t* val, const hl_int128_t* BitDepth, hl_int128_t* ret);
        extern void hl_math_addclip_4x4_asm_sse41(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);
        extern void hl_math_addclip_4x4_u8xi32_asm_sse41(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) uint8_t* ret, int32_t ret_stride);
		extern void hl_math_addclip_8x8_asm_sse41(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);
        extern void hl_math_addclip_16x16_asm_sse41(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);

        hl_math_tap6filter4x1_u32 = hl_math_tap6filter4x1_u32_asm_sse41;
        hl_math_tap6filter4x2_u16_partial = hl_math_tap6filter4x2_u16_partial_asm_sse41;
        hl_math_allzero16 = hl_math_allzero16_asm_sse41;
        hl_math_clip3_4x1 = hl_math_clip3_4x1_asm_sse41;
        hl_math_clip2_4x1 = hl_math_clip2_4x1_asm_sse41;
        hl_math_clip1_4x1 = hl_math_clip1_4x1_asm_sse41;
        hl_math_addclip_4x4 = hl_math_addclip_4x4_asm_sse41;
		hl_math_addclip_4x4_u8xi32 = hl_math_addclip_4x4_u8xi32_asm_sse41;
        hl_math_addclip_8x8 = hl_math_addclip_8x8_asm_sse41;
        hl_math_addclip_16x16 = hl_math_addclip_16x16_asm_sse41;
    }
#endif /* HL_HAVE_X86_ASM */

    return HL_ERROR_SUCCESS;
}

hl_bool_t hl_math_allzero16_cpp(HL_ALIGNED(16) const int32_t (*block)[16])
{
    return !((*block)[0] || (*block)[1] || (*block)[2] || (*block)[3] ||
             (*block)[4] || (*block)[5] || (*block)[6] || (*block)[7] ||
             (*block)[8] || (*block)[9] || (*block)[10] || (*block)[11] ||
             (*block)[12] || (*block)[13] || (*block)[14] || (*block)[15]);
}

// TODO: ASM and INTRIN
int32_t hl_math_nnz16_cpp(HL_ALIGNED(16) const int32_t block[16])
{
    int32_t c = 0, i;
    for (i = 0; i < 16; i +=4) {
        if (block[i]) {
            ++c;
        }
        if (block[i + 1]) {
            ++c;
        }
        if (block[i + 2]) {
            ++c;
        }
        if (block[i + 3]) {
            ++c;
        }
    }
    return c;
}

// TODO: ASM and INTRIN
int32_t hl_math_nz16_cpp(HL_ALIGNED(16) const int32_t block[16])
{
    int32_t c = 0, i;
    for (i = 0; i < 16; i +=4) {
        if (!block[i]) {
            ++c;
        }
        if (!block[i + 1]) {
            ++c;
        }
        if (!block[i + 2]) {
            ++c;
        }
        if (!block[i + 3]) {
            ++c;
        }
    }
    return c;
}

// TODO: ASM and INTRIN
int32_t hl_math_mae4x4_u8_cpp(const uint8_t block1[4][4], const uint8_t block2[4][4])
{
    int32_t mae4x4=0;

    mae4x4+=HL_MATH_ABS_INT32(block1[0][0]-block2[0][0]);
    mae4x4+=HL_MATH_ABS_INT32(block1[0][1]-block2[0][1]);
    mae4x4+=HL_MATH_ABS_INT32(block1[0][2]-block2[0][2]);
    mae4x4+=HL_MATH_ABS_INT32(block1[0][3]-block2[0][3]);

    mae4x4+=HL_MATH_ABS_INT32(block1[1][0]-block2[1][0]);
    mae4x4+=HL_MATH_ABS_INT32(block1[1][1]-block2[1][1]);
    mae4x4+=HL_MATH_ABS_INT32(block1[1][2]-block2[1][2]);
    mae4x4+=HL_MATH_ABS_INT32(block1[1][3]-block2[1][3]);

    mae4x4+=HL_MATH_ABS_INT32(block1[2][0]-block2[2][0]);
    mae4x4+=HL_MATH_ABS_INT32(block1[2][1]-block2[2][1]);
    mae4x4+=HL_MATH_ABS_INT32(block1[2][2]-block2[2][2]);
    mae4x4+=HL_MATH_ABS_INT32(block1[2][3]-block2[2][3]);

    mae4x4+=HL_MATH_ABS_INT32(block1[3][0]-block2[3][0]);
    mae4x4+=HL_MATH_ABS_INT32(block1[3][1]-block2[3][1]);
    mae4x4+=HL_MATH_ABS_INT32(block1[3][2]-block2[3][2]);
    mae4x4+=HL_MATH_ABS_INT32(block1[3][3]-block2[3][3]);

    return mae4x4 >> 4;
}

// TODO: ASM and INTRIN
int32_t hl_math_mse4x4_u8_cpp(const uint8_t block1[4][4], const uint8_t block2[4][4])
{
    int32_t mse4x4=0,k;

    k=(block1[0][0]-block2[0][0]), mse4x4+=(k*k);
    k=(block1[0][1]-block2[0][1]), mse4x4+=(k*k);
    k=(block1[0][2]-block2[0][2]), mse4x4+=(k*k);
    k=(block1[0][3]-block2[0][3]), mse4x4+=(k*k);

    k=(block1[1][0]-block2[1][0]), mse4x4+=(k*k);
    k=(block1[1][1]-block2[1][1]), mse4x4+=(k*k);
    k=(block1[1][2]-block2[1][2]), mse4x4+=(k*k);
    k=(block1[1][3]-block2[1][3]), mse4x4+=(k*k);

    k=(block1[2][0]-block2[2][0]), mse4x4+=(k*k);
    k=(block1[2][1]-block2[2][1]), mse4x4+=(k*k);
    k=(block1[2][2]-block2[2][2]), mse4x4+=(k*k);
    k=(block1[2][3]-block2[2][3]), mse4x4+=(k*k);

    k=(block1[3][0]-block2[3][0]), mse4x4+=(k*k);
    k=(block1[3][1]-block2[3][1]), mse4x4+=(k*k);
    k=(block1[3][2]-block2[3][2]), mse4x4+=(k*k);
    k=(block1[3][3]-block2[3][3]), mse4x4+=(k*k);

    return mse4x4 >> 4;
}

// TODO: ASM
// Sum of Absolute Difference (SAD) / Sum of Absolute Errors (SAE)
int32_t hl_math_sad4x4_u8_cpp(const uint8_t* b1, int32_t b1_stride, const uint8_t* b2, int32_t b2_stride)
{
#define _HL_MATH_SAD(_b1_, _b2_, _ret_, _idx_) (_ret_)+=HL_MATH_ABS((b1)[(_idx_)]-(b2)[(_idx_)]);
#define _HL_MATH_SAD3(_b1_, _b2_, _ret_) _HL_MATH_SAD(_b1_,_b2_,sad4x4,0);_HL_MATH_SAD(_b1_,_b2_,sad4x4,1);_HL_MATH_SAD(_b1_,_b2_,sad4x4,2);_HL_MATH_SAD(_b1_,_b2_,sad4x4,3)
    int32_t sad4x4=0;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SAD3(b1,b2,sad4x4);

    return sad4x4;
#undef _HL_MATH_SAD
#undef _HL_MATH_SAD3
}

// TODO: ASM and INTRIN
// Sum of Absolute Difference (SAD) / Sum of Absolute Errors (SAE)
int32_t hl_math_sad4x4_u32_cpp(HL_ALIGNED(16) const int32_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride)
{
#define _HL_MATH_SAD(_b1_, _b2_, _ret_, _idx_) (_ret_)+=HL_MATH_ABS_INT32((b1)[(_idx_)]-(b2)[(_idx_)]);
#define _HL_MATH_SAD3(_b1_, _b2_, _ret_) _HL_MATH_SAD(_b1_,_b2_,sad4x4,0);_HL_MATH_SAD(_b1_,_b2_,sad4x4,1);_HL_MATH_SAD(_b1_,_b2_,sad4x4,2);_HL_MATH_SAD(_b1_,_b2_,sad4x4,3)
    int32_t sad4x4=0;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SAD3(b1,b2,sad4x4);
    return sad4x4;
#undef _HL_MATH_SAD
#undef _HL_MATH_SAD3
}

// FIXME: add ASM and INTRIN versions
int32_t hl_math_satd4x4_u8_cpp(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride)
{
    int32_t HL_ALIGN(HL_ALIGN_V) Diff4x4[4][4], satd4x4;

    // Diff4x4 = b1 - b2
    Diff4x4[0][0] = b1[0] - b2[0], Diff4x4[0][1] = b1[1] - b2[1], Diff4x4[0][2] = b1[2] - b2[2], Diff4x4[0][3] = b1[3] - b2[3];
    b1 += b1_stride, b2 += b2_stride;
    Diff4x4[1][0] = b1[0] - b2[0], Diff4x4[1][1] = b1[1] - b2[1], Diff4x4[1][2] = b1[2] - b2[2], Diff4x4[1][3] = b1[3] - b2[3];
    b1 += b1_stride, b2 += b2_stride;
    Diff4x4[2][0] = b1[0] - b2[0], Diff4x4[2][1] = b1[1] - b2[1], Diff4x4[2][2] = b1[2] - b2[2], Diff4x4[2][3] = b1[3] - b2[3];
    b1 += b1_stride, b2 += b2_stride;
    Diff4x4[3][0] = b1[0] - b2[0], Diff4x4[3][1] = b1[1] - b2[1], Diff4x4[3][2] = b1[2] - b2[2], Diff4x4[3][3] = b1[3] - b2[3];

    // Diff4x4 = H*Diff4x4*H NOT EQUAL to Hadamard(Diff4x4), because the "/2" operation is done after sum(abs(x))
    // TODO : not cache opt. see hl_codec_264_transf_scale_luma_dc_coeff_intra16x16_cpp()
    {
        int32_t HL_ALIGN(HL_ALIGN_V) tmp4x4[4][4];

        // tmp4x4 = MULT(H, Diff4x4)
        tmp4x4[0][0] = Diff4x4[0][0] + Diff4x4[1][0] + Diff4x4[2][0] + Diff4x4[3][0];
        tmp4x4[0][1] = Diff4x4[0][1] + Diff4x4[1][1] + Diff4x4[2][1] + Diff4x4[3][1];
        tmp4x4[0][2] = Diff4x4[0][2] + Diff4x4[1][2] + Diff4x4[2][2] + Diff4x4[3][2];
        tmp4x4[0][3] = Diff4x4[0][3] + Diff4x4[1][3] + Diff4x4[2][3] + Diff4x4[3][3];

        tmp4x4[1][0] = Diff4x4[0][0] + Diff4x4[1][0] - Diff4x4[2][0] - Diff4x4[3][0];
        tmp4x4[1][1] = Diff4x4[0][1] + Diff4x4[1][1] - Diff4x4[2][1] - Diff4x4[3][1];
        tmp4x4[1][2] = Diff4x4[0][2] + Diff4x4[1][2] - Diff4x4[2][2] - Diff4x4[3][2];
        tmp4x4[1][3] = Diff4x4[0][3] + Diff4x4[1][3] - Diff4x4[2][3] - Diff4x4[3][3];

        tmp4x4[2][0] = Diff4x4[0][0] - Diff4x4[1][0] - Diff4x4[2][0] + Diff4x4[3][0];
        tmp4x4[2][1] = Diff4x4[0][1] - Diff4x4[1][1] - Diff4x4[2][1] + Diff4x4[3][1];
        tmp4x4[2][2] = Diff4x4[0][2] - Diff4x4[1][2] - Diff4x4[2][2] + Diff4x4[3][2];
        tmp4x4[2][3] = Diff4x4[0][3] - Diff4x4[1][3] - Diff4x4[2][3] + Diff4x4[3][3];

        tmp4x4[3][0] = Diff4x4[0][0] - Diff4x4[1][0] + Diff4x4[2][0] - Diff4x4[3][0];
        tmp4x4[3][1] = Diff4x4[0][1] - Diff4x4[1][1] + Diff4x4[2][1] - Diff4x4[3][1];
        tmp4x4[3][2] = Diff4x4[0][2] - Diff4x4[1][2] + Diff4x4[2][2] - Diff4x4[3][2];
        tmp4x4[3][3] = Diff4x4[0][3] - Diff4x4[1][3] + Diff4x4[2][3] - Diff4x4[3][3];

        // Diff4x4 = MULT(tmp4x4, H)
        Diff4x4[0][0] = tmp4x4[0][0] + tmp4x4[0][1] + tmp4x4[0][2] + tmp4x4[0][3];
        Diff4x4[1][0] = tmp4x4[1][0] + tmp4x4[1][1] + tmp4x4[1][2] + tmp4x4[1][3];
        Diff4x4[2][0] = tmp4x4[2][0] + tmp4x4[2][1] + tmp4x4[2][2] + tmp4x4[2][3];
        Diff4x4[3][0] = tmp4x4[3][0] + tmp4x4[3][1] + tmp4x4[3][2] + tmp4x4[3][3];

        Diff4x4[0][1] = tmp4x4[0][0] + tmp4x4[0][1] - tmp4x4[0][2] - tmp4x4[0][3];
        Diff4x4[1][1] = tmp4x4[1][0] + tmp4x4[1][1] - tmp4x4[1][2] - tmp4x4[1][3];
        Diff4x4[2][1] = tmp4x4[2][0] + tmp4x4[2][1] - tmp4x4[2][2] - tmp4x4[2][3];
        Diff4x4[3][1] = tmp4x4[3][0] + tmp4x4[3][1] - tmp4x4[3][2] - tmp4x4[3][3];

        Diff4x4[0][2] = tmp4x4[0][0] - tmp4x4[0][1] - tmp4x4[0][2] + tmp4x4[0][3];
        Diff4x4[1][2] = tmp4x4[1][0] - tmp4x4[1][1] - tmp4x4[1][2] + tmp4x4[1][3];
        Diff4x4[2][2] = tmp4x4[2][0] - tmp4x4[2][1] - tmp4x4[2][2] + tmp4x4[2][3];
        Diff4x4[3][2] = tmp4x4[3][0] - tmp4x4[3][1] - tmp4x4[3][2] + tmp4x4[3][3];

        Diff4x4[0][3] = tmp4x4[0][0] - tmp4x4[0][1] + tmp4x4[0][2] - tmp4x4[0][3];
        Diff4x4[1][3] = tmp4x4[1][0] - tmp4x4[1][1] + tmp4x4[1][2] - tmp4x4[1][3];
        Diff4x4[2][3] = tmp4x4[2][0] - tmp4x4[2][1] + tmp4x4[2][2] - tmp4x4[2][3];
        Diff4x4[3][3] = tmp4x4[3][0] - tmp4x4[3][1] + tmp4x4[3][2] - tmp4x4[3][3];
    }

    // satd4x4 sum(abs(Diff4x4[i][j])) / 2;
#undef FAST_ABS
#define FAST_ABS(x) ((x) < 0 ? -(x) : (x)) // ((x) ^ ((x) >> 31)) - ((x) >> 31)
    satd4x4 = FAST_ABS(Diff4x4[0][0]), satd4x4 += FAST_ABS(Diff4x4[0][1]), satd4x4 += FAST_ABS(Diff4x4[0][2]), satd4x4 += FAST_ABS(Diff4x4[0][3]);
    satd4x4 += FAST_ABS(Diff4x4[1][0]), satd4x4 += FAST_ABS(Diff4x4[1][1]), satd4x4 += FAST_ABS(Diff4x4[1][2]), satd4x4 += FAST_ABS(Diff4x4[1][3]);
    satd4x4 += FAST_ABS(Diff4x4[2][0]), satd4x4 += FAST_ABS(Diff4x4[2][1]), satd4x4 += FAST_ABS(Diff4x4[2][2]), satd4x4 += FAST_ABS(Diff4x4[2][3]);
    satd4x4 += FAST_ABS(Diff4x4[3][0]), satd4x4 += FAST_ABS(Diff4x4[3][1]), satd4x4 += FAST_ABS(Diff4x4[3][2]), satd4x4 += FAST_ABS(Diff4x4[3][3]);
    satd4x4 >>= 1;
#undef FAST_ABS

    return satd4x4;
}

// TODO: ASM and INTRIN
// HL_MATH_ABS not needed because of the square
// Sum of Squared Differences (U8)
int32_t hl_math_ssd4x4_u8_cpp(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const uint8_t* b2, int32_t b2_stride)
{
#define _HL_MATH_SSD(_b1_, _b2_, _ret_, _idx_) k = ((_b1_)[_idx_] - (_b2_)[(_idx_)]), (_ret_) += (k*k);
#define _HL_MATH_SSD_3(_b1_, _b2_, _ret_)_HL_MATH_SSD((_b1_),(_b2_),(_ret_),0);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),1);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),2);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),3);
    int32_t ssd4x4 = 0, k;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    return ssd4x4;
#undef _HL_MATH_SSD
#undef _HL_MATH_SSD_3
}

// TODO: ASM and INTRIN
// HL_MATH_ABS not needed because of the square
// Sum of Squared Differences (U32)
int32_t hl_math_ssd4x4_u32_cpp(HL_ALIGNED(16) const int32_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride)
{
#define _HL_MATH_SSD(_b1_, _b2_, _ret_, _idx_) k = ((_b1_)[_idx_] - (_b2_)[(_idx_)]), (_ret_) += (k*k);
#define _HL_MATH_SSD_3(_b1_, _b2_, _ret_)_HL_MATH_SSD((_b1_),(_b2_),(_ret_),0);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),1);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),2);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),3);
    int32_t ssd4x4 = 0, k;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    return ssd4x4;
#undef _HL_MATH_SSD
#undef _HL_MATH_SSD_3
}

// FIXME: ASM and INTRIN
// HL_MATH_ABS not needed because of the square
// Sum of Squared Differences (U8x32)
int32_t hl_math_ssd4x4_u8x32_cpp(HL_ALIGNED(16) const uint8_t* b1, int32_t b1_stride, HL_ALIGNED(16) const int32_t* b2, int32_t b2_stride)
{
#define _HL_MATH_SSD(_b1_, _b2_, _ret_, _idx_) k = ((_b1_)[_idx_] - (_b2_)[(_idx_)]), (_ret_) += (k*k);
#define _HL_MATH_SSD_3(_b1_, _b2_, _ret_)_HL_MATH_SSD((_b1_),(_b2_),(_ret_),0);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),1);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),2);_HL_MATH_SSD((_b1_),(_b2_),(_ret_),3);
    int32_t ssd4x4 = 0, k;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    b1 += b1_stride;
    b2 += b2_stride;
    _HL_MATH_SSD_3(b1,b2,ssd4x4);
    return ssd4x4;
#undef _HL_MATH_SSD
#undef _HL_MATH_SSD_3
}

#define _HL_MATH_SUB(_m1_, _m2_, _ret_, _idx_) (_ret_)[(_idx_)] = (_m1_)[_idx_] - (_m2_)[(_idx_)]
// for(idx=0...3)
#define _HL_MATH_SUB_3(_m1_, _m2_, _ret_)_HL_MATH_SUB((_m1_),(_m2_),(_ret_),0);_HL_MATH_SUB((_m1_),(_m2_),(_ret_),1);_HL_MATH_SUB((_m1_),(_m2_),(_ret_),2);_HL_MATH_SUB((_m1_),(_m2_),(_ret_),3);

// FIXME: Add INTRIN and ASM versions
void hl_math_sub4x4_u8x32_cpp(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride)
{
    _HL_MATH_SUB_3(m1,m2,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_SUB_3(m1,m2,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_SUB_3(m1,m2,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_SUB_3(m1,m2,ret);
}

// FIXME: Add INTRIN and ASM versions
void hl_math_sub4x4_u8x8_cpp(HL_ALIGNED(16) const uint8_t* m1, int32_t m1_stride, HL_ALIGNED(16) const uint8_t* m2, int32_t m2_stride, HL_ALIGNED(16) int32_t* ret, int32_t ret_stride)
{
    _HL_MATH_SUB_3(m1,m2,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_SUB_3(m1,m2,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_SUB_3(m1,m2,ret);
    ret += ret_stride;
    m1 += m1_stride;
    m2 += m2_stride;
    _HL_MATH_SUB_3(m1,m2,ret);
}


// FIXME: Add INTRIN and ASM versions
// JVT-O079 - 2.1.3.4.1 Edge Map Generation, Equation (2-35)
//!\ "non_hz_vt_edge_start" MUST not point to an hz/vt edge -> x€[1, W - 2], y€[1, H - 2]
int32_t hl_math_homogeneousity8x8_u8_cpp(const uint8_t* non_hz_vt_edge_start, int32_t m1_stride)
{
    int32_t i, j, dx, dy, ret = 0;
    const uint8_t *u8, *u8_jminus1, *u8_jplus1;

    for (j = 0; j < 8; ++j) {
        u8 = non_hz_vt_edge_start + (m1_stride * j);
        u8_jminus1 = u8 - m1_stride;
        u8_jplus1 = u8 + m1_stride;
        for (i = 0; i <8; ++i) {
            dx = u8_jplus1[i - 1] + (u8_jplus1[i] << 1) + u8_jplus1[i + 1] - u8_jminus1[i - 1] - (u8_jminus1[i] << 1) - u8_jminus1[i+1];
            dy = u8_jminus1[i + 1] + (u8[i+1] << 1) + u8_jplus1[i + 1] - u8_jminus1[i - 1] - (u8[i - 1] << 1) - u8_jplus1[i - 1];
            ret += HL_MATH_ABS_INT32(dx) + HL_MATH_ABS_INT32(dy);
        }
    }
    return ret;
}

#undef _HL_MATH_SUB
#undef _HL_MATH_SUB_3

