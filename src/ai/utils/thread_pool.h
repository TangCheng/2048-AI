#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "constants.h"

typedef struct _thread_pool thread_pool;
typedef void (*thread_proc)(void *arg);

bool thread_pool_create(thread_pool **self, uint8 thread_num);
void thread_pool_destory(thread_pool **self);
bool thread_pool_add_job(thread_pool *self, thread_proc func, void *arg);

#endif /* __THREAD_POOL_H__ */
