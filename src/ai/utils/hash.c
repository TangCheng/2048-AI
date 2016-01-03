#include <stdlib.h>
#include <string.h>
#include "hash.h"

typedef struct _hash
{

} hash;

bool _hash_create(hash **self, ...)
{
  bool ret = false;

  *self = (hash *)malloc(sizeof(hash));
  if (*self != NULL) {
    ret = true;
  }

  return ret;
}

void hash_destory(hash **self)
{
  if (*self != NULL) {
    free(*self);
    *self = NULL;
  }
}
