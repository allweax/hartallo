#ifndef _HARTALLO_CODEC_264_BITS_H_
#define _HARTALLO_CODEC_264_BITS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_math.h"
#include "hartallo/hl_bits.h"
#include "hartallo/hl_debug.h"

HL_BEGIN_DECLS

#if defined(_MSC_VER)
#	pragma intrinsic(_byteswap_ulong, _BitScanForward, _BitScanReverse, _bittest)
#endif

#if !defined(HL_CODEC_264_BITS_USE_MACROS)
#	define HL_CODEC_264_BITS_USE_MACROS	0 // FIXME
#endif /* HL_CODEC_264_BITS_USE_MACROS */
#if !defined(HL_CODEC_264_BITS_USE_INTRINSICS)
#	define HL_CODEC_264_BITS_USE_INTRINSICS	1
#endif /* HL_CODEC_264_BITS_USE_INTRINSICS */
#if !defined(HL_CODEC_264_BITS_USE_LEGACY)
#	define HL_CODEC_264_BITS_USE_LEGACY	0
#endif /* HL_CODEC_264_BITS_USE_LEGACY */

#define HL_CODEC_264_BITS_NEXT_8(ptr) ((ptr)[0])
#define HL_CODEC_264_BITS_NEXT_16(ptr) (((int16_t)(ptr)[0])<<8 | (int16_t)(ptr)[1])
#define HL_CODEC_264_BITS_NEXT_24(ptr) (((int32_t)(ptr)[0])<<16 | ((int32_t)(ptr)[1])<<8 | (int32_t)(ptr)[2])
#if defined(_MSC_VER) && HL_CODEC_264_BITS_USE_INTRINSICS
#	define HL_CODEC_264_BITS_NEXT_32(ptr)	_byteswap_ulong(*((unsigned long*)(ptr)))
#elif defined(__GNUC__) && HL_CODEC_264_BITS_USE_INTRINSICS
#	include <byteswap.h>
#	define HL_CODEC_264_BITS_NEXT_32(ptr)	__builtin_bswap32(*((int32_t*)(ptr)))
#else
#	define HL_CODEC_264_BITS_NEXT_32(ptr) (((int32_t)(ptr)[0])<<24 | ((int32_t)(ptr)[1])<<16 | ((int32_t)(ptr)[2])<<8 | (int32_t)(ptr)[3])
#endif

typedef struct hl_codec_264_bits_s {
    HL_DECLARE_OBJECT;

    const uint8_t* pc_start;
    const uint8_t* pc_end;
    /*const*/ uint8_t* pc_current;
    struct {
        int32_t i_val;
        int32_t i_index;
    } next_32_bits;
    int32_t i_bits_count;
}
hl_codec_264_bits_t;

// caller must call IsEoB()
#if HL_CODEC_264_BITS_USE_MACROS
#define hl_codec_264_bits_get_next_32bits(self) \
{ \
	self->next_32_bits.i_val = HL_CODEC_264_BITS_NEXT_32(self->pc_current); \
	self->next_32_bits.i_index = (7 - self->i_bits_count); \
}
#else
static HL_SHOULD_INLINE void hl_codec_264_bits_get_next_32bits(hl_codec_264_bits_t* self)
{
    self->next_32_bits.i_val = HL_CODEC_264_BITS_NEXT_32(self->pc_current);
    self->next_32_bits.i_index = (7 - self->i_bits_count);
}
#endif

#define hl_codec_264_bits_read_f1(self)			hl_codec_264_bits_read_u1((self))
#define hl_codec_264_bits_read_f(self, n)		hl_codec_264_bits_read_u((self), (count))
#define hl_codec_264_bits_write_f1(self, u1)	hl_codec_264_bits_write_u1((self), (u1))

HL_ERROR_T hl_codec_264_bits_create(hl_codec_264_bits_t** pp_bits, const void* pc_buff, hl_size_t u_buff_size);
HL_ERROR_T hl_codec_264_bits_create_2(hl_codec_264_bits_t** pp_bits);

static HL_SHOULD_INLINE uint32_t _hl_codec_264_bits_read_u_packed(hl_codec_264_bits_t* self, uint32_t n);
static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_read_u(hl_codec_264_bits_t* self, int32_t n);

