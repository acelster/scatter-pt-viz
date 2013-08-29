// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "node.h"
#include "point.h"
#include "filter.h"
#include "settings.h"
#include "raycreator.h"
#include "batch.h"
#include "grid.h"

#define num_points 13

double data[num_points][4] = {
  {0.76, 0.77, 0.99, 1.0},
  {0.8, 0.7, 0.99, 2.0},
  {0.88, 0.83, 0.99, 3.0},
  {0.76, 0.99, 0.99, 4.0},
  {0.99, 0.76, 0.99, 5.0},
  {0.6, 0.51, 0.99, 6.0},
  {0.81, 0.76, 0.99, 7.0},
  {0.92, 0.97, 0.99, 8.0},
  {0.97, 0.96, 0.99, 9.0},
  {0.85, 0.91, 0.99, 10.0},
  {0.7, 0.71, 0.99, 1000},
  {0.88, 0.74, 0.99, 11.0},
  {0.75, 0.25, 0.99, 12.0},
};

double data_rect[num_points][4] = {
  {0.94, 1.88, 0.01, 1.0},
  {0.99, 1.99, 0.01, 2.0},
  {0.95, 1.89, 0.01, 3.0},
  {0.95, 1.99, 0.01, 4.0},
  {0.99, 1.97, 0.01, 5.0},
  {0.98, 1.88, 0.01, 6.0},
  {0.96, 1.91, 0.01, 7.0},
  {0.94, 1.97, 0.01, 8.0},
  {0.97, 1.96, 0.01, 9.0},
  {0.96, 1.91, 0.01, 10.0},
  {0.99, 1.90, 0.01, 1000},
  {0.97, 1.97, 0.01, 11.0},
  {0.98, 1.96, 0.01, 12.0},
};

Ranges ranges;
Node* root;
PointList pl;

/*
void dfs_with_check(Node * n, void (*check)(double)){
  if(n->is_leaf){
    for(int c = 0; c < n->num_children; c++){
      (*check)(((Point**)n->pointer)[c]->intensity);
    }
  }
  else{
    for(int c = 0; c < 8; c++){
      dfs_with_check(((Node**)n->pointer)[c], (*check));
    }
  }
  return;
}

void filter_check(double value){
  assert(value != 1000.0);
}

*/
void create_pl(double data[][4]){
  pl.index = num_points;
  pl.points = (Point*)malloc(sizeof(Point) * num_points);

  for(int c = 0; c < num_points; c++){
    pl.points[c].x = data[c][0];
    pl.points[c].y = data[c][1];
    pl.points[c].z = data[c][2];
    pl.points[c].intensity = data[c][3];
  }
}

void test_tree(){
  create_pl(data);
  Tree* tree = create_tree(&ranges);
  insert_points(tree, &pl);

  print_tree(&tree->root, 0);

  printf("\n\n\n");
  for(int c = 0; c < tree->node_list->current_pos; c++){
    print_Node(&tree->node_list->nodes[c]);
  }

  coalesce_points(tree);
  fix_pointers_for_gpu(tree);

  printf("\n\n\n");
  for(int c = 0; c < tree->node_list->current_pos; c++){
    print_Node(&tree->node_list->nodes[c]);
  }
}
unsigned int get_covered_subnodes(Coord pos, Node n){
  printf("Get subnodes %f, %f, %f \n", n.x, n.y, n.z);

  char xmin = pos.x - INTERPOLATION_RADIUS < n.x;
  char ymin = pos.y - INTERPOLATION_RADIUS < n.y;
  char zmin = pos.z - INTERPOLATION_RADIUS < n.z;

  char xmax = pos.x + INTERPOLATION_RADIUS > n.x;
  char ymax = pos.y + INTERPOLATION_RADIUS > n.y;
  char zmax = pos.z + INTERPOLATION_RADIUS > n.z;

  printf("%d,%d,%d,%d,%d,%d\n", xmin,ymin,zmin,xmax,ymax,zmax);

  unsigned char b = 0;

  b = b | (unsigned int)(128 * (xmin && ymin && zmin));
  b = b | (unsigned int)(64 * (xmin && ymin && zmax));
  b = b | (unsigned int)(32 * (xmin && ymax && zmin));
  b = b | (unsigned int)(16 * (xmin && ymax && zmax));
  b = b | (unsigned int)(8 * (xmax && ymin && zmin));
  b = b | (unsigned int)(4 * (xmax && ymin && zmax));
  b = b | (unsigned int)(2 * (xmax && ymax && zmin));
  b = b | (unsigned int)(1 * (xmax && ymax && zmax));

  return b;
}

