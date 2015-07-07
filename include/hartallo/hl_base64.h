#ifndef _HARTALLO_BASE64_H_
#define _HARTALLO_BASE64_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_BASE64_ENCODE_LEN(IN_LEN)		((2 + (IN_LEN) - (((IN_LEN) + 2) % 3)) * 4 / 3)
#define HL_BASE64_DECODE_LEN(IN_LEN)		(((IN_LEN * 3)/4) + 2)

hl_size_t hl_base64_encode(const uint8_t* input, hl_size_t input_size, char **output);
hl_size_t hl_base64_decode(const uint8_t* input, hl_size_t input_size, char **output);

HL_END_DECLS

#endif /* _HARTALLO_BASE64_H_ */
