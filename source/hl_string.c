#include "hartallo/hl_string.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_time.h"
#include "hartallo/hl_debug.h"

#if HL_UNDER_WINDOWS_RT
#include <Windows.h> /* MultiByteToWideChar */
#include <vector>
#endif


#if defined(_MSC_VER)
#	define snprintf		_snprintf
#	define vsnprintf	_vsnprintf
#	define strdup		_strdup
#	define stricmp		_stricmp
#	define strnicmp		_strnicmp
#else
#	if !HAVE_STRNICMP && !HAVE_STRICMP
#	define stricmp		strcasecmp
#	define strnicmp		strncasecmp
#	endif
#endif

char hl_b10tob16(char c)
{
    static char HEX[] = "0123456789abcdef";
    return HEX[c & 15];
}

char hl_b16tob10(char c)
{
    return isdigit(c) ? c - '0' : tolower(c) - 'a' + 10;
}

int hl_stricmp(const char * str1, const char * str2)
{
    return (str1 && str2) ?
           ( (tolower(*str1) == tolower(*str2)) ? stricmp(str1, str2) : (*str1-*str2) )  /* Compare first charaters before doing complete comparison */
               :
               ( (!str1 && !str2) ? 0 : -1 );
}

int hl_strnicmp(const char * str1, const char * str2, hl_size_t n)
{
    return (str1 && str2 && n) ?
           ( (tolower(*str1) == tolower(*str2)) ? strnicmp(str1, str2, n) : (*str1-*str2) )  /* Compare first charaters before doing complete comparison */
               :
               ( (!str1 && !str2) ? 0 : -1 );
}

int hl_strcmp(const char * str1, const char * str2)
{
    return (str1 && str2) ?
           ( (*str1 == *str2) ? stricmp(str1, str2) : (*str1-*str2) )  /* Compare first charaters before doing complete comparison */
               :
               ( (!str1 && !str2) ? 0 : -1 );
}

int hl_strncmp(const char * str1, const char * str2, hl_size_t n)
{
    return (str1 && str2) ? ((*str1 != *str2) ? -1 : strncmp(str1, str2, n)) : ((!str1 && !str2) ? 0 : -1);
}

char* hl_strdup(const char *s1)
{
    if(s1) {
        return strdup(s1);
    }
    return hl_null;
}

char* hl_strndup(const char *s1, hl_size_t n)
{
    char *ret = hl_null;

    if(s1 && n) {
        hl_size_t len = hl_strlen(s1);
        hl_size_t nret = (n > len) ? (len) : (n);

        if((ret = (char*)hl_memory_calloc((nret+1), sizeof(uint8_t)))) {
            memcpy(ret, s1, nret);
        }
    }

    return ret;
}

hl_bool_t hl_strcontains(const char * str, hl_size_t size, const char * substring)
{
    return (hl_strindexOf(str, size, substring) >= 0);
}

int hl_strindexOf(const char * str, hl_size_t size, const char * substring)
{
    if(str && substring) {
        const char* sub_start = strstr(str, substring);
        if(sub_start && (sub_start < (str + size))) {
            return (int)(sub_start - str);
        }
    }
    return -1;
}

int hl_strLastIndexOf(const char * str, hl_size_t size, const char * substring)
{
    if(str && substring) {
        hl_size_t sub_size = hl_strlen(substring);
        const char* last_sub_start = hl_null;
        const char* sub_start = strstr(str, substring);
        const char* end = (str + size);
        while(sub_start && (sub_start < end)) {
            last_sub_start = sub_start;
            if((sub_start + sub_size)<end) {
                sub_start = strstr((sub_start + sub_size), substring);
            }
            else {
                break;
            }
        }
        if(last_sub_start) {
            return (int)(last_sub_start - str);
        }
    }
    return -1;
}

void hl_strcat(char** destination, const char* source)
{
    hl_strncat(destination, source, hl_strlen(source));
}

void hl_strcat_2(char** destination, const char* format, ...)
{
    char* temp = hl_null;
    int len;
    va_list ap;

    /* initialize variable arguments */
    va_start(ap, format);
    /* compute */
    if((len = hl_sprintf_2(&temp, format, &ap))) {
        hl_strncat(destination, temp, len);
    }
    /* reset variable arguments */
    va_end(ap);
    HL_MEMORY_FREE(temp);
}

void hl_strncat(char** destination, const char* source, hl_size_t n)
{
    hl_size_t index = 0;
    hl_size_t hl_size_to_cat = (n > hl_strlen(source)) ? hl_strlen(source) : n;

    if(!source || !n) {
        return;
    }

    if(!*destination) {
        *destination = (char*)hl_memory_malloc(hl_size_to_cat+1);
        strncpy(*destination, source, hl_size_to_cat+1);
    }
    else {
        index = hl_strlen(*destination);
        *destination = (char*)hl_memory_realloc(*destination, index + hl_size_to_cat+1);
        strncpy(((*destination)+index), source, hl_size_to_cat+1);
    }
    (*destination)[index + hl_size_to_cat] = '\0';
}

int hl_sprintf(char** str, const char* format, ...)
{
    int len;
    va_list ap;

    /* initialize variable arguments */
    va_start(ap, format);
    /* compute */
    len = hl_sprintf_2(str, format, &ap);
    /* reset variable arguments */
    va_end(ap);

    return len;
}