double get_intensity_for_pos_full(Node root, Coord pos, Node* nodes, Point* points, unsigned int* stack, int base){

  int tos = 0;

  unsigned int current_node = 0;
  unsigned int b = get_covered_subnodes(pos, root);
  char inc = 16;
  printf("%d, %x\n", b,b);
  stack[base + tos] = current_node | (b << 24);
  printf("%d, %x\n", stack[base + tos], stack[base + tos]);
  tos += 16;

  int xx;
  while(tos > 0){
    xx++;
    printf("%d\n", tos);
    current_node = stack[base + tos - inc] & 0x00ffffff;
    b = (stack[base + tos -inc] & 0xff000000) >> 24;

    printf("Popped: %d : %d\n", current_node, b);

    if(b > 255){
      return 0;
    }

    unsigned int t = 1;
    while((b & t) == 0){
      t = t << 1;
    }
    current_node += (unsigned int)log2((float)t);
    b = b ^ t;
    //printf("%d,%d,%d\n", current_node, b, t);

    if(b == 0){
      tos -= inc;
    }

    else{
      stack[base + tos-inc] = (stack[base + tos -inc] & 0x00ffffff) | (b << 24);
    }

    printf("Current node: %d\n", current_node);




    if(nodes[current_node].is_leaf){
      print_Node(&nodes[current_node]);
    }
    else{
      b = get_covered_subnodes(pos, nodes[current_node]);
      stack[base + tos] = ((unsigned int)nodes[current_node].pointer) | (b << 24);
      tos += inc;
    }
    printf("Stack:\n");
    for(int c= 0; c < tos; c++){
      printf("%d, %x\n", stack[c*inc], stack[c*inc]);
    }
    //if(xx > 3)
     // break;
  }

  return 1;
}

void test_gpu_range_search(){
  create_pl(data);
  Tree* tree = create_tree(&ranges);
  insert_points(tree, &pl);

  print_tree(&tree->root, 0);

  printf("\n\n\n");
  for(int c = 0; c < tree->node_list->current_pos; c++){
    print_Node(&tree->node_list->nodes[c]);
  }

  printf("Coalese\n");
  coalesce_points(tree);
  printf("Fixi'n\n");
  fix_pointers_for_gpu(tree);

  int* buffer = (int*)malloc(sizeof(int) * 100);

  Coord pos; pos.x = 0.99; pos.y = 0.99; pos.z = 0.99;
  printf("Getting intensity\n");
  double d = get_intensity_for_pos_full(tree->root, pos, tree->node_list->nodes, tree->point_list->points, buffer, 0);
  printf("%f\n", d);
}
/*

void test_filtering(){
  create_pl(data);
  root = create_tree();
  insert_points(root, &pl, &ranges, 3);
  filter(root, &ranges);
  dfs_with_check(root, &filter_check);
}

void test_find_neighbours(){
  create_pl(data);
  Node* root = create_tree();
  int depth = find_max_depth(&ranges);
  insert_points(root, &pl, &ranges, depth);

  Coord pos;
  pos.x = 0.99;
  pos.y = 0.99;
  pos.z = 0.99;
  Point* neighbours[32];
  int num_neighbours;
  get_neighbour_points(root, &ranges, pos, neighbours, 32, &num_neighbours);

  assert(num_neighbours == 4);

  pos.x = 0.76;
  pos.y = 0.76;
  pos.z = 0.99;
  get_neighbour_points(root, &ranges, pos, neighbours, 32, &num_neighbours);
  assert(num_neighbours == 3);

  pos.x = 0.87;
  pos.y = 0.87;
  pos.z = 0.99;
  get_neighbour_points(root, &ranges, pos, neighbours, 32, &num_neighbours);
  assert(num_neighbours == 11);

  pos.x = 0.87;
  pos.y = 0.5;
  pos.z = 0.99;
  get_neighbour_points(root, &ranges, pos, neighbours, 32, &num_neighbours);
  assert(num_neighbours == 2);
}

void test_find_neighbour_nodes(){
  create_pl(data);
  Node* root = create_tree();
  int depth = find_max_depth(&ranges);
  insert_points(root, &pl, &ranges, depth);

  Coord pos;
  pos.x = 0.1;
  pos.y = 0.1;
  pos.z = 0.1;

  Node* neighbours[8];
  int num_neighbours;
  get_neighbour_nodes(root, &ranges, pos, neighbours, &num_neighbours);

  assert(num_neighbours == 1);

  pos.x = 0.875;
  pos.y = 0.875;
  pos.z = 0.999;
  get_neighbour_nodes(root, &ranges, pos, neighbours, &num_neighbours);

  assert(num_neighbours == 4);
}

void test_range_search(){
  create_pl(data);
  Node* root = create_tree();
  int depth = find_max_depth(&ranges);
  insert_points(root, &pl, &ranges, depth);

  Ranges search_ranges;
  search_ranges.xmin = 0.05; search_ranges.ymin = 0.05; search_ranges.zmin = 0.05;
  search_ranges.xmax = 0.15; search_ranges.ymax = 0.15; search_ranges.zmax = 0.15;


  Node* neighbours[8];
  int num_neighbours = 0;
 
  range_search(root, ranges, &search_ranges, neighbours, &num_neighbours);

  assert(num_neighbours == 1);

  search_ranges.xmin = 0.87; search_ranges.ymin = 0.87; search_ranges.zmin = 0.99;
  search_ranges.xmax = 0.88; search_ranges.ymax = 0.88; search_ranges.zmax = 1.1;

  num_neighbours = 0;
  range_search(root, ranges, &search_ranges, neighbours, &num_neighbours);
  assert(num_neighbours == 4);
}

void test_find_neighbours_rect(){
  Ranges ranges_rect;
  ranges_rect.xmin = 0; ranges_rect.ymin = 0; ranges_rect.zmin = 0;
  ranges_rect.xmax = 1.0; ranges_rect.ymax = 2.0; ranges_rect.zmax = 1.0;

  create_pl(data_rect);
  Node* root = create_tree();
  int depth = find_max_depth(&ranges_rect);
  insert_points(root, &pl, &ranges_rect, depth);

  Coord pos;
  pos.x = 0.95;
  pos.y = 1.95;
  pos.z = 0.01;
  Point* neighbours[32];
  int num_neighbours;
  get_neighbour_points(root, &ranges_rect, pos, neighbours, 32, &num_neighbours);

  assert(num_neighbours == 13);
}

void test_find_max_depth(){
  Ranges r1;
  r1.xmin = 0; r1.ymin = 0; r1.zmin = 0;
  r1.xmax = 10; r1.ymax = 10; r1.zmax = 10;

  int depth = find_max_depth(&r1);
  assert(depth == 6);

  Ranges r2;
  r2.xmin = 0; r2.ymin = 0; r2.zmin = 0;
  r2.xmax = 15; r2.ymax = 17; r2.zmax = 41;

  depth = find_max_depth(&r2);
  assert(depth == 6);
}
*/

