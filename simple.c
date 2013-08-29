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
#include "settings.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

Ranges* ranges;
Root* root;
Raycreator* rc;
Ray* rays;
Display_color** images;
Transfer_Overlay* transfer_overlay;

void load_data(){
  Loader l = init_loader(1e9);

  root = create_root(l.ranges);

  while(has_next(&l)){
    root->insert_points(root->s, next(&l));
  }

  root->finalize(root->s);
}

void setup(){
  rc = init_raycreator(&root->ranges);

  rays = (Ray*)malloc(sizeof(Ray)*RESOLUTION*RESOLUTION);

  images = (Display_color**)malloc(sizeof(Display_color*) * (int)(log2(RESOLUTION) - 3));
  int c = 0;
  for(int res = 16; res <= RESOLUTION; res *=2){
    images[c] = (Display_color*)malloc(sizeof(Display_color) * res * res);
    c++;
  }

  transfer_overlay = init_transfer_overlay(&root->ranges);
}


int main(int argc, char** argv){
  load_config_file(0);

  load_data();

  setup();

  create_rays(rc, rays);

  trace_rays(RESOLUTION, 0, RESOLUTION*RESOLUTION);
  printf("Done tracing rays!\n");

  int i = log2(RESOLUTION) - 4;
  write_bmp(images[i], RESOLUTION,RESOLUTION,NULL);
}

