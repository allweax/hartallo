/*
* Copyright (C) 2013 Mamadou DIOP
* Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
* License: GPLv3
* This file is part of the open source Hartallo SDK <https://code.google.com/p/hartallo/>
*/
/* CPUID code partially comes from LIBYUV project (https://code.google.com/p/libyuv). See COPYRIGHT file. */

#include "hartallo/hl_cpu.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_debug.h"

#if HL_UNDER_WINDOWS
#	include <windows.h>
#endif /* HL_UNDER_WINDOWS */

#if HL_UNDER_APPLE
#	include <sys/sysctl.h> /* sysctlbyname */
#endif /* HL_UNDER_APPLE */

#if defined(__GNUC__)
#	include <unistd.h> /* sysconf() */
#endif

#if defined(_MSC_VER)
#	pragma intrinsic(__rdtsc, __cpuid)
#endif

#if HL_HAVE_X86_ASM
extern uint64_t hl_utils_rdtsc_x86_asm();
#endif /* HL_HAVE_ASM_X86 */

#if (defined(__pic__) || defined(__APPLE__)) && defined(__i386__)
static __inline void __cpuid(int cpu_info[4], int info_type)
{
    asm volatile (
        "mov %%ebx, %%edi                          \n"
        "cpuid                                     \n"
        "xchg %%edi, %%ebx                         \n"
        : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(info_type));
}
#elif defined(__i386__) || defined(__x86_64__)
static __inline void __cpuid(int cpu_info[4], int info_type)
{
    asm volatile (
        "cpuid                                     \n"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(info_type));
}
#endif

// Low level cpuid for X86. Returns zeros on other CPUs.
#if !defined(__CLR_VER) && (defined(_M_IX86) || defined(_M_X64) || \
	defined(__i386__) || defined(__x86_64__))
void hl_cpu_id(int cpu_info[4], int info_type)
{
    __cpuid(cpu_info, info_type);
}
#else
void hl_cpu_id(int cpu_info[4], int)
{
    cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
}
#endif

// X86 CPUs have xgetbv to detect OS saves high parts of ymm registers.
#if !defined(__CLR_VER) && !defined(__native_client__)
#if defined(_M_X64) && defined(_MSC_VER) && (_MSC_FULL_VER >= 160040219)
#define HAS_XGETBV
static uint32 XGetBV(unsigned int xcr)
{
    return static_cast<uint32>(_xgetbv(xcr));
}
#elif !defined(__CLR_VER) && defined(_M_IX86) && defined(_MSC_VER)
#define HAS_XGETBV
HL_NAKED HL_ALIGN(HL_ALIGN_V)
static uint32_t XGetBV(unsigned int xcr)
{
    __asm {
        mov        ecx, [esp + 4]    // xcr
        push       edx
        _asm _emit 0x0f _asm _emit 0x01 _asm _emit 0xd0  // xgetbv for vs2005.
        pop        edx
        ret
    }
}
#elif defined(__i386__) || defined(__x86_64__)
#define HAS_XGETBV
static uint32_t XGetBV(unsigned int xcr)
{
    uint32_t xcr_feature_mask;
    asm volatile (  // NOLINT
        ".byte 0x0f, 0x01, 0xd0\n"
        : "=a"(xcr_feature_mask)
        : "c"(xcr)
        : "memory", "cc", "edx");  // edx unused.
    return xcr_feature_mask;
}
#endif
#endif  // !defined(__CLR_VER) && !defined(__native_client__)
#ifdef HAS_XGETBV
static const int kXCR_XFEATURE_ENABLED_MASK = 0;
#endif

// For Arm, but public to allow testing on any CPU
int64_t hl_cpu_caps_arm(const char* cpuinfo_name)
{
    FILE* f = fopen(cpuinfo_name, "r");
    if (f) {
        char buf[512];
        while (fgets(buf, 511, f)) {
            if (memcmp(buf, "Features", 8) == 0) {
                char* p = strstr(buf, " neon");
                if (p && (p[5] == ' ' || p[5] == '\n')) {
                    fclose(f);
                    return kCpuFlagNEON;
                }
            }
        }
        fclose(f);
    }
    return 0;
}