void test_find_intersections(){
  Ranges r;
  r.xmin = 0; r.ymin = 0; r.zmin = 0;
  r.xmax = 7; r.ymax = 7; r.zmax = 7;

  Coord start;
  start.x = 0; start.y = -2; start.z = 1;

  Coord dir;
  dir.x = 1; dir.y = 2; dir.z = 0;

  Ray ray;
  ray.start = start;
  ray.dir = dir;

  find_intersections(&ray, &r);

  assert(ray.start.x == 1);
  assert(ray.start.y == 0);
  assert(ray.distance == sqrt(3.5*3.5 + 7*7));


  start.x = 6; start.y = 1; start.z = 5;
  dir.x = -3; dir.y = 0; dir.z = -1;

  ray.start = start;
  ray.dir = dir;

  find_intersections(&ray, &r);

  assert(ray.start.x == 6);
  assert(ray.start.y == 1);
  assert(ray.start.z == 5);
  assert(ray.distance == sqrt(6*6 + 2*2));

  
  dir.x = 3; dir.z = 1;
  ray.dir = dir;

  find_intersections(&ray, &r);
  assert(ray.distance < 1.055 && ray.distance > 1.053);


  start.x = 5.0; start.y = 8.0; start.z = 1;
  dir.x = 1; dir.y = 1; dir.z = 0;
  ray.start = start;
  ray.dir = dir;

  find_intersections(&ray, &r);
  assert(ray.distance < 0);
}

