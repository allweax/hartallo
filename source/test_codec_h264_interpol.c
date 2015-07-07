#if 0
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_types.h>
#include <hartallo/hl_math.h>
#include <hartallo/hl_debug.h>
#include <hartallo/h264/hl_codec_264_interpol.h>

#define TEST_CODEC_H264_INTERPOL_BITDEPTH		8
#define TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH	176
#define TEST_CODEC_H264_INTERPOL_IMAGE_HEIGHT	144
hl_pixel_t __cSL[TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH][TEST_CODEC_H264_INTERPOL_IMAGE_HEIGHT];
static const int32_t __cSL_length = sizeof(__cSL)/sizeof(hl_pixel_t);

#if HL_HAVE_X86_INTRIN
#	include <hartallo/h264/intrinsics/x86/hl_codec_x86_264_interpol_intrin.h>
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
extern void hl_codec_x86_264_interpol_load_samples_asm_sse2(const uint32_t* pc_indices, int32_t i_gap, const hl_pixel_t *cSL, __m128i* ret);
extern void hl_codec_x86_264_tap6filter_vert4_asm_sse41(const uint32_t* pc_indices, int32_t i_indices_stride, const hl_pixel_t* cSL, __m128i* ret);
extern void hl_codec_x86_264_tap6filter_horiz4_asm_sse41(const uint32_t* pc_indices, const hl_pixel_t* cSL, __m128i* ret);
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
#endif /* HL_HAVE_X86_ASM */


//FIXME
/*void kaka(const uint32_t* pc_indices, int32_t i_gap, const hl_pixel_t *cSL, __m128i* ret)
{
	// _mm_store_si128((hl_int128_t*)ret, _mm_set_epi32(0,0,0,0));
}*/
//FIXME: remove
void demba(
    hl_int128_t* h,
    hl_int128_t* s
)
{
    //_mm_store_si128((hl_int128_t*)b, _mm_set_epi32(0,0,0,0));
    //_mm_store_si128((hl_int128_t*)b1, _mm_set_epi32(0,0,0,0));
}

