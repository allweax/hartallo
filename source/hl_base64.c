#include "hartallo/hl_base64.h"
#include "hartallo/hl_memory.h"

/** Pad char.*/
#define HL_BASE64_PAD '='

/** Encoding block size. */
#define HL_BASE64_ENCODE_BLOCK_SIZE	3 /* 24-bit input group */
/** Decoding block size. */
#define HL_BASE64_DECODE_BLOCK_SIZE	4

/*==================================================================
     Value Encoding  Value Encoding  Value Encoding  Value Encoding
         0 A            17 R            34 i            51 z
         1 B            18 S            35 j            52 0
         2 C            19 T            36 k            53 1
         3 D            20 U            37 l            54 2
         4 E            21 V            38 m            55 3
         5 F            22 W            39 n            56 4
         6 G            23 X            40 o            57 5
         7 H            24 Y            41 p            58 6
         8 I            25 Z            42 q            59 7
         9 J            26 a            43 r            60 8
        10 K            27 b            44 s            61 9
        11 L            28 c            45 t            62 +
        12 M            29 d            46 u            63 /
        13 N            30 e            47 v
        14 O            31 f            48 w         (pad) =
        15 P            32 g            49 x
        16 Q            33 h            50 y

		RFC 4548 - Table 1: The Base 64 Alphabet
*/

hl_size_t hl_base64_encode(const uint8_t* input, hl_size_t input_size, char **output)
{
    static const char* HL_BASE64_ENCODE_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /*=================================================================================
    	content					T					S					K
    	ASCII					0x54				0x53				0x4B
    	Binary					0101 0100			0101 0011			0100 1011
    	------------------------------------------------------------------------------
    	------------------------------------------------------------------------------
    	Packs of 6bits			010101			000101			001101			001011
    	Indexes					21				5				13				11
    	Base64 encoded			V				F				N				L			<=== HERE IS THE RESULT OF hl_base64_encode("HL")
    */

    hl_size_t i = 0;
    hl_size_t output_size = 0;

    /* Caller provided his own buffer? */
    if (!*output) {
        *output = (char*)hl_memory_calloc(1, (HL_BASE64_ENCODE_LEN(input_size)+1));
    }

    /* Too short? */
    if (input_size < HL_BASE64_ENCODE_BLOCK_SIZE) {
        goto quantum;
    }

    do {
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ input[i]>> 2 ];
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ (input[i]<<4 | input[i+1]>>4) & 0x3F ];
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ (input[i+1]<<2 | input[i+2]>>6) & 0x3F ];
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ input[i+2] & 0x3F ];

        i += HL_BASE64_ENCODE_BLOCK_SIZE;
    }
    while(( i+ HL_BASE64_ENCODE_BLOCK_SIZE) <= input_size);

quantum:

    if ((input_size - i) == 1) {
        /* The final quantum of encoding input is exactly 8 bits; here, the
           final unit of encoded output will be two characters followed by
           two "=" padding characters.
        */
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ input[i]>> 2 ];
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ input[i]<<4 & 0x3F ];
        *(*output + output_size++) = HL_BASE64_PAD, *(*output + output_size++) = HL_BASE64_PAD;
    }
    else if((input_size-i) == 2) {
        /*	The final quantum of encoding input is exactly 16 bits; here, the
        	final unit of encoded output will be three characters followed by
        	one "=" padding character.
        */
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ input[i]>> 2 ];
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ (input[i]<<4 | input[i+1]>>4) & 0x3F ];
        *(*output + output_size++) = HL_BASE64_ENCODE_ALPHABET [ (input[i+1]<<2 | input[i+2]>>6) & 0x3F ];
        *(*output + output_size++) = HL_BASE64_PAD;
    }

    return output_size;
}

hl_size_t hl_base64_decode(const uint8_t* input, hl_size_t input_size, char **output)
{
    static const uint8_t HL_BASE64_DECODE_ALPHABET[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1,
        62,
        -1, -1, -1,
        63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
        -1, -1, -1, -1, -1, -1, -1,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        -1, -1, -1, -1, -1, -1,
        26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1,
    };

    hl_size_t i, pay_size;
    hl_size_t output_size = 0;

    /* Caller provided his own buffer? */
    if (!*output) {
        *output = (char*)hl_memory_calloc(1, (HL_BASE64_DECODE_LEN(input_size)+1));
    }

    /* Count pads and remove them from the base64 string */
    for (i = input_size, pay_size = input_size; i > 0; i--) {
        if (input[i-1] == HL_BASE64_PAD) {
            pay_size--;
        }
        else {
            break;
        }
    }

    /* Reset i */
    i = 0;

    if(pay_size < HL_BASE64_DECODE_BLOCK_SIZE) {
        goto quantum;
    }

    do {
        *(*output + output_size++) = (HL_BASE64_DECODE_ALPHABET [ input[i] ]<< 2
                                      | HL_BASE64_DECODE_ALPHABET [ input[i+1] ]>>4);
        *(*output + output_size++) = (HL_BASE64_DECODE_ALPHABET [ input[i+1] ]<< 4
                                      | HL_BASE64_DECODE_ALPHABET [ input[i+2] ]>>2);
        *(*output + output_size++) = (HL_BASE64_DECODE_ALPHABET [ input[i+2] ]<<6
                                      | HL_BASE64_DECODE_ALPHABET [ input[i+3] ]);

        i += HL_BASE64_DECODE_BLOCK_SIZE;
    }
    while (( i+ HL_BASE64_DECODE_BLOCK_SIZE) <= pay_size);

quantum:

    if ((input_size - pay_size) == 1) {
        *(*output + output_size++) = (HL_BASE64_DECODE_ALPHABET [ input[i] ]<< 2
                                      | HL_BASE64_DECODE_ALPHABET [ input[i+1] ]>>4);
        *(*output + output_size++) = (HL_BASE64_DECODE_ALPHABET [ input[i+1] ]<< 4
                                      | HL_BASE64_DECODE_ALPHABET [ input[i+2] ]>>2);
    }
    else if ((input_size-pay_size) == 2) {
        *(*output + output_size++) = (HL_BASE64_DECODE_ALPHABET [ input[i] ]<< 2
                                      | HL_BASE64_DECODE_ALPHABET [ input[i+1] ]>>4);
    }

    return output_size;
}
