#include "hartallo/hl_asynctask.h"
#include "hartallo/hl_thread.h"
#include "hartallo/hl_time.h"
#include "hartallo/hl_semaphore.h"
#include "hartallo/hl_debug.h"

#include <limits.h> /* LONG_MAX */

#if HL_UNDER_WINDOWS
#	include <windows.h>
#endif

extern const hl_object_def_t *hl_asynctask_def_t;

static void* HL_STDCALL _hl_asynctask_run(void *p_arg);

#if !defined(HL_ASYNCTASK_MAX_TOKEN_COUNT)
#define HL_ASYNCTASK_MAX_TOKEN_COUNT			32
#endif /* HL_ASYNCTASK_MAX_TOKEN_COUNT */

#if !defined(HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT)
#define HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT		16
#endif /* HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT */

#define HL_ASYNCTOKEN_ID_IS_VALID(_id_) ((_id_) >=0 && (_id_) < HL_ASYNCTASK_MAX_TOKEN_COUNT)
#define HL_ASYNCTASK_PARAM_INDEX_IS_VALID(_id_) ((_id_) >=0 && (_id_) < HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT)

typedef struct hl_asynctoken_xs {
    hl_bool_t b_taken;
    hl_bool_t b_executing;
    hl_bool_t b_execute;
    hl_asynctoken_f f_func;
    hl_asynctoken_param_xt params[HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT];
    int32_t i_params_count; // number of active params
}
hl_asynctoken_xt;

typedef struct hl_asynctask_s {
    HL_DECLARE_OBJECT;

    hl_bool_t b_started;
    hl_thread_handle_t* ph_thread;
    hl_semaphore_handle_t* ph_sem_run;
    hl_semaphore_handle_t* ph_sem_exec;
    int32_t i_core_id;

    struct hl_asynctoken_xs tokens[HL_ASYNCTASK_MAX_TOKEN_COUNT];
    int32_t i_tokens_count; // number of active tokens
}
hl_asynctask_t;

