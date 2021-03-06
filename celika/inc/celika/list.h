#pragma once
#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef struct list_t list_t;

list_t* list_create(size_t val_size);
void list_del(list_t* list);
void* list_append(list_t* list, void* val);
void* list_insert(list_t* list, size_t before, void* val);
void list_remove(void* item);
void* list_nth(list_t* list, size_t index);
size_t list_len(list_t* list);
size_t list_item_getindex(void* item);
#endif
