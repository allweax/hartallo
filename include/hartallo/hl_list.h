#ifndef _HARTALLO_LIST_H_
#define _HARTALLO_LIST_H_

#include "hl_config.h"
#include "hartallo/hl_types.h"
#include "hartallo/hl_object.h"
#include "hartallo/hl_mutex.h"

HL_BEGIN_DECLS

#define HL_LIST_STATIC_CLEAR_OBJECTS(__list__, __count__) \
	if(__list__ && __count__){ \
		hl_size_t __i__; \
		for(__i__ = 0; __i__ < __count__; ++__i__){ \
			HL_OBJECT_SAFE_FREE(__list__[__i__]); \
		} \
		__count__ = 0; \
	}

#define HL_LIST_STATIC_ADD_OBJECT(__list__, __count__, __max__, __obj__, __err__) \
	if (__count__ >= __max__) { __err__= HL_ERROR_OUTOFBOUND; HL_DEBUG_ERROR("OutOfBoundException (%d>=%d)", __count__, __max__); } \
	else { \
		__list__[__count__] = __obj__; \
		__obj__ = HL_NULL; \
		++__count__; \
		__err__= HL_ERROR_SUCCESS; \
	}
#define HL_LIST_STATIC_ADD_OBJECT_AT_IDX(__list__, __count__, __max__, __obj__, __idx__, __err__) \
	if ((__idx__) >= __max__) { __err__= HL_ERROR_OUTOFBOUND; HL_DEBUG_ERROR("OutOfBoundException (%d>=%d)", (__idx__), (__max__)); } \
	else { \
		HL_OBJECT_SAFE_FREE((__list__)[(__idx__)]); \
		(__list__)[(__idx__)] = (__obj__); \
		(__count__) = (__count__) > (__idx__) ? (__count__) : (__idx__) + 1; \
		(__obj__) = HL_NULL; \
		(__err__)= HL_ERROR_SUCCESS; \
	}

#define HL_LIST_STATIC_DELETE_OBJECT(__list__, __count__, __idx__)\
	if(__list__ && __count__ > __idx__){\
		hl_size_t __i__; \
		HL_OBJECT_SAFE_FREE(__list__[__idx__]);\
		for(__i__=__idx__; __i__<__count__;++__i__){\
			__list__[__i__]=__list__[__i__+1];\
		}\
		--__count__;\
	}

// only for "Ptype**"
#define HL_LIST_STATIC_SAFE_FREE_OBJECTS(__list__, __count__) \
	HL_LIST_STATIC_CLEAR_OBJECTS((__list__), (__count__)) \
	HL_SAFE_FREE((__list__))



#define HL_LIST_IS_EMPTY(list)				((list) ? (!(list)->head) : hl_true)

#define HL_LIST_IS_FIRST(list, item)		((list) ? ((list)->head == item) : hl_false)
#define HL_LIST_IS_LAST(list, item)		((list) ? ((list)->tail == item) : hl_false)

#define HL_LIST_FIRST_DATA(list)			(((list) && (list)->head) ? (list)->head->data : hl_null)
#define HL_LIST_LAST_DATA(list)			(((list) && (list)->tail) ? (list)->tail->data : hl_null)

typedef struct hl_list_item_s {
    HL_DECLARE_OBJECT;
    void* data; /**< Opaque data. <b>Must</b> be a "well-defined" object. */
    struct hl_list_item_s* next; /**< Next item. */
}
hl_list_item_t;

typedef struct hl_list_s {
    HL_DECLARE_OBJECT;

    hl_list_item_t* head; /**< The head of the linked list. */
    hl_list_item_t* tail; /**< The tail of the linked list. */
    hl_mutex_handle_t* mutex; /**< on-demand mutex. */
}
hl_list_t;

typedef int (*hl_list_func_predicate)(const hl_list_item_t* item, const void* data);

#define hl_list_foreach(item, list) for(item = list ? list->head : hl_null; item; item = item->next)

hl_list_t* hl_list_create();
hl_list_item_t* hl_list_item_create();

int hl_list_lock(hl_list_t* list);
int hl_list_unlock(hl_list_t* list);

hl_bool_t hl_list_remove_item(hl_list_t* list, hl_list_item_t* item);
#define hl_list_remove_first_item(list) hl_list_remove_item((list), (list) ? (list)->head : hl_null)
#define hl_list_remove_last_item(list) hl_list_remove_item((list), (list) ? (list)->tail : hl_null)
hl_list_item_t* hl_list_pop_item_by_data(hl_list_t* list, const hl_object_t * hlobj);
hl_bool_t hl_list_remove_item_by_data(hl_list_t* list, const hl_object_t * hlobj);
hl_bool_t hl_list_remove_item_by_pred(hl_list_t* list, hl_list_func_predicate predicate, const void * data);
hl_list_item_t* hl_list_pop_item_by_pred(hl_list_t* list, hl_list_func_predicate predicate, const void * data);
void hl_list_clear_items(hl_list_t* list);

hl_list_item_t* hl_list_pop_first_item(hl_list_t* list);
void hl_list_push_item(hl_list_t* list, hl_list_item_t** item, hl_bool_t back);
#define hl_list_push_back_item(list, item) hl_list_push_item(list, item, hl_true)
#define hl_list_push_front_item(list, item) hl_list_push_item(list, item, hl_false)
void hl_list_push_filtered_item(hl_list_t* list, hl_list_item_t** item, hl_bool_t ascending);
#define hl_list_push_ascending_item(list, item) hl_list_pushfiltered_item(list, item, hl_true)
#define hl_list_push_descending_item(list, item) hl_list_pushfiltered_item(list, item, hl_false)

int hl_list_push_list(hl_list_t* destination, const hl_list_t* source, hl_bool_t back);
#define hl_list_pushback_list(destination, source) hl_list_push_list(destination, source, hl_true)
#define hl_list_pushfront_list(destination, source) hl_list_push_list(destination, source, hl_false)

int hl_list_push_data(hl_list_t* list, hl_object_t** hlobj, hl_bool_t back);
#define hl_list_push_back_data(list, data) hl_list_push_data(list, data, hl_true)
#define hl_list_push_front_data(list, data) hl_list_push_data(list, data, hl_false)
int hl_list_push_filtered_data(hl_list_t* list, hl_object_t** hlobj, hl_bool_t ascending);
#define hl_list_push_ascending_data(list, hlobj) hl_list_push_filtered_data(list, hlobj, hl_true)
#define hl_list_push_descending_data(list, hlobj) hl_list_push_filtered_data(list, hlobj, hl_false)

const hl_list_item_t* hl_list_find_item_by_data(const hl_list_t* list, const hl_object_t * hlobj);
const hl_list_item_t* hl_list_find_item_by_pred(const hl_list_t* list, hl_list_func_predicate predicate, const void* data);
const hl_object_t* hl_list_find_object_by_data(const hl_list_t* list, const hl_object_t* hlobj);
const hl_object_t* hl_list_find_object_by_pred(const hl_list_t* list, hl_list_func_predicate predicate, const void* data);
const hl_object_t* hl_list_find_object_by_pred_at_index(const hl_list_t* list, hl_list_func_predicate predicate, const void* data, hl_size_t index);
int hl_list_find_index_by_pred(const hl_list_t* list, hl_list_func_predicate predicate, const void* data);

hl_size_t hl_list_count(const hl_list_t* list, hl_list_func_predicate predicate, const void* data);

HL_END_DECLS

#endif /* _HARTALLO_LIST_H_ */
