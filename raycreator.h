// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef RAYCREATOR
#define RAYCREATOR

#include "ray.h"
#include "point.h"
#include "lasso.h"
#include "real.h"

typedef struct rcstruct{
  int resolution;
  Ranges * ranges;
  real_t fov;

  Coord eye;
  Coord forward;
  Coord right;
  Coord up;
  
  Coord screen_center;
  real_t pixel_width;

} Raycreator;

Raycreator* init_raycreator(Ranges* ranges);
void update_raycreator(Raycreator* rc);
void create_rays(Raycreator* rc, Ray* rays);
void find_intersections(Ray* ray, Ranges* ranges);
void update_camera(Raycreator* rc, Lasso lasso);

#endif
