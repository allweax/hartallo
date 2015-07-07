#include "hartallo/hl_list.h"
#include "hartallo/hl_memory.h"
#include "hartallo/hl_debug.h"

extern const hl_object_def_t *hl_list_item_def_t;
extern const hl_object_def_t *hl_list_def_t;

static int hl_list_find_by_item(const hl_list_item_t* item, const void* _item)
{
    return (item == (const hl_list_item_t*)_item) ? 0 : -1;
}

hl_list_t* hl_list_create()
{
    return (hl_list_t*)hl_object_create(hl_list_def_t);
}

hl_list_item_t* hl_list_item_create()
{
    return (hl_list_item_t*)hl_object_create(hl_list_item_def_t);
}

int hl_list_lock(hl_list_t* list)
{
    if(list) {
        if(!list->mutex) {
            list->mutex = hl_mutex_create();
        }
        return hl_mutex_lock(list->mutex);
    }
    else {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
}

int hl_list_unlock(hl_list_t* list)
{
    if(list && list->mutex) {
        return hl_mutex_unlock(list->mutex);
    }
    else {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
}

hl_bool_t hl_list_remove_item(hl_list_t* list, hl_list_item_t* item)
{
    if(item) {
        return hl_list_remove_item_by_pred(list, hl_list_find_by_item, (const void*)item);
    }
    return hl_false;
}

hl_list_item_t* hl_list_pop_item_by_data(hl_list_t* list, const hl_object_t *hlobj)
{
    if(list) {
        hl_list_item_t *prev = hl_null;
        hl_list_item_t *curr = prev = list->head;

        while(curr) {
            if(!hl_object_cmp(curr->data, hlobj)) {
                if(prev == curr) {
                    /* Found at first position. */
                    if(list->head == list->tail) {
                        /* There was only one item */
                        list->head = list->tail = hl_null;
                    }
                    else {
                        list->head = curr->next;
                    }
                }
                else {
                    if(curr == list->tail) {
                        /* Found at last position */
                        list->tail = prev;
                        list->tail->next = hl_null;
                    }
                    else {
                        prev->next = curr->next;
                    }
                }

                return curr;
            }

            prev = curr;
            curr = curr->next;
        }
    }

    return hl_null;
}

hl_bool_t hl_list_remove_item_by_data(hl_list_t* list, const hl_object_t * hlobj)
{
    hl_list_item_t* item;
    if((item = hl_list_pop_item_by_data(list, hlobj))) {
        hl_object_unref(item);
        return hl_true;
    }
    return hl_false;
}

hl_list_item_t* hl_list_pop_item_by_pred(hl_list_t* list, hl_list_func_predicate predicate, const void * data)
{
    if(list) {
        hl_list_item_t *prev = hl_null;
        hl_list_item_t *curr = prev = list->head;

        while(curr) {
            if(!predicate(curr, data)) {
                if(prev == curr) {
                    /* Found at first position. */
                    if(list->head == list->tail) {
                        /* There was only one item */
                        list->head = list->tail = hl_null;
                    }
                    else {
                        list->head = curr->next;
                    }
                }
                else {
                    if(curr == list->tail) {
                        /* Found at last position */
                        list->tail = prev;
                        list->tail->next = hl_null;
                    }
                    else {
                        prev->next = curr->next;
                    }
                }

                return curr;
            }

            prev = curr;
            curr = curr->next;
        }
    }

    return 0;
}

hl_bool_t hl_list_remove_item_by_pred(hl_list_t* list, hl_list_func_predicate predicate, const void * data)
{
    hl_list_item_t* item;
    if((item = hl_list_pop_item_by_pred(list, predicate, data))) {
        hl_object_unref(item);
        return hl_true;
    }
    return hl_false;
}

void hl_list_clear_items(hl_list_t* list)
{
    if(list) {
        hl_list_item_t* next = hl_null;
        hl_list_item_t* curr = list->head;

        while(curr) {
            next = curr->next;
            hl_object_unref(curr);
            curr = next;
        }
        list->head = hl_null;
        list->tail = hl_null;
    }
}

hl_list_item_t* hl_list_pop_first_item(hl_list_t* list)
{
    hl_list_item_t* item = hl_null;
    if(list) {
        item = list->head;
        if(list->head) {
            if(list->head->next) {
                list->head = list->head->next;
            }
            else {
                list->head = list->tail = hl_null;
            }
        }
    }

    return item;
}

void hl_list_push_item(hl_list_t* list, hl_list_item_t** item, hl_bool_t back)
{
    // do not test
    hl_bool_t first = !list->head;

    if(back && list->tail) {
        list->tail->next = *item, list->tail = *item, (*item)->next = hl_null;
    }
    else {
        (*item)->next = list->head, list->head = *item;
    }

    if(first) {
        list->tail = list->head = *item, list->tail->next = hl_null;
    }
    (*item) = hl_null;
}

void hl_list_push_filtered_item(hl_list_t* list, hl_list_item_t** item, hl_bool_t ascending)
{
    if(list) {
        hl_list_item_t *prev = hl_null;
        hl_list_item_t *curr = prev = list->head;

        while(curr) {
            int diff = hl_object_cmp((*item), curr);
            if((diff </*=*/ 0 && ascending) || (diff >/*=*/0 && !ascending)) {
                if(curr == list->head) {
                    hl_list_push_front_item(list, item);
                }
                else {
                    (*item)->next = curr;
                    prev->next = (*item);
                }

                return;
            }

            prev = curr;
            curr = curr->next;
        }

        hl_list_push_back_item(list, item);
    }
}

int hl_list_push_list(hl_list_t* dest, const hl_list_t* src, hl_bool_t back)
{
    const hl_list_item_t* curr = (src)->head;
    hl_object_t* copy;

    if(!dest || !src) {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    while(curr) {
        copy = hl_object_ref(curr->data);
        hl_list_push_data(dest, (void**)&copy, back);

        curr = curr->next;
    }
    return 0;
}

int hl_list_push_data(hl_list_t* list, hl_object_t** hlobj, hl_bool_t back)
{
    if(list && hlobj && *hlobj) {
        hl_list_item_t *item = hl_list_item_create();
        item->data = *hlobj; // stolen

        hl_list_push_item(list, &item, back);
        (*hlobj) = hl_null;

        return 0;
    }
    else {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
}

int hl_list_push_filtered_data(hl_list_t* list, hl_object_t** hlobj, hl_bool_t ascending)
{
    if(list && hlobj && *hlobj) {
        hl_list_item_t *item = hl_list_item_create();
        item->data = *hlobj;

        hl_list_push_filtered_item(list, &item, ascending);
        (*hlobj) = hl_null;

        return 0;
    }
    else {
        HL_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
}

const hl_list_item_t* hl_list_find_item_by_data(const hl_list_t* list, const hl_object_t* hlobj)
{
    if(list && hlobj) {
        hl_list_item_t *item;
        hl_list_foreach(item, list) {
            if(!hl_object_cmp(item->data, hlobj)) {
                return item;
            }
        }
    }

    return 0;
}

const hl_list_item_t* hl_list_find_item_by_pred(const hl_list_t* list, hl_list_func_predicate predicate, const void* data)
{
    if(predicate) {
        const hl_list_item_t *item;
        hl_list_foreach(item, list) {
            if(predicate(item, data) == 0) {
                return item;
            }
        }
    }
    else {
        HL_DEBUG_WARN("Cannot use a null predicate function");
    }
    return hl_null;
}

const hl_object_t* hl_list_find_object_by_pred(const hl_list_t* list, hl_list_func_predicate predicate, const void* data)
{
    return hl_list_find_object_by_pred_at_index(list, predicate, data, 0);
}

const hl_object_t* hl_list_find_object_by_data(const hl_list_t* list, const hl_object_t* hlobj)
{
    const hl_list_item_t* item = hl_list_find_item_by_data(list, hlobj);
    if(item) {
        return (const hl_object_t*)item->data;
    }
    return hl_null;
}

const hl_object_t* hl_list_find_object_by_pred_at_index(const hl_list_t* list, hl_list_func_predicate predicate, const void* data, hl_size_t index)
{
    hl_size_t pos = 0;
    const hl_list_item_t *item;

    hl_list_foreach(item, list) {
        if((!predicate || predicate(item, data) == 0) && pos++ >= index) {
            return item->data;
        }
    }

    return hl_null;
}

int hl_list_find_index_by_pred(const hl_list_t* list, hl_list_func_predicate predicate, const void* data)
{
    if(list && predicate) {
        int index = 0;
        const hl_list_item_t *item;
        hl_list_foreach(item, list) {
            if(predicate(item, data) == 0) {
                return index;
            }
            ++index;
        }
    }
    return -1;
}

hl_size_t hl_list_count(const hl_list_t* list, hl_list_func_predicate predicate, const void* data)
{
    hl_size_t count = 0;
    if(list) {
        const hl_list_item_t *item;
        hl_list_foreach(item, list) {
            if(!predicate || (predicate(item, data) == 0)) {
                ++count;
            }
        }
    }
    else {
        HL_DEBUG_ERROR("Invalid parameter");
    }

    return count;
}











//=================================================================================================
//	Item object definition
//
static hl_object_t* hl_list_item_ctor(hl_object_t * self, va_list * app)
{
    hl_list_item_t *item = (hl_list_item_t*)self;
    if(item) {
    }
    return self;
}

static hl_object_t* hl_list_item_dtor(hl_object_t *self)
{
    hl_list_item_t *item = (hl_list_item_t*)self;
    if(item) {
        item->data = hl_object_unref(item->data);
    }
    else {
        HL_DEBUG_WARN("Cannot free an uninitialized item");
    }
    return item;
}

static int hl_list_item_cmp(const hl_object_t *_item1, const hl_object_t *_item2)
{
    const hl_list_item_t* item1 = (const hl_list_item_t*)_item1;
    const hl_list_item_t* item2 = (const hl_list_item_t*)_item2;

    if(item1 && item2) {
        return hl_object_cmp(item1->data, item2->data);
    }
    else {
        return -1;
    }
}

static const hl_object_def_t hl_list_item_def_s = {
    sizeof(hl_list_item_t),
    hl_list_item_ctor,
    hl_list_item_dtor,
    hl_list_item_cmp,
};
const hl_object_def_t *hl_list_item_def_t = &hl_list_item_def_s;

//=================================================================================================
//	List object definition
//
static hl_object_t* hl_list_ctor(hl_object_t *self, va_list *app)
{
    hl_list_t *list = (hl_list_t *)self;
    if(list) {
    }

    return self;
}

static hl_object_t* hl_list_dtor(hl_object_t *self)
{
    hl_list_t *list = (hl_list_t *)self;
    if(list) {
#if 0
        /* Not thread-safe */
        hl_list_item_t* next = hl_null;
        hl_list_item_t* curr = list->head;

        while(curr) {
            next = curr->next;
            /*curr =*/ hl_object_unref(curr);
            curr = next;
        }
#else
        /* Thread-safe method */
        hl_list_item_t* item;
        while((item = hl_list_pop_first_item(list))) {
            hl_object_unref(item);
        }
#endif

        /* destroy the on-demand mutex */
        if(list->mutex) {
            hl_mutex_destroy(&list->mutex);
        }
    }
    else {
        HL_DEBUG_WARN("Cannot free an uninitialized list");
    }
    return list;
}

static const hl_object_def_t hl_list_def_s = {
    sizeof(hl_list_t),
    hl_list_ctor,
    hl_list_dtor,
    hl_null,
};
const hl_object_def_t *hl_list_def_t = &hl_list_def_s;