static HL_SHOULD_INLINE HL_ERROR_T hl_codec_264_bits_reset(hl_codec_264_bits_t* self, const void* newBuff, hl_size_t newBuffSize)
{
    if (!self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    self->i_bits_count = 7;
    if ((self->pc_start = self->pc_current = (uint8_t*)newBuff)) {
        self->pc_end = self->pc_start + newBuffSize;
        hl_codec_264_bits_get_next_32bits(self);
    }
    else {
        self->pc_end = HL_NULL;
    }

    return HL_ERROR_SUCCESS;
}

/**
* Checks whether we reached the end of buffer
* @param self the bitstream object
* @retval true if we reached the end of the buffer and false otherwise
*/
#if HL_CODEC_264_BITS_USE_MACROS
#	define hl_codec_264_bits_is_eob(self)	((self)->pc_current > (self)->pc_end)
#else
static HL_SHOULD_INLINE hl_bool_t hl_codec_264_bits_is_eob(hl_codec_264_bits_t* self)
{
    return (self->pc_current > self->pc_end);
}
#endif/* HL_CODEC_264_BITS_USE_MACROS */

#if HL_CODEC_264_BITS_USE_MACROS
#	define	hl_codec_264_bits_is_aligned(self)	((self)->i_bits_count == 7)
#else
static HL_SHOULD_INLINE hl_bool_t hl_codec_264_bits_is_aligned(hl_codec_264_bits_t* self)
{
    /* 7.2 Specification of syntax functions, categories, and descriptors
    	byte_aligned( ) is specified as follows.
    		– If the current position in the bitstream is on a byte boundary, i.e., the next bit in the bitstream is the first bit in a
    		byte, the return value of byte_aligned( ) is equal to TRUE.
    		– Otherwise, the return value of byte_aligned( ) is equal to FALSE.
    */
    return (self->i_bits_count == 7);
}
#endif/* HL_CODEC_264_BITS_USE_MACROS */

// used for debugging
#if HL_CODEC_264_BITS_USE_MACROS
#define hl_codec_264_bits_get_index(self) (((self)->pc_current - (self)->start) << 3) + (7 - (self)->i_bits_count)
#else
static HL_SHOULD_INLINE int64_t hl_codec_264_bits_get_index(hl_codec_264_bits_t* self)
{
    return ((self->pc_current - self->pc_start) << 3) + (7 - self->i_bits_count);
}
#endif


#if HL_CODEC_264_BITS_USE_MACROS
#define hl_codec_264_bits_discard(self, n) \
{ \
	uint32_t bitsCount = ((7 - (self)->i_bits_count) + (n)); \
	(self)->pc_current += (bitsCount >> 3); \
	(self)->next_32_bits.i_index+= (n); \
	(self)->i_bits_count -= (HL_MATH_MOD_POW2_INT32((n), 8)); \
	if((self)->i_bits_count < 0){ \
		(self)->i_bits_count = (8 + (self)->i_bits_count); \
	} \
}
#else
static HL_SHOULD_INLINE void hl_codec_264_bits_discard(hl_codec_264_bits_t* self, uint32_t n)
{
    uint32_t bitsCount = ((7 - self->i_bits_count) + n);
    self->pc_current += (bitsCount >> 3);
    self->next_32_bits.i_index += n;
    self->i_bits_count -= HL_MATH_MOD_POW2_INT32(n, 8);
    if(self->i_bits_count < 0) {
        self->i_bits_count = (8 + self->i_bits_count);
    }
}
#endif/* HL_CODEC_264_BITS_USE_MACROS */

#if HL_CODEC_264_BITS_USE_MACROS
#	define		hl_codec_264_bits_align(self) \
	if (!hl_codec_264_bits_is_eob((self))) { \
		if((self)->i_bits_count != 7){ \
			++(self)->pc_current; \
			(self)->next_32_bits.i_index+= (8 - (7 - (self)->i_bits_count)); \
			(self)->i_bits_count = 7; \
		} \
	}
#else
static HL_SHOULD_INLINE void hl_codec_264_bits_align(hl_codec_264_bits_t* self)
{
    if (!hl_codec_264_bits_is_eob(self)) {
        if (self->i_bits_count != 7) {
            ++self->pc_current;
            self->next_32_bits.i_index += (8 - (7 - self->i_bits_count));
            self->i_bits_count = 7;
        }
    }
}
#endif/* HL_CODEC_264_BITS_USE_MACROS */

static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_read_u1(hl_codec_264_bits_t* self)
{
    if (hl_codec_264_bits_is_eob(self)) {
        HL_DEBUG_ERROR("EOB");
        return 0;
    }
#if defined(_MSC_VER) && HL_CODEC_264_BITS_USE_INTRINSICS
    {
        uint32_t r = 0;
        if (_bittest((long*)self->pc_current, self->i_bits_count--)) {
            r = 1;
        }
        if (self->i_bits_count < 0) {
            self->pc_current++, self->i_bits_count = 7;
        }
        ++self->next_32_bits.i_index;
        return r;
    }
#else
#if 1
    {
#if defined(__GNUC__)
        uint32_t r = 0;
        __asm__("bt %[self->i_bits_count], %[self->pc_current]; setb %self->i_bits_count[r]" : [r] "=q" (r) : [self->pc_current] "mr" (*self->pc_current), [self->i_bits_count] "r" (self->i_bits_count));
        --self->i_bits_count;
#else /* !__GNUC__ */
        uint32_t r = (*((int32_t*)self->pc_current) & (1 << (self->i_bits_count--))) ? 1 : 0;
        if (self->i_bits_count < 0) {
            self->pc_current++, self->i_bits_count = 7;
        }
#endif
        ++self->next_32_bits.i_index;
        return r;
    }
#else
    return hl_codec_264_bits_read_u(self, 1);
#endif
#endif
}

#if HL_CODEC_264_BITS_USE_MACROS
#define hl_codec_264_bits_write_u1(self, u1) {\
	 if (hl_codec_264_bits_is_eob((self))) {\
        HL_DEBUG_ERROR("EoB");\
     }\
	 else {\
		if ((u1)) {\
			*(self)->pc_current |= (1 << (self)->i_bits_count--);\
		}\
		else {\
			*(self)->pc_current &= ~(1 << (self)->i_bits_count--);\
		}\
		if ((self)->i_bits_count < 0) {\
			(self)->pc_current++, (self)->i_bits_count = 7;\
		}\
	 }\
}
#else
static HL_SHOULD_INLINE void hl_codec_264_bits_write_u1(hl_codec_264_bits_t* self, uint32_t u1)
{
    if (hl_codec_264_bits_is_eob(self)) {
        HL_DEBUG_ERROR("EoB");
        return;
    }
    if (u1) {
        *self->pc_current |= (1 << self->i_bits_count--);
    }
    else {
        *self->pc_current &= ~(1 << self->i_bits_count--);
    }
    if (self->i_bits_count < 0) {
        self->pc_current++, self->i_bits_count = 7;
    }
}
#endif

