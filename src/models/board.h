#ifndef __BOARD_H__
#define __BOARD_H__

#include "constants.h"

typedef struct _board board;

bool board_create(board **self, uint32 rows, uint32 cols);
void board_destory(board **self);
uint32 board_get_rows(board *self);
uint32 board_get_cols(board *self);
void board_set_value(board *self, uint32 x, uint32 y, uint32 val);
void board_set_value_by_pos(board *self, uint64 pos, uint32 val);
uint32 board_get_value(board *self, uint32 x, uint32 y);
uint32 board_count_availables(board *self);
uint64 *board_get_availables(board *self);
bool board_clone_data(board *self, board *mother);
bool board_is_equal(board *self, board *other);

#endif /* __BOARD_H__ */
