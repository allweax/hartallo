#include "hartallo/hl_plugin.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_string.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

typedef int (*symbol_get_def_count)(void);
typedef hl_plugin_def_type_t (*symbol_get_def_type_at)(int index);
typedef hl_plugin_def_media_type_t (*symbol_get_def_media_type_at)(int index);
typedef hl_plugin_def_ptr_const_t (*symbol_get_def_at)(int index);

#define HL_PLUGIN_FUNC_NAME_DEF_COUNT				"__plugin_get_def_count"
#define HL_PLUGIN_FUNC_NAME_DEF_TYPE_AT			"__plugin_get_def_type_at"
#define HL_PLUGIN_FUNC_NAME_DEF_MEDIA_TYPE_AT		"__plugin_get_def_media_type_at"
#define HL_PLUGIN_FUNC_NAME_DEF_AT					"__plugin_get_def_at"

#if HL_UNDER_WINDOWS
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif

#include <sys/stat.h> /* stat() */

static int _hl_plugin_handle_destroy(hl_plugin_handle_t** self);
static hl_plugin_symbol_t* _hl_plugin_handle_get_symbol(hl_plugin_handle_t* handle, const char* symbol_name);

typedef struct hl_plugin_s {
    HL_DECLARE_OBJECT;

    hl_plugin_handle_t* handle;
    int def_count;
    char* path;
}
hl_plugin_t;

static hl_object_t* hl_plugin_ctor(hl_object_t * self, va_list * app)
{
    hl_plugin_t *plugin = (hl_plugin_t*)self;
    if(plugin) {

    }
    return self;
}
static hl_object_t* hl_plugin_dtor(hl_object_t * self)
{
    hl_plugin_t *plugin = (hl_plugin_t*)self;
    if(plugin) {
        HL_MEMORY_FREE(plugin->path);
        _hl_plugin_handle_destroy(&plugin->handle);
    }

    return self;
}
static const hl_object_def_t hl_plugin_def_s = {
    sizeof(hl_plugin_t),
    hl_plugin_ctor,
    hl_plugin_dtor,
    hl_null,
};

hl_plugin_t* hl_plugin_create(const char* path)
{
    hl_plugin_t* plugin;
    symbol_get_def_count funcptr_get_def_count;
    hl_plugin_handle_t* handle;

#if HL_UNDER_WINDOWS
#	if HL_UNDER_WINDOWS_RT
    wchar_t* szPath = (wchar_t*)hl_memory_calloc(hl_strlen(path) + 1, sizeof(wchar_t));
    static const wchar_t* szFormat = L"%hs";
    swprintf(szPath, hl_strlen(path) * sizeof(wchar_t), szFormat, path);
    handle = LoadPackagedLibrary(szPath, 0x00000000);
    HL_MEMORY_FREE(szPath);
#	else /* Windows desktop */
    UINT currErrMode = SetErrorMode(SEM_FAILCRITICALERRORS); // save current ErrorMode. GetErrorMode() not supported on XP.
    SetErrorMode(currErrMode | SEM_FAILCRITICALERRORS);
    handle = LoadLibraryA(path);
    SetErrorMode(currErrMode); // restore ErrorMode
#	endif
#else
    handle = dlopen(path, RTLD_NOW);
#endif

    if(!handle) {
        HL_DEBUG_ERROR("Failed to load library with path=%s", path);
        return hl_null;
    }

    if(!(funcptr_get_def_count = (symbol_get_def_count)_hl_plugin_handle_get_symbol(handle, HL_PLUGIN_FUNC_NAME_DEF_COUNT))) {
        HL_DEBUG_ERROR("Cannot find function with name=%s", HL_PLUGIN_FUNC_NAME_DEF_COUNT);
        _hl_plugin_handle_destroy(&handle);
        return hl_null;
    }

    if(!(plugin = (hl_plugin_t*)hl_object_create(&hl_plugin_def_s))) {
        HL_DEBUG_ERROR("Failed to create plugin object");
        _hl_plugin_handle_destroy(&handle);
        return hl_null;
    }

    plugin->handle = handle;
    plugin->def_count = funcptr_get_def_count();
    plugin->path = hl_strdup(path);

    HL_DEBUG_INFO("Plugin with path=[%s] created with [%d] defs", plugin->path, plugin->def_count);

    return plugin;
}

