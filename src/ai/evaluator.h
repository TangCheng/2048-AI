#ifndef __EVALUATOR_H__
#define __EVALUATOR_H__

#include "constants.h"
#include "../models/board.h"

typedef struct _evaluator evaluator;

bool evaluator_create(evaluator **self);
void evaluator_destory(evaluator **self);
void evaluator_set_smoothness_weight(evaluator *self, float weight);
void evaluator_set_monotonicity_weight(evaluator *self, float weight);
void evaluator_set_empty_weight(evaluator *self, float weight);
void evaluator_set_max_value_weight(evaluator *self, float weight);
float evaluator_get_value(evaluator *self, board *b);
uint32 evaluator_islands(evaluator *self, board *b);
int32 evaluator_smoothness(evaluator *self, board *b);
int32 evaluator_monotonicity(evaluator *self, board *b);
uint32 evaluator_empty(evaluator *self, board *b);
uint32 evaluator_max_value(evaluator *self, board *b);
uint32 evaluator_sum(evaluator *self, board *b);

#endif /* __EVALUATOR_H__ */
