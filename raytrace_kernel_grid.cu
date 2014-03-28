// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology

#include <cuda.h>
#include <stdio.h>

#include "settings.h"
#include "point.h"
#include "color.h"
#include "grid.h"
#include "global.h"
#include "real.h"
#include "raycreator.h"

namespace g{
__device__ __constant__ real_t STEP_SIZE_D;
__device__ __constant__ real_t INTERPOLATION_RADIUS_D;
__device__ __constant__ Ranges top_ranges;
__device__ __constant__ int x_size;
__device__ __constant__ int y_size;
__device__ __constant__ int z_size;
__device__ __constant__ int num_points;

inline __device__ Coord make_Coord(real_t x, real_t y, real_t z){
  Coord c = {x,y,z};
  return c;
}

inline __device__ Coord operator+(Coord a, Coord b){
  return make_Coord(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline __device__ Coord operator*(Coord a, real_t s){
  return make_Coord(a.x * s, a.y * s, a.z * s);
}

__device__ Ray normalize_ray(Ray r){
  Ray t = r;
  real_t d = sqrt(r.dir.x*r.dir.x + r.dir.y*r.dir.y + r.dir.z*r.dir.z);
  t.dir.x = t.dir.x / d;
  t.dir.y = t.dir.y / d;
  t.dir.z = t.dir.z / d;

  return t;
}

__device__ Color blend_d(Color c, real_t intensity, Color* colors){
  Color new_color, output;

  new_color = colors[(int)((intensity/10.59f)*10000)];

  output.r = c.r + (1-c.a)*new_color.a*new_color.r;
  output.g = c.g + (1-c.a)*new_color.a*new_color.g;
  output.b = c.b + (1-c.a)*new_color.a*new_color.b;

  output.a = c.a + (1-c.a)*new_color.a;

  return output;
}

__device__ Display_color to_display_color_d(Color c){
  Display_color dc;

  if(c.r > 1.0f){
    c.r = 1.0f;
  }

  if(c.b > 1.0f){
    c.b = 1.0f;
  }

  if(c.g > 1.0f){
    c.g = 1.0f;
  }
  
  dc.a = (unsigned char)(c.a*255);
  dc.r = (unsigned char)(c.r*255);
  dc.g = (unsigned char)(c.g*255);
  dc.b = (unsigned char)(c.b*255);

  return dc;
}

__device__ real_t get_intensity_for_pos_d(Coord pos, Grid_cell* indices, Point* points){
  short x = x_size * ((pos.x - top_ranges.xmin)/(top_ranges.xmax - top_ranges.xmin));
  short y = y_size * ((pos.y - top_ranges.ymin)/(top_ranges.ymax - top_ranges.ymin));
  short z = z_size * ((pos.z - top_ranges.zmin)/(top_ranges.zmax - top_ranges.zmin));

  short sub_x = (2*x_size) * ((pos.x - top_ranges.xmin)/(top_ranges.xmax - top_ranges.xmin));
  short sub_y = (2*y_size) * ((pos.y - top_ranges.ymin)/(top_ranges.ymax - top_ranges.ymin));
  short sub_z = (2*z_size) * ((pos.z - top_ranges.zmin)/(top_ranges.zmax - top_ranges.zmin));

  sub_x = sub_x - (x*2);
  sub_y = sub_y - (y*2);
  sub_z = sub_z - (z*2);

  char off[3*8] =  {
    1,1,1,1,0,0,0,0,
    1,1,0,0,1,1,0,0,
    1,0,1,0,1,0,1,0};

  for(char c = 0; c < 8; c ++){
    if(sub_x == 0){
      off[c] *= -1;
    }
    if(sub_y == 0){
      off[8+c] *= -1;
    }
    if(sub_z == 0){
      off[16+c] *= -1;
    }
  }

  real_t intensity = 0;
  real_t weight = 0;

  for(char c = 0; c < 8; c++){
    int index = (z+off[16+c])*x_size*y_size + (y+off[8+c])*x_size + (x+off[c]);

    if(index < 0 || (x+off[c]) >= x_size || (y+off[8+c]) >= y_size || (z+off[16+c]) >= z_size){
      continue;
    }
    if(indices[index].index == -1){
      continue;
    }

    for(int i = indices[index].index; i < indices[index].index + indices[index].length; i++){

      real_t dx = pos.x - points[i].x;
      real_t dy = pos.y - points[i].y;
      real_t dz = pos.z - points[i].z;

      real_t distance = sqrt(dx*dx + dy*dy + dz*dz);

      if(distance < INTERPOLATION_RADIUS_D){
        intensity += (1/distance) * points[i].intensity;
        weight += (1/distance);
      }
    }
  }

  if(intensity <= 0){
    return 0;
  }

  real_t ratio = intensity/weight;
  if(ratio <= 1){
    return 0;
  }

  return log(ratio);
}

__global__ void kernel(Point* points, Grid_cell* indices, Display_color* image, Ray* rays, Color* colors){
  int i = blockIdx.x * blockDim.x + threadIdx.x;
 
  if(rays[i].distance <= 0){
    Display_color b = {0,0,0,0};
    image[i] = b;
    return;
  }

  rays[i] = normalize_ray(rays[i]);

  Coord pos = rays[i].start;
  real_t acc_distance = 0;
  Color output = {0.0,0,0,0.0};

  while(acc_distance < rays[i].distance){
    real_t intensity = get_intensity_for_pos_d(pos, indices, points);

    output = blend_d(output, intensity, colors);

    if(output.a > 0.99f){
      break;
    }

    pos = pos + (rays[i].dir*STEP_SIZE_D);
    acc_distance += STEP_SIZE_D; 
  }

  image[i] = to_display_color_d(output);
}
}

extern "C" void launch_ray_trace_kernel_grid(void* v, Raycreator* rc){
  printf("Tracing rays on GPU using grid datastructure!\n");
  Grid* grid = (Grid*)v;
  int grid_size = grid->x_size*grid->y_size*grid->z_size;

  cudaSetDevice(0);

  Point* points_d;
  Grid_cell* indices_d;
  Display_color* image_d;
  Ray* rays_d;
  Color* colors_d;
  cudaMalloc((void**)&points_d, sizeof(Point)*grid->num_points);
  cudaMalloc((void**)&indices_d, sizeof(Grid_cell)*grid_size);
  cudaMalloc((void**)&image_d, sizeof(Display_color)*RESOLUTION*RESOLUTION);
  cudaMalloc((void**)&rays_d, sizeof(Ray)*RESOLUTION*RESOLUTION);
  cudaMalloc((void**)&colors_d, sizeof(Color)*transfer_overlay->color_table_size);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  cudaMemcpy(points_d, grid->points, sizeof(Point)*grid->num_points, cudaMemcpyHostToDevice);
  cudaMemcpy(indices_d, grid->indices, sizeof(Grid_cell)*grid_size, cudaMemcpyHostToDevice);
  cudaMemcpy(rays_d, rays, sizeof(Ray)*RESOLUTION*RESOLUTION, cudaMemcpyHostToDevice);
  cudaMemcpy(colors_d, transfer_overlay->color_table, sizeof(Color)*transfer_overlay->color_table_size, cudaMemcpyHostToDevice);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));
  
