// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "filter.h"
#include "node.h"
#include "settings.h"

static Node* g_root;
static Ranges* g_ranges;

static int num_removed;
static int num_collapsed;

void filter(Node* root, Ranges* ranges){
  g_root = root;
  g_ranges = ranges;

  num_removed = 0;
  num_collapsed = 0;

  mark_for_removal(root);
  //collapse(root);

  if(VERBOSE){
    printf("\tRemoved %d points\n", num_removed);
    printf("\tCollapsed %d points\n", num_collapsed);
  }
}

void mark_for_removal(Node* node){
  if(node->is_leaf){
    //printf("Leaf, checking points\n");
    int num_neighbours = 0;
    PointList* neighbours = (PointList*)malloc(sizeof(PointList) * 8*512);
    for(int c = 0; c < node->num_children; c++){
      Coord pos;
      pos.x = ((Point**)node->pointer)[c]->x;
      pos.y = ((Point**)node->pointer)[c]->y;
      pos.z = ((Point**)node->pointer)[c]->z;

      Ranges search_ranges;
      search_ranges.xmin = pos.x -INTERPOLATION_RADIUS/2.0;
      search_ranges.ymin = pos.y -INTERPOLATION_RADIUS/2.0;
      search_ranges.zmin = pos.z -INTERPOLATION_RADIUS/2.0;
      search_ranges.xmax = pos.x + INTERPOLATION_RADIUS/2.0;
      search_ranges.ymax = pos.y + INTERPOLATION_RADIUS/2.0;
      search_ranges.zmax = pos.z + INTERPOLATION_RADIUS/2.0;

      range_search(g_root, &search_ranges, neighbours, &num_neighbours);

      int num_points = 0;
      for(int n = 0; n < num_neighbours; n++){
        num_points += neighbours[n].index;
      }

      Point** points = (Point**)malloc(sizeof(Point*)*num_points);

      int i = 0;
      for(int n = 0; n < num_neighbours; n++){
        for(int p = 0; p < neighbours[n].index; p++){
          points[i] = ((Point**)neighbours[n].points)[p];
          i++;
        }
      }

      int keep = 0;
      for(int all = 0; all < num_points; all++){
        double dx = points[all]->x - pos.x;
        double dy = points[all]->y - pos.y;
        double dz = points[all]->z - pos.z;
        if(sqrt(dx*dx + dy*dy + dz*dz) < INTERPOLATION_RADIUS/2.0){
          points[keep] = points[all];
          keep++;
        }
      }

      sort(points, keep);

      double median = fabs(points[keep/2]->intensity);
      double threshold = THRESHOLD_MEDIAN_FILTER*sqrt(median);

      if( fabs(((Point**)node->pointer)[c]->intensity - median) > threshold){
          ((Point**)node->pointer)[c]->intensity *= -1;
          num_removed++;
      }

      free(points);
    }
    free(neighbours);
  }
  else{
    for(int c = 0; c < 8; c++){
      mark_for_removal(&((Node*)node->pointer)[c]);
    }
  }
}

int comparator(const void* p, const void* n){
  Point** pp = (Point**)p;
  Point** pn = (Point**)n;

  if( fabs((*pp)->intensity) > fabs((*pn)->intensity)){
    return 1;
  }
  else if (fabs((*pp)->intensity) < fabs((*pn)->intensity)){
    return -1;
  }
  else{
    return 0;
  }
}

void sort(Point** p, int num_points){
  qsort(p, num_points, sizeof(Point*), comparator);
}


