// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "thread_pool.h"

Thread_pool * new_thread_pool(int no_of_threads){
  Thread_pool * tp = (Thread_pool*)malloc(sizeof(Thread_pool));

  tp->work_queue = 0;
  pthread_mutex_init(&tp->work_queue_mutex, NULL);

  tp->no_of_threads = no_of_threads;

  tp->thread_ids = (pthread_t*)malloc(sizeof(pthread_t) * no_of_threads);

  tp->work_mutexes = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * no_of_threads);
  tp->finished_mutexes = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * no_of_threads);

  tp->work_conds = (pthread_cond_t*)malloc(sizeof(pthread_cond_t) * no_of_threads);
  tp->finished_conds = (pthread_cond_t*)malloc(sizeof(pthread_cond_t) * no_of_threads);

  tp->work = (int*)malloc(sizeof(int) * no_of_threads);
  tp->finished = (int*)malloc(sizeof(int) * no_of_threads);

  for(int i = 0; i < no_of_threads; i++){
    pthread_mutex_init(&tp->work_mutexes[i], NULL);
    pthread_mutex_init(&tp->finished_mutexes[i], NULL);

    pthread_cond_init(&tp->work_conds[i], NULL);
    pthread_cond_init(&tp->finished_conds[i], NULL);

    tp->work[i] = 0;
    tp->finished[i] = 0; 
  }

  return tp;
}

Thread_args ** new_thread_args(int no_of_threads){
  Thread_args ** tas = (Thread_args**)malloc(sizeof(Thread_args*) * no_of_threads);
  
  for(int i = 0; i < no_of_threads; i++){
    tas[i] = (Thread_args*)malloc(sizeof(Thread_args));
  }
  return tas;
}

void create_threads(Thread_pool * tp, Thread_args ** tas){
  for(int i = 0; i < tp->no_of_threads; i++){
    tas[i]->id = i;
    tas[i]->tp = tp;

    pthread_create(&tp->thread_ids[i], NULL, do_work, (void*)tas[i]);
  }
}

void thread_pool_exec(Thread_pool * tp, int work_id, int work_amount){
  tp->work_queue = work_amount;

  for(int i = 0; i < tp->no_of_threads; i++){
    pthread_mutex_lock(&tp->work_mutexes[i]);
    tp->work[i] = work_id;
    pthread_cond_signal(&tp->work_conds[i]);
    int succ = pthread_mutex_unlock(&tp->work_mutexes[i]);
  }

  for(int i = 0; i < tp->no_of_threads; i++){
    pthread_mutex_lock(&tp->finished_mutexes[i]);
    while(tp->finished[i] == 0){
      pthread_cond_wait(&tp->finished_conds[i], &tp->finished_mutexes[i]);
    }
    tp->finished[i] = 0;
    pthread_mutex_unlock(&tp->finished_mutexes[i]);
  }
}

void thread_pool_stop(Thread_pool* tp){
  pthread_mutex_lock(&tp->work_queue_mutex);
  tp->work_queue = 0;
  pthread_mutex_unlock(&tp->work_queue_mutex);
}

Work_item get_work(Thread_pool * tp, int first){
  Work_item new_work_item;

  int size = 64;
  new_work_item.first = 0;
  if(first){
    size = 128;
    new_work_item.first = 1;
  }

  pthread_mutex_lock(&tp->work_queue_mutex);

  new_work_item.stop = tp->work_queue;
  tp->work_queue -= size;
  new_work_item.start = tp->work_queue;

  if(new_work_item.start < 0){
    new_work_item.start = 0;
    tp->work_queue = 0;
  }

  pthread_mutex_unlock(&tp->work_queue_mutex);

  return new_work_item;
}

void * do_work(void * args){
  Thread_args * ta = (Thread_args*)args;
  int id = ta->id;
  int local_work;

  while(1){
    pthread_mutex_lock(&ta->tp->work_mutexes[id]);

    while(ta->tp->work[id] == 0){
      pthread_cond_wait(&ta->tp->work_conds[id], &ta->tp->work_mutexes[id]);
    }
    local_work = ta->tp->work[id];
    pthread_mutex_unlock(&ta->tp->work_mutexes[id]);


    int first = 1;
    while(1){
      Work_item item = get_work(ta->tp, first);
      first = 0;

      if(item.start == 0 && item.stop == 0){
        break;
      }

      trace_rays(local_work, item.start, item.stop);
    }

    pthread_mutex_lock(&ta->tp->work_mutexes[id]);
    ta->tp->work[id] = 0;
    pthread_mutex_unlock(&ta->tp->work_mutexes[id]);

    pthread_mutex_lock(&ta->tp->finished_mutexes[id]);
    ta->tp->finished[id] = 1;
    pthread_cond_signal(&ta->tp->finished_conds[id]);
    pthread_mutex_unlock(&ta->tp->finished_mutexes[id]);
  }
}


void free_thread_pool(Thread_pool * tp, int no_of_threads){
  free(tp->thread_ids);

  pthread_mutex_destroy(&tp->work_queue_mutex);

  for(int i = 0; i < no_of_threads; i++){
    pthread_cond_destroy(&tp->work_conds[i]);
    pthread_cond_destroy(&tp->finished_conds[i]);
    pthread_mutex_destroy(&tp->work_mutexes[i]);
    pthread_mutex_destroy(&tp->finished_mutexes[i]);
  }

  free(tp->work_mutexes);
  free(tp->finished_mutexes);
  free(tp->work_conds);
  free(tp->finished_conds);
  free(tp->work);
  free(tp->finished);

  free(tp);
}

void free_thread_args(Thread_args ** tas, int no_of_threads){
  for(int i = 0; i < no_of_threads; i++){
    free_thread_arg(tas[i]);
  }

  free(tas);
}

void free_thread_arg(Thread_args * ta){
  free(ta);
}

