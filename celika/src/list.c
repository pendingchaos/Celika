#include "list.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct list_head_t {
    list_t* list;
    struct list_head_t* prev;
    struct list_head_t* next;
} list_head_t;

struct list_t {
    size_t val_size;
    list_head_t* first;
};

list_t* list_new(size_t val_size) {
   list_t* list = malloc(sizeof(list_t));
   list->val_size = val_size;
   list->first = NULL;
   return list;
}

void list_free(list_t* list) {
    for (list_head_t* cur = list->first; cur;) {
        list_head_t* next = cur->next;
        free(cur);
        cur = next;
    }
    free(list);
}

void* list_append(list_t* list, void* val) {
    list_head_t* new = malloc(sizeof(list_head_t)+list->val_size);
    new->list = list;
    new->next = NULL;
    memcpy(new+1, val, list->val_size);
    
    if (list->first) {
        list_head_t* last;
        for (last = list->first; last->next; last = last->next);
        new->prev = last;
        last->next = new;
    } else {
        new->prev = NULL;
        list->first = new;
    }
    
    return new+1;
}

void* list_insert(list_t* list, size_t before, void* val) {
    list_head_t* new = malloc(sizeof(list_head_t)+list->val_size);
    new->list = list;
    memcpy(new+1, val, list->val_size);
    
    if (!before && list->first) { //insert beginning
        new->prev = NULL;
        new->next = list->first;
        list->first->prev = new;
        list->first = new;
    } else if (list->first) { //normal insert //TODO: test this
        list_head_t* after = list_nth(list, before-1);
        new->prev = after;
        new->next = NULL;
        after->next = new;
    } else { //insert into empty list
        new->prev = new->next = NULL;
        list->first = new;
    }
    
    return new+1;
}

void list_remove(void* item) {
    list_head_t* head = (list_head_t*)item - 1;
    list_t* list = head->list;
    
    if (head->prev) head->prev->next = head->next;
    else list->first = head->next; //It's the first one
    if (head->next) head->next->prev = head->prev;
    
    free(head);
}

void* list_nth(list_t* list, size_t index) {
    list_head_t* cur = list->first;
    for (size_t i = 0; i < index; i++) {
        if (cur) cur = cur->next;
        else return NULL;
    }
    return cur + 1;
}

size_t list_len(list_t* list) {
    size_t len = 0;
    for (list_head_t* cur = list->first; cur; cur = cur->next) len++;
    return len;
}

size_t list_item_getindex(void* item) {
    list_head_t* head = (list_head_t*)item - 1;
    size_t i = 0;
    for (list_head_t* cur = head; cur->prev; cur = cur->prev) i++;
    return i;
}
