#include <stdlib.h>
#include "board_pool.h"
#include "utils/list.h"

typedef struct _board_pool{
  list *board_list;
  uint32 count;
} board_pool;

static board_pool *bp = NULL;

static void board_pool_release_all(board_pool *self);

bool board_pool_create(board_pool **self)
{
  bool ret = false;

  if (bp != NULL)
  {
    *self = bp;
    bp->count++;
    ret = true;
  }
  else
  {
    bp = (board_pool *)malloc(sizeof(board_pool));
    if (bp != NULL)
    {
      bp->count = 1;
      ret = list_create(&bp->board_list);
      *self = bp;
    }
  }

  return ret;
}

void board_pool_destory(board_pool **self)
{
  if (*self == bp)
  {
    bp->count--;
    *self = NULL;
    if (bp->count == 0)
    {
      board_pool_release_all(bp);
      list_destory(&bp->board_list);
      free(bp);
      bp = NULL;
    }
  }
}

board_data *board_pool_get(board_pool *self)
{
  board_data *bd = NULL;

  if (self != NULL)
  {
    if (list_is_empty(self->board_list) == true)
    {
      bd = (board_data*)malloc(sizeof(board_data));
      board_create(&bd->b, ROWS_OF_BOARD, COLS_OF_BOARD);
    }
    else
    {
      bd = (board_data *)list_get_from_first(self->board_list);
    }
  }

  if (bd != NULL)
  {
    bd->dir = BOTTOM_OF_DIRECTION;
    bd->r = BOTTOM_OF_ROUND;
    bd->value = 0.0;
    bd->alpha = INT32_MAX;
    bd->beta = INT32_MIN;
  }

  return bd;
}

void board_pool_put(board_pool *self, board_data *bd)
{
  if (self != NULL)
  {
    list_add_to_last(self->board_list, (void *)bd);
  }
}

static void board_pool_release_all(board_pool *self)
{
  board_data *bd = NULL;
  bd = (board_data *)list_get_from_first(self->board_list);
  while (bd != NULL)
  {
    board_destory(&bd->b);
    free(bd);
    bd = (board_data *)list_get_from_first(self->board_list);
  }
}
