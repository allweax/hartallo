#include "hartallo/hl_parser.h"
#include "hartallo/hl_option.h"
#include "hartallo/hl_debug.h"

static const hl_parser_plugin_def_t* __hl_parser_plugins[HL_PARSER_MAX_PLUGINS] = {0};

HL_ERROR_T hl_parser_create(const struct hl_parser_plugin_def_s* plugin, hl_parser_t** pp_parser)
{
    if (!plugin || !pp_parser) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    *pp_parser = hl_object_create(plugin->objdef);
    if (!*pp_parser) {
        return HL_ERROR_OUTOFMEMMORY;
    }
    (*pp_parser)->plugin = plugin;

    return HL_ERROR_SUCCESS;
}

#define hl_parser_set_option(self, val, type) \
{ \
	if (!self || !self->plugin || !self->plugin->set_option) { \
		HL_DEBUG_ERROR("Invalid parameter"); \
        return HL_ERROR_INVALID_PARAMETER; \
	} \
	else { \
		hl_option_t* p_option = HL_NULL; \
		HL_ERROR_T err = hl_option_create_##type(val, &p_option); \
		if (err == HL_ERROR_SUCCESS) { \
			err = self->plugin->set_option(self, p_option); \
		} \
		HL_OBJECT_SAFE_FREE(p_option); \
		return err; \
	} \
}

HL_ERROR_T hl_parser_set_option_int64(hl_parser_t* self, int64_t val)
{
    hl_parser_set_option(self, val, i64);
}

HL_ERROR_T hl_parser_set_option_pchar(hl_parser_t* self, const char* val)
{
    hl_parser_set_option(self, val, pchar);
}

HL_ERROR_T hl_parser_set_option_pobj(hl_parser_t* self, hl_object_t* val)
{
    hl_parser_set_option(self, val, pobj);
}

HL_ERROR_T hl_parser_find_bounds(hl_parser_t* self , const void* pc_indata, hl_size_t size_indata, hl_size_t *p_start, hl_size_t *p_end)
{
    if (!self || !self->plugin || !pc_indata || !size_indata || !p_start || !p_end) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    return self->plugin->find_bounds(self, pc_indata, size_indata, p_start, p_end);
}

HL_ERROR_T hl_parser_plugin_register(const hl_parser_plugin_def_t* plugin)
{
    hl_size_t i;
    if (!plugin) {
        HL_DEBUG_ERROR("Invalid parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    /* add or replace the plugin */
    for (i = 0; i<HL_PARSER_MAX_PLUGINS; i++) {
        if(!__hl_parser_plugins[i] || (__hl_parser_plugins[i] == plugin)) {
            __hl_parser_plugins[i] = plugin;
            HL_DEBUG_INFO("Registered parser: %s", plugin->description);
            return HL_ERROR_SUCCESS;
        }
    }

    HL_DEBUG_ERROR("There are already %d plugins.", HL_PARSER_MAX_PLUGINS);
    return HL_ERROR_OUTOFBOUND;
}

HL_ERROR_T hl_parser_plugin_unregister(const hl_parser_plugin_def_t* plugin)
{
    hl_size_t i;
    hl_bool_t found = hl_false;
    if (!plugin) {
        HL_DEBUG_ERROR("Invalid Parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }

    /* find the plugin to unregister */
    for (i = 0; i<HL_PARSER_MAX_PLUGINS && __hl_parser_plugins[i]; i++) {
        if(__hl_parser_plugins[i] == plugin) {
            __hl_parser_plugins[i] = hl_null;
            found = hl_true;
            break;
        }
    }

    /* compact */
    if (found) {
        for(; i<(HL_PARSER_MAX_PLUGINS - 1); i++) {
            if(__hl_parser_plugins[i+1]) {
                __hl_parser_plugins[i] = __hl_parser_plugins[i+1];
            }
            else {
                break;
            }
        }
        __hl_parser_plugins[i] = hl_null;
    }
    return (found ? HL_ERROR_SUCCESS : HL_ERROR_NOT_FOUND);
}

HL_ERROR_T hl_parser_plugin_find(HL_PARSER_TYPE_T type, const hl_parser_plugin_def_t** ppc_plugin)
{
    hl_size_t i;
    if (!ppc_plugin) {
        HL_DEBUG_ERROR("Invalid Parameter");
        return HL_ERROR_INVALID_PARAMETER;
    }
    for (i = 0; i<HL_PARSER_MAX_PLUGINS && __hl_parser_plugins[i]; i++) {
        if	(__hl_parser_plugins[i]->type == type) {
            *ppc_plugin = __hl_parser_plugins[i];
            return HL_ERROR_SUCCESS;
        }
    }
    return HL_ERROR_NOT_FOUND;
}
