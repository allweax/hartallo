#ifndef _HARTALLO_DEBUG_H_
#define _HARTALLO_DEBUG_H_

#include "hl_config.h"
#include "hartallo/hl_types.h" /* HL_NULL */

HL_BEGIN_DECLS

#if !defined(DEBUG_LEVEL)
#	define DEBUG_LEVEL DEBUG_LEVEL_ERROR
#endif

#define DEBUG_LEVEL_TALKATIVE	5
#define DEBUG_LEVEL_INFO		4
#define DEBUG_LEVEL_WARN		3
#define DEBUG_LEVEL_ERROR		2
#define DEBUG_LEVEL_FATAL		1

typedef int (*hl_debug_f)(int level, const void* arg, const char* fmt, ...);

HARTALLO_GEXTERN hl_debug_f __hl_debug_cb;
HARTALLO_GEXTERN int __hl_debug_level;
HARTALLO_GEXTERN const void* __hl_debug_arg_data;

/* TALKATIVE */
#define HL_DEBUG_TALKATIVE(FMT, ...)		\
	if(__hl_debug_level >= DEBUG_LEVEL_TALKATIVE){ \
		if(__hl_debug_cb) \
			__hl_debug_cb(DEBUG_LEVEL_TALKATIVE, __hl_debug_arg_data, "*TALKATIVE: " FMT "\n", ##__VA_ARGS__); \
		else \
			fprintf(stderr, "*TALKATIVE: " FMT "\n", ##__VA_ARGS__); \
	}

/* INFO */
#define HL_DEBUG_INFO(FMT, ...)		\
	if(__hl_debug_level >= DEBUG_LEVEL_INFO){ \
		if(__hl_debug_cb) \
			__hl_debug_cb(DEBUG_LEVEL_INFO, __hl_debug_arg_data, "*INFO: " FMT "\n", ##__VA_ARGS__); \
		else \
			fprintf(stderr, "*INFO: " FMT "\n", ##__VA_ARGS__); \
	}


/* WARN */
#define HL_DEBUG_WARN(FMT, ...)		\
	if(__hl_debug_level >= DEBUG_LEVEL_WARN){ \
		if(__hl_debug_cb) \
			__hl_debug_cb(DEBUG_LEVEL_WARN, __hl_debug_arg_data, "**WARN: function: \"%s()\" \nfile: \"%s\" \nline: \"%u\" \nMSG: " FMT "\n", __FUNCTION__,  __FILE__, __LINE__, ##__VA_ARGS__); \
		else \
			fprintf(stderr, "**WARN: function: \"%s()\" \nfile: \"%s\" \nline: \"%u\" \nMSG: " FMT "\n", __FUNCTION__,  __FILE__, __LINE__, ##__VA_ARGS__); \
	}

/* ERROR */
#define HL_DEBUG_ERROR(FMT, ...) 		\
	if(__hl_debug_level >= DEBUG_LEVEL_ERROR){ \
		if(__hl_debug_cb) \
			__hl_debug_cb(DEBUG_LEVEL_ERROR, __hl_debug_arg_data, "***ERROR: function: \"%s()\" \nfile: \"%s\" \nline: \"%u\" \nMSG: " FMT "\n", __FUNCTION__,  __FILE__, __LINE__, ##__VA_ARGS__); \
		else \
			fprintf(stderr, "***ERROR: function: \"%s()\" \nfile: \"%s\" \nline: \"%u\" \nMSG: " FMT "\n", __FUNCTION__,  __FILE__, __LINE__, ##__VA_ARGS__); \
	}


/* FATAL */
#define HL_DEBUG_FATAL(FMT, ...) 		\
	if(__hl_debug_level >= DEBUG_LEVEL_FATAL){ \
		if(__hl_debug_cb) \
			__hl_debug_cb(DEBUG_LEVEL_FATAL, __hl_debug_arg_data, "****FATAL: function: \"%s()\" \nfile: \"%s\" \nline: \"%u\" \nMSG: " FMT "\n", __FUNCTION__,  __FILE__, __LINE__, ##__VA_ARGS__); \
		else \
			fprintf(stderr, "****FATAL: function: \"%s()\" \nfile: \"%s\" \nline: \"%u\" \nMSG: " FMT "\n", __FUNCTION__,  __FILE__, __LINE__, ##__VA_ARGS__); \
	}


HARTALLO_API HL_ERROR_T hl_debug_set_arg_data(const void* arg);
HARTALLO_API HL_ERROR_T hl_debug_set_cb(hl_debug_f cb);
HARTALLO_API int hl_debug_get_level( );
HARTALLO_API HL_ERROR_T hl_debug_set_level(int level);

HL_END_DECLS

#endif /* _HARTALLO_DEBUG_H_ */
