#ifndef _HARTALLO_SHA1_H_
#define _HARTALLO_SHA1_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

typedef enum hl_sha1_errcode_e {
    shaSuccess = 0,		/**< Success */
    shaNull,            /**< Null pointer parameter */
    shaInputTooLong,    /**< input data too long */
    shaStateError       /**< called Input after Result */
}
hl_sha1_errcode_t;

#define HL_SHA1_DIGEST_SIZE			20
#define HL_SHA1_BLOCK_SIZE				64

#define HL_SHA1_STRING_SIZE		(HL_SHA1_DIGEST_SIZE*2)
typedef uint8_t hl_sha1string_t[HL_SHA1_STRING_SIZE+1];
typedef uint8_t hl_sha1digest_t[HL_SHA1_DIGEST_SIZE]; /**< SHA-1 digest bytes. */

#define HL_SHA1_DIGEST_CALC(input, input_size, digest)			\
			{													\
				hl_sha1context_t ctx;							\
				hl_sha1reset(&ctx);							\
				hl_sha1input(&ctx, (input), (input_size));		\
				hl_sha1result(&ctx, (digest));					\
			}

typedef struct hl_sha1context_s {
    uint32_t Intermediate_Hash[HL_SHA1_DIGEST_SIZE/4]; /* Message Digest  */

    uint32_t Length_Low;            /**< Message length in bits      */
    uint32_t Length_High;           /**< Message length in bits      */


    int_least16_t Message_Block_Index;/**< Index into message block array   */
    uint8_t Message_Block[64];      /**< 512-bit message blocks      */

    int32_t Computed;               /**< Is the digest computed?         */
    int32_t Corrupted;             /**< Is the message digest corrupted? */
}
hl_sha1context_t;

/*
 *  Function Prototypes
 */

hl_sha1_errcode_t hl_sha1reset(hl_sha1context_t *);
hl_sha1_errcode_t hl_sha1input(hl_sha1context_t *, const uint8_t *, hl_size_t length);
hl_sha1_errcode_t hl_sha1result(hl_sha1context_t *, hl_sha1digest_t Message_Digest);
void hl_sha1final(uint8_t *Message_Digest, hl_sha1context_t *context);
hl_sha1_errcode_t hl_sha1compute(const char* input, hl_size_t size, hl_sha1string_t *result);


HL_END_DECLS

#endif /* _HARTALLO_SHA1_H_ */
