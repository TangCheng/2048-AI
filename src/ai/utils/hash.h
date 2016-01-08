#ifndef __HASH_H__
#define __HASH_H__

#include "constants.h"

typedef struct _hash hash;

#define hash_create(h, size, ...)              \
       _hash_create(h, size, #__VA_ARGS__)

#define hash_add(h, key, value)                \
       _hash_add((h), (key), (value))

#define hash_find(h, key, value)               \
       _hash_find((h), (key), (value))

#define hash_del(h, key)                       \
       _hash_del((h), (key))

#define hash_exists(h, key)                    \
       _hash_exists((h), (key))

bool _hash_create(hash **self, uint32 size, const char *type_name);
void hash_destory(hash **self);
bool _hash_add(hash *self, ...);
bool _hash_find(hash *self, ...);
bool _hash_del(hash *self, ...);
bool _hash_exists(hash *self, ...);
uint32  hash_num_elements(hash *self);

#endif /* __HASH_H__ */
