#ifndef __LIST_H__
#define __LIST_H__

#include "constants.h"

typedef struct _list list;

bool list_create(list **self);
void list_destory(list **self);
void list_add_to_last(list *self, void *data);
void *list_get_from_first(list *self);
void list_clear(list *self);
bool list_is_empty(list *self);

#endif /* __LIST_H__ */
