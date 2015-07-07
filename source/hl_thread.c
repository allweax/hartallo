#include "hartallo/hl_thread.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_debug.h"

#if HL_UNDER_WINDOWS
#	include <windows.h>
#endif
#if HL_UNDER_WINDOWS_RT
#	include "../winrt/ThreadEmulation.h"
using namespace ThreadEmulation;
#endif

#include <string.h>

void hl_thread_sleep(uint64_t ms)
{
#if HL_UNDER_WINDOWS
    Sleep((DWORD)ms);
#else
    struct timespec interval;

    interval.tv_sec = (long)(ms/1000);
    interval.tv_nsec = (long)(ms%1000) * 1000000;
    nanosleep(&interval, 0);
#endif
}

int hl_thread_create(hl_thread_handle_t** handle, void *(HL_STDCALL *start) (void *), void *arg)
{
#if HL_UNDER_WINDOWS
    *((HANDLE*)handle) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, arg, 0, NULL);
    return *((HANDLE*)handle) ? 0 : -1;
#else
    *handle = hl_memory_calloc(1, sizeof(pthread_t));
    return pthread_create((pthread_t*)*handle, 0, start, arg);
#endif
}

int hl_thread_set_priority(hl_thread_handle_t* handle, int32_t priority)
{
    if (!handle) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
#if HL_UNDER_WINDOWS
    {
        int ret = SetThreadPriority(handle, priority) ? 0 : -1;
#if HL_UNDER_WINDOWS_RT
        // It's not possible to set priority on WP8 when thread is not in suspended state -> do nothing and don't bother us
        if(ret) {
            HL_DEBUG_INFO("SetThreadPriority() failed but nothing to worry about");
            ret = 0;
        }
#endif
        return ret;
    }
#else
    struct sched_param sp;
    int ret;
    memset(&sp, 0, sizeof(struct sched_param));
    sp.sched_priority = priority;
    if ((ret = pthread_setschedparam(*((pthread_t*)handle), SCHED_OTHER, &sp))) {
        HL_DEBUG_ERROR("Failed to change priority to %d with error code=%d", priority, ret);
        return ret;
    }
    return 0;
#endif
}

int hl_thread_set_priority_2(int32_t priority)
{
#if HL_UNDER_WINDOWS
    HL_DEBUG_ERROR("Not implemented");
    return -1;
#else
    pthread_t thread = pthread_self();
    return hl_thread_set_priority(&thread, priority);
#endif
}

hl_thread_id_t hl_thread_get_id()
{
#if HL_UNDER_WINDOWS
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

//  core_id: 0...64
int hl_thread_set_affinity(hl_thread_handle_t* handle, int32_t core_id)
{
    if (!handle || core_id < 0 || core_id > hl_cpu_get_cores_count()) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
#if HL_UNDER_WINDOWS
    {
        const DWORD_PTR mask = (((DWORD_PTR)1) << core_id);
        if (!SetThreadAffinityMask(handle, mask)) {
            HL_DEBUG_ERROR("SetThreadAffinityMask() failed");
            return -2;
        }
        return 0;
    }
#elif HL_UNDER_ANDROID
    s/*yscall(__NR_sched_setaffinity,
            pid,
            sizeof(mask),
            &mask);*/
#elif HL_UNDER_APPLE
    /* /usr/include/mach/thread_policy.h: thread_policy_set and mach_thread_self() */
    // FIXME
    HL_DEBUG_ERROR("Not implemented");
    return 0;
#else
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        return pthread_setaffinity_np((pthread_t*)handle, sizeof(cpu_set_t), &cpuset);
    }
#endif
}

// TODO: check ASM version
int32_t hl_thread_get_core_id()
{
#if _WIN32_WINNT >= 0x0600
    return (int32_t)GetCurrentProcessorNumber();
#elif _WIN32_WINNT >= 0x0601
    PROCESSOR_NUMBER ProcNumber;
    GetCurrentProcessorNumberEx(&ProcNumber);
    return  ProcNumber.Number;
#elif HL_HAVE_X86_ASM
    extern int32_t hl_utils_thread_get_core_id_x86_asm();
    return hl_utils_thread_get_core_id_x86_asm();
#else
    HL_DEBUG_WARN("Not Implemented yet");
    return 0;
#endif
}

hl_bool_t hl_thread_id_equals(hl_thread_id_t* id_1, hl_thread_id_t *id_2)
{
    if (!id_1 || !id_2) {
        HL_DEBUG_ERROR("Invalid parameter");
        return hl_false;
    }
#if HL_UNDER_WINDOWS
    return (*id_1 == *id_2);
#else
    return (pthread_equal(*id_1, *id_2) != 0);
#endif
}

int hl_thread_join(hl_thread_handle_t** handle)
{
    int ret;

    if (!handle) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if (!*handle) {
        HL_DEBUG_WARN("Cannot join NULL handle");
        return 0;
    }

#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
    ret = (WaitForSingleObjectEx(*((HANDLE*)handle), INFINITE, TRUE) == WAIT_FAILED) ? -1 : 0;
#	else
    ret = (WaitForSingleObject(*((HANDLE*)handle), INFINITE) == WAIT_FAILED) ? -1 : 0;
#endif
    if (ret == 0) {
        ret = hl_thread_destroy(handle);
    }
#else
    if ((ret = pthread_join(*((pthread_t*)*handle), 0)) == 0) {
        ret = hl_thread_destroy(handle);
    }
#endif

    return ret;
}

int hl_thread_destroy(hl_thread_handle_t** handle)
{
    if (handle && *handle) {
#if HL_UNDER_WINDOWS
        CloseHandle(*((HANDLE*)handle));
        *handle = hl_null;
#else
        HL_MEMORY_FREE(handle);
#endif
    }
    return 0;
}

