#include "hartallo/hl_engine.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_cpu.h"
#include "hartallo/hl_codec.h"
#include "hartallo/hl_parser.h"
#include "hartallo/hl_time.h"
#include "hartallo/hl_debug.h"

static hl_bool_t __engine_initialized = HL_FALSE;
static int64_t __engine_cpu_flags = kCpuFlagAll;

// Built-in initialization functions (always ther)
extern HL_ERROR_T hl_codec_264_cavlc_InitEncodingTable();
// Built-in codecs (always there)
extern const hl_codec_plugin_def_t *hl_codec_264_plugin_def_t; // H.264 SVC, AVC, MVC
// Built-in parsers (always there)
extern const hl_parser_plugin_def_t *hl_parser_264_plugin_def_t;

static HL_ERROR_T _hl_engine_init_cpu_flags(int64_t i64_flags);
static HL_ERROR_T _hl_engine_init_funcs();

HARTALLO_API HL_ERROR_T hl_engine_set_cpu_flags(int64_t i64_flags)
{
    HL_ERROR_T err = _hl_engine_init_cpu_flags(i64_flags);
    if (err) {
        return err;
    }
    // re-init all elements using CPU flags
    if (__engine_initialized) {
        err = _hl_engine_init_funcs();
        if (err) {
            return err;
        }
    }

    return err;
}

HL_ERROR_T hl_engine_init()
{
    HL_ERROR_T err = HL_ERROR_SUCCESS;

    if(!__engine_initialized) {
        srand((int)hl_time_now());
        err = _hl_engine_init_cpu_flags(__engine_cpu_flags);
        if (err) {
            return err;
        }

        if ((err = hl_codec_264_cavlc_InitEncodingTable())) {
            return err;
        }
        if ((err = hl_codec_plugin_register(hl_codec_264_plugin_def_t))) {
            return err;
        }
        if ((err = hl_parser_plugin_register(hl_parser_264_plugin_def_t))) {
            return err;
        }

        if ((err = _hl_engine_init_funcs())) {
            return err;
        }
        __engine_initialized = HL_TRUE;
    }

    return err;
}

HARTALLO_API HL_ERROR_T hl_engine_deinit()
{
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _hl_engine_init_cpu_flags(int64_t i64_flags)
{
    hl_cpu_flags_enable(i64_flags);
    __engine_cpu_flags = i64_flags;
    HL_DEBUG_INFO("CPU flags: %s, Number of cores: #%d, Cache Line Size: #%d", hl_cpu_flags_names(), hl_cpu_get_cores_count(), hl_cpu_get_cache_line_size());
    return HL_ERROR_SUCCESS;
}

static HL_ERROR_T _hl_engine_init_funcs()
{
    extern HL_ERROR_T hl_math_init_funcs();
    extern HL_ERROR_T hl_memory_init_funcs();
    extern HL_ERROR_T hl_bits_init_funcs();
    extern HL_ERROR_T hl_codec_264_init_functions();
    HL_ERROR_T err;

    /* Common */
    err = hl_math_init_funcs();
    if (err) {
        return err;
    }
    err = hl_memory_init_funcs();
    if (err) {
        return err;
    }
    err = hl_bits_init_funcs();
    if (err) {
        return err;
    }
    /* H.264 AVC/SVC codec */
    err = hl_codec_264_init_functions();
    if (err) {
        return err;
    }

    return err;
}