//HL_NAKED
void samba(
    const uint32_t* pc_indices_horiz, // ebp + 8
    const uint32_t* pc_indices_vert, // ebp + 12
    int32_t i_indices_stride, // ebp + 16
    const hl_pixel_t* cSL, // ebp + 20
    HL_ALIGNED(16) int32_t* predPartLXL, // ebp + 24
    HL_ALIGNED(16) const int32_t MaxPixelValueY[4] // ebp + 28
)
{
//	extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_sixteens[4];
//    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_five_hundred_and_twelves[4];
//    extern HL_ALIGN(HL_ALIGN_V) const int32_t __x86_globals_array4_ones[4];
//	//__asm push ebp
//    //__asm mov ebp, esp
//
//	__asm push esi
//	__asm push edi
//	//__asm push ebx//FIXME: do not forget ebx
//	//__asm mov ebx, [ebp + 8]; &pc_indices_vert
//	//__asm mov ebx, [ebx]; pc_indices_vert[0]
//    __asm mov esi, [ebp + 28]; &MaxPixelValueY
//    __asm mov edi, [ebp + 20]; &cSL
//
//	//FIXME: do not forget ebx
//
//	__asm and esp, -16; align stack pointer on 16 bytes
//
//	//__asm shl dword esi, 2; make i_indices_stride in unit of sizeof("int32_t")
//
//	__asm; Local variables: 2x16 = 32 - > s(@32), m(@16)
//	__asm; Parameter addresses: 4x4 = 16
//	__asm sub esp, 48
//
//
//	__asm;;;hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &s);;;
//	__asm mov eax, [ebp + 8]
//	__asm mov [esp], eax
//	__asm mov [esp + 4], edi
//	__asm lea eax, [esp + 32]
//	__asm mov [esp + 8], eax
//	__asm call hl_codec_x86_264_tap6filter_horiz4_intrin_sse41
//
//	__asm;;; s = HL_MATH_CLIP1Y(((s1 + 16) >> 5), BitDepthY);;;
//	__asm movdqa xmm0, [esp + 32]
//	__asm paddd xmm0, [__x86_globals_array4_sixteens]
//	__asm psrad xmm0, 5
//	__asm movdqa [esp + 32], xmm0
//	__asm mov [esp], esi
//	__asm lea eax, [esp + 32]
//	__asm mov [esp + 4], eax
//	__asm mov [esp + 8], eax
//	__asm call hl_math_clip2_4x1_intrin_sse41
//
//	__asm;;;hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices_vert, i_indices_stride, cSL, &m);;;
//	__asm mov eax, [ebp + 12]
//	__asm mov [esp], eax
//	__asm mov eax, [ebp + 16]
//	__asm mov [esp + 4], eax
//	__asm mov [esp + 8], edi
//	__asm lea eax, [esp + 16]
//	__asm mov [esp + 12], eax
//	__asm call hl_codec_x86_264_tap6filter_vert4_intrin_sse41
//
//	__asm;;;m = HL_MATH_CLIP1Y(((m1 + 16) >> 5), BitDepthY);;;
//	__asm movdqa xmm0, [esp + 16]
//	__asm paddd xmm0, [__x86_globals_array4_sixteens]
//	__asm psrad xmm0, 5
//	__asm movdqa [esp + 16], xmm0
//	__asm mov [esp], esi
//	__asm lea eax, [esp + 16]
//	__asm mov [esp + 4], eax
//	__asm mov [esp + 8], eax
//	__asm call hl_math_clip2_4x1_intrin_sse41
//
//	__asm;;;predPartLXL = (m + s + 1) >> 1;;;
//	__asm movdqa xmm0, [esp + 16]
//	__asm paddd xmm0, [__x86_globals_array4_ones]
//	__asm paddd xmm0, [esp + 32]
//	__asm psrad xmm0, 1
//	__asm mov eax, [ebp + 24]
//	__asm movdqa [eax], xmm0
//
//	//FIXME: not correct
//	__asm; restore stack pointer
//	__asm add esp, 48 + 12
//
//	//__asm mov esp, ebp ; before push esi, edi
//	//__asm sub esp, 8; after push esi, edi
//
//	/*
//
//	*/
//	/*
//	__asm;;;hl_codec_x86_264_interpol_load_samples_intrin_sse2(&pc_indices_horiz[2], 1, cSL, &G);;;
//	__asm mov eax, esi
//	__asm add eax, 8
//	__asm mov [esp], eax
//	__asm mov eax, 1
//	__asm mov [esp + 4], eax
//	__asm mov [esp + 8], edi
//	__asm lea eax, [esp + 48]
//	__asm mov [esp + 12], eax
//	__asm call hl_codec_x86_264_interpol_load_samples_intrin_sse2
//
//	__asm;;;hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(pc_indices_horiz, cSL, &b1);;;
//	__asm mov [esp], esi
//	__asm mov [esp + 4], edi
//	__asm lea eax, [esp + 32]
//	__asm mov [esp + 8], eax
//	__asm call hl_codec_x86_264_tap6filter_horiz4_intrin_sse41
//
//	__asm;;;b1 = ((b1 + 16) >> 5);;;
//	__asm movdqa xmm0, [esp + 32]
//	__asm paddd xmm0, [__x86_globals_array4_sixteens]
//	__asm psrad xmm0, 5
//	__asm movdqa [esp + 32], xmm0
//
//	__asm;;;hl_math_clip2_4x1_intrin_sse41((hl_int128_t*)MaxPixelValueY, &b1, &b);;;
//	__asm mov eax, [ebp + 20]
//	__asm mov [esp], eax
//	__asm lea eax, [esp + 32]
//	__asm mov [esp + 4], eax
//	__asm lea eax, [esp + 16]
//	__asm mov [esp + 8], eax
//	__asm call hl_math_clip2_4x1_intrin_sse41
//
//	__asm;;;b = (b + 1 + G) >> 1;;;
//	__asm movdqa xmm0, [esp + 16]
//	__asm paddd xmm0, [__x86_globals_array4_ones]
//	__asm paddd xmm0, [esp + 48]
//	__asm psrad xmm0, 1
//
//	__asm;;;predPartLXL=b;;;
//	__asm mov eax, [ebp + 16]
//	__asm movdqa [eax], xmm0
//	*/
//
//	//__asm pop ebx
//	__asm pop edi
//	__asm pop esi
////	__asm mov esp, ebp
////    __asm pop ebp
//	//__asm ret
//	//int kaka = 0;
}