void test_create_rays(){
  /*
  Ranges r;
  r.xmin = 0; r.ymin = 0; r.zmin = 0;
  r.xmax = 7; r.ymax = 7; r.zmax = 7;

  Raycreator* rc = init_raycreator(&r);
  //rc->resolution = 2;
  //rc->r = 7;
  //rc->phi = 0;
  //rc->theta = 3.14159/2;

  //Ray* rays = create_rays(rc);

  assert(rays[0].start.x == 7.0);
  assert(rays[0].start.y > 2.804 && rays[0].start.y < 2.805);
  assert(rays[0].distance > 7.0 && rays[0].distance < 8.0);

  //rc->r = 0.6;
  rays = create_rays(rc);
  assert(rays[0].start.x > 3.999 && rays[0].start.x < 4.001);
  assert(rays[0].start.y > 3.48 && rays[0].start.y < 3.49);
  assert(rays[0].distance > 4.0 && rays[0].distance < 5.0);
  */
}

void test_rotate(){
  Coord a, b, r;
  a.x = -0.23; a.y = 0.11; a.z = 2.34;
  b.x = -1.01; b.y = -1.02; b.z = 3.14;

  double angle = angle_between_Coords(a,b);

  r = rotate(a,b,0.4);

  double ar_angle = angle_between_Coords(a,r);
  double br_angle = angle_between_Coords(b,r);

  printf("%f,%f,%f\n",angle, br_angle, ar_angle);

  assert(fabs(angle - (ar_angle + br_angle)) < 0.001);
  assert(fabs(0.4 - (ar_angle/angle) < 0.001));
}

void test_comparator(){
  Point p;
  Point n;
  Point* pp = &p;
  Point* np = &n;
  Point** ppp = &pp;
  Point** npp = &np;

  p.intensity = 10;
  n.intensity = 100;
  int a = comparator((void*)ppp, (void*)npp);
  assert(a < 0);

  p.intensity = 100;
  n.intensity = 10;
  a = comparator((void*)ppp, (void*)npp);
  assert(a > 0);

  p.intensity = 10;
  a = comparator((void*)ppp, (void*)npp);
  assert(a == 0);

  p.intensity = -100;
  a = comparator((void*)ppp, (void*)npp);
  assert(a > 0);
}

void test_sort(){
  Point points[100];
  Point* points_p[100];
  for(int c = 0; c < 100; c++){
    points[c].intensity = -(100 - c);
    points_p[c] = &points[c];
  }

  assert(points_p[0]->intensity == -100);

  sort(points_p, 100);

  assert(points_p[0]->intensity == -1);
}

void test_get_index(){
  Point a;
  a.x = 0.75;
  a.y = 0.75;
  a.z = 0.75;

  assert(get_index(a, ranges) == 0);

  a.z = 0.25;
  assert(get_index(a, ranges) == 1);
  
  a.y = 0.25;
  assert(get_index(a, ranges) == 3);
  
  a.x = 0.25;
  assert(get_index(a, ranges) == 7);
  
  a.y = 0.75;
  assert(get_index(a, ranges) == 5);
  
  a.z = 0.75;
  assert(get_index(a, ranges) == 4);
  
  a.y = 0.25;
  a.x = 0.75;
  assert(get_index(a, ranges) == 2);

  a.z = 0.75;
  a.x = 0.25;
  assert(get_index(a, ranges) == 6);
}


void test_grid(){
  create_pl(data);
  Grid* g = init_grid(&ranges);

  printf("Inserting points\n");
  insert_points_grid(g, &pl);

  printf("printing points\n");
  print_grid(g);
  Coord p;
  p.x = 0.76;
  p.y = 0.49;
  p.z = 0.99;
  printf("\n\n\n\n\n\n");
  printf("%f\n", get_intensity_for_pos_grid(g, p));
}

void test_combine(){
  unsigned int a = 2047;
  printf("%x\n", a);
  int b = 65;
  printf("%x\n", b);

  unsigned int c = a | (b << 24);
  printf("%x\n", c);

  unsigned int ae = c & 0x00ffffff;
  unsigned int be = (c & 0xff000000) >> 24;

  printf("%x\n", ae);
  printf("%x\n", be);
}


int main(int argc, char** argv){
  INTERPOLATION_RADIUS = 0.125;
  NEIGHBOUR_MODE = 1;

  ranges.xmin = 0; ranges.ymin = 0; ranges.zmin = 0;
  ranges.xmax = 1.0; ranges.ymax = 1.0; ranges.zmax = 1.0;


  //test_filtering();
  //test_find_neighbours();
  //test_find_neighbours_rect();
  //test_find_max_depth();
  //test_find_neighbour_nodes();
  //test_range_search();
  //test_find_intersections();
  //test_create_rays();
  //test_rotate();
  //test_comparator();
  //test_sort();
  //test_tree();
  //test_get_index();
  test_grid();
  //test_combine();
  //test_gpu_range_search();
}

