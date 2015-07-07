#if 0
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_types.h>
#include <hartallo/hl_math.h>
#include <hartallo/hl_debug.h>

#if HL_HAVE_X86_INTRIN
#	include <hartallo/h264/intrinsics/x86/hl_codec_x86_264_transf_intrin.h>
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_INTRIN && HL_HAVE_X86_ASM
//***********************************************
// test_codec_h264_transf_inverse_residual4x4
//***********************************************
static HL_ERROR_T test_codec_h264_transf_inverse_residual4x4()
{
    HL_ALIGN(HL_ALIGN_V) int32_t d[4][4] = { {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} };
    HL_ALIGN(HL_ALIGN_V) int32_t r_intrin[4][4];
    HL_ALIGN(HL_ALIGN_V) int32_t r_asm[4][4];
    int32_t i, j, k;
    extern void hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2(int32_t bitDepth, HL_ALIGNED(16) int32_t d[4][4], HL_ALIGNED(16) int32_t r[4][4]);

    HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", "hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2");
    HL_DEBUG_INFO("[TEST MATH] Checking '%s()'...", "hl_codec_x86_264_transf_inverse_residual4x4_intrin_sse2");

    for (k = 0; k < 20; ++k) {
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                //FIXME: d[i][j] = rand();
            }
        }
        hl_codec_x86_264_transf_inverse_residual4x4_intrin_sse2(8, d, r_intrin);
        hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2(8, d, r_asm);
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                if (r_intrin[i][j] != r_asm[i][j]) {
                    HL_DEBUG_ERROR("[TEST H264 TRANSF] '%s()' NOK", "hl_codec_x86_264_transf_inverse_residual4x4_asm_sse2");
                    return HL_ERROR_TEST_FAILED;
                }
            }
        }
    }

    return HL_ERROR_SUCCESS;
}
#endif /*  HL_HAVE_X86_INTRIN && HL_HAVE_X86_ASM */

HL_ERROR_T hl_test_codec_h264_transf()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t i;

    struct test {
        HL_ERROR_T (*pf)();
        const char* pf_name;
        const char* desc;
    };
    struct test tests[] = {
#if HL_HAVE_X86_INTRIN && HL_HAVE_X86_ASM
        { test_codec_h264_transf_inverse_residual4x4, "test_codec_h264_transf_inverse_residual4x4", "TRANSF_INVERSE_RESIDUAL4x4" },
#endif //  HL_HAVE_X86_INTRIN && HL_HAVE_X86_ASM
    };

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        HL_DEBUG_INFO("\n[TEST H264 TRANSF] ======== %s =======", tests[i].desc);
        err = tests[i].pf();
        if (err) {
            break;
        }
    }

    if (!err) {
        HL_DEBUG_INFO("[TEST H264 TRANSF] OK");
    }

    return err;
}
#endif