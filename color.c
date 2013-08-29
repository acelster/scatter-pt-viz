// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>

#include "color.h"
#include "transfer.h"
#include "global.h"


Display_color to_display_color(Color c){
  Display_color dc;

  if(c.r > 1.0){
    c.r = 1.0;
  }

  if(c.b > 1.0){
    c.b = 1.0;
  }

  if(c.g > 1.0){
    c.g = 1.0;
  }
  
  dc.a = (unsigned char)(c.a*255);
  dc.r = (unsigned char)(c.r*255);
  dc.g = (unsigned char)(c.g*255);
  dc.b = (unsigned char)(c.b*255);

  return dc;
}


void blend(Color* output, real_t intensity){
  Color new_color;
  set_color_from_table(transfer_overlay, intensity/transfer_overlay->max_intensity, &new_color);

  output->r = output->r + (1-output->a)*new_color.a*new_color.r;
  output->g = output->g + (1-output->a)*new_color.a*new_color.g;
  output->b = output->b + (1-output->a)*new_color.a*new_color.b;

  output->a = output->a + (1-output->a)*new_color.a;
}

void print_color(Color c){
  printf("%f, %f, %f, %f\n", c.r, c.g, c.b, c.a);
}
