// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef RAY
#define RAY

#include "point.h"
#include "ray.h"
#include "node.h"
#include "color.h"
#include "transfer.h"
#include "root.h"
#include "real.h"

typedef struct{
  Coord start;
  Coord dir;
  real_t distance;
  Color color;
} Ray;

typedef struct{
  Ray* rays;
  Node* root;
  Ranges* ranges;
  int resolution;
  Display_color* image;
  int thread_id;
} Trace_ray_args;


void* trace_rays(int resolution, int start, int stop);
Display_color trace_ray(Ray* ray, Root* root, int print);
void normalize_ray(Ray* r);
void print_ray(Ray r);

#endif
