#include "hartallo/hl_hmac.h"
#include "hartallo/hl_string.h"
#include "hartallo/hl_buffer.h"
#include "hartallo/hl_debug.h"

typedef enum hl_hash_type_e { md5, sha1 } hl_hash_type_t;

int hl_hmac_xxxcompute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_hash_type_t type, uint8_t* digest)
{
#define HL_MAX_BLOCK_SIZE	HL_SHA1_BLOCK_SIZE

    hl_size_t i, newkey_size;

    hl_size_t block_size = type == md5 ? HL_MD5_BLOCK_SIZE : HL_SHA1_BLOCK_SIZE; // Only SHA-1 and MD5 are supported for now
    hl_size_t digest_size = type == md5 ? HL_MD5_DIGEST_SIZE : HL_SHA1_DIGEST_SIZE;
    char hkey [HL_MAX_BLOCK_SIZE];

    uint8_t ipad [HL_MAX_BLOCK_SIZE];
    uint8_t opad [HL_MAX_BLOCK_SIZE];


    memset(ipad, 0, sizeof(ipad));
    memset(opad, 0, sizeof(ipad));

    /*
    *	H(K XOR opad, H(K XOR ipad, input))
    */

    // Check key len
    if (key_size > block_size) {
        if(type == md5) {
            HL_MD5_DIGEST_CALC(key, key_size, (uint8_t*)hkey);
        }
        else if(type == sha1) {
            HL_SHA1_DIGEST_CALC((uint8_t*)key, key_size, (uint8_t*)hkey);
        }
        else {
            return -3;
        }

        newkey_size = digest_size;
    }
    else {
        memcpy(hkey, key, key_size);
        newkey_size = key_size;
    }

    memcpy(ipad, hkey, newkey_size);
    memcpy(opad, hkey, newkey_size);

    /* [K XOR ipad] and [K XOR opad]*/
    for (i=0; i<block_size; i++) {
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5c;
    }


    {
        hl_buffer_t *passx; // pass1 or pass2
        int pass1_done = 0;

        passx = hl_buffer_create(ipad, block_size); // pass1
        hl_buffer_append(passx, input, input_size);

digest_compute:
        if(type == md5) {
            HL_MD5_DIGEST_CALC(HL_BUFFER_TO_U8(passx), HL_BUFFER_SIZE(passx), digest);
        }
        else {
            HL_SHA1_DIGEST_CALC(HL_BUFFER_TO_U8(passx), HL_BUFFER_SIZE(passx), digest);
        }

        if(pass1_done) {
            HL_OBJECT_SAFE_FREE(passx);
            goto pass1_and_pass2_done;
        }
        else {
            pass1_done = 1;
        }

        hl_buffer_cleanup(passx);
        hl_buffer_append(passx, opad, block_size); // pass2
        hl_buffer_append(passx, digest, digest_size);

        goto digest_compute;
    }

pass1_and_pass2_done:

    return 0;
}

int hl_hmac_md5_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_md5string_t *result)
{
    hl_md5digest_t digest;
    int ret;

    if((ret = hl_hmac_md5digest_compute(input, input_size, key, key_size, digest))) {
        return ret;
    }
    hl_str_from_hex(digest, HL_MD5_DIGEST_SIZE, *result);
    (*result)[HL_MD5_STRING_SIZE] = '\0';

    return 0;
}


int hl_hmac_md5digest_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_md5digest_t result)
{
    return hl_hmac_xxxcompute(input, input_size, key, key_size, md5, result);
}

int hl_hmac_sha1_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_sha1string_t *result)
{
    hl_sha1digest_t digest;
    int ret;

    if((ret = hl_hmac_sha1digest_compute(input, input_size, key, key_size, digest))) {
        return ret;
    }
    hl_str_from_hex((uint8_t*)digest, HL_SHA1_DIGEST_SIZE, (char*)*result);
    (*result)[HL_SHA1_STRING_SIZE] = '\0';

    return 0;
}

int hl_hmac_sha1digest_compute(const uint8_t* input, hl_size_t input_size, const char* key, hl_size_t key_size, hl_sha1digest_t result)
{
    return hl_hmac_xxxcompute(input, input_size, key, key_size, sha1, (uint8_t*)result);
}

