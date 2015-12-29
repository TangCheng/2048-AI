#ifndef __MINMAX_H__
#define __MINMAX_H__

#include "constants.h"
#include "../models/board.h"

typedef struct _minmax minmax;

bool minmax_create(minmax **self);
void minmax_destory(minmax **self);
enum direction minmax_search(minmax *self, board *b, enum direction last_dir, 
  uint32 depth);

#endif /* __MINMAX_H__ */
