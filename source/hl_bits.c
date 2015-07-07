#include "hartallo/hl_bits.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_debug.h"

hl_size_t (*hl_bits_clz32)(int32_t n) = hl_bits_clz32_cpp;
hl_size_t (*hl_bits_clz16)(int16_t n) = hl_bits_clz16_cpp;
hl_size_t (*hl_bits_ctz32)(int32_t n) = hl_bits_ctz32_cpp;
hl_size_t (*hl_bits_ctz16)(int16_t n) = hl_bits_ctz16_cpp;
int32_t (*hl_bits_bswap32)(int32_t n) = hl_bits_bswap32_cpp;
int16_t (*hl_bits_bswap16)(int16_t n) = hl_bits_bswap16_cpp;
int32_t (*hl_bits_bittest32)(int32_t n, int32_t p) = hl_bits_bittest32_cpp;

HL_ERROR_T hl_bits_init_funcs()
{
    HL_DEBUG_INFO("Initializing bit functions");

    hl_bits_clz32 = hl_bits_clz32_cpp;
    hl_bits_clz16 = hl_bits_clz16_cpp;
    hl_bits_ctz32 = hl_bits_ctz32_cpp;
    hl_bits_ctz16 = hl_bits_ctz16_cpp;
    hl_bits_bswap32 = hl_bits_bswap32_cpp;
    hl_bits_bswap16 = hl_bits_bswap16_cpp;
    hl_bits_bittest32 = hl_bits_bittest32_cpp;

#if (_MSC_VER >= 1400) && !HL_DISABLE_INTRIN
    hl_bits_clz32 = hl_bits_clz32_mvs;
    hl_bits_clz16 = hl_bits_clz16_mvs;
    hl_bits_ctz32 = hl_bits_ctz32_mvs;
    hl_bits_ctz16 = hl_bits_ctz16_mvs;
    hl_bits_bswap32 = hl_bits_bswap32_mvs;
    hl_bits_bswap16 = hl_bits_bswap16_mvs;
#endif

#if (_MSC_VER >= 1500) && !HL_DISABLE_INTRIN
    if (hl_cpu_flags_test(kCpuFlagLZCNT)) {
        hl_bits_clz32 = hl_bits_clz32_mvs_lzcnt;
    }
#endif

#if defined(__GNUC__) && !HL_DISABLE_INTRIN
    hl_bits_clz32 = hl_bits_clz32_gcc;
    hl_bits_clz16 = hl_bits_clz16_gcc;
    hl_bits_ctz32 = hl_bits_ctz32_gcc;
    hl_bits_ctz16 = hl_bits_ctz16_gcc;
    hl_bits_bswap32 = hl_bits_bswap32_gcc;
    hl_bits_bswap16 = hl_bits_bswap16_gcc;
#endif

#if HL_HAVE_X86_ASM
    {
        extern hl_size_t hl_bits_clz32_asm_x86(int32_t n);
        extern hl_size_t hl_bits_clz16_asm_x86(int16_t n);
        extern hl_size_t hl_bits_ctz32_asm_x86(int32_t n);
        extern hl_size_t hl_bits_ctz16_asm_x86(int16_t n);
        extern int32_t hl_bits_bswap32_asm_x86(int32_t n);
        extern int32_t hl_bits_bittest32_asm_x86(int32_t n, int32_t p);
        hl_bits_clz32 = hl_bits_clz32_asm_x86;
        hl_bits_clz16 = hl_bits_clz16_asm_x86;
        hl_bits_ctz32 = hl_bits_ctz32_asm_x86;
        hl_bits_ctz16 = hl_bits_ctz16_asm_x86;
        hl_bits_bswap32 = hl_bits_bswap32_asm_x86;
        hl_bits_bittest32 = hl_bits_bittest32_asm_x86;
    }
#endif

    return HL_ERROR_SUCCESS;
}
