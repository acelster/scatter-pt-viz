// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>

#include <pthread.h>

#include "ray.h"
#include "point.h"
#include "node.h"
#include "color.h"
#include "transfer.h"
#include "global.h"
#include "settings.h"
#include "kriging.h"
#include "idw.h"
#include "root.h"
#include "grid.h"



void* trace_rays(int resolution, int start, int stop){

  Display_color* image = images[(int)log2(resolution) - 4];

  int limit = resolution*resolution/64;
  int factor = RESOLUTION/resolution;
  for(int c = start; c < stop; c++){
    int row = c/resolution;
    int col = c%resolution;
    row *= factor;
    col *= factor;
    image[c] = trace_ray(&rays[row*RESOLUTION + col], root, 0);
  }
}

Display_color trace_ray(Ray* ray, Root* root, int print){
#ifdef COUNT
	raysTraced++;
#endif
  if(ray->distance <= 0){
    Display_color b = {0,0,0,0};
#ifdef COUNT
    zeroLengthRays++;
#endif
    return b;
  }

  if(ray->distance < -1){
    Display_color b = {255,0,0,255};
    return b;
  }

  if(ray->color.r > -1 && print == 0){
#ifdef COUNT
		preTracedRays++;
#endif
    return to_display_color(ray->color);
  }

  FILE* file;
  if(print){
    file = fopen("raydump.txt", "w+");
  }

  normalize_ray(ray);
  Color output = {0.0,0.0,0.0,0.0};

  Coord pos = ray->start;

  real_t acc_distance = 0;
  real_t local_step_size = STEP_SIZE;
  while(acc_distance < ray->distance){
        
    if(inside(pos, &root->ranges)){
      real_t intensity = root->get_intensity_for_pos(root->s, pos);
#ifdef COUNT
			rangeSearches++;
#endif

      if(intensity > 0 && local_step_size > STEP_SIZE){
        acc_distance -= local_step_size;
        pos = add_scaled_Coord(pos, ray->dir, -local_step_size);
        local_step_size = STEP_SIZE;
      }
      else if(intensity == 0 && (local_step_size * STEP_FACTOR) <= STEP_LIMIT){
        local_step_size *= STEP_FACTOR;
      }
      else{
        blend(&output, intensity);

        if(print){
          fprintf(file, "%f,%f,%f,%f,%f\n",intensity, output.r, output.g, output.b, output.a);
        }
        if(output.a > OPACITY_THRESHOLD){
          break;
        }
      }
    }

    pos = add_scaled_Coord(pos, ray->dir, local_step_size);
    acc_distance += local_step_size;
  }
  ray->color = output;

  if(print){
    fclose(file);
  }
  return to_display_color(output);
}

void normalize_ray(Ray* r){
  real_t length = sqrt(r->dir.x*r->dir.x + r->dir.y*r->dir.y + r->dir.z*r->dir.z);
  r->dir.x = r->dir.x/length;
  r->dir.y = r->dir.y/length;
  r->dir.z = r->dir.z/length;
}

void print_ray(Ray r){
  printf("Start: ");
  printf("%f,%f,%f\n", r.start.x, r.start.y, r.start.z);
  printf("Dir: ");
  printf("%f,%f,%f\n", r.dir.x, r.dir.y, r.dir.z);
  printf("Dist: ");
  printf("%f\n", r.distance);
}
