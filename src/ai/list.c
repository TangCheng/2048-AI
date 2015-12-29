#include <stdlib.h>
#include "list.h"

typedef struct _list_node list_node;

typedef struct _list
{
  list_node *head;
  list_node *tail;
  list_node *removed;
  uint32    node_count;
} list;

struct _list_node
{
  void      *data;
  list_node *next;
};

static list_node *list_get_node(list *self);

bool list_create(list **self)
{
  bool ret = false;

  *self = (list *)malloc(sizeof(list));
  if (*self != NULL)
  {
    (*self)->head = NULL;
    (*self)->tail = NULL;
    (*self)->removed = NULL;
    (*self)->node_count = 0;
    ret = true;
  }

  return ret;
}

void list_destory(list **self)
{
  if (*self != NULL)
  {
    list_clear(*self);
    free(*self);
    *self = NULL;
  }
}

void list_add_to_last(list *self, void *data)
{
  if (self != NULL)
  {
    list_node *node = list_get_node(self);
    node->data = data;
    if (self->tail == NULL)
    {
      self->head = node;
      self->tail = node;
    }
    else
    {
      self->tail->next = node;
      self->tail = node;
    }
    self->node_count++;
  }
}

void *list_get_from_first(list *self)
{
  void *data = NULL;

  if (self != NULL && self->head != NULL)
  {
    list_node *node = NULL;
    node = self->head;
    self->head = node->next;
    if (self->tail == node)
    {
      self->tail = node->next;
    }
    data = node->data;
    self->node_count--;
    node->next = self->removed;
    self->removed = node;
  }

  return data;
}

void list_clear(list *self)
{
  if (self != NULL)
  {
    list_node *node = NULL;
    for (node = self->head; node != NULL; node = self->head)
    {
      self->head = node->next;
      free(node);
    }
    for (node = self->removed; node != NULL; node = self->removed)
    {
      self->removed = node->next;
      free(node);
    }
  }
}

bool list_is_empty(list *self)
{
  if (self != NULL)
  {
    return self->node_count == 0;
  }
  return true;
}

static list_node *list_get_node(list *self)
{
  list_node *node = NULL;

  if (self->removed != NULL)
  {
    node = self->removed;
    self->removed = node->next;
  }
  else
  {
    node = (list_node *)malloc(sizeof(list_node));
  }

  if (node != NULL)
  {
    node->data = NULL;
    node->next = NULL;
  }

  return node;
}