#if defined(__mips__) && defined(__linux__)
static int64_t MipsCpuCaps(const char* search_string)
{
    const char* file_name = "/proc/cpuinfo";
    char cpuinfo_line[256];
    FILE* f = NULL;
    if ((f = fopen(file_name, "r")) != NULL) {
        while (fgets(cpuinfo_line, sizeof(cpuinfo_line), f) != NULL) {
            if (strstr(cpuinfo_line, search_string) != NULL) {
                fclose(f);
                return kCpuFlagMIPS_DSP;
            }
        }
        fclose(f);
    }
    /* Did not find string in the proc file, or not Linux ELF. */
    return 0;
}
#endif

// CPU detect function for SIMD instruction sets.
HARTALLO_EXPORT int64_t cpu_info_ = kCpuInit;  // cpu_info is not initialized yet.

// Test environment variable for disabling CPU features. Any non-zero value
// to disable. Zero ignored to make it easy to set the variable on/off.
#if !defined(__native_client__)
static hl_bool_t TestEnv(const char* name)
{
    const char* var = getenv(name);
    if (var) {
        if (var[0] != '0') {
            return HL_TRUE;
        }
    }
    return HL_FALSE;
}
#else  // nacl does not support getenv().
static bool TestEnv(const char*)
{
    return false;
}
#endif

