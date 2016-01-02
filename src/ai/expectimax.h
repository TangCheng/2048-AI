#ifndef __EXPECTIMAX_H__
#define __EXPECTIMAX_H__

#include "constants.h"
#include "../models/board.h"

typedef struct _expectimax expectimax;

bool expectimax_create(expectimax **self);
void expectimax_destory(expectimax **self);
enum direction expectimax_search(expectimax *self, board *b,
  enum direction last_dir, uint32 depth);

#endif /* __EXPECTIMAX_H__ */
