// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef IDW
#define IDW

#include "point.h"
#include "node.h"
#include "real.h"

real_t idw_interpolate(PointList* pls, int num_pls, Coord pos);

#endif
