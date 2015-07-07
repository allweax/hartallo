#include "hartallo/hl_debug.h"

HARTALLO_GEXTERN hl_debug_f __hl_debug_cb = HL_NULL;
HARTALLO_GEXTERN int __hl_debug_level = DEBUG_LEVEL;
HARTALLO_GEXTERN const void* __hl_debug_arg_data = HL_NULL;

HL_ERROR_T hl_debug_set_arg_data(const void* arg)
{
    __hl_debug_arg_data = arg;
    return HL_ERROR_SUCCESS;
}

HL_ERROR_T hl_debug_set_cb(hl_debug_f cb)
{
    __hl_debug_cb = cb;
    return HL_ERROR_SUCCESS;
}

int hl_debug_get_level( )
{
    return __hl_debug_level;
}

HL_ERROR_T hl_debug_set_level(int level)
{
    __hl_debug_level = level;
    return HL_ERROR_SUCCESS;
}