// can only read [1-8] or 16
// should never be called by external function
static HL_SHOULD_INLINE uint32_t _hl_codec_264_bits_read_u_packed(hl_codec_264_bits_t* self, uint32_t n)
{
    uint32_t r = 0;

    if (hl_codec_264_bits_is_eob(self)) {
        HL_DEBUG_ERROR("EoB");
        return 0;
    }
    switch(self->i_bits_count) {
    case 7: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 7), self->i_bits_count = 6;
            break;;
        case 2:
            r = (self->pc_current[0] >> 6), self->i_bits_count = 5;
            break;;
        case 3:
            r = (self->pc_current[0] >> 5), self->i_bits_count = 4;
            break;;
        case 4:
            r = (self->pc_current[0] >> 4), self->i_bits_count = 3;
            break;;
        case 5:
            r = (self->pc_current[0] >> 3), self->i_bits_count = 2;
            break;;
        case 6:
            r = (self->pc_current[0] >> 2), self->i_bits_count = 1;
            break;;
        case 7:
            r = (self->pc_current[0] >> 1), self->i_bits_count = 0;
            break;;
        case 8:
            r = self->pc_current[0], ++self->pc_current;
            break;;
        case 16:
            r = self->pc_current[0] << 8 | self->pc_current[1], self->pc_current+=2;
            break;;
        }
        break;
    }
    case 6: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 6) & 0x01, self->i_bits_count = 5;
            break;;
        case 2:
            r = (self->pc_current[0] >> 5) & 0x03, self->i_bits_count = 4;
            break;;
        case 3:
            r = (self->pc_current[0] >> 4) & 0x07, self->i_bits_count = 3;
            break;;
        case 4:
            r = (self->pc_current[0] >> 3) & 0x0F, self->i_bits_count = 2;
            break;;
        case 5:
            r = (self->pc_current[0] >> 2) & 0x1F, self->i_bits_count = 1;
            break;;
        case 6:
            r = (self->pc_current[0] >> 1) & 0x3F, self->i_bits_count = 0;
            break;;
        case 7:
            r = (self->pc_current[0] & 0x7F), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 8:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }
    case 5: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 5) & 0x01, self->i_bits_count = 4;
            break;;
        case 2:
            r = (self->pc_current[0] >> 4) & 0x03, self->i_bits_count = 3;
            break;;
        case 3:
            r = (self->pc_current[0] >> 3) & 0x07, self->i_bits_count = 2;
            break;;
        case 4:
            r = (self->pc_current[0] >> 2) & 0x0F, self->i_bits_count = 1;
            break;;
        case 5:
            r = (self->pc_current[0] >> 1) & 0x1F, self->i_bits_count = 0;
            break;;
        case 6:
            r = (self->pc_current[0] & 0x3F), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 7:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0x7F, self->i_bits_count = 6, ++self->pc_current;
            break;;
        case 8:
            r = (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }

    case 4: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 4) & 0x01, self->i_bits_count = 3;
            break;;
        case 2:
            r = (self->pc_current[0] >> 3) & 0x03, self->i_bits_count = 2;
            break;;
        case 3:
            r = (self->pc_current[0] >> 2) & 0x07, self->i_bits_count = 1;
            break;;
        case 4:
            r = (self->pc_current[0] >> 1) & 0x0F, self->i_bits_count = 0;
            break;;
        case 5:
            r = (self->pc_current[0] & 0x1F), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 6:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0x3F, self->i_bits_count = 6, ++self->pc_current;
            break;;
        case 7:
            r = (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0x7F, self->i_bits_count = 5, ++self->pc_current;
            break;;
        case 8:
            r = (self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }

    case 3: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 3) & 0x01, self->i_bits_count = 2;
            break;;
        case 2:
            r = (self->pc_current[0] >> 2) & 0x03, self->i_bits_count = 1;
            break;;
        case 3:
            r = (self->pc_current[0] >> 1) & 0x07, self->i_bits_count = 0;
            break;;
        case 4:
            r = (self->pc_current[0] & 0x0F), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 5:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0x1F, self->i_bits_count = 6, ++self->pc_current;
            break;;
        case 6:
            r = (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0x3F, self->i_bits_count = 5, ++self->pc_current;
            break;;
        case 7:
            r = (self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0x7F, self->i_bits_count = 4, ++self->pc_current;
            break;;
        case 8:
            r = (self->pc_current[0] << 4 | self->pc_current[1] >> 4) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 4 | self->pc_current[1] >> 4) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 4 | self->pc_current[1] >> 4) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }

    case 2: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 2) & 0x01, self->i_bits_count = 1;
            break;;
        case 2:
            r = (self->pc_current[0] >> 1) & 0x03, self->i_bits_count = 0;
            break;;
        case 3:
            r = (self->pc_current[0] & 0x07), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 4:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0x0F, self->i_bits_count = 6, ++self->pc_current;
            break;;
        case 5:
            r = (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0x1F, self->i_bits_count = 5, ++self->pc_current;
            break;;
        case 6:
            r = (self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0x3F, self->i_bits_count = 4, ++self->pc_current;
            break;;
        case 7:
            r = (self->pc_current[0] << 4 | self->pc_current[1] >> 4) & 0x7F, self->i_bits_count = 3, ++self->pc_current;
            break;;
        case 8:
            r = (self->pc_current[0] << 5 | self->pc_current[1] >> 3) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 5 | self->pc_current[1] >> 3) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 5 | self->pc_current[1] >> 3) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }

    case 1: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] >> 1) & 0x01, self->i_bits_count = 0;
            break;;
        case 2:
            r = (self->pc_current[0] & 0x03), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 3:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0x07, self->i_bits_count = 6, ++self->pc_current;
            break;;
        case 4:
            r = (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0x0F, self->i_bits_count = 5, ++self->pc_current;
            break;;
        case 5:
            r = (self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0x1F, self->i_bits_count = 4, ++self->pc_current;
            break;;
        case 6:
            r = (self->pc_current[0] << 4 | self->pc_current[1] >> 4) & 0x3F, self->i_bits_count = 3, ++self->pc_current;
            break;;
        case 7:
            r = (self->pc_current[0] << 5 | self->pc_current[1] >> 3) & 0x7F, self->i_bits_count = 2, ++self->pc_current;
            break;;
        case 8:
            r = (self->pc_current[0] << 6 | self->pc_current[1] >> 2) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 6 | self->pc_current[1] >> 2) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 6 | self->pc_current[1] >> 2) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }

    case 0: {
        switch(n) {
        case 1:
            r = (self->pc_current[0] & 0x01), ++self->pc_current, self->i_bits_count = 7;
            break;;
        case 2:
            r = (self->pc_current[0] << 1 | self->pc_current[1] >> 7) & 0x03, self->i_bits_count = 6, ++self->pc_current;
            break;;
        case 3:
            r = (self->pc_current[0] << 2 | self->pc_current[1] >> 6) & 0x07, self->i_bits_count = 5, ++self->pc_current;
            break;;
        case 4:
            r = (self->pc_current[0] << 3 | self->pc_current[1] >> 5) & 0x0F, self->i_bits_count = 4, ++self->pc_current;
            break;;
        case 5:
            r = (self->pc_current[0] << 4 | self->pc_current[1] >> 4) & 0x1F, self->i_bits_count = 3, ++self->pc_current;
            break;;
        case 6:
            r = (self->pc_current[0] << 5 | self->pc_current[1] >> 3) & 0x3F, self->i_bits_count = 2, ++self->pc_current;
            break;;
        case 7:
            r = (self->pc_current[0] << 6 | self->pc_current[1] >> 2) & 0x7F, self->i_bits_count = 1, ++self->pc_current;
            break;;
        case 8:
            r = (self->pc_current[0] << 7 | self->pc_current[1] >> 1) & 0xFF, ++self->pc_current;
            break;;
        case 16: {
            r = ((self->pc_current[0] << 7 | self->pc_current[1] >> 1) & 0xFF) << 8;
            ++self->pc_current;
            r |= (self->pc_current[0] << 7 | self->pc_current[1] >> 1) & 0xFF;
            ++self->pc_current;
            break;;
        }
        }
        break;
    }
    }

    self->next_32_bits.i_index += n;

    return r;
}

