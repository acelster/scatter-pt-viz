// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef BIN_NODE
#define BIN_NODE

#include "point.h"

typedef struct{
  char is_leaf;
  double value;
  void* left;
  void* right;
} Bin_node;

Bin_node* create_bin_tree(Point*** points, int num_points, int level);
void print_bin_node(Bin_node n);

#endif
