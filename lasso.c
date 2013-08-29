// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>

#include "lasso.h"
#include "quad.h"
#include "settings.h"

void init_lasso(Lasso* l){
  l->active = 0;
}

void render_lasso(Lasso l){
  l.quad.x = ((double)l.start_x)/RESOLUTION;
  l.quad.y = 1.0 - ((double)l.start_y)/RESOLUTION;
  l.quad.w = ((double)(l.end_x - l.start_x))/RESOLUTION;
  l.quad.h = -((double)(l.end_y - l.start_y))/RESOLUTION;
  Color color = {0.3,1.0,1.0,1.0};
  l.quad.c = color;

  render(&l.quad);
}
