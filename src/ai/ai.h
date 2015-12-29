#ifndef __AI_H__
#define __AI_H__

#include "constants.h"
#include "../models/board.h"

typedef struct _ai ai;

bool ai_create(ai **self);
void ai_destory(ai **self);
void ai_set_thinking_duration(ai *self, uint32 duration);
enum direction ai_get(ai *self, board *b);

#endif /* __AI_H__ */
