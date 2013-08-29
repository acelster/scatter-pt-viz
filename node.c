// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>

#include "node.h"
#include "point.h"
#include "settings.h"
#include "global.h"
#include "timer.h"
#include "filter.h"
#include "kriging.h"
#include "idw.h"

static int max_depth;
static int max_num_children;
static int num_leaves;
static Node_list** node_list;

Tree* create_tree(Ranges* r){
  Tree* t = (Tree*)malloc(sizeof(Tree));
  t->ranges = *r;
  t->max_depth = find_max_depth(r);

  int numNodesUpperBound;
  if(t->max_depth > 9){
    numNodesUpperBound = 1000000;
#ifdef USE_CUDA
    fprintf(stderr, "WARNING: Unable to predict upper bound on number of nodes.");
#endif
  }
  else{
    int numNodesUpperBound = (int)(pow(8,t->max_depth) * 1.15);
  }

  t->node_list = init_node_list(numNodesUpperBound);
  t->total_num_points = 0;

  t->pls = (PointList**)malloc(sizeof(PointList*) * 100);
  t->pls_index = 0;

  t->root.num_children = 0;
  t->root.is_leaf = 8;
  t->root.x = r->xmin + (r->xmax - r->xmin)/2.0;
  t->root.y = r->ymin + (r->ymax - r->ymin)/2.0;
  t->root.z = r->zmin + (r->zmax - r->zmin)/2.0;

  return t;
}

void insert_points(void* v, PointList* point_list){
  Tree* t = (Tree*)v;
  insert_points_node(&t->root,
      point_list,
      &t->node_list,
      &t->ranges,
      t->max_depth);
  t->total_num_points += point_list->index;
  t->pls[t->pls_index] = point_list;
  t->pls_index++;
}

void finalize(void* v){
  Tree* tree = (Tree*)v;

  if(MEDIAN_FILTERING){
    timer_start("Filtering...\n");
    filter(&tree->root, &tree->ranges);
    timer_end();
  }

  coalesce_points(tree);

#ifdef USE_CUDA
  fix_pointers_for_gpu(tree);
#endif

  if(VERBOSE >= 1){
    printf("Max tree depth: %d\n", tree->max_depth);
    printf("Total number of points: %d\n", tree->total_num_points);
  }
}

real_t get_intensity_for_pos_tree(void* v, Coord pos){
  Tree* tree = (Tree*)v;

  //All points within radius in this
  if(NEIGHBOUR_MODE == 0){
    Node* node = get_node_for_pos(&tree->root, &tree->ranges, pos);
    PointList pl;
    pl.index = node->num_children;
    pl.points = (Point*)node->pointer;
    if(INTERPOLATION_MODE == 0){
      return krige(&pl, 1, pos);
    }
    else{
      return idw_interpolate(&pl, 1, pos);
    }
  }

  //All points within radius
  else if(NEIGHBOUR_MODE == 1){

    //PointList* neighbours = (PointList*)malloc(sizeof(PointList) * 2048);
    //256 neighbours ought to be enough for anybody...
    PointList neighbours[256];
    int num_neighbours = 0;

    Ranges search_ranges;
    real_t x_radius = INTERPOLATION_RADIUS;
    real_t y_radius = INTERPOLATION_RADIUS;
    real_t z_radius = INTERPOLATION_RADIUS;

    if(ANISOTROPIC){
        x_radius *= (1/sqrt(ANISO_MATRIX[0]));
        y_radius *= (1/sqrt(ANISO_MATRIX[4]));
        z_radius *= (1/sqrt(ANISO_MATRIX[8]));
    }

    search_ranges.xmin = pos.x - x_radius;
    search_ranges.ymin = pos.y - y_radius;
    search_ranges.zmin = pos.z - z_radius;
    search_ranges.xmax = pos.x + x_radius;
    search_ranges.ymax = pos.y + y_radius;
    search_ranges.zmax = pos.z + z_radius;

    range_search(&tree->root, &search_ranges, neighbours, &num_neighbours);
    //range_search2(&tree->root, &search_ranges, neighbours, &num_neighbours);

    if(INTERPOLATION_MODE == 0){
      real_t d = krige(neighbours, num_neighbours, pos);
      //free(neighbours);
      return d;
    }
    else{
     real_t d = idw_interpolate(neighbours, num_neighbours, pos);
     //free(neighbours);
     return d;
    }
  }
  else{
    fprintf(stderr, "ERROR: Invalid value for option 'NEIGHBOUR_MODE'. Exiting.\n");
    exit(-1);
  }
}

void coalesce_points(Tree* t){
  t->point_list = init_point_list(t->total_num_points);
  move_points(&t->root, t->point_list);

  for(int c = 0; c < t->pls_index; c++){
    free(t->pls[c]->points);
    free(t->pls[c]);
  }
  free(t->pls);
}

