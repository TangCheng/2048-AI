#ifndef __HASH_H__
#define __HASH_H__

#include "constants.h"

typedef struct _hash hash;

#define hash_create(hash, ...) _hash_create(hash, __VA_ARGS__)

bool _hash_create(hash **self, ...);
void hash_destory(hash **self);

#endif /* __HASH_H__ */
