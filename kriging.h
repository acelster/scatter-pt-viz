// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef KRIGING
#define KRIGING

#include "point.h"
#include "node.h"
#include "real.h"

//LAPACK double
extern void dgetrf_(int*,int*,double*,int*,int*,int*);
extern void dgetri_(int*,double*,int*,int*,double*,int*,int*);
extern void dgesv_(int*,int*,double*,int*,int*,double*,int*,int*);

//LAPACK float
extern void sgetrf_(int*,int*,float*,int*,int*,int*);
extern void sgetri_(int*,float*,int*,int*,float*,int*,int*);
extern void sgesv_(int*,int*,float*,int*,int*,float*,int*,int*);

real_t krige(PointList* neighbours, int num_neighbours, Coord pos);
real_t variogram(Point* a, Point* b);
real_t anisotropic_distance(Point* a, Point* b);

#endif
