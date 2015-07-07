#ifndef _HARTALLO_ENGINE_H_
#define _HARTALLO_ENGINE_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

HARTALLO_API HL_ERROR_T hl_engine_set_cpu_flags(int64_t i64_flags);
HARTALLO_API HL_ERROR_T hl_engine_init();
HARTALLO_API HL_ERROR_T hl_engine_deinit();

HL_END_DECLS

#endif /* _HARTALLO_ENGINE_H_ */
