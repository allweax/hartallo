#include "hartallo/hl_memory.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_debug.h"

#include "hartallo/intrinsics/x86/hl_memory_x86_intrin.h"

// #include <malloc.h>

#if HL_UNDER_WINDOWS
#	include <Windows.h>
#endif

// FIXME: memcpy_amd


// SIMD versions requires memory to be aligned.
void (*hl_memory_set)(HL_ALIGNED(16)int32_t *p_mem, hl_size_t count, int32_t val) = hl_memory_set_cpp;
void (*hl_memory_copy4x4)(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride) = hl_memory_copy4x4_cpp;
void (*hl_memory_copy4x4_unaligned)(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride) = hl_memory_copy4x4_cpp;
void (*hl_memory_copy4x4_u32_to_u8)(uint8_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const uint32_t *pc_src, hl_size_t src_stride) = hl_memory_copy4x4_u32_to_u8_cpp;
void (*hl_memory_copy4x4_u32_to_u8_stride4x4)(uint8_t *p_dst, HL_ALIGNED(16) const uint32_t *pc_src) = hl_memory_copy4x4_u32_to_u8_stride4x4_cpp;
void (*hl_memory_copy16x16_u8_to_u32_stride16x16)(HL_ALIGNED(16) uint32_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src) = hl_memory_copy16x16_u8_to_u32_stride16x16_cpp;
void (*hl_memory_copy16x16_u8_to_u32_stride16x4)(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/) = hl_memory_copy16x16_u8_to_u32_stride16x4_cpp;
void (*hl_memory_copy16x16_u8_stride16x4)(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/) = hl_memory_copy16x16_u8_stride16x4_cpp;
void (*hl_memory_copy4x4_u8_to_u32_stride16x16)(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src) = hl_memory_copy4x4_u8_to_u32_stride16x16_cpp;
void (*hl_memory_copy4x4_u8_to_u32_stride16x4)(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src) = hl_memory_copy4x4_u8_to_u32_stride16x4_cpp;
void (*hl_memory_copy4x4_u8_stride16x4)(uint8_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src) = hl_memory_copy4x4_u8_stride16x4_cpp;
void (*hl_memory_setzero4x4)(int32_t* p_mem) = hl_memory_setzero4x4_cpp;
void (*hl_memory_setzero16x16)(int32_t* p_mem) = hl_memory_setzero16x16_cpp;

extern const hl_object_def_t *hl_memory_blocks_def_t;

