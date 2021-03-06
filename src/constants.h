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

#define ROWS_OF_BOARD    4
#define COLS_OF_BOARD    4

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
