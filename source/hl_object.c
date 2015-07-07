#include "hartallo/hl_object.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

#if HL_UNDER_WINDOWS
#	include <windows.h>
#endif /* HL_UNDER_WINDOWS */

#if defined (_DEBUG) || defined (DEBUG)
#	define HL_DEBUG_OBJECTS	0
static int hl_objects_count = 0;
#else
#	define HL_DEBUG_OBJECTS	0
#endif

#ifdef __GNUC__
#	define hl_atomic_inc(_ptr_) __sync_fetch_and_add((_ptr_), 1)
#	define hl_atomic_dec(_ptr_) __sync_fetch_and_sub((_ptr_), 1)
#elif defined(_MSC_VER)
#	define hl_atomic_inc(_ptr_) InterlockedIncrement((_ptr_))
#	define hl_atomic_dec(_ptr_) InterlockedDecrement((_ptr_))
#else
#	define hl_atomic_inc(_ptr_) ++(*(_ptr_))
#	define hl_atomic_dec(_ptr_) --(*(_ptr_))
#endif

hl_object_t* hl_object_create(const hl_object_def_t *objdef, ...)
{
    // Do not check "objdef", let the application die if it's null
    hl_object_t *newobj = objdef->aligned ? hl_memory_calloc(1, objdef->size) : hl_memory_calloc_unaligned(1, objdef->size);
    if(newobj) {
        (*(const hl_object_def_t **) newobj) = objdef;
        HL_OBJECT_HEADER(newobj)->refCount = 1;
        if(objdef->constructor) {
            va_list ap;
            hl_object_t * newobj_ = newobj;// save
            va_start(ap, objdef);
            newobj = objdef->constructor(newobj, &ap); // must return new
            va_end(ap);

            if(!newobj) { // null if constructor failed to initialized the object
                if(objdef->destructor) {
                    objdef->destructor(newobj_);
                }
                objdef->aligned ? hl_memory_free(&newobj_) : hl_memory_free_unaligned(&newobj_);
            }

#if HL_DEBUG_OBJECTS
            HL_DEBUG_INFO("Num objects:%d", ++hl_objects_count);
#endif
        }
        else {
            HL_DEBUG_WARN("No constructor found.");
        }
    }
    else {
        HL_DEBUG_ERROR("Failed to create new hl_object.");
    }

    return newobj;
}

hl_object_t* hl_object_create_2(const hl_object_def_t *objdef, va_list* ap)
{
    hl_object_t *newobj = hl_memory_calloc(1, objdef->size);
    if(newobj) {
        (*(const hl_object_def_t **) newobj) = objdef;
        HL_OBJECT_HEADER(newobj)->refCount = 1;
        if(objdef->constructor) {
            newobj = objdef->constructor(newobj, ap);

#if HL_DEBUG_OBJECTS
            HL_DEBUG_INFO("Num objects:%d", ++hl_objects_count);
#endif
        }
        else {
            HL_DEBUG_WARN("No constructor found.");
        }
    }
    else {
        HL_DEBUG_ERROR("Failed to create new hl_object.");
    }

    return newobj;
}

hl_size_t hl_object_sizeof(const hl_object_t *self)
{
    const hl_object_def_t **objdef = (const hl_object_def_t **)self;
    if(objdef && *objdef) {
        return (*objdef)->size;
    }
    else {
        HL_DEBUG_ERROR("NULL object definition.");
        return 0;
    }
}

int hl_object_cmp(const hl_object_t *object1, const hl_object_t *object2)
{
    const hl_object_def_t **objdef = (const hl_object_def_t **)object1;

    if(objdef && *objdef && (*objdef)->comparator) {
        return (*objdef)->comparator(object1, object2);
    }
    return (int)((int*)object1 - (int*)object2);
}

hl_object_t* hl_object_ref(hl_object_t *self)
{
    hl_object_header_t* objhdr = HL_OBJECT_HEADER(self);
    if(objhdr && objhdr->refCount) {
        hl_atomic_inc(&objhdr->refCount);
        return self;
    }
    return hl_null;
}

hl_object_t* hl_object_unref(hl_object_t *self)
{
    if(self) {
        hl_object_header_t* objhdr = HL_OBJECT_HEADER(self);
        if(objhdr->refCount) { // If refCount is == 0 then, nothing should happen.
            hl_atomic_dec(&objhdr->refCount);
            if(!objhdr->refCount) {
                hl_object_delete(self);
                return hl_null;
            }
        }
        else {
            return hl_null;
        }
    }
    return self;
}

hl_size_t hl_object_get_refcount(hl_object_t *self)
{
    return self ? HL_OBJECT_HEADER(self)->refCount : 0;
}

void hl_object_update(hl_object_t **self, hl_object_t* newobj)
{
    if (self) {
        HL_OBJECT_SAFE_FREE((*self));
        (*self) = hl_object_ref(newobj);
    }
}

void hl_object_delete(hl_object_t *self)
{
    const hl_object_def_t ** objdef = (const hl_object_def_t **)self;
    if(self && *objdef) {
        if((*objdef)->destructor) {
            self = (*objdef)->destructor(self);
#if HL_DEBUG_OBJECTS
            HL_DEBUG_INFO("Num objects:%d", --hl_objects_count);
#endif
        }
        else {
            HL_DEBUG_WARN("No destructor found.");
        }
        (*objdef)->aligned ? hl_memory_free(&self) : hl_memory_free_unaligned(&self);;
    }
}