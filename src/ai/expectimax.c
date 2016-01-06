#include <stdlib.h>
#include <math.h>
#include "expectimax.h"
#include "board_pool.h"
#include "../models/calculator.h"
#include "utils/hash.h"

typedef struct _expectimax
{
  board_pool    *bp;
  calculator    *bc;
  hash          *depth_hash;
  hash          *heur_hash;
  uint32        depth_limit;
  uint32        current_depth;
  uint32        max_depth;
  float         heur_score_table[SCORE_TABLE_SIZE];
} expectimax;

#define MAX(a, b)   (((a) >= (b)) ? (a) : (b))
#define MIN(a, b)   (((a) <= (b)) ? (a) : (b))

// Heuristic scoring settings
#define SCORE_LOST_PENALTY          200000.0f
#define SCORE_MONOTONICITY_POWER    4.0f
#define SCORE_MONOTONICITY_WEIGHT   47.0f
#define SCORE_SUM_POWER             3.5f
#define SCORE_SUM_WEIGHT            11.0f
#define SCORE_MERGES_WEIGHT         700.0f
#define SCORE_EMPTY_WEIGHT          270.0f

static void expectimax_init_table(expectimax *self);
static float expectimax_score_toplevel_move(expectimax *self, board *b,
  enum direction dir, uint32 depth);
static float expectimax_score_tilechoose_node(expectimax *self, board *b,
  float cprob);
static float expectimax_score_move_node(expectimax *self, board *b,
  float cprob);
static inline float expectimax_score_heur_board(expectimax *self, board *b);
static inline float expectimax_score_helper(expectimax *self, board_t contents,
  const float *table);

bool expectimax_create(expectimax **self)
{
  bool ret = false;

  *self = (expectimax *)malloc(sizeof(expectimax));
  if (*self != NULL)
  {
    board_pool_create(&(*self)->bp);
    calculator_create(&(*self)->bc);
    (*self)->depth_limit = 0;
    (*self)->current_depth = 0;
    (*self)->max_depth = 0;
    expectimax_init_table(*self);
    ret = true;
  }

  return ret;
}

void expectimax_destory(expectimax **self)
{
  if (*self != NULL)
  {
    calculator_destory(&(*self)->bc);
    board_pool_destory(&(*self)->bp);
    free(*self);
    *self = NULL;
  }
}

enum direction expectimax_search(expectimax *self, board *b,
  enum direction last_dir, uint32 depth)
{
  enum direction best_dir = BOTTOM_OF_DIRECTION;
  enum direction dir = UP;
  float res = 0.0f;
  float best = 0.0f;

  if (self != NULL && b != NULL && depth != 0)
  {
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      res = expectimax_score_toplevel_move(self, b, dir, depth);
      if(res > best) {
          best = res;
          best_dir = dir;
      }
    }
  }
  //LOG("search depth is %u", self->max_depth);
  return best_dir;
}

static void expectimax_init_table(expectimax *self)
{
  uint32 row = 0;
  uint32 i = 0;
  uint32 line[COLS_OF_BOARD];
  float sum = 0.0f;
  float monotonicity_left = 0.0f;
  float monotonicity_right = 0.0f;
  int empty = 0;
  int merges = 0;
  int prev = 0;
  int counter = 0;
  int rank = 0;

  for (row = 0; row < SCORE_TABLE_SIZE; row++) {
    for (i = 0; i < COLS_OF_BOARD; i++) {
      line[i] = (row >> i * ELEMENT_BITS) & 0x0F;
      //LOG("line[%u] is 0x%.2x", i, line[i]);
    };
    empty = 0;
    merges = 0;
    sum = 0.0f;
    prev = 0;
    counter = 0;
    for (i = 0; i < COLS_OF_BOARD; i++) {
      rank = line[i];
      sum += pow(rank, SCORE_SUM_POWER);
      if (rank == 0) {
        empty++;
      } else {
        if (prev == rank) {
          counter++;
        } else if (counter > 0) {
          merges += 1 + counter;
          counter = 0;
        }
        prev = rank;
      }
    }
    if (counter > 0) {
      merges += 1 + counter;
    }

    monotonicity_left = 0;
    monotonicity_right = 0;
    for (i = 1; i < COLS_OF_BOARD; i++) {
        if (line[i - 1] > line[i]) {
            monotonicity_left += pow(line[i - 1], SCORE_MONOTONICITY_POWER)
              - pow(line[i], SCORE_MONOTONICITY_POWER);
        } else {
            monotonicity_right += pow(line[i], SCORE_MONOTONICITY_POWER)
              - pow(line[i - 1], SCORE_MONOTONICITY_POWER);
        }
    }

    self->heur_score_table[row] = SCORE_LOST_PENALTY +
        SCORE_EMPTY_WEIGHT * empty +
        SCORE_MERGES_WEIGHT * merges -
        SCORE_MONOTONICITY_WEIGHT * MIN(monotonicity_left, monotonicity_right) -
        SCORE_SUM_WEIGHT * sum;
  }
}