static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_read_u(hl_codec_264_bits_t* self, int32_t n)
{
    if (hl_codec_264_bits_is_eob(self)) {
        HL_DEBUG_ERROR("EoB");
        return 0;
    }

    if (n <= 32) {
        uint32_t ret;
        if ((self->next_32_bits.i_index + n) >= 32) {
            hl_codec_264_bits_get_next_32bits (self);
        }
        ret = self->next_32_bits.i_val >> (32 - (self->next_32_bits.i_index + n));
        hl_codec_264_bits_discard(self, n);
        return (ret & ~(0xFFFFFFFF << n));
    }
    else {
        switch (n) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 16: { // FIXME: never called because of "if (n <= 32)"
            return _hl_codec_264_bits_read_u_packed(self, n);
        }
        default: {
            uint32_t k, r;
            r = _hl_codec_264_bits_read_u_packed(self, 8), n-=8;
            while (n > 0) {
                k = (n & 7);
                if (!k) {
                    k=8;
                }
                r<<=k;
                r |= _hl_codec_264_bits_read_u_packed(self, k);
                n -= k;
            }
            return r;
        }
        }
    }
}



static HL_SHOULD_INLINE void hl_codec_264_bits_write_u(hl_codec_264_bits_t* self, uint32_t u, uint32_t n)
{
    register int32_t i;
    if (hl_codec_264_bits_is_eob(self)) {
        HL_DEBUG_ERROR("EoB");
        return;
    }

    // FIXME: optimize for (n=8,16,32) and bits_count == 7
    i = (int32_t)n - 1;
    while (i >= 0) {
        // FIXME: use _bitset(i)
        hl_codec_264_bits_write_u1(self, ((u >> i--) & 0x01));
    }
}

