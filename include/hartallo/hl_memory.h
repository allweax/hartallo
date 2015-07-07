#ifndef _HARTALLO_MEMORY_H_
#define _HARTALLO_MEMORY_H_

#include "hl_config.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_bits.h"
#include "hartallo/hl_debug.h"

HL_BEGIN_DECLS

#define HL_SAFE_FREE(ptr) (void)hl_memory_free((void**)(&ptr));
#define HL_MEMORY_FREE(ptr) HL_SAFE_FREE((ptr))
#define HL_SAFE_FREE_ARRAY(pptr, count){ \
	int __i; \
	for(__i = 0; __i < (count); ++__i) \
		HL_SAFE_FREE((pptr)[__i]); \
}
#define HL_MEMORY_FREE_ARRAY(pptr, count) HL_SAFE_FREE_ARRAY((pptr), (count))
#define HL_SAFE_FREE_TABLE(pptr) HL_SAFE_FREE_ARRAY((pptr), (sizeof((pptr))/sizeof((pptr)[0])))
#define HL_MEMORY_FREE_TABLE(pptr) HL_SAFE_FREE_TABLE((pptr))

#if HL_HAVE_X86_INTRIN
#define		HL_MEMORY_PREFETCH0(ptr) _mm_prefetch((const char*)(ptr), _MM_HINT_T0);
#else
#define		HL_MEMORY_PREFETCH0(ptr) ((void)(ptr))
#endif


HL_ERROR_T hl_memory_init_funcs();

#define hl_memory_is_position_aligned16(position) (((position) & 3) == 0)
#define hl_memory_is_address_aligned16(pc_void) ((((uintptr_t)(pc_void)) & 15) == 0)
#define hl_memory_is_position_aligned32(position) (((position) & 7) == 0)
#define hl_memory_is_address_aligned32(pc_void) ((((uintptr_t)(pc_void)) & 31) == 0)

#if HL_ALIGN_V == 16
#	define hl_memory_is_position_aligned(position) hl_memory_is_position_aligned16((position))
#	define hl_memory_is_address_aligned(pc_void) hl_memory_is_address_aligned16((pc_void))
#elif HL_ALIGN_V == 32
#	define hl_memory_is_position_aligned(position) hl_memory_is_position_aligned32((position))
#	define hl_memory_is_address_aligned(pc_void) hl_memory_is_address_aligned32((pc_void))
#else
#	error "Not supported"
#endif


HARTALLO_API void* hl_memory_malloc(hl_size_t size);
HARTALLO_API void* hl_memory_malloc_aligned(hl_size_t size, hl_size_t alignment);
HARTALLO_API void* hl_memory_realloc(void * ptr, hl_size_t size);
HARTALLO_API void* hl_memory_realloc_aligned(void * ptr, hl_size_t size, hl_size_t alignment);
HARTALLO_API void hl_memory_free(void** ptr);
HARTALLO_API void hl_memory_free_unaligned(void** ptr);
HARTALLO_API void* hl_memory_calloc(hl_size_t num, hl_size_t size);
HARTALLO_API void* hl_memory_calloc_aligned(hl_size_t num, hl_size_t size, hl_size_t alignment);
HARTALLO_API void* hl_memory_calloc_unaligned(hl_size_t num, hl_size_t size);
HARTALLO_API void hl_memory_copy(void* p_dst, const void* pc_src, hl_size_t size_in_bytes);

// TODO: add SIMD ASM/INTRIN versions
HL_ALWAYS_INLINE static void hl_memory_set_cpp(HL_ALIGNED(16)int32_t *p_mem, hl_size_t count, int32_t val)
{
    hl_size_t i;
    for (i = 0; i < count; ++i) {
        p_mem[i] = val;
    }
}
extern void (*hl_memory_set)(HL_ALIGNED(16)int32_t *p_mem, hl_size_t count, int32_t val);

