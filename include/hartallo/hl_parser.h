#ifndef _HARTALLO_PARSER_H_
#define _HARTALLO_PARSER_H_

#include "hl_config.h"
#include "hartallo/hl_object.h"

HL_BEGIN_DECLS

#if !defined(HL_PARSER_MAX_PLUGINS)
#	define HL_PARSER_MAX_PLUGINS			0xFF
#endif

struct hl_option_s;
struct hl_parser_s;

typedef struct hl_parser_s {
    HL_DECLARE_OBJECT;

    const struct hl_parser_plugin_def_s* plugin;
}
hl_parser_t;

#define HL_DECLARE_PARSER hl_parser_t __base_parser__

typedef struct hl_parser_plugin_def_s {
    const hl_object_def_t* objdef;

    HL_PARSER_TYPE_T type;
    HL_MEDIA_TYPE_T media;
    const char* description;

    HL_ERROR_T (*set_option) (struct hl_parser_s* , const struct hl_option_s*);
    HL_ERROR_T (*find_bounds) (struct hl_parser_s* , const void* pc_indata, hl_size_t size_indata, hl_size_t *p_start, hl_size_t *p_end);
}
hl_parser_plugin_def_t;

HARTALLO_API HL_ERROR_T hl_parser_create(const struct hl_parser_plugin_def_s* plugin, hl_parser_t** pp_parser);
HARTALLO_API HL_ERROR_T hl_parser_set_option_int64(hl_parser_t* self, int64_t val);
HARTALLO_API HL_ERROR_T hl_parser_set_option_pchar(hl_parser_t* self, const char* val);
HARTALLO_API HL_ERROR_T hl_parser_set_option_pobj(hl_parser_t* self, hl_object_t* val);
HARTALLO_API HL_ERROR_T hl_parser_find_bounds(hl_parser_t* self , const void* pc_indata, hl_size_t size_indata, hl_size_t *p_start, hl_size_t *p_end);

HARTALLO_API HL_ERROR_T hl_parser_plugin_register(const hl_parser_plugin_def_t* plugin);
HARTALLO_API HL_ERROR_T hl_parser_plugin_unregister(const hl_parser_plugin_def_t* plugin);
HARTALLO_API HL_ERROR_T hl_parser_plugin_find(HL_PARSER_TYPE_T type, const hl_parser_plugin_def_t** ppc_plugin);

HL_END_DECLS

#endif /* _HARTALLO_PARSER_H_ */
