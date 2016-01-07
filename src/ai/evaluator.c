#include <stdlib.h>
#include <math.h>
#include "evaluator.h"

typedef struct _evaluator
{
  double   smoothness_weight;
  double   monotonicity_weight;
  double   empty_weight;
  double   max_value_weight;
} evaluator;

#define SMOOTHNESS_WEIGHT         0.1
#define MONOTONICITY_WEIGHT       1.0
#define EMPTY_WEIGHT              2.7
#define MAX_VALUE_WEIGHT          1.0

static void evaluator_check_around(evaluator *self, board *b, uint32 x,
  uint32 y, uint32 value, bool **flags);
static bool evluator_find_farthest_pos(evaluator *self, board *b, uint32 x,
  uint32 y, uint32 *target_x, uint32 *target_y, enum direction dir);

bool evaluator_create(evaluator **self)
{
  bool ret = false;

  *self = (evaluator *)malloc(sizeof(evaluator));
  if (*self != NULL)
  {
    ret = true;
    (*self)->smoothness_weight = SMOOTHNESS_WEIGHT;
    (*self)->monotonicity_weight = MONOTONICITY_WEIGHT;
    (*self)->empty_weight = EMPTY_WEIGHT;
    (*self)->max_value_weight = MAX_VALUE_WEIGHT;
  }

  return ret;
}

void evaluator_destory(evaluator **self)
{
  if (*self != NULL)
  {
    free(*self);
    *self = NULL;
  }
}

void evaluator_set_smoothness_weight(evaluator *self, float weight)
{
  if (self != NULL)
  {
    self->smoothness_weight = weight;
  }
}

void evaluator_set_monotonicity_weight(evaluator *self, float weight)
{
  if (self != NULL)
  {
    self->monotonicity_weight = weight;
  }
}

void evaluator_set_empty_weight(evaluator *self, float weight)
{
  if (self != NULL)
  {
    self->empty_weight = weight;
  }
}

void evaluator_set_max_value_weight(evaluator *self, float weight)
{
  if (self != NULL)
  {
    self->max_value_weight = weight;
  }
}

double evaluator_get_value(evaluator *self, board *b)
{
  double smoothness = 0.0;
  double monotonicity = 0.0;
  double empty = 0.0;
  double max_value = 0.0;
  double value = 0.0;

  if (self != NULL)
  {
    smoothness = (double)evaluator_smoothness(self, b) * self->smoothness_weight;
    monotonicity = (double)evaluator_monotonicity(self, b) * self->monotonicity_weight;
    empty = evaluator_empty(self, b) * self->empty_weight;
    max_value = (double)evaluator_max_value(self, b) * self->max_value_weight;
  }

  value = smoothness + monotonicity + empty + max_value;
  //LOG("value is %.13f", value);
  return value;
}

uint32 evaluator_islands(evaluator *self, board *b)
{
  uint32 islands = 0;
  uint32 rows = 0, cols = 0;
  uint32 val = 0;
  uint32 x = 0, y = 0;
  bool **flags = NULL;

  if (self != NULL && b != NULL)
  {
    rows = board_get_rows(b);
    cols = board_get_cols(b);
    flags = (bool **)malloc(sizeof(bool *) * rows);
    if (flags != NULL)
    {
      flags[0] = (bool *)malloc(sizeof(bool) * rows * cols);
      if (flags[0] != NULL)
      {
        for (y = 0; y < rows; y++)
        {
          flags[y] = flags[0] + y * cols;
          for (x = 0; x < cols; x++)
          {
            flags[y][x] = false;
          }
        }

        for (x = 0; x < cols; x++)
        {
          for (y = 0; y < rows; y++)
          {
            val = board_get_value(b, x, y);
            if (val != 0 && flags[y][x] == false)
            {
              islands++;
              evaluator_check_around(self, b, x, y, val, flags);
            }
          }
        }
        free(flags[0]);
      }
      free(flags);
    }
  }
  //LOG("islands = %u", islands);
  return islands;
}

int32 evaluator_smoothness(evaluator *self, board *b)
{
  int32 smoothness = 0;
  uint32 rows = 0, cols = 0;
  uint32 val = 0, target_val = 0;
  int32 value = 0, target_value = 0;
  uint32 x = 0, y = 0;
  uint32 target_x = 0, target_y = 0;
  enum direction dir = BOTTOM_OF_DIRECTION;

  if (self != NULL && b != NULL)
  {
    rows = board_get_rows(b);
    cols = board_get_cols(b);
    for (x = 0; x < cols; x++)
    {
      for (y = 0; y < rows; y++)
      {
        val = board_get_value(b, x, y);
        if (val != 0)
        {
          value = log(val) / log(2);
          for (dir = DOWN; dir < BOTTOM_OF_DIRECTION; dir += 2)
          {
            if (evluator_find_farthest_pos(self, b, x, y, &target_x, &target_y,
              dir) == true)
            {
              target_val = board_get_value(b, target_x, target_y);
              if (target_val != 0)
              {
                target_value = log(target_val) / log(2);
                smoothness -= abs(value - target_value);
              }
            }
          }
        }
      }
    }
  }
  //LOG("smoothness is %d", smoothness);
  return smoothness;
}

