#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "board.h"

typedef struct _board
{
  uint32  rows;
  uint32  cols;
  board_t contents;
} board;

#define ELEMENT_BITS        (sizeof(board_t) * 8 / (ROWS_OF_BOARD * COLS_OF_BOARD))
#define OFFSET_BY_X_Y(x, y) ((y) * ELEMENT_BITS * COLS_OF_BOARD + (x) * ELEMENT_BITS)

static inline uint8 board_get_power_val(board *self, uint32 x, uint32 y);
static inline void board_set_power_val(board *self, uint32 x, uint32 y,
  uint8 power_val);

bool board_create(board **self, uint32 rows, uint32 cols)
{
  bool ret = false;

  *self = (board *)malloc(sizeof(board));
  if (*self != NULL)
  {
    (*self)->rows = rows;
    (*self)->cols = cols;
    (*self)->contents = 0LL;
    ret = true;
  }

  return ret;
}

void board_destory(board **self)
{
  if (*self != NULL)
  {
    free(*self);
    *self = NULL;
  }
}

uint32 board_get_rows(board *self)
{
  if (self != NULL)
  {
    return self->rows;
  }
  else
  {
    return 0;
  }
}

uint32 board_get_cols(board *self)
{
  if (self != NULL)
  {
    return self->cols;
  }
  else
  {
    return 0;
  }
}

void board_set_value(board *self, uint32 x, uint32 y, uint32 val)
{
  uint8 power_val = 0;

  if (self != NULL)
  {
    if (x < self->cols && y < self->rows)
    {
      if ((val & (val - 1)) == 0)
      {
        power_val = (uint8)log2(val);
        board_set_power_val(self, x, y, power_val);
      }
    }
  }
}

void board_set_value_by_pos(board *self, uint64 pos, uint32 val)
{
  if (self != NULL)
  {
    uint32 x = pos >> 32;
    uint32 y = pos & 0x00000000FFFFFFFF;
    board_set_value(self, x, y, val);
  }
}

uint32 board_get_value(board *self, uint32 x, uint32 y)
{
  uint32 value = 0;
  uint8 power_val = 0;

  if (self != NULL)
  {
    if (x < self->cols && y < self->rows)
    {
      power_val = board_get_power_val(self, x, y);
      value = (power_val == 0) ? 0 : (1 << power_val);
    }
  }

  return value;
}

void board_get_empty(board *self, uint64 **array, uint32 *len)
{
  uint32 x = 0, y = 0;
  uint8 power_val = 0;

  if (self != NULL)
  {
    *array = (uint64 *)malloc(sizeof(uint64) * self->rows * self->cols);
    if (*array != NULL)
    {
      memset((char *)*array, 0, sizeof(uint64) * self->rows * self->cols);
      *len = 0;
      for (x = 0; x < self->cols; x++)
      {
        for (y = 0; y < self->rows; y++)
        {
          power_val = board_get_power_val(self, x, y);
          if (power_val == 0)
          {
            (*array)[*len] = x;
            (*array)[*len] <<= 32;
            (*array)[*len] |= y;
            (*len)++;
          }
        }
      }
    }
  }
}

bool board_clone_data(board *self, board *mother)
{
  bool ret = false;

  if (self != NULL && mother != NULL)
  {
    self->contents = mother->contents;
    ret = true;
  }

  return ret;
}

bool board_is_equal(board *self, board *other)
{
  bool ret = false;

  if (self != NULL && other != NULL)
  {
      ret = (self->contents == other->contents);
  }

  return ret;
}

static inline uint8 board_get_power_val(board *self, uint32 x, uint32 y)
{
  return (self->contents >> OFFSET_BY_X_Y(x, y)) & 0x0F;
}

static inline void board_set_power_val(board *self, uint32 x, uint32 y,
  uint8 power_val)
{
  uint64 temp = 0LL;

  temp = 0x0FLL << OFFSET_BY_X_Y(x, y);
  temp = ~temp;

  self->contents &= temp;

  temp = power_val;
  temp <<= OFFSET_BY_X_Y(x, y);
  self->contents |= temp;
}