HL_ERROR_T hl_asynctask_create(struct hl_asynctask_s** pp_asynctask)
{
    if (!pp_asynctask) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_asynctask = hl_object_create(hl_asynctask_def_t);
    if (!*pp_asynctask) {
        return HL_ERROR_OUTOFMEMMORY;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_asynctask_start(struct hl_asynctask_s* p_self)
{
    int ret;
    if (!p_self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (p_self->b_started) {
        return HL_ERROR_SUCCESS;
    }
    if (!p_self->ph_sem_run) {
        p_self->ph_sem_run = hl_semaphore_create();
        if (!p_self->ph_sem_run) {
            return HL_ERROR_SYSTEM;
        }
    }
    if (!p_self->ph_sem_exec) {
        p_self->ph_sem_exec = hl_semaphore_create();
        if (!p_self->ph_sem_exec) {
            return HL_ERROR_SYSTEM;
        }
    }
    p_self->b_started = HL_TRUE;
    ret = hl_thread_create(&p_self->ph_thread, _hl_asynctask_run, p_self);
    if (ret) {
        p_self->b_started = HL_FALSE;
        return HL_ERROR_SYSTEM;
    }
    ret = hl_thread_set_priority(p_self->ph_thread, HL_THREAD_PRIORITY_TIME_CRITICAL);
    if (ret) {
        HL_DEBUG_ERROR("Failed to set thread priority value to %d with error code = %d", HL_THREAD_PRIORITY_TIME_CRITICAL, ret);
        return HL_ERROR_SYSTEM;
    }
    ret = hl_thread_set_affinity(p_self->ph_thread, p_self->i_core_id);
    if (ret) {
        HL_DEBUG_ERROR("Failed to set thread affinity value to %d with error code = %d", p_self->i_core_id, ret);
        return HL_ERROR_SYSTEM;
    }

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_asynctask_set_affinity(struct hl_asynctask_s* p_self, int32_t core_id)
{
    if (!p_self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (p_self->ph_thread) {
        int ret = hl_thread_set_affinity(p_self->ph_thread, core_id);
        if (ret) {
            HL_DEBUG_ERROR("Failed to set thead affinity value to %d with error code = %d", p_self->i_core_id, ret);
            return HL_ERROR_SYSTEM;
        }
    }
    p_self->i_core_id = core_id;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_asynctask_take_token(struct hl_asynctask_s* p_self, hl_asynctoken_id_t* pi_token)
{
    if (!p_self || !pi_token) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (p_self->i_tokens_count >= HL_ASYNCTASK_MAX_TOKEN_COUNT) {
        HL_DEBUG_ERROR("OUTOFCAPACITY: Already %d active tokens", p_self->i_tokens_count);
        return HL_ERROR_OUTOFCAPACITY;
    }
    else {
        int32_t i;
        for (i = 0; i < HL_ASYNCTASK_MAX_TOKEN_COUNT; ++i) {
            if (!p_self->tokens[i].b_taken) {
                p_self->tokens[i].b_taken = HL_TRUE;
                *pi_token = i;
                ++p_self->i_tokens_count;
                break;
            }
        }
        return HL_ERROR_SUCCESS;
    }
}

HL_ERROR_T hl_asynctask_release_token(struct hl_asynctask_s* p_self, hl_asynctoken_id_t* pi_token)
{
    hl_asynctoken_xt* p_token;
    if (!p_self || !pi_token) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (!HL_ASYNCTOKEN_ID_IS_VALID(*pi_token)) {
        return HL_ERROR_SUCCESS;
    }
    p_token = &p_self->tokens[*pi_token];
    if (p_token->b_taken) {
        p_token->b_taken = HL_FALSE;
        p_self->i_tokens_count--;
    }
    *pi_token = HL_ASYNCTOKEN_ID_INVALID;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_asynctask_token_set_param(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, int32_t param_index, const void* param_ptr, hl_size_t param_size)
{
    hl_asynctoken_xt* p_token;
    if (!p_self || !HL_ASYNCTOKEN_ID_IS_VALID(token_id) || !HL_ASYNCTASK_PARAM_INDEX_IS_VALID(param_index)) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    p_token = &p_self->tokens[token_id];
    p_token->params[param_index].pc_param_ptr = param_ptr;
    p_token->params[param_index].u_param_size = param_size;

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_asynctask_token_set_params(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, hl_asynctoken_f f_func, ...)
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    hl_asynctoken_xt* p_token;
    va_list ap;
    const void* pc_param_ptr;
    if (!p_self || !HL_ASYNCTOKEN_ID_IS_VALID(token_id) || !f_func) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    p_token = &p_self->tokens[token_id];
    if (p_token->b_executing) {
        HL_DEBUG_ERROR("Token wit id = %d already executing", token_id);
        return HL_ERROR_INVALID_STATE;
    }
    p_token->f_func = f_func;
    p_token->i_params_count = 0;

    va_start(ap, f_func);
    while ((pc_param_ptr = va_arg(ap, const void*))) {
        if (p_token->i_params_count >= HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT) {
            HL_DEBUG_ERROR("Too many params");
            err = HL_ERROR_OUTOFCAPACITY;
            goto bail;
        }
        p_token->params[p_token->i_params_count++].pc_param_ptr = pc_param_ptr;
    }

bail:
    va_end(ap);
    return err;
}

HL_ERROR_T hl_asynctask_execute(struct hl_asynctask_s* p_self, hl_asynctoken_id_t i_token, hl_asynctoken_f f_func, ...)
{
    hl_asynctoken_xt* p_token;
    int ret;
    va_list ap;
    const void* pc_param_ptr;
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    if (!p_self || !HL_ASYNCTOKEN_ID_IS_VALID(i_token)) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (!p_self->b_started) {
        HL_DEBUG_ERROR("Not started yet");
        return HL_ERROR_INVALID_STATE;
    }
    p_token = &p_self->tokens[i_token];
    if (p_token->b_executing || p_token->b_execute) {
        HL_DEBUG_ERROR("Token with id = %d already executing or scheduled", i_token);
        return HL_ERROR_INVALID_STATE;
    }
    if (!p_token->b_taken) {
        // token was not taken -> use it with warning
        p_token->b_taken = HL_TRUE;
        ++p_self->i_tokens_count;
    }
    p_token->f_func = f_func;
    p_token->i_params_count = 0;

    va_start(ap, f_func);
    while ((pc_param_ptr = va_arg(ap, const void*))) {
        if (p_token->i_params_count >= HL_ASYNCTASK_MAX_TOKEN_PARAMS_COUNT) {
            HL_DEBUG_ERROR("Too many params");
            err = HL_ERROR_OUTOFCAPACITY;
            goto bail;
        }
        p_token->params[p_token->i_params_count++].pc_param_ptr = pc_param_ptr;
    }

    p_token->b_execute = HL_TRUE;
    ret = hl_semaphore_increment(p_self->ph_sem_run);
    if (ret) {
        p_token->b_execute = HL_FALSE;
        err = HL_ERROR_SYSTEM;
        goto bail;
    }

bail:
    va_end(ap);
    return err;
}

HL_ERROR_T hl_asynctask_wait(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id, uint64_t u_timeout)
{
    hl_asynctoken_xt* p_token;
    uint64_t u_end;
    if (!p_self || !HL_ASYNCTOKEN_ID_IS_VALID(token_id)) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (!p_self->b_started) {
        HL_DEBUG_ERROR("Not started yet");
        return HL_ERROR_INVALID_STATE;
    }
    p_token = &p_self->tokens[token_id];
    if (p_token->b_executing || p_token->b_execute) { // "b_execute" means not started yet
        u_end = (hl_time_now() + u_timeout);
        while ((p_token->b_executing || p_token->b_execute) && u_end > hl_time_now()) {
            // hl_thread_sleep(100000);
            // __asm PAUSE; // FIXME
            hl_semaphore_decrement(p_self->ph_sem_exec);
        }
        if ((p_token->b_executing || p_token->b_execute)) {
            HL_DEBUG_WARN("Async token with id = %d timedout", token_id);
            return HL_ERROR_TIMEDOUT;
        }
    }
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_asynctask_wait_2(struct hl_asynctask_s* p_self, hl_asynctoken_id_t token_id)
{
    return hl_asynctask_wait(p_self, token_id, 86400000/* 1 day */);
}

HL_ERROR_T hl_asynctask_stop(struct hl_asynctask_s* p_self)
{
    if (!p_self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if (p_self->ph_thread) {
        p_self->b_started = HL_FALSE;
        if (p_self->ph_sem_run) {
            hl_semaphore_increment(p_self->ph_sem_run);
        }
        hl_thread_join(&p_self->ph_thread);
    }
    p_self->b_started = HL_FALSE;

    return HL_ERROR_SUCCESS;
}

static void* HL_STDCALL _hl_asynctask_run(void *p_arg)
{
    struct hl_asynctask_s* p_self = (struct hl_asynctask_s*)p_arg;
    hl_asynctoken_xt* p_token;
    int ret;
    hl_size_t s;

    HL_DEBUG_INFO("_hl_asynctask_run - ENTER");

    while (p_self->b_started) {
        ret = hl_semaphore_decrement(p_self->ph_sem_run);
        if (ret || !p_self->b_started) {
            break;
        }
        for (s = 0; s < HL_ASYNCTASK_MAX_TOKEN_COUNT; ++s) {
            p_token = &p_self->tokens[s];
            if (p_token->b_execute) {
                p_token->b_executing = HL_TRUE; // must be set first because "wait()" uses both "b_execute" and "b_executing"
                p_token->f_func(p_token->params);
                p_token->b_execute = HL_FALSE;
                p_token->b_executing = HL_FALSE;
                ret = hl_semaphore_increment(p_self->ph_sem_exec);
            }
        }
    }

    HL_DEBUG_INFO("_hl_asynctask_run - EXIT");

    return HL_NULL;
}


/*** OBJECT DEFINITION FOR "hl_asynctask_t" ***/
static hl_object_t* hl_asynctask_ctor(hl_object_t * self, va_list * app)
{
    hl_asynctask_t *p_task = (hl_asynctask_t*)self;
    if (p_task) {

    }
    return self;
}
static hl_object_t* hl_asynctask_dtor(hl_object_t * self)
{
    hl_asynctask_t *p_task = (hl_asynctask_t*)self;
    if (p_task) {
        hl_asynctask_stop(p_task); // stop(), join(), free() "thread"
        if (p_task->ph_sem_run) {
            hl_semaphore_destroy(&p_task->ph_sem_run);
        }
        if (p_task->ph_sem_exec) {
            hl_semaphore_destroy(&p_task->ph_sem_exec);
        }
    }
    return self;
}
static int hl_asynctask_cmp(const hl_object_t *_a1, const hl_object_t *_a2)
{
    return (int)((int*)_a1 - (int*)_a2);
}
static const hl_object_def_t hl_asynctask_def_s = {
    sizeof(hl_asynctask_t),
    hl_asynctask_ctor,
    hl_asynctask_dtor,
    hl_asynctask_cmp,
};
const hl_object_def_t *hl_asynctask_def_t = &hl_asynctask_def_s;