int32 evaluator_monotonicity(evaluator *self, board *b)
{
  int32 monotonicity = 0;
  int32 totals[BOTTOM_OF_DIRECTION] = {0, 0, 0, 0};
  uint32 rows = 0, cols = 0;
  uint32 x = 0, y = 0;
  uint32 current = 0, next = 0;
  uint32 val = 0, current_val = 0, next_val = 0;

  if (self != NULL && b != NULL)
  {
    rows = board_get_rows(b);
    cols = board_get_cols(b);

    // LEFT and RIGHT
    for (y = 0; y < rows; y++)
    {
      current = 0;
      next = current + 1;
      while (next < cols)
      {
        val = board_get_value(b, next, y);
        while (next < cols && val == 0)
        {
          next++;
        }
        if (next >= cols)
        {
          next--;
        }
        val = board_get_value(b, next, y);
        if (val != 0)
        {
          next_val = log(val) / log(2);
        }
        else
        {
          next_val = 0;
        }
        val = board_get_value(b, current, y);
        if (val != 0)
        {
          current_val = log(val) / log(2);
        }
        else
        {
          current_val = 0;
        }
        if (current_val > next_val)
        {
          totals[LEFT] += next_val - current_val;
        }
        else if (next_val > current_val)
        {
          totals[RIGHT] += current_val - next_val;
        }
        current = next;
        next++;
      }
    }

    // UP and DOWN
    for (x = 0; x < cols; x++)
    {
      current = 0;
      next = current + 1;
      while (next < rows)
      {
        val = board_get_value(b, x, next);
        while (next < rows && val == 0)
        {
          next++;
        }
        if (next >= rows)
        {
          next--;
        }
        val = board_get_value(b, x, next);
        if (val != 0)
        {
          next_val = log(val) / log(2);
        }
        else
        {
          next_val = 0;
        }
        val = board_get_value(b, x, current);
        if (val != 0)
        {
          current_val = log(val) / log(2);
        }
        else
        {
          current_val = 0;
        }
        if (current_val > next_val)
        {
          totals[UP] += next_val - current_val;
        }
        else if (next_val > current_val)
        {
          totals[DOWN] += current_val - next_val;
        }
        current = next;
        next++;
      }
    }
  }
  monotonicity = MAX(totals[UP], totals[DOWN])
     +  MAX(totals[LEFT], totals[RIGHT]);
  //LOG("monotonicity is %d", monotonicity);
  return monotonicity;
}

double evaluator_empty(evaluator *self, board *b)
{
  uint32 empty_count = 0;

  if (self != NULL && b != NULL)
  {
    empty_count = board_count_availables(b);
  }

  //LOG("empty is %f", log(empty_count));
  if (empty_count == 0)
  {
    return 0;
  }
  else
  {
    return log(empty_count);
  }
}

uint32 evaluator_max_value(evaluator *self, board *b)
{
  uint32 max = 0;
  uint32 x = 0, y = 0;

  if (self != NULL && b != NULL)
  {
    uint32 rows = board_get_rows(b);
    uint32 cols = board_get_cols(b);

    for (x = 0; x < cols; x++)
    {
      for (y = 0; y < rows; y++)
      {
        max = MAX(max, board_get_value(b, x, y));
      }
    }
  }
  //LOG("max value is %u", (uint32)(log(max) / log(2)));
  return (uint32)(log(max) / log(2));
}

uint32 evaluator_sum(evaluator *self, board *b)
{
  uint64 sum = 0;
  uint32 x = 0, y = 0;

  if (self != NULL && b != NULL)
  {
    uint32 rows = board_get_rows(b);
    uint32 cols = board_get_cols(b);

    for (x = 0; x < cols; x++)
    {
      for (y = 0; y < rows; y++)
      {
        sum += board_get_value(b, x, y);
      }
    }
  }
  return (uint32)(log(sum) / log(2));
}

static void evaluator_check_around(evaluator *self, board *b, uint32 x,
  uint32 y, uint32 value, bool **flags)
{
  uint32 rows = board_get_rows(b);
  uint32 cols = board_get_cols(b);
  if (x < cols && y < rows && flags[y][x] == false)
  {
    uint32 current_value = board_get_value(b, x, y);
    if (current_value != 0 && current_value == value)
    {
      flags[y][x] = true;
      if (y > 0)
      {
        evaluator_check_around(self, b, x, y - 1, value, flags);
      }
      if (x > 0)
      {
        evaluator_check_around(self, b, x - 1, y, value, flags);
      }
      evaluator_check_around(self, b, x, y + 1, value, flags);
      evaluator_check_around(self, b, x + 1, y, value, flags);
    }
  }
}

static bool evluator_find_farthest_pos(evaluator *self, board *b, uint32 x,
  uint32 y, uint32 *target_x, uint32 *target_y, enum direction dir)
{
  bool ret = false;
  uint32 rows = board_get_rows(b);
  uint32 cols = board_get_cols(b);
  uint32 val = 0;

  switch (dir)
  {
    case DOWN:
      *target_x = x;
      do {
        *target_y = y + 1;
        y = *target_y;
        val = board_get_value(b, x, y);
        if (val != 0)
        {
          ret = true;
          break;
        }
      } while(*target_y < rows);
      break;
    case RIGHT:
      *target_y = y;
      do {
        *target_x = x + 1;
        x = *target_x;
        val = board_get_value(b, x, y);
        if (val != 0)
        {
          ret = true;
          break;
        }
      } while(*target_x < cols);
      break;
    default:
      break;
  }

  return ret;
}
