// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


//#include <sys/time.h>
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "timer.h"

//static struct timeval start;
//static struct timeval end;
static struct timespec start;
static struct timespec end;

void timer_start(char* message){
  printf("%s", message);
  //gettimeofday(&start, NULL);
  clock_gettime(CLOCK_MONOTONIC, &start);
}

double timer_end(){
  //gettimeofday(&end, NULL);
  clock_gettime(CLOCK_MONOTONIC, &end);
  long nsec = (end.tv_sec - start.tv_sec)*1e9 + (end.tv_nsec - start.tv_nsec);
  double sec = (double)nsec/1e9;

  //long sec = (long)(end.tv_sec - start.tv_sec);
  //unsigned long usec = (unsigned long)(end.tv_nsec - start.tv_nsec);
  printf("DONE in %f s\n\n", sec);
  return sec;
}