hl_plugin_def_ptr_const_t hl_plugin_get_def_2(struct hl_plugin_s* self, hl_plugin_def_type_t type, hl_plugin_def_media_type_t media_type, hl_size_t index)
{
    hl_plugin_def_ptr_const_t def_ptr_const;
    symbol_get_def_type_at funcptr_get_def_type_at;
    symbol_get_def_media_type_at funcptr_get_def_media_type_at;
    symbol_get_def_at funcptr_get_def_at;
    int i;
    hl_size_t _index = 0;

    if(!self) {
        HL_DEBUG_ERROR("Invalid parameter");
        return hl_null;
    }

    if(!(funcptr_get_def_type_at = (symbol_get_def_type_at)hl_plugin_get_symbol(self, HL_PLUGIN_FUNC_NAME_DEF_TYPE_AT))) {
        HL_DEBUG_ERROR("[%s] function not implemented in plugin with path=[%s]", HL_PLUGIN_FUNC_NAME_DEF_TYPE_AT, self->path);
        return hl_null;
    }
    if(!(funcptr_get_def_media_type_at = (symbol_get_def_media_type_at)hl_plugin_get_symbol(self, HL_PLUGIN_FUNC_NAME_DEF_MEDIA_TYPE_AT))) {
        HL_DEBUG_ERROR("[%s] function not implemented in plugin with path=[%s]", HL_PLUGIN_FUNC_NAME_DEF_MEDIA_TYPE_AT, self->path);
        return hl_null;
    }
    if(!(funcptr_get_def_at = (symbol_get_def_at)hl_plugin_get_symbol(self, HL_PLUGIN_FUNC_NAME_DEF_AT))) {
        HL_DEBUG_ERROR("[%s] function not implemented in plugin with path=[%s]", HL_PLUGIN_FUNC_NAME_DEF_AT, self->path);
        return hl_null;
    }

    for(i = 0; i < self->def_count; ++i) {
        if((funcptr_get_def_type_at(i) & type) && (funcptr_get_def_media_type_at(i) & media_type)) {
            if((def_ptr_const = funcptr_get_def_at(i))) {
                if(_index++ == index) {
                    return def_ptr_const;
                }
            }
        }
    }
    return hl_null;
}

hl_plugin_def_ptr_const_t hl_plugin_get_def(hl_plugin_t* self, hl_plugin_def_type_t type, hl_plugin_def_media_type_t media_type)
{
    return hl_plugin_get_def_2(self, type, media_type, 0);
}

hl_plugin_symbol_t* hl_plugin_get_symbol(hl_plugin_t* self, const char* symbol_name)
{
    if(!self || !self->handle || !symbol_name) {
        HL_DEBUG_ERROR("Invalid parameter");
        return hl_null;
    }
    return _hl_plugin_handle_get_symbol(self->handle, symbol_name);
}

hl_bool_t hl_plugin_file_exist(const char* path)
{
    if(path) {
        struct stat _stat;
        return (stat(path, &_stat) == 0 && _stat.st_size > 0);
    }
    return hl_false;
}

static hl_plugin_symbol_t* _hl_plugin_handle_get_symbol(hl_plugin_handle_t* handle, const char* symbol_name)
{
    if(!handle || !symbol_name) {
        HL_DEBUG_ERROR("Invalid parameter");
        return hl_null;
    }
#if HL_UNDER_WINDOWS
    return (hl_plugin_symbol_t*)GetProcAddress((HMODULE)handle, symbol_name);
#else
    return (hl_plugin_symbol_t*)dlsym(handle, symbol_name);
#endif
}

static int _hl_plugin_handle_destroy(hl_plugin_handle_t** handle)
{
    if(handle && *handle) {
#if HL_UNDER_WINDOWS
        FreeLibrary((HMODULE)*handle);
#else
        dlclose(*handle);
#endif
        *handle = hl_null;
    }
    return 0;
}
