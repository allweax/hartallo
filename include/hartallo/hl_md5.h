#ifndef _HARTALLO_MD5_H_
#define _HARTALLO_MD5_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_MD5_DIGEST_SIZE		16
#define HL_MD5_BLOCK_SIZE		64

#define HL_MD5_EMPTY			"d41d8cd98f00b204e9800998ecf8427e"

#define HL_MD5_STRING_SIZE		(HL_MD5_DIGEST_SIZE*2)
typedef char hl_md5string_t[HL_MD5_STRING_SIZE+1]; /**< Hexadecimal MD5 string. */
typedef uint8_t hl_md5digest_t[HL_MD5_DIGEST_SIZE]; /**< MD5 digest bytes. */

#define HL_MD5_DIGEST_CALC(input, input_size, digest)		\
	{														\
		hl_md5context_t ctx;								\
		hl_md5init(&ctx);									\
		hl_md5update(&ctx, (const uint8_t*)(input), (input_size));			\
		hl_md5final((digest), &ctx);						\
	}

typedef struct hl_md5context_s {
    uint32_t buf[4];
    uint32_t bytes[2];
    uint32_t in[16];
}
hl_md5context_t;

void hl_md5init(hl_md5context_t *context);
void hl_md5update(hl_md5context_t *context, uint8_t const *buf, hl_size_t len);
void hl_md5final(hl_md5digest_t digest, hl_md5context_t *context);
void hl_md5transform(uint32_t buf[4], uint32_t const in[HL_MD5_DIGEST_SIZE]);
int hl_md5compute(const char* input, hl_size_t size, hl_md5string_t *result);

HL_END_DECLS

#endif /* _HARTALLO_MD5_H_ */
