#ifndef __GAME_H__
#define __GAME_H__

#include "constants.h"

typedef struct _game game;

bool game_create(game **self);
void game_destory(game **self);
void game_start(game *self);

#endif /* __GAME_H__ */
