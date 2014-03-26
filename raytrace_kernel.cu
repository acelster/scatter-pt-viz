#include <stdio.h>
#include <cuda.h>
#include <omp.h>

#include "node.h"
#include "point.h"
#include "color.h"
#include "global.h"
#include "settings.h"
#include "real.h"
#include "raycreator.h"

#define NODE_CACHE_SIZE 128
//#define TEXTURE

//Preallocating for up to 4 devices, a little wasteful if fewer are used...
Node* nodes_d[4];
Point* points_d[4];
Display_color* image_d[4];
Ray* rays_d[4];
Color* colors_d[4];
unsigned int * stack_d[4];

__device__ __constant__ real_t STEP_SIZE_D;
__device__ __constant__ real_t INTERPOLATION_RADIUS_D;
__device__ __constant__ real_t X_RADIUS;
__device__ __constant__ real_t Y_RADIUS;
__device__ __constant__ real_t Z_RADIUS;
__device__ __constant__ real_t STEP_FACTOR_D;
__device__ __constant__ real_t STEP_LIMIT_D;
__device__ __constant__ real_t MAX_INTENSITY_D;
__device__ __constant__ Ranges top_ranges;
__device__ __constant__ Node root_node;
__device__ __constant__ real_t aniso_x;
__device__ __constant__ real_t aniso_y;
__device__ __constant__ real_t aniso_z;

//From raycreator
__device__ __constant__ Coord up;
__device__ __constant__ Coord eye;
__device__ __constant__ Coord screen_center_d;
__device__ __constant__ Coord right;
__device__ __constant__ Coord top_d;
__device__ __constant__ Coord bottom_d;

__device__ __constant__ int RESOLUTION_D;

#ifdef TEXTURE
texture<float4, cudaTextureType1D, cudaReadModeElementType> pointTexture;
#endif

inline __device__ Coord make_Coord(real_t x, real_t y, real_t z){
  Coord c = {x,y,z};
  return c;
}