static void test_codec_h264_interpol_generate_random_indices(uint32_t *pc_indices, const hl_pixel_t *cSL, int32_t cSL_length, int32_t* gap)
{
    int32_t k;
    *gap = rand();
    *gap %= ((cSL_length - 1)/3);
    for (k = 0; k < cSL_length; ++k) {
        pc_indices[k] = rand() % (cSL_length - 1 - ((*gap) * 3));
    }
}

//**************************************
// test_codec_h264_interpol_load_samples
//**************************************
static HL_ERROR_T test_codec_h264_interpol_load_samples()
{
    int32_t i, k, gap, count = 0;
    uint32_t pc_indices[10];
    hl_pixel_t cSL[sizeof(pc_indices)/sizeof(pc_indices[0])] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 }; // samples values equal to indices * 10
    static const int32_t cSL_length = sizeof(cSL)/sizeof(hl_pixel_t);
    __m128i ret;
    struct func {
        void (*pf)(const uint32_t* pc_indices, int32_t i_gap, const hl_pixel_t* cSL, __m128i* ret);
        const char* name;
    };
    struct func pfs[10];

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_codec_x86_264_interpol_load_samples_asm_sse2;
        pfs[count++].name = "hl_codec_x86_264_interpol_load_samples_asm_sse2";
    }
