// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef GRID
#define GRID

#include "point.h"
#include "grid.h"

typedef struct{
  int index;
  int length;
} Grid_cell;

typedef struct{
  Point* points;
  Grid_cell* indices;
  Ranges ranges;
  int x_size;
  int y_size;
  int z_size;
  int num_points;
} Grid;


Grid* init_grid(Ranges* r);
void insert_points_grid(void* g, PointList* pl);
void finalize_grid(void* v);
real_t get_intensity_for_pos_grid(void* v, Coord pos);
long get_memory_usage_grid(void* v);


void set_grid_index(Grid_point* p, Grid* g);
void print_grid(Grid* g);
int point_comparator(const void* p, const void* n);


#endif
