#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "ai.h"
#include "minmax.h"

typedef struct _ai
{
  uint32            count;
  minmax            *engine;
  uint32            thinking_duration;
  enum direction    last_dir;
} ai;

#define THINKING_DURATION     0     /* in million seconds */

static ai *a = NULL;

bool ai_create(ai **self)
{
  bool ret = false;

  if (a != NULL)
  {
    a->count++;
    *self = a;
    ret = true;
  }
  else
  {
    a = (ai *)malloc(sizeof(ai));
    if (a != NULL)
    {
      minmax_create(&a->engine);
      a->count = 1;
      a->thinking_duration = THINKING_DURATION;
      a->last_dir = BOTTOM_OF_DIRECTION;
      *self = a;
      ret = true;
    }
  }

  return ret;
}

void ai_destory(ai **self)
{
  if (*self == a)
  {
    *self = NULL;
    a->count--;
    if (a->count == 0)
    {
      minmax_destory(&a->engine);
      free(a);
      a = NULL;
    }
  }
}

void ai_set_thinking_duration(ai *self, uint32 duration)
{
  if (self != NULL)
  {
    self->thinking_duration = duration;
  }
}

enum direction ai_get(ai *self, board *b)
{
  enum direction best = BOTTOM_OF_DIRECTION;
  uint32 depth = 3;
  struct timeval now;
  uint32 start = 0, end = 0;

  if (self != NULL)
  {
    if (self->thinking_duration > 0)
    {
      gettimeofday(&now, NULL);
      start = now.tv_sec * 1000 + now.tv_usec / 1000;
      do {
        best = minmax_search(self->engine, b, self->last_dir, depth);
        if (best == BOTTOM_OF_DIRECTION)
        {
          break;
        }
        depth++;
        gettimeofday(&now, NULL);
        end = now.tv_sec * 1000 + now.tv_usec / 1000;
      } while (end - start < self->thinking_duration);
    }
    else
    {
      best = minmax_search(self->engine, b, self->last_dir, 15);
    }
    self->last_dir = best;
  }

  return best;
}
