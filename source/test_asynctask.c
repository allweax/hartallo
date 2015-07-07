#include <hartallo/hl_asynctask.h>
#include <hartallo/hl_time.h>
#include <hartallo/hl_debug.h>
#include <assert.h>

static HL_ERROR_T hl_test_asynctask_f(const struct hl_asynctoken_param_xs* pc_params)
{
    uint64_t end = hl_time_now() + 0;
    const char* name;
    int32_t age;
    while (end > hl_time_now()) ;

    age = HL_ASYNCTASK_GET_PARAM(pc_params[0].pc_param_ptr, int32_t);
    name = HL_ASYNCTASK_GET_PARAM(pc_params[1].pc_param_ptr, const char*);

    HL_DEBUG_INFO("name=%s, age=%d", name, age);

    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_test_asynctask()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;
    int32_t age = 25;
    const char* name = "John Doe";
    struct hl_asynctask_s* p_asynctask = HL_NULL;
    hl_asynctoken_id_t token_id = HL_ASYNCTOKEN_ID_INVALID;

    err = hl_asynctask_create(&p_asynctask);
    if (err) {
        goto bail;
    }
    err = hl_asynctask_set_affinity(p_asynctask, 1);
    if (err) {
        goto bail;
    }
    err = hl_asynctask_take_token(p_asynctask, &token_id);
    if (err) {
        goto bail;
    }
    err = hl_asynctask_take_token(p_asynctask, &token_id);
    if (err) {
        goto bail;
    }
    err = hl_asynctask_release_token(p_asynctask, &token_id);
    if (err) {
        goto bail;
    }
    err = hl_asynctask_take_token(p_asynctask, &token_id);
    if (err) {
        goto bail;
    }

    err = hl_asynctask_start(p_asynctask);
    if (err) {
        goto bail;
    }

    err = hl_asynctask_execute(p_asynctask, token_id, hl_test_asynctask_f,
                               HL_ASYNCTASK_SET_PARAM_VAL(age),
                               HL_ASYNCTASK_SET_PARAM_VAL(name),
                               HL_ASYNCTASK_SET_PARAM_NULL());
    if (err) {
        goto bail;
    }

    err = hl_asynctask_wait_2(p_asynctask, token_id);
    if (err) {
        goto bail;
    }

    getchar();

bail:
    HL_OBJECT_SAFE_FREE(p_asynctask);
    return err;
}
