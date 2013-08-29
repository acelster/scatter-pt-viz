// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>
#include <stdlib.h>

#include "filter.h"
#include "bin_node.h"

Bin_node* create_bin_tree(Point*** points, int num_points, int depth){
  int level = depth % 3;

  if(num_points == 1 || num_points == 2){
    Bin_node* n = (Bin_node*)malloc(sizeof(Bin_node));

    n->is_leaf = 1;
    n->value = ((double*)points[level][0])[level];
    n->left = (void*)points[level][0];
    if(num_points == 2){
      n->right = (void*)points[level][1];
    }
    else{
      n->right = NULL;
    }

    return n;
  }

}

void print_bin_node(Bin_node n){
  printf("is leaf: %d\n", n.is_leaf);
  printf("value: %f\n", n.value);
}
    

  