// Info on flags: http://msdn.microsoft.com/en-us/library/hskdteyh(v=vs.90).aspx
int64_t hl_cpu_flags_init(void)
{
#if !defined(__CLR_VER) && defined(HL_CPU_TYPE_X86)
    int cpu_info[4] = { 0 }, num_ids, num_ids_ex;
    cpu_info_ = kCpuFlagX86;
    __cpuid(cpu_info, 0);
    num_ids = cpu_info[0];
    __cpuid(cpu_info, 0x80000000);
    num_ids_ex = cpu_info[0];
    if (num_ids > 0) {
        __cpuid(cpu_info, 0x00000001);
        cpu_info_ |=
            ((cpu_info[3] & (1 << 23)) ? kCpuFlagMMX : 0)
            | ((cpu_info[3] & (1 << 25)) ? kCpuFlagSSE : 0)
            | ((cpu_info[3] & (1 << 26)) ? kCpuFlagSSE2 : 0)

            | ((cpu_info[2] & (1 << 0)) ? kCpuFlagSSE3 : 0)
            | ((cpu_info[2] & (1 << 9)) ? kCpuFlagSSSE3 : 0)
            | ((cpu_info[2] & (1 << 19)) ? kCpuFlagSSE41 : 0)
            | ((cpu_info[2] & (1 << 20)) ? kCpuFlagSSE42 : 0)
            | ((cpu_info[2] & (1 << 28)) ? kCpuFlagAVX : 0)
            | ((cpu_info[2] & (1 << 12)) ? kCpuFlagFMA3 : 0);
#ifdef HAS_XGETBV
        if ((cpu_info[2] & 0x18000000) == 0x18000000 &&  // AVX and OSSave
                (XGetBV(kXCR_XFEATURE_ENABLED_MASK) & 0x06) == 0x06) {  // Saves YMM.
            int cpu_info7[4] = { 0 };
            __cpuid(cpu_info7, 0x00000007);
            cpu_info_ |= ((cpu_info7[1] & 0x00000020) ? kCpuFlagAVX2 : 0) |
                         kCpuFlagAVX;
        }
#endif
    }
    if (num_ids_ex > 0x80000000) {
        __cpuid(cpu_info, 0x80000001);
        cpu_info_ |=
            ((cpu_info[3] & (1 << 29)) ? kCpuFlagX64: 0) // FIXME: not correct
            | ((cpu_info[2] & (1 << 5)) ? kCpuFlagLZCNT: 0)
            | ((cpu_info[2] & (1 << 6)) ? kCpuFlagSSE4a: 0)
            | ((cpu_info[2] & (1 << 16)) ? kCpuFlagFMA4: 0)
            | ((cpu_info[2] & (1 << 11)) ? kCpuFlagXOP: 0);
    }

    // Environment variable overrides for testing.
    if (TestEnv("HARTALLO_DISABLE_X86")) {
        cpu_info_ &= ~kCpuFlagX86;
    }
    if (TestEnv("HARTALLO_DISABLE_SSE2")) {
        cpu_info_ &= ~kCpuFlagSSE2;
    }
    if (TestEnv("HARTALLO_DISABLE_SSSE3")) {
        cpu_info_ &= ~kCpuFlagSSSE3;
    }
    if (TestEnv("HARTALLO_DISABLE_SSE41")) {
        cpu_info_ &= ~kCpuFlagSSE41;
    }
    if (TestEnv("HARTALLO_DISABLE_SSE42")) {
        cpu_info_ &= ~kCpuFlagSSE42;
    }
    if (TestEnv("HARTALLO_DISABLE_AVX")) {
        cpu_info_ &= ~kCpuFlagAVX;
    }
    if (TestEnv("HARTALLO_DISABLE_AVX2")) {
        cpu_info_ &= ~kCpuFlagAVX2;
    }
    if (TestEnv("HARTALLO_DISABLE_ERMS")) {
        cpu_info_ &= ~kCpuFlagERMS;
    }
#elif defined(__mips__) && defined(__linux__)
    // Linux mips parse text file for dsp detect.
    cpu_info_ = MipsCpuCaps("dsp");  // set kCpuFlagMIPS_DSP.
#if defined(__mips_dspr2)
    cpu_info_ |= kCpuFlagMIPS_DSPR2;
#endif
    cpu_info_ |= kCpuFlagMIPS;

    if (getenv("HARTALLO_DISABLE_MIPS")) {
        cpu_info_ &= ~kCpuFlagMIPS;
    }
    if (getenv("HARTALLO_DISABLE_MIPS_DSP")) {
        cpu_info_ &= ~kCpuFlagMIPS_DSP;
    }
    if (getenv("HARTALLO_DISABLE_MIPS_DSPR2")) {
        cpu_info_ &= ~kCpuFlagMIPS_DSPR2;
    }
#elif defined(__arm__)
#if defined(__linux__) && (defined(__ARM_NEON__) || defined(HARTALLO_NEON)) && \
	!defined(__native_client__)
    // Linux arm parse text file for neon detect.
    cpu_info_ = hl_cpu_caps_arm("/proc/cpuinfo");
#elif defined(__ARM_NEON__) || defined(__native_client__)
    // gcc -mfpu=neon defines __ARM_NEON__
    // Enable Neon if you want support for Neon and Arm, and use hl_cpu_flags_mask
    // to disable Neon on devices that do not have it.
    cpu_info_ = kCpuFlagNEON;
#endif
    cpu_info_ |= kCpuFlagARM;
    if (TestEnv("HARTALLO_DISABLE_NEON")) {
        cpu_info_ &= ~kCpuFlagNEON;
    }
#endif  // __arm__
    if (TestEnv("HARTALLO_DISABLE_ASM")) {
        cpu_info_ = 0;
    }
    return cpu_info_;
}

void hl_cpu_flags_enable(int64_t flags)
{
    cpu_info_ = hl_cpu_flags_init() & flags;
}