HL_ALWAYS_INLINE static void hl_memory_copy4x4_cpp(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride)
{
    p_dst[0] = pc_src[0];
    p_dst[1] = pc_src[1];
    p_dst[2] = pc_src[2];
    p_dst[3] = pc_src[3];
    p_dst+=dst_stride;
    pc_src+=src_stride;
    p_dst[0] = pc_src[0];
    p_dst[1] = pc_src[1];
    p_dst[2] = pc_src[2];
    p_dst[3] = pc_src[3];
    p_dst+=dst_stride;
    pc_src+=src_stride;
    p_dst[0] = pc_src[0];
    p_dst[1] = pc_src[1];
    p_dst[2] = pc_src[2];
    p_dst[3] = pc_src[3];
    p_dst+=dst_stride;
    pc_src+=src_stride;
    p_dst[0] = pc_src[0];
    p_dst[1] = pc_src[1];
    p_dst[2] = pc_src[2];
    p_dst[3] = pc_src[3];
}
extern void (*hl_memory_copy4x4)(HL_ALIGNED(16) int32_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const int32_t *pc_src, hl_size_t src_stride);
extern void (*hl_memory_copy4x4_unaligned)(int32_t *p_dst, hl_size_t dst_stride, const int32_t *pc_src, hl_size_t src_stride);
#define hl_memory_copy16(p_dst, p_src) hl_memory_copy4x4((p_dst), 4, (p_src), 4)
#define hl_memory_copy16x16(p_dst, p_src) \
	hl_memory_copy16(&p_dst[0], &p_src[0]); hl_memory_copy16(&p_dst[16], &p_src[16]); hl_memory_copy16(&p_dst[32], &p_src[32]); hl_memory_copy16(&p_dst[48], &p_src[48]); \
	hl_memory_copy16(&p_dst[64], &p_src[64]); hl_memory_copy16(&p_dst[80], &p_src[80]); hl_memory_copy16(&p_dst[96], &p_src[96]); hl_memory_copy16(&p_dst[112], &p_src[112]); \
	hl_memory_copy16(&p_dst[128], &p_src[128]); hl_memory_copy16(&p_dst[144], &p_src[144]); hl_memory_copy16(&p_dst[160], &p_src[160]); hl_memory_copy16(&p_dst[176], &p_src[176]); \
	hl_memory_copy16(&p_dst[192], &p_src[192]); hl_memory_copy16(&p_dst[208], &p_src[208]); hl_memory_copy16(&p_dst[224], &p_src[224]); hl_memory_copy16(&p_dst[240], &p_src[240])

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u32_to_u8_cpp(uint8_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const uint32_t *pc_src, hl_size_t src_stride)
{
    *((uint32_t*)p_dst) = (pc_src[3] << 24) | (pc_src[2] << 16) | (pc_src[1] << 8) | pc_src[0];
    p_dst+=dst_stride;
    pc_src+=src_stride;
    *((uint32_t*)p_dst) = (pc_src[3] << 24) | (pc_src[2] << 16) | (pc_src[1] << 8) | pc_src[0];
    p_dst+=dst_stride;
    pc_src+=src_stride;
    *((uint32_t*)p_dst) = (pc_src[3] << 24) | (pc_src[2] << 16) | (pc_src[1] << 8) | pc_src[0];
    p_dst+=dst_stride;
    pc_src+=src_stride;
    *((uint32_t*)p_dst) = (pc_src[3] << 24) | (pc_src[2] << 16) | (pc_src[1] << 8) | pc_src[0];
}
extern void (*hl_memory_copy4x4_u32_to_u8)(uint8_t *p_dst, hl_size_t dst_stride, HL_ALIGNED(16) const uint32_t *pc_src, hl_size_t src_stride);

// SSE/MMX versions are optimized when strides=(4,4)
HL_ALWAYS_INLINE static void hl_memory_copy4x4_u32_to_u8_stride4x4_cpp(uint8_t *p_dst, HL_ALIGNED(16) const uint32_t *pc_src)
{
    hl_memory_copy4x4_u32_to_u8_cpp(p_dst, 4, pc_src, 4);
}
extern void (*hl_memory_copy4x4_u32_to_u8_stride4x4)(uint8_t *p_dst, HL_ALIGNED(16) const uint32_t *pc_src);

