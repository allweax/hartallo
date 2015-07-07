#ifndef _HARTALLO_API_H_
#define _HARTALLO_API_H_

#include <hl_config.h>
#include <hartallo/hl_types.h>

/***** Engine *****/

extern HARTALLO_API HL_ERROR_T hl_engine_set_cpu_flags(int64_t i64_flags);
extern HARTALLO_API HL_ERROR_T hl_engine_init();
extern HARTALLO_API HL_ERROR_T hl_engine_deinit();

/***** Parser *****/

extern HARTALLO_API HL_ERROR_T hl_parser_create(const struct hl_parser_plugin_def_s* plugin, struct hl_parser_s** pp_parser);
extern HARTALLO_API HL_ERROR_T hl_parser_set_option_int64(struct hl_parser_s* parser, int64_t val);
extern HARTALLO_API HL_ERROR_T hl_parser_set_option_pchar(struct hl_parser_s* parser, const char* val);
extern HARTALLO_API HL_ERROR_T hl_parser_set_option_pobj(struct hl_parser_s* parser, void* val);
extern HARTALLO_API HL_ERROR_T hl_parser_find_bounds(struct hl_parser_s* self , const void* pc_indata, hl_size_t size_indata, hl_size_t *p_start, hl_size_t *p_end);

extern HARTALLO_API HL_ERROR_T hl_parser_plugin_register(const struct hl_parser_plugin_def_s* plugin);
extern HARTALLO_API HL_ERROR_T hl_parser_plugin_unregister(const struct hl_parser_plugin_def_s* plugin);
extern HARTALLO_API HL_ERROR_T hl_parser_plugin_find(HL_CODEC_TYPE_T type, const struct hl_parser_plugin_def_s** ppc_plugin);

/***** Codec *****/

extern HARTALLO_API HL_ERROR_T hl_codec_result_create(struct hl_codec_result_s** pp_result);

extern HARTALLO_API HL_ERROR_T hl_codec_create(const struct hl_codec_plugin_def_s* plugin, struct hl_codec_s** pp_codec);
extern HARTALLO_API HL_ERROR_T hl_codec_set_option_int64(struct hl_codec_s* codec, int64_t val);
extern HARTALLO_API HL_ERROR_T hl_codec_set_option_pchar(struct hl_codec_s* codec, const char* val);
extern HARTALLO_API HL_ERROR_T hl_codec_set_option_pobj(struct hl_codec_s* codec, void* val);
extern HARTALLO_API HL_ERROR_T hl_codec_decode(struct hl_codec_s* self, const void* pc_data, hl_size_t data_size, struct hl_codec_result_s* p_result);
extern HARTALLO_API HL_ERROR_T hl_codec_encode(struct hl_codec_s* self, const struct hl_frame_s* pc_frame, struct hl_codec_result_s* p_result);

extern HARTALLO_API HL_ERROR_T hl_codec_plugin_register(const struct hl_codec_plugin_def_s* plugin);
extern HARTALLO_API HL_ERROR_T hl_codec_plugin_unregister(const struct hl_codec_plugin_def_s* plugin);
extern HARTALLO_API HL_ERROR_T hl_codec_plugin_find(HL_CODEC_TYPE_T type, const struct hl_codec_plugin_def_s** ppc_plugin);

/***** Debug *****/
extern HARTALLO_API HL_ERROR_T hl_debug_set_arg_data(const void* arg);
extern HARTALLO_API HL_ERROR_T hl_debug_set_cb(int (*hl_debug_f)(int level, const void* arg, const char* fmt, ...));
extern HARTALLO_API HL_ERROR_T hl_debug_set_level(int level);


#endif /* _HARTALLO_API_H_ */