#endif /* HL_HAVE_X86_ASM */

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_codec_x86_264_interpol_load_samples_intrin_sse2;
        pfs[count++].name = "hl_codec_x86_264_interpol_load_samples_intrin_sse2";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count == 0) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < 100; ++i) {
        test_codec_h264_interpol_generate_random_indices(pc_indices, cSL, cSL_length, &gap);
        for (k = 0; k < count; ++k) {
            pfs[k].pf(pc_indices, gap, cSL, &ret);
            if (((int32_t*)&ret)[0] != pc_indices[0]*10 || ((int32_t*)&ret)[1] != pc_indices[gap]*10 || ((int32_t*)&ret)[2] != pc_indices[gap<<1]*10 ||((int32_t*)&ret)[3] != pc_indices[gap*3]*10) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[i].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_tap6filter_vert4
//***********************************************
#if HL_HAVE_X86_INTRIN && HL_HAVE_X86_ASM // tap6filter_vert4_cpp not visible
static HL_ERROR_T test_codec_h264_interpol_tap6filter_vert4()
{
    int32_t i_indices_stride;
    int32_t i;
    int32_t pc_indices[sizeof(__cSL)/sizeof(hl_pixel_t)];
    __m128i ret_intrin, ret_asm;

    if (!hl_cpu_flags_test(kCpuFlagSSE41)) {
        return HL_ERROR_SUCCESS;
    }

    HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", "hl_codec_x86_264_tap6filter_vert4_intrin_sse41");
    HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", "hl_codec_x86_264_tap6filter_vert4_asm_sse41");

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % (__cSL_length >> 3);
        hl_codec_x86_264_tap6filter_vert4_intrin_sse41(pc_indices, i_indices_stride, (hl_pixel_t*)__cSL, &ret_intrin);
        hl_codec_x86_264_tap6filter_vert4_asm_sse41(pc_indices, i_indices_stride, (hl_pixel_t*)__cSL, &ret_asm);
        if (((int32_t*)&ret_intrin)[0] != ((int32_t*)&ret_asm)[0] || ((int32_t*)&ret_intrin)[1] != ((int32_t*)&ret_asm)[1] || ((int32_t*)&ret_intrin)[2] != ((int32_t*)&ret_asm)[2] || ((int32_t*)&ret_intrin)[3] != ((int32_t*)&ret_asm)[3]) {
            HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", "hl_codec_x86_264_tap6filter_vert4_asm_sse41");
            return HL_ERROR_TEST_FAILED;
        }
    }

    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_tap6filter_horiz4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_tap6filter_horiz4()
{
    int32_t i, start;
    int32_t pc_indices[sizeof(__cSL)/sizeof(hl_pixel_t)];
    __m128i ret_intrin, ret_asm;

    if (!hl_cpu_flags_test(kCpuFlagSSE41)) {
        return HL_ERROR_SUCCESS;
    }

    HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", "hl_codec_x86_264_tap6filter_horiz4_intrin_sse41");
    HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", "hl_codec_x86_264_tap6filter_horiz4_asm_sse41");

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 10; ++i) {
        start = rand() % (__cSL_length - 16);
        hl_codec_x86_264_tap6filter_horiz4_intrin_sse41(&pc_indices[start], (hl_pixel_t*)__cSL, &ret_intrin);
        hl_codec_x86_264_tap6filter_horiz4_asm_sse41(&pc_indices[start], (hl_pixel_t*)__cSL, &ret_asm);
        if (((int32_t*)&ret_intrin)[0] != ((int32_t*)&ret_asm)[0] || ((int32_t*)&ret_intrin)[1] != ((int32_t*)&ret_asm)[1] || ((int32_t*)&ret_intrin)[2] != ((int32_t*)&ret_asm)[2] || ((int32_t*)&ret_intrin)[3] != ((int32_t*)&ret_asm)[3]) {
            HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", "hl_codec_x86_264_tap6filter_horiz4_asm_sse41");
            return HL_ERROR_TEST_FAILED;
        }
    }

    return HL_ERROR_SUCCESS;
}
#endif /* HL_HAVE_X86_INTRIN || HL_HAVE_X86_ASM */

//***********************************************
// test_codec_h264_interpol_luma00_horiz4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma00_horiz4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start;
    struct func {
        void (*pf)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma00_horiz4_cpp, "hl_codec_264_interpol_luma00_horiz4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma00_horiz4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma00_horiz4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma00_horiz4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 10; ++i) {
        start = rand() % (__cSL_length - 16);
        pfs[0].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_cpp);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_test);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_luma01_vert4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma01_vert4()
{
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, i_indices_stride, k, count = 1;
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma01_vert4_cpp, "hl_codec_264_interpol_luma01_vert4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma01_vert4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma01_vert4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma01_vert4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma01_vert4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % (__cSL_length >> 3);
        pfs[0].pf(pc_indices_vert, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(pc_indices_vert, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }
    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_luma02_vert4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma02_vert4()
{
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, i_indices_stride, k, count = 1;
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma02_vert4_cpp, "hl_codec_264_interpol_luma02_vert4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma02_vert4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma02_vert4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma02_vert4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma02_vert4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % (__cSL_length >> 3);
        pfs[0].pf(pc_indices_vert, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(pc_indices_vert, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }
    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma03_vert4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma03_vert4()
{
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, i_indices_stride, k, count = 1;
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma03_vert4_cpp, "hl_codec_264_interpol_luma03_vert4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma03_vert4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma03_vert4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma03_vert4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma03_vert4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % (__cSL_length >> 3);
        pfs[0].pf(pc_indices_vert, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(pc_indices_vert, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }
    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma10_horiz4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma10_horiz4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma10_horiz4_cpp, "hl_codec_264_interpol_luma10_horiz4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma10_horiz4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma10_horiz4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma10_horiz4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 10; ++i) {
        start = rand() % (__cSL_length - 16);
        pfs[0].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_luma11_diag4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma11_diag4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start, i_indices_stride;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[11] = { { hl_codec_264_interpol_luma11_diag4_cpp, "hl_codec_264_interpol_luma11_diag4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma11_diag4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma11_diag4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma11_diag4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma11_diag4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 11; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        start = rand() % (__cSL_length - (i_indices_stride << 3));
        pfs[0].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma12_vert4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma12_vert4()
{
    uint32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    uint32_t* pc_indices_verts[6][4];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, i_indices_stride, k, count = 1, m;
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma12_vert4_cpp, "hl_codec_264_interpol_luma12_vert4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma12_vert4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma12_vert4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma12_vert4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma12_vert4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        m = rand() % 9;
        for (k = 0; k < 6*4; ++k) {
            ((uint32_t**)pc_indices_verts)[k] = &pc_indices_vert[m + (rand()%(k+1))];
        }
        pfs[0].pf((const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf((const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }
    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma13_diag4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma13_diag4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start, i_indices_stride;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[13] = { { hl_codec_264_interpol_luma13_diag4_cpp, "hl_codec_264_interpol_luma13_diag4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma13_diag4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma13_diag4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma13_diag4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma13_diag4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 13; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        start = rand() % (__cSL_length - (i_indices_stride << 3));
        pfs[0].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma20_horiz4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma20_horiz4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[20] = { { hl_codec_264_interpol_luma20_horiz4_cpp, "hl_codec_264_interpol_luma20_horiz4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma20_horiz4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma20_horiz4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma20_horiz4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 20; ++i) {
        start = rand() % (__cSL_length - 16);
        pfs[0].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma21_diag4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma21_diag4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start, m, i_indices_stride;
    uint32_t* pc_indices_verts[6][4];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[21] = { { hl_codec_264_interpol_luma21_diag4_cpp, "hl_codec_264_interpol_luma21_diag4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma21_diag4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma21_diag4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma21_diag4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma21_diag4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 51; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        m = rand() % 9;
        for (k = 0; k < 6*4; ++k) {
            ((uint32_t**)pc_indices_verts)[k] = &pc_indices_vert[m + (rand()%(k+1))];
        }
        start = rand() % (__cSL_length - (i_indices_stride << 3));
        pfs[0].pf(&pc_indices_horiz[start], (const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_horiz[start], (const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma22_vert4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma22_vert4()
{
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, i_indices_stride, k, m, count = 1;
    uint32_t* pc_indices_verts[6][4];
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma22_vert4_cpp, "hl_codec_264_interpol_luma22_vert4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma22_vert4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma22_vert4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma22_vert4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma22_vert4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % (__cSL_length >> 3);
        m = rand() % 9;
        for (k = 0; k < 6*4; ++k) {
            ((uint32_t**)pc_indices_verts)[k] = &pc_indices_vert[m + (rand()%(k+1))];
        }
        pfs[0].pf((const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf((const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }
    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma23_diag4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma23_diag4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start, m, i_indices_stride;
    uint32_t* pc_indices_verts[6][4];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_horiz, const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[23] = { { hl_codec_264_interpol_luma23_diag4_cpp, "hl_codec_264_interpol_luma23_diag4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma23_diag4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma23_diag4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma23_diag4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma23_diag4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 23; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        m = rand() % 9;
        for (k = 0; k < 6*4; ++k) {
            ((uint32_t**)pc_indices_verts)[k] = &pc_indices_vert[m + (rand()%(k+1))];
        }
        start = rand() % (__cSL_length - (i_indices_stride << 3));
        pfs[0].pf(&pc_indices_horiz[start], (const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_horiz[start], (const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma30_horiz4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma30_horiz4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_horiz, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[30] = { { hl_codec_264_interpol_luma30_horiz4_cpp, "hl_codec_264_interpol_luma30_horiz4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma30_horiz4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma30_horiz4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma30_horiz4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 30; ++i) {
        start = rand() % (__cSL_length - 16);
        pfs[0].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_horiz[start], (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_luma31_diag4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma31_diag4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start, i_indices_stride;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[31] = { { hl_codec_264_interpol_luma31_diag4_cpp, "hl_codec_264_interpol_luma31_diag4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma31_diag4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma31_diag4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma31_diag4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma31_diag4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 31; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        start = rand() % (__cSL_length - (i_indices_stride << 3));
        pfs[0].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//***********************************************
// test_codec_h264_interpol_luma32_vert4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma32_vert4()
{
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, i_indices_stride, k, m, count = 1;
    uint32_t* pc_indices_verts[7][4];
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert[6], int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[10] = { { hl_codec_264_interpol_luma32_vert4_cpp, "hl_codec_264_interpol_luma32_vert4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma32_vert4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma32_vert4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma32_vert4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma32_vert4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 100; ++i) {
        i_indices_stride = rand() % (__cSL_length >> 3);
        m = rand() % 8;
        for (k = 0; k < 7*4; ++k) {
            ((uint32_t**)pc_indices_verts)[k] = &pc_indices_vert[m + (rand()%(k+1))];
        }
        pfs[0].pf((const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf((const uint32_t **)pc_indices_verts, i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }
    return HL_ERROR_SUCCESS;
}


//***********************************************
// test_codec_h264_interpol_luma33_diag4
//***********************************************
static HL_ERROR_T test_codec_h264_interpol_luma33_diag4()
{
    int32_t pc_indices_horiz[sizeof(__cSL)/sizeof(hl_pixel_t)];
    int32_t pc_indices_vert[sizeof(__cSL)/sizeof(hl_pixel_t)];
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_cpp[4] = { 0 };
    HL_ALIGN(HL_ALIGN_V) int32_t predPartLXL_test[4] = { 0 };
    int32_t i, k, count = 1, start, i_indices_stride;
    HL_ALIGN(HL_ALIGN_V) int32_t MaxPixelValueY[4] = { (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1, (1 << TEST_CODEC_H264_INTERPOL_BITDEPTH)-1 };
    struct func {
        void (*pf)(const uint32_t* pc_indices_vert, const uint32_t* pc_indices_horiz, int32_t i_indices_stride, const hl_pixel_t* cSL, HL_ALIGNED(16) int32_t* predPartLXL, HL_ALIGNED(16) const int32_t MaxPixelValueY[4]);
        const char* name;
    };
    struct func pfs[33] = { { hl_codec_264_interpol_luma33_diag4_cpp, "hl_codec_264_interpol_luma33_diag4_cpp" } };

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma33_diag4_asm_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma33_diag4_asm_sse41";
    }
#endif /* HL_HAVE_X86_ASM */
#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        pfs[count].pf = hl_codec_x86_264_interpol_luma33_diag4_intrin_sse41;
        pfs[count++].name = "hl_codec_x86_264_interpol_luma33_diag4_intrin_sse41";
    }
#endif /* HL_HAVE_X86_INTRIN */

    if (count < 2) {
        return HL_ERROR_SUCCESS;
    }

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < __cSL_length; ++i) {
        pc_indices_vert[i] = rand() % ((__cSL_length - 1) / 6);
        pc_indices_horiz[i] = rand() % ((__cSL_length - 1) / 6);
    }

    for (i = 0; i < 33; ++i) {
        i_indices_stride = rand() % TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH;
        start = rand() % (__cSL_length - (i_indices_stride << 3));
        pfs[0].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_cpp, MaxPixelValueY);
        for (k = 1; k < count; ++k) {
            pfs[k].pf(&pc_indices_vert[start], &pc_indices_horiz[start], i_indices_stride, (hl_pixel_t*)__cSL, predPartLXL_test, MaxPixelValueY);
            if (((int32_t*)&predPartLXL_cpp)[0] != ((int32_t*)&predPartLXL_test)[0] || ((int32_t*)&predPartLXL_cpp)[1] != ((int32_t*)&predPartLXL_test)[1] || ((int32_t*)&predPartLXL_cpp)[2] != ((int32_t*)&predPartLXL_test)[2] || ((int32_t*)&predPartLXL_cpp)[3] != ((int32_t*)&predPartLXL_test)[3]) {
                HL_DEBUG_ERROR("[TEST H264 INTERPOL] '%s()' NOK", pfs[k].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_test_codec_h264_interpol()
{
    HL_ERROR_T err;
    int32_t i, j;

    struct test {
        HL_ERROR_T (*pf)();
        const char* pf_name;
        const char* desc;
    };
    struct test tests[] = {
        { test_codec_h264_interpol_load_samples, "test_codec_h264_interpol_load_samples", "INTERPOL_LOAD_SAMPLES" },
#if HL_HAVE_X86_INTRIN && HL_HAVE_X86_ASM
        { test_codec_h264_interpol_tap6filter_horiz4, "test_codec_h264_interpol_tap6filter_horiz4", "INTERPOL_TAP6FILTER_HORIZ4" },
        { test_codec_h264_interpol_tap6filter_vert4, "test_codec_h264_interpol_tap6filter_vert4", "INTERPOL_TAP6FILTER_VERT4" },
#endif
        { test_codec_h264_interpol_luma00_horiz4, "test_codec_h264_interpol_luma00_horiz4", "INTERPOL_LUMA00_HORIZ4" },
        { test_codec_h264_interpol_luma01_vert4, "test_codec_h264_interpol_luma01_vert4", "INTERPOL_LUMA01_VERT4" },
        { test_codec_h264_interpol_luma02_vert4, "test_codec_h264_interpol_luma02_vert4", "INTERPOL_LUMA02_VERT4" },
        { test_codec_h264_interpol_luma03_vert4, "test_codec_h264_interpol_luma03_vert4", "INTERPOL_LUMA03_VERT4" },
        { test_codec_h264_interpol_luma10_horiz4, "test_codec_h264_interpol_luma10_horiz4", "INTERPOL_LUMA10_HORIZ4" },
        { test_codec_h264_interpol_luma11_diag4, "test_codec_h264_interpol_luma11_diag4", "INTERPOL_LUMA11_DIAG4" },
        { test_codec_h264_interpol_luma12_vert4, "test_codec_h264_interpol_luma12_vert4", "INTERPOL_LUMA12_VERT4" },
        { test_codec_h264_interpol_luma13_diag4, "test_codec_h264_interpol_luma13_diag4", "INTERPOL_LUMA13_DIAG4" },
        { test_codec_h264_interpol_luma20_horiz4, "test_codec_h264_interpol_luma20_horiz4", "INTERPOL_LUMA20_HORIZ4" },
        { test_codec_h264_interpol_luma21_diag4, "test_codec_h264_interpol_luma21_diag4", "INTERPOL_LUMA21_DIAG4" },
        { test_codec_h264_interpol_luma22_vert4, "test_codec_h264_interpol_luma22_vert4", "INTERPOL_LUMA22_VERT4" },
        { test_codec_h264_interpol_luma23_diag4, "test_codec_h264_interpol_luma23_diag4", "INTERPOL_LUMA23_DIAG4" },
        { test_codec_h264_interpol_luma30_horiz4, "test_codec_h264_interpol_luma30_horiz4", "INTERPOL_LUMA30_HORIZ4" },
        { test_codec_h264_interpol_luma31_diag4, "test_codec_h264_interpol_luma31_diag4", "INTERPOL_LUMA31_DIAG4" },
        { test_codec_h264_interpol_luma32_vert4, "test_codec_h264_interpol_luma32_vert4", "INTERPOL_LUMA32_VERT4" },
        { test_codec_h264_interpol_luma33_diag4, "test_codec_h264_interpol_luma33_diag4", "INTERPOL_LUMA33_DIAG4" },
    };

    for (i = 0; i < TEST_CODEC_H264_INTERPOL_IMAGE_WIDTH; ++i) {
        for (j = 0; j < TEST_CODEC_H264_INTERPOL_IMAGE_HEIGHT; ++j) {
            __cSL[i][j] = (rand() % ((rand() & 1) ? 34 : INT_MAX)); // 34 used to make sure we will test both clipping and non-clipping
        }
    }
    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        HL_DEBUG_INFO("\n[TEST H264 INTERPOL] ======== %s =======", tests[i].desc);
        err = tests[i].pf();
        if (err) {
            break;
        }
    }

    if (!err) {
        HL_DEBUG_INFO("[TEST H264 INTERPOL] OK");
    }

    return err;
}

#endif