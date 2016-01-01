#include <stdlib.h>

#include "game.h"
#include "board.h"
#include "random.h"
#include "calculator.h"
#include "../controllers/input.h"
#include "../views/output.h"
#include "../ai/ai.h"

typedef struct _game
{
  board             *b[2];
  uint8             current_board;
#if defined(AUTO_PLAY)
  ai                *a;
#else
  input             *in;
#endif
  cout              *out;
  random_generator  *rg;
  calculator        *calc;
} game;

static void game_over(game *self);
static void game_abort(game *self);
static void game_new_step(game *self);
static bool game_is_over(game *self);

static void game_init(game *self)
{
  uint32 val = 0;
  uint64 val_array[] = GAME_NUBMER_ELEMENTS;
  uint64 *pos_array = NULL;
  uint32 pos_array_len = 0;
  uint64 pos = 0;
  int i = 0;

  self->current_board = 0;
  for (i = 0; i < GAME_INIT_NUMBER_COUNT; i++)
  {
#if (GAME_INIT_NUMBER == 0)
    val = (uint32)random_generator_select(self->rg, val_array, ARRAY_SIZE(val_array));
#else
    val_array[0] = val_array[0];
    val = GAME_INIT_NUMBER;
#endif
    board_get_empty(self->b[self->current_board], &pos_array, &pos_array_len);
    pos = random_generator_select(self->rg, pos_array, pos_array_len);
    free(pos_array);
    board_set_value_by_pos(self->b[self->current_board], pos, val);
  }
#if defined(AUTO_PLAY) && !defined(THINKING_BY_DEPTH)
  ai_set_thinking_duration(self->a, THINKING_DURATION);
#endif
}

bool game_create(game **self)
{
  bool ret = true;
  int i = 0;

  *self = (game *)malloc(sizeof(game));
  if (*self != NULL)
  {
    for (i = 0; i < ARRAY_SIZE((*self)->b); i++)
    {
      ret &= board_create(&(*self)->b[i], ROWS_OF_BOARD, COLS_OF_BOARD);
    }
#if defined(AUTO_PLAY)
    ret &= ai_create(&(*self)->a);
#else
    ret &= input_create(&(*self)->in);
#endif
    ret &= cout_create(&(*self)->out);
    ret &= random_generator_create(&(*self)->rg);
    ret &= calculator_create(&(*self)->calc);
  }
  else
  {
    ret = false;
  }

  if (ret == true)
  {
    game_init(*self);
  }

  return ret;
}

void game_destory(game **self)
{
  int i = 0;

  if (*self != NULL)
  {
    calculator_destory(&(*self)->calc);
    random_generator_destory(&(*self)->rg);
    cout_destory(&(*self)->out);
#if defined(AUTO_PLAY)
    ai_destory(&(*self)->a);
#else
    input_destory(&(*self)->in);
#endif
    for (i = 0; i < ARRAY_SIZE((*self)->b); i++)
    {
      board_destory(&(*self)->b[i]);
    }
    free(*self);
    *self = NULL;
  }
}

void game_start(game *self)
{
  if (self != NULL)
  {
    cout_display_text(self->out, "I");
    cout_display_board(self->out, self->b[self->current_board]);
    enum direction dir;
    uint8 current = self->current_board;
    uint8 next = self->current_board == 0 ? 1 : 0;
    bool new_step = false;
    do {
#if defined(AUTO_PLAY)
      dir = ai_get(self->a, self->b[current]);
#else
      dir = input_get(self->in);
#endif
      switch (dir)
      {
        case UP:
        case LEFT:
        case RIGHT:
        case DOWN:
          new_step = calculator_move(self->calc, self->b[current], self->b[next], dir);
          if (!new_step)
          {
            continue;
          }
          break;
        case BOTTOM_OF_DIRECTION:
        default:
            game_abort(self);
            return;
          break;
      }
      cout_display_direction(self->out, dir);
      self->current_board = next;
      next = current;
      current = self->current_board;
      game_new_step(self);
      cout_display_board(self->out, self->b[current]);
      cout_display_score(self->out, calculator_get_score(self->calc));
      if (game_is_over(self) == true)
      {
        game_over(self);
        return;
      }
    } while(true);
  }
}

static void game_over(game *self)
{
  cout_display_text(self->out, "E");
}

static void game_abort(game *self)
{
  cout_display_text(self->out, "Q");
}

static void game_new_step(game *self)
{
  uint32 val = 0;
  uint64 val_array[] = GAME_NUBMER_ELEMENTS;
  uint64 *pos_array = NULL;
  uint32 pos_array_len = 0;
  uint64 pos = 0;

  board_get_empty(self->b[self->current_board], &pos_array, &pos_array_len);
  if (pos_array_len > 0)
  {
    val = (uint32)random_generator_select(self->rg, val_array, ARRAY_SIZE(val_array));
    pos = random_generator_select(self->rg, pos_array, pos_array_len);
    board_set_value_by_pos(self->b[self->current_board], pos, val);
  }
  free(pos_array);
}

static bool game_is_over(game *self)
{
  bool ret = true;
  uint64 *pos_array = NULL;
  uint32 pos_array_len = 0;
  uint8 current = self->current_board;

  board_get_empty(self->b[self->current_board], &pos_array, &pos_array_len);
  if (pos_array_len == 0)
  {
    ret &= !calculator_check_direction(self->calc, self->b[current], UP);
    ret &= !calculator_check_direction(self->calc, self->b[current], DOWN);
    ret &= !calculator_check_direction(self->calc, self->b[current], LEFT);
    ret &= !calculator_check_direction(self->calc, self->b[current], RIGHT);
  }
  else
  {
    ret = false;
  }
  free(pos_array);

  /*
  uint32 x = 0, y = 0;
  uint32 rows = board_get_rows(self->b[current]);
  uint32 cols = board_get_cols(self->b[current]);
  for (x = 0; x < cols; x++)
  {
    for (y = 0; y < rows; y++)
    {
      if (board_get_value(self->b[current], x, y) == 2048)
      {
        ret = true;
        return ret;
      }
    }
  }
  */

  return ret;
}