HL_ERROR_T hl_memory_blocks_create(hl_memory_blocks_t** pp_blocks)
{
    int32_t* p_memory;
    if (!pp_blocks) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    p_memory = hl_memory_malloc((16 * 16) * 32 * sizeof(int32_t)); // enough memory to hold 32 16x16 blocks
    if (!p_memory) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    *pp_blocks = hl_object_create(hl_memory_blocks_def_t);
    if (!*pp_blocks) {
        HL_SAFE_FREE(p_memory);
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_blocks)->p_memory = p_memory;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_memory_init_funcs()
{
    HL_DEBUG_INFO("Initializing memory functions");

    hl_memory_set = hl_memory_set_cpp;
    hl_memory_copy4x4 = hl_memory_copy4x4_cpp;
    hl_memory_copy4x4_unaligned = hl_memory_copy4x4_cpp;
    hl_memory_copy4x4_u32_to_u8 = hl_memory_copy4x4_u32_to_u8_cpp;
    hl_memory_copy4x4_u32_to_u8_stride4x4 = hl_memory_copy4x4_u32_to_u8_stride4x4_cpp;
    hl_memory_copy16x16_u8_to_u32_stride16x16 = hl_memory_copy16x16_u8_to_u32_stride16x16_cpp;
    hl_memory_copy16x16_u8_to_u32_stride16x4 = hl_memory_copy16x16_u8_to_u32_stride16x4_cpp;
    hl_memory_copy16x16_u8_stride16x4 = hl_memory_copy16x16_u8_stride16x4_cpp;
    hl_memory_copy4x4_u8_to_u32_stride16x16 = hl_memory_copy4x4_u8_to_u32_stride16x16_cpp;
    hl_memory_copy4x4_u8_to_u32_stride16x4 = hl_memory_copy4x4_u8_to_u32_stride16x4_cpp;
    hl_memory_copy4x4_u8_stride16x4 = hl_memory_copy4x4_u8_stride16x4_cpp;
    hl_memory_setzero4x4 = hl_memory_setzero4x4_cpp;
    hl_memory_setzero16x16 = hl_memory_setzero16x16_cpp;

#if HL_HAVE_X86_INTRIN
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        hl_memory_copy4x4 = hl_memory_copy4x4_intrin_sse2;
        hl_memory_copy4x4_unaligned = hl_memory_copy4x4_unaligned_intrin_sse2;
        hl_memory_setzero4x4 = hl_memory_setzero4x4_intrin_sse2;
        hl_memory_setzero16x16 = hl_memory_setzero16x16_intrin_sse2;
        hl_memory_copy16x16_u8_to_u32_stride16x16 = hl_memory_copy16x16_u8_to_u32_stride16x16_intrin_sse2;
        hl_memory_copy16x16_u8_to_u32_stride16x4 = hl_memory_copy16x16_u8_to_u32_stride16x4_intrin_sse2;
        hl_memory_copy16x16_u8_stride16x4 = hl_memory_copy16x16_u8_stride16x4_intrin_sse2;
        hl_memory_copy4x4_u8_to_u32_stride16x16 = hl_memory_copy4x4_u8_to_u32_stride16x16_intrin_sse2;
        hl_memory_copy4x4_u8_to_u32_stride16x4 = hl_memory_copy4x4_u8_to_u32_stride16x4_intrin_sse2;
        hl_memory_copy4x4_u8_stride16x4 = hl_memory_copy4x4_u8_stride16x4_intrin_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        hl_memory_copy4x4_u32_to_u8 = hl_memory_copy4x4_u32_to_u8_intrin_sse41;
        hl_memory_copy4x4_u32_to_u8_stride4x4 = hl_memory_copy4x4_u32_to_u8_stride4x4_intrin_sse41;
    }
#endif /* HL_HAVE_X86_INTRIN */

#if HL_HAVE_X86_ASM
    if (hl_cpu_flags_test(kCpuFlagSSE2)) {
        extern void hl_memory_copy4x4_asm_sse2(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride);
        extern void hl_memory_copy4x4_unaligned_asm_sse2(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride);
        extern void hl_memory_copy16x16_u8_to_u32_stride16x16_asm_sse2(HL_ALIGNED(16) uint32_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src);
        extern void hl_memory_copy16x16_u8_to_u32_stride16x4_asm_sse2(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/);
        extern void hl_memory_copy16x16_u8_stride16x4_asm_sse2(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/);
        extern void hl_memory_copy4x4_u8_to_u32_stride16x16_asm_sse2(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src);
        extern void hl_memory_copy4x4_u8_to_u32_stride16x4_asm_sse2(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, const uint8_t *pc_src/*stride=4*/);
        extern void hl_memory_copy4x4_u8_stride16x4_asm_sse2(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/);
        extern void hl_memory_setzero4x4_asm_sse2(int32_t* p_mem);
        extern void hl_memory_setzero16x16_asm_sse2(int32_t* p_mem);
        hl_memory_copy4x4 = hl_memory_copy4x4_asm_sse2;
        hl_memory_copy4x4_unaligned = hl_memory_copy4x4_unaligned_asm_sse2;
        hl_memory_copy16x16_u8_to_u32_stride16x16 = hl_memory_copy16x16_u8_to_u32_stride16x16_asm_sse2;
        hl_memory_copy16x16_u8_to_u32_stride16x4 = hl_memory_copy16x16_u8_to_u32_stride16x4_asm_sse2;
        hl_memory_copy16x16_u8_stride16x4 = hl_memory_copy16x16_u8_stride16x4_asm_sse2;
        hl_memory_copy4x4_u8_to_u32_stride16x16 = hl_memory_copy4x4_u8_to_u32_stride16x16_asm_sse2;
        hl_memory_copy4x4_u8_to_u32_stride16x4 = hl_memory_copy4x4_u8_to_u32_stride16x4_asm_sse2;
        hl_memory_copy4x4_u8_stride16x4 = hl_memory_copy4x4_u8_stride16x4_asm_sse2;
        hl_memory_setzero4x4 = hl_memory_setzero4x4_asm_sse2;
        hl_memory_setzero16x16 = hl_memory_setzero16x16_asm_sse2;
    }
    if (hl_cpu_flags_test(kCpuFlagSSE41)) {
        extern void hl_memory_copy4x4_u32_to_u8_asm_sse41(uint8_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const uint32_t *pc_src, hl_size_t src_stride);
        extern void hl_memory_copy4x4_u32_to_u8_stride4x4_asm_sse41(uint8_t *p_dst, HL_ALIGNED(16) const uint32_t *pc_src);
        hl_memory_copy4x4_u32_to_u8 = hl_memory_copy4x4_u32_to_u8_asm_sse41;
        hl_memory_copy4x4_u32_to_u8_stride4x4 = hl_memory_copy4x4_u32_to_u8_stride4x4_asm_sse41;
    }
#endif /* HL_HAVE_X86_ASM */
    return HL_ERROR_SUCCESS;
}

void* hl_memory_malloc(hl_size_t size)
{
    return hl_memory_malloc_aligned(size, HL_ALIGN_V);
}

