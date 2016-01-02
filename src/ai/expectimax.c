#include <stdlib.h>
#include "expectimax.h"

typedef struct _expectimax
{

} expectimax;

bool expectimax_create(expectimax **self)
{
  bool ret = false;

  *self = (expectimax *)malloc(sizeof(expectimax));
  if (*self != NULL)
  {

    ret = true;
  }

  return ret;
}

void expectimax_destory(expectimax **self)
{
  if (*self != NULL)
  {
    free(*self);
    *self = NULL;
  }
}

enum direction expectimax_search(expectimax *self, board *b,
  enum direction last_dir, uint32 depth)
{
  enum direction best = BOTTOM_OF_DIRECTION;

  if (self != NULL && b != NULL && depth != 0)
  {

  }
  
  return best;
}
