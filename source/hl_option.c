#include "hartallo/hl_option.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_string.h"
#include "hartallo/hl_debug.h"

static hl_object_t* hl_option_ctor(hl_object_t * self, va_list * app)
{
    hl_option_t *p_option = (hl_option_t*)self;
    if (p_option) {
    }
    return self;
}

static hl_object_t* hl_option_dtor(hl_object_t * self)
{
    hl_option_t *p_option = (hl_option_t*)self;
    if (p_option) {
        switch(p_option->type) {
        case HL_OPTION_TYPE_INT64:
        default:
            break;
        case HL_OPTION_TYPE_PCHAR:
            HL_SAFE_FREE(p_option->val.pchar);
            break;
        case HL_OPTION_TYPE_POBJ:
            HL_OBJECT_SAFE_FREE(p_option->val.pobj);
            break;
        }
    }
    return self;
}

static int hl_option_cmp(const hl_object_t *_c1, const hl_object_t *_c2)
{
    return (int)((int*)_c1 - (int*)_c2);
}

static const hl_object_def_t hl_option_def_s = {
    sizeof(hl_option_t),
    hl_option_ctor,
    hl_option_dtor,
    hl_option_cmp,
};

static HL_ERROR_T _hl_option_create(HL_OPTION_TYPE_T type, hl_option_t** pp_option)
{
    if (!pp_option) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    if(!(*pp_option = hl_object_create(&hl_option_def_s))) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_option)->type = type;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_option_create_i64(int64_t val, hl_option_t** pp_option)
{
    HL_ERROR_T err = _hl_option_create(HL_OPTION_TYPE_INT64, pp_option);
    if (err == HL_ERROR_SUCCESS) {
        (*pp_option)->val.i64 = val;
    }
    return err;
}

HL_ERROR_T hl_option_create_pchar(const char* val, hl_option_t** pp_option)
{
    HL_ERROR_T err = _hl_option_create(HL_OPTION_TYPE_PCHAR, pp_option);
    if (err == HL_ERROR_SUCCESS) {
        (*pp_option)->val.pchar = hl_strdup(val);
    }
    return err;
}

HL_ERROR_T hl_option_create_pobj(hl_object_t* val, hl_option_t** pp_option)
{
    HL_ERROR_T err = _hl_option_create(HL_OPTION_TYPE_POBJ, pp_option);
    if (err == HL_ERROR_SUCCESS) {
        (*pp_option)->val.pobj = hl_object_ref(val);
    }
    return err;
}