// FIXME: use next32bits
// (n) must be equal or less than 32
static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_show_u(hl_codec_264_bits_t* self, uint32_t n)
{
    uint32_t ret;
    uint32_t next32bits_val, next32bits_index;
    if((self->next_32_bits.i_index + n) & ~0x1F) {
        next32bits_val = HL_CODEC_264_BITS_NEXT_32(self->pc_current);
        next32bits_index = (7 - self->i_bits_count);
    }
    else {
        next32bits_val = self->next_32_bits.i_val;
        next32bits_index = self->next_32_bits.i_index;
    }
    ret = next32bits_val >> (32 - (next32bits_index + n));
    return (ret & ~(0xFFFFFFFF << n));
}

static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_read_ue(hl_codec_264_bits_t* self)
{
    /* 9.1 Parsing process for Exp-Golomb codes
    	This process is invoked when the descriptor of a syntax element in the syntax tables in subclause 7.3 is equal to ue(v),
    	me(v), se(v), or te(v). For syntax elements in subclauses 7.3.4 and 7.3.5, this process is invoked only when
    	entropy_coding_mode_flag is equal to 0.
    	Inputs to this process are bits from the RBSP.
    	Outputs of this process are syntax element values.
    	Syntax elements coded as ue(v), me(v), or se(v) are Exp-Golomb-coded. Syntax elements coded as te(v) are truncated
    	Exp-Golomb-coded. The parsing process for these syntax elements begins with reading the bits starting at the current
    	location in the bitstream up to and including the first non-zero bit, and counting the number of leading bits that are
    	equal to 0. This process is specified as follows:

    		leadingZeroBits = -1
    		for( b = 0; !b; leadingZeroBits++ ) (9-1)
    		b = read_bits( 1 )

    	The variable u_code_num is then assigned as follows:
    		u_code_num = 2 pow(leadingZeroBits) - 1 + read_bits( leadingZeroBits ) (9-2)

    	where the value returned from read_bits( leadingZeroBits ) is interpreted as a binary representation of an unsigned
    	integer with most significant bit written first.

    	If the syntax element is coded as ue(v), the value of the syntax element is equal to u_code_num
    */
#if HL_CODEC_264_BITS_USE_LEGACY
#	if 0
    int32_t leadingZeroBits = -1;
    uint32_t b;
    for (b=0; !b && !hl_codec_264_bits_is_eob(self); ++leadingZeroBits) {
        b = hl_codec_264_bits_read_u1(self);
    }
    return leadingZeroBits > 0 ? (1 << leadingZeroBits) - 1 + hl_codec_264_bits_read_u(self, leadingZeroBits) : 0;
#	else
    int32_t r = hl_codec_264_bits_read_u1(self);
    if ( r == 0) {
        uint32_t len = 0;
        while (!(r & 1)) {
            r = hl_codec_264_bits_read_u1(self);
            len ++;
        }
        r = hl_codec_264_bits_read_u(self, len);
        r += (1 << len) - 1;
        return r;
    }
    else {
        return 0;
    }
#endif
#else // FIXME: Show(16?)
    uint16_t tmp = hl_codec_264_bits_show_u(self, 16);
    uint32_t leadingZeroBits = (uint32_t)hl_bits_clz16(tmp);
    if(leadingZeroBits > 0) {
        hl_codec_264_bits_discard(self, leadingZeroBits + 1);
        return (1 << leadingZeroBits) - 1 + hl_codec_264_bits_read_u(self, leadingZeroBits);
    }
    hl_codec_264_bits_discard(self, 1);
    return 0;

    // FIXME: increment current each time bits_count reach zero
#endif
}