int hl_sprintf_2(char** str, const char* format, va_list* ap)
{
    int len = 0;
    va_list ap2;

    /* free previous value */
    if(*str) {
        HL_MEMORY_FREE(str);
    }

    /* needed for 64bit platforms where vsnprintf will change the va_list */
    hl_va_copy(ap2, *ap);

    /* compute destination len for windows mobile
    */
#if defined(_WIN32_WCE)
    {
        int n;
        len = (hl_strlen(format)*2);
        *str = (char*)hl_memory_calloc(1, len+1);
        for(;;) {
            if( (n = vsnprintf(*str, len, format, *ap)) >= 0 && (n<len) ) {
                len = n;
                goto done;
            }
            else {
                len += 10;
                *str = hl_memory_realloc(*str, len+1);
            }
        }
done:
        (*str)[len] = '\0';
    }
#else
    len = vsnprintf(0, 0, format, *ap);
    *str = (char*)hl_memory_calloc(1, len+1);
    vsnprintf(*str, len
#if !defined(_MSC_VER) || defined(__GNUC__)
              +1
#endif
              , format, ap2);
#endif

    va_end(ap2);

    return len;
}

void hl_strupdate(char** str, const char* newval)
{
    if(str && *str != newval) { // do nothing if same memory address
        // use realloc() to keep same memory address
        hl_size_t length = hl_strlen(newval);
        if(!length) {
            HL_MEMORY_FREE(str);
        }
        else if((*str = (char*)hl_memory_realloc(*str, length + 1))) {
            memcpy(*str, newval, length);
            (*str)[length] = '\0';
        }
    }
}


void hl_strtrim_left(char **str)
{
    if(str && *str) {
        hl_size_t count = 0;
        while(isspace(*((*str)+count))) {
            count++;
        }
        if(count) {
            hl_size_t len = hl_strlen((*str));
            memmove((*str), (*str)+count, (len - count));
            (*str)[len - count] = '\0';
        }
    }
}

void hl_strtrim_right(char **str)
{
    if(str && *str) {
        hl_size_t size;
        if((size = hl_strlen(*str))) {
            while(isspace(*((*str)+size-1))) {
                size--;
            }
            *(*str + size) = '\0';
        }
    }
}

void hl_strtrim(char **str)
{
    // left
    hl_strtrim_left(str);
    // right
    hl_strtrim_right(str);
}

void hl_strquote(char **str)
{
    hl_strquote_2(str, '"', '"');
}

void hl_strquote_2(char **str, char lquote, char rquote)
{
    if(str && *str) {
        char *result = hl_null;
        hl_sprintf(&result, "%c%s%c", lquote, *str, rquote);
        HL_MEMORY_FREE(str);
        *str = result;
    }
}

void hl_strunquote(char **str)
{
    hl_strunquote_2(str, '"', '"');
}

void hl_strunquote_2(char **str, char lquote, char rquote)
{
    if(str && *str) {
        hl_size_t size = hl_strlen(*str);
        if(size>=2 && **str == lquote && *((*str)+size-1) == rquote) {
            memmove((*str), (*str)+1, (size-2));
            *((*str)+size-2) = '\0';
        }
    }
}

void hl_itoa(int64_t i, hl_istr_t *result)
{
    memset(result, 0, sizeof(*result));
    sprintf(*result,"%lld",i);
}

int64_t hl_atoll(const char* str)
{
    // FIXME: use HAVE_ATOLL and use macro instead of function
    if(str) {
#if defined(_MSC_VER)
        return _atoi64(str);
#elif defined(__GNUC__)
        return atoll(str);
#else
        return atol(str);
#endif
    }
    return 0;
}

long hl_atox(const char* str)
{
    long ret = 0;
    if(str) {
        sscanf(str, "%lx", &ret);
    }
    return ret;
}

void hl_strrandom(hl_istr_t *result)
{
    static uint64_t __counter = 1;
    hl_itoa((hl_time_now() ^ (rand())) ^ ++__counter, result);
}

void hl_str_from_hex(const uint8_t *hex, hl_size_t size, char* str)
{
    static const char *HL_HEXA_VALUES = {"0123456789abcdef"};
    hl_size_t i;

    for (i = 0 ; i<size; i++) {
        str[2*i] = HL_HEXA_VALUES [ (*(hex+i) & 0xf0) >> 4 ];
        str[(2*i)+1] = HL_HEXA_VALUES [ (*(hex+i) & 0x0f)		];
    }
}

void hl_str_to_hex(const char *str, hl_size_t size, uint8_t* hex)
{
    // to avoid SIGBUS error when memory is misaligned do not use sscanf("%2x")
    HL_DEBUG_FATAL("Not implemented.");
}



#if HL_UNDER_WINDOWS_RT

HARTALLO_API std::vector<char> rt_hl_str_to_native(Platform::String^ str)
{
    if(str != nullptr && !str->IsEmpty()) {
        int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str->Data(), str->Length(), nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            std::vector<char> vec(len + 1);
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, str->Data(), str->Length(), vec.data(), len, nullptr, nullptr) == len) {
                return std::move(vec);
            }
        }
    }
    return std::move(std::vector<char>(0));
}

HARTALLO_API Platform::String^  rt_hl_str_to_managed(char const* str)
{
    if(str) {
        int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);
        if (len > 0) {
            std::vector<wchar_t> vec(len);
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, vec.data(), len) == len) {
                return ref new Platform::String(vec.data());
            }
        }
    }
    return nullptr;
}

#endif /* HL_UNDER_WINDOWS_RT */
