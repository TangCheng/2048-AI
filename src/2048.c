/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * 2048.c
 * Copyright (C) 2015 TangCheng <tangcheng2005@gmail.com>
 *
 * 2048 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * 2048 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "constants.h"
#include "ai/utils/uthash.h"

#define BUF_LEN	100
#define MILLIONSECONDS_PER_SECOND	1000ULL
#define MILLIONSECONDS_PER_MINUTE (MILLIONSECONDS_PER_SECOND * 60ULL)
#define MILLIONSECONDS_PER_HOUR		(MILLIONSECONDS_PER_MINUTE * 60ULL)
#define MILLIONSECONDS_PER_DAY 		(MILLIONSECONDS_PER_HOUR * 24ULL)

typedef struct _cache {
  void *b;
  uint64 depth;
  float  heur;
  UT_hash_handle hh;
} cache_t;

/* Optimizing the game */
typedef struct _eval_state {
  cache_t *caches;
  uint64 maxdepth;
  uint64 curdepth;
  uint64 cachehits;
  unsigned long moves_evaled;
  uint64 depth_limit;
} eval_state;

static inline void show_time(struct timeval *now)
{
	char buf[BUF_LEN] = {0};
	time_t t;
	struct tm *today;
	t = now->tv_sec;
	today = localtime(&t);
	strftime(buf, BUF_LEN, "%Y-%m-%d %H:%M:%S", today);
	LOG("%s.%u", buf, (uint32)(now->tv_usec / 1000));
}

static inline void show_elapsed(struct timeval *begin, struct timeval *end)
{
	uint64 elapsed = 0;
	elapsed = (end->tv_sec * 1000 + end->tv_usec / 1000);
	elapsed -= (begin->tv_sec * 1000 + begin->tv_usec / 1000);

	uint32 days = 0, hours = 0, mins = 0, secs = 0;
	days = elapsed / MILLIONSECONDS_PER_DAY;
	elapsed %= MILLIONSECONDS_PER_DAY;
	hours = elapsed / MILLIONSECONDS_PER_HOUR;
	elapsed %= MILLIONSECONDS_PER_HOUR;
	mins = elapsed / MILLIONSECONDS_PER_MINUTE;
	elapsed %= MILLIONSECONDS_PER_MINUTE;
	secs = elapsed / MILLIONSECONDS_PER_SECOND;
	elapsed %= MILLIONSECONDS_PER_SECOND;
	LOG("The program elapsed %u days, %u hours, %u mins, %u secs, %llu ms",
		days, hours, mins, secs, elapsed);
}

static inline void print_board(board_t b) {
  int i, j;
  for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
          uint8_t powerVal = (b) & 0xf;
          fprintf(stdout, "|%u|", (powerVal == 0) ? 0 : 1 << powerVal);
          b >>= 4;
      }
      fprintf(stdout, "\n");
  }
}

// Count the number of empty positions (= zero nibbles) in a board.
// Precondition: the board cannot be fully empty.
static int count_empty(board_t x)
{
  if (x == 0) {
    return 16;
  } else {
    x |= (x >> 2) & 0x3333333333333333ULL;
    x |= (x >> 1);
    x = ~x & 0x1111111111111111ULL;
    // At this point each nibble is:
    //  0 if the original nibble was non-zero
    //  1 if the original nibble was zero
    // Next sum them all
    x += x >> 32;
    x += x >> 16;
    x += x >>  8;
    x += x >>  4; // this can overflow to the next nibble if there were 16 empty positions
    return x & 0xf;
  }
}

/* We can perform state lookups one row at a time by using arrays with 65536 entries. */

/* Move tables. Each row or compressed column is mapped to (oldrow^newrow) assuming row/col 0.
 *
 * Thus, the value is 0 if there is no move, and otherwise equals a value that can easily be
 * xor'ed into the current board state to update the board. */
static row_t row_left_table [65536];
static row_t row_right_table[65536];
static board_t col_up_table[65536];
static board_t col_down_table[65536];
static float heur_score_table[65536];
static float score_table[65536];

// Heuristic scoring settings
static const float SCORE_LOST_PENALTY = 200000.0f;
static const float SCORE_MONOTONICITY_POWER = 4.0f;
static const float SCORE_MONOTONICITY_WEIGHT = 47.0f;
static const float SCORE_SUM_POWER = 3.5f;
static const float SCORE_SUM_WEIGHT = 11.0f;
static const float SCORE_MERGES_WEIGHT = 700.0f;
static const float SCORE_EMPTY_WEIGHT = 270.0f;

