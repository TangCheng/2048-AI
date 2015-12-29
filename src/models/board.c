#include <stdlib.h>
#include <string.h>

#include "board.h"

typedef struct _board
{
  uint32 rows;
  uint32 cols;
  uint32 **contents;
} board;

bool board_create(board **self, uint32 rows, uint32 cols)
{
  bool ret = false;
  uint32 *buffer = NULL;

  *self = (board *)malloc(sizeof(board));
  if (*self == NULL)
  {
    return ret;
  }

  (*self)->rows = rows;
  (*self)->cols = cols;

  (*self)->contents = (uint32 **)malloc(sizeof(uint32 *) * rows);
  if ((*self)->contents == NULL)
  {
    board_destory(self);
    return ret;
  }

  buffer = (uint32 *)malloc(sizeof(uint32) * rows * cols);
  if (buffer == NULL)
  {
    board_destory(self);
    return ret;
  }

  memset((char *)buffer, 0x00, sizeof(uint32) * rows * cols);
  int i = 0;
  for (i = 0; i < rows; i++)
  {
    (*self)->contents[i] = buffer + i * cols;
  }
  ret = true;

  return ret;
}

void board_destory(board **self)
{
  if ((*self)->contents[0] != NULL)
  {
    free((*self)->contents[0]);
    (*self)->contents[0] = NULL;
  }

  if ((*self)->contents != NULL)
  {
    free((*self)->contents);
    (*self)->contents = NULL;
  }

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
  if (self != NULL)
  {
    if (x < self->cols && y < self->rows)
    {
      self->contents[y][x] = val;
    }
  }
}

void board_set_value_by_pos(board *self, uint64 pos, uint32 val)
{
  if (self != NULL)
  {
    uint32 x = pos >> 32;
    uint32 y = pos & 0x00000000FFFFFFFF;
    if (x < self->cols && y < self->rows)
    {
      self->contents[y][x] = val;
    }
  }
}

uint32 board_get_value(board *self, uint32 x, uint32 y)
{
  if (self != NULL)
  {
    if (x < self->cols && y < self->rows)
    {
      return self->contents[y][x];
    }
  }

  return 0;
}

void board_get_empty(board *self, uint64 **array, uint32 *len)
{
  uint32 x = 0, y = 0;
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
          if (self->contents[y][x] == 0)
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

bool board_clone_data(board *self, board *dest)
{
  bool ret = false;

  if (self != NULL && dest != NULL)
  {
    uint32 rows = board_get_rows(dest);
    uint32 cols = board_get_cols(dest);
    if (rows >= self->rows && cols >= self->cols)
    {
      uint32 x = 0, y = 0;
      uint32 val = 0;
      for (x = 0; x < self->cols; x++)
      {
        for (y = 0; y < self->rows; y++)
        {
          val = board_get_value(self, x, y);
          board_set_value(dest, x, y, val);
        }
      }
      ret = true;
    }
  }

  return ret;
}

bool board_is_equal(board *self, board *other)
{
  bool ret = false;

  if (self != NULL && other != NULL)
  {
    uint32 rows = board_get_rows(other);
    uint32 cols = board_get_cols(other);
    if (rows == self->rows && cols == self->cols)
    {
      uint32 x = 0, y = 0;
      uint32 val1 = 0, val2 = 0;
      ret = true;
      for (x = 0; x < self->cols; x++)
      {
        for (y = 0; y < self->rows; y++)
        {
          val1 = board_get_value(self, x, y);
          val2 = board_get_value(other, x, y);
          ret &= (val1 == val2);
        }
      }
    }
  }

  return ret;
}
