// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef THREAD_POOL
#define THREAD_POOL

#include <pthread.h>

#include "point.h"
#include "color.h"
#include "ray.h"
#include "node.h"


typedef struct {
  int work_queue;
  pthread_mutex_t work_queue_mutex;

  int no_of_threads;

  pthread_t * thread_ids;

  pthread_mutex_t * work_mutexes;
  pthread_mutex_t * finished_mutexes;

  pthread_cond_t * work_conds;
  pthread_cond_t * finished_conds;

  int * work;
  int * finished;
} Thread_pool;

typedef struct {
  Thread_pool * tp;
  int id;
} Thread_args;

typedef struct {
  int start;
  int stop;
  int first;
} Work_item;

Thread_pool * new_thread_pool(int no_of_threads);
Thread_args ** new_thread_args(int no_of_threads);
void free_thread_pool(Thread_pool * tp, int no_of_threads);
void free_thread_args(Thread_args ** tas, int no_of_threads);
void free_thread_arg(Thread_args * ta);
void create_threads(Thread_pool * tp, Thread_args ** tas);
void * do_work(void * args);
void thread_pool_exec(Thread_pool * tp, int work_id, int work_amount);
void thread_pool_stop(Thread_pool* tp);
Work_item get_work(Thread_pool * tp, int first);

#endif