void init_tables()
{
  for (unsigned row = 0; row < 65536; ++row) {
    unsigned line[4] = {
      (row >>  0) & 0xf,
      (row >>  4) & 0xf,
      (row >>  8) & 0xf,
      (row >> 12) & 0xf
    };

    // Score
    float score = 0.0f;
    for (int i = 0; i < 4; ++i) {
      int rank = line[i];
      if (rank >= 2) {
        // the score is the total sum of the tile and all intermediate merged tiles
        score += (rank - 1) * (1 << rank);
      }
    }
    score_table[row] = score;


    // Heuristic score
    float sum = 0;
    int empty = 0;
    int merges = 0;

    int prev = 0;
    int counter = 0;
    for (int i = 0; i < 4; ++i) {
      int rank = line[i];
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

    float monotonicity_left = 0;
    float monotonicity_right = 0;
    for (int i = 1; i < 4; ++i) {
      if (line[i-1] > line[i]) {
        monotonicity_left += pow(line[i-1], SCORE_MONOTONICITY_POWER) -
          pow(line[i], SCORE_MONOTONICITY_POWER);
      } else {
        monotonicity_right += pow(line[i], SCORE_MONOTONICITY_POWER) -
          pow(line[i-1], SCORE_MONOTONICITY_POWER);
      }
    }

    heur_score_table[row] = SCORE_LOST_PENALTY +
        SCORE_EMPTY_WEIGHT * empty +
        SCORE_MERGES_WEIGHT * merges -
        SCORE_MONOTONICITY_WEIGHT * MIN(monotonicity_left, monotonicity_right) -
        SCORE_SUM_WEIGHT * sum;

#ifndef MERGE_ONCE_PER_ROW_OR_COL
    // execute a move to the left
    for (int i = 0; i < 3; ++i) {
      int j;
      for (j = i + 1; j < 4; ++j) {
        if (line[j] != 0) break;
      }
      if (j == 4) break; // no more tiles to the right

      if (line[i] == 0) {
        line[i] = line[j];
        line[j] = 0;
        i--; // retry this entry
      } else if (line[i] == line[j]) {
        if(line[i] != 0xf) {
          /* Pretend that 32768 + 32768 = 32768 (representational limit). */
          line[i]++;
        }
        line[j] = 0;
      }
    }
#else
    // execute a move to the left
    for (int i = 0; i < 4 - 1; i++) {
      int j;
      for (j = i + 1; j < 4; j++) {
        if (line[j] != 0) {
          break;
        }
      }
      if (j == 4) {
        break; // no more tiles to the right
      }

      if (line[i] == line[j]) {
        if(line[i] != 0xf) {
          /* Pretend that 32768 + 32768 = 32768 (representational limit). */
          line[i]++;
        }
        line[j] = 0;
        break;
      }
    }
    for (int i = 0; i < 4 - 1; i++) {
      int j;
      if (line[i] == 0) {
        for (j = i + 1; j < 4; j++) {
          if (line[j] != 0) {
            break;
          }
        }
        if (j == 4) {
          break; // no more tiles to the right
        }
        line[i] = line[j];
        line[j] = 0;
        i--; // retry this entry
      }
    }
#endif
    row_t result = (line[0] <<  0) |
                   (line[1] <<  4) |
                   (line[2] <<  8) |
                   (line[3] << 12);
    row_t rev_result = reverse_row(result);
    unsigned rev_row = reverse_row(row);

    row_left_table [    row] =                row  ^                result;
    row_right_table[rev_row] =            rev_row  ^            rev_result;
    col_up_table   [    row] = unpack_col(    row) ^ unpack_col(    result);
    col_down_table [rev_row] = unpack_col(rev_row) ^ unpack_col(rev_result);
  }
}

static inline board_t execute_move_up(board_t b) {
  board_t ret = b;
  board_t t = transpose(b);
  ret ^= col_up_table[(t >>  0) & ROW_MASK] <<  0;
  ret ^= col_up_table[(t >> 16) & ROW_MASK] <<  4;
  ret ^= col_up_table[(t >> 32) & ROW_MASK] <<  8;
  ret ^= col_up_table[(t >> 48) & ROW_MASK] << 12;
  return ret;
}

static inline board_t execute_move_down(board_t b) {
  board_t ret = b;
  board_t t = transpose(b);
  ret ^= col_down_table[(t >>  0) & ROW_MASK] <<  0;
  ret ^= col_down_table[(t >> 16) & ROW_MASK] <<  4;
  ret ^= col_down_table[(t >> 32) & ROW_MASK] <<  8;
  ret ^= col_down_table[(t >> 48) & ROW_MASK] << 12;
  return ret;
}

static inline board_t execute_move_left(board_t b) {
  board_t ret = b;
  ret ^= (board_t)(row_left_table[(b >>  0) & ROW_MASK]) <<  0;
  ret ^= (board_t)(row_left_table[(b >> 16) & ROW_MASK]) << 16;
  ret ^= (board_t)(row_left_table[(b >> 32) & ROW_MASK]) << 32;
  ret ^= (board_t)(row_left_table[(b >> 48) & ROW_MASK]) << 48;
  return ret;
}

static inline board_t execute_move_right(board_t b) {
  board_t ret = b;
  ret ^= (board_t)(row_right_table[(b >>  0) & ROW_MASK]) <<  0;
  ret ^= (board_t)(row_right_table[(b >> 16) & ROW_MASK]) << 16;
  ret ^= (board_t)(row_right_table[(b >> 32) & ROW_MASK]) << 32;
  ret ^= (board_t)(row_right_table[(b >> 48) & ROW_MASK]) << 48;
  return ret;
}

/* Execute a move. */
static inline board_t execute_move(enum direction dir, board_t b) {
  switch(dir) {
    case UP:
      return execute_move_up(b);
    case DOWN:
      return execute_move_down(b);
    case LEFT:
      return execute_move_left(b);
    case RIGHT:
      return execute_move_right(b);
    default:
      return ~0ULL;
  }
}

static inline int count_distinct_tiles(board_t b) {
  uint16_t bitset = 0;
  while (b) {
    bitset |= 1<<(b & 0xf);
    b >>= 4;
  }

  // Don't count empty tiles.
  bitset >>= 1;

  int count = 0;
  while (bitset) {
    bitset &= bitset - 1;
    count++;
  }
  return count;
}

// score a single board heuristically
static float score_heur_board(board_t b);
// score a single board actually (adding in the score from spawned 4 tiles)
static float score_board(board_t b);
// score over all possible moves
static float score_move_node(eval_state *state, board_t b, float cprob);
// score over all possible tile choices and placements
static float score_tilechoose_node(eval_state *state, board_t b, float cprob);


static float score_helper(board_t b, const float* table) {
  return table[(b >>  0) & ROW_MASK] +
         table[(b >> 16) & ROW_MASK] +
         table[(b >> 32) & ROW_MASK] +
         table[(b >> 48) & ROW_MASK];
}

static float score_heur_board(board_t b) {
  return score_helper(          b , heur_score_table) +
         score_helper(transpose(b), heur_score_table);
}

static float score_board(board_t b) {
  return score_helper(b, score_table);
}

// Statistics and controls
// cprob: cumulative probability
// don't recurse into a node with a cprob less than this threshold
static const float CPROB_THRESH_BASE = 0.0001f;

static float score_tilechoose_node(eval_state *state, board_t b, float cprob) {
  if (cprob < CPROB_THRESH_BASE || state->curdepth >= state->depth_limit) {
      state->maxdepth = MAX(state->curdepth, state->maxdepth);
      return score_heur_board(b);
  }
  float res = 0.0f;
  if (state->curdepth < MAX_SEARCH_DEPTH) {
    cache_t *cache = NULL;
    HASH_FIND_PTR(state->caches, &b, cache);
    if (cache != NULL) {
      if (cache->depth <= state->curdepth) {
        state->cachehits++;
        return cache->heur;
      }
    }
  }

  int num_open = count_empty(b);
  cprob /= num_open;

  board_t tmp = b;
  board_t tile_2 = 1;
  while (tile_2) {
    if ((tmp & 0xf) == 0) {
      res += score_move_node(state, b |  tile_2      , cprob * 0.5f) * 0.5f;
      res += score_move_node(state, b | (tile_2 << 1), cprob * 0.5f) * 0.5f;
    }
    tmp >>= 4;
    tile_2 <<= 4;
  }
  res = res / num_open;

  if (state->curdepth < MAX_SEARCH_DEPTH) {
    cache_t *cache = (cache_t *)malloc(sizeof(cache_t));
    cache->b = (void *)b;
    cache->depth = state->curdepth;
    cache->heur = res;
    HASH_ADD_PTR(state->caches, b, cache);
  }

  return res;
}

static float score_move_node(eval_state *state, board_t b, float cprob) {
  float best = 0.0f;
  float res = 0.0f;
  enum direction dir = UP;
  state->curdepth++;
  for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
    board_t newboard = execute_move(dir, b);
    state->moves_evaled++;

    if (b != newboard) {
      res = score_tilechoose_node(state, newboard, cprob);
      best = MAX(best, res);
    }
  }
  state->curdepth--;

  return best;
}

