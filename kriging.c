// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "point.h"
#include "kriging.h"
#include "settings.h"

real_t krige(PointList* neighbours, int num_neighbours, Coord pos){

  //Find the points within radius 
  int num_points = 0;
  //Point** points = (Point**)malloc(sizeof(Point*) * 2*4096);
  Point* points[1024];

  for(int c = 0; c < num_neighbours; c++){
    for(int p = 0; p < neighbours[c].index; p++){
      real_t dx = pos.x-((Point*)neighbours[c].points)[p].x;
      real_t dy = pos.y-((Point*)neighbours[c].points)[p].y;
      real_t dz = pos.z-((Point*)neighbours[c].points)[p].z;

      real_t distance = sqrt(dx*dx + dy*dy + dz*dz);

      if(distance < INTERPOLATION_RADIUS){
        points[num_points] = &((Point*)neighbours[c].points)[p];
        num_points++;
        if(num_points >= 1024){
          printf("oops... \n");
          exit(0);
        }
      }
    }
  }


  if(num_points == 0){
    return 0;
  }

  //Fill V matrix
  real_t* V = (real_t*)malloc(sizeof(real_t) * (num_points + 1) * (num_points + 1));

  for(int c = 0; c < num_points; c++){
    for(int d = 0; d < num_points; d++){
      V[c*(num_points + 1) + d] = variogram(points[c], points[d]);
    }
  }

  for(int c = 0; c < num_points + 1; c++){
    V[c*(num_points+1) + num_points] = 1;
    V[num_points*(num_points+1) + c] = 1;
  }
  V[(num_points+1) * (num_points + 1) -1] = 0;

  //Fill v
  real_t v[num_points + 1];

  Point p = {pos.x, pos.y, pos.z, 0};
  for(int c = 0; c < num_points; c++){
    v[c] = variogram(&p, points[c]);
  }
  v[num_points] = 1;

  //LAPACK stuff
  int NRHS = 1;
  int N = num_points + 1;
  int IPIV[N];
  int INFO;

  //Solve equation
#ifdef USE_FLOAT
  sgesv_(&N, &NRHS, V, &N, IPIV, v, &N, &INFO);
#else
  dgesv_(&N, &NRHS, V, &N, IPIV, v, &N, &INFO);
#endif


  if(INFO != 0){
    printf("LAPACK error: %d\n", INFO);
    exit(0);
  }

  //printf("Combining\n");
  real_t output = 0;
  for(int c = 0; c < num_points; c++){
    output += points[c]->intensity * v[c];
  }
  free(V);
  //free(points);

  return (output <= 0) ? 0 : log(output);
}

real_t variogram(Point* a, Point* b){
  real_t d = anisotropic_distance(a, b);

  real_t r = (1 - exp(-(d/VARIOGRAM_RADIUS))); //Magic number
  //printf("%f, %e\n", d,r);
  return r;
}

real_t anisotropic_distance(Point* a, Point* b){
  real_t dx = a->x - b->x;
  real_t dy = a->y - b->y;
  real_t dz = a->z - b->z;
  if(ANISOTROPIC == 0){
    return sqrt(dx*dx + dy*dy + dz*dz);
  }
  else if(ANISOTROPIC == 1){
    return sqrt( ANISO_MATRIX[0]*dx*dx + ANISO_MATRIX[4]*dy*dy + ANISO_MATRIX[8]*dz*dz);
  }
  else if(ANISOTROPIC == 2){
    return sqrt(ANISO_MATRIX[0]*dx*dx + ANISO_MATRIX[1]*dy*dx + ANISO_MATRIX[2]*dz*dx +
        ANISO_MATRIX[3]*dx*dy + ANISO_MATRIX[4]*dy*dy + ANISO_MATRIX[5]*dz*dy +
        ANISO_MATRIX[6]*dx*dz + ANISO_MATRIX[7]*dy*dz + ANISO_MATRIX[8]*dz*dz);
  }
}


