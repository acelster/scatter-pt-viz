// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef LOADER
#define LOADER

#include "point.h"
#include "real.h"
#include "raw_point.h"

typedef struct{
  Ranges* ranges;
  char* filename;
  int block_size;
  int total_size;
  int current_pos;
} Loader;

Loader init_loader(int block_size);
PointList* next(Loader* l);
int has_next(Loader* l);

Point* filter_and_convert(Raw_Point* raw_points, int num_points, int* n);
int loader_filter(Point p);

char* find_filename();
char** list_dir(char* dirname, int* num_files);
char* pathcat(char* dir, char* file);

#endif