static float _score_toplevel_move(eval_state *state, board_t b,
  enum direction dir) {
  board_t newboard = execute_move(dir, b);

  if (b == newboard)
    return 0;

  return score_tilechoose_node(state, newboard, 1.0f) + 1e-6;
}

//#define SHOW_STEP_ELAPSED_TIME
static eval_state state;
float score_toplevel_move(board_t b, enum direction dir) {
  float res = 0.0f;
#ifdef SHOW_STEP_ELAPSED_TIME
  struct timeval start, finish;
  double elapsed = 0.0;
#endif
  int distinct = count_distinct_tiles(b);
  state.depth_limit = MAX(MIN_SEARCH_DEPTH, distinct - 2);
  state.moves_evaled = 0;
  state.cachehits = 0;
  state.maxdepth = 0;
  state.curdepth = 0;
  state.caches = NULL;
#ifdef SHOW_STEP_ELAPSED_TIME
  gettimeofday(&start, NULL);
#endif
  res = _score_toplevel_move(&state, b, dir);
#ifdef SHOW_STEP_ELAPSED_TIME
  gettimeofday(&finish, NULL);
  elapsed = (finish.tv_sec - start.tv_sec);
  elapsed += (finish.tv_usec - start.tv_usec) / 1000000.0;
  LOG("Dir %d: result %f: eval'd %ld moves (%llu cache hits, %d cache size) in %.2f seconds (maxdepth=%llu)",
    dir, res, state.moves_evaled, state.cachehits, HASH_COUNT(state.caches),
    elapsed, state.maxdepth);
#endif
  cache_t *cache, *tmp;
  HASH_ITER(hh, state.caches, cache, tmp) {
    HASH_DEL(state.caches, cache);
    free(cache);
  }
  return res;
}

