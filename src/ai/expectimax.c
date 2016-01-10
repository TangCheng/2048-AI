#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "expectimax.h"
#include "board_pool.h"
#include "../models/calculator.h"
#include "utils/uthash.h"
#include "utils/thread_pool.h"
#include "../views/output.h"

typedef struct _search_job
{
  expectimax      *self;
  board           *b;
  enum direction  dir;
  uint32          depth;
} search_job;

typedef struct _cache {
  void *b;
  uint64 depth;
  float  heur;
  UT_hash_handle hh;
} cache_t;

typedef struct _expectimax
{
  //board_pool        *bp;
  calculator        *bc;
  thread_pool       *tp;
  float             search_score[BOTTOM_OF_DIRECTION];
  search_job        search_jobs[BOTTOM_OF_DIRECTION];
  uint8             search_completed;
  pthread_mutex_t   search_mutex;
  pthread_cond_t    search_cond;
  cout              *o;
  float             heur_score_table[SCORE_TABLE_SIZE];
} expectimax;

typedef struct _eval_state
{
  cache_t       *caches;
  uint32        depth_limit;
  uint32        current_depth;
  uint32        max_depth;
  uint32        cache_hits;
  uint32        moves_evaled;
} eval_state;

// Heuristic scoring settings
#define SCORE_LOST_PENALTY          200000.0f
#define SCORE_MONOTONICITY_POWER    4.0f
#define SCORE_MONOTONICITY_WEIGHT   47.0f
#define SCORE_SUM_POWER             3.5f
#define SCORE_SUM_WEIGHT            11.0f
#define SCORE_MERGES_WEIGHT         700.0f
#define SCORE_EMPTY_WEIGHT          270.0f

static void expectimax_init_table(expectimax *self);
static void expectimax_score_toplevel_move(void *arg);
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
  enum direction dir = BOTTOM_OF_DIRECTION;

  *self = (expectimax *)malloc(sizeof(expectimax));
  if (*self != NULL)
  {
    //board_pool_create(&(*self)->bp);
    calculator_create(&(*self)->bc);
    cout_create(&(*self)->o);
    expectimax_init_table(*self);
    // do NOTHING, just remove warning of compiler
    expectimax_show_board(*self, NULL);
    (*self)->search_completed = 0;
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      (*self)->search_jobs[dir].self = *self;
      (*self)->search_jobs[dir].dir = dir;
      (*self)->search_score[dir] = 0.0f;
    }
    pthread_mutex_init(&(*self)->search_mutex, NULL);
    pthread_cond_init(&(*self)->search_cond, NULL);
    thread_pool_create(&(*self)->tp, sysconf(_SC_NPROCESSORS_ONLN) + 1);
    ret = true;
  }

  return ret;
}

