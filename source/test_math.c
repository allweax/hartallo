#if 0
#include <hartallo/hl_math.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_debug.h>
#include <assert.h>

#if HL_HAVE_X86_INTRIN
#	include <hartallo/intrinsics/x86/hl_math_x86_intrin.h>
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
extern hl_bool_t hl_math_allzero16_asm_sse2(HL_ALIGNED(16) int32_t (*block)[16]);
extern hl_bool_t hl_math_allzero16_asm_sse41(HL_ALIGNED(16) int32_t (*block)[16]);
extern void hl_math_clip3_4x1_asm_sse41(const hl_int128_t* min, const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);
extern void hl_math_clip2_4x1_asm_sse41(const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);
extern void hl_math_clip1_4x1_asm_sse41(const hl_int128_t* val, const hl_int128_t* BitDepth, hl_int128_t* ret);
extern void hl_math_tap6filter4x1_u32_asm_sse41(
    const hl_int128_t* E,
    const hl_int128_t* F,
    const hl_int128_t* G,
    const hl_int128_t* H,
    const hl_int128_t* I,
    const hl_int128_t* J,
    hl_int128_t* RET);
extern void hl_math_addclip_4x4_asm_sse41(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);
extern void hl_math_transpose_4x4_asm_sse2(const int32_t m[4][4], int32_t ret[4][4]);
#endif /* HL_HAVE_X86_ASM */

