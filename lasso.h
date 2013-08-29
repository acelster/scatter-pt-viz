// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef LASSO
#define LASSO

#include "quad.h"

typedef struct{
  int start_x;
  int start_y;
  int end_x;
  int end_y;
  int active;
  Quad quad;
} Lasso;

void init_lasso(Lasso* l);
void render_lasso(Lasso l);

#endif
