#include <stdlib.h>
#include <math.h>
#include "expectimax.h"
#include "board_pool.h"
#include "../models/calculator.h"
#include "utils/hash.h"
#include "../views/output.h"

typedef struct _expectimax
{
  board_pool    *bp;
  calculator    *bc;
  cout          *o;
  float         heur_score_table[SCORE_TABLE_SIZE];
} expectimax;

typedef struct _eval_state
{
  hash          *depth_hash;
  hash          *heur_hash;
  uint32        depth_limit;
  uint32        current_depth;
  uint32        max_depth;
  uint32        cache_hits;
  uint32        moves_evaled;
} eval_state;

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
static float expectimax_score_tilechoose_node(expectimax *self,
  eval_state *state, board *b, float cprob);
static float expectimax_score_move_node(expectimax *self, eval_state *state,
  board *b, float cprob);
static inline float expectimax_score_heur_board(expectimax *self, board *b);
static inline float expectimax_score_helper(expectimax *self, board_t contents,
  const float *table);
static inline void expectimax_show_board(expectimax *self, board *b);

bool expectimax_create(expectimax **self)
{
  bool ret = false;

  *self = (expectimax *)malloc(sizeof(expectimax));
  if (*self != NULL)
  {
    board_pool_create(&(*self)->bp);
    calculator_create(&(*self)->bc);
    cout_create(&(*self)->o);
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
    cout_destory(&(*self)->o);
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
    //LOG("board scores: heur %.0f", expectimax_score_heur_board(self, b));
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      res = expectimax_score_toplevel_move(self, b, dir, depth);
      if (res > best) {
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
  eval_state state;
  uint32 distinct = 0;

  distinct = board_count_distinct_tiles(b);
  state.depth_limit = MAX(depth, distinct - 2);
  state.current_depth = 0;
  state.max_depth = 0;
  state.cache_hits = 0;
  state.moves_evaled = 0;
  hash_create(&state.depth_hash, 65535, long long, long);
  hash_create(&state.heur_hash, 65535, long, float);
  new_bd = board_pool_get(self->bp);
  if (new_bd != NULL) {
    if (calculator_move(self->bc, b, new_bd->b, dir) == true) {
      res = expectimax_score_tilechoose_node(self, &state, new_bd->b, 1.0f) + 1e-6;
    }
    board_pool_put(self->bp, new_bd);
  }
  /*
  LOG("res is %f, dir is %u, cachehits %u, cachesize %u, maxdepth %u, evaled %u moves",
    res, dir, state.cache_hits, hash_num_elements(state.depth_hash),
    state.max_depth, state.moves_evaled);
  */
  hash_destory(&state.heur_hash);
  hash_destory(&state.depth_hash);
  return res;
}

// Statistics and controls
// cprob: cumulative probability
// don't recurse into a node with a cprob less than this threshold
#define CPROB_THRESH_BASE     0.0001f
static float expectimax_score_tilechoose_node(expectimax *self,
  eval_state *state, board *b, float cprob)
{
  uint32 availables_count = 0;
  uint64 *pos_array = NULL;
  board_data *bd = NULL;
  uint32 i = 0, j = 0;
  uint32 val_array[] = GAME_NUBMER_ELEMENTS;
  float res = 0.0f;
  board_t contents = 0ULL;

  if (cprob < CPROB_THRESH_BASE || state->current_depth >= state->depth_limit) {
      state->max_depth = MAX(state->current_depth, state->max_depth);
      return expectimax_score_heur_board(self, b);
  }
  contents = board_get_contents(b);
  if (state->current_depth < MAX_SEARCH_DEPTH) {
    /*
    return heuristic from transposition table only if it means that
    the node will have been evaluated to a minimum depth of state.depth_limit.
    This will result in slightly fewer cache hits, but should not impact the
    strength of the ai negatively.
    */
    long depth = 0;
    if (hash_find(state->depth_hash, contents, &depth) == true) {
      if (depth <= state->current_depth) {
        hash_find(state->heur_hash, contents, &res);
        state->cache_hits++;
        return res;
      }
    }
  }

  availables_count = board_count_availables(b);
  pos_array = board_get_availables(b);
  cprob /= availables_count;

  bd = board_pool_get(self->bp);
  //LOG("current depth is %u", state->current_depth);
  if (bd != NULL) {
    while (i < availables_count) {
      for (j = 0; j < ARRAY_SIZE(val_array); j++) {
        board_clone_data(bd->b, b);
        board_set_value_by_pos(bd->b, pos_array[i], val_array[j]);
        //expectimax_show_board(self, bd->b);
        if (val_array[j] == 2) {
          res += expectimax_score_move_node(self, state, bd->b , cprob * 0.5f)
            * 0.5f;
        } else {
          res += expectimax_score_move_node(self, state, bd->b , cprob * 0.5f)
            * 0.5f;
        }
      }
      i++;
    }
    board_pool_put(self->bp, bd);
  }
  free(pos_array);
  //LOG("res is %f", res);
  res = res / availables_count;

  if (state->current_depth < MAX_SEARCH_DEPTH) {
    hash_add(state->depth_hash, contents, state->current_depth);
    hash_add(state->heur_hash, contents, res);
  }

  return res;
}

static float expectimax_score_move_node(expectimax *self, eval_state *state,
  board *b, float cprob)
{
  float best = 0.0f;
  float ret = 0.0f;
  enum direction dir = UP;
  board_data *new_bd = NULL;

  state->current_depth++;
  new_bd = board_pool_get(self->bp);
  if (new_bd != NULL) {
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      //LOG("dir is %u", dir);
      state->moves_evaled++;
      if (calculator_move(self->bc, b, new_bd->b, dir) == true) {
        ret = expectimax_score_tilechoose_node(self, state, new_bd->b, cprob);
        best = MAX(best, ret);
      }
    }
    board_pool_put(self->bp, new_bd);
  }
  state->current_depth--;
  //LOG("best is %f, evaled %u moves", best, state->moves_evaled);
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

static inline void expectimax_show_board(expectimax *self, board *b)
{
  LOG("");
  cout_display_board(self->o, b);
}
