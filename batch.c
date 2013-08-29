// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "batch.h"
#include "point.h"
#include "raycreator.h"

#ifdef USE_FLOAT
char coord_format_batch[] = "%f %f %f %f %f %f";
#else
char coord_format_batch[] = "%lf %lf %lf %lf %lf %lf";
#endif

void init_batch(Batch* b){

  b->num_positions = -1;
  b->current_pos = 0;
  b->current_pos_progress = 0;
  b->pic_number = 0;

  FILE* batch_file = fopen("batch.txt", "r");
  if(batch_file == NULL){
    fprintf(stderr, "ERROR: Did not find batch file. Exiting.\n");
    exit(0);
  }

  char line[100];
  int line_no = 0;
  while(fgets(line, 100, batch_file) != NULL){
    if(strlen(line) < 2 || line[0] == '#'){
      continue;
    }
    else{
      if(b->num_positions == -1){
        sscanf(line, "%d", &b->num_positions);
        b->positions = (Coord*)malloc(sizeof(Coord) * 2 * b->num_positions);
        b->transitions = (int*)malloc(sizeof(int) * (b->num_positions - 1));
      }
      else{
        if(line_no % 2 == 0){
          //Position
          int index = line_no;
          sscanf(line, coord_format_batch,
              &b->positions[index].x,
              &b->positions[index].y,
              &b->positions[index].z,
              &b->positions[index + 1].x,
              &b->positions[index + 1].y,
              &b->positions[index + 1].z);

        }
        else{
          //Transition
          int index = line_no/2;
          sscanf(line, "%d", &b->transitions[index]);
        }
        line_no++;
      }
    }
  }
}

int batch_has_next(Batch* b){
  if(b->current_pos < b->num_positions){
    return 1;
  }
  else{
    return 0;
  }
}

void batch_next(Batch* b, Raycreator* rc){

  b->pic_number++;

  if(b->current_pos == b->num_positions -1){
    rc->eye = b->positions[b->current_pos * 2];
    rc->forward = b->positions[b->current_pos* 2 + 1];
    b->current_pos++;
  }
  else{
    Coord pos;
    Coord dir;

    double aaa = (double)(b->current_pos_progress);
    double bbb = (double)(b->transitions[b->current_pos]);
    double weight = aaa / bbb;

    pos.x = (1-weight)*b->positions[b->current_pos*2].x + weight*b->positions[(b->current_pos + 1)*2].x;
    pos.y = (1-weight)*b->positions[b->current_pos*2].y + weight*b->positions[(b->current_pos + 1)*2].y;
    pos.z = (1-weight)*b->positions[b->current_pos*2].z + weight*b->positions[(b->current_pos + 1)*2].z;

    dir = rotate(b->positions[b->current_pos*2 + 1],b->positions[(b->current_pos+1)*2 + 1], weight);

    rc->eye = pos;
    rc->forward = dir;
    print_coord(dir);

    b->current_pos_progress++;
    if(b->current_pos_progress == b->transitions[b->current_pos]){
      b->current_pos_progress = 0;
      b->current_pos++;
    }
  }
}

Coord rotate(Coord v1, Coord v2, double fraction){
  double angle = angle_between_Coords(v1,v2);
  angle = angle*fraction;

  Coord axis = cross_product(v1,v2);
  if(length_Coord(axis) == 0){
    fprintf(stderr, "ERROR: Plane undefined. Exiting\n");
    exit(-1);
  }
  normalize_Coord(&axis);

  Coord r;
  double u = axis.x; double v = axis.y; double w = axis.z;
  double x = v1.x; double y = v1.y; double z = v1.z;
  r.x = u*(u*x+v*y+w*z)*(1-cos(angle)) + x*cos(angle) + (-w*y+v*z)*sin(angle); 
  r.y = v*(u*x+v*y+w*z)*(1-cos(angle)) + y*cos(angle) + (w*x-u*z)*sin(angle); 
  r.z = w*(u*x+v*y+w*z)*(1-cos(angle)) + z*cos(angle) + (-v*x+u*y)*sin(angle); 

  return r;
}

void print_batch(Batch* b){
  for(int c = 0; c < b->num_positions; c++){
    print_coord(b->positions[2*c]);
    print_coord(b->positions[2*c+1]);
    printf("\n");
  }

  for(int c = 0; c < b->num_positions -1; c++){
    printf("%d\n", b->transitions[c]);
  }
}
