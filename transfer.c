// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include <string.h>

#include "transfer.h"
#include "color.h"
#include "quad.h"
#include "settings.h"

#ifdef USE_FLOAT
#define real_format_color "%f"
#else
#define real_format_color "%lf"
#endif

Transfer_Overlay* init_transfer_overlay(Ranges* r){
  Transfer_Overlay* t = (Transfer_Overlay*)malloc(sizeof(Transfer_Overlay));
  t->color_selected = 0;
  t->visible = 0;
  t->max_intensity = log(r->imax);
  MAX_INTENSITY = t->max_intensity;
  printf("Max intensity: %f\n", t->max_intensity);

  t->transfers = (Transfer*)malloc(sizeof(Transfer)*4);
  init_transfer(&t->transfers[0], 0);
  init_transfer(&t->transfers[1], 1);
  init_transfer(&t->transfers[2], 2);
  init_transfer(&t->transfers[3], 3);

  t->quads = (Quad*)malloc(sizeof(Quad)*6);

  //For convenience...
  Quad* quads = t->quads;

  Color c = {1.0,1.0,1.0,1.0};
  quads[0].x = 0; quads[0].y = 0; quads[0].w = 1; quads[0].h = 0.4;
  c.a = 0.5;
  quads[0].c = c;

  quads[1].x = 0.05; quads[1].y = 0.01; quads[1].w = 0.08; quads[1].h = 0.08;
  c.a = 1.0; c.g = 0; c.b = 0;
  quads[1].c = c;

  quads[2].x = 0.05; quads[2].y = 0.11; quads[2].w = 0.08; quads[2].h = 0.08;
  c.g = 1.0; c.r = 0;
  quads[2].c = c;

  quads[3].x = 0.05; quads[3].y = 0.21; quads[3].w = 0.08; quads[3].h = 0.08;
  c.g = 0; c.b = 1.0;
  quads[3].c = c;

  quads[4].x = 0.05; quads[4].y = 0.31; quads[4].w = 0.08; quads[4].h = 0.08;
  c.r = 0.6; c.g = 0.6; c.b = 0.6;
  quads[4].c = c;

  quads[5].x = 0.2; quads[5].y = 0.05; quads[5].w = 0.77; quads[5].h = 0.32;
  c.r = 1.0; c.g = 1.0; c.b = 1.0;
  quads[5].c = c;

  read_colors(t);

  t->color_table_size = 10000;
  t->color_table = (Color*)malloc(sizeof(Color)* t->color_table_size);
  update_color_table(t);

  return t;
}

void render_transfer_overlay(Transfer_Overlay* transfer_overlay){
  if(!transfer_overlay->visible){
    return;
  }

  for(int c = 0; c < 6; c++){
    render(&transfer_overlay->quads[c]);
  }

  int cs = transfer_overlay->color_selected;
  render_transfer(&transfer_overlay->transfers[cs], cs, 0.2, 0.05, 0.75, 0.3);
}

void handle_mouse(Transfer_Overlay* t, real_t x, real_t y, int state){
  if(!t->visible){
    return;
  }

  if(state == GLUT_DOWN){
    for(int c = 1; c < 5; c++){
      if(inside_quad(&t->quads[c], x, y)){
        t->color_selected = c -1;
        return;
      }
    }
  }

  if(state == GLUT_DOWN){
    start_drag(&t->transfers[t->color_selected],x,y);
  }
  else{
    stop_drag(&t->transfers[t->color_selected],x,y);
    update_color_table(t);
  }
}

void set_color(Transfer_Overlay* t, real_t iso, Color* c){
  real_t step_size = 1.0/(COLOR_BANDS - 1.0);
  int low_index = (int)(iso/step_size);
  int high_index = low_index + 1;

  real_t p = (iso - low_index*step_size)/step_size;
  
  real_t low_val = (t->transfers[3].values[low_index]);
  real_t high_val = (t->transfers[3].values[high_index]);
  c->a = low_val + p*(high_val - low_val);

  low_val = (t->transfers[0].values[low_index]);
  high_val = (t->transfers[0].values[high_index]);
  c->r = low_val + p*(high_val - low_val);

  low_val = (t->transfers[1].values[low_index]);
  high_val = (t->transfers[1].values[high_index]);
  c->g = low_val + p*(high_val - low_val);

  low_val = (t->transfers[2].values[low_index]);
  high_val = (t->transfers[2].values[high_index]);
  c->b = low_val + p*(high_val - low_val);
}

void set_color_from_table(Transfer_Overlay* t, real_t iso, Color* c){
  if(iso > 1.0){
    printf("Too big values...\n");
  }
  int index = (int)(iso * t->color_table_size);
  *c = t->color_table[index];
}