  cudaMemcpyToSymbol(g::STEP_SIZE_D, &STEP_SIZE, sizeof(real_t));
  cudaMemcpyToSymbol(g::INTERPOLATION_RADIUS_D, &INTERPOLATION_RADIUS, sizeof(real_t));
  cudaMemcpyToSymbol(g::top_ranges, &grid->ranges, sizeof(Ranges));
  cudaMemcpyToSymbol(g::x_size, &grid->x_size, sizeof(int));
  cudaMemcpyToSymbol(g::y_size, &grid->y_size, sizeof(int));
  cudaMemcpyToSymbol(g::z_size, &grid->z_size, sizeof(int));
  cudaMemcpyToSymbol(g::num_points, &grid->num_points, sizeof(int));
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  int nThreads=128;
  int totalThreads = RESOLUTION*RESOLUTION;
  int nBlocks = totalThreads/nThreads;
  nBlocks += ((totalThreads%nThreads)>0)?1:0;

  g::kernel<<<nBlocks, nThreads>>>(points_d, indices_d, image_d, rays_d, colors_d);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  int images_index = log2((real_t)RESOLUTION) - 4;
  cudaMemcpy(images[images_index], image_d, sizeof(Display_color)*RESOLUTION*RESOLUTION, cudaMemcpyDeviceToHost);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  cudaFree(indices_d);
  cudaFree(points_d);
  cudaFree(image_d);
  cudaFree(rays_d);
  cudaFree(colors_d);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));
}

extern "C" void deviceProperties(){
  int deviceCount;
  cudaGetDeviceCount(&deviceCount);
  printf("Found %d devices\n", deviceCount);
  for(int d = 0; d < deviceCount; d++){
    cudaDeviceProp devProp;
    cudaGetDeviceProperties(&devProp, d);
    printf("\n");
    printf("Device: %d\n", d);
    printf("%s\n", devProp.name);
    printf("Compute capability: %d.%d\n", devProp.major, devProp.minor);
    printf("Timeout enabled: %d\n", devProp.kernelExecTimeoutEnabled);
    printf("Global memory: %ld\n", devProp.totalGlobalMem);
    printf("Shared memory pr block: %ld\n", devProp.sharedMemPerBlock);
    printf("Registers pr block: %d\n", devProp.regsPerBlock);
  }
}
    