void fix_pointers_for_gpu(Tree* t){
  void** base_nodes = (void**) t->node_list->nodes;
  void** base_points = (void**) t->point_list->points;

  for(int c = 0; c < t->node_list->current_pos; c++){
    if(t->node_list->nodes[c].is_leaf){
      t->node_list->nodes[c].pointer = (void**) ((t->node_list->nodes[c].pointer - base_points)*sizeof(void*)/sizeof(Point));
    }
    else{
      t->node_list->nodes[c].pointer = (void**) ((t->node_list->nodes[c].pointer - base_nodes)*sizeof(void*)/sizeof(Node));
    }
  }

  t->root.pointer = (void**) ((t->root.pointer - base_nodes) *sizeof(void*)/sizeof(Node));
}

void insert_points_node(Node* n, PointList* point_list, Node_list** nl, Ranges* ranges, int m_d){
  max_depth = m_d;
  max_num_children = 0;
  num_leaves = 0;
  node_list = nl;

  for(int c = 0; c < point_list->index; c++){
    insert_point(n, &point_list->points[c], *ranges, 0);
  }
  if(VERBOSE == 2){
    printf("\tMax depth: %d\n", max_depth);
    printf("\tMax number of children: %d\n", max_num_children);
    printf("\tNumber of leaves: %d\n", num_leaves);
  }
}

void insert_point(Node* n, Point* p, Ranges r, int depth){
  if(n->is_leaf){

    //We insert the point in this node
    if(n->num_children < 8 || depth >= max_depth){
      if(n->num_children == 0){
        n->pointer = (void**)malloc(sizeof(void*) * 8);
      }

      //We need to expand the child array
      if(n->num_children == n->is_leaf){
        n->is_leaf *= 2; //We should check that it doesn't overflow...
        n->pointer = (void**)realloc((void*)n->pointer, sizeof(void*) * n->is_leaf);

        if(n->pointer == NULL){
          printf("%d NULL!\n", n->is_leaf);
        }
      }

      n->pointer[n->num_children++] = (void*)p;

      if(n->num_children > max_num_children){
        max_num_children = n->num_children;
      }
    }

    //This node is full, and should be split
    else{
      //Back up points
      Point* points[8];
      for(int c = 0; c < 8; c++){
        points[c] = (Point*)n->pointer[c];
      }
      free(n->pointer);

      //Overwrite to make new leaf nodes
      n->pointer = (void**)get_new_nodes(node_list, 8);
      real_t dx = (r.xmax - r.xmin)/4.0;
      real_t dy = (r.ymax - r.ymin)/4.0;
      real_t dz = (r.zmax - r.zmin)/4.0;

      for(int c= 0; c < 8; c++){
        ((Node*)n->pointer)[c].is_leaf = 8;
        ((Node*)n->pointer)[c].num_children = 0;
        ((Node*)n->pointer)[c].x = n->x + (dx * ((c < 4) ? 1 : -1));
        ((Node*)n->pointer)[c].y = n->y + (dy * ((c%4 < 2) ? 1 : -1));
        ((Node*)n->pointer)[c].z = n->z + (dz * ((c%2 == 0) ? 1 : -1));
      }

      //Insert existing points into new leafs
      for(int c = 0; c < 8; c++){
        int index = get_index(*points[c], r);
        if(index == -1){
          //No point left behind!
          continue;
        }
        Ranges nr = get_ranges_for_index(index, r);
        insert_point( &((Node*)n->pointer)[index], points[c], nr, depth+1);
      }

      //Insert new point into new leafs
      int index = get_index(*p,r);
      Ranges nr = get_ranges_for_index(index, r);
      insert_point( &((Node*)n->pointer)[index] , p, nr, depth+1);

      //This node is no longer a leaf
      n->is_leaf = 0;

      num_leaves += 7;
    }
  }
  //This ain't no leaf, we just continue down
  else{
    int index = get_index(*p,r);
    Ranges nr = get_ranges_for_index(index, r);
    insert_point( &((Node*)n->pointer)[index] , p, nr, depth+1);
  }
}

void move_points(Node* n, PointList* pl){
  if(n->is_leaf){
    Point* p = get_new_points(pl, n->num_children);
    for(int c = 0; c < n->num_children; c++){
      p[c] = *((Point**)n->pointer)[c];
    }
    if(n->num_children > 0){
      free(n->pointer);
    }
    n->pointer = (void**)p;
  }
  else{
    for(int c = 0; c < 8; c++){
      move_points(&((Node*)n->pointer)[c], pl);
    }
  }
}

Point get_point_for_coord(Coord c){
  Point p = {c.x,c.y,c.z,0};
  return p;
}

