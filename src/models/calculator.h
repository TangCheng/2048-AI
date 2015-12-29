#ifndef __CALCULATOR_H__
#define __CALCULATOR_H__

#include "constants.h"
#include "board.h"

typedef struct _calculator calculator;

bool calculator_create(calculator **self);
void calculator_destory(calculator **self);
bool calculator_check_direction(calculator *self, board *b, enum direction dir);
bool calculator_move(calculator *self, board *current, board *next, enum direction dir);
uint64 calculator_get_score(calculator *self);

#endif /* __CALCULATOR_H__ */
