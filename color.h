// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef COLOR
#define COLOR

#include "real.h"

typedef struct{
  real_t a;
  real_t b;
  real_t g;
  real_t r;
} Color;

typedef struct{
  unsigned char a;
  unsigned char b;
  unsigned char g;
  unsigned char r;
} Display_color;

void blend(Color* output, real_t intensity);
Display_color to_display_color(Color c);
void print_color(Color c);

#endif