static float expectimax_score_toplevel_move(expectimax *self, board *b,
  enum direction dir, uint32 depth)
{
  float res = 0.0f;
  board_data *new_bd = NULL;

  self->depth_limit = MAX(depth, board_count_distinct_tiles(b) - 2);
  self->current_depth = 0;
  self->max_depth = 0;
  hash_create(&self->depth_hash, 65535, long, long);
  hash_create(&self->heur_hash, 65535, long, float);
  new_bd = board_pool_get(self->bp);
  if (new_bd != NULL) {
    if (calculator_move(self->bc, b, new_bd->b, dir)) {
      res = expectimax_score_tilechoose_node(self, new_bd->b, 1.0f) + 1e-6;
    }
    board_pool_put(self->bp, new_bd);
  }
  hash_destory(&self->heur_hash);
  hash_destory(&self->depth_hash);
  return res;
}

// Statistics and controls
// cprob: cumulative probability
// don't recurse into a node with a cprob less than this threshold
#define CPROB_THRESH_BASE     0.0001f
static float expectimax_score_tilechoose_node(expectimax *self, board *b,
  float cprob)
{
  uint32 availables_count = 0;
  uint64 *pos_array = NULL;
  board_data *bd = NULL;
  uint32 i = 0, j = 0;
  uint32 val_array[] = GAME_NUBMER_ELEMENTS;
  float res = 0.0f;
  board_t contents = 0ULL;

  if (cprob < CPROB_THRESH_BASE || self->current_depth >= self->depth_limit) {
      self->max_depth = MAX(self->current_depth, self->max_depth);
      return expectimax_score_heur_board(self, b);
  }
  contents = board_get_contents(b);
  if (self->current_depth < MAX_SEARCH_DEPTH) {
    /*
    return heuristic from transposition table only if it means that
    the node will have been evaluated to a minimum depth of state.depth_limit.
    This will result in slightly fewer cache hits, but should not impact the
    strength of the ai negatively.
    */
    long depth = 0;
    if (hash_find(self->depth_hash, contents, &depth) == true) {
      if (depth <= self->current_depth) {
        hash_find(self->heur_hash, contents, &res);
        return res;
      }
    }
  }

  availables_count = board_count_availables(b);
  pos_array = board_get_availables(b);
  cprob /= availables_count;

  bd = board_pool_get(self->bp);
  if (bd != NULL) {
    while (i < availables_count) {
      for (j = 0; j < ARRAY_SIZE(val_array); j++) {
        board_clone_data(bd->b, b);
        board_set_value_by_pos(bd->b, pos_array[i], val_array[j]);
        res += expectimax_score_move_node(self, bd->b , cprob * 0.5f) * 0.5f;
      }
      i++;
    }
    board_pool_put(self->bp, bd);
  }
  free(pos_array);
  res = res / availables_count;

  if (self->current_depth < MAX_SEARCH_DEPTH) {
    hash_add(self->depth_hash, contents, self->current_depth);
    hash_add(self->heur_hash, contents, res);
  }

  return res;
}

static float expectimax_score_move_node(expectimax *self, board *b,
  float cprob)
{
  float best = 0.0f;
  enum direction dir = UP;
  board_data *new_bd = NULL;

  self->current_depth++;
  new_bd = board_pool_get(self->bp);
  if (new_bd != NULL) {
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      //state.moves_evaled++;
      if (calculator_move(self->bc, b, new_bd->b, dir) == true) {
        best = MAX(best, expectimax_score_tilechoose_node(self, new_bd->b, cprob));
      }
    }
    board_pool_put(self->bp, new_bd);
  }
  self->current_depth--;

  return best;
}

static inline float expectimax_score_heur_board(expectimax *self, board *b)
{
  float ret = 0.0f;
  board_t contents = 0ULL;

  contents = board_get_contents(b);
  ret = expectimax_score_helper(self, contents, self->heur_score_table);
  contents = transpose(contents);
  ret += expectimax_score_helper(self, contents, self->heur_score_table);

  return ret;
}

static inline float expectimax_score_helper(expectimax *self, board_t contents,
  const float *table)
{
  float ret = 0.0f;

  ret = table[(contents >>  0) & ROW_MASK] +
        table[(contents >> 16) & ROW_MASK] +
        table[(contents >> 32) & ROW_MASK] +
        table[(contents >> 48) & ROW_MASK];

  return ret;
}
