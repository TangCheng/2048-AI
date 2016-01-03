#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include "log.h"

typedef char                  int8;
typedef short                 int16;
typedef int                   int32;
typedef long long             int64;
typedef unsigned char         uint8;
typedef unsigned short        uint16;
typedef unsigned int          uint32;
typedef unsigned long long    uint64;

/* The fundamental trick: the 4x4 board is represented as a 64-bit word,
 * with each board square packed into a single 4-bit nibble.
 *
 * The maximum possible board value that can be supported is 32768 (2^15), but
 * this is a minor limitation as achieving 65536 is highly unlikely under normal
 * circumstances.
 *
 * The space and computation savings from using this representation should be
 * significant.
 *
 * The nibble shift can be computed as (r,c) -> shift (4*r + c). That is, (0,0)
 * is the LSB.
 */
typedef uint64                board_t;
typedef uint16                row_t;

#ifndef bool
typedef enum boolean
{
  false   = 0,
  true    = 1
} bool;
#endif

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))

#define AUTO_PLAY
#define THINKING_BY_DEPTH
#define THINKING_DURATION     200     /* in million seconds */
#define MIN_SEARCH_DEPTH      3
#define MAX_SEARCH_DEPTH      15

#define AI_ENGINE                   minmax /* minmax or expectimax */
#define _AI_ENGINE_CREATE(name, a)  name##_create(a)
#define _AI_ENGINE_DESTORY(name, a) name##_destory(a)
#define _AI_ENGINE_SEARCH(name, a, board, dir, depth) \
  name##_search(a, board, dir, depth)
#define AI_ENGINE_CREATE(name, a)  _AI_ENGINE_CREATE(name, a)
#define AI_ENGINE_DESTORY(name, a) _AI_ENGINE_DESTORY(name, a)
#define AI_ENGINE_SEARCH(name, a, board, dir, depth) \
  _AI_ENGINE_SEARCH(name, a, board, dir, depth)

#define ROWS_OF_BOARD     4
#define COLS_OF_BOARD     4

#define ROW_MASK          0xFFFFULL
#define COL_MASK          0x000F000F000F000FULL

#define ELEMENT_BITS      (sizeof(board_t) * 8 / (ROWS_OF_BOARD * COLS_OF_BOARD))
#define TRANSPOSE_TABLE_SIZE    (1L << (ELEMENT_BITS * COLS_OF_BOARD))
#define SCORE_TABLE_SIZE        TRANSPOSE_TABLE_SIZE

#define GAME_INIT_NUMBER_COUNT  3
#define GAME_INIT_NUMBER        2
#define GAME_NUBMER_ELEMENTS    {2, 4}

enum direction
{
  UP                    = 0,
  DOWN                  = 1,
  LEFT                  = 2,
  RIGHT                 = 3,
  BOTTOM_OF_DIRECTION
};

enum round
{
  PLAYER_TURN           = 0,
  COMPUTER_TURN         = 1,
  BOTTOM_OF_ROUND
};

#endif /* __CONSTANTS_H__ */
