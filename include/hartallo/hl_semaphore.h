#ifndef _HARTALLO_SEMAPHORE_H_
#define _HARTALLO_SEMAPHORE_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef void hl_semaphore_handle_t;

hl_semaphore_handle_t* hl_semaphore_create();
hl_semaphore_handle_t* hl_semaphore_create_2(int initial_val);
int hl_semaphore_increment(hl_semaphore_handle_t* handle);
int hl_semaphore_decrement(hl_semaphore_handle_t* handle);
void hl_semaphore_destroy(hl_semaphore_handle_t** handle);

HL_END_DECLS

#endif /* _HARTALLO_SEMAPHORE_H_ */
