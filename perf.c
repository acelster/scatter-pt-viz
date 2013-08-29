// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include "loader.h"
#include "node.h"
#include "settings.h"
#include "point.h"
#include "global.h"
#include "raycreator.h"
#include "color.h"
#include "ray.h"
#include "transfer.h"
#include "bmp.h"
#include "root.h"
#include "grid.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

Ranges* ranges;
Root* root;
Raycreator* rc;
Ray* rays;
Display_color** images;
Transfer_Overlay* transfer_overlay;
Node_list* node_list;

Root* create_root(Ranges* ranges){
  Root* r = (Root*)malloc(sizeof(Root));
  r->ranges = *ranges;

  if(DATA_STRUCTURE == 0){
    r->s = create_tree(ranges);
    r->insert_points = insert_points;
    r->finalize = finalize;
    r->get_intensity_for_pos = get_intensity_for_pos_tree;
  }
  else if(DATA_STRUCTURE == 1){
    r->s = init_grid(ranges);
    r->insert_points = insert_points_grid;
    r->finalize = finalize_grid;
    r->get_intensity_for_pos = get_intensity_for_pos_grid;
  }
  return r;
}

void setup(){
  set_default_values();
  char n[100] = "/home/thomas/Master/extracted/27uc_jun09";
  strcpy(DEFAULT_FOLDER, n);
  FILEARG = 0;
  FILTERING_THRESHOLD = 20;
  INTERPOLATION_RADIUS = 0.005;
  RESOLUTION = 512;
  STEP_SIZE = 0.0005;
  BATCH = 0;
  ANISOTROPIC = 0;
  INTERPOLATION_MODE=2;
  NEIGHBOUR_MODE=1;
  DATA_STRUCTURE = 1;

  Loader l = init_loader(1e7);

  root = create_root(l.ranges);

  while(has_next(&l)){
    root->insert_points(root->s, next(&l));
  }
  printf("Finalizing\n");
  root->finalize(root->s);





  printf("Initializing raycreator\n");
  rc = init_raycreator(&root->ranges);

  rays = (Ray*)malloc(sizeof(Ray)*RESOLUTION*RESOLUTION);

  images = (Display_color**)malloc(sizeof(Display_color*) * 6);
  int c = 0;
  for(int res = 16; res <= RESOLUTION; res *=2){
    images[c] = (Display_color*)malloc(sizeof(Display_color) * res * res);
    c++;
  }

  transfer_overlay = (Transfer_Overlay*)malloc(sizeof(Transfer_Overlay));
  init_transfer_overlay(transfer_overlay);
  
}


int main(int argc, char** argv){

  setup();
  printf("Done setting up\n");

  double acc;
  for(int c = 0; c < 1; c++){
    rc->eye.x += (c*0.0001);
    printf("Creating rays\n");
    create_rays(rc, rays);
    printf("Tracing rays\n");
    trace_rays(512, 0, 512*512);
    
    acc += rays[12].dir.x;
    acc += images[5][129].a;
  }

  write_bmp(images[5], 512,512,NULL);
  printf("Done %f\n", acc);
}


