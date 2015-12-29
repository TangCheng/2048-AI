#ifndef __BOARD_POOL_H__
#define __BOARD_POOL_H__

#include "constants.h"
#include "../models/board.h"

typedef struct _board_pool board_pool;

typedef struct _board_data
{
  enum direction  dir;
  enum round      r;
  board           *b;
  float           value;
  int32           alpha;
  int32           beta;
} board_data;

bool board_pool_create(board_pool **self);
void board_pool_destory(board_pool **self);
board_data *board_pool_get(board_pool *self);
void board_pool_put(board_pool *self, board_data *bd);

#endif /* __BOARD_POOL_H__ */
