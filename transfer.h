// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef TRANSFER
#define TRANSFER

#include "quad.h"
#include "color.h"
#include "real.h"
#include "point.h"

typedef struct{
  real_t* values;
  Quad* quads;
  int drag_started;
  int index_of_draged;
} Transfer;

typedef struct{
  Color* color_table;
  int color_table_size;
  Transfer* transfers;
  Quad* quads;
  int color_selected;
  int visible;
  real_t max_intensity;
} Transfer_Overlay;

Transfer_Overlay* init_transfer_overlay(Ranges* r);
void render_transfer_overlay(Transfer_Overlay* t);
void handle_mouse(Transfer_Overlay* t, real_t x, real_t y, int state);
void set_color(Transfer_Overlay * t, real_t iso, Color* color);
void set_color_from_table(Transfer_Overlay * t, real_t iso, Color* color);
void update_color_table(Transfer_Overlay* t);
void dump_colors(Transfer_Overlay* t);
void read_colors(Transfer_Overlay* t);

void init_transfer(Transfer* t, int channel);
real_t get_default_color_value(int channel, int c);
void render_transfer(Transfer* t, int color, real_t x, real_t y, real_t w, real_t h);
void start_drag(Transfer* t, real_t x, real_t y);
void stop_drag(Transfer* t, real_t x, real_t y);

#endif
