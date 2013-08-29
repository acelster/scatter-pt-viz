// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "point.h"

void normalize_Coord(Coord* c){
  real_t length = sqrt(c->x*c->x + c->y*c->y + c->z*c->z);
  c->x /= length;
  c->y /= length;
  c->z /= length;
}

Coord sub_Coord(Coord a, Coord b){
  Coord c;
  c.x = a.x - b.x;
  c.y = a.y - b.y;
  c.z = a.z - b.z;

  return c;
}

Coord div_Coord(Coord a, Coord b, real_t safe){
  Coord c;
  c.x = (b.x == 0) ? safe/b.x :a.x/b.x;
  c.y = (b.y == 0) ? safe/b.y :a.y/b.y;
  c.z = (b.z == 0) ? safe/b.z :a.z/b.z;

  return c;
}

Coord min_Coord(Coord a, Coord b){
  Coord c;
  c.x = fmin(a.x, b.x);
  c.y = fmin(a.y, b.y);
  c.z = fmin(a.z, b.z);

  return c;
}

Coord max_Coord(Coord a, Coord b){
  Coord c;
  c.x = fmax(a.x, b.x);
  c.y = fmax(a.y, b.y);
  c.z = fmax(a.z, b.z);

  return c;
}

Coord add_scaled_Coord(Coord a, Coord b, real_t c){
  Coord d;
  d.x = a.x + b.x*c;
  d.y = a.y + b.y*c;
  d.z = a.z + b.z*c;

  return d;
}

int inside(Coord pos, Ranges* r){
  int i = 1;
  if(pos.x <= r->xmin || pos.x >= r->xmax){
    i = 0;
  }
  if(pos.y <= r->ymin || pos.y >= r->ymax){
    i = 0;
  }
  if(pos.z <= r->zmin || pos.z >= r->zmax){
    i = 0;
  }
  return i;
}

real_t angle_between_Coords(Coord a, Coord b){
  real_t abs_a = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
  real_t abs_b = sqrt(b.x*b.x + b.y*b.y + b.z*b.z);

  real_t dot_p = a.x*b.x + a.y*b.y + a.z*b.z;


  real_t temp = dot_p/(abs_a * abs_b);
  temp = (temp > 1.0) ? 1.0 : temp;
  real_t angle = acos(temp);

  return angle;
}

real_t length_Coord(Coord c){
  return sqrt(c.x*c.x + c.y*c.y + c.z*c.z);
}

Coord cross_product(Coord a, Coord b){
  Coord r;
  r.x = a.y*b.z - a.z*b.y;
  r.y = a.z*b.x - a.x*b.z;
  r.z = a.x*b.y - a.y*b.x;

  return r;
}

real_t distance_Coord(Coord a, Coord b){
  real_t dx = a.x - b.x;
  real_t dy = a.y - b.y;
  real_t dz = a.z - b.z;
  return sqrt(dx*dx + dy*dy + dz*dz);
}

void print_point(Point p){
  printf("x: %f\n", p.x);
  printf("y: %f\n", p.y);
  printf("z: %f\n", p.z);
  printf("intensity: %f\n", p.intensity);
  //printf("grid_index: %d\n", p.grid_index);
}

void print_coord(Coord p){
  printf("x: %f\n", p.x);
  printf("y: %f\n", p.y);
  printf("z: %f\n", p.z);
}

void print_ranges(Ranges r){
  printf("xmin: %f\n", r.xmin);
  printf("xmax: %f\n", r.xmax);
  printf("ymin: %f\n", r.ymin);
  printf("ymax: %f\n", r.ymax);
  printf("zmin: %f\n", r.zmin);
  printf("zmax: %f\n", r.zmax);
  printf("imin: %f\n", r.imin);
  printf("imax: %f\n", r.imax);
}

void print_histogram(PointList* pl, Ranges* r, int no_buckets){
  int* buckets = (int*)calloc(no_buckets, sizeof(int));

  for(int c = 0; c < pl->index; c++){
    int i = floor(no_buckets * (pl->points[c].intensity - r->imin)/(r->imax - r->imin));
    if(i == no_buckets){
      i--;
    }
    buckets[i]++;
  }

  real_t cum_pct = 0;
  for(int c = 0; c < no_buckets; c++){
    real_t pct = (real_t)buckets[c]/(real_t)pl->index;
    cum_pct += pct;
    printf("%d\t\t%10.2f - %.2f\t\t%d\t%0.4f\t%0.4f\n",
        c,
        ((r->imax -r->imin)/no_buckets) * c + r->imin,
        ((r->imax -r->imin)/no_buckets) * (c + 1) + r->imin,
        buckets[c],
        pct,
        cum_pct);
    if(cum_pct >= 0.999){
      break;
    }
  }
}
