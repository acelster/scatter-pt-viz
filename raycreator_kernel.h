// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef RAYCREATOR_KERNEL
#define RAYCREATOR_KERNEL

#include "real.h"

void launch_kernel(Ray* rays, Raycreator* rc, Coord screen_center, real_t pixel_width);

#endif