const char* hl_cpu_flags_names()
{
    static char __cpu_flags_names[1024] = { 0 };
    static int64_t __cpu_info = kCpuInit;

    if(__cpu_info != cpu_info_) {
        size_t i, len, j;
        struct hl_cpu_desc {
            const char* name;
            int64_t flag;
        };

#if !defined(_MSC_VER) // C2099
        static
#endif
        const struct hl_cpu_desc __hl_cpu_desc[] = {
            { "ARM", kCpuFlagARM },
            { "NEON", kCpuFlagNEON },

            { "X86", kCpuFlagX86 },
            { "X64", kCpuFlagX64 },
            { "MMX", kCpuFlagMMX },
            { "SSE", kCpuFlagSSE },
            { "SSE2", kCpuFlagSSE2 },
            { "SSE3", kCpuFlagSSE3 },
            { "SSSE3", kCpuFlagSSSE3 },
            { "SSE41", kCpuFlagSSE41 },
            { "SSE42", kCpuFlagSSE42 },
            { "SSE4a", kCpuFlagSSE4a },
            { "AVX", kCpuFlagAVX },
            { "AVX2", kCpuFlagAVX2 },
            { "FMA3", kCpuFlagFMA3 },
            { "FMA4", kCpuFlagFMA4 },
            { "ERMS", kCpuFlagERMS },
            { "XOP", kCpuFlagXOP },

            { "MIPS", kCpuFlagMIPS },
            { "MIPS_DSP", kCpuFlagMIPS_DSP },
            { "MIPS_DSPR2", kCpuFlagMIPS_DSPR2 },
        };
        static const unsigned __hl_cpu_desc_count = sizeof(__hl_cpu_desc)/sizeof(__hl_cpu_desc[0]);

        for (i = 0, j = 0; i < __hl_cpu_desc_count; ++i) {
            if(__hl_cpu_desc[i].flag & cpu_info_) {
                if(j) {
                    __cpu_flags_names[j] = ',';
                    ++j;
                }
                len = strlen(__hl_cpu_desc[i].name);
                memcpy(&__cpu_flags_names[j], __hl_cpu_desc[i].name, len);
                j += len;
            }
        }
        __cpu_flags_names[j] = '\0';
        __cpu_info = cpu_info_;
    }
    return __cpu_flags_names;
}

int32_t hl_cpu_get_cores_count()
{
    static int32_t g_cores = 0;

    if (g_cores == 0) {
#if HL_UNDER_WINDOWS
        SYSTEM_INFO SystemInfo;
#		if HL_UNDER_WINDOWS_RT
        GetNativeSystemInfo(&SystemInfo);
#		else
        GetSystemInfo(&SystemInfo);
#		endif
        g_cores = SystemInfo.dwNumberOfProcessors;
#elif HL_HAVE_OPEN_MP
        g_cores = omp_get_num_procs();
#elif defined(__APPLE__)
        size_t len = sizeof(g_cores);
        int mib[2] = { CTL_HW, HW_NCPU };
        sysctl(mib, 2, &g_cores, &len, NULL, 0);
#elif defined(__GNUC__)
        g_cores = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
#else
#	error "Not implemented"
#endif
    }
    return g_cores;
}

uint64_t hl_cpu_get_cycles_count_global()
{
#if HL_UNDER_WINDOWS
    return __rdtsc();
#elif HL_HAVE_X86_ASM
    return hl_utils_rdtsc_x86_asm();
#else
#	error "Not implemented: use rdtsc inline asm"
#endif
}

/*
Gets CPU time.
Returned value is in milliseconds
*/
uint64_t hl_cpu_get_time_process()
{
#if HL_UNDER_WINDOWS
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime)) {
        SYSTEMTIME systemTime;
        if (FileTimeToSystemTime(&userTime, &systemTime)) {
            return (uint64_t)(((systemTime.wHour * 3600) + (systemTime.wMinute * 60) + systemTime.wSecond) * 1000 + systemTime.wMilliseconds);
        }
    }
    return 0;
#else
// #	error "Not implemented: use CLOCK_PROCESS_CPUTIME_ID"
    return 0; //FIXME
#endif
}

/*
Gets the cache line size.
*/
int32_t hl_cpu_get_cache_line_size()
{
    static int32_t __cache_line_size = 0;
    if (!__cache_line_size) {
#if HL_UNDER_WINDOWS
        DWORD bs = 0;
        if (!GetLogicalProcessorInformation(0, &bs)) {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buff = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(bs);
            DWORD i;
            GetLogicalProcessorInformation(buff, &bs);
            for (i = 0; i != bs / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
                if (buff[i].Relationship == RelationCache && buff[i].Cache.Level == 1) {
                    __cache_line_size = buff[i].Cache.LineSize;
                    break;
                }
            }
            if (buff) {
                free(buff);
            }
        }
        else {
            HL_DEBUG_ERROR("GetLogicalProcessorInformation() failed with error code = %08x", GetLastError());
        }

#elif HL_UNDER_APPLE
        size_t sizeof_cls = sizeof(__cache_line_size);
        sysctlbyname("hw.cachelinesize", &__cache_line_size, &sizeof_cls, 0, 0);
#else
        __cache_line_size = (int32_t)sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#endif
    }
    return __cache_line_size;
}