inline __device__ Coord operator+(Coord a, Coord b){
  return make_Coord(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline __device__ Coord operator-(Coord a, Coord b){
  return make_Coord(a.x - b.x, a.y - b.y, a.z - b.z);
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

  new_color = colors[(int)((intensity/MAX_INTENSITY_D)*10000)];

  output.r = c.r + (1-c.a)*new_color.a*new_color.r;
  output.g = c.g + (1-c.a)*new_color.a*new_color.g;
  output.b = c.b + (1-c.a)*new_color.a*new_color.b;

  output.a = c.a + (1-c.a)*new_color.a;

  return output;
}

__device__ Display_color to_display_color_d(Color c){
  Display_color dc;

  if(c.r > 1.0f){
    c.r = 1.0;
  }

  if(c.b > 1.0f){
    c.b = 1.0;
  }

  if(c.g > 1.0f){
    c.g = 1.0;
  }
  
  dc.a = (unsigned char)(c.a*255);
  dc.r = (unsigned char)(c.r*255);
  dc.g = (unsigned char)(c.g*255);
  dc.b = (unsigned char)(c.b*255);

  return dc;
}

__device__ int get_index_d(Coord pos, Ranges r){
  char a = pos.x < r.xmin + (r.xmax - r.xmin)/2.0f;
  char b = pos.y < r.ymin + (r.ymax - r.ymin)/2.0f;
  char c = pos.z < r.zmin + (r.zmax - r.zmin)/2.0f;

  return a*4 + b*2 + c*1;
}

__device__ Ranges get_ranges_for_index_d(Ranges r, int index){
  Ranges nr = r;
  if(index == 0){
    nr.xmin += (r.xmax -r.xmin)/2.0f;
    nr.ymin += (r.ymax -r.ymin)/2.0f;
    nr.zmin += (r.zmax -r.zmin)/2.0f;
  }
  if(index == 1){
    nr.xmin += (r.xmax -r.xmin)/2.0f;
    nr.ymin += (r.ymax -r.ymin)/2.0f;
    nr.zmax -= (r.zmax -r.zmin)/2.0f;
  }
  if(index == 2){
    nr.xmin += (r.xmax -r.xmin)/2.0f;
    nr.ymax -= (r.ymax -r.ymin)/2.0f;
    nr.zmin += (r.zmax -r.zmin)/2.0f;
  }
  if(index == 3){
    nr.xmin += (r.xmax -r.xmin)/2.0f;
    nr.ymax -= (r.ymax -r.ymin)/2.0f;
    nr.zmax -= (r.zmax -r.zmin)/2.0f;
  }
  if(index == 4){
    nr.xmax -= (r.xmax -r.xmin)/2.0f;
    nr.ymin += (r.ymax -r.ymin)/2.0f;
    nr.zmin += (r.zmax -r.zmin)/2.0f;
  }
  if(index == 5){
    nr.xmax -= (r.xmax -r.xmin)/2.0f;
    nr.ymin += (r.ymax -r.ymin)/2.0f;
    nr.zmax -= (r.zmax -r.zmin)/2.0f;
  }
  if(index == 6){
    nr.xmax -= (r.xmax -r.xmin)/2.0f;
    nr.ymax -= (r.ymax -r.ymin)/2.0f;
    nr.zmin += (r.zmax -r.zmin)/2.0f;
  }
  if(index == 7){
    nr.xmax -= (r.xmax -r.xmin)/2.0f;
    nr.ymax -= (r.ymax -r.ymin)/2.0f;
    nr.zmax -= (r.zmax -r.zmin)/2.0f;
  }

  return nr;
}

__device__ real_t interpolate(Node n, Coord pos, Point* points){
  real_t weight = 0; 
  real_t intensity = 0;

  for(int c = 0; c < n.num_children; c++){
    real_t dx = pos.x - points[(long int)n.pointer + c].x;
    real_t dy = pos.y - points[(long int)n.pointer + c].y;
    real_t dz = pos.z - points[(long int)n.pointer + c].z;

    real_t distance = sqrt(dx*dx + dy*dy + dz*dz);

    if(distance < INTERPOLATION_RADIUS_D){
      intensity += (1/distance)*points[(long int)n.pointer + c].intensity;
      weight += (1/distance);
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

inline __device__ unsigned int get_covered_subnodes(Coord pos, Node n){

//    real_t X_RADIUS = INTERPOLATION_RADIUS_D;
 //   real_t Y_RADIUS = INTERPOLATION_RADIUS_D;
  //  real_t Z_RADIUS = INTERPOLATION_RADIUS_D;

  char xmin = pos.x - X_RADIUS < n.x;
  char ymin = pos.y - Y_RADIUS < n.y;
  char zmin = pos.z - Z_RADIUS < n.z;

  char xmax = pos.x + X_RADIUS > n.x;
  char ymax = pos.y + Y_RADIUS > n.y;
  char zmax = pos.z + Z_RADIUS > n.z;

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

#ifdef TEXTURE
__device__ real_t get_intensity_for_pos_full(Coord pos, Node* nodes, Node* node_cache, unsigned int* stack, int base){
#else
__device__ real_t get_intensity_for_pos_full(Coord pos, const __restrict__ Point* points, Node* nodes, Node* node_cache, unsigned int* stack, int base){
#endif

  real_t intensity = 0;
  real_t weight = 0;
  short tos = 0;
  unsigned char inc = 1;

  unsigned int current_node = 0;
  unsigned int b = get_covered_subnodes(pos, root_node);
  stack[base + tos] = current_node | (b << 24);
  tos += inc;

  while(tos > 0){
    current_node = stack[base + tos -inc] & 0x00ffffff;
    b = (stack[base + tos -inc] & 0xff000000) >> 24;

    if(b > 255){
      return 0;
    }

    unsigned int t = 1;
    while((b & t) == 0){
      t = t << 1;
    }
    current_node += (unsigned int)log2((float)t);
    b = b ^ t;

    if(b == 0){
      tos -= inc;
    }

    else{
      stack[base + tos-inc] = (stack[base + tos -inc] & 0x00ffffff) | (b << 24);
    }

    Node n;
    
    if(current_node < NODE_CACHE_SIZE){
      n = node_cache[current_node];
    }
    else{
      n = nodes[current_node];
    }

    if(n.is_leaf){
      for(short c = 0; c < n.num_children; c++){
#ifdef TEXTURE
        float4 point = tex1Dfetch(pointTexture, (long int)n.pointer + c);
#else
        //float4 p = __ldg((const float4*)(&points[(long int)n.pointer + c]));
        //Point point; point.x = p.x; point.y = p.y; point.z = p.z; point.intensity = p.w;
        Point point = points[(long int)n.pointer + c];
#endif

        real_t dx = pos.x - point.x;
        real_t dy = pos.y - point.y;
        real_t dz = pos.z - point.z;

        real_t distance = sqrt(dx*dx*aniso_x + dy*dy*aniso_y + dz*dz*aniso_z);

        if(distance < INTERPOLATION_RADIUS_D){
#ifdef TEXTURE
          intensity += (1/distance)*point.w;
#else
          intensity += (1/distance)*point.intensity;
#endif
          weight += (1/distance);
        }
      }
    }
    else{
      b = get_covered_subnodes(pos, nodes[current_node]);
      unsigned int temp = (unsigned int)nodes[current_node].pointer;
      stack[base + tos] = (temp) | (b << 24);
      tos += inc;
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

__device__ real_t get_intensity_for_pos_d(Coord pos, Node* nodes, Point* points){

  Node n = root_node;
  Ranges ranges = top_ranges;
  real_t d = 0;

  while(n.is_leaf == 0){
    d++;
    int index = get_index_d(pos, ranges);
    ranges = get_ranges_for_index_d(ranges, index);

    n = nodes[((long int)n.pointer) + index];
  }

  return interpolate(n, pos, points);
}

__device__ void create_rays(int index, Ray* rays, real_t pixel_width, int res){

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

#ifdef TEXTURE
__global__ void kernel(Node* nodes, Display_color* image, Ray* rays, Color* colors, unsigned int* stack, int stack_size, real_t pixel_width, int offset, int multigpu){
#else
__global__ void kernel(const __restrict__ Point* points, Node* nodes, Display_color* image, Ray* rays, Color* colors, unsigned int* stack, int stack_size, real_t pixel_width, int offset, int multigpu){
#endif

	/*
	int virtualBlockId = blockIdx.x * 10;
	while(virtualBlockId >= gridDim.x){
		virtualBlockId = virtualBlockId - (gridDim.x -1);
	}
	*/


  int i = (blockIdx.x * blockDim.x + threadIdx.x) + offset;
  
  if(multigpu == 0){
    create_rays(i, rays, pixel_width, RESOLUTION_D);
  }

  __shared__ Node node_cache[NODE_CACHE_SIZE];

  if(threadIdx.x < NODE_CACHE_SIZE){
    node_cache[threadIdx.x] = nodes[threadIdx.x];
  }
  __syncthreads();
  

if(rays[i].distance <= 0){
    Display_color b = {0,0,0,0};
    image[i -offset] = b;
    return;
  }

  rays[i] = normalize_ray(rays[i]);

  Coord pos = rays[i].start;
  real_t acc_distance = 0;
  Color output = {0.0,0.0,0.0,0.0};
  
  real_t local_step_size = STEP_SIZE_D;

  while(acc_distance < rays[i].distance){
#ifdef TEXTURE
    real_t intensity = get_intensity_for_pos_full(pos, nodes, node_cache, stack, i*stack_size);
#else
    real_t intensity = get_intensity_for_pos_full(pos, points, nodes, node_cache, stack, i*stack_size);
#endif
    
    if(intensity > 0 && local_step_size > STEP_SIZE_D){
        acc_distance -= local_step_size;
        pos = pos + (rays[i].dir*(-1*local_step_size));
        local_step_size = STEP_SIZE_D;
    }
    else if(intensity == 0 && (local_step_size * STEP_FACTOR_D) <= STEP_LIMIT_D){
        local_step_size *= STEP_FACTOR_D;
    }
    else{

      output = blend_d(output, intensity, colors);

      if(output.a > 0.99f){
        break;
      }
    }

    pos = pos + (rays[i].dir*local_step_size);
    acc_distance += local_step_size; 
  }

  image[i - offset] = to_display_color_d(output);
}


int getPowerfullness(cudaDeviceProp* p){
	int cudaCores = 0;
	if(p->major == 1){
		cudaCores = 8;
	}
	if(p->major == 2 && p->minor == 0){
		cudaCores = 32;
	}
	if(p->major == 2 && p->minor == 1){
		cudaCores = 48;
	}
	if(p->major == 3){
		cudaCores = 192;
	}

	cudaCores = cudaCores*p->multiProcessorCount;

	return cudaCores*p->clockRate;
}

float logit(float x){
	float y = log(x/(1-x));
	y = y + 5;
	y = y / 10;
	return y;
}

void printError(cudaError_t error, char* message){
    if(error != cudaSuccess){
        printf("%s\n", cudaGetErrorString(error));
        printf("%s\n", message);
    }
}

extern "C" void copy_to_devices_invariant(int deviceCount, Tree* tree, Raycreator* rc){
	for(int device = 0; device < deviceCount; device++){
		cudaSetDevice(device);

		cudaMalloc((void**)&nodes_d[device], sizeof(Node)*tree->node_list->current_pos);
		cudaMalloc((void**)&points_d[device], sizeof(Point)*tree->total_num_points);
		cudaMalloc((void**)&image_d[device], sizeof(Display_color)*RESOLUTION*RESOLUTION);
		cudaMalloc((void**)&rays_d[device], sizeof(Ray)*RESOLUTION*RESOLUTION);
		cudaMalloc((void**)&stack_d[device], sizeof(unsigned int)*RESOLUTION*RESOLUTION*10);//tree->max_depth + 2);
		cudaMalloc((void**)&colors_d[device], sizeof(Color)*transfer_overlay->color_table_size);
    printError(cudaGetLastError(), "Problem mallocing");

		cudaMemcpy(nodes_d[device], tree->node_list->nodes, sizeof(Node)*tree->node_list->current_pos, cudaMemcpyHostToDevice);
		cudaMemcpy(points_d[device], tree->point_list->points, sizeof(Point)*tree->total_num_points, cudaMemcpyHostToDevice);
		cudaMemcpy(colors_d[device], transfer_overlay->color_table, sizeof(Color)*transfer_overlay->color_table_size, cudaMemcpyHostToDevice);
    printError(cudaGetLastError(), "Memcpy");

#ifdef TEXTURE
		cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc(32,32,32,32, cudaChannelFormatKindFloat);
		pointTexture.filterMode = cudaFilterModePoint;
		pointTexture.normalized = false;
		cudaBindTexture(NULL, pointTexture, points_d[device], channelDesc, sizeof(Point)*tree->total_num_points);
    printError(cudaGetLastError(), "texture");
#endif

		cudaMemcpyToSymbol(top_ranges, &tree->ranges, sizeof(Ranges));
		cudaMemcpyToSymbol(root_node, &tree->root, sizeof(Node));
		cudaMemcpyToSymbol(MAX_INTENSITY_D, &MAX_INTENSITY, sizeof(real_t));
		cudaMemcpyToSymbol(RESOLUTION_D, &RESOLUTION, sizeof(int));
    printError(cudaGetLastError(), "memcpy to symbol");
	}
}


void copy_to_devices(int deviceCount, Tree* tree, Raycreator* rc){

	Coord top = {rc->ranges->xmax, rc->ranges->ymax, rc->ranges->zmax};
	Coord bottom = {rc->ranges->xmin, rc->ranges->ymin, rc->ranges->zmin};

    real_t x_radius = INTERPOLATION_RADIUS;
    real_t y_radius = INTERPOLATION_RADIUS;
    real_t z_radius = INTERPOLATION_RADIUS;

    if(ANISOTROPIC){
        x_radius *= (1/sqrt(ANISO_MATRIX[0]));
        y_radius *= (1/sqrt(ANISO_MATRIX[4]));
        z_radius *= (1/sqrt(ANISO_MATRIX[8]));
    }
 		
	for(int device = 0; device < deviceCount; device++){
		cudaSetDevice(device);

		cudaMemcpy(rays_d[device], rays, sizeof(Ray)*RESOLUTION*RESOLUTION, cudaMemcpyHostToDevice);
    printError(cudaGetLastError(), "Memcpy");

		cudaMemcpyToSymbol(STEP_SIZE_D, &STEP_SIZE, sizeof(real_t));
		cudaMemcpyToSymbol(STEP_FACTOR_D, &STEP_FACTOR, sizeof(real_t));
		cudaMemcpyToSymbol(STEP_LIMIT_D, &STEP_LIMIT, sizeof(real_t));
		cudaMemcpyToSymbol(INTERPOLATION_RADIUS_D, &INTERPOLATION_RADIUS, sizeof(real_t));
		cudaMemcpyToSymbol(X_RADIUS, &x_radius, sizeof(real_t));
		cudaMemcpyToSymbol(Y_RADIUS, &y_radius, sizeof(real_t));
		cudaMemcpyToSymbol(Z_RADIUS, &z_radius, sizeof(real_t));
		cudaMemcpyToSymbol(aniso_x, &ANISO_MATRIX[0], sizeof(real_t));
		cudaMemcpyToSymbol(aniso_y, &ANISO_MATRIX[4], sizeof(real_t));
		cudaMemcpyToSymbol(aniso_z, &ANISO_MATRIX[8], sizeof(real_t));
		
		cudaMemcpyToSymbol(up, &rc->up, sizeof(Coord));
		cudaMemcpyToSymbol(top_d, &top, sizeof(Coord));
		cudaMemcpyToSymbol(bottom_d, &bottom, sizeof(Coord));
		cudaMemcpyToSymbol(eye, &rc->eye, sizeof(Coord));
		cudaMemcpyToSymbol(right, &rc->right, sizeof(Coord));
		cudaMemcpyToSymbol(screen_center_d, &rc->screen_center, sizeof(Coord));
		
    printError(cudaGetLastError(), "memcpy to symbol");
	}
}

float* get_work_fractions(int deviceCount, int useFiftyFifty){
	float* workFractions = (float*) malloc(sizeof(float) * deviceCount);
	if(stored_work_fractions[0] != 0){
		for (int d = 0; d < deviceCount; ++d) {
			workFractions[d] = stored_work_fractions[d];
		}
	}
	else{
		if(useFiftyFifty != 0){
			workFractions[0] = 0.5;
			workFractions[1] = 0.5;
		}
		float sum = 0;
		for(int d = 0; d < deviceCount; d++){
			cudaDeviceProp p;
			cudaGetDeviceProperties(&p, d);
			workFractions[d] = getPowerfullness(&p);
			sum += workFractions[d];
		}
		for(int d = 0; d < deviceCount; d++){
			workFractions[d] = workFractions[d]/sum;
		}
	}

	return workFractions;
}

void get_work_fractions_logit(float* workFractions){
	workFractions[0] = logit(workFractions[0]);
	workFractions[1] = 1- workFractions[0];
}

void get_work_fractions_ray_length(float* workFractions){

	real_t totalLength = 0;
	for(int i = 0; i < RESOLUTION*RESOLUTION; i++){
		totalLength += rays[i].distance;
	}

	int i = 0;
	real_t partial_length = 0;
	real_t target_partial_length = workFractions[0] * totalLength;
	while(partial_length < target_partial_length){
		partial_length += rays[i].distance;
		i++;
	}

	workFractions[0] = (real_t)i/(real_t)(RESOLUTION*RESOLUTION);
	workFractions[1] = 1 - workFractions[0];
}

int* get_blocks_pr_device(int deviceCount, float* originalWorkFractions, int nBlocks, int useLogit, int useRayLength){
	float workFractions[deviceCount];
	for (int i = 0; i < deviceCount; ++i) {
		workFractions[i] = originalWorkFractions[i];
	}

	if(useLogit != 0){
		get_work_fractions_logit(workFractions);
	}
	if(useRayLength != 0){
		get_work_fractions_ray_length(workFractions);
	}

	int* blocksPrDevice = (int*)malloc(sizeof(int) * deviceCount);
	for (int d = 0; d < deviceCount; ++d) {
			blocksPrDevice[d] = (int)(workFractions[d] * nBlocks);
			printf("Work for device %d: %f, %d/%d\n", d, workFractions[d], blocksPrDevice[d], nBlocks);
	}

	int sum = 0;
	do{
		sum = 0;
		for (int d = 0; d < deviceCount; ++d) {
			sum += blocksPrDevice[d];
		}
		if(sum < nBlocks){
			blocksPrDevice[0]++;
		}
		else if(sum > nBlocks){
			blocksPrDevice[0]--;
		}
	}while(sum != nBlocks);

	return blocksPrDevice;
}


extern "C" void launch_ray_trace_kernel(void* v, Raycreator* rc){

	Tree* tree = (Tree*)v;

	int deviceCount = 0;
    float* workFractions;
    if(MULTIGPU == 1){
        cudaGetDeviceCount(&deviceCount);

        int useFiftyFifty = 0;
        workFractions = get_work_fractions(deviceCount, useFiftyFifty);
    }
    else{
        deviceCount = 1;
        workFractions = (float*)malloc(sizeof(float)*1);
        workFractions[0] = 1.0;
    }

	printf("Using %d devices\n", deviceCount);

	copy_to_devices(deviceCount, tree, rc);

	int nThreads=128;
	int totalThreads = RESOLUTION*RESOLUTION;
	int nBlocks = totalThreads/nThreads;
	nBlocks += ((totalThreads%nThreads)>0)?1:0;

    int* blocksPrDevice;
    if(MULTIGPU == 1){
        int useLogit = 0;
        int useRayLength = 1;
        blocksPrDevice = get_blocks_pr_device(deviceCount, workFractions, nBlocks, useLogit, useRayLength);
    }
    else{
        blocksPrDevice = (int*)malloc(sizeof(int)*1);
        blocksPrDevice[0] = nBlocks;
    }

	cudaEvent_t startEvents[deviceCount];
	cudaEvent_t endEvents[deviceCount];
	cudaStream_t streams[deviceCount];

	for(int device = 0; device < deviceCount; device++){
		int start = 0;
		if(device > 0){
			start = blocksPrDevice[device -1] * nThreads;
		}
		cudaSetDevice(device);

		cudaEventCreate(&startEvents[device]);
		cudaEventCreate(&endEvents[device]);
		cudaStreamCreate(&streams[device]);
#ifdef TEXTURE
		cudaEventRecord(startEvents[device]);
		kernel<<<blocksPrDevice[device], nThreads,0, streams[device]>>>(nodes_d[device], image_d[device], rays_d[device], colors_d[device], stack_d[device], 10, rc->pixel_width, start, MULTIGPU);//tree->max_depth + 2);
		cudaEventRecord(endEvents[device]);
#else
		cudaEventRecord(startEvents[device]);
		printf("start %d blocks: %d\n", start, blocksPrDevice[device]);
		kernel<<<blocksPrDevice[device], nThreads,0, streams[device]>>>(points_d[device], nodes_d[device], image_d[device], rays_d[device], colors_d[device], stack_d[device], 10, rc->pixel_width, start, MULTIGPU);//tree->max_depth + 2);
		cudaEventRecord(endEvents[device]);

#endif
	}

	float r[deviceCount];
	for (int d = 0; d < deviceCount; ++d) {
		cudaEventSynchronize(endEvents[d]);
		float time;
		cudaEventElapsedTime(&time,startEvents[d], endEvents[d]);
		r[d] = time/workFractions[d];
		printf("Time for device %d: %f\n", d, time);

	}
	float total = r[0] + r[1];
	stored_work_fractions[0] = r[1]/total;
	stored_work_fractions[1] = r[0]/total;


	int previousStart = 0;
	for(int device = 0; device < deviceCount; device++){
		cudaSetDevice(device);

		int images_index = log2((real_t)RESOLUTION) - 4;

		int offset = previousStart;
		previousStart = offset + blocksPrDevice[device]*nThreads;

		cudaMemcpy(&images[images_index][offset], image_d[device], sizeof(Display_color)*blocksPrDevice[device]*nThreads, cudaMemcpyDeviceToHost);
	}

    //freeAndReset(deviceCount);
}

extern "C" void freeAndReset(){
    int deviceCount = 0;
    if(MULTIGPU == 1){
        cudaGetDeviceCount(&deviceCount);
    }
    else{
        deviceCount = 1;
    }
    for(int device = 0; device < deviceCount; device++){
        cudaSetDevice(device);

        cudaFree(nodes_d[device]);
        cudaFree(points_d[device]);
        cudaFree(image_d[device]);
        cudaFree(rays_d[device]);
        cudaFree(colors_d[device]);
        cudaFree(stack_d[device]);
        cudaDeviceReset();
        printError(cudaGetLastError(), "Free");
    }
}

extern "C" void initDevice(Tree* tree, Raycreator* rc){
    int deviceCount = 0;
    if(MULTIGPU == 1){
        cudaGetDeviceCount(&deviceCount);
    }
    else{
        deviceCount = 1;
    }
    copy_to_devices_invariant(deviceCount, tree, rc);
}

extern "C" long int get_memory_size(){
  cudaDeviceProp devProp;
  cudaGetDeviceProperties(&devProp, 0);
  return devProp.totalGlobalMem;
}
