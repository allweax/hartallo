#ifndef _HARTALLO_MUTEX_H_
#define _HARTALLO_MUTEX_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef void hl_mutex_handle_t;

hl_mutex_handle_t* hl_mutex_create();
hl_mutex_handle_t* hl_mutex_create_2(hl_bool_t recursive);
int hl_mutex_lock(hl_mutex_handle_t* handle);
int hl_mutex_unlock(hl_mutex_handle_t* handle);
void hl_mutex_destroy(hl_mutex_handle_t** handle);

HL_END_DECLS

#endif /* _HARTALLO_MUTEX_H_ */
