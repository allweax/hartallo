#include <hartallo/hl_math.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_bits.h>
#include <assert.h>

#if HL_HAVE_X86_ASM
extern hl_size_t hl_bits_clz32_asm_x86(int32_t n);
extern hl_size_t hl_bits_clz16_asm_x86(int16_t n);
extern hl_size_t hl_bits_ctz32_asm_x86(int32_t n);
extern hl_size_t hl_bits_ctz16_asm_x86(int16_t n);
extern int32_t hl_bits_bswap32_asm_x86(int32_t n);
extern int32_t hl_bits_bittest32_asm_x86(int32_t n, int32_t p);
#endif /* HL_HAVE_X86_ASM */

//**************************************
// test_bits_clz32
//**************************************
static HL_ERROR_T test_bits_clz32()
{
    int32_t i, j, v, count = 1;
    struct func {
        hl_size_t (*pf)(int32_t n);
        const char* name;
    };
    struct func pfs[10] = { { hl_bits_clz32_cpp, "hl_bits_clz32_cpp" } };
#if _MSC_VER >= 1400 && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_clz32_mvs;
    pfs[count++].name = "hl_bits_clz32_mvs";
#endif /* _MSC_VER >= 1400  */
#if _MSC_VER >= 1500 && !HL_DISABLE_INTRIN
    if (hl_cpu_flags_test(kCpuFlagLZCNT)) {
        pfs[count].pf = hl_bits_clz32_mvs_lzcnt;
        pfs[count++].name = "hl_bits_clz32_mvs_lzcnt";
    }
#endif /* _MSC_VER >= 1500 */
#if defined(__GNUC__) && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_clz32_gcc;
    pfs[count++].name = "hl_bits_clz32_gcc";
#endif /* __GNUC__ */
#if HL_HAVE_X86_ASM
    pfs[count].pf = hl_bits_clz32_asm_x86;
    pfs[count++].name = "hl_bits_clz32_asm_x86";
#endif

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST BITS] Checking '%s()'...", pfs[i].name);
    }

    // val is 1 bit set
    for (i = 0; i < 32; ++i) {
        for (j = 0; j < count; ++j) {
            v = (int32_t)pfs[j].pf((1 << i));
            if (pfs[j].pf((1 << i)) != (31 - i)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    // val is all bits set
    for (j = 0; j < count; ++j) {
        if (pfs[j].pf(0xFFFFFFFF) != 0) {
            HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
            return HL_ERROR_TEST_FAILED;
        }
    }

    // compare CPP implementation with others
    for (i = 1; i < count; ++i) {
        for (j = 0; j < 100; ++j) {
            v = rand();
            if (pfs[0].pf(v) != pfs[i].pf(v)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s(%d)' NOK", pfs[i].name, v);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//**************************************
// test_bits_clz16
//**************************************
static HL_ERROR_T test_bits_clz16()
{
    int16_t i, j, v, count = 1;
    struct func {
        hl_size_t (*pf)(int16_t n);
        const char* name;
    };
    struct func pfs[10] = { { hl_bits_clz16_cpp, "hl_bits_clz16_cpp" } };
#if _MSC_VER >= 1400 && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_clz16_mvs;
    pfs[count++].name = "hl_bits_clz16_mvs";
#endif /* _MSC_VER >= 1400  */
#if defined(__GNUC__) && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_clz16_gcc;
    pfs[count++].name = "hl_bits_clz16_gcc";
#endif /* __GNUC__ */
#if HL_HAVE_X86_ASM
    pfs[count].pf = hl_bits_clz16_asm_x86;
    pfs[count++].name = "hl_bits_clz16_asm_x86";
#endif

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST BITS] Checking '%s()'...", pfs[i].name);
    }

    // val is 1 bit set
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < count; ++j) {
            if (pfs[j].pf((1 << i)) != (15 - i)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    // val is all bits set
    for (j = 0; j < count; ++j) {
        if (pfs[j].pf(0xFFFF) != 0) {
            HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
            return HL_ERROR_TEST_FAILED;
        }
    }

    // compare CPP implementation with others
    for (i = 1; i < count; ++i) {
        for (j = 0; j < 100; ++j) {
            v = rand();
            if (pfs[0].pf(v) != pfs[i].pf(v)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s(%d)' NOK", pfs[i].name, v);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//**************************************
// test_bits_ctz32
//**************************************
static HL_ERROR_T test_bits_ctz32()
{
    int32_t i, j, v, count = 1;
    struct func {
        hl_size_t (*pf)(int32_t n);
        const char* name;
    };
    struct func pfs[10] = { { hl_bits_ctz32_cpp, "hl_bits_ctz32_cpp" } };
#if _MSC_VER >= 1400 && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_ctz32_mvs;
    pfs[count++].name = "hl_bits_clz32_mvs";
#endif /* _MSC_VER >= 1400  */
#if defined(__GNUC__) && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_ctz32_gcc;
    pfs[count++].name = "hl_bits_ctz32_gcc";
#endif /* __GNUC__ */
#if HL_HAVE_X86_ASM
    pfs[count].pf = hl_bits_ctz32_asm_x86;
    pfs[count++].name = "hl_bits_ctz32_asm_x86";
#endif

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST BITS] Checking '%s()'...", pfs[i].name);
    }

    // val is 1 bit set
    for (i = 0; i < 32; ++i) {
        for (j = 0; j < count; ++j) {
            if (pfs[j].pf((1 << i)) != i) {
                HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    // val is all bits set
    for (j = 0; j < count; ++j) {
        if (pfs[j].pf(0xFFFFFFFF) != 0) {
            HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
            return HL_ERROR_TEST_FAILED;
        }
    }

    // compare CPP implementation with others
    for (i = 1; i < count; ++i) {
        for (j = 0; j < 100; ++j) {
            v = rand();
            if (pfs[0].pf(v) != pfs[i].pf(v)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s(%d)' NOK", pfs[i].name, v);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//**************************************
// test_bits_ctz16
//**************************************
static HL_ERROR_T test_bits_ctz16()
{
    int16_t i, j, v, count = 1;
    struct func {
        hl_size_t (*pf)(int16_t n);
        const char* name;
    };
    struct func pfs[10] = { { hl_bits_ctz16_cpp, "hl_bits_ctz16_cpp" } };
#if _MSC_VER >= 1400 && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_ctz16_mvs;
    pfs[count++].name = "hl_bits_ctz16_mvs";
#endif /* _MSC_VER >= 1400  */
#if defined(__GNUC__) && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_ctz16_gcc;
    pfs[count++].name = "hl_bits_ctz16_gcc";
#endif /* __GNUC__ */
#if HL_HAVE_X86_ASM
    pfs[count].pf = hl_bits_ctz16_asm_x86;
    pfs[count++].name = "hl_bits_ctz16_asm_x86";
#endif

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST BITS] Checking '%s()'...", pfs[i].name);
    }

    // val is 1 bit set
    for (i = 0; i < 16; ++i) {
        for (j = 0; j < count; ++j) {
            if (pfs[j].pf((1 << i)) != i) {
                HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    // val is all bits set
    for (j = 0; j < count; ++j) {
        if (pfs[j].pf(0xFFFF) != 0) {
            HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
            return HL_ERROR_TEST_FAILED;
        }
    }

    // compare CPP implementation with others
    for (i = 1; i < count; ++i) {
        for (j = 0; j < 100; ++j) {
            v = rand();
            if (pfs[0].pf(v) != pfs[i].pf(v)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s(%d)' NOK", pfs[i].name, v);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//**************************************
// test_bits_bswap32
//**************************************
static HL_ERROR_T test_bits_bswap32()
{
    int32_t i, j, v, count = 1;
    struct func {
        int32_t (*pf)(int32_t n);
        const char* name;
    };
    struct test {
        int32_t val;
        int32_t result;
    };
    struct func pfs[10] = { { hl_bits_bswap32_cpp, "hl_bits_bswap32_cpp" } };
    static const struct test tests[] = {
        { 0x00000000, 0x00000000 },
        { 0x11111111, 0x11111111 },
        { 0x01020304, 0x04030201 },
        { 0x00000001, 0x01000000 },
        { 0x10000000, 0x00000010 },
    };
#if _MSC_VER >= 1400 && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_bswap32_mvs;
    pfs[count++].name = "hl_bits_bswap32_mvs";
#endif /* _MSC_VER >= 1400  */
#if defined(__GNUC__) && !HL_DISABLE_INTRIN
    pfs[count].pf = hl_bits_bswap32_gcc;
    pfs[count++].name = "hl_bits_bswap32_gcc";
#endif /* __GNUC__ */
#if HL_HAVE_X86_ASM
    pfs[count].pf = hl_bits_bswap32_asm_x86;
    pfs[count++].name = "hl_bits_bswap32_asm_x86";
#endif

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST BITS] Checking '%s()'...", pfs[i].name);
    }

    // test "tests"
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        for (j = 0; j < count; ++j) {
            if (pfs[j].pf(tests[i].val) != tests[i].result) {
                HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    // compare CPP implementation with others
    for (i = 1; i < count; ++i) {
        for (j = 0; j < 100; ++j) {
            v = rand();
            if (pfs[0].pf(v) != pfs[i].pf(v)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s(%d)' NOK", pfs[i].name, v);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//**************************************
// test_bits_bittest32
//**************************************
static HL_ERROR_T test_bits_bittest32()
{
    int32_t i, j, v, p, count = 1;
    struct func {
        int32_t (*pf)(int32_t n, int32_t p);
        const char* name;
    };
    struct test {
        int32_t val;
        int32_t p;
        int32_t result;
    };
    struct func pfs[10] = { { hl_bits_bittest32_cpp, "hl_bits_bittest32_cpp" } };
    static const struct test tests[] = {
        { 0x00000000, 15, 0 },
        { 0x11111111, 20, 1 },
        { 0x01020304, 2, 1 },
        { 0x00000001, 4, 0 },
        { 0x80000000, 31, 1 },
    };
#if HL_HAVE_X86_ASM
    pfs[count].pf = hl_bits_bittest32_asm_x86;
    pfs[count++].name = "hl_bits_bittest32_asm_x86";
#endif

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST BITS] Checking '%s()'...", pfs[i].name);
    }

    // test "tests"
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        for (j = 0; j < count; ++j) {
            if (pfs[j].pf(tests[i].val, tests[i].p) != tests[i].result) {
                HL_DEBUG_ERROR("[TEST BITS] '%s()' NOK", pfs[j].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    // compare CPP implementation with others
    for (i = 1; i < count; ++i) {
        for (j = 0; j < 100; ++j) {
            v = rand();
            p = rand();
            if (pfs[0].pf(v, p) != pfs[i].pf(v, p)) {
                HL_DEBUG_ERROR("[TEST BITS] '%s(%d)' NOK", pfs[i].name, v);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_test_bits()
{
    HL_ERROR_T err;
    int32_t i;

    struct test {
        HL_ERROR_T (*pf)();
        const char* pf_name;
        const char* desc;
    };
    struct test tests[] = {
        { test_bits_clz32, "test_bits_clz32", "CLZ32" },
        { test_bits_clz16, "test_bits_clz16", "CLZ16" },
        { test_bits_ctz32, "test_bits_ctz32", "CTZ32" },
        { test_bits_ctz16, "test_bits_ctz16", "CTZ16" },
        { test_bits_bswap32, "test_bits_bswap32", "BSWAP32" },
        { test_bits_bittest32, "test_bits_bittest32", "BITTEST32" },
    };

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        HL_DEBUG_INFO("[TEST BITS] ======== %s =======", tests[i].desc);
        err = tests[i].pf();
        if (err) {
            break;
        }
    }
    if (!err) {
        HL_DEBUG_INFO("[TEST BITS] OK");
    }

    return err;
}