Node* search(Node* root, Point pos, Ranges* r){
  if(root->is_leaf){
    return root;
  }

  int index = get_index(pos, *r);
  if(index == -1){
    return NULL;
  }

  Ranges nr = get_ranges_for_index(index, *r);
  return search( &((Node*)root->pointer)[index], pos, &nr);
}

Node* get_node_for_pos(Node* root, Ranges* ranges, Coord pos){
  Point pos_point = get_point_for_coord(pos);
  return search(root,pos_point, ranges);
}


int find_max_depth(Ranges* ranges){

  int depth = 0;

  real_t min_range = ranges->xmax-ranges->xmin;
  min_range = (ranges->ymax-ranges->ymin < min_range) ? ranges->ymax - ranges->ymin : min_range;
  min_range = (ranges->zmax-ranges->zmin < min_range) ? ranges->zmax - ranges->zmin : min_range;

  while(min_range >= 2*INTERPOLATION_RADIUS){
    depth += 1;
    min_range /= 2;
  }

  int unadjusted_max_depth = depth - 1;
  return unadjusted_max_depth + 2;
}

Ranges get_ranges_for_index(int index, Ranges r){
  Ranges nr = r;
  if(index == 0){
    nr.xmin += (r.xmax -r.xmin)/2.0;
    nr.ymin += (r.ymax -r.ymin)/2.0;
    nr.zmin += (r.zmax -r.zmin)/2.0;
  }
  if(index == 1){
    nr.xmin += (r.xmax -r.xmin)/2.0;
    nr.ymin += (r.ymax -r.ymin)/2.0;
    nr.zmax -= (r.zmax -r.zmin)/2.0;
  }
  if(index == 2){
    nr.xmin += (r.xmax -r.xmin)/2.0;
    nr.ymax -= (r.ymax -r.ymin)/2.0;
    nr.zmin += (r.zmax -r.zmin)/2.0;
  }
  if(index == 3){
    nr.xmin += (r.xmax -r.xmin)/2.0;
    nr.ymax -= (r.ymax -r.ymin)/2.0;
    nr.zmax -= (r.zmax -r.zmin)/2.0;
  }
  if(index == 4){
    nr.xmax -= (r.xmax -r.xmin)/2.0;
    nr.ymin += (r.ymax -r.ymin)/2.0;
    nr.zmin += (r.zmax -r.zmin)/2.0;
  }
  if(index == 5){
    //nr.xmax -+ (r.xmax -r.xmin)/2.0;
    nr.xmax -= (r.xmax -r.xmin)/2.0;
    nr.ymin += (r.ymax -r.ymin)/2.0;
    nr.zmax -= (r.zmax -r.zmin)/2.0;
  }
  if(index == 6){
    nr.xmax -= (r.xmax -r.xmin)/2.0;
    nr.ymax -= (r.ymax -r.ymin)/2.0;
    nr.zmin += (r.zmax -r.zmin)/2.0;
  }
  if(index == 7){
    nr.xmax -= (r.xmax -r.xmin)/2.0;
    nr.ymax -= (r.ymax -r.ymin)/2.0;
    nr.zmax -= (r.zmax -r.zmin)/2.0;
  }

  return nr;
}

int get_index(Point current_point, Ranges r){

  if(current_point.x > r.xmax || current_point.x < r.xmin){
    return -1;
  }
  if(current_point.y > r.ymax || current_point.y < r.ymin){
    return -1;
  }
  if(current_point.z > r.zmax || current_point.z < r.zmin){
    return -1;
  }

  char a = current_point.x < r.xmin + (r.xmax - r.xmin)/2.0;
  char b = current_point.y < r.ymin + (r.ymax - r.ymin)/2.0;
  char c = current_point.z < r.zmin + (r.zmax - r.zmin)/2.0;

  return a*4 + b*2 + c*1;
}


