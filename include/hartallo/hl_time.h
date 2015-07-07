#ifndef _HARTALLO_TIME_H_
#define _HARTALLO_TIME_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

//#if defined(__SYMBIAN32__) || ANDROID /* Forward declaration */
struct timeval;
struct timezone;
struct timespec;
//#endif

#define HL_TIME_S_2_MS(S) ((S)*1000)
#define HL_TIME_MS_2_S(MS) ((MS)/1000)

HARTALLO_API int hl_gettimeofday(struct timeval *tv, struct timezone *tz);
HARTALLO_API uint64_t hl_gettimeofday_ms();
HARTALLO_API uint64_t hl_time_get_ms(const struct timeval *tv);
HARTALLO_API uint64_t hl_time_epoch();
HARTALLO_API uint64_t hl_time_now();
HARTALLO_API uint64_t hl_time_ntp();
HARTALLO_API uint64_t hl_time_get_ntp_ms(const struct timeval *tv);

HL_END_DECLS

#endif /* _HARTALLO_TIME_H_ */
