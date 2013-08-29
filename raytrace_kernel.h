// Copyright (c) 2013, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is a part of the Scattered Point Visualization application
// developed at the Norwegian University of Science and Technology


#ifndef RAYTRACE_KERNEL
#define RAYTRACE_KERNEL

void launch_ray_trace_kernel(void* v, Raycreator* rc);
long int get_memory_size();
void freeAndReset();
void copy_to_devices_invariant(int deviceCount, Tree* tree, Raycreator* rc);
void initDevice(Tree* tree, Raycreator* rc);

#endif
