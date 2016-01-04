#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "ai.h"
#include "minmax.h"
#include "expectimax.h"

typedef struct _ai
{
  uint32            count;
  AI_ENGINE         *engine;
  uint32            thinking_duration;
  enum direction    last_dir;
} ai;

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
      AI_ENGINE_CREATE(&a->engine);
      a->count = 1;
      a->thinking_duration = 0;
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
      AI_ENGINE_DESTORY(&a->engine);
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
  uint32 depth = MIN_SEARCH_DEPTH;
  struct timeval now;
  uint64 start = 0, end = 0;

  if (self != NULL)
  {
    if (self->thinking_duration > 0)
    {
      gettimeofday(&now, NULL);
      start = now.tv_sec * 1000 + now.tv_usec / 1000;
      do {
        best = AI_ENGINE_SEARCH(self->engine, b, self->last_dir, depth);
        if (best == BOTTOM_OF_DIRECTION)
        {
          break;
        }
        depth++;
        if (depth > MAX_SEARCH_DEPTH)
        {
          break;
        }
        gettimeofday(&now, NULL);
        end = now.tv_sec * 1000 + now.tv_usec / 1000;
      } while (end - start < self->thinking_duration);
    }
    else
    {
      depth = MIN_SEARCH_DEPTH;
      best = AI_ENGINE_SEARCH(self->engine, b, self->last_dir, depth);
    }
    self->last_dir = best;
  }

  return best;
}
