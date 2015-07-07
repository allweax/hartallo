#ifndef _HARTALLO_THREAD_H_
#define _HARTALLO_THREAD_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef void hl_thread_handle_t;
#if HL_UNDER_WINDOWS
typedef unsigned long hl_thread_id_t;
#	define HL_THREAD_PRIORITY_LOW				THREAD_PRIORITY_LOWEST
#	define HL_THREAD_PRIORITY_MEDIUM			THREAD_PRIORITY_NORMAL
#	define HL_THREAD_PRIORITY_HIGH				THREAD_PRIORITY_HIGHEST
#	define HL_THREAD_PRIORITY_TIME_CRITICAL	THREAD_PRIORITY_TIME_CRITICAL
#else
#	include <pthread.h>
#	include <sched.h>
typedef pthread_t hl_thread_id_t;
#	define HL_THREAD_PRIORITY_LOW			sched_get_priority_min(SCHED_OTHER)
#	define HL_THREAD_PRIORITY_TIME_CRITICAL		sched_get_priority_max(SCHED_OTHER)
#	define HL_THREAD_PRIORITY_MEDIUM				((HL_THREAD_PRIORITY_TIME_CRITICAL - HL_THREAD_PRIORITY_LOW) >> 1)
#	define HL_THREAD_PRIORITY_HIGH					((HL_THREAD_PRIORITY_MEDIUM * 3) >> 1)
#endif

void hl_thread_sleep(uint64_t ms);
int hl_thread_create(hl_thread_handle_t** handle, void *(HL_STDCALL *start) (void *), void *arg);
int hl_thread_set_priority(hl_thread_handle_t* handle, int32_t priority);
int hl_thread_set_priority_2(int32_t priority);
hl_thread_id_t hl_thread_get_id();
int hl_thread_set_affinity(hl_thread_handle_t* handle, int32_t core_id);
int32_t hl_thread_get_core_id();
hl_bool_t hl_thread_id_equals(hl_thread_id_t* id_1, hl_thread_id_t *id_2);
int hl_thread_destroy(hl_thread_handle_t** handle);
int hl_thread_join(hl_thread_handle_t** handle);

HL_END_DECLS

#endif /* _HARTALLO_THREAD_H_ */
