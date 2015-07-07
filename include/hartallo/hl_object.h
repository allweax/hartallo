#ifndef _HARTALLO_OBJECT_H_
#define _HARTALLO_OBJECT_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"

HL_BEGIN_DECLS

#define HL_OBJECT(self) ((hl_object_t*)(self))

typedef void hl_object_t;

#define HL_OBJECT_SAFE_FREE(self)		if((self)) hl_object_unref((self)), (self) = hl_null

#define HL_OBJECT_SAFE_FREE_ARRAY(self, count) { \
	int __i; \
	for(__i = 0; __i < (count); ++__i) \
		HL_OBJECT_SAFE_FREE((self)[__i]); \
}
#define HL_OBJECT_SAFE_FREE_TABLE(self) HL_OBJECT_SAFE_FREE_ARRAY((self), (sizeof((self))/sizeof((self)[0])))

#define HL_DECLARE_OBJECT \
	const void* __def__;  /**< Opaque data holding a pointer to the actual meta-data(size, constructor, destructor and comparator) */ \
	long	refCount /**< Reference counter. */

#define HL_OBJECT_DEF(self)			((const hl_object_def_t*)self)

typedef struct hl_object_header_s {
    HL_DECLARE_OBJECT;
}
hl_object_header_t;
#define HL_OBJECT_HEADER(object)	((hl_object_header_t*)object)

typedef struct hl_object_def_s {
    //! The size of the object.
    hl_size_t size;
    //! Pointer to the constructor.
    hl_object_t*	(* constructor) (hl_object_t *, va_list *);
    //! Pointer to the destructor.
    hl_object_t*	(* destructor) (hl_object_t *);
    //! Pointer to the comparator.
    int		(* comparator) (const hl_object_t *, const hl_object_t *);
    //! Boolean to know whether the object must be allocated aligned.
    hl_bool_t aligned;
}
hl_object_def_t;

HARTALLO_API hl_object_t* hl_object_create(const hl_object_def_t *objdef, ...);
HARTALLO_API hl_object_t* hl_object_create_2(const hl_object_def_t *objdef, va_list* ap);
HARTALLO_API hl_size_t hl_object_sizeof(const hl_object_t *);
HARTALLO_API int hl_object_cmp(const hl_object_t *object1, const hl_object_t *object2);
HARTALLO_API hl_object_t* hl_object_ref(hl_object_t *self);
HARTALLO_API hl_object_t* hl_object_unref(hl_object_t *self);
HARTALLO_API hl_size_t hl_object_get_refcount(hl_object_t *self);
HARTALLO_API void hl_object_update(hl_object_t **self, hl_object_t* newobj);
HARTALLO_API void hl_object_delete(hl_object_t *self);

HL_END_DECLS

#endif /* _HARTALLO_OBJECT_H_ */
