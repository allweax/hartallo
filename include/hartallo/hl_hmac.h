#ifndef _HARTALLO_HMAC_H_
#define _HARTALLO_HMAC_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_sha1.h"
#include "hartallo/hl_md5.h"

HL_BEGIN_DECLS

int hl_hmac_md5_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_md5string_t *result);
int hl_hmac_md5digest_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_md5digest_t result);

int hl_hmac_sha1_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_sha1string_t *result);
int hl_hmac_sha1digest_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_sha1digest_t result);

HL_END_DECLS

#endif /* _HARTALLO_HMAC_H_ */
