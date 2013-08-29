// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef WIREFRAME
#define WIREFRAME

#include "raycreator.h"
#include "point.h"
#include "color.h"


void draw_wireframe(Raycreator* rc, Ranges* r);

void get_screen_pos(Raycreator* rc, Coord origin, double* screen_pos);
double get_base(double limit);
void draw_line(Raycreator* rc, Coord start, Coord end, Color color);

#endif
