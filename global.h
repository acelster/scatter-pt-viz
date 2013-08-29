// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef GLOBAL
#define GLOBAL

#include "transfer.h"
#include "node.h"
#include "ray.h"
#include "color.h"
#include "point.h"
#include "root.h"

extern Transfer_Overlay* transfer_overlay;
extern Root* root;
extern Display_color** images;
extern Ray* rays;

extern Tree* tree;

#ifdef COUNT
extern unsigned long raysTraced;
extern unsigned long zeroLengthRays;
extern unsigned long rangeSearches;
extern unsigned long preTracedRays;
extern unsigned long samplesAccessed;
extern unsigned long samplesFullyAccessed;
#endif

#endif
