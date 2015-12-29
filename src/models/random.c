#include <stdlib.h>
#include <time.h>
#include "random.h"

typedef struct _random_generator
{
  uint32 count;
} random_generator;

static random_generator *rg = NULL;

bool random_generator_create(random_generator **self)
{
  bool ret = false;

  if (rg != NULL)
  {
    rg->count++;
    *self = rg;
    ret = true;
  }
  else
  {
    rg = (random_generator *)malloc(sizeof(random_generator));
    if (rg != NULL)
    {
      srand((uint32)time(NULL));
      rg->count = 1;
      *self = rg;
      ret = true;
    }
  }

  return ret;
}

void random_generator_destory(random_generator **self)
{
  if (rg != NULL && *self == rg)
  {
    rg->count--;
    *self = NULL;
    if (rg->count == 0)
    {
      free(rg);
      rg = NULL;
    }
  }
}

uint64 random_generator_select(random_generator *self, uint64 *array, size_t len)
{
  uint32 r = 0;
  uint32 range = (RAND_MAX - (RAND_MAX % len));
  if (self != NULL)
  {
    do
    {
      r = rand();
    } while (r > range);
    r = r % len;
  }

  return array[r];
}
