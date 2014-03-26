#include <cuda.h>
#include <stdio.h>

#include "ray.h"
#include "raycreator.h"
#include "settings.h"
#include "point.h"
#include "real.h"

//#include "kernel.h"

__device__ __constant__ Coord up;
__device__ __constant__ Coord eye;
__device__ __constant__ Coord screen_center_d;
__device__ __constant__ Coord right;
__device__ __constant__ Ranges ranges; //DEPRECATED
__device__ __constant__ Coord top_d;
__device__ __constant__ Coord bottom_d;


inline __device__ Coord make_Coord(real_t x, real_t y, real_t z){
  Coord c = {x,y,z};
  return c;
}

inline __device__ Coord operator-(Coord a, Coord b){
  return make_Coord(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline __device__ Coord operator+(Coord a, Coord b){
  return make_Coord(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline __device__ Coord operator*(Coord a, real_t s){
  return make_Coord(a.x * s, a.y * s, a.z * s);
}

inline __device__ Coord operator/(Coord a, Coord b){
  return make_Coord(a.x / b.x, a.y / b.y, a.z / b.z);
}

static __inline__ __device__ Coord fminf(Coord a, Coord b){
  return make_Coord(fminf(a.x,b.x), fminf(a.y,b.y), fminf(a.z,b.z));
}

static __inline__ __device__ Coord fmaxf(Coord a, Coord b){
  return make_Coord(fmaxf(a.x,b.x), fmaxf(a.y,b.y), fmaxf(a.z,b.z));
}

__global__ void kernel(Ray* rays, real_t pixel_width, int res){

  int index = blockIdx.x * blockDim.x + threadIdx.x;

  short c = (index % res) - (res/2);
  short d = (index / res) - (res/2);

  Coord start = screen_center_d + up * (d*pixel_width) + right * (c*pixel_width);

  Coord dir = start - eye;

  Coord t1 = (top_d - start)/dir;
  Coord t2 = (bottom_d - start)/dir;

  Coord tmin = fminf(t1, t2);
  Coord tmax = fmaxf(t1, t2);

  real_t tnear = fmaxf(fmaxf(tmin.x, tmin.y), fmaxf(tmin.x, tmin.z));
  real_t tfar = fminf(fminf(tmax.x, tmax.y), fminf(tmax.x, tmax.z));

  if(tfar <= tnear){
    rays[index].distance = 0;
    return;
  }

  Coord far = start + dir * tfar;

  tnear = fmax(0, tnear);
  start = start + dir * tnear;

  Coord diff = start - far;

  rays[index].distance = sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);

  rays[index].start = start;
  rays[index].dir = dir;

  rays[index].color.r = -1;
  rays[index].color.g = -1;
  rays[index].color.b = -1;
  rays[index].color.a = -1;
}

extern "C" void launch_kernel(Ray* rays,
    Raycreator* rc,
    Coord screen_center,
    real_t pixel_width){

  cudaSetDevice(0);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  Ray* rays_d;
  cudaMalloc((void**)&rays_d, sizeof(Ray) * RESOLUTION * RESOLUTION);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  int nThreads=128;
  int totalThreads = RESOLUTION*RESOLUTION;
  int nBlocks = totalThreads/nThreads;
  nBlocks += ((totalThreads%nThreads)>0)?1:0;

  Coord top = {rc->ranges->xmax, rc->ranges->ymax, rc->ranges->zmax};
  Coord bottom = {rc->ranges->xmin, rc->ranges->ymin, rc->ranges->zmin};


  cudaMemcpyToSymbol(up, &rc->up, sizeof(Coord));
  cudaMemcpyToSymbol(top_d, &top, sizeof(Coord));
  cudaMemcpyToSymbol(bottom_d, &bottom, sizeof(Coord));
  cudaMemcpyToSymbol(eye, &rc->eye, sizeof(Coord));
  cudaMemcpyToSymbol(right, &rc->right, sizeof(Coord));
  cudaMemcpyToSymbol(screen_center_d, &screen_center, sizeof(Coord));
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  kernel<<< nBlocks, nThreads>>>(rays_d, pixel_width, RESOLUTION);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));


  cudaMemcpy(rays, rays_d, sizeof(Ray) * RESOLUTION*RESOLUTION, cudaMemcpyDeviceToHost);
  printf("%s\n", cudaGetErrorString(cudaGetLastError()));

  cudaFree(rays_d);
}