void range_search(Node* node,
    Ranges* search_ranges,
    PointList* nodes_found,
    int* num_nodes_found){


  if(node->is_leaf){
    nodes_found[*num_nodes_found].points = (Point*)node->pointer;
    nodes_found[*num_nodes_found].index = node->num_children;
    (*num_nodes_found)++;
    return;
  }

  int xmin = search_ranges->xmin < node->x;
  int ymin = search_ranges->ymin < node->y;
  int zmin = search_ranges->zmin < node->z;

  int xmax = search_ranges->xmax > node->x;
  int ymax = search_ranges->ymax > node->y;
  int zmax = search_ranges->zmax > node->z;

  if(xmin && ymin && zmin){
    range_search( &((Node*)node->pointer)[7], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmin && ymin && zmax){
    range_search( &((Node*)node->pointer)[6], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmin && ymax && zmin){
    range_search( &((Node*)node->pointer)[5], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmin && ymax && zmax){
    range_search( &((Node*)node->pointer)[4], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmax && ymin && zmin){
    range_search( &((Node*)node->pointer)[3], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmax && ymin && zmax){
    range_search( &((Node*)node->pointer)[2], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmax && ymax && zmin){
    range_search( &((Node*)node->pointer)[1], search_ranges, nodes_found, num_nodes_found);
  }
  if(xmax && ymax && zmax){
    range_search( &((Node*)node->pointer)[0], search_ranges, nodes_found, num_nodes_found);
  }

  return;
}



/*
void range_search2(Node* node,
    Ranges* search_ranges,
    PointList* nodes_found,
    int* num_nodes_found){

  Node* stack[100];
  stack[0] = node;
  int tos = 1;

  while(tos > 0){
    Node* current = stack[tos - 1];
    tos--;

    if(current->is_leaf){
      nodes_found[*num_nodes_found].points = (Point*)current->pointer;
      nodes_found[*num_nodes_found].index = current->num_children;
      (*num_nodes_found)++;

      if(*num_nodes_found > 127){
        fprintf(stderr,"ERROR: too many neighbours... Exiting.");
        exit(-1);
      }
    }
    else{
      int xmin = search_ranges->xmin < current->x;
      int ymin = search_ranges->ymin < current->y;
      int zmin = search_ranges->zmin < current->z;

      int xmax = search_ranges->xmax > current->x;
      int ymax = search_ranges->ymax > current->y;
      int zmax = search_ranges->zmax > current->z;


      if(xmin && ymin && zmin){
        stack[tos] = &((Node*)current->pointer)[7];
        tos++;
      }
      if(xmin && ymin && zmax){
        stack[tos] = &((Node*)current->pointer)[6];
        tos++;
      }
      if(xmin && ymax && zmin){
        stack[tos] = &((Node*)current->pointer)[5];
        tos++;
      }
      if(xmin && ymax && zmax){
        stack[tos] = &((Node*)current->pointer)[4];
        tos++;
      }
      if(xmax && ymin && zmin){
        stack[tos] = &((Node*)current->pointer)[3];
        tos++;
      }
      if(xmax && ymin && zmax){
        stack[tos] = &((Node*)current->pointer)[2];
        tos++;
      }
      if(xmax && ymax && zmin){
        stack[tos] = &((Node*)current->pointer)[1];
        tos++;
      }
      if(xmax && ymax && zmax){
        stack[tos] = &((Node*)current->pointer)[0];
        tos++;
      }

      if(tos > 99){
        fprintf(stderr,"ERROR: Range search stack too small. Exiting.");
        exit(-1);
      }
    }
  }
}
*/


Node* get_new_nodes(Node_list** nl, int num_nodes){
  if((*nl)->current_pos + num_nodes > (*nl)->size + 1){
#ifdef USE_CUDA
    fprintf(stderr, "ERROR: Failed to allocate enough nodes. Exiting.\n");
    exit(-1);
#endif
    *nl = init_node_list(1000000);
  }

  Node* n = &((*nl)->nodes[(*nl)->current_pos]);
  (*nl)->current_pos += 8;

  return n;
}

Node_list* init_node_list(int num_nodes){
  Node_list* nl = (Node_list*)malloc(sizeof(Node_list));

  nl->size = num_nodes;
  nl->current_pos = 0;
  nl->nodes = (Node*)malloc(sizeof(Node) * nl->size);
  return nl;
}

Point* get_new_points(PointList* pl, int num_points){
  Point* p = &pl->points[pl->index];
  pl->index += num_points;
  return p;
}

PointList* init_point_list(int num_points){
  PointList* pl = (PointList*)malloc(sizeof(PointList));
  pl->index = 0;
  pl->points = (Point*)malloc(sizeof(Point)*num_points);

  return pl;
}

long get_memory_useage_tree(void* v){
  Tree* t = (Tree*)v;
  long nodes = sizeof(Node)*t->node_list->current_pos;
  long points = sizeof(Point)*t->total_num_points;

  if(VERBOSE >= 1){
    printf("Sizeof nodes: %ld, sizeof points: %ld\n", nodes, points);
  }

  return nodes + points;
}


void print_tree(Node* root, int depth){
  for(int c = 0; c < depth; c++){
    printf("\t");
  }
  print_Node(root);

  if(root->is_leaf){
    return;
  }
  else{
    for(int c = 0; c < 8; c++){
      print_tree(&((Node*)root->pointer)[c], depth+1);
    }
  }
}

void print_Node(Node* n){
  printf("is_leaf: %d num_children: %d x: %f y: %f z: %f\n", n->is_leaf, n->num_children, n->x, n->y, n->z);
}

