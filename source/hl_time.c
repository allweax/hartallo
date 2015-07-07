#include "hartallo/hl_time.h"
#include "hartallo/hl_debug.h"

#if HL_UNDER_WINDOWS
#	include <Winsock2.h> // timeval
#	include <windows.h>
#elif defined(__SYMBIAN32__)
#	include <_timeval.h>
#else
#	include <sys/time.h>
#endif

#include <time.h>
#if defined (__APPLE__)
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#endif

/**@defgroup hl_time_group Datetime functions.
*/

#if !HAVE_GETTIMEOFDAY
#if HL_UNDER_WINDOWS

/* For windows implementation of "gettimeofday" Thanks to "http://www.cpp-programming.net/c-tidbits/gettimeofday-function-for-windows" */
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif

struct timezone {
    int  tz_minuteswest; // minutes W of Greenwich
    int  tz_dsttime;     // type of dst correction
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    uint64_t tmpres = 0;
    static int tzflag = 0;

    if(tv) {
#ifdef _WIN32_WCE
        SYSTEMTIME st;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
#else
        GetSystemTimeAsFileTime(&ft);
#endif

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres /= 10;  /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (tz) {
        if (!tzflag) {
#if !HL_UNDER_WINDOWS_RT
            _tzset();
#endif
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

#else
#pragma error "You MUST provide an implement for 'gettimeofday'"
#endif /* WIN32 */

#endif /* !HAVE_GETTIMEOFDAY */

int hl_gettimeofday(struct timeval *tv, struct timezone *tz)
{
    return gettimeofday(tv, tz);
}

uint64_t hl_gettimeofday_ms()
{
    struct timeval tv;
    hl_gettimeofday(&tv, hl_null);
    return (((uint64_t)tv.tv_sec)*(uint64_t)1000) + (((uint64_t)tv.tv_usec)/(uint64_t)1000);
}

uint64_t hl_time_get_ms(const struct timeval* tv)
{
    if(!tv) {
        HL_DEBUG_ERROR("Invalid parameter");
        return 0;
    }
    return (((uint64_t)tv->tv_sec)*(uint64_t)1000) + (((uint64_t)tv->tv_usec)/(uint64_t)1000);
}

uint64_t hl_time_epoch()
{
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *)hl_null);
    return (((uint64_t)tv.tv_sec)*(uint64_t)1000) + (((uint64_t)tv.tv_usec)/(uint64_t)1000);
}

// /!\ NOT CURRENT TIME
// only make sense when comparing two values (e.g. for duration)
uint64_t hl_time_now()
{
#if HL_UNDER_WINDOWS
    static int __cpu_count = 0;
    if(__cpu_count == 0) {
        SYSTEM_INFO SystemInfo;
#	if HL_UNDER_WINDOWS_RT
        GetNativeSystemInfo(&SystemInfo);
#	else
        GetSystemInfo(&SystemInfo);
#	endif
        __cpu_count = SystemInfo.dwNumberOfProcessors;
    }
    if(__cpu_count == 1) {
        static LARGE_INTEGER __liFrequency = {0};
        LARGE_INTEGER liPerformanceCount;
        if(!__liFrequency.QuadPart) {
            QueryPerformanceFrequency(&__liFrequency);
        }
        QueryPerformanceCounter(&liPerformanceCount);
        return (uint64_t)(((double)liPerformanceCount.QuadPart/(double)__liFrequency.QuadPart)*1000.0);
    }
    else {
#	if HL_UNDER_WINDOWS_RT
        return hl_time_epoch();
#	else
        return timeGetTime();
#	endif
    }
#elif HAVE_CLOCK_GETTIME || _POSIX_TIMERS > 0
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (((uint64_t)ts.tv_sec)*(uint64_t)1000) + (((uint64_t)ts.tv_nsec)/(uint64_t)1000000);
#elif defined(__APPLE__)
    static mach_timebase_info_data_t __apple_timebase_info = {0, 0};
    if (__apple_timebase_info.denom == 0) {
        (void) mach_timebase_info(&__apple_timebase_info);
    }
    return (uint64_t)((mach_absolute_time() * __apple_timebase_info.numer) / (1e+6 * __apple_timebase_info.denom));
#else
    struct timeval tv;
    gettimeofday(&tv, hl_null);
    return (((uint64_t)tv.tv_sec)*(uint64_t)1000) + (((uint64_t)tv.tv_usec)/(uint64_t)1000);
#endif
}

// http://en.wikipedia.org/wiki/Network_Time_Protocol
uint64_t hl_time_ntp()
{
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *)hl_null);
    return hl_time_get_ntp_ms(&tv);
}

uint64_t hl_time_get_ntp_ms(const struct timeval *tv)
{
    static const unsigned long __epoch = 2208988800UL;
    static const double __ntp_scale_frac = 4294967295.0;

    uint64_t tv_ntp;
    uint64_t tv_usecs;

    if(!tv) {
        HL_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    tv_ntp = tv->tv_sec + __epoch;
#if 0 // ARM floating point calc issue (__aeabi_d2uiz)
    tv_usecs = (tv->tv_usec * 1e-6) * __ntp_scale_frac;
    return ((tv_ntp << 32) | (uint32_t)tv_usecs);
#else
    tv_usecs = ((uint64_t)tv->tv_usec * (uint64_t)__ntp_scale_frac) / (uint64_t)1000000;
    return ((((uint64_t)tv_ntp) << 32) | (uint32_t)tv_usecs);
#endif
}
