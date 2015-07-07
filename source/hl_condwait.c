#include "hartallo/hl_condwait.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"
#include "hartallo/hl_time.h"
#include "hartallo/hl_mutex.h"
#include "hartallo/hl_errno.h"
#include <time.h>

#if HL_UNDER_WINDOWS
#	include <windows.h>
#	define CONDWAIT_S void
typedef HANDLE	CONDWAIT_T;
#	define TIMED_OUT	WAIT_TIMEOUT
#else
#	include <sys/time.h>
#	include <pthread.h>
#	define CONDWAIT_S pthread_cond_t
typedef CONDWAIT_S* CONDWAIT_T;
#	define TIMED_OUT	ETIMEDOUT
#endif

typedef struct hl_condwait_s {
    CONDWAIT_T pcond; /**< Pthread handle pointing to the internal condwait object. */
#if !HL_UNDER_WINDOWS
    hl_mutex_handle_t* mutex;  /**< Locker. */
#endif
}
hl_condwait_t;

hl_condwait_handle_t* hl_condwait_create()
{
    hl_condwait_t *condwait = (hl_condwait_t*)hl_memory_calloc(1, sizeof(hl_condwait_t));

    if (condwait) {
#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
        condwait->pcond = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
#else
        condwait->pcond = CreateEvent(NULL, TRUE, FALSE, NULL);
#	endif
        if (!condwait->pcond) {
            HL_MEMORY_FREE(condwait);
        }
#else
        condwait->pcond = (CONDWAIT_T)hl_memory_calloc(1, sizeof(CONDWAIT_S));
        if (pthread_cond_init(condwait->pcond, 0)) {
            HL_DEBUG_ERROR("Failed to initialize the new conwait.");
        }

        if(!(condwait->mutex = hl_mutex_create())) {
            pthread_cond_destroy(condwait->pcond);

            HL_MEMORY_FREE(condwait);
            HL_DEBUG_ERROR("Failed to initialize the internal mutex.");
        }
#endif
    }

    if (!condwait) {
        HL_DEBUG_ERROR("Failed to create new conwait.");
    }
    return condwait;
}

int hl_condwait_wait(hl_condwait_handle_t* handle)
{
    int ret = EINVAL;
    hl_condwait_t *condwait = (hl_condwait_t*)handle;
    if (!condwait) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
    if((ret = (WaitForSingleObjectEx(condwait->pcond, INFINITE, TRUE) == WAIT_FAILED) ? -1 : 0)) {
#	else
    if((ret = (WaitForSingleObject(condwait->pcond, INFINITE) == WAIT_FAILED) ? -1 : 0)) {
#endif
        HL_DEBUG_ERROR("WaitForSingleObject function failed: %d", ret);
    }
#else
    if(condwait && condwait->mutex) {
        hl_mutex_lock(condwait->mutex);
        if((ret = pthread_cond_wait(condwait->pcond, (pthread_mutex_t*)condwait->mutex))) {
            HL_DEBUG_ERROR("pthread_cond_wait function failed: %d", ret);
        }
        hl_mutex_unlock(condwait->mutex);
    }
#endif
    return ret;
}

int hl_condwait_timedwait(hl_condwait_handle_t* handle, uint64_t ms)
{
#if HL_UNDER_WINDOWS
    DWORD ret = WAIT_FAILED;
#else
    int ret = EINVAL;
#endif
    hl_condwait_t *condwait = (hl_condwait_t*)handle;

#if HL_UNDER_WINDOWS
#	   if HL_UNDER_WINDOWS_RT
    if((ret = WaitForSingleObjectEx(condwait->pcond, (DWORD)ms, TRUE)) != WAIT_OBJECT_0) {
#	   else
    if((ret = WaitForSingleObject(condwait->pcond, (DWORD)ms)) != WAIT_OBJECT_0) {
#endif
        if(ret == TIMED_OUT) {
            /* HL_DEBUG_INFO("WaitForSingleObject function timedout: %d", ret); */
        }
        else {
            HL_DEBUG_ERROR("WaitForSingleObject function failed: %d", ret);
        }
        return ((ret == TIMED_OUT) ? 0 : ret);
    }
#else
    if(condwait && condwait->mutex) {
        struct timespec   ts = {0, 0};
        struct timeval    tv = {0, 0};
        /*int rc =*/  hl_gettimeofday(&tv, 0);

        ts.tv_sec  = ( tv.tv_sec + ((long)ms/1000) );
        ts.tv_nsec += ( (tv.tv_usec * 1000) + ((long)ms % 1000 * 1000000) );
        if(ts.tv_nsec > 999999999) {
            ts.tv_sec+=1, ts.tv_nsec = ts.tv_nsec % 1000000000;
        }

        hl_mutex_lock(condwait->mutex);
        if((ret = pthread_cond_timedwait(condwait->pcond, (pthread_mutex_t*)condwait->mutex, &ts))) {
            if(ret == TIMED_OUT) {
                /* HL_DEBUG_INFO("pthread_cond_timedwait function timedout: %d", ret); */
            }
            else {
                HL_DEBUG_ERROR("pthread_cond_timedwait function failed: %d", ret);
            }
        }

        hl_mutex_unlock(condwait->mutex);

        return ((ret == TIMED_OUT) ? 0 : ret);
    }
#endif

    return ret;
}

int hl_condwait_signal(hl_condwait_handle_t* handle)
{
    int ret = EINVAL;
    hl_condwait_t *condwait = (hl_condwait_t*)handle;
    if (!condwait) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

#if HL_UNDER_WINDOWS
    if (ret = ((SetEvent(condwait->pcond) && ResetEvent(condwait->pcond)) ? 0 : -1)) {
        ret = GetLastError();
        HL_DEBUG_ERROR("SetEvent/ResetEvent function failed: %d", ret);
    }
#else
    if (condwait && condwait->mutex) {
        hl_mutex_lock(condwait->mutex);

        if ((ret = pthread_cond_signal(condwait->pcond))) {
            HL_DEBUG_ERROR("pthread_cond_signal function failed: %d", ret);
        }
        hl_mutex_unlock(condwait->mutex);
    }
#endif
    return ret;
}


int hl_condwait_broadcast(hl_condwait_handle_t* handle)
{
    int ret = EINVAL;
    hl_condwait_t *condwait = (hl_condwait_t*)handle;
    if (!condwait) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

#if HL_UNDER_WINDOWS
    if (ret = ((SetEvent(condwait->pcond) && ResetEvent(condwait->pcond)) ? 0 : -1)) {
        ret = GetLastError();
        HL_DEBUG_ERROR("SetEvent function failed: %d", ret);
    }
#else
    if (condwait && condwait->mutex) {
        hl_mutex_lock(condwait->mutex);
        if((ret = pthread_cond_broadcast(condwait->pcond))) {
            HL_DEBUG_ERROR("pthread_cond_broadcast function failed: %d", ret);
        }
        hl_mutex_unlock(condwait->mutex);
    }
#endif

    return ret;
}

void hl_condwait_destroy(hl_condwait_handle_t** handle)
{
    hl_condwait_t **condwait = (hl_condwait_t**)handle;

    if (condwait && *condwait) {
#if HL_UNDER_WINDOWS
        CloseHandle((*condwait)->pcond);
#else
        hl_mutex_destroy(&((*condwait)->mutex));
        pthread_cond_destroy((*condwait)->pcond);
        HL_MEMORY_FREE((*condwait)->pcond);
#endif
        HL_MEMORY_FREE(condwait);
    }
    else {
        HL_DEBUG_WARN("Cannot free an uninitialized condwait object");
    }
}