void* hl_memory_malloc_aligned(hl_size_t size, hl_size_t alignment)
{
#if defined(_MSC_VER)
    void *ret = _aligned_malloc(size, alignment);
    if (!ret) {
        HL_DEBUG_ERROR("_aligned_malloc(%d, %d) failed", size, alignment);
    }
#elif defined(__GNUC__) || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 || HAVE_POSIX_MEMALIGN
    void *ret = HL_NULL;
    int err = posix_memalign(&ret, alignment, size);
    if (err) {
        HL_DEBUG_ERROR("posix_memalign(%d, %d) failed with error code = %d", alignment, size, err);
    }
#elif _ISOC11_SOURCE
    void *ret = aligned_alloc(alignment, size);
    if (!ret) {
        HL_DEBUG_ERROR("aligned_alloc(%d, %d) failed", alignment, size);
    }
#elif 0
    // http://en.wikipedia.org/wiki/Data_structure_alignment
    void* ret = malloc(size + alignment);
    if (ret) {
        long pad = ((~(long)ret) % alignment) + 1;
        ret = ((uint8_t*)ret) + pad; // pad
        ((uint8_t*)ret)[-1] = pad; // store the pad for later use
    }
    else {
        HL_DEBUG_ERROR("malloc(%d) failed", (size + alignment));
    }
#endif
    return ret;
}

void* hl_memory_realloc (void* ptr, hl_size_t size)
{
    return hl_memory_realloc_aligned(ptr, size, HL_ALIGN_V);
}

void* hl_memory_realloc_aligned(void * ptr, hl_size_t size, hl_size_t alignment)
{
#if defined(_MSC_VER)
    void *ret = _aligned_realloc(ptr, size, alignment);
    if (!ret) {
        HL_DEBUG_ERROR("_aligned_realloc(%d, %d) failed", size, alignment);
    }
#elif defined(__GNUC__) || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 || HAVE_POSIX_MEMALIGN
    // http://www.daemon-systems.org/man/posix_memalign.3.html
    // Memory that is allocated via posix_memalign() can be used as an argument in subsequent calls to realloc(3) and free(3).
    void* ret = realloc(ptr, size);
    if (!ret) {
        HL_DEBUG_ERROR("realloc(%d, %d) failed", size, alignment);
    }
#else
#error "hl_memory_realloc_aligned() not implemented"
#endif
    return ret;
}

void hl_memory_free(void** ptr)
{
    if (ptr && *ptr) {
#if defined(_MSC_VER)
        _aligned_free(*ptr);
#elif defined(__GNUC__) || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 || HAVE_POSIX_MEMALIGN
        // http://www.daemon-systems.org/man/posix_memalign.3.html
        // Memory that is allocated via posix_memalign() can be used as an argument in subsequent calls to realloc(3) and free(3).
        free(*ptr);
#else
#error "hl_memory_free() not implemented"
#endif
        *ptr = hl_null;
    }
}

void hl_memory_free_unaligned(void** ptr)
{
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = HL_NULL;
    }
}

void* hl_memory_calloc(hl_size_t num, hl_size_t size)
{
    return hl_memory_calloc_aligned(num, size, HL_ALIGN_V);
}

void* hl_memory_calloc_aligned(hl_size_t num, hl_size_t size, hl_size_t alignment)
{
    if (num && size) {
        void* ret = hl_memory_malloc_aligned((num * size), alignment);
        if (!ret) {
            HL_DEBUG_ERROR("Memory allocation failed. num=%u and size=%u", num, size);
            return HL_NULL;
        }
#if defined(_MSC_VER)
        ZeroMemory(ret, (num * size));
#else
        memset(ret, 0, (num * size));
#endif /* _MSC_VER */
        return ret;
    }
    return HL_NULL;
}

void* hl_memory_calloc_unaligned(hl_size_t num, hl_size_t size)
{
    if (num && size) {
        void* ret = calloc(num, size);
        if (!ret) {
            HL_DEBUG_ERROR("Memory allocation failed. num=%u and size=%u", num, size);
        }
        return ret;
    }
    return HL_NULL;
}

// TODO: optimize
void hl_memory_copy(void* p_dst, const void* pc_src, hl_size_t size_in_bytes)
{
    if (p_dst && pc_src && size_in_bytes) {
        memcpy(p_dst, pc_src, size_in_bytes);
    }
}


/*** OBJECT DEFINITION FOR "hl_memory_blocks_t" ***/
static hl_object_t* hl_memory_blocks_ctor(hl_object_t * self, va_list * app)
{
    hl_memory_blocks_t *p_blocks = (hl_memory_blocks_t*)self;
    if (p_blocks) {

    }
    return self;
}
static hl_object_t* hl_memory_blocks_dtor(hl_object_t * self)
{
    hl_memory_blocks_t *p_blocks = (hl_memory_blocks_t*)self;
    if (p_blocks) {
        HL_SAFE_FREE(p_blocks->p_memory);
    }
    return self;
}
static int hl_memory_blocks_cmp(const hl_object_t *_m1, const hl_object_t *_m2)
{
    return (int)((int*)_m1 - (int*)_m2);
}
static const hl_object_def_t hl_memory_blocks_def_s = {
    sizeof(hl_memory_blocks_t),
    hl_memory_blocks_ctor,
    hl_memory_blocks_dtor,
    hl_memory_blocks_cmp,
};
const hl_object_def_t *hl_memory_blocks_def_t = &hl_memory_blocks_def_s;
