// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include "root.h"
#include "settings.h"
#include "node.h"
#include "grid.h"
#include "point.h"
#include "raycreator.h"
#include "raytrace_kernel.h"
#include "raytrace_kernel_grid.h"

#include <stdlib.h>

Root* create_root(Ranges* ranges){
  Root* r = (Root*)malloc(sizeof(Root));
  r->ranges = *ranges;

  if(DATA_STRUCTURE == 0){
    r->s = create_tree(ranges);
    r->insert_points = insert_points;
    r->finalize = finalize;
    r->get_intensity_for_pos = get_intensity_for_pos_tree;
    r->get_memory_useage = get_memory_useage_tree;
  }
  else if(DATA_STRUCTURE == 1){
    r->s = init_grid(ranges);
    r->insert_points = insert_points_grid;
    r->finalize = finalize_grid;
    r->get_intensity_for_pos = get_intensity_for_pos_grid;
    r->get_memory_useage = get_memory_usage_grid;
  }

#ifdef USE_CUDA
  if(DATA_STRUCTURE == 0){
    r->launch_kernel = launch_ray_trace_kernel;
  }
  else if(DATA_STRUCTURE == 1){
    r->launch_kernel = launch_ray_trace_kernel_grid;
  }
#endif

  return r;
}
