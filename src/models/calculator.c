#include <stdlib.h>
#include "calculator.h"

typedef struct _calculator
{
  uint64 score;
  board *temp;
} calculator;

static bool calculator_move_up(calculator *self, board *c, board *n, bool only_check);
static bool calculator_move_down(calculator *self, board *c, board *n, bool only_check);
static bool calculator_move_left(calculator *self, board *c, board *n, bool only_check);
static bool calculator_move_right(calculator *self, board *c, board *n, bool only_check);
static bool calculator_proc_line(calculator *self, uint32 *array, size_t len,
                                 bool only_check);
static bool calculator_merge_array(calculator *self, uint32 *array, size_t len,
                                   bool only_check);
static bool calculator_move_array(calculator *self, uint32 *array, size_t len);

bool calculator_create(calculator **self)
{
  bool ret = false;

  *self = (calculator *)malloc(sizeof(calculator));
  if (*self != NULL)
  {
    ret = board_create(&(*self)->temp, ROWS_OF_BOARD, COLS_OF_BOARD);
  }

  return ret;
}

void calculator_destory(calculator **self)
{
  if (*self != NULL)
  {
    board_destory(&(*self)->temp);
    free(*self);
    *self = NULL;
  }
}

bool calculator_check_direction(calculator *self, board *b, enum direction dir)
{
  bool ret = false;

  if (self != NULL)
  {
    switch (dir)
    {
      case UP:
        ret = calculator_move_up(self, b, self->temp, true);
        break;
      case DOWN:
        ret = calculator_move_down(self, b, self->temp, true);
        break;
      case LEFT:
        ret = calculator_move_left(self, b, self->temp, true);
        break;
      case RIGHT:
        ret = calculator_move_right(self, b, self->temp, true);
        break;
      default:
        break;
    }
  }

  return ret;
}

bool calculator_move(calculator *self, board *current, board *next, enum direction dir)
{
  bool ret = false;

  if (self != NULL && current != NULL && next != NULL)
  {
    switch (dir)
    {
      case UP:
        ret = calculator_move_up(self, current, next, false);
        break;
      case DOWN:
        ret = calculator_move_down(self, current, next, false);
        break;
      case LEFT:
        ret = calculator_move_left(self, current, next, false);
        break;
      case RIGHT:
        ret = calculator_move_right(self, current, next, false);
        break;
      default:
        break;
    }
  }

  return ret;
}

uint64 calculator_get_score(calculator *self)
{
  uint64 ret = 0;
  if (self != NULL)
  {
    ret = self->score;
  }
  return ret;
}

static bool calculator_move_up(calculator *self, board *c, board *n, bool only_check)
{
  bool ret = false;
  uint32 cols = board_get_cols(c);
  uint32 rows = board_get_rows(c);
  uint32 x = 0, y = 0;
  uint32 val = 0;
  uint32 *val_array = NULL;

  val_array = (uint32 *)malloc(sizeof(uint32) * rows);
  if (val_array != NULL)
  {
    for (x = 0; x < cols; x++)
    {
      for (y = 0; y < rows; y++)
      {
        val = board_get_value(c, x, y);
        val_array[y] = val;
      }
      ret |= calculator_proc_line(self, val_array, rows, only_check);
      for (y = 0; y < rows; y++)
      {
        val = val_array[y];
        board_set_value(n, x, y, val);
      }
    }
    free(val_array);
  }

  return ret;
}

static bool calculator_move_down(calculator *self, board *c, board *n, bool only_check)
{
  bool ret = false;
  uint32 cols = board_get_cols(c);
  uint32 rows = board_get_rows(c);
  uint32 x = 0, y = 0;
  uint32 val = 0;
  uint32 *val_array = NULL;

  val_array = (uint32 *)malloc(sizeof(uint32) * rows);
  if (val_array != NULL)
  {
    for (x = 0; x < cols; x++)
    {
      for (y = rows - 1; y != ~0; y--)
      {
        val = board_get_value(c, x, y);
        val_array[rows - y - 1] = val;
      }
      ret |= calculator_proc_line(self, val_array, rows, only_check);
      for (y = rows - 1; y != ~0; y--)
      {
        val = val_array[rows - y - 1];
        board_set_value(n, x, y, val);
      }
    }
    free(val_array);
  }

  return ret;
}

static bool calculator_move_left(calculator *self, board *c, board *n, bool only_check)
{
  bool ret = false;
  uint32 cols = board_get_cols(c);
  uint32 rows = board_get_rows(c);
  uint32 x = 0, y = 0;
  uint32 val = 0;
  uint32 *val_array = NULL;

  val_array = (uint32 *)malloc(sizeof(uint32) * cols);
  if (val_array != NULL)
  {
    for (y = 0; y < rows; y++)
    {
      for (x = 0; x < cols; x++)
      {
        val = board_get_value(c, x, y);
        val_array[x] = val;
      }
      ret |= calculator_proc_line(self, val_array, cols, only_check);
      for (x = 0; x < cols; x++)
      {
        val = val_array[x];
        board_set_value(n, x, y, val);
      }
    }
    free(val_array);
  }

  return ret;
}

static bool calculator_move_right(calculator *self, board *c, board *n, bool only_check)
{
  bool ret = false;
  uint32 cols = board_get_cols(c);
  uint32 rows = board_get_rows(c);
  uint32 x = 0, y = 0;
  uint32 val = 0;
  uint32 *val_array = NULL;

  val_array = (uint32 *)malloc(sizeof(uint32) * cols);
  if (val_array != NULL)
  {
    for (y = 0; y < rows; y++)
    {
      for (x = cols - 1; x != ~0; x--)
      {
        val = board_get_value(c, x, y);
        val_array[cols - x - 1] = val;
      }
      ret |= calculator_proc_line(self, val_array, cols, only_check);
      for (x = cols - 1; x != ~0; x--)
      {
        val = val_array[cols - x - 1];
        board_set_value(n, x, y, val);
      }
    }
    free(val_array);
  }

  return ret;
}

static bool calculator_proc_line(calculator *self, uint32 *array, size_t len,
                                 bool only_check)
{
  bool ret = false;

  ret = calculator_merge_array(self, array, len, only_check);
  ret |= calculator_move_array(self, array, len);

  return ret;
}

static bool calculator_merge_array(calculator *self, uint32 *array, size_t len,
                                   bool only_check)
{
  bool ret = false;
  uint32 i = 0, j = 0;

  for (i = 0; i < len - 1; i++)
  {
    if (array[i] != 0)
    {
      for (j = i + 1; j < len; j++)
      {
        if (array[j] == 0)
        {
          continue;
        }
        else if (array[i] != array[j])
        {
          break;
        }
        else
        {
          array[i] += array[j];
          array[j] = 0;
          ret = true;
          if (only_check == false)
          {
            self->score += array[i];
          }
          break;
        }
      }
      if (ret == true)
      {
        break;
      }
    }
  }

  return ret;
}

static bool calculator_move_array(calculator *self, uint32 *array, size_t len)
{
  bool ret = false;
  uint32 i = 0, j = 0;

  for (i = j = 0; i < len; i++)
  {
    if (array[i] != 0)
    {
      if (i != j)
      {
        ret = true;
      }
      array[j++] = array[i];
    }
  }

  if (i != j)
  {
    for (; j < len; j++)
    {
      array[j] = 0;
    }
  }
  return ret;
}