static HL_SHOULD_INLINE void hl_codec_264_bits_write_ue(hl_codec_264_bits_t* self, uint32_t u_code_num)
{
    switch(u_code_num) {
    case 0: {
        hl_codec_264_bits_write_u1(self, 1);
        break;
    }
    default: {
#if HL_CODEC_264_BITS_USE_LEGACY
        register uint32_t leadingZeroBits = 1;
        while((1 << leadingZeroBits) < (int32_t)(u_code_num + 2)) {
            ++leadingZeroBits;
        }
        --leadingZeroBits;
#else
        uint32_t leadingZeroBits = (uint32_t)(31 - hl_bits_clz32(u_code_num + 2));
        if ((u_code_num + 2) == (1 << leadingZeroBits)) { // ((u_code_num + 2)) == pow(2, leadingZeroBits) ??
            --leadingZeroBits;
        }
#endif

        hl_codec_264_bits_write_u(self, 0, leadingZeroBits);
        hl_codec_264_bits_write_u(self, ((u_code_num - (1 << leadingZeroBits) + 1) | HL_MATH_POW2_VAL[leadingZeroBits]), leadingZeroBits + 1);
        break;
    }
    }
}

static HL_SHOULD_INLINE hl_size_t hl_codec_264_bits_count_bits_ue(uint32_t u_code_num)
{
    switch(u_code_num) {
    case 0:
        return 1;
    default: {
#if HL_CODEC_264_BITS_USE_LEGACY
        register uint32_t leadingZeroBits = 1;
        while ((1 << leadingZeroBits) < (int32_t)(u_code_num + 2)) {
            ++leadingZeroBits;
        }
        --leadingZeroBits;
        return (leadingZeroBits << 1) + 1;
#else
        hl_size_t leadingZeroBits = 31 - hl_bits_clz32(u_code_num + 2);
        if ((u_code_num + 2) == (1 << leadingZeroBits)) { // ((u_code_num + 2)) == pow(2, leadingZeroBits) ??
            return (leadingZeroBits << 1) - 1; // = (((leadingZeroBits - 1)) << 1) + 1
        }
        return (leadingZeroBits << 1) + 1;
#endif
    }
    }
}


