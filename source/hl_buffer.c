#include "hartallo/hl_buffer.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"
#include "hartallo/hl_string.h"

#if defined(_MSC_VER) || HL_UNDER_WINDOWS
#	define vsnprintf	_vsnprintf
#endif

extern const hl_object_def_t *hl_buffer_def_t;

hl_buffer_t* hl_buffer_create(const void* data, hl_size_t size)
{
    return (hl_buffer_t*)hl_object_create(hl_buffer_def_t, data, size);
}

hl_buffer_t* hl_buffer_create_null()
{
    return hl_buffer_create(hl_null, 0);
}

int hl_buffer_append_2(hl_buffer_t* self, const char* format, ...)
{
    /*
     * I suppose that sizeof(char) = 1-byte
     */
    int len = 0;
    va_list ap;
    char *buffer;
    hl_size_t oldsize;
    va_list ap2;

    if(!self) {
        return -1;
    }

    oldsize = self->size;
    buffer = (char*)HL_BUFFER_DATA(self);

    /* initialize variable arguments (needed for 64bit platforms where vsnprintf will change the va_list) */
    va_start(ap, format);
    va_start(ap2, format);

    /* compute destination len for windows mobile
    */
#if defined(_WIN32_WCE)
    {
        int n;
        len = (hl_strlen(format)*2);
        buffer = hl_memory_realloc(buffer, (oldsize+len));
        for(;;) {
            if( (n = vsnprintf((char*)(buffer + oldsize), len, format, ap)) >= 0 && (n<=len) ) {
                len = n;
                break;
            }
            else {
                len += 10;
                buffer = hl_memory_realloc(buffer, (oldsize+len));
            }
        }
    }
#else
    len = vsnprintf(hl_null, 0, format, ap);
    buffer = (char*)hl_memory_realloc(buffer, oldsize+len+1);
    vsnprintf((buffer + oldsize), len
#if !defined(_MSC_VER) || defined(__GNUC__)
              +1
#endif
              , format, ap2);
#endif

    /* reset variable arguments */
    va_end(ap);
    va_end(ap2);

    self->data = buffer;
    self->size = (oldsize+len);

    return 0;
}

int hl_buffer_append(hl_buffer_t* self, const void* data, hl_size_t size)
{
    if(self && size) {
        hl_size_t oldsize = self->size;
        hl_size_t newsize = oldsize + size;

        if(oldsize) {
            self->data = hl_memory_realloc(self->data, newsize);
        }
        else {
            self->data = hl_memory_calloc(size, sizeof(uint8_t));
        }

        if(self->data) {
            if(data) {
                memcpy((void*)(HL_BUFFER_TO_U8(self) + oldsize), data, size);
            }
            self->size = newsize;
            return 0;
        }
    }
    else {
        HL_DEBUG_ERROR("Invalid parameter");
    }
    return -1;
}

int hl_buffer_realloc(hl_buffer_t* self, hl_size_t size)
{
    if(self) {
        if(size == 0) {
            return hl_buffer_cleanup(self);
        }

        if(self->size == 0) { // first time?
            self->data = hl_memory_calloc(size, sizeof(uint8_t));
        }
        else if(self->size != size) {   // only realloc if different sizes
            self->data = hl_memory_realloc(self->data, size);
        }

        self->size = size;
        return 0;
    }
    return -1;
}

int hl_buffer_remove(hl_buffer_t* self, hl_size_t position, hl_size_t size)
{
    if(self && self->data && size) {
        if((position == 0) && ((position + size) >= self->size)) { /* Very common case. */
            return hl_buffer_cleanup(self);
        }
        else if((position + size) < self->size) {
            memcpy(((uint8_t*)self->data) + position, ((uint8_t*)self->data) + position + size,
                   self->size-(position+size));
            return hl_buffer_realloc(self, (self->size-size));
        }
    }
    return -1;
}

int hl_buffer_insert(hl_buffer_t* self, hl_size_t position, const void* data, hl_size_t size)
{
    if(self && size) {
        int ret;
        hl_size_t tomove;

        if(position > self->size) {
            HL_DEBUG_ERROR("Invalid parameter");
            return -2;
        }

        tomove = (self->size - position);

        if((ret = hl_buffer_realloc(self, (self->size + size)))) {
            return ret;
        }
        memmove(((uint8_t*)self->data) + position + size, ((uint8_t*)self->data) + position,
                tomove/*self->size - (position + size)*/);


        if(data) {
            memcpy(((uint8_t*)self->data) + position, data, size);
        }
        else {
            memset(((uint8_t*)self->data) + position, 0, size);
        }

        return 0;
    }
    return -1;
}

int hl_buffer_copy(hl_buffer_t* self, hl_size_t start, const void* data, hl_size_t size)
{
    int ret = 0;
    if(!self || !data || !size) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    // realloc the buffer to match the overral size
    if(self->size != (start + size) && (ret = hl_buffer_realloc(self, (start + size)))) {
        HL_DEBUG_ERROR("failed to realloc the buffer");
        return ret;
    }

    memcpy(((uint8_t*)self->data) + start, data, size);
    return ret;
}

int hl_buffer_cleanup(hl_buffer_t* self)
{
    if(self && self->data) {
        hl_memory_free(&(self->data));
        self->size = 0;
    }
    return 0;
}

int hl_buffer_takeownership(hl_buffer_t* self, void** data, hl_size_t size)
{
    if(!self || !data || !*data || !size) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    if(self->data) {
        hl_memory_free(&(self->data));
    }
    self->data = *data;
    self->size = size;
    *data = hl_null;

    return 0;
}












//=================================================================================================
//	Buffer object definition
//
static hl_object_t* hl_buffer_ctor(hl_object_t * self, va_list * app)
{
    hl_buffer_t *buffer = (hl_buffer_t *)self;
    const void *data = va_arg(*app, const void *);
    hl_size_t size = va_arg(*app, hl_size_t);

    if(size) {
        buffer->data = hl_memory_calloc((size+1), sizeof(uint8_t));
        if(data) {
            memcpy(buffer->data, data, size);
        }
        buffer->size = size;
    }
    return self;
}

static hl_object_t* hl_buffer_dtor(hl_object_t * self)
{
    hl_buffer_t *buffer = (hl_buffer_t *)self;
    if(buffer) {
        HL_MEMORY_FREE(buffer->data);
        buffer->size = 0;
    }

    return self;
}

static const hl_object_def_t hl_buffer_def_s = {
    sizeof(hl_buffer_t),
    hl_buffer_ctor,
    hl_buffer_dtor,
    hl_null,
};
const hl_object_def_t *hl_buffer_def_t = &hl_buffer_def_s;

