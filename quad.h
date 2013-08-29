// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef QUAD
#define QUAD

#include "color.h"
#include "real.h"

typedef struct{
  real_t x;
  real_t y;
  real_t w;
  real_t h;
  Color c;
} Quad;

int inside_quad(Quad* q, real_t x, real_t y);
void render(Quad* q);



#endif
