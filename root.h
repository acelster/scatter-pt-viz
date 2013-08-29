// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef ROOT
#define ROOT

#include "point.h"
#include "real.h"

//#include "raycreator.h"
struct rcstruct;

typedef struct{
  void* s;
  Ranges ranges;

  void (*insert_points)(void* s, PointList* pl);
  void (*finalize)(void* s);
  real_t (*get_intensity_for_pos)(void* s, Coord pos);
  void (*launch_kernel)(void* s, struct rcstruct* rc);
  long int (*get_memory_useage)(void* s);
} Root;

Root* create_root(Ranges* ranges);

#endif
