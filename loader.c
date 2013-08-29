// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#include <stdlib.h>
#include <hdf5.h>
#include <dirent.h>
#include <string.h>

#include "loader.h"
#include "point.h"
#include "raw_point.h"
#include "settings.h"
#include "global.h"

Loader init_loader(int block_size){
  Loader l;

  l.filename = find_filename();

  l.block_size = block_size;
  l.current_pos = 0;

  l.ranges = (Ranges*)malloc(sizeof(Ranges));
  Raw_Ranges raw_ranges;
  hid_t file = H5Fopen(l.filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  hid_t dataset = H5Dopen1(file, "ranges");
  H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, (void*)&raw_ranges);
  H5Dclose(dataset);

  l.ranges->xmin = (real_t)raw_ranges.xmin;
  l.ranges->xmax = (real_t)raw_ranges.xmax;
  l.ranges->ymin = (real_t)raw_ranges.ymin;
  l.ranges->ymax = (real_t)raw_ranges.ymax;
  l.ranges->zmin = (real_t)raw_ranges.zmin;
  l.ranges->zmax = (real_t)raw_ranges.zmax;
  l.ranges->imin = (real_t)raw_ranges.imin;
  l.ranges->imax = (real_t)raw_ranges.imax;

  dataset = H5Dopen1(file, "allpoints");
  hid_t space = H5Dget_space(dataset);
  hsize_t dims[2];
  hsize_t maxdims[2];
  H5Sget_simple_extent_dims(space, dims, maxdims);
  l.total_size = dims[0];

  H5Sclose(space);
  H5Dclose(dataset);
  H5Fclose(file);

  return l;
}


PointList* next(Loader* l){
  int remaining_points = l->total_size - l->current_pos;
  int points_to_read = (remaining_points > l->block_size) ? l->block_size : remaining_points;
  Raw_Point* raw_points = (Raw_Point*)malloc(sizeof(Raw_Point) * points_to_read);

  hid_t file = H5Fopen(l->filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  hid_t dataset = H5Dopen1(file, "allpoints");
  hid_t space = H5Dget_space(dataset);

  hsize_t offset[2];
  hsize_t count[2];
  hsize_t stride[2];

  offset[0] = l->current_pos; offset[1] = 0;
  count[0] = points_to_read; count[1] = 4;
  stride[0] = 1; stride[1] = 1;

  hid_t memspace = H5Screate_simple(2, count, NULL);
  H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, stride, count, NULL);

  H5Dread(dataset, H5T_NATIVE_DOUBLE, memspace, space, H5P_DEFAULT, (void*)raw_points);

  H5Fclose(file);

  l->current_pos += points_to_read;

  int num_points;
  Point* points = filter_and_convert(raw_points, points_to_read, &num_points);
  free(raw_points);

  if(VERBOSE){
    printf("Ignored %d of %d points  (%2.2f%%).\n",
        points_to_read - num_points,
        points_to_read,
        100 * (double)(points_to_read -num_points)/(double)(points_to_read));
  }

  PointList* pl = (PointList*)malloc(sizeof(PointList));
  pl->index = num_points;
  pl->points = points;

  return pl;
}

Point* filter_and_convert(Raw_Point* raw_points, int num_points, int* num_final_points){
  Point* points = (Point*)malloc(sizeof(Point)* num_points);
  int num_filtered_points = 0;
  for(int c = 0; c < num_points; c++){
    Point p;
    p.x = (real_t)raw_points[c].x;
    p.y = (real_t)raw_points[c].y;
    p.z = (real_t)raw_points[c].z;
    p.intensity = (real_t)raw_points[c].intensity;

    if(loader_filter(p)){
      points[num_filtered_points] = p;
      num_filtered_points++;
    }
  }

  Point* final_points = (Point*)malloc(sizeof(Point) * num_filtered_points);
  for(int c = 0; c < num_filtered_points; c++){
    final_points[c] = points[c];
  }

  free(points);
  *num_final_points = num_filtered_points;
  return final_points;
}

int loader_filter(Point p){
  if(p.intensity <= FILTERING_THRESHOLD){
    return 0;
  }
  if(p.x < root->ranges.xmin || p.x > root->ranges.xmax){
    return 0;
  }
  if(p.y < root->ranges.ymin || p.y > root->ranges.ymax){
    return 0;
  }
  if(p.z < root->ranges.zmin || p.z > root->ranges.zmax){
    return 0;
  }
  return 1;
}


int has_next(Loader* l){
  return (l->total_size - l->current_pos) > 0;
}

char* find_filename(){
  if(FILEARG){
    FILE* fp = fopen(FILENAME, "r");
    if(fp == NULL){
      fprintf(stderr,"ERROR: File '%s' does not exist. Exiting.\n", FILENAME);
      exit(-1);
    }
    return FILENAME;
  }
  else{
    int num_files;
    char** files = list_dir(DEFAULT_FOLDER, &num_files);

    for(int c = 0; c < num_files; c++){
      if(strstr(files[c], ".hdf5") != NULL){
        return pathcat(DEFAULT_FOLDER, files[c]);
      }
    }

    fprintf(stderr,"ERROR: No .hdf5 files found in '%s'. Exiting.\n", DEFAULT_FOLDER);
    exit(-1);
  }
}

char** list_dir(char* dirname, int* num_files){
  DIR* dp = opendir(dirname);
  if(dp == NULL){
    fprintf(stderr, "ERROR: directory '%s' does not exist. Exiting.\n", DEFAULT_FOLDER);
    exit(-1);
  }
  struct dirent* entry;

  int counter = 0;
  while(entry = readdir(dp)){
    counter++;
  }

  rewinddir(dp);

  char** filenames = (char**)malloc(sizeof(char*)*counter);

  int c = 0;
  while(entry = readdir(dp)){
    int length = strlen(entry->d_name);

    filenames[c] = (char*)malloc(length + 1);
    strcpy(filenames[c], entry->d_name);
    c++;
  }

  closedir(dp);

  *num_files = counter;
  return filenames;
}

char* pathcat(char* dir, char* file){
  char* fullpath = (char*)malloc(sizeof(char) * strlen(dir) + strlen(file) + 2);
  strcpy(&fullpath[0], dir);
  fullpath[strlen(dir)] = '/';
  strcpy(&fullpath[strlen(dir) + 1], file);
  fullpath[strlen(dir) + strlen(file) + 1] = 0x00; 

  return fullpath;
}

