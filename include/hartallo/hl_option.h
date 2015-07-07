#ifndef _HARTALLO_OPTION_H_
#define _HARTALLO_OPTION_H_

#include "hl_config.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef struct hl_option_s {
    HL_DECLARE_OBJECT;

    HL_OPTION_TYPE_T type;

    union {
        int64_t i64;
        char* pchar;
        hl_object_t* pobj;
    } val;
}
hl_option_t;

HL_ERROR_T hl_option_create_i64(int64_t val, hl_option_t** pp_option);
HL_ERROR_T hl_option_create_pchar(const char* val, hl_option_t** pp_option);
HL_ERROR_T hl_option_create_pobj(hl_object_t* val, hl_option_t** pp_option);

HL_END_DECLS

#endif /* _HARTALLO_OPTION_H_ */
