// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef POINT
#define POINT

#include "real.h"

typedef struct{
  real_t x;
  real_t y;
  real_t z;
  real_t intensity;
} Point;

typedef struct{
  real_t x;
  real_t y;
  real_t z;
  real_t intensity;
  int grid_index;
} Grid_point;

typedef struct{
  Point* points;
  int index;
} PointList;

typedef struct{
  real_t xmin;
  real_t xmax;
  real_t ymin;
  real_t ymax;
  real_t zmin;
  real_t zmax;
  real_t imin;
  real_t imax;
} Ranges;

typedef struct{
  real_t x;
  real_t y;
  real_t z;
} Coord;

void print_point(Point p);
void print_ranges(Ranges r);
void print_coord(Coord c);
void normalize_Coord(Coord* c);
Coord sub_Coord(Coord a, Coord b);
Coord div_Coord(Coord a, Coord b, real_t safe);
Coord min_Coord(Coord a, Coord b);
Coord max_Coord(Coord a, Coord b);
Coord add_scaled_Coord(Coord a, Coord b, real_t c);
int inside(Coord pos, Ranges* r);
real_t angle_between_Coords(Coord a, Coord b);
real_t length_Coord(Coord c);
Coord cross_product(Coord a, Coord b);
real_t distance_Coord(Coord a, Coord b);
void print_histogram(PointList* pl, Ranges* r, int no_buckets);

#endif
