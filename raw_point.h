// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef RAW_POINT
#define RAW_POINT

typedef struct{
  double xmin;
  double xmax;
  double ymin;
  double ymax;
  double zmin;
  double zmax;
  double imin;
  double imax;
} Raw_Ranges;

typedef struct{
  double x;
  double y;
  double z;
  double intensity;
} Raw_Point;

#endif
