#ifndef __INPUT_H__
#define __INPUT_H__

#include "constants.h"

typedef struct _input input;

bool input_create(input **self);
void input_destory(input **self);
enum direction input_get(input *self);

#endif /* __INPUT_H__ */
