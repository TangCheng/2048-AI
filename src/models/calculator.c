#include <stdlib.h>
#include "calculator.h"

typedef struct _calculator
{
  row_t   row_left_table[TRANSPOSE_TABLE_SIZE];
  row_t   row_right_table[TRANSPOSE_TABLE_SIZE];
  board_t col_up_table[TRANSPOSE_TABLE_SIZE];
  board_t col_down_table[TRANSPOSE_TABLE_SIZE];
  float   score_table[SCORE_TABLE_SIZE];
  uint64  score;
  board   *temp;
} calculator;

static void calculator_init_tables(calculator *self);
static inline bool calculator_move_up(calculator *self, board *c, board *n);
static inline bool calculator_move_down(calculator *self, board *c, board *n);
static inline bool calculator_move_left(calculator *self, board *c, board *n);
static inline bool calculator_move_right(calculator *self, board *c, board *n);

bool calculator_create(calculator **self)
{
  bool ret = false;

  *self = (calculator *)malloc(sizeof(calculator));
  if (*self != NULL)
  {
    ret = board_create(&(*self)->temp, ROWS_OF_BOARD, COLS_OF_BOARD);
    calculator_init_tables(*self);
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
    ret = calculator_move(self, b, self->temp, dir);
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
        ret = calculator_move_up(self, current, next);
        break;
      case DOWN:
        ret = calculator_move_down(self, current, next);
        break;
      case LEFT:
        ret = calculator_move_left(self, current, next);
        break;
      case RIGHT:
        ret = calculator_move_right(self, current, next);
        break;
      default:
        break;
    }
    if (ret == true)
    {
      board_t contents = board_get_contents(next);
      self->score = self->score_table[(contents >>  0) & ROW_MASK] +
                    self->score_table[(contents >> 16) & ROW_MASK] +
                    self->score_table[(contents >> 32) & ROW_MASK] +
                    self->score_table[(contents >> 48) & ROW_MASK];
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

static void calculator_init_tables(calculator *self)
{
  uint32 row = 0;
  uint32 i = 0, j = 0;
  uint32 line[COLS_OF_BOARD];
  uint32 rank = 0;
  float score = 0.0f;
  row_t result = 0;
  row_t rev_result = 0;
  uint32 rev_row = 0;

  for (row = 0; row < TRANSPOSE_TABLE_SIZE; row++)
  {
    for (i = 0; i < COLS_OF_BOARD; i++)
    {
      line[i] = (row >> i * ELEMENT_BITS) & 0x0F;
      //LOG("line[%u] is 0x%.2x", i, line[i]);
    };

    // Score
    score = 0.0f;
    for (i = 0; i < COLS_OF_BOARD; i++)
    {
      rank = line[i];
      if (rank >= 2)
      {
          // the score is the total sum of the tile and all intermediate merged tiles
          score += (rank - 1) * (1 << rank);
      }
    }
    self->score_table[row] = score;

    // execute a move to the left
    for (i = 0; i < COLS_OF_BOARD - 1; i++)
    {
      for (j = i + 1; j < COLS_OF_BOARD; j++)
      {
          if (line[j] != 0)
          {
            break;
          }
      }
      if (j == COLS_OF_BOARD)
      {
        break; // no more tiles to the right
      }

      if (line[i] == 0)
      {
          line[i] = line[j];
          line[j] = 0;
          i--; // retry this entry
      }
      else if (line[i] == line[j])
      {
        if(line[i] != 0xf)
        {
          /* Pretend that 32768 + 32768 = 32768 (representational limit). */
          line[i]++;
        }
        line[j] = 0;
      }
    }

    result = (line[0] <<  0) |
             (line[1] <<  4) |
             (line[2] <<  8) |
             (line[3] << 12);
    rev_result = reverse_row(result);
    rev_row = reverse_row(row);

    self->row_left_table [    row] =                row  ^                result;
    self->row_right_table[rev_row] =            rev_row  ^            rev_result;
    self->col_up_table   [    row] = unpack_col(    row) ^ unpack_col(    result);
    self->col_down_table [rev_row] = unpack_col(rev_row) ^ unpack_col(rev_result);
  }
}

static inline bool calculator_move_up(calculator *self, board *c, board *n)
{
  bool ret = false;
  board_t origin = board_get_contents(c);
  board_t t = transpose(origin);
  origin ^= self->col_up_table[(t >>  0) & ROW_MASK] <<  0;
  origin ^= self->col_up_table[(t >> 16) & ROW_MASK] <<  4;
  origin ^= self->col_up_table[(t >> 32) & ROW_MASK] <<  8;
  origin ^= self->col_up_table[(t >> 48) & ROW_MASK] << 12;

  ret = (origin != transpose(t));
  if (ret == true) {
    board_set_contents(n, origin);
  }
  return ret;
}

static inline bool calculator_move_down(calculator *self, board *c, board *n)
{
  bool ret = false;
  board_t origin = board_get_contents(c);
  board_t t = transpose(origin);
  origin ^= self->col_down_table[(t >>  0) & ROW_MASK] <<  0;
  origin ^= self->col_down_table[(t >> 16) & ROW_MASK] <<  4;
  origin ^= self->col_down_table[(t >> 32) & ROW_MASK] <<  8;
  origin ^= self->col_down_table[(t >> 48) & ROW_MASK] << 12;

  ret = (origin != transpose(t));
  if (ret == true) {
    board_set_contents(n, origin);
  }
  return ret;
}

static inline bool calculator_move_left(calculator *self, board *c, board *n)
{
  bool ret = false;
  board_t origin = board_get_contents(c);
  board_t t = origin;
  origin ^= (board_t)(self->row_left_table[(t >>  0) & ROW_MASK]) <<  0;
  origin ^= (board_t)(self->row_left_table[(t >> 16) & ROW_MASK]) << 16;
  origin ^= (board_t)(self->row_left_table[(t >> 32) & ROW_MASK]) << 32;
  origin ^= (board_t)(self->row_left_table[(t >> 48) & ROW_MASK]) << 48;

  ret = (origin != t);
  if (ret == true) {
    board_set_contents(n, origin);
  }
  return ret;
}

static inline bool calculator_move_right(calculator *self, board *c, board *n)
{
  bool ret = false;
  board_t origin = board_get_contents(c);
  board_t t = origin;
  origin ^= (board_t)(self->row_right_table[(t >>  0) & ROW_MASK]) <<  0;
  origin ^= (board_t)(self->row_right_table[(t >> 16) & ROW_MASK]) << 16;
  origin ^= (board_t)(self->row_right_table[(t >> 32) & ROW_MASK]) << 32;
  origin ^= (board_t)(self->row_right_table[(t >> 48) & ROW_MASK]) << 48;

  ret = (origin != t);
  if (ret == true) {
    board_set_contents(n, origin);
  }
  return ret;
}
