#ifndef _HARTALLO_STRING_H_
#define _HARTALLO_STRING_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_STRING_STR(self)				((self) ? ((hl_string_t*)(self))->value : hl_null)

typedef char hl_istr_t[21]; /**< Integer number as string value. */

HARTALLO_API char hl_b10tob16(char c);
HARTALLO_API char hl_b16tob10(char c);

HARTALLO_API int hl_stricmp(const char * str1, const char * str2);
HARTALLO_API int hl_strnicmp(const char * str1, const char * str2, hl_size_t n);
HARTALLO_API int hl_strcmp(const char * str1, const char * str2);
HARTALLO_API int hl_strncmp(const char * str1, const char * str2, hl_size_t n);
HARTALLO_API char* hl_strdup(const char *s1);
HARTALLO_API char* hl_strndup(const char *s1, hl_size_t n);
HARTALLO_API hl_bool_t hl_strcontains(const char * str, hl_size_t size, const char * substring);
HARTALLO_API int hl_strindexOf(const char * str, hl_size_t size, const char * substring);
HARTALLO_API int hl_strLastIndexOf(const char * str, hl_size_t size, const char * substring);
HARTALLO_API void hl_strcat(char** destination, const char* source);
HARTALLO_API void hl_strcat_2(char** destination, const char* format, ...);
HARTALLO_API void hl_strncat(char** destination, const char* source, hl_size_t n);
HARTALLO_API int hl_sprintf(char** str, const char* format, ...);
HARTALLO_API int hl_sprintf_2(char** str, const char* format, va_list* ap);
HARTALLO_API void hl_strupdate(char** str, const char* newval);
HARTALLO_API void hl_strtrim_left(char **str);
HARTALLO_API void hl_strtrim_right(char **str);
HARTALLO_API void hl_strtrim(char **str);
HARTALLO_API void hl_strquote(char **str);
HARTALLO_API void hl_strquote_2(char **str, char lquote, char rquote);
HARTALLO_API void hl_strunquote(char **str);
HARTALLO_API void hl_strunquote_2(char **str, char lquote, char rquote);
HARTALLO_API void hl_itoa(int64_t i, hl_istr_t *result);
HARTALLO_API int64_t hl_atoll(const char*);
HARTALLO_API long hl_atox(const char*);
HARTALLO_API void hl_strrandom(hl_istr_t *result);
HARTALLO_API void hl_str_from_hex(const uint8_t *hex, hl_size_t size, char* str);
HARTALLO_API void hl_str_to_hex(const char *str, hl_size_t size, uint8_t* hex);

#define hl_strtrim_both(str) hl_strtrim_left(str), hl_strtrim_right(str);
#define hl_strempty(s) (*(s) == '\0')
#define hl_strnullORempty(s) (!(s) || hl_strempty((s)))
#define hl_striequals(s1, s2) (hl_stricmp((const char*)(s1), (const char*)(s2)) ? HL_FALSE : HL_TRUE)
#define hl_strniequals(s1, s2, n) (hl_strnicmp((const char*)(s1), (const char*)(s2), n) ? HL_FALSE : HL_TRUE)
#define hl_strequals(s1, s2) (hl_strcmp((const char*)(s1), (const char*)(s2)) ? HL_FALSE : HL_TRUE)
#define hl_strnequals(s1, s2, n) (hl_strncmp((const char*)(s1), (const char*)(s2), n) ? HL_FALSE : HL_TRUE)
#define hl_strlen(s) ((s) ? strlen((s)) : 0)

HL_END_DECLS

#endif /* _HARTALLO_STRING_H_ */
