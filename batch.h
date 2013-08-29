// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef BATCH_HEADER
#define BATCH_HEADER

#include "point.h"
#include "raycreator.h"

typedef struct{
  Coord* positions;
  int* transitions;
  int num_positions;
  int current_pos;
  int current_pos_progress;
  int pic_number;
} Batch;

void init_batch(Batch* b);
int batch_has_next(Batch* b);
void batch_next(Batch* b, Raycreator* rc);

Coord rotate(Coord a, Coord b, double fraction);
void print_batch(Batch* b);

#endif
