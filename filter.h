// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef FILTER
#define FILTER

#include "node.h"

void filter(Node* root, Ranges* ranges);
void mark_for_removal(Node* root);
void collapse(Node* root);
int comparator(const void* p, const void* n);
void sort(Point** p, int num_points);

#endif
