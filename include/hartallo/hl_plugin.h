#ifndef _HARTALLO_PLUGIN_H_
#define _HARTALLO_PLUGIN_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef enum hl_plugin_def_type_e {
    hl_plugin_def_type_none = 0,
    hl_plugin_def_type_consumer = (1 << 0),
    hl_plugin_def_type_producer = (1 << 1),
    hl_plugin_def_type_session = (1 << 2),
    hl_plugin_def_type_codec = (1 << 3),
    hl_plugin_def_type_resampler = (1 << 4),
    hl_plugin_def_type_jb = (1 << 5),
    hl_plugin_def_type_denoiser = (1 << 6),
    hl_plugin_def_type_converter = (1 << 7),
    hl_plugin_def_type_all = (~0)
}
hl_plugin_def_type_t;

typedef enum hl_plugin_def_media_type_e {
    hl_plugin_def_media_type_none = 0,
    hl_plugin_def_media_type_audio = (1 << 0),
    hl_plugin_def_media_type_video = (1 << 1),
    hl_plugin_def_media_type_all = (~0)
}
hl_plugin_def_media_type_t;

typedef void hl_plugin_handle_t;
typedef void hl_plugin_symbol_t;
typedef const void* hl_plugin_def_ptr_const_t;

struct hl_plugin_s* hl_plugin_create(const char* path);
hl_plugin_def_ptr_const_t hl_plugin_get_def(struct hl_plugin_s* self, hl_plugin_def_type_t type, hl_plugin_def_media_type_t media_type);
hl_plugin_def_ptr_const_t hl_plugin_get_def_2(struct hl_plugin_s* self, hl_plugin_def_type_t type, hl_plugin_def_media_type_t media_type, hl_size_t index);
hl_plugin_symbol_t* hl_plugin_get_symbol(struct hl_plugin_s* self, const char* symbol_name);
hl_bool_t hl_plugin_file_exist(const char* path);

HL_END_DECLS

#endif /* _HARTALLO_PLUGIN_H_ */