void update_color_table(Transfer_Overlay* t){
  real_t step = 1.0/(real_t)t->color_table_size;
  real_t iso = 0;
  for(int c = 0; c < t->color_table_size; c++){
    set_color(t, iso, &t->color_table[c]);
    iso += step;
  }
}

void dump_colors(Transfer_Overlay* t){
  FILE* fp = fopen("colors.txt", "w+");
  if(fp == NULL){
    fprintf(stderr, "ERROR: Unable to open 'colors.txt' for writing.\n");
    return;
  }

  for(int c = 0; c < 4; c++){
    for(int d = 0; d < COLOR_BANDS; d++){
      fprintf(fp, "%f ", t->transfers[c].values[d]);
    }
    fprintf(fp, "\n");
  }

  fclose(fp);
  printf("Successfully created 'colors.txt'.\n");
}

void read_colors(Transfer_Overlay* t){
  FILE* fp = fopen("colors.txt", "r");

  if(fp == NULL){
    if(VERBOSE == 1){
      printf("Using default colors.\n");
    }
    return;
  }

  char line[200]; //200 chars shoud to be enough for anybody...
  char* next;

  for(int c = 0; c < 4; c++){
    if(fgets(line, 200, fp) != NULL){
      next = strtok(line, " ");
      for(int d = 0; d < COLOR_BANDS; d++){
        sscanf(next, real_format_color, &t->transfers[c].values[d]);
        next = strtok(NULL, " ");
      }
    }
    else{
      fprintf(stderr, "ERROR: Wrong format in 'colors.txt'. Exiting. \n");
      exit(0);
    }
  }
}


void init_transfer(Transfer* t, int channel){
  t->values = (real_t*)malloc(sizeof(real_t) * COLOR_BANDS);
  t->quads = (Quad*)malloc(sizeof(Quad) * COLOR_BANDS);

  for(int c = 0; c < COLOR_BANDS; c++){
    t->values[c] = get_default_color_value(channel, c);
  }

  t->drag_started = 0;
}

real_t get_default_color_value(int channel, int c){
  real_t p = (real_t)c/COLOR_BANDS;

  switch(channel){
    case 0:
      return p;
    case 1:
      if(p < 0.3)
        return 0;
      else if(p > 0.6)
        return 1;
      else
        return (p - 0.3)/0.3;

    case 2:
      if(p < 0.3)
        return 1;
      else if(p > 0.6)
        return 0;
      else
        return 1 - (p - 0.3)/0.3;
    case 3:
      if(p < 0.3)
        return 0;
      else if(p > 1)
        return 1;
      else{
        double d = (p - 0.3)/2.0;
        if(d > 1)
          d = 1;
        return d;
      }
  }
}

void render_transfer(Transfer* t, int color_index, real_t x, real_t y, real_t w, real_t h){

  Color color = {1.0,0.0,0.0,0.0};
  if(color_index == 0){
    color.r = 1.0;
  }
  if(color_index == 1){
    color.g = 1.0;
  }
  if(color_index == 2){
    color.b = 1.0;
  }
  if(color_index == 3){
    color.r = 0.5; color.g = 0.5; color.b = 0.5;
  }


  for(int c = 0; c < COLOR_BANDS; c++){

    glLineWidth(2);
    glEnable(GL_LINE_SMOOTH);
    if(c < COLOR_BANDS -1){
      real_t x1 = x + ((real_t)c/(COLOR_BANDS-1.0))*w + 0.01;
      real_t y1 = y + t->values[c]*h + 0.01;

      real_t x2 = x + ((real_t)(c+1)/(COLOR_BANDS-1.0))*w + 0.01;
      real_t y2 = y + t->values[(c+1)]*h + 0.01;

      glColor4f(color.r,color.g,color.b,color.a);

      glBegin(GL_LINES);
      glVertex3f(x1, y1, 0);
      glVertex3f(x2,y2, 0);
      glEnd();
    }

    glLineWidth(1);
    glDisable(GL_LINE_SMOOTH);

    t->quads[c].y = y + t->values[c]*h;
    t->quads[c].x = x + ((real_t)c/(COLOR_BANDS-1.0))*w;
    t->quads[c].w = 0.02;
    t->quads[c].h = 0.02;
    t->quads[c].c = color;

    render(&t->quads[c]);
  }
}

void start_drag(Transfer* transfer, real_t x, real_t y){
  for(int c = 0; c < COLOR_BANDS; c++){
    if(inside_quad(&transfer->quads[c], x, y)){
      transfer->drag_started = 1;
      transfer->index_of_draged = c;
    }
  }
}

void stop_drag(Transfer* transfer, real_t x, real_t y){
  if(transfer->drag_started){
    transfer->values[transfer->index_of_draged] = (y-0.05)/0.3;
  }

  transfer->drag_started = 0;
}

