#ifndef _HARTALLO_BITS_H_
#define _HARTALLO_BITS_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

#if defined(__GNUC__)
#	include <byteswap.h>
#endif

HL_BEGIN_DECLS

HL_ALWAYS_INLINE static hl_size_t hl_bits_clz32_cpp(int32_t n)
{
    if (n) {
        hl_size_t ret = 0;
        if (!(n & 0xFFFF0000)) {
            ret += 16, n <<= 16;
        }
        if (!(n & 0xFF000000)) {
            ret +=  8, n <<=  8;
        }
        if (!(n & 0xF0000000)) {
            ret +=  4, n <<=  4;
        }
        if (!(n & 0xC0000000)) {
            ret +=  2, n <<=  2;
        }
        if (!(n & 0x80000000)) {
            ret +=  1;
        }
        return ret;
    }
    return 32;
}

HL_ALWAYS_INLINE static hl_size_t hl_bits_clz16_cpp(int16_t n)
{
    if (n) {
        hl_size_t ret = 0;
        if (!(n & 0xFF00)) {
            ret +=  8, n <<=  8;
        }
        if (!(n & 0xF000)) {
            ret +=  4, n <<=  4;
        }
        if (!(n & 0xC000)) {
            ret +=  2, n <<=  2;
        }
        if (!(n & 0x8000)) {
            ret +=  1;
        }
        return ret;
    }
    return 16;
}

#if (_MSC_VER >= 1400) && !HL_DISABLE_INTRIN
#	pragma intrinsic(_BitScanReverse)
HL_ALWAYS_INLINE static hl_size_t hl_bits_clz32_mvs(int32_t n)
{
    if (n) {
        long r = 0;
        _BitScanReverse(&r, n);
        return (r ^ 31);
    }
    return 32;
}

HL_ALWAYS_INLINE static hl_size_t hl_bits_clz16_mvs(int16_t n)
{
    if (n) {
        long r = 0;
        _BitScanReverse(&r, n << 16);
        return (r ^ 31);
    }
    return 16;
}
#endif /* _MSC_VER >= 1400 */

#if (_MSC_VER >= 1500) && !HL_DISABLE_INTRIN
#	pragma intrinsic(__lzcnt)
HL_ALWAYS_INLINE static hl_size_t hl_bits_clz32_mvs_lzcnt(int32_t n)
{
    if (n) {
        return __lzcnt(n) ^ 31;
    }
    return 32;
}
#endif /* _MSC_VER >= 1500 */

#if defined(__GNUC__) && !HL_DISABLE_INTRIN
HL_ALWAYS_INLINE static hl_size_t hl_bits_clz32_gcc(int32_t n)
{
    if (n) {
        return __builtin_clz(n);
    }
    return 32;
}
HL_ALWAYS_INLINE static hl_size_t hl_bits_clz16_gcc(int16_t n)
{
    if (n) {
        return __builtin_clzs(n);
    }
    return 16;
}
#endif

extern hl_size_t (*hl_bits_clz32)(int32_t n);
extern hl_size_t (*hl_bits_clz16)(int16_t n);

HL_ALWAYS_INLINE static hl_size_t hl_bits_ctz32_cpp(int32_t n)
{
    if (n) {
        hl_size_t ret = 0;
        if (!(n & 0x0000FFFF)) {
            ret += 16, n >>= 16;
        }
        if (!(n & 0x000000FF)) {
            ret +=  8, n >>=  8;
        }
        if (!(n & 0x0000000F)) {
            ret +=  4, n >>=  4;
        }
        if (!(n & 0x00000003)) {
            ret +=  2, n >>=  2;
        }
        if (!(n & 0x00000001)) {
            ret +=  1;
        }
        return ret;
    }
    return 32;
}

HL_ALWAYS_INLINE static hl_size_t hl_bits_ctz16_cpp(int16_t n)
{
    if (n) {
        hl_size_t ret = 0;
        if (!(n & 0x00FF)) {
            ret += 8, n >>= 8;
        }
        if (!(n & 0x000F)) {
            ret += 4, n >>= 4;
        }
        if (!(n & 0x0003)) {
            ret += 2, n >>= 2;
        }
        if (!(n & 0x0001)) {
            ret += 1;
        }
        return ret;
    }
    return 16;
}

#if (_MSC_VER >= 1400) && !HL_DISABLE_INTRIN
#	pragma intrinsic(_BitScanForward)
HL_ALWAYS_INLINE static hl_size_t hl_bits_ctz32_mvs(int32_t n)
{
    if (n) {
        long r = 0;
        _BitScanForward(&r, n);
        return r;
    }
    return 32;
}

HL_ALWAYS_INLINE static hl_size_t hl_bits_ctz16_mvs(int16_t n)
{
    if (n) {
        long r = 0;
        _BitScanForward(&r, n);
        return r;
    }
    return 16;
}
#endif /* _MSC_VER >= 1400 */

#if defined(__GNUC__) && !HL_DISABLE_INTRIN
HL_ALWAYS_INLINE static hl_size_t hl_bits_ctz32_gcc(int32_t n)
{
    if (n) {
        return __builtin_ctz(n);
    }
    return 32;
}
HL_ALWAYS_INLINE static hl_size_t hl_bits_ctz16_gcc(int16_t n)
{
    if (n) {
        return __builtin_ctzs(n);
    }
    return 16;
}
#endif

extern hl_size_t (*hl_bits_ctz32)(int32_t n);
extern hl_size_t (*hl_bits_ctz16)(int16_t n);

HL_ALWAYS_INLINE static int32_t hl_bits_bswap32_cpp(int32_t n)
{
    const uint8_t* ptr = (const uint8_t*)&n;
    return (((int32_t)(ptr)[0])<<24 | ((int32_t)(ptr)[1])<<16 | ((int32_t)(ptr)[2])<<8 | (int32_t)(ptr)[3]);
}

HL_ALWAYS_INLINE static int16_t hl_bits_bswap16_cpp(int16_t n)
{
    const uint8_t* ptr = (const uint8_t*)&n;
    return (((int16_t)(ptr)[0])<<8 | (int16_t)(ptr)[1]);
}

#if (_MSC_VER >= 1400) && !HL_DISABLE_INTRIN
#	pragma intrinsic(_BitScanForward)
HL_ALWAYS_INLINE static int32_t hl_bits_bswap32_mvs(int32_t n)
{
    return _byteswap_ulong(n);
}
HL_ALWAYS_INLINE static int16_t hl_bits_bswap16_mvs(int16_t n)
{
    return _byteswap_ushort(n);
}
#endif /* _MSC_VER >= 1400 */

#if defined(__GNUC__)
HL_ALWAYS_INLINE static int32_t hl_bits_bswap32_gcc(int32_t n)
{
    return __builtibswap32(n);
}
HL_ALWAYS_INLINE static int16_t hl_bits_bswap16_gcc(int16_t n)
{
    return __builtin_bswap16(n);
}
#endif /* __GNUC__ */

extern int32_t (*hl_bits_bswap32)(int32_t n);
extern int16_t (*hl_bits_bswap16)(int16_t n);

HL_ALWAYS_INLINE static int32_t hl_bits_bittest32_cpp(int32_t n, int32_t p)
{
    return (n & (1 << p)) ? 1 : 0;
}

extern int32_t (*hl_bits_bittest32)(int32_t n, int32_t p);

HL_ERROR_T hl_bits_init_funcs();

HL_END_DECLS

#endif /* _HARTALLO_BITS_H_ */
