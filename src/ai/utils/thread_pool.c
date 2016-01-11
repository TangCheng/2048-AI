#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"
#include "list.h"

typedef struct _job
{
  thread_proc func;
  void        *arg;
} job;

typedef struct _thread_pool
{
  uint8           thread_num;
  list            *jobs;
  pthread_t       *pthreads;
  pthread_mutex_t mutex;
  pthread_cond_t  jobs_not_empty;
  bool            pool_close;
} thread_pool;

static void *thread_pool_func(void *arg);
static void thread_pool_release_jobs(thread_pool *self);

bool thread_pool_create(thread_pool **self, uint8 thread_num)
{
  bool ret = false;
  uint8 i = 0;

  *self = (thread_pool *)malloc(sizeof(thread_pool));
  if (NULL != *self) {
    (*self)->thread_num = thread_num;
    (*self)->pool_close = false;
    pthread_mutex_init(&(*self)->mutex, NULL);
    pthread_cond_init(&(*self)->jobs_not_empty, NULL);
    ret |= list_create(&(*self)->jobs);
    if (ret == true) {
      (*self)->pthreads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
      if (NULL != (*self)->pthreads) {
        pthread_attr_t attr;
        struct sched_param param;
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        param.sched_priority = 10;
        pthread_attr_setschedparam(&attr, &param);
        for (i = 0; i < thread_num; i++) {
          pthread_create(&(*self)->pthreads[i], &attr, thread_pool_func,
            (void *)(*self));
        }
        pthread_attr_destroy(&attr);
      }
    } else {
      thread_pool_destory(self);
    }
  }

  return ret;
}

void thread_pool_destory(thread_pool **self)
{
  uint8 i = 0;
  if (*self != NULL) {
    pthread_mutex_lock(&(*self)->mutex);
    (*self)->pool_close = true;
    pthread_mutex_unlock(&(*self)->mutex);
    pthread_cond_broadcast(&(*self)->jobs_not_empty);
    if ((*self)->pthreads != NULL) {
      for (i = 0; i < (*self)->thread_num; i++) {
        pthread_join((*self)->pthreads[i], NULL);
      }
    }
    pthread_mutex_destroy(&(*self)->mutex);
    pthread_cond_destroy(&(*self)->jobs_not_empty);
    free((*self)->pthreads);
    thread_pool_release_jobs(*self);
    free(*self);
    *self = NULL;
  }
}

bool thread_pool_add_job(thread_pool *self, thread_proc func, void *arg)
{
  bool ret = false;
  job *p = NULL;

  if (self != NULL && func != NULL) {
    pthread_mutex_lock(&self->mutex);
    p =(job*)malloc(sizeof(job));
    if (NULL != p) {
      p->func = func;
      p->arg = arg;
      if (list_is_empty(self->jobs)) {
        list_add_to_last(self->jobs, (void *)p);
        pthread_cond_broadcast(&self->jobs_not_empty);
      } else {
        list_add_to_last(self->jobs, (void *)p);
      }
    }
    pthread_mutex_unlock(&self->mutex);
  }

  return ret;
}

static void *thread_pool_func(void *arg)
{
  thread_pool *pool = (thread_pool *)arg;
  job *p = NULL;

  while (true) {
    pthread_mutex_lock(&pool->mutex);
    while (list_is_empty(pool->jobs) == true && pool->pool_close == false) {
      pthread_cond_wait(&pool->jobs_not_empty, &pool->mutex);
    }
    if (pool->pool_close == true) {
      fflush(stdout);
      pthread_mutex_unlock(&pool->mutex);
      pthread_exit(0);
    }
    p = (job *)list_get_from_first(pool->jobs);
    pthread_mutex_unlock(&pool->mutex);
    if (p != NULL && p->func != NULL) {
      (*(p->func))(p->arg);
    }
    free(p);
    p = NULL;
  }
}

static void thread_pool_release_jobs(thread_pool *self)
{
  job *p = NULL;
  while (list_is_empty(self->jobs) != true) {
    p = (job *)list_get_from_first(self->jobs);
    free(p);
    p = NULL;
  }
}
