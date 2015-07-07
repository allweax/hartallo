#ifndef _HARTALLO_UUID_H_
#define _HARTALLO_UUID_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_UUID_DIGEST_SIZE			16
#define HL_UUID_STRING_SIZE			((HL_UUID_DIGEST_SIZE*2)+4/*-*/)

typedef char hl_uuidstring_t[HL_UUID_STRING_SIZE+1]; /**< Hexadecimal UUID digest string. */
typedef char hl_uuiddigest_t[HL_UUID_DIGEST_SIZE]; /**< UUID digest bytes. */

int hl_uuidgenerate(hl_uuidstring_t *result);

HL_END_DECLS

#endif /* _HARTALLO_UUID_H_ */