static HL_SHOULD_INLINE int32_t hl_codec_264_bits_read_se(hl_codec_264_bits_t* self)
{
    /* 9.1 Parsing process for Exp-Golomb codes
    	Otherwise, if the syntax element is coded as se(v), the value of the syntax element is derived by invoking the
    	mapping process for signed Exp-Golomb codes as specified in subclause 9.1.1 with u_code_num as the input.
    */
#if HL_CODEC_264_BITS_USE_LEGACY
    int32_t r = hl_codec_264_bits_read_u1(self);
    if (r == 0) {
        uint32_t len = 0;
        while (!(r & 1)) {
            r = hl_codec_264_bits_read_u1(self);
            len++;
        }
        r = hl_codec_264_bits_read_u(self, len);
        r += (1 << len);
        return (r & 1) ? -(r>>1) : (r>>1);
    }
    else {
        return 0;
    }
#else
    int32_t r = hl_codec_264_bits_read_ue(self);
    // (-1) pow(k+1) ceil(k/2)
    // ceil(x/y) = (x + y - 1) / y
    return (r & 0x01) ? ((r+1)>>1) : -(r>>1);
#endif
}

static HL_SHOULD_INLINE void hl_codec_264_bits_write_se(hl_codec_264_bits_t* self, int32_t n)
{
    uint32_t u_code_num = (n<=0) ? (-n<<1) : ((n<<1)-1);
    hl_codec_264_bits_write_ue(self, u_code_num);
}

static HL_SHOULD_INLINE hl_size_t hl_codec_264_bits_count_bits_se(int32_t n)
{
    uint32_t u_code_num = (n<=0) ? (-n<<1) : ((n<<1)-1);
    return hl_codec_264_bits_count_bits_ue(u_code_num);
}