/* Find the best move for a given board. */
enum direction find_best_move(board_t b) {
  enum direction dir = UP;
  float best = 0;
  enum direction bestmove = BOTTOM_OF_DIRECTION;

  print_board(b);

  for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
    float res = score_toplevel_move(b, dir);

    if(res > best) {
      best = res;
      bestmove = dir;
    }
  }

  return bestmove;
}

/* Playing the game */
static inline board_t draw_tile() {
  return (rand() % 2) ? 1 : 2;
}

static inline board_t insert_tile_rand(board_t b, board_t tile) {
  int index = rand() % count_empty(b);
  board_t tmp = b;
  while (true) {
    while ((tmp & 0xf) != 0) {
      tmp >>= 4;
      tile <<= 4;
    }
    if (index == 0) break;
    --index;
    tmp >>= 4;
    tile <<= 4;
  }
  return b | tile;
}

static board_t initial_board() {
  board_t b = 0;
  b = insert_tile_rand(b, 1);
  b = insert_tile_rand(b, 1);
  return insert_tile_rand(b, 1);
}

void play_game(void)
{
  board_t b = initial_board();
  int moveno = 0;
  int scorepenalty = 0; // "penalty" for obtaining free 4 tiles

  while(true) {
    enum direction dir;
    board_t newboard;

    for (dir = UP; dir < BOTTOM_OF_DIRECTION; dir++) {
      if (execute_move(dir, b) != b)
        break;
    }
    if (dir == BOTTOM_OF_DIRECTION)
      break; // no legal moves

    dir = find_best_move(b);
    if (dir == BOTTOM_OF_DIRECTION)
      break;
    fprintf(stdout, "%c\n", "UDLR"[dir]);
    newboard = execute_move(dir, b);
    if (newboard == b) {
      LOG("Illegal move!\n");
      moveno--;
      continue;
    }
    LOG("Move #%d, current score=%.0f", ++moveno, score_board(b) - scorepenalty);
    board_t tile = draw_tile();
    if (tile == 2) scorepenalty += 4;
    b = insert_tile_rand(newboard, tile);
  }
  print_board(b);
}

int main()
{
	struct timeval begin, end;
  srand((uint32)time(NULL));
	gettimeofday(&begin, NULL);
	show_time(&begin);
  fprintf(stdout, "I\n");
  init_tables();
  play_game();
  fprintf(stdout, "E\n");
	gettimeofday(&end, NULL);
	show_time(&end);
	show_elapsed(&begin, &end);

	return (0);
}
