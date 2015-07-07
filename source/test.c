#include <hartallo/hl_api.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_debug.h>
#include <assert.h>
#if HL_CPU_TYPE_X86
#	include "hl_x86_globals.c" /* global data */
#endif /* HL_HAVE_X86_INTRIN || HL_HAVE_X86_ASM */

#if HL_UNDER_WINDOWS
#	include <Windows.h>
#endif

#if !defined(HL_TEST_CPUID)
#define HL_TEST_CPUID	0
#endif
#if !defined(HL_TEST_BITS)
#define HL_TEST_BITS	0
#endif
#if !defined(HL_TEST_MATH)
#define HL_TEST_MATH	0
#endif
#if !defined(HL_TEST_MEMORY)
#define HL_TEST_MEMORY	0
#endif
#if !defined(HL_TEST_PARSER)
#define HL_TEST_PARSER	0
#endif
#if !defined(HL_TEST_ASYNCTASK)
#define HL_TEST_ASYNCTASK	0
#endif
#if !defined(HL_TEST_CODEC_H264_PEL)
#define HL_TEST_CODEC_H264_PEL 0
#endif
#if !defined(HL_TEST_CODEC_H264_INTERPOL)
#define HL_TEST_CODEC_H264_INTERPOL 0
#endif
#if !defined(HL_TEST_CODEC_H264_TRANSF)
#define HL_TEST_CODEC_H264_TRANSF 0
#endif
#if !defined(HL_TEST_ENCODER)
#define HL_TEST_ENCODER	0
#endif
#if !defined(HL_TEST_DECODER)
#define HL_TEST_DECODER	1
#endif
#if !defined(HL_TEST_CONFORMANCE)
#define HL_TEST_CONFORMANCE	0
#endif

#if HL_TEST_BITS
extern HL_ERROR_T hl_test_bits();
#endif

#if HL_TEST_CPUID
extern HL_ERROR_T hl_test_cpuid();
#endif

#if HL_TEST_MATH
extern HL_ERROR_T hl_test_math();
#endif

#if HL_TEST_MEMORY
extern HL_ERROR_T hl_test_memory();
#endif

#if HL_TEST_PARSER
extern HL_ERROR_T hl_test_parser();
#endif

#if HL_TEST_ASYNCTASK
extern HL_ERROR_T hl_test_asynctask();
#endif

#if HL_TEST_CODEC_H264_PEL
extern HL_ERROR_T hl_test_codec_h264_pel();
#endif

#if HL_TEST_CODEC_H264_INTERPOL
extern HL_ERROR_T hl_test_codec_h264_interpol();
#endif

#if HL_TEST_CODEC_H264_TRANSF
extern HL_ERROR_T hl_test_codec_h264_transf();
#endif

#if HL_TEST_ENCODER
extern HL_ERROR_T hl_test_encoder();
#endif

#if HL_TEST_DECODER
extern HL_ERROR_T hl_test_decoder();
#endif

#if HL_TEST_CONFORMANCE
extern HL_ERROR_T hl_test_conformance();
#endif

int main()
{
    HL_ERROR_T err;
    int32_t i;

    struct test {
        HL_ERROR_T (*pf)();
        const char* pf_name;
        const char* desc;
    };
    struct test tests[] = {
#if HL_TEST_CPUID
        { hl_test_cpuid, "hl_test_cpuid", "Test CPUID" },
#endif

#if HL_TEST_BITS
        { hl_test_bits, "hl_test_bits", "Test BITS" },
#endif

#if HL_TEST_MATH
        { hl_test_math, "hl_test_math", "Test MATH" },
#endif

#if HL_TEST_MEMORY
        { hl_test_memory, "hl_test_memory", "Test MEMORY" },
#endif

#if HL_TEST_PARSER
        { hl_test_parser, "hl_test_parser", "Test PARSER" },
#endif

#if HL_TEST_ASYNCTASK
        { hl_test_asynctask, "hl_test_asynctask", "Test ASYNCTASK" },
#endif

#if HL_TEST_CODEC_H264_PEL
        { hl_test_codec_h264_pel, "hl_test_codec_h264_pel", "Test H.264 PEL" },
#endif

#if HL_TEST_CODEC_H264_INTERPOL
        { hl_test_codec_h264_interpol, "hl_test_codec_h264_interpol", "Test H.264 INTERPOL" },
#endif

#if HL_TEST_CODEC_H264_TRANSF
        { hl_test_codec_h264_transf, "hl_test_codec_h264_transf", "Test H.264 TRANSF" },
#endif

#if HL_TEST_ENCODER
        { hl_test_encoder, "hl_test_encoder", "Test ENCODER" },
#endif

#if HL_TEST_DECODER
        { hl_test_decoder, "hl_test_decoder", "Test DECODER" },
#endif

#if HL_TEST_CONFORMANCE
        { hl_test_conformance, "hl_test_conformance", "Test CONFORMANCE" },
#endif
    };

    err = hl_debug_set_level(HL_DEBUG_LEVEL_TALKATIVE);
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }
    err = hl_engine_set_cpu_flags(kCpuFlagAll); // FIXME
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }
    err = hl_engine_init();
    if (err != HL_ERROR_SUCCESS) {
        assert(0);
        goto bail;
    }

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        HL_DEBUG_INFO("======== %s =======", tests[i].desc);
        err = tests[i].pf();
        if (err) {
            break;
        }
    }

bail:
    getchar();
    return 0;
}
