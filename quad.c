// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <GL/glut.h>
#include <stdio.h>

#include "quad.h"
#include "color.h"

int inside_quad(Quad* q, real_t x, real_t y){
  int inside_x = x > q->x && x < (q->x + q->w);
  int inside_y = y > q->y && y < (q->y + q->h);

  return inside_x && inside_y;
}

void render(Quad* q){
  glBegin(GL_QUADS);
  glColor4f(q->c.r, q->c.g, q->c.b, q->c.a);
  glVertex3f(q->x, q->y, 0.0f);
  glVertex3f(q->x + q->w, q->y, 0.0f);
  glVertex3f(q->x + q->w, q->y + q->h, 0.0f);
  glVertex3f(q->x, q->y + q->h, 0.0f);
  glEnd();
}