static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_read_me(hl_codec_264_bits_t* self, int32_t i_chroma_array_type, hl_bool_t b_intra4x4_pred_mode)
{
    // Table 9-4 – Assignment of u_code_num to values of u_coded_block_pattern for macroblock prediction modes
    // i_chroma_array_type = 1 or 2
    // HL_H264_CBP_2_CODE_NUM_MAP_12[u_code_nume=0-47][Intra_4x4=0/Intra_8x8=0/Inter=1]
    static const int32_t HL_H264_CBP_2_CODE_NUM_MAP_12[48][2] = {
        {47, 0}, {31, 16}, {15, 1}, { 0, 2}, {23, 4}, {27, 8},
        {29, 32}, {30, 3}, { 7, 5}, {11, 10}, {13, 12}, {14, 15},
        {39, 47}, {43, 7}, {45, 11}, {46, 13}, {16, 14}, { 3, 6},
        { 5, 9}, {10, 31}, {12, 35}, {19, 37}, {21, 42}, {26, 44},
        {28, 33}, {35, 34}, {37, 36}, {42, 40}, {44, 39}, { 1, 43},
        { 2, 45}, { 4, 46}, { 8, 17}, {17, 18}, {18, 20}, {20, 24},
        {24, 19}, { 6, 21}, { 9, 26}, {22, 28}, {25, 23}, {32, 27},
        {33, 29}, {34, 30}, {36, 22}, {40, 25}, {38, 38}, {41, 41},
    };
    // i_chroma_array_type = 0 or 3
    static const int32_t HL_H264_CBP_2_CODE_NUM_MAP_03[16][2] = {
        {15, 0}, {0, 1}, {7, 2}, {11, 4}, {13, 8}, {14, 3}, {3, 5}, {5, 10},
        {10, 12}, {12, 15}, {1, 7}, {2, 11}, {4, 13}, {8, 14},{6, 6}, {9, 9}
    };
    /* 9.1 Parsing process for Exp-Golomb codes
    	Otherwise, if the syntax element is coded as me(v), the value of the syntax element is derived by invoking the
    	mapping process for coded block pattern as specified in subclause 9.1.2 with u_code_num as the input.

    	Output => "u_coded_block_pattern" syntax element
    	Table 9-4 shows the assignment of u_coded_block_pattern to u_code_num depending on whether the macroblock prediction
    	mode is equal to Intra_4x4, Intra_8x8 or Inter.
    */
    int32_t r = hl_codec_264_bits_read_ue(self);
    switch(i_chroma_array_type) {
    case 1:
    case 2:
        return HL_H264_CBP_2_CODE_NUM_MAP_12[r][b_intra4x4_pred_mode?0:1];
    case 0:
    case 3:
    default:
        return HL_H264_CBP_2_CODE_NUM_MAP_03[r][b_intra4x4_pred_mode?0:1];
    }
}

static HL_SHOULD_INLINE void hl_codec_264_bits_write_me(hl_codec_264_bits_t* self, uint32_t u_coded_block_pattern, int32_t i_chroma_array_type, hl_bool_t b_intra4x4_pred_mode)
{
    // Table 9-4 – Assignment of u_code_num to values of u_coded_block_pattern for macroblock prediction modes
    // i_chroma_array_type = 1 or 2
    const static uint8_t HL_H264_CBP_2_CODE_NUM_MAP_12[48][2] = {
        {3, 0}, {29, 2}, {30, 3}, {17, 7}, {31, 4}, {18, 8}, {37, 17}, {8, 13}, {32, 5}, {38, 18}, {19, 9}, {9, 14},
        {20, 10}, {10, 15}, {11, 16}, {2, 11}, {16, 1}, {33, 32}, {34, 33}, {21, 36}, {35, 34}, {22, 37}, {39, 44}, {4, 40},
        {36, 35}, {40, 45}, {23, 38}, {5, 41}, {24, 39}, {6, 42}, {7, 43}, {1, 19}, {41, 6}, {42, 24}, {43, 25}, {25, 20},
        {44, 26}, {26, 21}, {46, 46}, {12, 28}, {45, 27}, {47, 47}, {27, 22}, {13, 29}, {28, 23}, {14, 30}, {15, 31}, {0, 12}
    };

    switch(i_chroma_array_type) {
    case 1:
    case 2: {
        uint32_t u_code_num = HL_H264_CBP_2_CODE_NUM_MAP_12[u_coded_block_pattern][b_intra4x4_pred_mode?0:1];
        hl_codec_264_bits_write_ue(self, u_code_num);
        break;
    }
    case 0:
    case 3:
    default:
        HL_DEBUG_ERROR("Not implemented yet");
        break;
    }


}

static HL_SHOULD_INLINE uint32_t hl_codec_264_bits_read_te(hl_codec_264_bits_t* self, uint32_t range)
{
    return (range > 1) ? hl_codec_264_bits_read_ue(self) : !hl_codec_264_bits_read_u1(self);
}

static HL_SHOULD_INLINE void hl_codec_264_bits_write_te(hl_codec_264_bits_t* self, uint32_t n, uint32_t u_range)
{
    if (u_range > 1) {
        hl_codec_264_bits_write_ue(self, n);
    }
    else {
        hl_codec_264_bits_write_u1(self, 1 - n);
    }
}


// "add +8" (NAL header) when comparing with Thialgou (https://code.google.com/p/thialgou/)
#define hl_codec_264_bits_get_stream_index(self) \
    (((self)->pc_current - (self)->pc_start) << 3) + (7 - (self)->i_bits_count)

HL_END_DECLS

#endif /* _HARTALLO_CODEC_264_BITS_H_ */