HL_ALWAYS_INLINE static void hl_memory_copy16x16_u8_to_u32_stride16x16_cpp(HL_ALIGNED(16) uint32_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src)
{
    int32_t i;
    for (i = 0; i < 16; ++i) {
        p_dst[0] = pc_src[0], p_dst[1] = pc_src[1], p_dst[2] = pc_src[2], p_dst[3] = pc_src[3];
        p_dst[4] = pc_src[4], p_dst[5] = pc_src[5], p_dst[6] = pc_src[6], p_dst[7] = pc_src[7];
        p_dst[8] = pc_src[8], p_dst[9] = pc_src[9], p_dst[10] = pc_src[10], p_dst[11] = pc_src[11];
        p_dst[13] = pc_src[13], p_dst[14] = pc_src[14], p_dst[15] = pc_src[15], p_dst[16] = pc_src[16];
        pc_src += 16;
        p_dst += 16;
    }
}
extern void (*hl_memory_copy16x16_u8_to_u32_stride16x16)(HL_ALIGNED(16) uint32_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src);


HL_ALWAYS_INLINE static void hl_memory_copy16x16_u8_to_u32_stride16x4_cpp(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/)
{
    int32_t i;
    for (i = 0; i < 16; ++i) {
        if (i && ((/*i % 4*/ i & 3) == 0)) { // new line?
            p_dst += 48; /*(16*3)*/;
        }
        p_dst[0] = pc_src[0], p_dst[1] = pc_src[1], p_dst[2] = pc_src[2], p_dst[3] = pc_src[3];
        p_dst[16] = pc_src[4], p_dst[17] = pc_src[5], p_dst[18] = pc_src[6], p_dst[19] = pc_src[7];
        p_dst[32] = pc_src[8], p_dst[33] = pc_src[9], p_dst[34] = pc_src[10], p_dst[35] = pc_src[11];
        p_dst[48] = pc_src[12], p_dst[49] = pc_src[13], p_dst[50] = pc_src[14], p_dst[51] = pc_src[15];
        pc_src += 16;
        p_dst += 4;
    }
}
extern void (*hl_memory_copy16x16_u8_to_u32_stride16x4)(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/);

HL_ALWAYS_INLINE static void hl_memory_copy16x16_u8_stride16x4_cpp(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/)
{
    int32_t i;
    for (i = 0; i < 16; ++i) {
        if (i && ((/*i % 4*/ i & 3) == 0)) { // new line?
            p_dst += 48; /*(16*3)*/;
        }
        p_dst[0] = pc_src[0], p_dst[1] = pc_src[1], p_dst[2] = pc_src[2], p_dst[3] = pc_src[3];
        p_dst[16] = pc_src[4], p_dst[17] = pc_src[5], p_dst[18] = pc_src[6], p_dst[19] = pc_src[7];
        p_dst[32] = pc_src[8], p_dst[33] = pc_src[9], p_dst[34] = pc_src[10], p_dst[35] = pc_src[11];
        p_dst[48] = pc_src[12], p_dst[49] = pc_src[13], p_dst[50] = pc_src[14], p_dst[51] = pc_src[15];
        pc_src += 16;
        p_dst += 4;
    }
}
extern void (*hl_memory_copy16x16_u8_stride16x4)(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/);


HL_ALWAYS_INLINE static void hl_memory_copy4x4_u8_to_u32_stride16x16_cpp(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src)
{
    p_dst[0] = pc_src[0], p_dst[1] = pc_src[1], p_dst[2] = pc_src[2], p_dst[3] = pc_src[3];
    p_dst[16] = pc_src[16], p_dst[17] = pc_src[17], p_dst[18] = pc_src[18], p_dst[19] = pc_src[19];
    p_dst[32] = pc_src[32], p_dst[33] = pc_src[33], p_dst[34] = pc_src[34], p_dst[35] = pc_src[35];
    p_dst[48] = pc_src[48], p_dst[49] = pc_src[49], p_dst[50] = pc_src[50], p_dst[51] = pc_src[51];
}
extern void (*hl_memory_copy4x4_u8_to_u32_stride16x16)(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src);

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u8_to_u32_stride16x4_cpp(HL_ALIGNED(16) uint32_t *p_dst/*stride=16*/, const uint8_t *pc_src/*stride=4*/)
{
    p_dst[0] = pc_src[0], p_dst[1] = pc_src[1], p_dst[2] = pc_src[2], p_dst[3] = pc_src[3];
    p_dst[16] = pc_src[4], p_dst[17] = pc_src[5], p_dst[18] = pc_src[6], p_dst[19] = pc_src[7];
    p_dst[32] = pc_src[8], p_dst[33] = pc_src[9], p_dst[34] = pc_src[10], p_dst[35] = pc_src[11];
    p_dst[48] = pc_src[12], p_dst[49] = pc_src[13], p_dst[50] = pc_src[14], p_dst[51] = pc_src[15];
}
extern void (*hl_memory_copy4x4_u8_to_u32_stride16x4)(HL_ALIGNED(16) uint32_t *p_dst, const uint8_t *pc_src);

