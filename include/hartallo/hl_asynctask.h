#ifndef _HARTALLO_ASYNCTASK_H_
#define _HARTALLO_ASYNCTASK_H_

#include "hl_config.h"
#include "hartallo/hl_object.h"

HL_BEGIN_DECLS

struct hl_asynctask_s;;

typedef int32_t hl_asynctoken_id_t;

typedef struct hl_asynctoken_param_xs {
    const void* pc_param_ptr;
    hl_size_t u_param_size;
}
hl_asynctoken_param_xt;

#if !defined(HL_ASYNCTOKEN_ID_INVALID)
#	define HL_ASYNCTOKEN_ID_INVALID		-1
#endif /* HL_ASYNCTOKEN_ID_INVALID */

#define HL_ASYNCTASK_GET_PARAM(param_ptr, type) *((type*)(param_ptr))
#define HL_ASYNCTASK_GET_PARAM_PTR(param_ptr, type) *(&((type)(param_ptr)))
#define HL_ASYNCTASK_GET_PARAM_STATIC_ARRAY(param_ptr, type, w, h) *((type (**)[w][h])(param_ptr))
#define HL_ASYNCTASK_SET_PARAM(param_ptr)	(const void*)(&(param_ptr))

#define HL_ASYNCTASK_SET_PARAM_VAL(param)		(const void*)(&(param))
#define HL_ASYNCTASK_SET_PARAM_NULL()			HL_NULL

typedef HL_ERROR_T (*hl_asynctoken_f)(const struct hl_asynctoken_param_xs* pc_params);

HARTALLO_API HL_ERROR_T hl_asynctask_create(struct hl_asynctask_s** pp_asynctask);
HARTALLO_API HL_ERROR_T hl_asynctask_start(struct hl_asynctask_s* p_self);
HARTALLO_API HL_ERROR_T hl_asynctask_set_affinity(struct hl_asynctask_s* p_self, int32_t core_id);
HARTALLO_API HL_ERROR_T hl_asynctask_take_token(struct hl_asynctask_s* p_self, hl_asynctoken_id_t* pi_token);
HARTALLO_API HL_ERROR_T hl_asynctask_release_token(struct hl_asynctask_s* p_self, hl_asynctoken_id_t* pi_token);
HARTALLO_API HL_ERROR_T hl_asynctask_token_set_param(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, int32_t param_index, const void* param_ptr, hl_size_t param_size);
HARTALLO_API HL_ERROR_T hl_asynctask_token_set_params(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, hl_asynctoken_f f_func, ...);
HARTALLO_API HL_ERROR_T hl_asynctask_execute(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, hl_asynctoken_f f_func, ...);
HARTALLO_API HL_ERROR_T hl_asynctask_wait(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, uint64_t u_timeout);
HARTALLO_API HL_ERROR_T hl_asynctask_wait_2(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id);
HARTALLO_API HL_ERROR_T hl_asynctask_stop(struct hl_asynctask_s* p_self);

HL_END_DECLS

#endif /* _HARTALLO_ASYNCTASK_H_ */

