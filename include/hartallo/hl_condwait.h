#ifndef _HARTALLO_CONDWAIT_H_
#define _HARTALLO_CONDWAIT_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef void hl_condwait_handle_t;

hl_condwait_handle_t* hl_condwait_create();
int hl_condwait_wait(hl_condwait_handle_t* handle);
int hl_condwait_timedwait(hl_condwait_handle_t* handle, uint64_t ms);
int hl_condwait_signal(hl_condwait_handle_t* handle);
int hl_condwait_broadcast(hl_condwait_handle_t* handle);
void hl_condwait_destroy(hl_condwait_handle_t** handle);

HL_END_DECLS

#endif /* _HARTALLO_CONDWAIT_H_ */
