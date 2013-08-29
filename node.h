// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef NODE
#define NODE

#include "point.h"
#include "real.h"

typedef struct{
  int is_leaf; //And also size of child array.. clever
  int num_children;
  void** pointer;
  real_t x;
  real_t y;
  real_t z;
} Node;

typedef struct{
  Node* nodes;
  int size;
  int current_pos;
} Node_list;

typedef struct{
  Node root;
  Ranges ranges;
  int max_depth;
  int total_num_points;
  Node_list* node_list;
  PointList* point_list;
  PointList** pls;
  int pls_index;
} Tree;

Tree* create_tree();
void insert_points(void* t, PointList* point_list);
void finalize(void* s);
real_t get_intensity_for_pos_tree(void* v, Coord pos);
void insert_points_node(Node* n, PointList* point_list, Node_list** nl, Ranges* ranges, int m_d);
void insert_point(Node* n, Point* p, Ranges r, int depth);
void move_points(Node* n, PointList* point_list);
void coalesce_points(Tree* tree);
void fix_pointers_for_gpu(Tree* t);
int find_max_depth(Ranges* ranges);
long get_memory_useage_tree(void* i);

int get_index(Point p, Ranges r);
Ranges get_ranges_for_index(int index, Ranges r);

Node* get_node_for_pos(Node* root,
    Ranges* ranges,
    Coord pos);

void range_search(Node* node,
    Ranges* search_ranges,
    PointList* nodes_found,
    int* num_nodes_found);

void range_search2(Node* nodes,
    Ranges* search_ranges,
    PointList* nodes_found,
    int* num_nodes_found);

void print_tree(Node* n, int depth);
void print_Node(Node* n);

Node_list* init_node_list(int num_nodes);
Node* get_new_nodes(Node_list** nl, int num_nodes);
PointList* init_point_list(int num_points);
Point* get_new_points(PointList* pl, int num_points);

#endif
