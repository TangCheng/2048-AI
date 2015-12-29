#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <stdio.h>
#include "constants.h"
#include "models/board.h"

typedef struct _console_output cout;

bool cout_create(cout **self);
void cout_destory(cout **self);
void cout_display_text(cout *self, char *text);
void cout_display_direction(cout *self, enum direction d);
void cout_display_board(cout *self, board *b);
void cout_display_score(cout *self, uint32 score);

#endif /* __OUTPUT_H__ */
