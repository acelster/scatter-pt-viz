// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include "grid.h"
#include "point.h"
#include "settings.h"
#include "idw.h"

Grid* init_grid(Ranges* r){
  Grid* g = (Grid*)malloc(sizeof(Grid));
  g->ranges = *r;

  return g;
}

void insert_points_grid(void* v, PointList* pl){
  Grid* g = (Grid*) v;
  g->num_points = pl->index;

  g->x_size = (int)((g->ranges.xmax - g->ranges.xmin)/(2*INTERPOLATION_RADIUS));
  g->y_size = (int)((g->ranges.ymax - g->ranges.ymin)/(2*INTERPOLATION_RADIUS));
  g->z_size = (int)((g->ranges.zmax - g->ranges.zmin)/(2*INTERPOLATION_RADIUS));

  long int grid_size = (long int)g->x_size * (long int)g->y_size * (long int)g->z_size;
  if(grid_size > INT_MAX){
    fprintf(stderr, "Grid is too big. Exiting.\n");
    exit(-1);
  }
  if(VERBOSE >= 1){
    printf("Grid size: %dx%dx%d, %ld bytes\n",g->x_size, g->y_size, g->z_size, sizeof(Grid_cell) * grid_size);
  }
  g->indices = (Grid_cell*)malloc(sizeof(Grid_cell) * grid_size); 


  for(int c= 0; c < grid_size; c++){
    g->indices[c].index = -1;
  }

  Grid_point* grid_points = (Grid_point*)malloc(sizeof(Grid_point) * pl->index);

  for(int c = 0; c < pl->index; c++){
    grid_points[c].x = pl->points[c].x;
    grid_points[c].y = pl->points[c].y;
    grid_points[c].z = pl->points[c].z;
    grid_points[c].intensity = pl->points[c].intensity;

    set_grid_index(&grid_points[c], g);
  }

  qsort(grid_points, pl->index, sizeof(Grid_point), point_comparator);

  int prev = -1;
  for(int c = 0; c < pl->index; c++){
    if(grid_points[c].grid_index != prev){
      g->indices[grid_points[c].grid_index].index = c;
      prev = grid_points[c].grid_index;
    }
  }

  for(int c = 0; c < grid_size; c++){
    if(g->indices[c].index != -1){
      
      int i = g->indices[c].index;

      while(grid_points[i].grid_index == c && i < pl->index){
        i++;
      }

      g->indices[c].length = i - g->indices[c].index;
    }
  }

  for(int c = 0; c < pl->index; c++){
    pl->points[c].x = grid_points[c].x;
    pl->points[c].y = grid_points[c].y;
    pl->points[c].z = grid_points[c].z;
    pl->points[c].intensity = grid_points[c].intensity;
  }

  g->points = pl->points;
}

int point_comparator(const void* p, const void* n){
  return ((Grid_point*)p)->grid_index - ((Grid_point*)n)->grid_index;
}

void finalize_grid(void* v){
  Grid* g = (Grid*)v;
  if(VERBOSE >= 1){
    printf("Total number of points: %d\n", g->num_points);
  }
}

real_t get_intensity_for_pos_grid(void *v, Coord pos){
  Grid* g = (Grid*)v;

  int x = g->x_size * ((pos.x - g->ranges.xmin)/(g->ranges.xmax - g->ranges.xmin));
  int y = g->y_size * ((pos.y - g->ranges.ymin)/(g->ranges.ymax - g->ranges.ymin));
  int z = g->z_size * ((pos.z - g->ranges.zmin)/(g->ranges.zmax - g->ranges.zmin));

  int sub_x = (2*g->x_size) * ((pos.x - g->ranges.xmin)/(g->ranges.xmax - g->ranges.xmin));
  int sub_y = (2*g->y_size) * ((pos.y - g->ranges.ymin)/(g->ranges.ymax - g->ranges.ymin));
  int sub_z = (2*g->z_size) * ((pos.z - g->ranges.zmin)/(g->ranges.zmax - g->ranges.zmin));

  sub_x = sub_x - (x*2);
  sub_y = sub_y - (y*2);
  sub_z = sub_z - (z*2);

  PointList pls[27];
  int i = 0;

  int ue,uc,ud,le,ld,lc;
  if(NEIGHBOUR_MODE == 1){
    if(sub_x == 1){
      ue = 2; le = 0;
    }
    else{
      ue = 1; le = -1;
    }

    if(sub_y == 1){
      ud = 2; ld = 0;
    }
    else{
      ud = 1; ld = -1;
    }

    if(sub_z == 1){
      uc = 2; lc = 0;
    }
    else{
      uc = 1; lc = -1;
    }
  }
  if(NEIGHBOUR_MODE == 0){
    uc = 1; lc = 0;
    ud = 1; ld = 0;
    ue = 1; le = 0;
  }

  for(int c = lc; c < uc; c++){
    for(int d = ld; d < ud; d++){
      for(int e = le; e < ue; e++){

        int index = (z+c)*g->x_size*g->y_size + (y+d)*g->x_size + (x+e);

        if(index < 0 || (x+e) >= g->x_size || (y+d) >= g->y_size || (z+c) >= g->z_size){
          continue;
        }
        if( g->indices[index].index == -1){
          continue;
        }

        pls[i].index = g->indices[index].length;
        pls[i].points = &g->points[g->indices[index].index];

        i++;
      }
    }
  }
  return i/1000.0;
  //return idw_interpolate(pls, i, pos);
}

void set_grid_index(Grid_point* p, Grid* g){
  int x = g->x_size * ((p->x - g->ranges.xmin)/(g->ranges.xmax - g->ranges.xmin));
  int y = g->y_size * ((p->y - g->ranges.ymin)/(g->ranges.ymax - g->ranges.ymin));
  int z = g->z_size * ((p->z - g->ranges.zmin)/(g->ranges.zmax - g->ranges.zmin));

  p->grid_index = z*g->x_size*g->y_size + y*g->x_size + x;
}

long get_memory_usage_grid(void * v){
  Grid* g = (Grid*)v;

  long grid = sizeof(Grid_cell)*g->x_size * g->y_size * g->z_size;
  long points = sizeof(Point)*g->num_points;

  if(VERBOSE >= 1){
    printf("sizeof grid: %ld, sizeof points: %ld\n", grid, points);
  }
  return grid + points;
}

void print_grid(Grid* g){

  for(int c= 0; c < g->num_points; c++){
    print_point(g->points[c]);
  }

  printf("\n\n\n");
  int grid_size = g->x_size * g->y_size * g->z_size;
  for(int c= 0; c < grid_size; c++){
    printf("%d %d\n", g->indices[c].index, g->indices[c].length);
  }
}

  