//**************************************
// test_math_all_zeros16
//**************************************
static HL_ERROR_T test_math_all_zeros16()
{
    HL_ALIGN(HL_ALIGN_V) int32_t val[16] = { 0 };
    int32_t i, count = 1;
    struct func {
        hl_bool_t (*pf)(HL_ALIGNED(16) int32_t (*block)[16]);
        const char* name;
    };
    struct func pfs[10] = { { hl_math_allzero16_cpp, "hl_math_allzero16_cpp" } };

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_math_allzero16_intrin_sse2;
        pfs[count++].name = "hl_math_allzero16_intrin_sse2";
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_math_allzero16_intrin_sse41;
        pfs[count++].name = "hl_math_allzero16_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */
#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_math_allzero16_asm_sse2;
        pfs[count++].name = "hl_math_allzero16_asm_sse2";
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_math_allzero16_asm_sse41;
        pfs[count++].name = "hl_math_allzero16_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", pfs[i].name);
    }

    // val is zero
    for (i = 0; i < count; ++i) {
        if (!pfs[i].pf(&val)) {
            HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", pfs[i].name);
            return HL_ERROR_TEST_FAILED;
        }
    }
    // compare optimized code with cpp implementation
    if (count > 1) {
        int32_t j;
        for (j = 0; j < 20; ++j) {
            val[rand() & 15] = rand() % rand();
            for (i = 1; i < count; ++i) {
                if (pfs[0].pf(&val) != pfs[i].pf(&val)) {
                    HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", pfs[i].name);
                    return HL_ERROR_TEST_FAILED;
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//**************************************
// test_math_clips
//**************************************
static HL_ERROR_T test_math_clips()
{
    HL_ALIGN(HL_ALIGN_V) int32_t max[4];
    HL_ALIGN(HL_ALIGN_V) int32_t min[4];
    HL_ALIGN(HL_ALIGN_V) int32_t val[4];
    HL_ALIGN(HL_ALIGN_V) int32_t ret[4];
    HL_ALIGN(HL_ALIGN_V) int32_t BitDepth[4] = {0};
    int32_t i, j, r, k, clip1_count = 1, clip2_count = 1, clip3_count = 1;

    struct clip1 {
        void (*pf)(const hl_int128_t* val, const hl_int128_t* BitDepth, hl_int128_t* ret);
        const char* name;
    };
    struct clip1 clip1s[10] = { { hl_math_clip1_4x1_cpp, "hl_math_clip1_4x1_cpp" } };
    struct clip2 {
        void (*pf)(const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);
        const char* name;
    };
    struct clip2 clip2s[10] = { { hl_math_clip2_4x1_cpp, "hl_math_clip2_4x1_cpp" } };
    struct clip3 {
        void (*pf)(const hl_int128_t* min, const hl_int128_t* max, const hl_int128_t* val, hl_int128_t* ret);
        const char* name;
    };
    struct clip3 clip3s[10] = { { hl_math_clip3_4x1_cpp, "hl_math_clip3_4x1_cpp" } };

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        clip1s[clip1_count].pf = hl_math_clip1_4x1_intrin_sse41;
        clip1s[clip1_count++].name = "hl_math_clip1_4x1_intrin_sse41";
        clip2s[clip2_count].pf = hl_math_clip2_4x1_intrin_sse41;
        clip2s[clip2_count++].name = "hl_math_clip2_4x1_intrin_sse41";
        clip3s[clip3_count].pf = hl_math_clip3_4x1_intrin_sse41;
        clip3s[clip3_count++].name = "hl_math_clip3_4x1_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        clip1s[clip1_count].pf = hl_math_clip1_4x1_asm_sse41;
        clip1s[clip1_count++].name = "hl_math_clip1_4x1_asm_sse41";
        clip2s[clip2_count].pf = hl_math_clip2_4x1_asm_sse41;
        clip2s[clip2_count++].name = "hl_math_clip2_4x1_asm_sse41";
        clip3s[clip3_count].pf = hl_math_clip3_4x1_asm_sse41;
        clip3s[clip3_count++].name = "hl_math_clip3_4x1_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */

    for (i = 0; i < clip1_count; ++i) {
        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", clip1s[i].name);
    }
    for (i = 0; i < clip2_count; ++i) {
        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", clip2s[i].name);
    }
    for (i = 0; i < clip3_count; ++i) {
        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", clip3s[i].name);
    }

    for (i = 0; i  < 100; ++i) {
        BitDepth[0] /*= BitDepth[1] = BitDepth[2] = BitDepth[3]*/ = rand() & 31;
        for (j = 0; j < 4; ++j) {
            min[j] = rand();
            max[j] = rand();
            val[j] = rand();
            if (min[j] > max[j]) {
                r = rand(); //!\ avoid using a function in macro
                max[j] = HL_MATH_MAX(min[j], r);
            }
        }
        // Test Clip3(min, max, val)
        for (k = 0; k < clip3_count; ++k) {
            clip3s[k].pf((const hl_int128_t*)min, (const hl_int128_t*)max, (const hl_int128_t*)val, (hl_int128_t*)ret);
            for (j = 0; j < 4; ++j) {
                if (ret[j] != HL_MATH_CLIP3(min[j], max[j], val[j])) {
                    HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", clip3s[k].name);
                    return HL_ERROR_TEST_FAILED;
                }
            }
            memset(ret, rand(), sizeof(ret)); // reset ret
        }
        // Test Clip2(min, max, val)
        for (k = 0; k < clip2_count; ++k) {
            clip2s[k].pf((const hl_int128_t*)max, (const hl_int128_t*)val, (hl_int128_t*)ret);
            for (j = 0; j < 4; ++j) {
                if (ret[j] != HL_MATH_CLIP3(0, max[j], val[j])) {
                    HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", clip2s[k].name);
                    return HL_ERROR_TEST_FAILED;
                }
            }
            memset(ret, rand(), sizeof(ret)); // reset ret
        }
        // Test Clip1(val, BitDepth)
        for (k = 0; k < clip1_count; ++k) {
            clip1s[k].pf((const hl_int128_t*)val, (const hl_int128_t*)BitDepth, (hl_int128_t*)ret);
            for (j = 0; j < 4; ++j) {
                if (ret[j] != HL_MATH_CLIP1Y(val[j], BitDepth[0])) {
                    HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", clip1s[k].name);
                    return HL_ERROR_TEST_FAILED;
                }
            }
            memset(ret, rand(), sizeof(ret)); // reset ret
        }
    }

    return HL_ERROR_SUCCESS;
}



//**************************************
// test_math_tap6filter
//**************************************
static HL_ERROR_T test_math_tap6filter()
{
    HL_ALIGN(HL_ALIGN_V) int32_t E[4] = { 4962, 6273, 6833, 6976 };
    HL_ALIGN(HL_ALIGN_V) int32_t F[4] = { 6273, 6833, 6976, 6976 };
    HL_ALIGN(HL_ALIGN_V) int32_t G[4] = { 214, 218, 218, 218 };
    HL_ALIGN(HL_ALIGN_V) int32_t H[4] = { 6976, 6976, 6976, 6976 };
    HL_ALIGN(HL_ALIGN_V) int32_t I[4] = { 6976, 6976, 6976, 6976 };
    HL_ALIGN(HL_ALIGN_V) int32_t J[4] = { 6976, 6976, 6976, 6976 };
    HL_ALIGN(HL_ALIGN_V) int32_t RET[4];
    int32_t i, j, k, count = 1;

    struct func {
        void (*pf)(
            const hl_int128_t* e,
            const hl_int128_t* f,
            const hl_int128_t* g,
            const hl_int128_t* h,
            const hl_int128_t* i,
            const hl_int128_t* j,
            hl_int128_t* ret);
        const char* name;
    };
    struct func pfs[10] = { { hl_math_tap6filter4x1_u32_cpp, "hl_math_tap6filter4x1_u32_cpp" } };

    (E);
    (F);
    (G);
    (H);
    (I);
    (J);
    (RET);

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_math_tap6filter4x1_u32_asm_sse41;
        pfs[count++].name = "hl_math_tap6filter4x1_u32_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_math_tap6filter4x1_u32_intrin_sse41;
        pfs[count++].name = "hl_math_tap6filter4x1_u32_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    for (k = 0; k < count; ++k) {
        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", pfs[k].name);
        for (i = 0; i  < 100; ++i) {
            if ( i != 0) {
                for (j = 0; j < 4; ++j) {
                    E[j] = rand();
                    F[j] = rand();
                    G[j] = rand();
                    H[j] = rand();
                    I[j] = rand();
                    I[j] = rand();
                    J[j] = rand();
                }
            }
            pfs[k].pf(
                (const hl_int128_t*)E,
                (const hl_int128_t*)F,
                (const hl_int128_t*)G,
                (const hl_int128_t*)H,
                (const hl_int128_t*)I,
                (const hl_int128_t*)J,
                (hl_int128_t*)RET);
            for (j = 0; j < 4; ++j) {
                if (RET[j] != HL_MATH_TAP6FILTER(E[j], F[j], G[j], H[j], I[j], J[j])) {
                    HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", "hl_math_tap6filter4x1_u32_intrin_sse41");
                    return HL_ERROR_TEST_FAILED;
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//**************************************
// test_math_addclip_4x4
//**************************************
static HL_ERROR_T test_math_addclip_4x4()
{
    HL_ALIGN(HL_ALIGN_V) int32_t m1[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t m2[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t ret_cpp[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t ret_test[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t max[4];
    int32_t i,j,k,f, count = 1;

    struct func {
        void (*pf)(HL_ALIGNED(16) const int32_t* m1, int32_t m1_stride, HL_ALIGNED(16) const int32_t* m2, int32_t m2_stride,  HL_ALIGNED(16) const int32_t max[4], HL_ALIGNED(16) int32_t* ret, int32_t ret_stride);
        const char* name;
    };
    struct func pfs[10] = { { hl_math_addclip_4x4_cpp, "hl_math_addclip_4x4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_math_addclip_4x4_asm_sse41;
        pfs[count++].name = "hl_math_addclip_4x4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_math_addclip_4x4_intrin_sse41;
        pfs[count++].name = "hl_math_addclip_4x4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    for (k = 0; k < count; ++k) {
        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", pfs[k].name);
    }
    for (i = 0; i < 20; ++i) {
        for (j = 0; j < 4; ++j) {
            if (j == 0) {
                max[0] = max[1] = max[2] = max[3] = rand();
            }
            for (k = 0; k < 4; ++k) {
                m1[j][k] = rand();
                m2[j][k] = rand();
            }
        }
        pfs[0].pf((const int32_t*)m1, 4, (const int32_t*)m2, 4, (const int32_t*)max, (int32_t*)ret_cpp, 4);
        for (k = 1; k < count; ++k) {
            pfs[k].pf((const int32_t*)m1, 4, (const int32_t*)m2, 4, (const int32_t*)max, (int32_t*)ret_test, 4);
            for (j = 0; j < 4; ++j) {
                for (f = 0; f < 4; ++f) {
                    if (ret_cpp[j][f] != ret_test[j][f]) {
                        HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", pfs[k].name);
                        return HL_ERROR_TEST_FAILED;
                    }
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//**************************************
// test_math_transpose_4x4
//**************************************
static HL_ERROR_T test_math_transpose_4x4()
{
#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        int32_t i, j;
        HL_ALIGN(HL_ALIGN_V) int32_t m0[4][4] = { {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} };
        HL_ALIGN(HL_ALIGN_V) int32_t mr[4][4] = { {0, 4, 8, 12}, {1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15} };
        HL_ALIGN(HL_ALIGN_V) int32_t ret[4][4];

        HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", "hl_math_transpose_4x4_asm_sse2");
        hl_math_transpose_4x4_asm_sse2(m0, ret);
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                if (mr[i][j] != ret[i][j]) {
                    HL_DEBUG_ERROR("[TEST MATH] '%s()' NOK", "hl_math_transpose_4x4_asm_sse2");
                    return HL_ERROR_TEST_FAILED;
                }
            }
        }
    }
#endif /* HL_HAVE_X86_ASM */

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_test_math()
{
    HL_ERROR_T err;
    int32_t i;

    struct test {
        HL_ERROR_T (*pf)();
        const char* pf_name;
        const char* desc;
    };
    struct test tests[] = {
        { test_math_all_zeros16, "test_math_all_zeros16", "AllZeros16" },
        { test_math_clips, "test_math_clips", "CLIPS" },
        { test_math_tap6filter, "test_math_tap6filter", "TAP6FILTER" },
        { test_math_addclip_4x4, "test_math_addclip_4x4", "ADDCLIP4x4" },
        { test_math_transpose_4x4, "test_math_transpose_4x4", "TRANSPOSE4x4" },
    };

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        HL_DEBUG_INFO("[TEST MATH] ======== %s =======", tests[i].desc);
        err = tests[i].pf();
        if (err) {
            break;
        }
    }
    if (!err) {
        HL_DEBUG_INFO("[TEST MATH] OK");
    }

    return err;
}
#endif