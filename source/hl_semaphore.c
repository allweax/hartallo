#include "hartallo/hl_semaphore.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_string.h"
#include "hartallo/hl_debug.h"

/* Apple claims that they fully support POSIX semaphore but ... */
#if HL_UNDER_APPLE /* Mac OSX/Darwin/Iphone/Ipod Touch */
#	define HL_USE_NAMED_SEM	1
#else
#	define HL_USE_NAMED_SEM	0
#endif

#if HL_UNDER_WINDOWS /* Windows XP/Vista/7/CE */
#	include <windows.h>
#	include "hartallo/hl_errno.h"
#	define SEMAPHORE_S void
typedef HANDLE	SEMAPHORE_T;
#	if HL_UNDER_WINDOWS_RT
#		if !defined(CreateSemaphoreEx)
#			define CreateSemaphoreEx CreateSemaphoreExW
#		endif
#	endif
//#else if define(__APPLE__) /* Mac OSX/Darwin/Iphone/Ipod Touch */
//#	include <march/semaphore.h>
//#	include <march/task.h>
#else /* All *nix */

#	include <pthread.h>
#	include <semaphore.h>
#	if HL_USE_NAMED_SEM
#	include <fcntl.h> /* O_CREAT */
#	include <sys/stat.h> /* S_IRUSR, S_IWUSR*/

static int sem_count = 0;
typedef struct named_sem_s {
    sem_t* sem;
    char* name;
} named_sem_t;
#		define SEMAPHORE_S named_sem_t
#		define GET_SEM(PSEM) (((named_sem_t*)(PSEM))->sem)
#	else
#		define SEMAPHORE_S sem_t
#		define GET_SEM(PSEM) ((PSEM))
#	endif /* HL_USE_NAMED_SEM */
typedef sem_t* SEMAPHORE_T;

#endif

#if defined(__GNUC__) || defined(__SYMBIAN32__)
#	include <errno.h>
#endif



hl_semaphore_handle_t* hl_semaphore_create()
{
    return hl_semaphore_create_2(0);
}

hl_semaphore_handle_t* hl_semaphore_create_2(int initial_val)
{
    SEMAPHORE_T handle = hl_null;

#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
    handle = CreateSemaphoreEx(NULL, initial_val, 0x7FFFFFFF, NULL, 0x00000000, SEMAPHORE_ALL_ACCESS);
#	else
    handle = CreateSemaphore(NULL, initial_val, 0x7FFFFFFF, NULL);
#	endif
#else
    handle = hl_memory_calloc(1, sizeof(SEMAPHORE_S));

#if HL_USE_NAMED_SEM
    named_sem_t * nsem = (named_sem_t*)handle;
    hl_sprintf(&(nsem->name), "/sem-%d", sem_count++);
    if ((nsem->sem = sem_open(nsem->name, O_CREAT /*| O_EXCL*/, S_IRUSR | S_IWUSR, initial_val)) == SEM_FAILED) {
        HL_MEMORY_FREE(nsem->name);
#else
    if (sem_init((SEMAPHORE_T)handle, 0, initial_val)) {
#endif
        HL_MEMORY_FREE(handle);
        HL_DEBUG_ERROR("Failed to initialize the new semaphore (errno=%d).", errno);
    }
#endif

    if (!handle) {
        HL_DEBUG_ERROR("Failed to create new semaphore");
    }
    return handle;
}

int hl_semaphore_increment(hl_semaphore_handle_t* handle)
{
    int ret = EINVAL;
    if (handle) {
#if HL_UNDER_WINDOWS
        if ((ret = ReleaseSemaphore((SEMAPHORE_T)handle, 1L, NULL) ? 0 : -1)) {
#else
        if ((ret = sem_post((SEMAPHORE_T)GET_SEM(handle)))) {
#endif
            HL_DEBUG_ERROR("sem_post function failed: %d", ret);
        }
    }
    return ret;
}

int hl_semaphore_decrement(hl_semaphore_handle_t* handle)
{
    int ret = EINVAL;
    if (handle) {
#if HL_UNDER_WINDOWS
#	   if HL_UNDER_WINDOWS_RT
        ret = (WaitForSingleObjectEx((SEMAPHORE_T)handle, INFINITE, TRUE) == WAIT_OBJECT_0) ? 0 : -1;
#	   else
        ret = (WaitForSingleObject((SEMAPHORE_T)handle, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
#endif
        if(ret)	{
            HL_DEBUG_ERROR("sem_wait function failed: %d", ret);
        }
#else
        do {
            ret = sem_wait((SEMAPHORE_T)GET_SEM(handle));
        }
        while ( errno == EINTR );
        if (ret)	{
            HL_DEBUG_ERROR("sem_wait function failed: %d", errno);
        }
#endif
    }

    return ret;
}

void hl_semaphore_destroy(hl_semaphore_handle_t** handle)
{
    if (handle && *handle) {
#if HL_UNDER_WINDOWS
        CloseHandle((SEMAPHORE_T)*handle);
        *handle = hl_null;
#else
#	if HL_USE_NAMED_SEM
        named_sem_t * nsem = ((named_sem_t*)*handle);
        sem_close(nsem->sem);
        HL_MEMORY_FREE(nsem->name);
#else
        sem_destroy((SEMAPHORE_T)GET_SEM(*handle));
#endif /* HL_USE_NAMED_SEM */
        HL_MEMORY_FREE(handle);
#endif
    }
    else {
        HL_DEBUG_WARN("Cannot free an uninitialized semaphore object");
    }
}