void expectimax_destory(expectimax **self)
{
  if (*self != NULL)
  {
    thread_pool_destory(&(*self)->tp);
    pthread_mutex_destroy(&(*self)->search_mutex);
    pthread_cond_destroy(&(*self)->search_cond);
    calculator_destory(&(*self)->bc);
    //board_pool_destory(&(*self)->bp);
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
  float best = 0.0f;

  if (self != NULL && b != NULL && depth != 0)
  {
    pthread_mutex_lock(&self->search_mutex);
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      self->search_score[dir] = 0.0f;
    }
    self->search_completed = 0;
    pthread_mutex_unlock(&self->search_mutex);
    //LOG("board scores: heur %.0f", expectimax_score_heur_board(self, b));
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      self->search_jobs[dir].b = b;
      self->search_jobs[dir].depth = depth;
      thread_pool_add_job(self->tp, expectimax_score_toplevel_move,
        &self->search_jobs[dir]);
    }

    pthread_mutex_lock(&self->search_mutex);
    while (self->search_completed != 0x0F) {
      pthread_cond_wait(&self->search_cond, &self->search_mutex);
    }
    pthread_mutex_unlock(&self->search_mutex);
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      if (self->search_score[dir] >= best) {
        best = self->search_score[dir];
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

static void expectimax_score_toplevel_move(void *arg)
{
  search_job *p = (search_job *)arg;
  expectimax *self = p->self;
  board *b = p->b;
  enum direction dir = p->dir;
  uint32 depth = p->depth;
  float res = 0.0f;
  board *new_bd = NULL;
  eval_state state;
  uint32 distinct = 0;

  distinct = board_count_distinct_tiles(b);
  state.depth_limit = MAX(depth, distinct / 2 + 1);
  state.current_depth = 0;
  state.max_depth = 0;
  state.cache_hits = 0;
  state.moves_evaled = 0;
  state.caches = NULL;
  //pthread_mutex_lock(&self->search_mutex);
  //new_bd = board_pool_get(self->bp);
  //pthread_mutex_unlock(&self->search_mutex);
  board_create(&new_bd, ROWS_OF_BOARD, COLS_OF_BOARD);
  if (new_bd != NULL) {
    if (calculator_move(self->bc, b, new_bd, dir) == true) {
      res = expectimax_score_tilechoose_node(self, &state, new_bd, 1.0f) + 1e-6;
    }
    //pthread_mutex_lock(&self->search_mutex);
    //board_pool_put(self->bp, new_bd);
    //pthread_mutex_unlock(&self->search_mutex);
    board_destory(&new_bd);
  }
  /*
  LOG("res is %f, dir is %u, cachehits %u, cachesize %u, maxdepth %u, evaled %u moves",
    res, dir, state.cache_hits, HASH_COUNT(state.caches), state.max_depth,
    state.moves_evaled);
  */
  cache_t *cache, *tmp;
  HASH_ITER(hh, state.caches, cache, tmp) {
    HASH_DEL(state.caches, cache);
    free(cache);
  }

  pthread_mutex_lock(&self->search_mutex);
  self->search_score[dir] = res;
  self->search_completed |= 1 << dir;
  pthread_mutex_unlock(&self->search_mutex);
  pthread_cond_signal(&self->search_cond);
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
  board *bd = NULL;
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
    cache_t *cache = NULL;
    HASH_FIND_PTR(state->caches, &contents, cache);
    if (cache != NULL) {
      if (cache->depth <= state->current_depth) {
        state->cache_hits++;
        return cache->heur;
      }
    }
  }

  availables_count = board_count_availables(b);
  pos_array = board_get_availables(b);
  cprob /= availables_count;

  //pthread_mutex_lock(&self->search_mutex);
  //bd = board_pool_get(self->bp);
  //pthread_mutex_unlock(&self->search_mutex);
  board_create(&bd, ROWS_OF_BOARD, COLS_OF_BOARD);
  //LOG("current depth is %u", state->current_depth);
  if (bd != NULL) {
    while (i < availables_count) {
      for (j = 0; j < ARRAY_SIZE(val_array); j++) {
        board_clone_data(bd, b);
        board_set_value_by_pos(bd, pos_array[i], val_array[j]);
        //expectimax_show_board(self, bd->b);
        res += expectimax_score_move_node(self, state, bd , cprob * 0.5f)
          * 0.5f;
      }
      i++;
    }
    //pthread_mutex_lock(&self->search_mutex);
    //board_pool_put(self->bp, bd);
    //pthread_mutex_unlock(&self->search_mutex);
    board_destory(&bd);
  }
  free(pos_array);
  //LOG("res is %f", res);
  res = res / availables_count;

  if (state->current_depth < MAX_SEARCH_DEPTH) {
    cache_t *cache = (cache_t *)malloc(sizeof(cache_t));
    cache->b = (void *)contents;
    cache->depth = state->current_depth;
    cache->heur = res;
    HASH_ADD_PTR(state->caches, b, cache);
  }

  return res;
}

static float expectimax_score_move_node(expectimax *self, eval_state *state,
  board *b, float cprob)
{
  float best = 0.0f;
  float ret = 0.0f;
  enum direction dir = UP;
  board *new_bd = NULL;

  state->current_depth++;
  //pthread_mutex_lock(&self->search_mutex);
  //new_bd = board_pool_get(self->bp);
  //pthread_mutex_unlock(&self->search_mutex);
  board_create(&new_bd, ROWS_OF_BOARD, COLS_OF_BOARD);
  if (new_bd != NULL) {
    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      //LOG("dir is %u", dir);
      state->moves_evaled++;
      if (calculator_move(self->bc, b, new_bd, dir) == true) {
        ret = expectimax_score_tilechoose_node(self, state, new_bd, cprob);
        best = MAX(best, ret);
      }
    }
    //pthread_mutex_lock(&self->search_mutex);
    //board_pool_put(self->bp, new_bd);
    //pthread_mutex_unlock(&self->search_mutex);
    board_destory(&new_bd);
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
