#include "hartallo/hl_mutex.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"
#include "hartallo/hl_errno.h"

#if HL_UNDER_WINDOWS
#	include <windows.h>
typedef HANDLE	MUTEX_T;
#	define MUTEX_S void
#	define HL_ERROR_NOT_OWNER ERROR_NOT_OWNER
#else
#	include <pthread.h>
#	define MUTEX_S pthread_mutex_t
typedef MUTEX_S* MUTEX_T;
#	define HL_ERROR_NOT_OWNER EPERM
#   if !defined(HL_RECURSIVE_MUTEXATTR)
#       if defined(PTHREAD_MUTEX_RECURSIVE)
#           define HL_RECURSIVE_MUTEXATTR PTHREAD_MUTEX_RECURSIVE
#       else
#           define HL_RECURSIVE_MUTEXATTR PTHREAD_MUTEX_RECURSIVE_NP
#       endif
#   endif /* HL_RECURSIVE_MUTEXATTR */
#endif

hl_mutex_handle_t* hl_mutex_create()
{
    return hl_mutex_create_2(hl_true);
}

hl_mutex_handle_t* hl_mutex_create_2(hl_bool_t recursive)
{
    MUTEX_T handle = hl_null;

#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
    handle = CreateMutexEx(NULL, NULL, 0x00000000, MUTEX_ALL_ACCESS);
#	else
    handle = CreateMutex(NULL, FALSE, NULL);
#	endif
#else
    int ret;
    pthread_mutexattr_t   mta;

    if ((ret = pthread_mutexattr_init(&mta))) {
        HL_DEBUG_ERROR("pthread_mutexattr_init failed with error code %d", ret);
        return hl_null;
    }
    if (recursive && (ret = pthread_mutexattr_settype(&mta, HL_RECURSIVE_MUTEXATTR))) {
        HL_DEBUG_ERROR("pthread_mutexattr_settype failed with error code %d", ret);
        pthread_mutexattr_destroy(&mta);
        return hl_null;
    }

    /* if we are here: all is ok */
    handle = hl_memory_calloc(1, sizeof(MUTEX_S));
    if (pthread_mutex_init((MUTEX_T)handle, &mta)) {
        HL_MEMORY_FREE(handle);
    }
    pthread_mutexattr_destroy(&mta);
#endif

    if (!handle) {
        HL_DEBUG_ERROR("Failed to create new mutex.");
    }
    return handle;
}

int hl_mutex_lock(hl_mutex_handle_t* handle)
{
    int ret = EINVAL;
    if (handle) {

#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
        if ((ret = WaitForSingleObjectEx((MUTEX_T)handle, INFINITE, TRUE)) == WAIT_FAILED)
#	else
        if ((ret = WaitForSingleObject((MUTEX_T)handle, INFINITE)) == WAIT_FAILED)
#endif
#else
        if ((ret = pthread_mutex_lock((MUTEX_T)handle)))
#endif
        {
            HL_DEBUG_ERROR("Failed to lock the mutex: %d", ret);
        }
    }
    return ret;
}

int hl_mutex_unlock(hl_mutex_handle_t* handle)
{
    int ret = EINVAL;
    if(handle) {
#if HL_UNDER_WINDOWS
        if((ret = ReleaseMutex((MUTEX_T)handle) ? 0 : -1)) {
            ret = GetLastError();
#else
        if((ret = pthread_mutex_unlock((MUTEX_T)handle))) {
#endif
            if(ret == HL_ERROR_NOT_OWNER) {
                HL_DEBUG_WARN("The calling thread does not own the mutex: %d", ret);
            }
            else {
                HL_DEBUG_ERROR("Failed to unlock the mutex: %d", ret);
            }
        }
    }
    return ret;
}

void hl_mutex_destroy(hl_mutex_handle_t** handle)
{
    if (handle && *handle) {
#if HL_UNDER_WINDOWS
        CloseHandle((MUTEX_T)*handle);
        *handle = hl_null;
#else
        pthread_mutex_destroy((MUTEX_T)*handle);
        HL_MEMORY_FREE(handle);
#endif
    }
    else {
        HL_DEBUG_WARN("Cannot free an uninitialized mutex");
    }
}