HL_ALWAYS_INLINE static void hl_memory_copy4x4_u8_stride16x4_cpp(uint8_t *p_dst/*stride=16*/, HL_ALIGNED(16) const uint8_t *pc_src/*stride=4*/)
{
    p_dst[0] = pc_src[0], p_dst[1] = pc_src[1], p_dst[2] = pc_src[2], p_dst[3] = pc_src[3];
    p_dst[16] = pc_src[4], p_dst[17] = pc_src[5], p_dst[18] = pc_src[6], p_dst[19] = pc_src[7];
    p_dst[32] = pc_src[8], p_dst[33] = pc_src[9], p_dst[34] = pc_src[10], p_dst[35] = pc_src[11];
    p_dst[48] = pc_src[12], p_dst[49] = pc_src[13], p_dst[50] = pc_src[14], p_dst[51] = pc_src[15];
}
extern void (*hl_memory_copy4x4_u8_stride16x4)(uint8_t *p_dst, HL_ALIGNED(16) const uint8_t *pc_src);

HL_ALWAYS_INLINE static void hl_memory_setzero4x4_cpp(int32_t* p_mem)
{
    memset(p_mem, 0, (sizeof(int32_t) << 4));
}

HL_ALWAYS_INLINE static void hl_memory_setzero16x16_cpp(int32_t* p_mem)
{
    memset(p_mem, 0, (sizeof(int32_t) << 8));
}

extern void (*hl_memory_setzero4x4)(int32_t* p_mem);
extern void (*hl_memory_setzero16x16)(int32_t* p_mem);
#define hl_memory_setzero16(p_mem) hl_memory_setzero4x4((p_mem))

typedef struct hl_memory_blocks_s {
    HL_DECLARE_OBJECT;
    int32_t* p_memory;
    int32_t i_avail_flags;
} hl_memory_blocks_t;


HL_ERROR_T hl_memory_blocks_create(struct hl_memory_blocks_s** pp_blocks);

// INPORTANT: not thread-safe. Use "hl_codec_264_get_mem_blocks(p_codec)" to get a thread-safe memory block manager.
#define hl_memory_blocks_map(p_blocks, pp_mem) \
{ \
	if ((p_blocks)->i_avail_flags == 0) { \
		(p_blocks)->i_avail_flags = 0x80000000; \
		*(pp_mem) = (void*)p_blocks->p_memory; \
	} \
	if ((p_blocks)->i_avail_flags == 0xFFFFFFFF) { \
		HL_DEBUG_FATAL("No block left"); \
		*(pp_mem) = HL_NULL; \
	} \
	else { \
		hl_size_t i = hl_bits_ctz32((p_blocks)->i_avail_flags); \
		*(pp_mem) = (void*)((p_blocks)->p_memory + (i << 8)); \
		(p_blocks)->i_avail_flags |= (1 << (i - 1));  \
	} \
}

#define hl_memory_blocks_map_4x4zeros(p_blocks, pp_mem) \
{ \
	hl_memory_blocks_map((p_blocks), (pp_mem)); \
	if (*(pp_mem)) { \
		hl_memory_setzero4x4((void*)*(pp_mem)); \
	} \
}

#define hl_memory_blocks_map_16zeros(p_blocks, pp_mem) hl_memory_blocks_map_4x4zeros((p_blocks), (pp_mem))

#define hl_memory_blocks_map_16x16zeros(p_blocks, pp_mem) \
{ \
	hl_memory_blocks_map((p_blocks), (pp_mem)); \
	if (*(pp_mem)) { \
		hl_memory_setzero16x16((void*)*(pp_mem)); \
	} \
}

// INPORTANT: not thread-safe
#define hl_memory_blocks_unmap(p_blocks, pc_mem) \
{ \
	hl_size_t i = ((((const int32_t*)(pc_mem)) - (p_blocks)->p_memory) >> 8); \
	(p_blocks)->i_avail_flags &= ~(1 << (i - 1)); \
}

HL_END_DECLS

#endif /* _HARTALLO_MEMORY_H_ */
