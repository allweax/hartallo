#include <hartallo/hl_memory.h>
#include <hartallo/hl_cpu.h>
#include <hartallo/hl_debug.h>
#include <hartallo/hl_bits.h>
#include <assert.h>

#if HL_HAVE_X86_INTRIN
#	include <hartallo/intrinsics/x86/hl_memory_x86_intrin.h>
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
extern void hl_memory_copy4x4_asm_sse2(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride);
extern void hl_memory_copy4x4_unaligned_asm_sse2(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride);
#endif /* HL_HAVE_X86_ASM */

//**************************************
// test_memory_copy4x4
//**************************************
static HL_ERROR_T test_memory_copy4x4()
{
    HL_ALIGN(HL_ALIGN_V) int32_t src[4][4] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 }, { 13, 14, 15, 16 } };
    int32_t i, j, count = 1;
    struct func {
        void (*pf)(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride);
        const char* name;
    };
    struct func pfs[10] = { { hl_memory_copy4x4_cpp, "hl_memory_copy4x4_cpp" } };

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_memory_copy4x4_intrin_sse2;
        pfs[count++].name = "hl_memory_copy4x4_intrin_sse2";

        pfs[count].pf = hl_memory_copy4x4_unaligned_intrin_sse2;
        pfs[count++].name = "hl_memory_copy4x4_unaligned_intrin_sse2";
    }
#endif /* HL_HAVE_X86_INTRIN */
#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        pfs[count].pf = hl_memory_copy4x4_asm_sse2;
        pfs[count++].name = "hl_memory_copy4x4_asm_sse2";

        pfs[count].pf = hl_memory_copy4x4_unaligned_asm_sse2;
        pfs[count++].name = "hl_memory_copy4x4_unaligned_asm_sse2";
    }
#endif /* HL_HAVE_X86_ASM */

    for (i = 0; i < count; ++i) {
        HL_DEBUG_INFO("[TEST MEMORY] Checking '%s()'...", pfs[i].name);
    }

    for (i = 0; i < count; ++i) {
        HL_ALIGN(HL_ALIGN_V) int32_t dst[4][4] = { 0 };
        pfs[i].pf((int32_t*)dst, 4, (const int32_t*)src, 4);
        for (j = 0; j < 16; ++j) {
            if(((int32_t*)dst)[j] != j + 1) {
                HL_DEBUG_ERROR("[TEST MEMORY] '%s()' NOK", pfs[i].name);
                return HL_ERROR_TEST_FAILED;
            }
        }
    }

    return HL_ERROR_SUCCESS;
}

//**************************************
// test_memory_blocks
//**************************************
static HL_ERROR_T test_memory_blocks()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    /*int32_t* p_mem_global = (int32_t*)hl_memory_malloc((16 * 16) * 32 * sizeof(int32_t));
    int32_t* pc_mem_mapped;
    int32_t flags = 0;
    hl_size_t i;
    if (!p_mem_global) {
    	err = HL_ERROR_OUTOFMEMMORY;
    	goto bail;
    }

    // map()
    if (!flags) {
    	pc_mem_mapped = p_mem_global;
    	flags = 0x80000000;
    }

    // map()
    i = hl_bits_ctz32_mvs(flags);
    pc_mem_mapped = (p_mem_global + (i << 8));
    flags |= (1 << (i - 1));

    // map()
    i = hl_bits_ctz32_mvs(flags);
    pc_mem_mapped = (p_mem_global + (i << 8));
    flags |= (1 << (i - 1));

    // map()
    i = hl_bits_ctz32_mvs(flags);
    pc_mem_mapped = (p_mem_global + (i << 8));
    flags |= (1 << (i - 1));

    // unmap()
    i = ((pc_mem_mapped - p_mem_global) >> 8);
    flags &= ~(1 << (i - 1));

    // map()
    i = hl_bits_ctz32_mvs(flags);
    pc_mem_mapped = (p_mem_global + (i << 8));
    flags |= (1 << (i - 1));

    // unmap()
    i = ((pc_mem_mapped - p_mem_global) >> 8);
    flags &= ~(1 << (i - 1));

    {
    	HL_ALIGN(HL_ALIGN_V) int32_t b4_0[4][4];
    	HL_ALIGN(HL_ALIGN_V) int32_t b4_1[4][4];
    	HL_ALIGN(HL_ALIGN_V) int32_t b4_2[4][4];
    	HL_ALIGN(HL_ALIGN_V) int32_t b16_0[16][16];
    	HL_ALIGN(HL_ALIGN_V) int32_t b16_1[16][16];
    	HL_ALIGN(HL_ALIGN_V) int32_t b16_2[16][16];
    	extern void hl_memory_setzero4x4_asm_sse2(int32_t* p_mem);
    	extern void hl_memory_setzero16x16_asm_sse2(int32_t* p_mem);

    	hl_memory_setzero4x4_intrin_sse2((int32_t*)b4_0);
    	hl_memory_setzero4x4_cpp((int32_t*)b4_1);
    	hl_memory_setzero4x4_asm_sse2((int32_t*)b4_2);

    	hl_memory_setzero16x16_intrin_sse2((int32_t*)b16_0);
    	hl_memory_setzero16x16_cpp((int32_t*)b16_1);
    	hl_memory_setzero16x16_asm_sse2((int32_t*)b16_2);

    	if(b4_0[0][0] == 0) {
    		int kaka = 0;
    	}
    }


    bail:
    HL_MEMORY_FREE(p_mem_global);*/
    return err;
}

HL_ERROR_T hl_test_memory()
{
    HL_ERROR_T err;

    // Copy 4x4
    err = test_memory_copy4x4();
    if (err) {
        HL_DEBUG_ERROR("[TEST MEMORY] Copy4x4 NOK");
        return err;
    }
    HL_DEBUG_INFO("[TEST MEMORY] ==Copy4x4 OK==");

    // Blocks
    err = test_memory_blocks();
    if (err) {
        HL_DEBUG_ERROR("[TEST MEMORY] Blocks NOK");
        return err;
    }
    HL_DEBUG_INFO("[TEST MEMORY] ==Blocks OK==");

    return err;
}
