#ifndef __RANDOM_H__
#define __RANDOM_H__

#include "constants.h"

typedef struct _random_generator random_generator;

bool random_generator_create(random_generator **self);
void random_generator_destory(random_generator **self);
uint64 random_generator_select(random_generator *self, uint64 *array, size_t len);

#endif /* __RANDOM_H__ */
