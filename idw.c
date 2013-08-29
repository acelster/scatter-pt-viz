// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "idw.h"
#include "point.h"
#include "node.h"
#include "settings.h"
#include "global.h"

real_t idw_interpolate(PointList* pls, int num_pls, Coord pos){

  real_t intensity = 0;
  real_t weight = 0;


  real_t interpolation_radius = INTERPOLATION_RADIUS;
  if(INTERPOLATION_MODE == 2){
    interpolation_radius = INTERPOLATION_RADIUS * INTERPOLATION_RADIUS;
  }

  for(int c = 0; c < num_pls; c++){
    for(int p = 0; p < pls[c].index; p++){
      
      real_t dx = pos.x-((Point*)pls[c].points)[p].x;
      real_t dy = pos.y-((Point*)pls[c].points)[p].y;
      real_t dz = pos.z-((Point*)pls[c].points)[p].z;
#ifdef COUNT
			samplesAccessed++;
#endif

      real_t distance;

      if(ANISOTROPIC){
        if(ANISOTROPIC == 1){
          distance = ANISO_MATRIX[0]*dx*dx + ANISO_MATRIX[4]*dy*dy + ANISO_MATRIX[8]*dz*dz;
        }
        if(ANISOTROPIC == 2){
          distance = ANISO_MATRIX[0]*dx*dx + ANISO_MATRIX[1]*dy*dx + ANISO_MATRIX[2]*dz*dx +
            ANISO_MATRIX[3]*dx*dy + ANISO_MATRIX[4]*dy*dy + ANISO_MATRIX[5]*dz*dy +
            ANISO_MATRIX[6]*dx*dz + ANISO_MATRIX[7]*dy*dz + ANISO_MATRIX[8]*dz*dz;
        }
      }
      else{
        distance = dx*dx + dy*dy + dz*dz;
      }

      if(INTERPOLATION_MODE == 1){
        distance = sqrt(distance);
      }

      if(distance < interpolation_radius){
        if(MEDIAN_FILTERING){
          if(((Point*)pls[c].points)[p].intensity < 0){
            continue;
          }
        }
#ifdef COUNT
				samplesFullyAccessed++;
#endif
        intensity += (1/distance)* ((Point*)pls[c].points)[p].intensity;
        weight += 1/distance;
      }
    }
  }

  if(intensity <= 0){
    return 0;
  }

  real_t ratio = intensity/weight;
  if(ratio <= 1){
    return 0;
  }

  return log(ratio);
}

