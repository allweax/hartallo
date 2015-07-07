/*
* Copyright (C) 2013 Mamadou DIOP
* Copyright (C) 2013 Doubango Telecom <http://www.doubango.org>
* License: GPLv3
* This file is part of the open source Hartallo SDK <https://code.google.com/p/hartallo/>
*/
/* CPUID code comes from LIBYUV project (https://code.google.com/p/libyuv). See COPYRIGHT file. */

#ifndef _HARTALLO_CPU_H_
#define _HARTALLO_CPU_H_

#include "hl_config.h"

#define kCpuFlagNone  (int64_t)0
#define kCpuFlagAll (int64_t)-1

// Internal flag to indicate cpuid requires initialization.
#if defined(_MSC_VER) // C2099
#define	kCpuInit	kCpuFlagNone
#else
static const int64_t kCpuInit = kCpuFlagNone;
#endif

// These flags are only valid on ARM processors.
static const int64_t kCpuFlagARM = ((int64_t)1 << 0);
static const int64_t kCpuFlagNEON = ((int64_t)1 << 1);
// 0x8 reserved for future ARM flag.

// These flags are only valid on x86/x64 processors.
static const int64_t kCpuFlagX86 = ((int64_t)1 << 2);
static const int64_t kCpuFlagX64 = ((int64_t)1 << 3);
static const int64_t kCpuFlagMMX = ((int64_t)1 << 4);
static const int64_t kCpuFlagSSE = ((int64_t)1 << 5);
static const int64_t kCpuFlagSSE2 = ((int64_t)1 << 6);
static const int64_t kCpuFlagSSE3 = ((int64_t)1 << 7);
static const int64_t kCpuFlagSSSE3 = ((int64_t)1 << 8);
static const int64_t kCpuFlagSSE41 = ((int64_t)1 << 9);
static const int64_t kCpuFlagSSE42 = ((int64_t)1 << 10);
static const int64_t kCpuFlagSSE4a = ((int64_t)1 << 11);
static const int64_t kCpuFlagAVX = ((int64_t)1 << 12);
static const int64_t kCpuFlagAVX2 = ((int64_t)1 << 13);
static const int64_t kCpuFlagFMA3 = ((int64_t)1 << 14);
static const int64_t kCpuFlagFMA4 = ((int64_t)1 << 15);
static const int64_t kCpuFlagERMS = ((int64_t)1 << 16);
static const int64_t kCpuFlagXOP = ((int64_t)1 << 17);
static const int64_t kCpuFlagLZCNT = ((int64_t)1 << 18); /* Should be present on AMD only */


// These flags are only valid on MIPS processors.
static const int64_t kCpuFlagMIPS = ((int64_t)1 << 30);
static const int64_t kCpuFlagMIPS_DSP = ((int64_t)1 << 31);
static const int64_t kCpuFlagMIPS_DSPR2 = ((int64_t)1 << 32);

// Internal function used to auto-init.
int64_t hl_cpu_flags_init(void);

// Internal function for parsing /proc/cpuinfo.
int64_t hl_cpu_caps_arm(const char* cpuinfo_name);

// Detect CPU has SSE2 etc.
// Test_flag parameter should be one of kCpuFlag constants above.
// returns non-zero if instruction set is detected
HL_ALWAYS_INLINE static int64_t hl_cpu_flags_test(int64_t test_flag)
{
    HARTALLO_GEXTERN int64_t cpu_info_;
    return (cpu_info_ & test_flag);
}

// For testing, allow CPU flags to be disabled.
// ie hl_cpu_flags_mask(~kCpuFlagSSSE3) to disable SSSE3.
// hl_cpu_flags_mask(kCpuFlagAll) to enable all cpu specific optimizations.
// hl_cpu_flags_mask(kCpuFlagNone) to disable all cpu specific optimizations.
HARTALLO_API void hl_cpu_flags_enable(int64_t flags);

// Low level cpuid for X86. Returns zeros on other CPUs.
HARTALLO_API void hl_cpu_id(int cpu_info[4], int info_type);

// Returns a comma-separated string with CPU flags (friendly name)
HARTALLO_API const char* hl_cpu_flags_names();

HARTALLO_API int32_t hl_cpu_get_cores_count();
HARTALLO_API uint64_t hl_cpu_get_cycles_count_global();
HARTALLO_API uint64_t hl_cpu_get_time_process();
HARTALLO_API int32_t hl_cpu_get_cache_line_size();

#endif /* _HARTALLO_CPU_H_ */
