#ifndef _HARTALLO_BUFFER_H_
#define _HARTALLO_BUFFER_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_list.h"

HL_BEGIN_DECLS

#define HL_BUFFER(self)					((hl_buffer_t*)self)
#define HL_BUFFER_DATA(self)				(self ? HL_BUFFER(self)->data : hl_null)
#define HL_BUFFER_SIZE(self)				(self ? HL_BUFFER(self)->size : 0)

#define HL_BUFFER_TO_STRING(self)			(self ? (const char*)HL_BUFFER_DATA(self) : hl_null)
#define HL_BUFFER_TO_U8(self)				(self ? (uint8_t*)HL_BUFFER_DATA(self) : hl_null)

typedef struct hl_buffer_s {
    HL_DECLARE_OBJECT;

    void *data; /**< Interanl data. */
    hl_size_t size; /**< The size of the internal data. */
}
hl_buffer_t;

typedef hl_list_t hl_buffers_L_t; /**<@ingroup hl_buffer_group List of @ref hl_buffer_t elements. */

hl_buffer_t* hl_buffer_create(const void* data, hl_size_t size);
hl_buffer_t* hl_buffer_create_null();

int hl_buffer_append_2(hl_buffer_t* self, const char* format, ...);
int hl_buffer_append(hl_buffer_t* self, const void* data, hl_size_t size);
int hl_buffer_realloc(hl_buffer_t* self, hl_size_t size);
int hl_buffer_remove(hl_buffer_t* self, hl_size_t position, hl_size_t size);
int hl_buffer_insert(hl_buffer_t* self, hl_size_t position, const void*data, hl_size_t size);
int hl_buffer_copy(hl_buffer_t* self, hl_size_t start, const void* data, hl_size_t size);
int hl_buffer_cleanup(hl_buffer_t* self);
int hl_buffer_takeownership(hl_buffer_t* self, void** data, hl_size_t size);

HL_END_DECLS

#endif /* _HARTALLO_BUFFER_H_ */
